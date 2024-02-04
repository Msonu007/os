/* Tests multiple file descriptors that refer to created
 * files.
 */
#include "tests/lib.h"
#include "tests/main.h"
#include <stdio.h>
#include <string.h>

#define N 6
#define FNAME_LEN 11
char filenames[N*3/2][FNAME_LEN];
char alphabet[2*26];

void
test_main (void)
{
  int i, j;
  for (i = 0; i < 2; i++)
    for (j = 'A'; j <= 'Z'; j++)
      alphabet[26*i+j-'A'] = j;

  int fd[N*3/2];
  // step 1. Create N files
  for (i = 0; i < N; i++)
    {
      snprintf(filenames[i], FNAME_LEN, "quux%02d.dat", i);
      CHECK (create (filenames[i], 26), "create %s", filenames[i]);
    }

  // step 2. Open N files and write data to them.
  for (i = 0; i < N; i++)
    {
      CHECK (fd[i] = open (filenames[i]), "open %s", filenames[i]);
      CHECK (write(fd[i], alphabet + i, 26) == 26, "write %s", filenames[i]);
    }

  // step 3. Close every other file
  for (i = 0; i < N; i+=2)
    {
      msg ("close %s", filenames[i]);
      close (fd[i]);
    }

  // step 4. Create and open more files.
  for (i = N; i < N*3/2; i++)
    {
      snprintf(filenames[i], FNAME_LEN, "quux%02d.dat", i);
      CHECK (create (filenames[i], 26), "create %s", filenames[i]);
      CHECK (fd[i] = open (filenames[i]), "open %s", filenames[i]);
      CHECK (write(fd[i], alphabet + i, 26) == 26, "write %s", filenames[i]);
    }

  // step 5. Rewind and read odd-numbered files
  for (i = 1; i < N; i+=2)
    {
      char buf[26];
      msg ("seek %s", filenames[i]);
      seek (fd[i], 0);
      read (fd[i], buf, 26);
      CHECK (memcmp (buf, alphabet + i, 26) == 0, "memcmp %s", filenames[i]);
    }

  // step 6. Open and close 3000 files, which must work
  // we don't output anything in order to not slow down the test too much
  const int times = 3000;
  msg ("starting %d open/close", times);
  for (i = 0; i < times; i++)
    {
      int fd = open (filenames[0]);
      if (fd < 0)
        fail("open#%d %s", i, filenames[0]);
      close (fd);
    }
  msg ("open/close completed");
}

