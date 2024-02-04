/*
 * Tests what happens when all CPUs try to send the same IPI
 * to one CPU at once. Can that CPU miss an interrupt?
 */
#include "threads/interrupt.h"
#include "lib/atomic-ops.h"
#include <debug.h>
#include "devices/lapic.h"
#include "tests.h"
#include "threads/mp.h"
#include "devices/timer.h"
#include "lib/kernel/console.h"
#include <stdio.h>

#define IPI_NUM 4
#define NUM_IPI_PER_THREAD 10

static int hits = 0;

static void
ipi_test_missed (struct intr_frame *f UNUSED)
{
  ASSERT (intr_get_level () == INTR_OFF);
  atomic_inci (&hits);
}

static void
register_test_ipi (void)
{
  intr_register_ipi (T_IPI + IPI_NUM, ipi_test_missed, "#IPI TEST");
}
static struct spinlock lock;
static int numstarted = 0;

static void
spam_ipi (void *aux UNUSED)
{
  intr_disable_push ();
  if (get_cpu ()->id == 0) {
      intr_enable_pop ();
      return;
  }
  intr_enable_pop ();
  int i;
  for (i = 0; i < NUM_IPI_PER_THREAD; i++)
    {
      spinlock_acquire (&lock);
      int next = hits + 1;
      lapic_send_ipi_to (IPI_NUM, 0);
      while (atomic_load (&hits) != next)
        ;
      spinlock_release (&lock);
    }
  atomic_inci (&numstarted);
}
/*
 * This test assumes that threads are assigned to
 * CPUs in a round robin fashion.
 */
void
test_ipi_missed (void)
{
  fail_if_false (ncpu > 1, "This test > 1 cpu running");
  register_test_ipi ();
  spinlock_init (&lock);
  unsigned int i;
  for (i = 0; i < ncpu; i++)
    {
      thread_create ("test_ipi_missed", 0, spam_ipi, NULL);
    }
//  while (atomic_load (&hits) < (int) (ncpu - 1))
//    ;

  timer_sleep (10000);
  int expected = (ncpu- 1) * NUM_IPI_PER_THREAD;
  fail_if_false (hits == expected,
               "Some IPI were missed: expected %d, actually %d\n", expected, hits);
  pass ();
}

