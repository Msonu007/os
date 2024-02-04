/* Thie file is derived from source code for the xv6 instruction 
   operating system. The xv6 copyright notice is printed below.

   The xv6 software is:

   Copyright (c) 2006-2009 Frans Kaashoek, Robert Morris, Russ Cox,
                        Massachusetts Institute of Technology

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef LIB_KERNEL_X86_H_
#define LIB_KERNEL_X86_H_

#include <stdint.h>
// Routines to let C code use special x86 instructions.

static inline void
stosb (void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosb" :
      "=D" (addr), "=c" (cnt) :
      "0" (addr), "1" (cnt), "a" (data) :
      "memory", "cc");
}

static inline void
stosl (void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosl" :
      "=D" (addr), "=c" (cnt) :
      "0" (addr), "1" (cnt), "a" (data) :
      "memory", "cc");
}

struct segdesc;

static inline void
lgdt (uint64_t gdtr_operand)
{
  asm volatile ("lgdt %0" : : "m" (gdtr_operand) : "memory");
}

static inline void
lidt (uint64_t idtr_operand)
{
  asm volatile ("lidt %0" : : "m" (idtr_operand) : "memory");
}

static inline void
ltr (uint16_t sel)
{
  asm volatile ("ltr %w0" : : "q" (sel) : "memory");
}

/* Push the flags register on the processor stack, then pop the
   value off the stack into `flags'.  See [IA32-v2b] "PUSHF"
   and "POP" and [IA32-v3a] 5.8.1 "Masking Maskable Hardware
   Interrupts". */
static inline uint32_t
readeflags (void)
{
  uint32_t eflags;
  asm volatile("pushfl; popl %0" : "=r" (eflags));
  return eflags;
}

static inline void
loadgs (uint16_t v)
{
  asm volatile("movw %0, %%gs" : : "r" (v) : "memory");
}

/* Disable interrupts by clearing the interrupt flag.
   See [IA32-v2b] "CLI" and [IA32-v3a] 5.8.1 "Masking Maskable
   Hardware Interrupts". */
static inline void
cli (void)
{
  asm volatile ("cli" : : : "memory");
}

/* Enable interrupts by setting the interrupt flag.

   See [IA32-v2b] "STI" and [IA32-v3a] 5.8.1 "Masking Maskable
   Hardware Interrupts". */
static inline void
sti (void)
{
  asm volatile("sti");
}

static inline uint32_t
rcr2 (void)
{
  uint32_t val;
  asm volatile("movl %%cr2,%0" : "=r" (val));
  return val;
}

static inline void
lcr3 (uint32_t val)
{
  asm volatile("movl %0,%%cr3" : : "r" (val) : "memory");
}

static inline void
flushtlb (void)
{
  asm volatile("movl %%cr3,%%eax"::: "memory");
  asm volatile("movl %%eax,%%cr3"::: "memory");
}

#endif /* LIB_KERNEL_X86_H_ */
