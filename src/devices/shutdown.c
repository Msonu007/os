#include "devices/shutdown.h"
#include <console.h>
#include <stdio.h>
#include "devices/kbd.h"
#include <stdbool.h>
#include "devices/serial.h"
#include "devices/lapic.h"
#include "devices/timer.h"
#include "threads/io.h"
#include "threads/thread.h"
#include "threads/cpu.h"
#ifdef USERPROG
#include "userprog/exception.h"
#endif
#ifdef FILESYS
#include "devices/block.h"
#include "filesys/filesys.h"
#endif

/* Keyboard control register port. */
#define CONTROL_REG 0x64
/* How to shut down when shutdown() is called. */
static enum shutdown_type how = SHUTDOWN_NONE;
static void print_stats (void);

/* Shuts down the machine in the way configured by
 shutdown_configure().  If the shutdown type is SHUTDOWN_NONE
 (which is the default), returns without doing anything. */
void
shutdown (void)
{

  switch (how)
    {
    case SHUTDOWN_POWER_OFF:
      shutdown_power_off ();
      break;

    case SHUTDOWN_REBOOT:
      shutdown_reboot ();
      break;

    default:
      /* Nothing to do. */
      break;
    }
}

/* Sets TYPE as the way that machine will shut down when Pintos
   execution is complete. */
void
shutdown_configure (enum shutdown_type type)
{
  how = type;
}

/* Reboots the machine via the keyboard controller. */
void
shutdown_reboot (void)
{
  printf ("Rebooting...\n");

    /* See [kbd] for details on how to program the keyboard
     * controller. */
  for (;;)
    {
      int i;

      /* Poll keyboard controller's status byte until
       * 'input buffer empty' is reported. */
      for (i = 0; i < 0x10000; i++)
        {
          if ((inb (CONTROL_REG) & 0x02) == 0)
            break;
          timer_udelay (2);
        }

      timer_udelay (50);

      /* Pulse bit 0 of the output port P2 of the keyboard controller.
       * This will reset the CPU. */
      outb (CONTROL_REG, 0xfe);
      timer_udelay (50);
    }
}

/* Received a shutdown signal from another CPU
   For now, it's just going to disable interrupts and spin so that
   the CPU stats remain consistent for the CPU that called shutdown() */
void
shutdown_handle_ipi (void)
{
  ASSERT(cpu_started_others);
  /* CPU0 needs to stay on in order to handle interrupts. Otherwise
     if an AP calls shutdown, it may get stuck trying to print to console */
  if (get_cpu ()->id == 0)
    return;
  while (1)
    ;
}

/* Powers down the machine we're running on,
   as long as we're running on Bochs or QEMU. */
void
shutdown_power_off (void)
{
  const char s[] = "Shutdown";
  const char *p;

#ifdef FILESYS
  filesys_done ();
#endif
  
  if (cpu_started_others)
    {
      lapic_send_ipi_to_all_but_self (IPI_SHUTDOWN);
    }
  print_stats ();

  printf ("Powering off...\n");
  serial_flush ();

  /* Shutdown sequence for QEMU 2.x */
  outw (0x604, 0x2000);

  /* Shutdown sequence for QEMU 1.6-1.7 */
  outw (0xB004, 0x2000);

  /* This is a special power-off sequence supported by Bochs and
     QEMU 1.0, but not by physical hardware. */
  for (p = s; *p != '\0'; p++)
    outb (0x8900, *p);

  /* This will power off a VMware VM if "gui.exitOnCLIHLT = TRUE"
     is set in its configuration file.  (The "pintos" script does
     that automatically.)  */
  asm volatile ("cli; hlt" : : : "memory");

  /* None of those worked. */
  printf ("still running...\n");
  for (;;);
}

/* Print statistics about Pintos execution. */
static void
print_stats (void)
{
  timer_print_stats ();
  thread_print_stats ();
#ifdef FILESYS
  block_print_stats ();
#endif
  console_print_stats ();
  kbd_print_stats ();
#ifdef USERPROG
  exception_print_stats ();
#endif
}
