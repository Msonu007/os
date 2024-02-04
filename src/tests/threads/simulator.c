#include <stdio.h>
#include "threads/thread.h"
#include "threads/cpu.h"
#include <stdio.h>
#include "threads/scheduler.h"
#include "threads/cpu.h"
#include <stdlib.h>
#include <debug.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "simulator.h"
#include <atomic-ops.h>

#define THREAD_MAGIC 0xcd6abf4b

static bool is_thread (struct thread *);
static void driver_schedule (void);
static struct thread *do_driver_create (const char *name, int nice);
static tid_t allocate_tid (void);
static void driver_init_thread (struct thread *, const char *, int);

/* Returns a pointer to the simulator's idle thread */
struct thread *
driver_idle ()
{
  return get_cpu ()->rq.idle_thread;
}

/* This simulates thread_current ().
   However, the current running Pintos thread is the one that
   is running this code, and not the threads that are being 
   simulated. 
   Therefore, the simulator does its own bookkeeping of what
   is the "current" thread. 
   It checks get_cpu ()->rq.curr which is set
   when after each scheduling decision is made*/
struct thread *
driver_current ()
{
  struct thread *t = get_cpu ()->rq.curr ? : get_cpu ()->rq.idle_thread;

  ASSERT(is_thread (t));

  return t;
}

/* Combines thread_init and thread_start
 * Thread_init and thread_start creates the initial and idle thread, respectively
 */
