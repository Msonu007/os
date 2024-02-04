/* Creates 20 CPU-bound processes and runs them under your 
 scheduler. To pass, this just needs to run till completion. */
#include <stdbool.h>
#include <stdlib.h>
#include "tests.h"
#include "threads/thread.h"
#include <debug.h>
#include "threads/synch.h"
#include <stdio.h>
#include "threads/mp.h"
#include "lib/atomic-ops.h"

#define N 30
#define N_MAX 46
#define NUM_THREADS 20

static int count = 0;

static int fib_numbers[] =
  { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597,
      2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418,
      317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465,
      14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296,
      433494437, 701408733, 1134903170, 1836311903 };

struct info
{
  int fib_id;
  struct semaphore sema;
};

static int fib (int n);

static void
fibtest (void)
{
  fail_if_false (N <= N_MAX, "fibonacci test argument exceeds max");
  int num = fib (N);
  fail_if_false (num == fib_numbers[N], "Fib of %d should be %d, calculated %d",
               N, fib_numbers[N], num);
  msg ("fib of %d is %d", N, num);

}

static void
child_fib (void *args)
{

  struct info *my_info = args;
  int id = atomic_inci (&count);
  if (id <= NUM_THREADS)
    {
      struct info child_info;
      child_info.fib_id = id;
      sema_init (&child_info.sema, 0);
      thread_create ("fib", NICE_DEFAULT, child_fib, &child_info);
      fibtest ();
      sema_down (&child_info.sema);
    }

  sema_up (&my_info->sema);

}
void
test_cfs_fib (void)
{
  msg ("Test your scheduler running batch processes.");
  msg ("To pass, this needs to run to completion without error.");
  struct info my_info;
  sema_init (&my_info.sema, 0);
  my_info.fib_id = atomic_inci (&count);
  child_fib (&my_info);
  pass ();
}

static int
fib (int n)
{
  if (n <= 1)
    return n;
  return fib (n - 1) + fib (n - 2);
}
