#include "tests/threads/tests.h"
#include "cfstest.h"
#include <debug.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct test 
  {
    const char *name;
    test_func *function;
  };

static const struct test tests[] = 
{
  { "alarm-single", test_alarm_single },
  { "alarm-multiple", test_alarm_multiple },
  { "alarm-synch", test_alarm_synch },
  { "alarm-zero", test_alarm_zero },
  { "alarm-negative", test_alarm_negative },
  { "cfs-create-new", test_create_new },
  { "cfs-idle", test_idle },
  { "cfs-yield", test_yield },
  { "cfs-tick", test_tick },
  { "cfs-tick2", test_tick2 },
  { "cfs-delayed-tick", test_delayed_tick },
  { "cfs-sleeper", test_sleeper },
  { "cfs-short-sleeper", test_short_sleeper },
  { "cfs-sleeper-minvruntime", test_sleeper_minvruntime },
  { "cfs-new-minvruntime", test_new_minvruntime },
  { "cfs-nice", test_nice },
  { "cfs-renice", test_renice },
  { "cfs-idle-unblock", test_idle_unblock },
  { "cfs-vruntime", test_vruntime }, 
  { "cfs-run-batch", test_cfs_fib },
  { "cfs-run-iobound", test_cfs_sleepers },
  { "balance", balance },
  { "balance-synch1", test_balance_synch1 },
  { "balance-synch2", test_balance_sleepers },
  };

static const char *test_name;

/* Runs the test named NAME. */
void
run_test (const char *name) 
{
  const struct test *t;

  for (t = tests; t < tests + sizeof tests / sizeof *tests; t++)
    if (!strcmp (name, t->name))
      {
        test_name = name;
        msg ("begin");
        t->function ();
        msg ("end");
        return;
      }
  PANIC ("no test named \"%s\"", name);
}

/* Prints FORMAT as if with printf(),
   prefixing the output by the name of the test
   and following it with a new-line character.
   Copies the entire message into one buffer 
   to prevent message interleaving. */
void
msg (const char *format, ...) 
{
  va_list args;
  
  char buf[1024];
  memset (buf, 0, sizeof buf);
  
  snprintf (buf, sizeof buf, "(%s) ", test_name);
  va_start (args, format);
  vsnprintf (buf + strlen (buf), sizeof buf - strlen (buf), format, args);
  va_end (args);
  printf ("%s\n", buf);
}

/*
 * Basically an assertion with msg
 * if (!truth)
 * 	fail (msg)
 */
void
fail_if_false (bool truth, const char *format, ...)
{
  if (!truth)
    {
      va_list args;

      printf ("(%s) FAIL: ", test_name);
      va_start (args, format);
      vprintf (format, args);
      va_end (args);
      putchar ('\n');

      PANIC("test failed");
    }
}

/* Prints failure message FORMAT as if with printf(),
   prefixing the output by the name of the test and FAIL:
   and following it with a new-line character,
   and then panics the kernel. */
void
fail (const char *format, ...) 
{
  va_list args;
  
  printf ("(%s) FAIL: ", test_name);
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
  putchar ('\n');

  PANIC ("test failed");
}

/* Prints a message indicating the current test passed. */
void
pass (void) 
{
  printf ("(%s) PASS\n", test_name);
}
