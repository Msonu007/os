/*
 * Create a new task and check that it is added to the queue
 * Requires pick_next_task
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_create_new ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (0);
  driver_exit ();
  cfstest_check_current (t1);
  pass ();
  cfstest_tear_down ();
}