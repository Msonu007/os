#include "threads/gdt.h"
#include <debug.h>
#include "threads/tss.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "x86.h"
#include "threads/mp.h"
#include "devices/lapic.h"
#include <string.h>
#include "threads/thread.h"

/* The Global Descriptor Table (GDT).

   The GDT, an x86-specific structure, defines segments that can
   potentially be used by all processes in a system, subject to
   their permissions.  There is also a per-process Local
   Descriptor Table (LDT) but that is not used by modern
   operating systems.

   Each entry in the GDT, which is known by its byte offset in
   the table, identifies a segment.  For our purposes only three
   types of segments are of interest: code, data, and TSS or
   Task-State Segment descriptors.  The former two types are
   exactly what they sound like.  The TSS is used primarily for
   stack switching on interrupts.

   For more information on the GDT as used here, refer to
   [IA32-v3a] 3.2 "Using Segments" through 3.5 "System Descriptor
   Types". */

/* System segment or code/data segment? */
enum seg_class
{
  CLS_SYSTEM = 0, 	/* System segment. */
  CLS_CODE_DATA = 1 	/* Code or data segment. */
};

/* Limit has byte or 4 kB page granularity? */
enum seg_granularity
{
  GRAN_BYTE = 0, 	/* Limit has 1-byte granularity. */
  GRAN_PAGE = 1 	/* Limit has 4 kB granularity. */
};

/* GDT helpers. */
static uint64_t make_code_desc (int dpl);
static uint64_t make_data_desc (int dpl);
static uint64_t make_tss_desc (void *laddr);
static uint64_t make_gdtr_operand (uint16_t limit, void *base);
static uint64_t make_seg_desc (uint32_t base, uint32_t limit, enum seg_class class, int type,
	       int dpl, enum seg_granularity granularity);

/* Sets up a proper GDT.  The bootstrap loader's GDT didn't
   include user-mode selectors or a TSS, but we need both now. 
   This also sets up per-CPU variables */
void
gdt_init (void)
{
  uint64_t gdtr_operand;
  struct cpu *c = &cpus[lapic_get_cpuid ()];
  /* Initialize GDT. */
  memset(c->gdt, 0, sizeof *c->gdt * SEL_CNT);
  c->gdt[SEL_NULL / sizeof *c->gdt] = 0;
  c->gdt[SEL_KCSEG / sizeof *c->gdt] = make_code_desc (0);
  c->gdt[SEL_KDSEG / sizeof *c->gdt] = make_data_desc (0);
  c->gdt[SEL_UCSEG / sizeof *c->gdt] = make_code_desc (3);
  c->gdt[SEL_UDSEG / sizeof *c->gdt] = make_data_desc (3);
  c->gdt[SEL_TSS / sizeof *c->gdt] = make_tss_desc (&c->ts);
  /* Returns a descriptor for the per-cpu storage. See cpu.h for more info */
  c->gdt[SEL_KCPU / sizeof *c->gdt] = make_seg_desc ((uint32_t)&c->cpu, 8, CLS_CODE_DATA, 2, 3,
					       GRAN_PAGE);
  /* Load GDTR, TR.  See [IA32-v3a] 2.4.1 "Global Descriptor
   Table Register (GDTR)", 2.4.4 "Task Register (TR)", and
   6.2.4 "Task Register".  */
  gdtr_operand = make_gdtr_operand (sizeof c->gdt - 1, c->gdt);
  lgdt(gdtr_operand);
  ltr(SEL_TSS);
  loadgs (SEL_KCPU);
  set_cpu (c);
}

