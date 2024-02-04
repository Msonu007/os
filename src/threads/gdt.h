#ifndef THREADS_GDT_H_
#define THREADS_GDT_H_

#include "threads/loader.h"

struct cpu;

/* Segment selectors.
   More selectors are defined by the loader in loader.h. */
#define SEL_UCSEG       0x1B    /* User code selector. */
#define SEL_UDSEG       0x23    /* User data selector. */
#define SEL_TSS         0x28    /* Task-state segment. */
#define SEL_CNT         7       /* Number of segments. */

void gdt_init (void);
void gdt_refer_cpu(struct cpu *);

#endif /* THREADS_GDT_H_ */
