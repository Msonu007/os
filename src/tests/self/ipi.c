/*
 * Test inter-processor interrupts
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

static int flag = 0;

static void
ipi_test (struct intr_frame *f UNUSED)
{
  ASSERT (intr_get_level () == INTR_OFF);
  __atomic_or_fetch (&flag, 1 << get_cpu ()->id, __ATOMIC_SEQ_CST);
}

static void
register_test_ipi (void)
{
  intr_register_ipi (T_IPI + IPI_NUM, ipi_test, "#IPI TEST");
}

void
test_ipi (void)
{
  console_set_mode (EMERGENCY_MODE);
  fail_if_false (ncpu > 1, "This test >1 cpu running");
  register_test_ipi ();
  intr_disable_push ();
  lapic_send_ipi_to_all_but_self (IPI_NUM);
  int expected_flag = 0;
  unsigned int i;
  for (i = 0; i < ncpu; i++)
    {
      if (i != get_cpu ()->id)
        expected_flag |= 1 << i;
    }

  intr_enable_pop ();
  while (atomic_load (&flag) != expected_flag)
    ;
  pass ();
}

static int numstarted = 0;
/*
 * Disable interrupts, so it cannot respond to IPI
 */
static void
AP_disable (void *aux UNUSED)
{
  intr_disable ();
  atomic_inci (&numstarted);
  while (1)
    ;
}
/*
 * This test assumes that threads are assigned to
 * CPUs in a round robin fashion. Attempt to disable
 * interrupts in all CPUs. 
 * Tests that IPI is blocked by disabling interrupts
 */
void
test_ipi_blocked (void)
{
  fail_if_false (ncpu > 1, "This test > 1 cpu running");
  register_test_ipi ();
  unsigned int i;
  for (i = 0; i < ncpu - 1; i++)
    {
      thread_create ("test_ipi", 0, AP_disable, NULL);
    }
  while (atomic_load (&numstarted) < (int) (ncpu - 1))
    ;
  lapic_send_ipi_to_all_but_self (IPI_NUM);
  timer_sleep (5);
  fail_if_false (flag == 0, "Other CPUs responded to IPI, despite interrupts "
	       "being off");
  pass ();
}

void
test_ipi_all (void)
{
  console_set_mode (EMERGENCY_MODE);
  fail_if_false (ncpu > 1, "This test requires > 1 cpu running");
  register_test_ipi ();
  lapic_send_ipi_to_all (IPI_NUM);
  int expected_flag = 0;
  unsigned int i;
  for (i = 0; i < ncpu; i++)
    {
      expected_flag |= 1 << i;
    }
  while (atomic_load (&flag) != expected_flag)
    ;
  pass ();
}
