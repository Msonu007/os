#include "threads/interrupt.h"
#include <debug.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include "threads/flags.h"
#include "threads/intr-stubs.h"
#include "threads/io.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/timer.h"
#include "devices/lapic.h"
#include "devices/ioapic.h"
#include "lib/kernel/x86.h"
#include "threads/cpu.h"
#include "threads/ipi.h"

/* Programmable Interrupt Controller (PIC) registers.
   A PC has two PICs, called the master and slave PICs, with the
   slave attached ("cascaded") to the master IRQ line 2. */
#define PIC0_CTRL	0x20    /* Master PIC control register address. */
#define PIC0_DATA	0x21    /* Master PIC data register address. */
#define PIC1_CTRL	0xa0    /* Slave PIC control register address. */
#define PIC1_DATA	0xa1    /* Slave PIC data register address. */

/* Number of x86 interrupts. */
#define INTR_CNT 256

/* Operand for lidt instruction, same for all CPUs*/
static uint64_t idtr_operand;

/* The Interrupt Descriptor Table (IDT).  The format is fixed by
   the CPU.  See [IA32-v3a] sections 5.10 "Interrupt Descriptor
   Table (IDT)", 5.11 "IDT Descriptors", 5.12.1.2 "Flag Usage By
   Exception- or Interrupt-Handler Procedure". */
static uint64_t idt[INTR_CNT];

/* Interrupt handler functions for each interrupt. */
static intr_handler_func *intr_handlers[INTR_CNT];

/* Names for each interrupt, for debugging purposes. */
static const char *intr_names[INTR_CNT];

/* Number of unexpected interrupts for each vector.  An
   unexpected interrupt is one that has no registered handler. */
static unsigned int unexpected_cnt[INTR_CNT];

static void pic8259_disable (void);

/* Interrupt Descriptor Table helpers. */
static uint64_t make_intr_gate (void (*) (void), int dpl);
static uint64_t make_trap_gate (void (*) (void), int dpl);
static inline uint64_t make_idtr_operand (uint16_t limit, void *base);

static void set_intr_context (bool context);
static bool yield_on_return (void);
static void set_yield_on_return (bool yield);

/* Interrupt handlers. */
void intr_handler (struct intr_frame *args);
static void unexpected_interrupt (const struct intr_frame *);

/* Returns the current interrupt status. */
enum intr_level
intr_get_level (void) 
{
  uint32_t flags;
  flags = readeflags ();
  return flags & FLAG_IF ? INTR_ON : INTR_OFF;
}

/* Disables interrupts */
void
intr_disable (void) 
{
  cli();
}

/* Enables interrupts */
void
intr_enable (void) 
{
  sti();
}

/* Disable interrupts and increment per-CPU nesting count.
 * If nesting count was zero, store the current interrupt level.
 *
 * intr_disable_push is used in conjunction with intr_enable_pop.
 * It allows a function to disable interrupts for its execution
 * and safely restore the interrupt level that was in effect upon
 * entry.
 */
void
intr_disable_push (void)
{
  enum intr_level old_level = intr_get_level ();
  cli ();
  if (get_cpu ()->ncli++ == 0)
    get_cpu ()->intena = old_level;
}

/*
 * Decrement per-CPU nesting count.
 * If nesting count reaches zero, restore interrupt level
 * to the state it was when intr_disable_push was called.
 */
void
intr_enable_pop (void)
{
  ASSERT (intr_get_level () == INTR_OFF);

  if (--get_cpu ()->ncli < 0)
    PANIC("unmatched call to intr_enable_pop");
  if (get_cpu ()->ncli == 0 && get_cpu ()->intena)
    sti ();
}

