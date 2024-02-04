#ifndef __LIB_KERNEL_CONSOLE_H
#define __LIB_KERNEL_CONSOLE_H

void console_init (void);

enum console_mode { NORMAL_MODE = 0, EMERGENCY_MODE };
void console_set_mode (enum console_mode);
void console_print_stats (void);
void console_use_spinlock(void);

#endif /* lib/kernel/console.h */
