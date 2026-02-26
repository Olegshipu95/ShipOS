//
// Created by ShipOS developers on 20.12.23.
// Copyright (c) 2023-2026 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_THREADS_H
#define UNTITLED_OS_THREADS_H

#include "../memlayout.h"
#include "../kalloc/kalloc.h"
#include "../lib/include/x86_64.h"
#include <stddef.h>
#include <inttypes.h>
#include "../lib/include/memset.h"
#include "sched_states.h"

struct argument {
    char *value;
    size_t arg_size;
};

struct thread {
    struct context *context;
    void (*start_function)(void *);
    uint64_t stack;
    uint64_t kstack;
    size_t argc;
    struct argument *args;
    enum sched_states state;
    void *chan; // Wait channel: memory address the thread is waiting on
};

struct thread_node {
    struct thread *data;
    struct thread_node *next;
    struct thread_node *prev;
};

void push_thread_list(struct thread_node **list, struct thread *thread);

struct thread *pop_thread_list(struct thread_node **list);

void shift_thread_list(struct thread_node **list);

struct thread *peek_thread_list(struct thread_node *list);

struct thread *create_thread(void (*start_function)(void *), int argc, struct argument *args);

void change_thread_state(struct thread *thread, enum sched_states new_state);

void thread_function(int argc, struct argument *args);

#endif //UNTITLED_OS_THREADS_H