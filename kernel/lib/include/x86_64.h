//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef X86_64_H
#define X86_64_H

#include <stdint.h>

struct __attribute__((packed, aligned(8))) context
{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
};

extern void switch_context(struct context **old, struct context *new);

static inline void
cli(void)
{
    asm volatile("cli");
}

static inline void
sti(void)
{
    asm volatile("sti");
}

static inline void
outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void
outw(uint16_t port, uint16_t val)
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void
outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t
inb(uint16_t port)
{
    uint8_t res;
    asm volatile("inb %1, %0" : "=a"(res) : "Nd"(port) : "memory");
    return res;
}

static inline uint16_t
inw(uint16_t port)
{
    uint16_t res;
    asm volatile("inw %1, %0" : "=a"(res) : "Nd"(port) : "memory");
    return res;
}

static inline uint32_t
inl(uint16_t port)
{
    uint32_t res;
    asm volatile("inl %1, %0" : "=a"(res) : "Nd"(port) : "memory");
    return res;
}

static inline uint32_t
xchg(volatile uint32_t *addr, uint32_t newval)
{
    uint32_t result;

    // The + in "+m" denotes a read-modify-write operand.
    asm volatile("lock; xchgl %0, %1" : "+m"(*addr), "=a"(result) : "1"(newval) : "cc");
    return result;
}

static inline uint32_t
readeflags(void)
{
    uint64_t eflags;
    asm volatile("pushf; pop %0" : "=r"(eflags));
    return eflags;
}

static inline uint64_t
rcr2(void)
{
    uint64_t val;
    asm volatile("mov %%cr2,%0" : "=r"(val));
    return val;
}

static inline uint64_t
rcr3(void)
{
    uint64_t val;
    asm volatile("mov %%cr3,%0" : "=r"(val));
    return val;
}

static inline void
wcr3(uint64_t val)
{
    asm("mov %0, %%rax" ::"a"(val));
    asm("mov %rax, %cr3");
}

#endif // X86_64_H