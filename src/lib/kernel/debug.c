/* This file is derived from source code for the xv6 instruction 
   operating system. The xv6 copyright notice is printed below.

   The xv6 software is:

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

#include <debug.h>
#include <console.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/switch.h"
#include "threads/vaddr.h"
#include "devices/serial.h"
#include "devices/shutdown.h"
#include "threads/cpu.h"
#include "devices/lapic.h"
#include "lib/kernel/x86.h"
#include <stdint.h>
#include "threads/mp.h"
#include "lib/atomic-ops.h"

static struct spinlock debug_backtrace_lock;

void 
debug_backtrace_with_lock (void)
{
  spinlock_acquire (&debug_backtrace_lock);
  debug_backtrace ();
  spinlock_release (&debug_backtrace_lock);
}

/* Halts the OS, printing the source file name, line number, and
   function name, plus a user-specific message. */
void
debug_panic (const char *file, int line, const char *function,
             const char *message, ...)
{
  static int level;
  va_list args;

  intr_disable ();
  int new_level = atomic_inci (&level);
  if (new_level == 1)
    {
      spinlock_init (&debug_backtrace_lock);

      /* After printing backtrace, send a message to all other CPUs
       * to print their backtraces as well.
       * Finally, it print a backtrace of all threads.
       */
      console_set_mode (EMERGENCY_MODE);

      printf ("Kernel PANIC from CPU %d at %s:%d in %s(): ",
        (cpu_started_others) ? get_cpu ()->id : 0, file, line, function);

      va_start (args, message);
      vprintf (message, args);
      printf ("\n");
      va_end (args);

      debug_backtrace_with_lock ();

      if (cpu_started_others)
        {
          printf ("Printing a backtrace of all CPUs\n");
          lapic_send_ipi_to_all_but_self (IPI_DEBUG);
        }

      printf ("Printing a backtrace of all threads\n");
      debug_backtrace_all ();
      printf ("\n");
    }
  else if (new_level == 2)
    {
      printf ("Kernel PANIC recursion at %s:%d in %s().\n",
        file, line, function);
    }
  else
    {
      /* Don't print anything: that's probably why we recursed. */
    }

  serial_flush ();
  shutdown ();
  for (;;);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"

/* Print call stack of a thread.
   The thread may be running, ready, or blocked. */
static void
print_stacktrace(struct thread *t, void *aux UNUSED)
{
  void *retaddr = NULL, **frame = NULL;
  const char *status = "UNKNOWN";

  switch (t->status) {
    case THREAD_RUNNING:  
      status = "RUNNING";
      break;

    case THREAD_READY:  
      status = "READY";
      break;

    case THREAD_BLOCKED:  
      status = "BLOCKED";
      break;

    default:
      break;
  }

  printf ("Call stack of thread '%s' (status %s, CPU%d):", t->name, status, t->cpu->id);

  if (t == thread_current() && t->cpu == get_cpu ())
    {
      frame = __builtin_frame_address (1);
      retaddr = __builtin_return_address (0);
    }
  else
    {
      /* We can't really print backtraces for stack running on another CPU. */
      if (t->status == THREAD_RUNNING)
        {
          printf ("thread appears currently running on another CPU, skipping.\n");
          return;
        }

      /* Retrieve the values of the base and instruction pointers
         as they were saved when this thread called switch_threads. */
      struct switch_threads_frame * saved_frame;

      saved_frame = (struct switch_threads_frame *)t->stack;

      /* Skip threads if they have been added to the all threads
         list, but have never been scheduled.
         We can identify because their `stack' member either points 
         at the top of their kernel stack page, or the 
         switch_threads_frame's 'eip' member points at switch_entry.
         See also threads.c. */
      if (t->stack == (uint8_t *)t + PGSIZE || saved_frame->eip == switch_entry)
        {
          printf (" thread was never scheduled.\n");
          return;
        }

      frame = (void **) saved_frame->ebp;
      retaddr = (void *) saved_frame->eip;
    }

  printf (" %p", retaddr);
  for (; (uintptr_t) frame >= 0x1000 && frame[0] != NULL; frame = frame[0])
    printf (" %p", frame[1]);
  printf (".\n");
}
#pragma GCC diagnostic pop

/* Attempt to prints call stacks of all threads.
   In the presence of multiple CPUs, this is inherently racy unless
   we stopped those CPUs first.
   print_stacktrace will skip threads it sees running on another CPU,
   but it will to traverse stacktraces of all other threads.

   We provide this only as a debugging aid.
 */
void
debug_backtrace_all (void)
{
  intr_disable_push();
  thread_foreach (print_stacktrace, 0);
  intr_enable_pop();
}

void
debug_init_callerinfo (struct callerinfo *info)
{
  memset (info, 0, sizeof (*info));
}
/*
 * Records the stack trace and information of the caller of
 * this function into info. Useful for storing acquire/release
 * info for debugging locks/spinlocks
 */
void
debug_save_callerinfo (struct callerinfo *info)
{
  intr_disable_push ();
  info->cpu = get_cpu ();
  intr_enable_pop ();
  info->t = running_thread ();
  
  uint32_t *ebp;
  uint32_t *pcs = info->pcs;
  int i;

  ebp =  __builtin_frame_address (0);
  for (i = 0; i < PCS_MAX; i++)
    {
      if (ebp == NULL || ebp < (uint32_t *)LOADER_PHYS_BASE)
    break;
      pcs[i] = ebp[1];     /* saved %eip */
      ebp = (uint32_t*) ebp[0];     /* saved %ebp */
    }
  for (; i < PCS_MAX; i++)
    pcs[i] = 0;
}

/*
 * Print caller information. If interrupts are disabled, it is 
 * up to caller to panic the console before calling this function.
 */
void
debug_print_callerinfo (struct callerinfo *info) 
{
  if (!info->t || !info->cpu) 
    {
      printf ("No call stack saved.");
    }
  else 
    {
      printf ("Thread '%s' from CPU%d\n", info->t->name, info->cpu->id);
      printf ("Call stack:");
      int i;
      uint32_t *pcs = info->pcs;
      for (i = 0; i < PCS_MAX; i++)
        {
          if (pcs[i] == 0)
            break;
          printf (" %p", (void *)pcs[i]);
        }
      printf (".\n");
    }
}
