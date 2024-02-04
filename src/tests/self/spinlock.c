/*
 * Tests spinlock_try_acquire, which wasn't implemented in xv6. 
 */
#include "threads/synch.h"
#include "tests.h"
#include "threads/cpu.h"
#include "threads/interrupt.h"
#include "lib/kernel/list.h"
#include "threads/malloc.h"
#include <string.h>

static void
switch_cpu (struct cpu *cpu)
{
  gdt_refer_cpu (cpu);
}

static void
test_trylock (void)
{
  intr_disable_push ();
  struct spinlock lock;
  spinlock_init (&lock);
  struct cpu vcpu1;
  struct cpu vcpu2;
  memset (&vcpu1, 0, sizeof (struct cpu));
  memset (&vcpu2, 0, sizeof (struct cpu));
  struct cpu *real = get_cpu ();
  switch_cpu (&vcpu1);
  fail_if_false (spinlock_try_acquire (&lock),
	       "Failed to acquire an unlocked spinlock");
  fail_if_false (spinlock_held_by_current_cpu (&lock),
	       "Failed to show spinlock held by CPU");
  switch_cpu (&vcpu2);
  fail_if_false (!spinlock_try_acquire (&lock), "Acquired a locked spinlock");
  switch_cpu (real);
  intr_enable_pop ();
}

struct shared_info {
  struct list_elem elem;
  int num;
};
static struct semaphore finished_sema;
static struct spinlock shared_lock;
static struct list shared_list;
static int shared_counter;
static bool done;
#define FINAL_VALUE 20000
#define NUM_THREADS 20
#define INC_EACH FINAL_VALUE/NUM_THREADS

static void
check_num (void) 
{
  if (list_empty (&shared_list)) {
      done = true;
      return;
  }
  struct list_elem *e = list_pop_front (&shared_list);
  struct shared_info *f = list_entry (e, struct shared_info, elem);
  fail_if_false (f->num == shared_counter, "Number from list is wrong");
  shared_counter++;  
}
static void
inc_shared (void *aux UNUSED) 
{
  while (!done) {
      spinlock_acquire (&shared_lock);
      check_num ();
      spinlock_release (&shared_lock);
  } 
  sema_up (&finished_sema);
}

static void
inc_shared_try (void *aux UNUSED)
{
  while (!done) {
      if (spinlock_try_acquire (&shared_lock)) {
	  check_num ();
	  spinlock_release (&shared_lock);
      }
  }
  sema_up (&finished_sema);
}


/* Twenty threads increment a shared counter. Test that it is correct */
static void
test_inc_shared (void)
{

  list_init (&shared_list);
  sema_init (&finished_sema, 0);
  spinlock_init (&shared_lock);
  shared_counter = 0;
  done = false;
  int i;
  
  struct shared_info *info = malloc (FINAL_VALUE * sizeof (*info));
  for (i=0;i<FINAL_VALUE;i++) {
      info[i].num = i;
      list_push_back (&shared_list, &info[i].elem);
  }

  for(i=0;i<NUM_THREADS;i++) {
      thread_func *func = i % 2 == 0 ? inc_shared : inc_shared_try;
      thread_create ("inc_shared", NICE_DEFAULT, func, NULL);    
  }
  for(i=0;i<NUM_THREADS;i++) {
      sema_down (&finished_sema);
  }
  fail_if_false (list_empty (&shared_list), "List should be empty");
  fail_if_false (shared_counter == FINAL_VALUE, "Incorrect value of shared counter!");
  free (info);
}

void
test_spinlock (void)
{
  test_trylock ();
  int i;
  for (i = 0;i<100;i++) {
      test_inc_shared ();    
  }
  
  pass ();
}

