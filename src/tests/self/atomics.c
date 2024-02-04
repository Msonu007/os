/*
 * Test atomic operations
 */
#include "lib/atomic-ops.h"
#include "tests.h"

static void
check_value (int actual, int expected)
{
  fail_if_false (expected == actual, "Expected %d, actually %d\n", expected,
	       actual);
}

void
test_atomics (void)
{
  int val;
  int var;
  int new;

  var = 0;
  new = 5;
  int old = atomic_xchg (&var, new);
  check_value (old, 0);
  check_value (var, new);

  var = 0;
  val = atomic_inci (&var);
  check_value (var, 1);
  check_value (val, 1);

  var = 0;
  atomic_deci (&var);
  val = atomic_deci (&var);
  check_value (val, -2);
  check_value (var, -2);

  var = 1;
  val = atomic_addi (&var, 5);
  check_value (val, 6);
  check_value (var, 6);

  var = 0;
  new = 5;
  int cmpTrue = 0;
  fail_if_false (atomic_cmpxchg (&var, &cmpTrue, &new), "cmpxchg returned false");
  check_value (var, new);

  var = 0;
  new = 5;
  int cmpFalse = 1;
  fail_if_false (!atomic_cmpxchg (&var, &cmpFalse, &new),
	       "cmpxchg returned true");
  check_value (var, 0);

  var = 5;
  check_value (atomic_load (&var), 5);

  var = 0;
  int store = 5;
  atomic_store (&var, store);
  check_value (var, store);

  pass ();
}
