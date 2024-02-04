#ifndef THREADS_SPINLOCK_H
#define THREADS_SPINLOCK_H

#include <debug.h>
#include <stdbool.h>

/* A spinlock. */
struct spinlock
{
  int locked;           /* Is the lock held? */
  struct cpu *cpu;      /* CPU that acquired the lock, or NULL 
                           if spinlock is not held */
  
  /* For debugging. If the lock is held, then debuginfo
     contains the call stack of the thread when the lock
     was acquired. If lock is not held, contains the
     call stack of the last thread that released the lock */
  struct callerinfo debuginfo;
};

void spinlock_acquire (struct spinlock *);
bool spinlock_try_acquire (struct spinlock *);
bool spinlock_held_by_current_cpu (const struct spinlock *);
void spinlock_init (struct spinlock *);
void spinlock_release (struct spinlock *);

#endif /* threads/spinlock.h */
