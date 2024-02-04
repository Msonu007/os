#ifndef TESTS_THREADS_TESTS_H
#define TESTS_THREADS_TESTS_H

#include <stdbool.h>

void run_test (const char *);

typedef void test_func (void);

extern test_func test_alarm_single;
extern test_func test_alarm_multiple;
extern test_func test_alarm_synch;
extern test_func test_alarm_zero;
extern test_func test_alarm_negative;
extern test_func test_idle;
extern test_func test_create_new;
extern test_func test_yield;
extern test_func test_tick;
extern test_func test_tick2;
extern test_func test_delayed_tick;
extern test_func test_sleeper;
extern test_func test_short_sleeper;
extern test_func test_sleeper_minvruntime;
extern test_func test_new_minvruntime;
extern test_func test_change_cpu;
extern test_func test_nice;
extern test_func test_renice;
extern test_func test_idle_unblock;
extern test_func test_vruntime;
extern test_func test_cfs_fib;
extern test_func test_cfs_sleepers;
extern test_func balance;
extern test_func test_balance_synch1;
extern test_func test_balance_sleepers;

void msg (const char *, ...);
void fail_if_false (bool truth, const char *, ...);
void fail (const char *, ...);
void pass (void);

#endif /* tests/threads/tests.h */

