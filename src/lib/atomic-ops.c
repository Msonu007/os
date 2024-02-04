/*
 * This module defines helper methods for the commonly used 
 * atomic operations.
 * All of these atomic operations use the GCC atomic builtins
 * https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html#g_t_005f_005fatomic-Builtins
 * This library works on 32 bit signed integers. 
 * If you need other data types, you will have to call the GCC builtins directly
 */

#include <atomic-ops.h>
#include <stddef.h>
#include <debug.h>

/*
 * Use sequentially consistent ordering on all our atomic 
 * operations. It is the slowest, but guarantees the
 * least amount of compiler/hardware reordering.
 * It is also the most portable.
 * I would not recommend changing this unless you really
 * know what you're doing.
 * Read more about memory ordering:
 * http://en.cppreference.com/w/c/atomic/memory_order
 * https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
 */
#define SEQ __ATOMIC_SEQ_CST

/* Exchanges the value stored at addr with newval,
   and return the old value. */
inline int
atomic_xchg (int *addr, int newval)
{
  return __atomic_exchange_n (addr, newval, SEQ);
}

/* Atomically increment *NUM by one. Returns the new value. */
inline int
atomic_inci (int *num)
{
  return __atomic_add_fetch (num, 1, SEQ);
}

/* Atomically decrement *NUM by one. Returns the new value. */
inline int
atomic_deci (int *num)
{
  ASSERT (num != NULL);
  return __atomic_sub_fetch (num, 1, SEQ);
}

/* Atomically add AMT to NUM. Returns the new value. */
inline int
atomic_addi (int *num, int amt)
{
  return __atomic_add_fetch (num, amt, SEQ);
}

/* Compare-And-Swap (int): If *NUM == *OLD, set it to *NEW and return true.
   Else return false. */
inline bool
atomic_cmpxchg (int *num, int *old, int *new)
{
  return __atomic_compare_exchange (num, old, new, 0, SEQ, SEQ);
}

/* Atomic load. Returns *NUM */
inline int
atomic_load (int *num)
{
  return __atomic_load_n (num, SEQ);
}

/* Atomic store. */
inline void
atomic_store (int *num, int val)
{
  __atomic_store_n (num, val, SEQ);
}
