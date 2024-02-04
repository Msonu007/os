#include <debug.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "lib/atomic-ops.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"

/* Prints the call stack, that is, a list of addresses, one in
   each of the functions we are nested within.  gdb or addr2line
   may be applied to kernel.o to translate these into file names,
   line numbers, and function names.  */
void
debug_backtrace (void) 
{
  static int explained;
  void **frame;
  
  printf ("Call stack: %p", __builtin_return_address (0));
  for (frame = __builtin_frame_address (1);
       (uintptr_t) frame >= 0x1000 && frame[0] != NULL;
       frame = frame[0]) 
    printf (" %p", frame[1]);
  printf (".\n");

  if (atomic_xchg (&explained, 1) == 0)
    {
      printf ("The `backtrace' program can make call stacks useful.\n"
              "Read \"Backtraces\" in the \"Debugging Tools\" chapter\n"
              "of the Pintos documentation for more information.\n\n");
    }
}

#pragma GCC diagnostic pop
