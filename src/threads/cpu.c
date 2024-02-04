/* Definitions for CPU-related data structures.
 * See cpu.h for their purpose.
 */
#include "cpu.h"

int cpu_can_acquire_spinlock;
struct cpu *bcpu;
struct cpu cpus[NCPU_MAX];
int cpu_ismp;
unsigned int ncpu;
int cpu_started_others;

