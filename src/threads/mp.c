/* This file is derived from source code for the xv6 instruction 
   operating system. The xv6 copyright notice is printed below.

   The xv6 software is:

   Copyright (c) 2006-2009 Frans Kaashoek, Robert Morris, Russ Cox,
                        Massachusetts Institute of Technology

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */
   

/* Multiprocessor support
   Search memory for MP description structures.
   http://developer.intel.com/design/pentium/datashts/24201606.pdf */
#include "mp.h"
#include "threads/cpu.h"
#include <string.h>
#include <stdio.h>
#include "threads/vaddr.h"
#include "threads/io.h"
#include <stdint.h>
#include "devices/lapic.h"
#include "devices/ioapic.h"

/* Return the number of the boot CPU. */
int
mp_bcpu (void)
{
  return bcpu - cpus;
}

static uint8_t
sum (uint8_t *addr, int len)
{
  int i, sum;

  sum = 0;
  for (i = 0; i < len; i++)
    sum += addr[i];
  return sum;
}

/* Look for an MP structure in the len bytes at addr. */
static struct mp*
mpsearch1 (uint32_t a, int len)
{
  uint8_t *e, *p, *addr;

  addr = ptov (a);
  e = addr + len;
  for (p = addr; p < e; p += sizeof(struct mp))
    if (memcmp (p, "_MP_", 4) == 0 && sum (p, sizeof(struct mp)) == 0)
      return (struct mp*) p;
  return 0;
}

/* Search for the MP Floating Pointer Structure, which according to the
   spec is in one of the following three locations:
   1) in the first KB of the EBDA;
   2) in the last KB of system base memory;
   3) in the BIOS ROM between 0xE0000 and 0xFFFFF. */
static struct mp*
mpsearch (void)
{
  uint8_t *bda;
  uint32_t p;
  struct mp *mp;

  bda = (uint8_t *) ptov (0x400);
  if ((p = ((bda[0x0F] << 8) | bda[0x0E]) << 4))
    {
      if ((mp = mpsearch1 (p, 1024)))
	return mp;
    }
  else
    {
      p = ((bda[0x14] << 8) | bda[0x13]) * 1024;
      if ((mp = mpsearch1 (p - 1024, 1024)))
	return mp;
    }
  return mpsearch1 (0xF0000, 0x10000);
}

/* Search for an MP configuration table.  For now,
   don't accept the default configurations (physaddr == 0).
   Check for correct signature, calculate the checksum and,
   if correct, check the version. */
static struct mpconf*
mpconfig (struct mp **pmp)
{
  struct mpconf *conf;
  struct mp *mp;

  if ((mp = mpsearch ()) == 0 || mp->physaddr == 0)
    return 0;
  conf = (struct mpconf*) ptov ((uint32_t) mp->physaddr);
  if (memcmp (conf, "PCMP", 4) != 0)
    return 0;
  if (conf->version != 1 && conf->version != 4)
    return 0;
  if (sum ((uint8_t*) conf, conf->length) != 0)
    return 0;
  *pmp = mp;
  return conf;
}

/* Parse the MP configuration table to find how how many
   CPUs are on this system */
void
mp_init (void)
{
  uint8_t *p, *e;
  struct mp *mp;
  struct mpconf *conf;
  struct mpproc *proc;
  struct mpioapic *ioapic;

  //Initialize all CPU data structures

  bcpu = &cpus[0];
  if ((conf = mpconfig (&mp)) == 0)
    return;
  cpu_ismp = 1;
  lapic_base_addr = (uint32_t*) conf->lapicaddr;
  for (p = (uint8_t*) (conf + 1), e = (uint8_t*) conf + conf->length; p < e;)
    {
      switch (*p)
	{
	case MPPROC:
	  proc = (struct mpproc*) p;
	  if (ncpu != proc->apicid)
	    {
	      printf ("mpinit: ncpu=%d apicid=%d\n", ncpu, proc->apicid);
	      cpu_ismp = 0;
	    }
	  if (proc->flags & MPBOOT)
	    bcpu = &cpus[ncpu];
	  memset(cpus, 0, sizeof(*cpus));
	  cpus[ncpu].id = ncpu;
	  ncpu++;
	  p += sizeof(struct mpproc);
	  continue;
	case MPIOAPIC:
	  ioapic = (struct mpioapic*) p;
	  ioapic_id = ioapic->apicno;
	  p += sizeof(struct mpioapic);
	  continue;
	case MPBUS:
	case MPIOINTR:
	case MPLINTR:
	  p += 8;
	  continue;
	default:
	  printf ("mpinit: unknown config type %x\n", *p);
	  cpu_ismp = 0;
	}
    }
  if (!cpu_ismp)
    {
      /* Didn't like what we found; fall back to no MP. */
      ncpu = 1;
      lapic_base_addr = 0;
      ioapic_id = 0;
      return;
    }

  if (mp->imcrp)
    {
      /* Bochs doesn't support IMCR, so this doesn't run on Bochs.
         But it would on real hardware. */
      outb (0x22, 0x70);     /* Select IMCR */
      outb (0x23, inb (0x23) | 1);     /* Mask external interrupts. */
    }
}
