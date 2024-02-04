#include "devices/serial.h"
#include <debug.h>
#include "devices/input.h"
#include "devices/intq.h"
#include "devices/timer.h"
#include "threads/io.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/ioapic.h"
#include "threads/synch.h"
#include "threads/cpu.h"
#include "devices/trap.h"
#include <stdbool.h>

/* Register definitions for the 16550A UART used in PCs.
   The 16550A has a lot more going on than shown here, but this
   is all we need.

   Refer to [PC16650D] for hardware information. */

/* I/O port base address for the first serial port. */
#define IO_BASE 0x3f8

/* DLAB=0 registers. */
#define RBR_REG (IO_BASE + 0)   /* Receiver Buffer Reg. (read-only). */
#define THR_REG (IO_BASE + 0)   /* Transmitter Holding Reg. (write-only). */
#define IER_REG (IO_BASE + 1)   /* Interrupt Enable Reg.. */

/* DLAB=1 registers. */
#define LS_REG (IO_BASE + 0)    /* Divisor Latch (LSB). */
#define MS_REG (IO_BASE + 1)    /* Divisor Latch (MSB). */

/* DLAB-insensitive registers. */
#define IIR_REG (IO_BASE + 2)   /* Interrupt Identification Reg. (read-only) */
#define FCR_REG (IO_BASE + 2)   /* FIFO Control Reg. (write-only). */
#define LCR_REG (IO_BASE + 3)   /* Line Control Register. */
#define MCR_REG (IO_BASE + 4)   /* MODEM Control Register. */
#define LSR_REG (IO_BASE + 5)   /* Line Status Register (read-only). */

/* Interrupt Enable Register bits. */
#define IER_RECV 0x01           /* Interrupt when data received. */
#define IER_XMIT 0x02           /* Interrupt when transmit finishes. */

/* Line Control Register bits. */
#define LCR_N81 0x03            /* No parity, 8 data bits, 1 stop bit. */
#define LCR_DLAB 0x80           /* Divisor Latch Access Bit (DLAB). */

/* MODEM Control Register. */
#define MCR_OUT2 0x08           /* Output line 2. */

/* Line Status Register. */
#define LSR_DR 0x01             /* Data Ready: received data byte is in RBR. */
#define LSR_THRE 0x20           /* THR Empty. */

/* Transmission mode. */
static enum { UNINIT, POLL, QUEUE } mode;

/* Data to be transmitted. */
static struct intq txq;

static void set_serial (int bps);
static void putc_poll (uint8_t);
static void write_ier (bool could_send_more, bool could_receive_more);
static intr_handler_func serial_interrupt;

/* Initializes the serial port device for polling mode.
   Polling mode busy-waits for the serial port to become free
   before writing to it.  It's slow, but until interrupts have
   been initialized it's all we can do. */
static void
init_poll (void) 
{
  ASSERT (mode == UNINIT);
  outb (IER_REG, 0);                    /* Turn off all interrupts. */
  outb (FCR_REG, 0);                    /* Disable FIFO. */
  set_serial (9600);                    /* 9.6 kbps, N-8-1. */
  outb (MCR_REG, MCR_OUT2);             /* Required to enable interrupts. */
  intq_init (&txq);
  mode = POLL;
} 

/* Initializes the serial port device for queued interrupt-driven
   I/O.  With interrupt-driven I/O we don't waste CPU time
   waiting for the serial device to become ready. */
void
serial_init_queue (void) 
{
  if (mode == UNINIT)
    init_poll ();
  ASSERT (mode == POLL);
  mode = QUEUE;
  write_ier (false, true);
  intr_register_ext (0x20 + IRQ_SERIAL, serial_interrupt, "serial");
  ioapic_enable (IRQ_SERIAL, IRQ_CPU);
}

/* Normally, the serial driver will switch from polling mode to
   interrupt-driven mode when serial_init_queue() is called.
   By calling serial_enter_poll_mode () it is possible to reset
   the driver into polling mode. This will enable to call
   printf () even from inside the scheduler.
 */
void
serial_set_poll_mode (bool on)
{
  ASSERT (mode != UNINIT);
  mode = on ? POLL : QUEUE;
}

void
serial_txq_acquire (void) 
{
  intq_acquire (&txq);
}

void
serial_txq_release (void) 
{
  intq_release (&txq);
}