/* Force per cpu variable to point to c. Used for scheduler testing */
void
gdt_refer_cpu(struct cpu *c) {
  uint64_t gdtr_operand;
  c->cpu = c;
  memset(c->gdt, 0, sizeof *c->gdt * SEL_CNT);
  c->gdt[SEL_NULL / sizeof *c->gdt] = 0;
  c->gdt[SEL_KCSEG / sizeof *c->gdt] = make_code_desc (0);
  c->gdt[SEL_KDSEG / sizeof *c->gdt] = make_data_desc (0);
  c->gdt[SEL_UCSEG / sizeof *c->gdt] = make_code_desc (3);
  c->gdt[SEL_UDSEG / sizeof *c->gdt] = make_data_desc (3);
  c->gdt[SEL_TSS / sizeof *c->gdt] = make_tss_desc (&c->ts);
  /* Returns a descriptor for the per-cpu storage. See cpu.h for more info */
  c->gdt[SEL_KCPU / sizeof *c->gdt] = make_seg_desc ((uint32_t)&c->cpu, 8, CLS_CODE_DATA, 2, 3,
					       GRAN_PAGE);

  /* Load GDTR, TR.  See [IA32-v3a] 2.4.1 "Global Descriptor
   Table Register (GDTR)", 2.4.4 "Task Register (TR)", and
   6.2.4 "Task Register".  */
  gdtr_operand = make_gdtr_operand (sizeof c->gdt - 1, c->gdt);
  lgdt(gdtr_operand);
  ltr(SEL_TSS);
  loadgs (SEL_KCPU);
  set_cpu (c);
}

/* Returns a segment descriptor with the given 32-bit BASE and
   20-bit LIMIT (whose interpretation depends on GRANULARITY).
   The descriptor represents a system or code/data segment
   according to CLASS, and TYPE is its type (whose interpretation
   depends on the class).

   The segment has descriptor privilege level DPL, meaning that
   it can be used in rings numbered DPL or lower.  In practice,
   DPL==3 means that user processes can use the segment and
   DPL==0 means that only the kernel can use the segment.  See
   [IA32-v3a] 4.5 "Privilege Levels" for further discussion. */
static uint64_t
make_seg_desc (uint32_t base,
               uint32_t limit,
               enum seg_class class,
               int type,
               int dpl,
               enum seg_granularity granularity)
{
  uint32_t e0, e1;

  ASSERT (limit <= 0xfffff);
  ASSERT (class == CLS_SYSTEM || class == CLS_CODE_DATA);
  ASSERT (type >= 0 && type <= 15);
  ASSERT (dpl >= 0 && dpl <= 3);
  ASSERT (granularity == GRAN_BYTE || granularity == GRAN_PAGE);

  e0 = ((limit & 0xffff)             /* Limit 15:0. */
        | (base << 16));             /* Base 15:0. */

  e1 = (((base >> 16) & 0xff)        /* Base 23:16. */
        | (type << 8)                /* Segment type. */
        | (class << 12)              /* 0=system, 1=code/data. */
        | (dpl << 13)                /* Descriptor privilege. */
        | (1 << 15)                  /* Present. */
        | (limit & 0xf0000)          /* Limit 16:19. */
        | (1 << 22)                  /* 32-bit segment. */
        | (granularity << 23)        /* Byte/page granularity. */
        | (base & 0xff000000));      /* Base 31:24. */

  return e0 | ((uint64_t) e1 << 32);
}

/* Returns a descriptor for a readable code segment with base at
   0, a limit of 4 GB, and the given DPL. */
static uint64_t
make_code_desc (int dpl)
{
  return make_seg_desc (0, 0xfffff, CLS_CODE_DATA, 10, dpl, GRAN_PAGE);
}

/* Returns a descriptor for a writable data segment with base at
   0, a limit of 4 GB, and the given DPL. */
static uint64_t
make_data_desc (int dpl)
{
  return make_seg_desc (0, 0xfffff, CLS_CODE_DATA, 2, dpl, GRAN_PAGE);
}

/* Returns a descriptor for an "available" 32-bit Task-State
   Segment with its base at the given linear address, a limit of
   0x67 bytes (the size of a 32-bit TSS), and a DPL of 0.
   See [IA32-v3a] 6.2.2 "TSS Descriptor". */
static uint64_t
make_tss_desc (void *laddr)
{
  return make_seg_desc ((uint32_t) laddr, 0x67, CLS_SYSTEM, 9, 0, GRAN_BYTE);
}


/* Returns a descriptor that yields the given LIMIT and BASE when
   used as an operand for the LGDT instruction. */
static uint64_t
make_gdtr_operand (uint16_t limit, void *base)
{
  return limit | ((uint64_t) (uint32_t) base << 16);
}
