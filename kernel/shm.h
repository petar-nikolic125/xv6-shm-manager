// Shared-memory support for xv6
// --------------------------------------------------

#ifndef XV6_SHM_H
#define XV6_SHM_H

#include "param.h"
#include "types.h"
#include "spinlock.h"

// ---------------------------------------------------------------------
// Tunable limits (from the specification)
// ---------------------------------------------------------------------
#define SHM_MAX_OBJ      64          // system-wide objects
#define SHM_MAX_PAGES    32          // pages per object
#define SHM_MAX_DESCS    16          // open descriptors per process

#define SHMBASE          0x40000000  // first virtual address for shm slots
#define SHM_SLOTSZ       (SHM_MAX_PAGES * PGSIZE)   // size of one VA slot

// Software flag in PTE marking a shared-memory page (bit 9 unused in Sv39)
#define PTE_SH           (1L << 9)

// ---------------------------------------------------------------------
// Kernel data structures
// ---------------------------------------------------------------------

// forward declaration
struct proc;
struct shmobj;

/* Per-process descriptor */
struct shmref {
    struct shmobj *obj;   // NULL if slot unused
    int             flags; // O_RDONLY / O_RDWR
    uint64          va;    // mapped virtual base (0 if not mapped)
};

/* System-wide shared-memory object */
struct shmobj {
    struct spinlock lock;                // protects this object
    int     used;                        // slot active
    char    name[64];                    // NUL-terminated identifier
    int     refcnt;                      // open descriptors across system
    int     size;                        // bytes (0 until first trunc)
    int     npages;                      // cached page count
    void   *pages[SHM_MAX_PAGES];        // physical pages
    uint64  va;                          // canonical mapping VA
};

// ---------------------------------------------------------------------
// Public / kernel-internal API
// ---------------------------------------------------------------------
void shm_init(void);                // called once from main()

int  sys_shm_open(void);
int  sys_shm_trunc(void);
int  sys_shm_map(void);
int  sys_shm_close(void);

/* Internal helper used by both sys_shm_close() and exit() */
void shm_close_desc(struct proc *p, int fd);

#endif  /* XV6_SHM_H */
