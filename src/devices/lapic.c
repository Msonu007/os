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

/* The local Advanced Programmable Interrupt Controller
   manages internal (non-I/O) interrupts.
   Pintos uses it to provide local timers to each CPU, as well
   as to send Inter-Processor Interrupts (IPI).
   See Chapter 8 & Appendix C of Intel processor manual volume 3. */
#include "devices/lapic.h"
#include "devices/trap.h"
#include "threads/flags.h"
#include "x86.h"
#include <string.h>
#include <stdio.h>
#include "threads/vaddr.h"
#include "threads/io.h"
#include "threads/interrupt.h"
#include <debug.h>
#include "threads/cpu.h"
#include "lib/kernel/bitmap.h"
#include <stdint.h>
#include "threads/mp.h"
#include <stdbool.h>
#include "devices/timer.h"

/* Estimate of the bus frequency, used to calculate initial count */
#define BUS_FREQUENCY 1000000000
#define COUNT BUS_FREQUENCY/TIMER_FREQ

/* Local APIC registers, divided by 4 for use as uint32_t[] indices. */
#define ID      (0x0020/4)      /* ID */
#define VER     (0x0030/4)      /* Version */
#define TPR     (0x0080/4)      /* Task Priority */
#define EOI     (0x00B0/4)      /* EOI */
#define SVR     (0x00F0/4)      /* Spurious Interrupt Vector */
#define ENABLE     0x00000100   /* Unit Enable */
#define ESR     (0x0280/4)      /* Error Status */
#define ICRLO   (0x0300/4)      /* Interrupt Command */
#define INIT       0x00000500   /* INIT/RESET */
#define STARTUP    0x00000600   /* Startup IPI */
#define DELIVS     0x00001000   /* Delivery status */
#define ASSERTINTR     0x00004000   /* Assert interrupt (vs deassert) */
#define DEASSERT   0x00000000
#define LEVEL      0x00008000   /* Level triggered */
#define BCAST      0x00080000   /* Send to all APICs, including self. */
#define BUSY       0x00001000
#define FIXED      0x00000000
#define ICRHI   (0x0310/4)      /* Interrupt Command [63:32] */
#define TIMER   (0x0320/4)      /* Local Vector Table 0 (TIMER) */
#define X1         0x0000000B   /* divide counts by 1 */
#define ONESHOT  (0 << 17)      /* One shot timer */
#define PERIODIC   (1 << 17)    /* Periodic */
#define TSCDEADLINE   (2 << 17) //TSC deadline mode
#define PCINT   (0x0340/4)      /* Performance Counter LVT */
#define LINT0   (0x0350/4)      /* Local Vector Table 1 (LINT0) */
#define LINT1   (0x0360/4)      /* Local Vector Table 2 (LINT1) */
#define ERROR   (0x0370/4)      /* Local Vector Table 3 (ERROR) */
#define MASKED     0x00010000   /* Interrupt masked */
#define TICR    (0x0380/4)      /* Timer Initial Count */
#define TCCR    (0x0390/4)      /* Timer Current Count */
#define TDCR    (0x03E0/4)      /* Timer Divide Configuration */

static void microdelay (int);
static void lapicw (int , int);
static void sendipi (uint8_t , int);

volatile uint32_t *lapic_base_addr;

