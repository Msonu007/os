/*
 * Check more cases around min_vruntime, used to assign threads that just unblocked
 * test_sleep and test_short_sleep already took care of sleeper cases, let's do some
 * for new threads
 *
 * Case 1: no threads in rq (should choose curr)
 * Case 2: leftmost thread vruntime < curr (should choose leftmost thread)
 * Case 3: leftmost thread vruntime > curr (should choose curr)
 *
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_new_minvruntime ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (2000000);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (2000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  struct thread *t2 = driver_create ("t2", 0);
  cfstest_check_current (t1);
  cfstest_advance_time (0);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (0);
  struct thread *t3 = driver_create ("t3", 0);
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t3);
  pass ();
  cfstest_tear_down ();

}