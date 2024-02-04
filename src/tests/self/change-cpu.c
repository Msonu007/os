/*
 * Test changing the current CPU to a "virtual" CPU by reloading %gs register
 */
#include "threads/gdt.h"
#include "threads/interrupt.h"
#include "tests.h"
#include "threads/cpu.h"
#include <string.h>

static void
switch_cpu (struct cpu *cpu)
{
  gdt_refer_cpu (cpu);
}

void
test_change_cpu ()
{
  struct cpu *real_cpu;
  struct cpu vcpu;
  intr_disable_push ();
  memset (&vcpu, 0, sizeof(struct cpu));
  real_cpu = get_cpu ();
  switch_cpu (&vcpu);
  if (get_cpu () != &vcpu)
    {
      fail ("CPU should now refers to %p, actually refers to %p", &vcpu,
	    get_cpu ());
    }
  switch_cpu (real_cpu);
  if (get_cpu () != real_cpu)
    fail ("CPU should now refers to %p, actually refers to %p", &vcpu,
	  get_cpu ());
  pass ();
  intr_enable_pop ();
}
