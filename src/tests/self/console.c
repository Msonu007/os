/*
 * Test the console.  This is an interactive test, not yet automated.
 * It's mainly for input_getc() for which there are no other tests aside
 * from project 2 read(0, ...).
 */
#include "devices/input.h"
#include "tests.h"
#include <stdio.h>
#include <debug.h>
#include <string.h>
#include "lib/kernel/console.h"

static bool backspace (char **pos, char line[]);

/* Reads a line of input from the user into LINE, which has room
   for SIZE bytes.  Handles backspace and Ctrl+U in the ways
   expected by Unix users.  On return, LINE will always be
   null-terminated and will not end in a new-line character. */
static void
read_line (char line[], size_t size) 
{
  char *pos = line;
  for (;;)
    {
      char c = input_getc ();

      switch (c) 
        {
        case '\r':
          *pos = '\0';
          putchar ('\n');
          return;

        case '\b':
          backspace (&pos, line);
          break;

        case ('U' - 'A') + 1:       /* Ctrl+U. */
          while (backspace (&pos, line))
            continue;
          break;

        default:
          /* Add character to line. */
          if (pos < line + size - 1) 
            {
              putchar (c);
              *pos++ = c;
            }
          break;
        }
    }
}

/* If *POS is past the beginning of LINE, backs up one character
   position.  Returns true if successful, false if nothing was
   done. */
static bool
backspace (char **pos, char line[]) 
{
  if (*pos > line)
    {
      /* Back up cursor, overwrite character, back up
         again. */
      printf ("\b \b");
      (*pos)--;
      return true;
    }
  else
    return false;
}

void
test_console (void)
{
  for (;;) 
    {
      char buf [80];
      printf ("> ");
      read_line (buf, sizeof buf);      
      printf ("You entered: %s\n", buf);
      if (!strcmp (buf, "exit"))
        {
          printf ("exiting...\n");
          break;
        }
    } 
  pass ();
}