/* Initialize the local advanced interrupt controller. */
void
lapic_init (void)
{
  if (!lapic_base_addr)
    return;

  /* Enable local APIC; set spurious interrupt vector. */
  lapicw (SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

  /* The timer repeatedly counts down at bus frequency
     from lapic[TICR] and then issues an interrupt.
     If PintOS cared more about precise timekeeping,
     TICR would be calibrated using an external time source. */
  lapicw (TDCR, X1);
  lapicw (TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
  lapicw (TICR, COUNT);

  /* Disable logical interrupt lines. */
  lapicw (LINT0, MASKED);
  lapicw (LINT1, MASKED);

  /* Disable performance counter overflow interrupts
   on machines that provide that interrupt entry. */
  if (((lapic_base_addr[VER] >> 16) & 0xFF) >= 4)
    lapicw (PCINT, MASKED);

  /* Map error interrupt to IRQ_ERROR. */
  lapicw (ERROR, T_IRQ0 + IRQ_ERROR);

  /* Clear error status register (requires back-to-back writes). */
  lapicw (ESR, 0);
  lapicw (ESR, 0);

  /* Ack any outstanding interrupts. */
  lapicw (EOI, 0);

  /* Send an Init Level De-Assert to synchronise arbitration ID's. */
  lapicw (ICRHI, 0);
  lapicw (ICRLO, BCAST | INIT | LEVEL);
  while (lapic_base_addr[ICRLO] & DELIVS)
    ;

  /* Enable interrupts on the APIC (but not on the processor). */
  lapicw (TPR, 0);
}

/* COMMENT missing.  Remove if unused. */
void
lapic_set_next_event (uint32_t delta)
{
  lapicw (TICR, delta);
}

int
lapic_get_cpuid (void)
{
  /* Cannot call cpu when interrupts are enabled:
     result not guaranteed to last long enough to be used!
     Would prefer to panic but even printing is chancy here:
     almost everything, including printf and panic, calls cpu,
     often indirectly through acquire and release. */
  if (intr_get_level () == INTR_ON)
    {
      static int n;
      if (n++ == 0)
        {
          PANIC("cpu called from %p with interrupts enabled\n",
                __builtin_return_address (0));
        }
    }

  if (lapic_base_addr)
    return lapic_base_addr[ID] >> 24;
  return 0;
}

/* Acknowledge interrupt. */
void
lapic_ack (void)
{
  if (lapic_base_addr)
    lapicw (EOI, 0);
}

#define CMOS_PORT    0x70
#define CMOS_RETURN  0x71

/* Start additional processor running entry code at addr.
   See Appendix B of MultiProcessor Specification. */
void
lapic_start_ap (uint8_t apicid, uint32_t addr)
{
  int i;
  uint16_t *wrv;

  /* "The BSP must initialize CMOS shutdown code to 0AH */
  /* and the warm reset vector (DWORD based at 40:67) to point at */
  /* the AP startup code prior to the [universal startup algorithm]." */
  outb (CMOS_PORT, 0xF);     /* offset 0xF is shutdown code */
  outb (CMOS_PORT + 1, 0x0A);
  wrv = (uint16_t*) ptov((0x40 << 4 | 0x67));     /* Warm reset vector */
  wrv[0] = 0;
  wrv[1] = addr >> 4;

  /* "Universal startup algorithm." */
  /* Send INIT (level-triggered) interrupt to reset other CPU. */
  lapicw (ICRHI, apicid << 24);
  lapicw (ICRLO, INIT | LEVEL | ASSERTINTR);
  microdelay (200);
  lapicw (ICRLO, INIT | LEVEL);
  microdelay (100);     /* should be 10ms, but too slow in Bochs! */

  /* Send startup IPI (twice!) to enter code.
     Regular hardware is supposed to only accept a STARTUP
     when it is in the halted state due to an INIT.  So the second
     should be ignored, but it is part of the official Intel algorithm.
     Bochs complains about the second one.  Too bad for Bochs. */
  for (i = 0; i < 2; i++)
    {
      lapicw (ICRHI, apicid << 24);
      lapicw (ICRLO, STARTUP | (addr >> 12));
      microdelay (200);
    }
}

/* Send an IPI to a CPU specific by its ID. */
void
lapic_send_ipi_to(int irq, uint8_t cpu_id)
{
  ASSERT (irq < NUM_IPI);
  sendipi (cpu_id, irq);
}

/* Sends interrupt irq to all cpus except the sender. Does so by looping over
   cpu structs and sending a signal separately to each one. */
void
lapic_send_ipi_to_all_but_self (int irq)
{
  ASSERT (irq < NUM_IPI);
  if (!cpu_started_others)
    return;
  struct cpu *c;
  intr_disable_push ();
  for (c = cpus; c < cpus + ncpu; c++)
    {
      if (c != get_cpu ())
        sendipi (c->id, irq);
    }
  intr_enable_pop ();
}

/* Sends interrupt irq to the cpus defined by mask */
static void
sendipi_mask (int irq, struct bitmap *mask)
{
  ASSERT(irq < NUM_IPI);
  ASSERT (bitmap_size (mask) <= ncpu);
  if (!cpu_started_others)
    return;

  size_t i;
  intr_disable_push ();
  for (i = 0; i < bitmap_size (mask); i++)
    {
      if (bitmap_test (mask, i))
        sendipi (i, irq);
    }
  intr_enable_pop ();
}

/* Sends interrupt irq to all cpus */
void
lapic_send_ipi_to_all (int irq)
{
  struct bitmap *mask = bitmap_create (ncpu);
  bitmap_set_all (mask, true);
  sendipi_mask (irq, mask);
  bitmap_destroy (mask);
}

static void
lapicw (int index, int value)
{
  lapic_base_addr[index] = value;
  lapic_base_addr[ID];     /* wait for write to finish, by reading */
}

static void
sendipi (uint8_t apicid, int irq)
{
  intr_disable_push ();
  lapicw (ICRHI, apicid << 24);
  lapicw (ICRLO, T_IPI + irq);
  while (lapic_base_addr[ICRLO] & DELIVS)
    ;
  microdelay (200);
  intr_enable_pop ();
}

/* Wait for a given number of microseconds.
   On real hardware would want to tune this dynamically. */
static void
microdelay (int us UNUSED)
{
}
