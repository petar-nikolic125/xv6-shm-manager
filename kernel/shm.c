// Shared-memory implementation for xv6-x86
// ---------------------------------------

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fcntl.h"
#include "string.h"
#include "shm.h"

extern pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);


// ---------------------------------------------------------------------
// Minimal local copies of VM helpers (vm.c versions are static)
// ---------------------------------------------------------------------
static int
shm_mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
    char *a = (char *)PGROUNDDOWN((uint)va);
    char *last = (char *)PGROUNDDOWN(((uint)va) + size - 1);

    for (;; a += PGSIZE, pa += PGSIZE) {
        pte_t *pte = walkpgdir(pgdir, a, 1);
        if (pte == 0)
            return -1;
        if (*pte & PTE_P)
            return -1;
        *pte = pa | perm | PTE_P;
        if (a == last)
            break;
    }
    return 0;
}

static void
shm_uvmunmap(pde_t *pgdir, void *va, int n, int do_free)
{
    char *a = (char *)PGROUNDDOWN((uint)va);
    for (int i = 0; i < n; i++, a += PGSIZE) {
        pte_t *pte = walkpgdir(pgdir, a, 0);
        if (pte == 0 || !(*pte & PTE_P))
            continue;
        if (do_free) {
            uint pa = PTE_ADDR(*pte);
            char *v = P2V(pa);
            kfree(v);
        }
        *pte = 0;
    }
}

// ---------------------------------------------------------------------
// Global table
// ---------------------------------------------------------------------
static struct shmobj shm_tab[SHM_MAX_OBJ];
static struct spinlock shm_tab_lk;

// return table index (used for canonical VA calculation)
static inline int obj_index(struct shmobj *o) { return (int)(o - shm_tab); }

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------
// look up a shared-memory object by name (shm_tab_lk must be held)
static struct shmobj *
shm_find(const char *name)
{
    for (int i = 0; i < SHM_MAX_OBJ; i++) {
        if (shm_tab[i].used &&
            strncmp(name, shm_tab[i].name, sizeof(shm_tab[i].name)) == 0) {
            return &shm_tab[i];
            }
    }
    return 0;
}


static struct shmobj *shm_alloc_obj(const char *name)     /* shm_tab_lk held */
{
    for (int i = 0; i < SHM_MAX_OBJ; i++) {
        if (!shm_tab[i].used) {
            struct shmobj *o = &shm_tab[i];
            o->used = 1;
            o->refcnt = 0;
            o->size = 0;
            o->npages = 0;
            o->va = 0;
            memset(o->pages, 0, sizeof(o->pages));
            safestrcpy(o->name, name, sizeof(o->name));
            initlock(&o->lock, "shmobj");
            return o;
        }
    }
    return 0;
}

static int shm_alloc_desc(struct proc *p, struct shmobj *o)
{
    for (int i = 0; i < SHM_MAX_DESCS; i++) {
        if (p->shm[i].obj == 0) {
            p->shm[i].obj = o;
            p->shm[i].flags = 0;
            p->shm[i].va = 0;
            o->refcnt++;
            return i;
        }
    }
    return -1;
}

static void shm_free_obj(struct shmobj *o)                /* o->lock held */
{
    for (int i = 0; i < o->npages; i++)
        if (o->pages[i])
            kfree(o->pages[i]);
    o->used = o->size = o->npages = o->va = 0;
}

// ---------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------
void shm_init(void) { initlock(&shm_tab_lk, "shm_table"); }

// ---------------------------------------------------------------------
// syscalls
// ---------------------------------------------------------------------
int sys_shm_open(void)
{
    char *up;
    char  kname[64];

    if (argstr(0, &up) < 0)
        return -1;
    safestrcpy(kname, up, sizeof(kname));

    struct proc *p = myproc();

    acquire(&shm_tab_lk);
    struct shmobj *o = shm_find(kname);
    if (!o && (o = shm_alloc_obj(kname)) == 0) {
        release(&shm_tab_lk);
        return -1;
    }
    acquire(&o->lock);
    int fd = shm_alloc_desc(p, o);
    release(&o->lock);
    release(&shm_tab_lk);
    return fd;
}