void
driver_init (void)
{
  struct cpu *c = get_cpu ();
  ASSERT(c != NULL);
  sched_init (&c->rq);
  struct thread *initial_thread = palloc_get_page (PAL_ZERO);
  driver_init_thread (initial_thread, "initial", NICE_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->cpu = get_cpu ();
  initial_thread->tid = allocate_tid();
  get_cpu ()->rq.curr = initial_thread;

  struct thread *idle = do_driver_create  ("idle_driver", NICE_MAX);
  get_cpu ()->rq.idle_thread = idle;

}

/* This simulates thread_tick, defined in thread.c.
   No changes were made.*/
void
driver_tick (void)
{
  struct thread *t = driver_current ();

  /* Update statistics. */
  if (t == get_cpu ()->rq.idle_thread)
    get_cpu ()->idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    get_cpu ()->user_ticks++;
#endif
  else
    get_cpu ()->kernel_ticks++;

  spinlock_acquire (&get_cpu ()->rq.lock);
  enum sched_return_action ret_action = sched_tick (&get_cpu ()->rq, t);
  if (ret_action == RETURN_YIELD)
    {
      /* We are processing an external interrupt, so we cannot yield
         right now. Instead, set a flag to yield at the end of the
         interrupt. */
      intr_yield_on_return ();
    }
  spinlock_release (&get_cpu ()->rq.lock);
}

/* This simulates thread_block, defined in thread.c
   No spinlock is passed in, assume lk == NULL. */
void
driver_block ()
{
  ASSERT(!intr_context ());
  ASSERT(intr_get_level () == INTR_OFF);
  spinlock_acquire (&get_cpu ()->rq.lock);
  struct thread *curr = driver_current ();
  curr->status = THREAD_BLOCKED;
  sched_block (&get_cpu ()->rq, curr);
  driver_schedule ();
  spinlock_release (&get_cpu ()->rq.lock);
}

/* This simulates thread_yield, defined in thread.c
   No changes were made.*/
void
driver_yield (void)
{
  struct thread *cur = driver_current ();
  ASSERT(!intr_context ());
  spinlock_acquire (&get_cpu ()->rq.lock);
  cur->status = THREAD_READY;
  if (cur != get_cpu ()->rq.idle_thread)
    {
      sched_yield (&get_cpu ()->rq, cur);
    }
  driver_schedule ();
  spinlock_release (&get_cpu ()->rq.lock);
}

/* Simulates wake_up_new_thread in thread.c
   Thread is put on the "fake CPU". */
static void
wake_up_new_thread (struct thread *t)
{
  t->cpu = get_cpu ();
  spinlock_acquire (&t->cpu->rq.lock);
  t->status = THREAD_READY;
  sched_unblock (&t->cpu->rq, t, 1);
  spinlock_release (&t->cpu->rq.lock);

}

/* Simulates do_thread_create in thread.c.
   Does not actually initialize a runnable thread, just sets a name and name
   for the thread. The nice is all the driver_scheduler should care about, 
   name is useful for debugging purposes */
static struct thread *
do_driver_create (const char *name, int nice) 
{
  struct thread *t;
  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return NULL;
  
  /* Initialize thread. */
  driver_init_thread (t, name, nice);
  t->tid = allocate_tid ();
  
  return t;
}

/* Simulates thread_create in thread.c */
struct thread *
driver_create (const char *name, int nice)
{
  struct thread *t = do_driver_create (name, nice);

  /* Add to run queue. */
  wake_up_new_thread (t);
  return t;
}

/* Simulates thread_unblock in thread.c. There is only one CPU in the
   simulation, so there is no need to check which CPU t got placed on. */
void
driver_unblock (struct thread *t)
{
  ASSERT(is_thread (t));
  ASSERT(t->cpu != NULL);
  ASSERT(t->status == THREAD_BLOCKED);
  bool yield_on_return = false;
  spinlock_acquire (&t->cpu->rq.lock);
  t->status = THREAD_READY;
  enum sched_return_action ret_action = sched_unblock (&t->cpu->rq, t, 0);

  if (ret_action == RETURN_YIELD)
    {
      /* In the simulator, there is only one CPU */
      ASSERT (t->cpu == get_cpu ());
      yield_on_return = true;
    }
  spinlock_release (&t->cpu->rq.lock);
  if (yield_on_return)
    driver_yield ();
}

/* This simulates thread_exit in thread.c. */
void
driver_exit (void)
{
  ASSERT(!intr_context ());
  struct thread * cur = driver_current ();

  spinlock_acquire (&get_cpu ()->rq.lock);
  cur->status = THREAD_DYING;
  driver_schedule ();
  
  /* In a real scheduler, we would never return here.
     However, in the simulator, driver_schedule () is just
     a normal function call, so we still release 
     the rq lock. */
  spinlock_release (&get_cpu ()->rq.lock);
}

/* Sets the current thread's nice value to NICE. */
void
driver_set_nice (int nice)
{
  driver_current ()->nice = nice;
}

/* Returns the current thread's nice value. */
int
driver_get_nice (void)
{
  return driver_current ()->nice;
}

/* No changes */
static struct thread *
next_thread_to_run (void)
{

  struct thread *ret = NULL;
   ret = sched_pick_next (&get_cpu ()->rq);
   if (!ret)
       ret = get_cpu ()->rq.idle_thread;
   return ret;
}

/* This simulates thread_schedule_tail.
   Took out process_activate () since we do not
   run the threads. */
static void
driver_schedule_tail (struct thread *prev)
{
  struct thread *cur = driver_current ();

  ASSERT(intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  if (prev != NULL && prev->status == THREAD_DYING)
    {
      ASSERT(prev != cur);
      palloc_free_page (prev);
    }
}

/* This simulates schedule () in thread.c with
   the notable difference that no context switching 
   actually takes place */
static void
driver_schedule (void)
{
  ASSERT(get_cpu ()->ncli == 1);
  struct thread *cur = driver_current ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT(intr_get_level () == INTR_OFF);
  ASSERT(cur->status != THREAD_RUNNING);
  ASSERT(is_thread (next));
  int intena = get_cpu ()->intena;
  if (cur != next)
    {
      get_cpu ()->cs++;
      get_cpu ()->rq.curr = next == get_cpu ()->rq.idle_thread ? NULL : next;
      prev = cur;
      /* NO context switching */
    }
  get_cpu ()->intena = intena;
  driver_schedule_tail (prev);
}

/* No changes from thread.c */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Allocates TIDs for the simulated threads. 
   No changes from thread.c */
static tid_t
allocate_tid (void)
{
  static tid_t next_tid = 0;
  return atomic_inci (&next_tid);
}

/* This simulates init_thread in thread.c.
   Doesn't add the thread to all list, and 
   also doesn't set up the initial stack frame */
static void
driver_init_thread (struct thread *t, const char *name, int nice)
{
  ASSERT(t != NULL);
  ASSERT(NICE_MIN <= nice && nice <= NICE_MAX);
  ASSERT(name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->nice = nice;
  t->magic = THREAD_MAGIC;
  t->cpu = get_cpu ();
}

/* This simulates an interrupt passing control to the interrupt handler in
   interrupt.c and invoking the timer interrupt handler in timer.c. */
void
driver_interrupt_tick (void)
{
  ASSERT(intr_get_level () == INTR_OFF);
  get_cpu ()->in_external_intr = true;
  get_cpu ()->yield_on_return = false;
  driver_tick ();
  get_cpu ()->in_external_intr = false;
  if (get_cpu ()->yield_on_return)
    driver_yield();
}
