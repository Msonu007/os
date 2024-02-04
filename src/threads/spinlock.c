/* This file is derived from source code for the xv6
   instructional operating systems. Copyright notices are printed
   below . */

/* The xv6 software is:

   Copyright (c) 2006-2009 Frans Kaashoek, Robert Morris, Russ Cox,
                        Massachusetts Institute of Technology

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "threads/spinlock.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/flags.h"
#include "threads/cpu.h"
#include "lib/atomic-ops.h"
#include "lib/kernel/console.h"

static void panic_on_already_acquired_lock (struct callerinfo *info);
static void panic_on_non_acquired_lock (struct callerinfo *info);

void
spinlock_init (struct spinlock *spinlock)
{
  spinlock->locked = 0;
  spinlock->cpu = NULL;
  debug_init_callerinfo (&spinlock->debuginfo);
}

/* Acquire the spinlock.
   Loops (spins) until the spinlock is acquired.
   Holding a spinlock for a long time may cause
   other CPUs to waste time spinning to acquire it. */
void
spinlock_acquire (struct spinlock *spinlock)
{
  intr_disable_push ();     /* disable interrupts to avoid race conditions from interrupts */
  if (spinlock_held_by_current_cpu (spinlock))
    panic_on_already_acquired_lock (&spinlock->debuginfo);

  /* The xchg is atomic.
     It also serializes, so that reads after acquire are not
     reordered before it. */
  while (atomic_xchg (&spinlock->locked, 1) != 0)
    ;

  /* Record info about lock acquisition for debugging. */
  spinlock->cpu = get_cpu ();
  debug_save_callerinfo (&spinlock->debuginfo);
}

/* Release the lock. */
void
spinlock_release (struct spinlock *spinlock)
{
  if (!spinlock_held_by_current_cpu (spinlock))
    panic_on_non_acquired_lock (&spinlock->debuginfo);

  spinlock->cpu = NULL;
  debug_save_callerinfo (&spinlock->debuginfo);

  /* The xchg serializes, so that reads before release are
     not reordered after it.  The 1996 PentiumPro manual (Volume 3,
     7.2) says reads can be carried out speculatively and in
     any order, which implies we need to serialize here.
     But the 2007 Intel 64 Architecture Memory Ordering White
     Paper says that Intel 64 and IA-32 will not move a load
     after a store. So lock->locked = 0 would work here.
     The xchg being asm volatile ensures gcc emits it after
     the above assignments (and after the critical section). */
  atomic_xchg (&spinlock->locked, 0);

  intr_enable_pop ();
}

bool
spinlock_try_acquire (struct spinlock *spinlock)
{
  intr_disable_push ();     /* disable interrupts to avoid race conditions from interrupts */
  if (spinlock_held_by_current_cpu (spinlock))
    panic_on_already_acquired_lock (&spinlock->debuginfo);

  if (atomic_xchg (&spinlock->locked, 1) != 0)
    {
      intr_enable_pop ();
      return false;
    }
  else
    {
      spinlock->cpu = get_cpu ();
      debug_save_callerinfo (&spinlock->debuginfo);
      return true;
    }
}

/* Check whether this cpu is holding the lock. */
bool
spinlock_held_by_current_cpu (const struct spinlock *lock)
{
  return lock->locked && lock->cpu == get_cpu ();
}

static void
panic_on_already_acquired_lock (struct callerinfo *info)
{
  console_set_mode (EMERGENCY_MODE);
  printf ("ERROR: Tried to acquire an already held spinlock!\n");
  printf ("Lock last acquired by: ");
  debug_print_callerinfo (info);
  printf ("\n");
  PANIC ("acquire");
}

static void
panic_on_non_acquired_lock (struct callerinfo *info)
{
  console_set_mode (EMERGENCY_MODE);
  printf ("ERROR: Tried to release an unacquired spinlock!\n");
  printf ("Lock last released by: ");
  debug_print_callerinfo (info);
  printf ("\n");
  PANIC ("release");
}
