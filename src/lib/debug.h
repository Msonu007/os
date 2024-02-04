#ifndef __LIB_DEBUG_H
#define __LIB_DEBUG_H

#include <stdint.h>

/* This module requires that Pintos be compiled with 
   -fno-omit-frame-pointer flag, since backtracing
   requires that the %ebp register be saved during 
   function calls. 
   See GCC's documentation on -fomit-frame-pointer */

/* GCC lets us add "attributes" to functions, function
   parameters, etc. to indicate their properties.
   See the GCC manual for details. */
#define UNUSED __attribute__ ((unused))
#define NO_RETURN __attribute__ ((noreturn))
#define NO_INLINE __attribute__ ((noinline))
#define PRINTF_FORMAT(FMT, FIRST) __attribute__ ((format (printf, FMT, FIRST)))

/* Halts the OS, printing the source file name, line number, and
   function name, plus a user-specific message. */
#define PANIC(...) debug_panic (__FILE__, __LINE__, __func__, __VA_ARGS__)

void debug_panic (const char *file, int line, const char *function,
                  const char *message, ...) PRINTF_FORMAT (4, 5) NO_RETURN;
void debug_backtrace (void);
void debug_backtrace_with_lock (void);
void debug_backtrace_all (void);

#define PCS_MAX 15
struct callerinfo {
  struct thread *t;	/* The thread that made the call */
  struct cpu *cpu;     	/* The cpu that t was executing on at the time
			   It can be different from t->cpu if t was migrated */
  uint32_t pcs[PCS_MAX];     /* The call stack (an array of program counters) of the thread
			   that made the call. */
};

void debug_init_callerinfo (struct callerinfo *);
void debug_save_callerinfo (struct callerinfo *);
void debug_print_callerinfo (struct callerinfo *);
#endif



/* This is outside the header guard so that debug.h may be
   included multiple times with different settings of NDEBUG. */
#undef ASSERT
#undef NOT_REACHED

#ifndef NDEBUG
#define ASSERT(CONDITION)                                       \
        if (CONDITION) { } else {                               \
                PANIC ("assertion `%s' failed.", #CONDITION);   \
        }
#define NOT_REACHED() PANIC ("executed an unreachable statement");
#else
#define ASSERT(CONDITION) ((void) 0)
#define NOT_REACHED() for (;;)
#endif /* lib/debug.h */
