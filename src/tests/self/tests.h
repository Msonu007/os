#ifndef TESTS_MISC_MISC_TESTS_H_
#define TESTS_MISC_MISC_TESTS_H_

#include <stdbool.h>

void
run_self_test (const char *);

typedef void
test_func (void);

extern test_func test_change_cpu;
extern test_func test_atomics;
extern test_func test_spinlock;
extern test_func test_all_list;
extern test_func test_ipi;
extern test_func test_ipi_blocked;
extern test_func test_ipi_all;
extern test_func test_ipi_missed;
extern test_func test_cli_print;
extern test_func test_savecallerinfo;
extern test_func test_console;
extern test_func test_realclock;

void msg (const char *, ...);
void fail_if_false (bool truth, const char *, ...);
void fail (const char *, ...);
void pass (void);

#endif /* TESTS_MISC_MISC_TESTS_H_ */
