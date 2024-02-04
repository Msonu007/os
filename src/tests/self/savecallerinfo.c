/*
 * Test savecallerinfo, which is useful to help debug locks/spinlocks
 */
#include "threads/synch.h"
#include "tests.h"
#include "threads/cpu.h"
#include "threads/interrupt.h"
#include "lib/kernel/list.h"
#include "threads/malloc.h"
#include <string.h>
#include <debug.h>

void
test_savecallerinfo (void)
{
  struct callerinfo info;
  debug_save_callerinfo (&info);
  debug_print_callerinfo (&info);
  pass ();
}
