/*
 * Tests that if a new thread is created right after a thread is unblocked, then that
 * new thread doesn't get the sleeper bonus (cfs_rq->min_vruntime should never go backwards)
 * Also makes sure that current thread's vruntime is updated prior to setting min_vruntime
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_sleeper_minvruntime ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (0);
  driver_block ();
  cfstest_check_current (initial);
  cfstest_advance_time (20000000);
  driver_unblock (t1);
  cfstest_check_current (t1);
  cfstest_advance_time (0);
  struct thread *t2 = driver_create ("t2", 0);
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  pass ();
  cfstest_tear_down ();
}
