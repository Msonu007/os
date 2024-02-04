/*
 * Test your scheduler running many "sleeper" threads by contending
 * for a lock
 * To pass, this just needs to run to completion
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

#define NUM_TESTS 10
#define FINAL_VALUE 20000
#define NUM_THREADS 20
#define INC_EACH FINAL_VALUE/NUM_THREADS

struct shared_info
{
  struct list_elem elem;
  int num;
};
static struct semaphore finished_sema;
static struct lock shared_lock;
static struct list shared_list;
static int shared_counter;
static bool done;

static void
check_num (void)
{
  if (list_empty (&shared_list))
    {
      done = true;
      return;
    }
  struct list_elem *e = list_pop_front (&shared_list);
  struct shared_info *f = list_entry(e, struct shared_info, elem);
  fail_if_false (f->num == shared_counter, "Number from list is wrong");
  shared_counter++;
}

static void
inc_shared (void *aux UNUSED)
{
  /* Done can be checked locklessly */
  while (!done)
    {
      lock_acquire (&shared_lock);
      check_num ();
      lock_release (&shared_lock);
    }
  sema_up (&finished_sema);
}

/* Multiple threads increment a shared counter. */
static void
test_inc_shared (void)
{

  list_init (&shared_list);
  sema_init (&finished_sema, 0);
  lock_init (&shared_lock);
  shared_counter = 0;
  done = false;
  int i;

  struct shared_info *info = malloc (FINAL_VALUE * sizeof(*info));
  for (i = 0; i < FINAL_VALUE; i++)
    {
      info[i].num = i;
      list_push_back (&shared_list, &info[i].elem);
    }

  for (i = 0; i < NUM_THREADS; i++)
    {
      thread_create ("inc_shared", NICE_DEFAULT, inc_shared, NULL);
    }
  for (i = 0; i < NUM_THREADS; i++)
    {
      sema_down (&finished_sema);
    }
  fail_if_false (list_empty (&shared_list), "List should be empty");
  fail_if_false (shared_counter == FINAL_VALUE,
               "Incorrect value of shared counter!");
  free (info);
}

void
test_cfs_sleepers (void)
{
  msg ("Test your scheduler running many 'sleeper' threads by contending.");
  msg ("for a lock. To pass, this needs to run to completion without error.");
  msg ("Running test %d times...", NUM_TESTS);
  int i;
  for (i = 0; i < NUM_TESTS; i++)
    {
      test_inc_shared ();
      msg ("Finished test %d", i);
    }

  pass ();
}
