#ifndef THREADS_SCHEDEVENTS_H_
#define THREADS_SCHEDEVENTS_H_

#include "threads/thread.h"
#include <stdio.h>
struct thread *driver_idle (void);
struct thread *driver_current (void);
void driver_init (void);
void driver_tick (void);
void driver_block (void);
void driver_yield (void);
struct thread *driver_create (const char *name, int priority);
void driver_unblock (struct thread *t);
void driver_exit (void);
void driver_set_nice (int nice);
int driver_get_nice (void);
void driver_interrupt_tick (void);
#endif /* THREADS_SCHEDEVENTS_H_ */
