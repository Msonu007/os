#include "threads/ipi.h"
#include "devices/lapic.h"
#include <stdbool.h>
#include "threads/interrupt.h"
#include <stdio.h>
#include "threads/cpu.h"
#include <debug.h>
#include "lib/kernel/x86.h"
#include "threads/synch.h"
#include "lib/atomic-ops.h"
#include "threads/mp.h"
#include "devices/shutdown.h"
#ifdef USERPROG
#include "userprog/pagedir.h"
#endif

static void ipi_debug (struct intr_frame *f UNUSED);
static void ipi_schedule (struct intr_frame *f UNUSED);
static void ipi_tlbflush (struct intr_frame *f UNUSED);
static void ipi_shutdown (struct intr_frame *f UNUSED);

/* Register interrupt handlers for the inter-processor
   interrupts that we support */
void
ipi_init ()
{
  intr_register_ipi (T_IPI + IPI_SHUTDOWN, ipi_shutdown,
                     "#IPI SHUTDOWN");
  intr_register_ipi (T_IPI + IPI_TLB, ipi_tlbflush,
                     "#IPI TLB");
  intr_register_ipi (T_IPI + IPI_DEBUG, ipi_debug,
                     "#IPI DEBUG");
  intr_register_ipi (T_IPI + IPI_SCHEDULE, ipi_schedule,
                     "#IPI SCHEDULE");
}

/* Received a shutdown signal from another CPU. */
static void
ipi_shutdown (struct intr_frame *f UNUSED)
{
  shutdown_handle_ipi ();
}

/* Received a request to flush TLB. */
static void
ipi_tlbflush (struct intr_frame *f UNUSED)
{
#ifdef USERPROG
  pagedir_handle_tlbflush_request ();
#endif
}

/* Preempt the currently running thread */
static void
ipi_schedule (struct intr_frame *f UNUSED)
{
  ASSERT (cpu_started_others);
  intr_yield_on_return ();
}

/* For debugging. Prints the backtrace of the thread running on the current CPU  */
static void
ipi_debug (struct intr_frame *f UNUSED)
{
  ASSERT (cpu_started_others);
  debug_backtrace_with_lock ();
}
