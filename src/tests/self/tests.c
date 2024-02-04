#include "tests.h"
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
    { "change-cpu", test_change_cpu },
    { "atomics", test_atomics },
    { "spinlock", test_spinlock },
    { "all-list", test_all_list },
    { "ipi", test_ipi },
    { "ipi-blocked", test_ipi_blocked },
    { "ipi-all", test_ipi_all },
    { "ipi-missed", test_ipi_missed },
    { "cli-print", test_cli_print },
    { "savecallerinfo", test_savecallerinfo },
    { "console", test_console },
    { "realclock", test_realclock },
  };

static const char *test_name;

/* Runs the test named NAME. */
void
run_self_test (const char *name)
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
  PANIC("no test named \"%s\"", name);
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

  PANIC("test failed");
}

/* Prints a message indicating the current test passed. */
void
pass (void)
{
  printf ("(%s) PASS\n", test_name);
}

/*
 * Basically an assertion
 * if (!truth)
 *  fail
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
