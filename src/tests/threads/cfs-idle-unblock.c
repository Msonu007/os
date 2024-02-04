/*
 * Check the case where an idle thread unblocks a thread.
 * May happen from device drivers interrupting the idle thread and
 * unblocking threads
 * Tests that:
 * 1) Idle threads don't yield (they call thread_block in a tight loop anyway)
 * 2) Idle weights don't contribute to the scheduling period calculation
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_idle_unblock ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  struct thread *idle = driver_idle ();
  cfstest_advance_time (0);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (0);
  driver_block ();
  cfstest_check_current (t1);
  cfstest_advance_time (0);
  driver_block ();
  cfstest_check_current (idle);
  cfstest_advance_time (4000000);
  driver_unblock (t1);
  cfstest_check_current (t1);
  cfstest_advance_time (0);
  struct thread *t2 = driver_create ("t2", 0);
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  pass ();
  cfstest_tear_down ();
}
