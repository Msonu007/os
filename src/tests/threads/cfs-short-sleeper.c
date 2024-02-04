/*
 * Test that:
 * 1) Priority boost does not let short sleepers gain time
 * 2) Sleepers vruntime is updated prior to sleeping
 * A "short sleep" is defined as a sleep for less than sched_latency (20ms by default)
 * In this test, even though initial thread went blocked at t=20ms, 
 * it only blocked for 10ms, which means it won't receive the full sleeper bonus.
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_short_sleeper ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (20000000);
  driver_block ();
  cfstest_check_current (t1);
  cfstest_advance_time (10000000);
  driver_unblock (initial);
  cfstest_check_current (t1);
  cfstest_advance_time (20000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (20000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  pass ();
  cfstest_tear_down ();

}
