/*
 * Test that:
 * 1) When there is only one task running, no rescheduling ever occurs
 * 2) If there are no tasks, then idle thread is scheduled
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_idle ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  struct thread *idle = driver_idle ();
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_exit ();
  cfstest_check_current (idle);
  pass ();
  cfstest_tear_down ();
}