/* Initializes the interrupt system. */
void
intr_init (void)
{
  
  int i;

  /* First, disable the PIC8259, which could give out spurious interrupts */
  pic8259_disable ();
  
  /* Initialize the advanced programmable interrupt controller. */
  ioapic_init ();

  /* Initialize IDT. */
  for (i = 0; i < INTR_CNT; i++)
    idt[i] = make_intr_gate (intr_stubs[i], 0);

  /* Load IDT register.
     See [IA32-v2a] "LIDT" and [IA32-v3a] 5.10 "Interrupt
     Descriptor Table (IDT)". */
  idtr_operand = make_idtr_operand (sizeof idt - 1, idt);
  intr_load_idt ();

  /* Initialize intr_names. */
  for (i = 0; i < INTR_CNT; i++)
    intr_names[i] = "unknown";
  intr_names[0] = "#DE Divide Error";
  intr_names[1] = "#DB Debug Exception";
  intr_names[2] = "NMI Interrupt";
  intr_names[3] = "#BP Breakpoint Exception";
  intr_names[4] = "#OF Overflow Exception";
  intr_names[5] = "#BR BOUND Range Exceeded Exception";
  intr_names[6] = "#UD Invalid Opcode Exception";
  intr_names[7] = "#NM Device Not Available Exception";
  intr_names[8] = "#DF Double Fault Exception";
  intr_names[9] = "Coprocessor Segment Overrun";
  intr_names[10] = "#TS Invalid TSS Exception";
  intr_names[11] = "#NP Segment Not Present";
  intr_names[12] = "#SS Stack Fault Exception";
  intr_names[13] = "#GP General Protection Exception";
  intr_names[14] = "#PF Page-Fault Exception";
  intr_names[16] = "#MF x87 FPU Floating-Point Error";
  intr_names[17] = "#AC Alignment Check Exception";
  intr_names[18] = "#MC Machine-Check Exception";
  intr_names[19] = "#XF SIMD Floating-Point Exception";
}

void
intr_load_idt (void)
{
  lidt (idtr_operand);
}

/* Registers interrupt VEC_NO to invoke HANDLER with descriptor
   privilege level DPL.  Names the interrupt NAME for debugging
   purposes.  The interrupt handler will be invoked with
   interrupt status set to LEVEL. */
static void
register_handler (uint8_t vec_no, int dpl, enum intr_level level,
                  intr_handler_func *handler, const char *name)
{
  ASSERT (intr_handlers[vec_no] == NULL);
  if (level == INTR_ON)
    idt[vec_no] = make_trap_gate (intr_stubs[vec_no], dpl);
  else
    idt[vec_no] = make_intr_gate (intr_stubs[vec_no], dpl);
  intr_handlers[vec_no] = handler;
  intr_names[vec_no] = name;
}

/* Registers external interrupt VEC_NO to invoke HANDLER, which
   is named NAME for debugging purposes.  The handler will
   execute with interrupts disabled. */
void
intr_register_ext (uint8_t vec_no, intr_handler_func *handler,
                   const char *name) 
{
  ASSERT (vec_no >= 0x20 && vec_no <= 0x2f);
  register_handler (vec_no, 0, INTR_OFF, handler, name);
}

/* Registers internal interrupt VEC_NO to invoke HANDLER, which
   is named NAME for debugging purposes.  The interrupt handler
   will be invoked with interrupt status LEVEL.

   The handler will have descriptor privilege level DPL, meaning
   that it can be invoked intentionally when the processor is in
   the DPL or lower-numbered ring.  In practice, DPL==3 allows
   user mode to invoke the interrupts and DPL==0 prevents such
   invocation.  Faults and exceptions that occur in user mode
   still cause interrupts with DPL==0 to be invoked.  See
   [IA32-v3a] sections 4.5 "Privilege Levels" and 4.8.1.1
   "Accessing Nonconforming Code Segments" for further
   discussion. */
void
intr_register_int (uint8_t vec_no, int dpl, enum intr_level level,
                   intr_handler_func *handler, const char *name)
{
  ASSERT (vec_no < 0x20 || vec_no > 0x2f);
  register_handler (vec_no, dpl, level, handler, name);
}

/* Registers interprocessor interrupts. They are similar to 
   external interrupts in the sense that they are asynchronous
   and can be blocked, and they run with interrupts off, and 
   require dpl 0. */
void
intr_register_ipi (uint8_t vec_no, intr_handler_func *handler, const char *name)
{
  ASSERT(vec_no >= T_IPI);
  register_handler (vec_no, 0, INTR_OFF, handler, name);
}

/* Returns true during processing of an external interrupt
   and false at all other times.
   External interrupts are those generated by devices outside the
   CPU, such as the timer.  External interrupts run with
   interrupts turned off, so they never nest, nor are they ever
   pre-empted.  Handlers for external interrupts also may not
   sleep, although they may invoke intr_yield_on_return() to
   request that a new process be scheduled just before the
   interrupt returns. */
bool
intr_context (void) 
{
  intr_disable_push ();
  bool in_external_intr = get_cpu ()->in_external_intr;
  intr_enable_pop ();
  return in_external_intr;
}

/* When called during processing of an external interrupt,
   directs the interrupt handler to yield to a new process
   just before returning from the interrupt.
   When called from a process context, the subsequent call
   to intr_yield_if_requested () will yield. */
void
intr_yield_on_return (void) 
{
  set_yield_on_return (true);
}


