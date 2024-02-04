#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/switch.h"
#include "threads/spinlock.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "threads/scheduler.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif
#include "threads/cpu.h"
#include "threads/mp.h"
#include "devices/lapic.h"
#include "lib/kernel/x86.h"
#include <atomic-ops.h>
#include "lib/kernel/bitmap.h"
#include "threads/ipi.h"

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;
static struct spinlock all_lock;
/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame
{
  void *eip; /* Return address. */
  thread_func *function; /* Function to call. */
  void *aux; /* Auxiliary data for function. */
};

static void kernel_thread_entry (thread_func *, void *aux);
static void idle (void *aux UNUSED);
static struct thread *next_thread_to_run (void);
static bool is_thread (struct thread *) UNUSED;
static void do_thread_exit (void) NO_RETURN;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);
static struct thread *do_thread_create (const char *, int, thread_func *, void *);
static void init_boot_thread (struct thread *boot_thread, struct cpu *cpu);
static void init_thread (struct thread *t, const char *name, int nice);
static void lock_own_ready_queue (void);
static void unlock_own_ready_queue (void);

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void)
{
  list_init (&all_list);
  spinlock_init (&all_lock);
  ASSERT (intr_get_level () == INTR_OFF);
  sched_init (&bcpu->rq);
  spinlock_init (&bcpu->rq.lock);
  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_boot_thread (initial_thread, bcpu);
}

/*
 * Create the initial thread on an application processor/CPU.
 * Called once for each AP.  Initializes the run queue on that CPU.
 */
void
thread_init_on_ap (void)
{
  ASSERT (intr_get_level () == INTR_OFF);
  struct cpu *cpu = &cpus[lapic_get_cpuid ()];
  ASSERT(cpu != NULL);
  sched_init (&cpu->rq);
  spinlock_init (&cpu->rq.lock);
  struct thread *cur_thread = running_thread ();
  init_boot_thread (cur_thread, cpu);
}

/* Creates the idle thread on this CPU. */
void
thread_start_idle_thread (void)
{
  ASSERT (intr_get_level () == INTR_OFF);
  /* Create name for idle thread that indicates which CPU it is on */
  char idle_name[THREAD_NAME_MAX];
  snprintf (idle_name, THREAD_NAME_MAX, "idle_cpu%"PRIu8, get_cpu ()->id);

  /* Create the idle thread. */
  struct thread *idle_thread = do_thread_create (idle_name, NICE_MAX, idle, NULL);
  ASSERT (idle_thread);
  idle_thread->cpu = get_cpu ();
  get_cpu ()->rq.idle_thread = idle_thread;
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void)
{
  ASSERT (intr_get_level () == INTR_OFF);
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == get_cpu ()->rq.idle_thread)
    get_cpu ()->idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    get_cpu ()->user_ticks++;
#endif
  else
    get_cpu ()->kernel_ticks++;

  lock_own_ready_queue ();
  enum sched_return_action ret_action = sched_tick (&get_cpu ()->rq, t);
  if (ret_action == RETURN_YIELD)
    {
      /* We are processing an external interrupt, so we cannot yield
         right now. Instead, set a flag to yield at the end of the
         interrupt. */
      intr_yield_on_return ();
    }
  unlock_own_ready_queue ();
}

/* Prints thread statistics. */
void
thread_print_stats (void)
{
  struct cpu *c;
  for (c = cpus; c < cpus + ncpu; c++)
    {
      printf (
          "CPU%d: %llu idle ticks, %llu kernel ticks, %llu user ticks, %llu context switches\n",
          c->id, c->idle_ticks, c->kernel_ticks, c->user_ticks, c->cs);
    }
}

/* The default policy for choosing to which CPU to assign a
 * new thread is a round-robin policy.
 */
static struct cpu *
choose_cpu_for_new_thread (struct thread *t)
{
  return &cpus[t->tid % ncpu];
}

