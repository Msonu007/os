/*
 * Tests that:
 * 1) Renicing has immediate effect on timeslice/vruntime on thread
 * 2) Renicing has immediate effect on other threads whose slices might be shortened/lengthened.
 * 	Although other thread's vruntime growth stays the same, its delta can be affected solely because ideal_runtime has changed
 * Note: Vruntime doesn't necessarily go up by gran after each yield. For example, if all your processes have a nice of
 * -20, their vruntimes go up by ~90000 after each yield rather than 4000000. The other tests had that hardcoded because it is
 * the case during the common case of nice = 0 and delta_exec = 4000000
 *
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_renice ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *renice = driver_create ("renice", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (renice);
  cfstest_advance_time (0);
  driver_set_nice (1);
  cfstest_check_current (renice);
  cfstest_advance_time (3557500);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (2221300);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (2221300);
  driver_interrupt_tick ();
  cfstest_check_current (renice);
  cfstest_advance_time (3557500);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (0);
  driver_set_nice (-1);
  cfstest_check_current (initial);
  cfstest_advance_time (4871800);
  driver_interrupt_tick ();
  cfstest_check_current (renice);
  cfstest_advance_time (3128300);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  pass ();
  cfstest_tear_down ();

}