int sys_shm_trunc(void)
{
    int fd, size;
    if (argint(0, &fd) < 0 || argint(1, &size) < 0)
        return -1;

    struct proc *p = myproc();
    if (fd < 0 || fd >= SHM_MAX_DESCS || p->shm[fd].obj == 0)
        return -1;
    if (size <= 0 || size > SHM_MAX_PAGES * PGSIZE)
        return -1;

    struct shmobj *o = p->shm[fd].obj;
    acquire(&o->lock);
    if (o->size) { int old=o->size; release(&o->lock); return old; }

    int np = (size + PGSIZE - 1) / PGSIZE;
    if (np > SHM_MAX_PAGES) { release(&o->lock); return -1; }

    for (int i = 0; i < np; i++) {
        void *pa = kalloc();
        if (!pa) {                          // rollback
            for (int j = 0; j < i; j++) kfree(o->pages[j]);
            release(&o->lock);
            return -1;
        }
        memset(pa, 0, PGSIZE);
        o->pages[i] = pa;
    }
    o->npages = np;
    o->size   = np * PGSIZE;
    release(&o->lock);
    return o->size;
}

int sys_shm_map(void)
{
    int  fd, flags;
    uint uva_ptr;
    if (argint(0,&fd)<0 || argint(1,(int*)&uva_ptr)<0 || argint(2,&flags)<0)
        return -1;

    struct proc *p = myproc();
    if (fd<0 || fd>=SHM_MAX_DESCS || p->shm[fd].obj==0) return -1;
    if ((flags&O_WRONLY) && !(flags&O_RDWR))              return -1;

    struct shmref *d = &p->shm[fd];
    if (d->va) return -1;

    struct shmobj *o = d->obj;
    acquire(&o->lock);
    if (o->size == 0) { release(&o->lock); return -1; }

    if (o->va == 0)
        o->va = SHMBASE + obj_index(o) * SHM_SLOTSZ;
    uint base = o->va;

    int perm = PTE_U | PTE_SH | ((flags & O_RDWR) ? PTE_W : 0);

    for (int i = 0; i < o->npages; i++) {
        if (shm_mappages(p->pgdir,
            (void *)(base + i * PGSIZE),
                         PGSIZE,
                         V2P(o->pages[i]),
                         perm) != 0) {
            shm_uvmunmap(p->pgdir, (void *)base, i, 0);
        release(&o->lock);
        return -1;
                         }
    }
    d->flags = flags;
    d->va    = base;
    release(&o->lock);
    if (copyout(p->pgdir, uva_ptr, (char *)&base, sizeof(base)) < 0)
        return -1;
    return 0;
}

// ---------------------------------------------------------------------
// close helper (also used by exit())
// ---------------------------------------------------------------------
void shm_close_desc(struct proc *p, int fd)
{
    if (fd < 0 || fd >= SHM_MAX_DESCS) return;

    struct shmref *d = &p->shm[fd];
    if (d->obj == 0) return;

    struct shmobj *o = d->obj;
    if (d->va) shm_uvmunmap(p->pgdir, (void *)(uintptr_t)d->va, o->npages, 0);

    d->obj = 0; d->flags = 0; d->va = 0;

    acquire(&o->lock);
    o->refcnt--;
    int drop = (o->refcnt == 0);
    release(&o->lock);

    if (drop) {
        acquire(&shm_tab_lk);
        acquire(&o->lock);
        if (o->refcnt == 0) shm_free_obj(o);
        release(&o->lock);
        release(&shm_tab_lk);
    }
}

int sys_shm_close(void)
{
    int fd; if (argint(0,&fd)<0) return -1;
    shm_close_desc(myproc(), fd);
    return 0;
}
