#include "devices/input.h"
#include <debug.h>
#include "devices/intq.h"
#include "devices/serial.h"

/*
 * Stores keys input from the keyboard and serial port in a single buffer,
 * which is implemented as an interrupt queue, see intq.c
 */
static struct intq buffer;

/* Initializes the input buffer. */
void
input_init (void) 
{
  intq_init (&buffer);
}

/* Adds a key to the input buffer.
   Must have called input_acquire() and the buffer must not be full. */
void
input_putc (uint8_t key) 
{
  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (!intq_full (&buffer));
  intq_putc (&buffer, key);
}

/* Retrieves a key from the input buffer.
   If the buffer is empty, waits for a key to be pressed. */
uint8_t
input_getc (void) 
{
  uint8_t key;

  input_acquire ();
  key = intq_getc (&buffer);
  input_release ();
  serial_notify (true);
  return key;
}

/* Returns true if the input buffer is full, false otherwise.
   Must have called input_acquire(). */
bool
input_full (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);
  return intq_full (&buffer);
}

void
input_acquire (void)
{
  intq_acquire(&buffer);
}

void
input_release (void)
{
  intq_release(&buffer);
}
