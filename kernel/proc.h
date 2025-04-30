#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include "types.h"
#include "mmu.h"
#include "param.h"
#include "shm.h"        // for SHM_MAX_DESCS / struct shmref

// ---------------------------------------------------------------------
// Per-CPU state
// ---------------------------------------------------------------------
struct cpu {
	uchar             apicid;      // Local APIC ID
	struct context   *scheduler;   // swtch() here to enter scheduler
	struct taskstate  ts;          // Used by x86 to find stack for interrupt
	segdesc           gdt[NSEGS];  // x86 global descriptor table
	volatile uint      started;    // Has the CPU started?
	int                ncli;       // Depth of pushcli nesting
	int                intena;     // Were interrupts enabled before pushcli?
	struct proc       *proc;       // The process running on this CPU or 0
};

extern struct cpu cpus[NCPU];
extern int        ncpu;

// ---------------------------------------------------------------------
// Context for kernel context-switch (matches swtch.S layout)
// ---------------------------------------------------------------------
struct context {
	uint edi;
	uint esi;
	uint ebx;
	uint ebp;
	uint eip;
};

// ---------------------------------------------------------------------
// Process states
// ---------------------------------------------------------------------
enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// ---------------------------------------------------------------------
// Per-process state
// ---------------------------------------------------------------------
struct proc {
	uint                 sz;                 // Size of process memory (bytes)
	pde_t               *pgdir;              // Page table
	char                *kstack;             // Bottom of kernel stack
	enum procstate       state;              // Process state
	int                  pid;                // Process ID
	struct proc         *parent;             // Parent process
	struct trapframe    *tf;                 // Trap frame for current syscall
	struct context      *context;            // swtch() here to run process
	void                *chan;               // If non-zero, sleeping on chan
	int                  killed;             // If non-zero, have been killed
	struct file         *ofile[NOFILE];      // Open files
	struct inode        *cwd;               // Current directory
	char                 name[16];           // Process name (debugging)

	// -------- shared-memory descriptors --------
	struct shmref        shm[SHM_MAX_DESCS]; // Up to 16 open shm objects
};

// ---------------------------------------------------------------------
// Memory layout (user space):
//   text | data+ bss | fixed stack | expandable heap
// ---------------------------------------------------------------------

#endif  // KERNEL_PROC_H