/* Creates an gate that invokes FUNCTION.

   The gate has descriptor privilege level DPL, meaning that it
   can be invoked intentionally when the processor is in the DPL
   or lower-numbered ring.  In practice, DPL==3 allows user mode
   to call into the gate and DPL==0 prevents such calls.  Faults
   and exceptions that occur in user mode still cause gates with
   DPL==0 to be invoked.  See [IA32-v3a] sections 4.5 "Privilege
   Levels" and 4.8.1.1 "Accessing Nonconforming Code Segments"
   for further discussion.

   TYPE must be either 14 (for an interrupt gate) or 15 (for a
   trap gate).  The difference is that entering an interrupt gate
   disables interrupts, but entering a trap gate does not.  See
   [IA32-v3a] section 5.12.1.2 "Flag Usage By Exception- or
   Interrupt-Handler Procedure" for discussion. */
static uint64_t
make_gate (void (*function) (void), int dpl, int type)
{
  uint32_t e0, e1;

  ASSERT (function != NULL);
  ASSERT (dpl >= 0 && dpl <= 3);
  ASSERT (type >= 0 && type <= 15);

  e0 = (((uint32_t) function & 0xffff)     /* Offset 15:0. */
        | (SEL_KCSEG << 16));              /* Target code segment. */

  e1 = (((uint32_t) function & 0xffff0000) /* Offset 31:16. */
        | (1 << 15)                        /* Present. */
        | ((uint32_t) dpl << 13)           /* Descriptor privilege level. */
        | (0 << 12)                        /* System. */
        | ((uint32_t) type << 8));         /* Gate type. */

  return e0 | ((uint64_t) e1 << 32);
}

/* Creates an interrupt gate that invokes FUNCTION with the given
   DPL. */
static uint64_t
make_intr_gate (void (*function) (void), int dpl)
{
  return make_gate (function, dpl, 14);
}

/* Creates a trap gate that invokes FUNCTION with the given
   DPL. */
static uint64_t
make_trap_gate (void (*function) (void), int dpl)
{
  return make_gate (function, dpl, 15);
}

/* Returns a descriptor that yields the given LIMIT and BASE when
   used as an operand for the LIDT instruction. */
static inline uint64_t
make_idtr_operand (uint16_t limit, void *base)
{
  return limit | ((uint64_t) (uint32_t) base << 16);
}

/* Interrupt handlers. */

/* Handler for all interrupts, faults, and exceptions.  This
   function is called by the assembly language interrupt stubs in
   intr-stubs.S.  FRAME describes the interrupt and the
   interrupted thread's registers. */
void
intr_handler (struct intr_frame *frame) 
{
  bool external, ipi;
  intr_handler_func *handler;

  ipi = frame->vec_no >= T_IPI && frame->vec_no < T_IPI + NUM_IPI;
  
  /* External interrupts are special.
     We only handle one at a time (so interrupts must be off)
     and they need to be acknowledged on the PIC (see below).
     An external interrupt handler cannot sleep. */
  external = frame->vec_no >= 0x20 && frame->vec_no < 0x30;
  if (external || ipi) 
    {
      ASSERT (intr_get_level () == INTR_OFF);
      ASSERT (!intr_context ());

      set_intr_context (true);
    }

  /* Invoke the interrupt's handler. */
  handler = intr_handlers[frame->vec_no];
  if (handler != NULL)
    handler (frame);
  else if (frame->vec_no == 0x27 || frame->vec_no == 0x2f)
    {
      /* There is no handler, but this interrupt can trigger
         spuriously due to a hardware fault or hardware race
         condition.  Ignore it. */
    }
  else
    unexpected_interrupt (frame);

  /* Complete the processing of an external interrupt. */
  if (external || ipi) 
    {
      ASSERT (intr_get_level () == INTR_OFF);
      ASSERT (intr_context ());

      set_intr_context (false);
      lapic_ack ();

      if (yield_on_return ())
        {
          set_yield_on_return (false);
          thread_yield ();
        }
    }
}

/* Handles an unexpected interrupt with interrupt frame F.  An
   unexpected interrupt is one that has no registered handler. */
static void
unexpected_interrupt (const struct intr_frame *f)
{
  /* Count the number so far. */
  unsigned int n = ++unexpected_cnt[f->vec_no];

  /* If the number is a power of 2, print a message.  This rate
     limiting means that we get information about an uncommon
     unexpected interrupt the first time and fairly often after
     that, but one that occurs many times will not overwhelm the
     console. */
  if ((n & (n - 1)) == 0)
    printf ("Unexpected interrupt %#04x (%s)\n",
    f->vec_no, intr_names[f->vec_no]);
}