static void
wake_up_new_thread (struct thread *t)
{
  t->status = THREAD_READY;
  t->cpu = choose_cpu_for_new_thread (t);
  spinlock_acquire (&t->cpu->rq.lock);
  sched_unblock (&t->cpu->rq, t, 1);
  spinlock_release (&t->cpu->rq.lock);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument.
   Returns a pointer to the new thread's struct thread, or NULL
   if creation fails. */
static struct thread *
do_thread_create (const char *name, int nice, thread_func *function, void *aux)
{
    struct thread *t;
    struct kernel_thread_frame *kf;
    struct switch_entry_frame *ef;
    struct switch_threads_frame *sf;
    ASSERT (function != NULL);

    /* Allocate thread. */
    t = palloc_get_page (PAL_ZERO);
    if (t == NULL)
      return NULL;

    /* Initialize thread. */
    init_thread (t, name, nice);
    t->tid = allocate_tid ();

    /* Stack frame for kernel_thread(). */
    kf = alloc_frame (t, sizeof *kf);
    kf->eip = NULL;
    kf->function = function;
    kf->aux = aux;

    /* Stack frame for switch_entry(). */
    ef = alloc_frame (t, sizeof *ef);
    ef->eip = (void (*) (void)) kernel_thread_entry;

    /* Stack frame for switch_threads(). */
    sf = alloc_frame (t, sizeof *sf);
    sf->eip = switch_entry;
    sf->ebp = 0;

    return t;
}

/* Creates a new thread and adds it to the ready queue.

   Returns the thread identifier for the new thread, or TID_ERROR
   if creation fails.

   Except during system startup, the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's 'nice' member to
   nice, but it's not actually used. You will implement it as part of
   Project 1. */
tid_t
thread_create (const char *name, int nice, thread_func *function, void *aux)
{
  struct thread *t;

  t = do_thread_create(name, nice, function, aux);
  if (t == NULL)
    return TID_ERROR;

  /* Must save tid here - 't' could already be freed when we return 
     from wake_up_new_thread */ 
  tid_t tid = t->tid;
  /* Add to ready queue. */
  wake_up_new_thread (t);
  return tid;
}

/* Puts the current thread to sleep (i.e., in the BLOCKED state).
   It will not be scheduled again until awoken by thread_unblock().

   This function must be called with interrupts on the current
   CPU turned off. It may not be called from an interrupt context.

   This function is usually called with a held spinlock to avoid
   lost wakeups.  If provided, this spinlock will be released
   before the thread is switched out, and reacquired upon return.

   This function is primarily used inside the threading system.
   For most other tasks, it is usually a better idea to use one
   of the synchronization primitives in synch.h. */
void
thread_block (struct spinlock *lk)
{
  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (!intr_context ());

  lock_own_ready_queue ();
  struct thread *curr = thread_current ();
  if (lk != NULL)
    spinlock_release (lk);

  curr->status = THREAD_BLOCKED;
  sched_block (&get_cpu ()->rq, curr);
  schedule ();
  unlock_own_ready_queue ();

  if (lk != NULL)
    spinlock_acquire (lk);
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t)
{
  ASSERT (is_thread (t));
  ASSERT (t->cpu != NULL);
  spinlock_acquire (&t->cpu->rq.lock);
  ASSERT (t->status == THREAD_BLOCKED);
  t->status = THREAD_READY;
  enum sched_return_action ret_action = sched_unblock (&t->cpu->rq, t, 0);

  if (ret_action == RETURN_YIELD)
    {
      if (t->cpu == get_cpu ())
        /* Make a note that the scheduler requested yielding
           the CPU at the earliest opportunity. */
        intr_yield_on_return ();
      else
        /* Send an inter-processor interrupt to instruct the CPU
           responsible for running thread t to preempt. */
        lapic_send_ipi_to(IPI_SCHEDULE, t->cpu->id);
    }
  spinlock_release (&t->cpu->rq.lock);
}

/* Returns the name of the running thread. */
const char *
thread_name (void)
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void)
{
  struct thread *t = running_thread ();

  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);
//  TODO: ASSERT (t->cpu == get_cpu ());

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void)
{
  return thread_current ()->tid;
}

/* Lock our ready queue from a context in which interrupts (and thus preemption)
 * may be enabled.  To ensure that the CPU obtained by get_cpu() is not stale
 * by the time we use it, we first disable preemption.  Otherwise, we may be
 * preempted and possibly migrated to another CPU once load balancing is 
 * implemented.
 */
static void
lock_own_ready_queue (void)
{
  intr_disable_push ();
  spinlock_acquire (&get_cpu ()->rq.lock);
  intr_enable_pop ();
}

static void
unlock_own_ready_queue (void)
{
  spinlock_release (&get_cpu ()->rq.lock);
}

