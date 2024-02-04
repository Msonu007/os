/*
 * Tests that:
 * 1) Tasks has their time slices adjusted based on nice
 * 2) Tasks have their vruntimes adjusted based on nice
 * 3) Both positive and negative nice values are respected
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_nice ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *n20 = driver_create ("n20", -20);
  cfstest_check_current (initial);
  cfstest_advance_time (0);
  struct thread *p19 = driver_create ("p19", 19);
  cfstest_check_current (initial);
  cfstest_advance_time (136900);
  driver_interrupt_tick ();
  cfstest_check_current (n20);
  cfstest_advance_time (5930600);
  driver_interrupt_tick ();
  cfstest_check_current (n20);
  cfstest_advance_time (5930600);
  driver_interrupt_tick ();
  cfstest_check_current (p19);
  cfstest_advance_time (2100);
  driver_interrupt_tick ();
  cfstest_check_current (n20);
  cfstest_advance_time (11861200);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (136900);
  driver_interrupt_tick ();
  cfstest_check_current (p19);
  pass ();
  cfstest_tear_down ();

}