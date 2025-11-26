//
// Created by ShipOS developers on 23.12.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// NOTE: This header is currently UNUSED.
// It is kept for future functionality and should not be deleted.
//

#ifndef UNTITLED_OS_CPU_H
#define UNTITLED_OS_CPU_H
typedef struct ucontext_t
{
    unsigned long int __ctx(uc_flags);
    struct ucontext_t *uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    sigset_t uc_sigmask;
    struct _libc_fpstate __fpregs_mem;
    unsigned long int __ssp[4];
} ucontext_t;
#endif // UNTITLED_OS_CPU_H