/* Dumps interrupt frame F to the console, for debugging. */
void
intr_dump_frame (const struct intr_frame *f) 
{
  uint32_t cr2;

  /* Store current value of CR2 into `cr2'.
     CR2 is the linear address of the last page fault.
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.14 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (cr2));

  printf ("Interrupt %#04x (%s) at eip=%p\n",
          f->vec_no, intr_names[f->vec_no], f->eip);
  printf (" cr2=%08"PRIx32" error=%08"PRIx32"\n", cr2, f->error_code);
  printf (" eax=%08"PRIx32" ebx=%08"PRIx32" ecx=%08"PRIx32" edx=%08"PRIx32"\n",
          f->eax, f->ebx, f->ecx, f->edx);
  printf (" esi=%08"PRIx32" edi=%08"PRIx32" esp=%08"PRIx32" ebp=%08"PRIx32"\n",
          f->esi, f->edi, (uint32_t) f->esp, f->ebp);
  printf (" cs=%04"PRIx16" ds=%04"PRIx16" es=%04"PRIx16" ss=%04"PRIx16"\n",
          f->cs, f->ds, f->es, f->ss);
}

/* Returns the name of interrupt VEC. */
const char *
intr_name (uint8_t vec) 
{
  return intr_names[vec];
}

/* return whether an interrupt vector is registered */
bool intr_is_registered(uint8_t vec_no)
{
  return (intr_handlers[vec_no] != NULL);
}

/* Disable the old 8259 PIC, which doesn't support SMP */
static void
pic8259_disable (void)
{
  /* Mask all interrupts on PIC1 */
  outb (PIC1_DATA, 0xFF);
  
  /* Mask all interrupts on PIC0 */
  outb (PIC0_DATA, 0xFF);

  /* Even though all interrupts are masked on both PIC's, interrupts fired
     before masking will still be delivered. Thus we still need to 
     remap the PICs so that they won't be interpreted by the OS as exceptions */

  /* Initialize master. */
  outb (PIC0_CTRL, 0x11); /* ICW1: single mode, edge triggered, expect ICW4. */
  outb (PIC0_DATA, 0x20); /* ICW2: line IR0...7 -> irq 0x20...0x27. */
  outb (PIC0_DATA, 0x04); /* ICW3: slave PIC on line IR2. */
  outb (PIC0_DATA, 0x01); /* ICW4: 8086 mode, normal EOI, non-buffered. */

  /* Initialize slave. */
  outb (PIC1_CTRL, 0x11); /* ICW1: single mode, edge triggered, expect ICW4. */
  outb (PIC1_DATA, 0x28); /* ICW2: line IR0...7 -> irq 0x28...0x2f. */
  outb (PIC1_DATA, 0x02); /* ICW3: slave ID is 2. */
  outb (PIC1_DATA, 0x01); /* ICW4: 8086 mode, normal EOI, non-buffered. */
}

/* Set intr_context status */
static void
set_intr_context (bool context)
{
  ASSERT (intr_get_level () == INTR_OFF);
  get_cpu ()->in_external_intr = context;
}

/* Return yield_on_return status, which 
   specifies whether the current thread should yield
   the cpu at the end of an external interrupt */
static bool
yield_on_return (void)
{
  ASSERT (intr_get_level () == INTR_OFF);
  return get_cpu ()->yield_on_return;
}

/* Set yield_on_return status */
static void
set_yield_on_return (bool yield)
{
  ASSERT (intr_get_level () == INTR_OFF);
  get_cpu ()->yield_on_return = yield;
}

/* Check if a yield was requested on the current CPU and
   yield if so.  This may trigger a context switch, as such it
   is illegal to call it while holding any spinlock or
   while having interrupts disabled.

   This is only tangentially related to interrupt handling,
   but (for now) it is in this file since the yield_on_return 
   flag is owned by interrupt.c.
 */
void
intr_yield_if_requested (void)
{
  /* Ignore the request to yield while interrupts are disabled,
     for instance, if intr_yield_if_requested() is called from
     an interrupt handler via sema_up().

     As a side effect, this check also prevents calling
     thread_yield() while the CFS simulator is active. */
  if (intr_get_level () == INTR_OFF)
    return;

  intr_disable_push ();
  bool should_yield = yield_on_return ();
  set_yield_on_return (false);
  intr_enable_pop ();
  if (should_yield)
    thread_yield ();
}
