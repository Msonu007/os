#ifndef TESTS_THREADS_CFSTEST_H_
#define TESTS_THREADS_CFSTEST_H_

#include <stdint.h>
#include "threads/thread.h"

static const unsigned int gran = 4000000ULL; /* 4ms */
static const unsigned int latency_gran_ratio = 5;
/* Keep it at gran * latency_gran_ratio, but C only lets you initialize constants */
static const unsigned int latency = 20000000ULL; /* 20 ms */

void cfstest_check_current (struct thread *expect);
void cfstest_advance_time (int64_t advance);
void cfstest_set_up (void);
void cfstest_tear_down (void);

#endif /* TESTS_THREADS_CFSTEST_H_ */
