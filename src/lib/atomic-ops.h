#ifndef __LIB_ATOMIC_OPS_H
#define __LIB_ATOMIC_OPS_H

#include <stdbool.h>
#include <stdint.h>

int atomic_xchg (int *, int);
int atomic_inci (int *);
int atomic_deci (int *);

int atomic_addi (int *, int);
bool atomic_cmpxchg (int *, int *, int *);
int atomic_load (int *);
void atomic_store (int *, int);

#endif /* lib/atomic-ops.h */
