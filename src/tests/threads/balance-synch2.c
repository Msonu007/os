/*
 * Checks for race conditions during load balancing. 
 * Threads increment a shared counter, then yield. 
 * This test calls the scheduler much more often than
 * balance-synch1, so it is much more unforgiving of 
 * race conditions
 * This test will not run fast!
 */
#include "threads/synch.h"
#include "tests/threads/tests.h"
#include "threads/cpu.h"
#include "threads/interrupt.h"
#include "lib/kernel/list.h"
#include "threads/malloc.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "threads/thread.h"

#define NUM_TESTS 1000
#define NUM_THREADS_PER_CPU 10
#define FINAL_VALUE 2000

struct shared_info {
  struct list_elem elem;
  int num;
};
static struct semaphore finished_sema;
static struct spinlock shared_lock;
static struct list shared_list;
static int shared_counter;
static bool done;

static void
check_num (void) 
{
  if (list_empty (&shared_list)) {
      done = true;
      return;
  }
  struct list_elem *e = list_pop_front (&shared_list);
  struct shared_info *f = list_entry (e, struct shared_info, elem);
  fail_if_false (f->num == shared_counter, "Number from list is wrong");
  shared_counter++;  
}

/* This thread gets migrated */
static void
inc_shared (void *aux UNUSED) 
{
  /* Done can be checked locklessly */
  while (!done) {
      spinlock_acquire (&shared_lock);
      check_num ();
      spinlock_release (&shared_lock);
      thread_yield ();
  } 

  sema_up (&finished_sema);
}

/* This thread finishes quickly on one CPU.
   When all the NOP threads finish, that CPU will load balance. */
static void
NOP (void *aux UNUSED)
{
  sema_up (&finished_sema);
}

/* Multiple threads increment a shared counter. */
static void
test_inc_shared (void)
{

  list_init (&shared_list);
  sema_init (&finished_sema, 0);
  spinlock_init (&shared_lock);
  shared_counter = 0;
  done = false;
  unsigned int i;
  
  struct shared_info *info = malloc (FINAL_VALUE * sizeof (*info));
  for (i=0;i<FINAL_VALUE;i++) {
      info[i].num = i;
      list_push_back (&shared_list, &info[i].elem);
  }

  for(i=0;i<NUM_THREADS_PER_CPU * ncpu;i++) {
      thread_func *func = i % 2 == 0 ? inc_shared : NOP;
      char *name = i % 2 == 0 ? "inc_shared" : "nop";
      thread_create (name, NICE_DEFAULT, func, NULL);    
  }

  for(i=0;i<NUM_THREADS_PER_CPU * ncpu;i++) {
      sema_down (&finished_sema);
  }

  fail_if_false (list_empty (&shared_list), "List should be empty");
  fail_if_false (shared_counter == FINAL_VALUE, "Incorrect value of shared counter!");
  free (info);
}

void
test_balance_sleepers (void)
{
  fail_if_false (ncpu == 2, "number of cpus must be 2");
  msg("This test is very unforgiving of race conditions.");
  msg("It will not run fast!.");
  msg ("Running %d tests.", NUM_TESTS);
  int i;
  for (i = 0;i<NUM_TESTS;i++) {
      test_inc_shared ();   
      if (i % 10 == 0)
        msg("Finished test %d", i);
  }
  
  pass ();
}