/* Sends BYTE to the serial port. */
void
serial_putc (uint8_t byte) 
{

  if (mode != QUEUE)
    {
      /* If we're not set up for interrupt-driven I/O yet,
         use dumb polling to transmit a byte. */
      if (mode == UNINIT)
        init_poll ();
      putc_poll (byte); 
    }
  else 
    {
      /* Otherwise, queue a byte and update the interrupt enable
         register. */
      enum intr_level old_level = intr_get_level ();
      serial_txq_acquire ();
      if (old_level == INTR_OFF && intq_full (&txq)) 
        {
          /* Interrupts are off; therefore we are likely
             in an interrupt handler, or else we are holding
             a spinlock. Transmit queue is
             full, so the next intq_putc would probably
             cause us to sleep. We can't sleep,
             so we'll send a character via polling instead. */
          putc_poll (intq_getc (&txq));
        }

      intq_putc (&txq, byte);
      serial_txq_release ();

      input_acquire ();
      bool could_receive_more = !input_full ();
      input_release ();
      write_ier (true, could_receive_more);
    }
}

/* Flushes anything in the serial buffer out the port in polling
   mode. */
void
serial_flush (void) 
{
  serial_txq_acquire ();
  while (!intq_empty (&txq))
    putc_poll (intq_getc (&txq));
  serial_txq_release ();
}

/* The fullness of the input buffer may have changed.
   Called by the input buffer routines when characters are added
   to or removed from the buffer. */
void
serial_notify (bool could_receive_more)
{
  serial_txq_acquire ();
  bool could_send_more = !intq_empty (&txq);
  serial_txq_release ();

  if (mode == QUEUE)
    write_ier (could_send_more, could_receive_more);
}

/* Configures the serial port for BPS bits per second. */
static void
set_serial (int bps)
{
  int base_rate = 1843200 / 16;         /* Base rate of 16550A, in Hz. */
  uint16_t divisor = base_rate / bps;   /* Clock rate divisor. */

  ASSERT (bps >= 300 && bps <= 115200);

  /* Enable DLAB. */
  outb (LCR_REG, LCR_N81 | LCR_DLAB);

  /* Set data rate. */
  outb (LS_REG, divisor & 0xff);
  outb (MS_REG, divisor >> 8);

  /* Reset DLAB. */
  outb (LCR_REG, LCR_N81);
}

/* Update interrupt enable register.  */
static void
write_ier (bool could_send_more, bool could_receive_more)
{
  uint8_t ier = 0;

  /* Enable transmit interrupt if we have any characters to
     transmit. */
  if (could_send_more)
    ier |= IER_XMIT;

  /* Enable receive interrupt if we have room to store any
     characters we receive. */
  if (could_receive_more)
    ier |= IER_RECV;
  
  outb (IER_REG, ier);
}

/* Polls the serial port until it's ready,
   and then transmits BYTE. */
static void
putc_poll (uint8_t byte) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  while ((inb (LSR_REG) & LSR_THRE) == 0)
    continue;
  outb (THR_REG, byte);
}

/* Serial interrupt handler. */
static void
serial_interrupt (struct intr_frame *f UNUSED) 
{
  /* Inquire about interrupt in UART.  Without this, we can
     occasionally miss an interrupt running under QEMU. */
  inb (IIR_REG);

  /* As long as we have room to receive a byte, and the hardware
     has a byte for us, receive a byte.  */
  input_acquire ();
  while (!input_full () && (inb (LSR_REG) & LSR_DR) != 0)
    input_putc (inb (RBR_REG));

  bool could_receive_more = !input_full ();
  input_release ();

  serial_txq_acquire ();
  /* As long as we have a byte to transmit, and the hardware is
     ready to accept a byte for transmission, transmit a byte. */
  while (!intq_empty (&txq) && (inb (LSR_REG) & LSR_THRE) != 0) 
    outb (THR_REG, intq_getc (&txq));

  bool could_send_more = !intq_empty (&txq);
  serial_txq_release ();

  /* Update interrupt enable register based on queue status.
   * This is not atomic with respect to the earlier call to input_full (),
   * but that's ok: in the worst case, we may allow an additional
   * interrupt but won't be able to put the received byte into the
   * queue.
   */
  write_ier (could_send_more, could_receive_more);
}