static void
do_thread_exit (void)
{
  struct thread * cur = thread_current ();

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  ASSERT (cpu_can_acquire_spinlock);

  spinlock_acquire (&all_lock);
  list_remove (&cur->allelem);
  spinlock_release (&all_lock);

  lock_own_ready_queue ();
  cur->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void)
{
  ASSERT(!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  do_thread_exit ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void)
{
  struct thread *cur = thread_current ();
  ASSERT (!intr_context ());

  lock_own_ready_queue ();

  cur->status = THREAD_READY;
  if (cur != get_cpu ()->rq.idle_thread)
    {
      sched_yield (&get_cpu ()->rq, cur);
    }
  schedule ();
  unlock_own_ready_queue ();
}

/* Called from ap_main to terminate an AP's main thread.  */
void
thread_exit_ap (void)
{
  do_thread_exit ();
}

/* Invoke function 'func' on all threads, passing along 'aux'. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;
  spinlock_acquire (&all_lock);
  for (e = list_begin (&all_list); e != list_end (&all_list); e = list_next (e))
    {
      struct thread *t = list_entry(e, struct thread, allelem);
      func (t, aux);
    }
  spinlock_release (&all_lock);
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice)
{
  thread_current ()->nice = nice;
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void)
{
  return thread_current ()->nice;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED)
{
  for (;;)
    {
      /* Let someone else run. */

      intr_disable ();

      /* Insert load balancing code here!
       * An CPU should not go idle if there are ready threads
       * in other CPUs' ready queues that are not running.
       *
       * The baseline implementation does not ensure this.
       */
      thread_block (NULL);

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* A new thread's very first scheduling by scheduler () returns here.
 * Called with current CPU's ready queue lock held.
 */
static void
kernel_thread_entry (thread_func *function, void *aux)
{
  ASSERT (function != NULL);
  ASSERT (intr_get_level () == INTR_OFF);

  /* Set intena to 1, so a call to popcli () will enable interrupts */
  get_cpu ()->intena = 1;
  /*
   * At this point, the only lock that should be held
   * is the rq spinlock acquired by the previous thread
   */
  ASSERT (get_cpu ()->ncli == 1);
  unlock_own_ready_queue ();
  ASSERT (intr_get_level () == INTR_ON);
  function (aux); /* Execute the thread function. */
  thread_exit (); /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void)
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Initialize the boot_thread as a Pintos thread. Boot threads
 * are special. They are not created with thread_create, but
 * rather they are threads created during the boot process
 * so that it can initialize the CPU. The boot thread for the
 * BSP is allocated by the boot loader, while the boot thread
 * for the AP's are allocated in start_other_cpus ().
 */
static void
init_boot_thread (struct thread *boot_thread, struct cpu *cpu)
{
  init_thread (boot_thread, "initial", NICE_DEFAULT);
  boot_thread->status = THREAD_RUNNING;
  boot_thread->tid = allocate_tid ();
  boot_thread->cpu = cpu;
  cpu->rq.curr = boot_thread;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int nice)
{
  ASSERT (t != NULL);
  ASSERT (NICE_MIN <= nice && nice <= NICE_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->nice = nice;
  t->magic = THREAD_MAGIC;
  if (cpu_can_acquire_spinlock)
    spinlock_acquire (&all_lock);
  list_push_back (&all_list, &t->allelem);
  if (cpu_can_acquire_spinlock)
    spinlock_release (&all_lock);
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size)
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof(uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   this CPU's idle_thread. */
static struct thread *
next_thread_to_run (void)
{
  struct thread *ret = NULL;
  ret = sched_pick_next (&get_cpu ()->rq);
  if (!ret)
      ret = get_cpu ()->rq.idle_thread;

  return ret;
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();

  ASSERT(intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread)
    {
      ASSERT(prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, the current CPU's ready queue
   must have been locked, and the running process's state must have been
   changed from running to some other state.  This function finds another
   thread to run and switches to it. */
static void
schedule (void)
{
  /* Schedule must be called with current CPU's ready queue lock held */
  ASSERT (spinlock_held_by_current_cpu (&get_cpu ()->rq.lock));

  /* Must not hold any other spinlocks, since interrupt handlers might
     attempt to acquire them, leading to deadlock since the outgoing
     thread would not be able to release them.
   */
  ASSERT (get_cpu ()->ncli == 1);

  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;
  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));
  int intena = get_cpu ()->intena;      /* Save current value of intena. */
  if (cur != next)
    {
      get_cpu ()->cs++;
      get_cpu ()->rq.curr = next == get_cpu ()->rq.idle_thread ? NULL : next;
      prev = switch_threads (cur, next);
    }

  get_cpu ()->intena = intena;          /* Restore value of intena. */
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void)
{
  static tid_t next_tid = 0;
  return atomic_inci (&next_tid);
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof(struct thread, stack);
