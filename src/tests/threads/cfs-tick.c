/*
 * Tests that:
 * 1) If current task hasn't run for ideal_runtime, no rescheduling occurs
 * 2) If current task has been run for ideal_runtime, then rescheduling occurs
 * Requires yield_fair working
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_tick ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (2000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (2000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  pass ();
  cfstest_tear_down ();

}