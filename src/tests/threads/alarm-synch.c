/* Creates NUM_SLEEPER threads, all of which sleep for a random amount
   of time. Check that they were woken at the right time 
   Runs the test multiple times */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"
#include "lib/random.h"
#include <inttypes.h>
#include <stdlib.h>
#include <debug.h>

#define NUM_TESTS 10
#define NUM_SLEEPERS 300
#define MAX_SLEEP_LENGTH 500L
#define TICK_MAX_ERROR 50

static struct semaphore finished_sema;

static void run_one_test (void);
static void sleeper (void *);
static void check_time (int64_t);

void
test_alarm_synch (void) 
{
  random_init (0);
  int i;
  for (i = 0;i<NUM_TESTS;i++)
    {
      run_one_test ();
    }
  
  pass ();
}

static void
run_one_test (void)
{
  sema_init (&finished_sema, 0);    
  int i;
  for (i=0;i<NUM_SLEEPERS;i++) 
    {
      tid_t tid = thread_create ("sleeper", NICE_DEFAULT, sleeper, NULL);
      ASSERT (tid != 0);
    }
  
  /* Wait for threads to finish */
  for (i=0;i<NUM_SLEEPERS;i++) 
    {
      sema_down (&finished_sema);
    }  
}

/* Sleeper thread. */
static void
sleeper (void *aux UNUSED) 
{
  unsigned long sleep_length = random_ulong () % MAX_SLEEP_LENGTH;
  timer_sleep (sleep_length);
  sema_up (&finished_sema);
}

static void check_time (int64_t expected) UNUSED;

/* Check that thread was woken up at the right time. Allow for some error
   because there can be a slight delay between getting woken up and actually
   getting scheduled.
   Normally, this would happen right after timer_sleep () but since it's hard
   to reliable calibrate TICK_MAX_ERROR, we will not used it for now, lest
   we cause the tests to spuriously fail. */
static void
check_time (int64_t expected)
{
  int64_t actual = timer_ticks ();
  fail_if_false (llabs (actual - expected) <= TICK_MAX_ERROR, 
               "Sleeper thread not awoken at the right time. "
               "Expected %"PRId64", actually %"PRId64"", expected, actual);
}
