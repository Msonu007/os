#include "threads/thread.h"
#include <stdio.h>
#include "tests.h"
#include "devices/timer.h"
#include "lib/atomic-ops.h"

void
test_realclock (void)
{
    printf("I am now trying to sleep for 3s, and then for 10s.  Check your watch!\n");
    timer_sleep (TIMER_FREQ * 3);
    printf("Start sleeping\n");
    timer_sleep (TIMER_FREQ * 10);
    printf("Done sleeping - how many seconds did I really sleep for?\n");
}
