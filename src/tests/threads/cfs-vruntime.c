/*
 * More fine grained test on how vruntime affects scheduling decisions.
 * Unlike other tests which mostly preempts at gran intervals (causing a
 * guaranteed context switch) this test will make interrupts arrive at more non-uniform
 * intervals
 */

#include "threads/thread.h"
#include "tests/threads/cfstest.h"
#include "tests/threads/simulator.h"
#include "tests/threads/tests.h"

void
test_vruntime ()
{
  cfstest_set_up ();
  struct thread *initial = driver_current ();
  cfstest_advance_time (0);
  struct thread *t1 = driver_create ("t1", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (1000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (1000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (1000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (1000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (10000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (1000000);
  struct thread *t2 = driver_create ("t2", 0);
  cfstest_check_current (initial);
  cfstest_advance_time (3000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  cfstest_advance_time (6000000);
  driver_interrupt_tick ();
  cfstest_check_current (initial);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t1);
  cfstest_advance_time (4000000);
  driver_interrupt_tick ();
  cfstest_check_current (t2);
  pass ();
  cfstest_tear_down ();

}