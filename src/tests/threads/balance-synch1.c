/*
 * This test creates short-running threads on CPU0 and long-running threads on 
 * CPU1. CPU0 is expected to finish its work much sooner and attempt to load
 * balance from CPU1 
 * The test is run multiple times to try to uncover any race conditions 
 * that may occur during load balancing.
 * This test will not run fast!
 */
#include <stdbool.h>
#include <stdlib.h>
#include "tests.h"
#include "threads/thread.h"
#include <debug.h>
#include "threads/synch.h"
#include <stdio.h>
#include "threads/mp.h"
#include "devices/timer.h"
#include "threads/malloc.h"

#define N 25
#define N_MAX 46
#define NUM_THREADS_PER_CPU 10
#define NUM_TESTS 500

static struct semaphore finished_sema;

static int fib_numbers[] =
  { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597,
      2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418,
      317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465,
      14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296,
      433494437, 701408733, 1134903170, 1836311903 };

static int fib (int n);

static void
calc_fib (void *aux UNUSED)
{

  int n = N;
  fail_if_false (n <= N_MAX, "fibonacci test argument exceeds max");

  int num = fib (n);
  fail_if_false (num == fib_numbers[n], "Fib of %d should be %d, calculated %d",
               n, fib_numbers[n], num);
  sema_up (&finished_sema);
}

static void
NOP (void *aux UNUSED)
{
  sema_up (&finished_sema);
}

static void
test_fib (void)
{
  sema_init (&finished_sema, 0);
  unsigned int i;

  for (i = 0; i < NUM_THREADS_PER_CPU * ncpu; i++)
    {
      thread_func *func = i % 2 == 0 ? calc_fib : NOP;
      char *name = i % 2 == 0 ? "calc_fib" : "nop";
      thread_create (name, NICE_DEFAULT, func, NULL);
    }
  for (i = 0; i < NUM_THREADS_PER_CPU * ncpu; i++)
    {
      sema_down (&finished_sema);
    }
}

void
test_balance_synch1 (void)
{
  fail_if_false (ncpu == 2, "number of cpus must be 2");
  msg ("Load balancing test is run multiple times.");
  msg ("to look for race conditions that may occur during.");
  msg ("load balancing..");
  msg ("This test will not run fast!.");

  msg ("Running %d tests.", NUM_TESTS);
  int i;
  for (i = 0; i < NUM_TESTS; i++)
    {
      test_fib ();
      if (i % 10 == 0)
        msg ("Finished test %d", i);
    }
  pass ();
}

static int
fib (int n)
{
  if (n <= 1)
    return n;
  return fib (n - 1) + fib (n - 2);
}
