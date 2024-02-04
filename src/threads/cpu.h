#ifndef THREADS_CPU_H_
#define THREADS_CPU_H_

#include "threads/tss.h"
#include "threads/gdt.h"
#include "threads/scheduler.h"
#include <stdint.h>
#include "threads/interrupt.h"

#define NCPU_MAX 8      /* Max number of cpus */

/* Per-CPU state */
struct cpu
{
  uint8_t id;               /* Local APIC ID; index into cpus[] below */
  struct tss ts;            /* Per-process TSS */
  uint64_t gdt[SEL_CNT];    /* Per-process GDT */
  int started;              /* Has this CPU been started? */
  
  /* State of interrupts. Owned by interrupt.c */
  int ncli;                 /* Depth of pushcli nesting. */
  int intena;               /* Were interrupts enabled before pushcli? */
  bool in_external_intr;    /* Are we processing an external interrupt? */

  /* Should we yield when interrupts are reenabled?
     In an interrupt context, this occurs upon return from the interrupt.
     In a thread context, this occurs as soon as interrupts are reenabled. */
  bool yield_on_return;

  /* Statistics. Owned by thread.c */
  uint64_t idle_ticks;
  uint64_t user_ticks;
  uint64_t kernel_ticks;
  uint64_t cs;          /* Number of context switches */
  
  /* Ready queue. Owned by scheduler.c */
  struct ready_queue rq;
  
  /* Cpu-local storage variable; see below */
  struct cpu *cpu;
};

/* Some information about the system */

/* Is it safe for the CPUs to acquire a spinlock? 0 during initial boot
   process, since each CPU needs the GDT set up to acquire a spinlock.
   Afterwards, it is set to 1 */
extern int cpu_can_acquire_spinlock;      
extern struct cpu *bcpu;               /* Pointer to the BSP */
extern struct cpu cpus[NCPU_MAX];      /* Array holding per-CPU states */
extern int cpu_ismp;            /* 1 if hardware supports SMP. */
extern unsigned int ncpu;       /* Number of cpus on this machine. */
extern int cpu_started_others;  /* 1 if the application processors have been started. */

/* Per-CPU variables, holding pointers to the
   current cpu.
   GDT sets up %gs segment register so that %gs:0 refers to the 
   memory containing a pointer to local cpu's struct cpu.
   This is similar to how thread-local variables are implemented
   in thread libraries such as Linux pthreads. */
static inline struct cpu *
get_cpu (void)
{
  /* Calling get_cpu () without first disabling interrupts creates a
     race condition in the presence of a load balancing regime. */
  ASSERT (intr_get_level () == INTR_OFF);
  struct cpu *ret;
  asm volatile("mov %%gs:0 ,%0" : "=r" (ret));
  return ret;
}

/* Called by GDT module
   gdt_init asks the lapic to get the id of the
   current CPU, then puts the address of 
   cpus[id] into the %gs register. */
static inline void
set_cpu (struct cpu *cpu)
{
  asm ("mov %0, %%gs:0" : : "r" (cpu));
}

#endif /* THREADS_CPU_H_ */
