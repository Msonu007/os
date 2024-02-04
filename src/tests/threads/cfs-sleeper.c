/*
 * Test that sleepers get their due priority boost. They should be the first ones to be scheduled after waking up
 * block() should cause a vruntime update
 * unblock() should then cause another vruntime update (setting to min_vruntime) minus sleeper bonus
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_sleeper ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (0);
  struct thread *t2 = driver_create ("t2", 0);
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
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (2000000);
  driver_block ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (0);
  driver_unblock (initial);
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (24000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  pass ();
  cfstest_tear_down ();
}
