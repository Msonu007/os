#include "devices/timer.h"
#include "threads/thread.h"
#include "threads/interrupt.h"
#include <stdio.h>
#include "threads/scheduler.h"
#include "threads/cpu.h"
#include <stdlib.h>
#include <string.h>
#include "tests/threads/tests.h"
#include <debug.h>
#include "threads/gdt.h"
#include "tests/threads/simulator.h"
#include "tests/threads/cfstest.h"

static struct cpu *real_cpu;
static struct cpu vcpu;

static uint64_t real_time;

/*
   Tests have the general format:
   1) Setup initial thread
   2) Set up "idle"
   3) foreach (event) {
   	timer_settime(event.time);
   	do_event(event.func);
   	assert(something);
 */

/*
   Events are basically interrupts, either internal or external
    - tick()
    - block()
    - yield()
    - new()
    - unblock()
    - exit()
    - setnice()
    - getnice()
 */

/*
 * How to deal with cpu and thread_current()?
 * Since no context switching takes place, thread_current() cannot look at esp. Instead it must rely on
 * get_cpu ()->curr being set correctly after a scheduling decision is made
 */
/*
 * How to deal with cpu local variable?
 * Reload the gdt and gs register with the address of the "fake cpu"
 */

/*
 * How to deal with time?
 * In testing mode, timer_clock() doesnt look at jiffies but rather returns time that is set by the driver
 */

/* Make cpu local variable point to cpu by reloading the gdt and gs register*/
static void
switch_cpu (struct cpu *cpu)
{
  gdt_refer_cpu (cpu);
}

void
cfstest_check_current (struct thread *expect)
{
  struct thread *actual = driver_current ();
  if (actual != expect)
    {
      cfstest_tear_down ();
      fail ("Current thread should be %p, actually %p", expect, actual);
    }
}

/*
 * CFS makes decisions based on the current time. It gets time from timer_gettime(), which normally would return
 * the current time based on jiffies or some other timing mechanism. However, in test mode, we articially set the time
 * in order to make CFS behave in a predictable way that we can test
 */
void
cfstest_advance_time (int64_t advance)
{
  timer_settime (timer_gettime () + advance);
}

/*
 * Set up vcpu. Requires a working implementation of init and task_new
 */
void
cfstest_set_up (void)
{
  /* Must come before switch_cpu so stats are recorded on the right CPU */
  intr_disable_push ();
  real_time = timer_gettime ();
  real_cpu = get_cpu ();
  memset (&vcpu, 0, sizeof(struct cpu));
  switch_cpu (&vcpu);
  timer_settime (0);
  driver_init ();

}

void
cfstest_tear_down (void)
{
  switch_cpu (real_cpu);
  timer_settime (real_time);
  intr_enable_pop ();
}
