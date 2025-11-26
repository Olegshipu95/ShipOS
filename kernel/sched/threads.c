//
// Created by ShipOS developers on 20.12.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#include "threads.h"
#include "sched_states.h"
#include "../lib/include/panic.h"
#include "../kalloc/kalloc.h"
#include "../lib/include/memset.h"
#include "../memlayout.h"
#include "scheduler.h"

struct thread *current_thread = 0;

void init_thread(struct thread *thread, void (*start_function)(void *), int argc, struct argument *args) {
    thread->stack = kalloc();
    thread->kstack = kalloc();
    memset(thread->stack, 0, PGSIZE);
    memset(thread->kstack, 0, PGSIZE);
    thread->kstack += PGSIZE;
    thread->stack += PGSIZE;
    thread->start_function = start_function;
    thread->argc = argc;
    thread->args = args;
    char *sp = thread->stack;
    sp -= sizeof(uint64_t);
    *(uint64_t * )(sp) = start_function;
    sp -= sizeof(struct context) - sizeof(uint64_t);
    memset(sp, 0, sizeof(struct context) - sizeof(uint64_t));
    thread->context = (struct context *) sp;
    thread->context->rdi = argc;
    thread->context->rsi = args;
}

struct thread *create_thread(void (*start_function)(void *), int argc, struct argument *args) {
    struct thread *new_thread = (struct thread *) kalloc();
    init_thread(new_thread, start_function, argc, args);
    return new_thread;
}

void push_thread_list(struct thread_node **list, struct thread *thread) {
    struct thread_node *new_node = kalloc();
    new_node->data = thread;
    if ((*list) != 0) {
        new_node->next = (*list);
        new_node->prev = (*list)->prev;
        (*list)->prev->next = new_node;
        (*list)->prev = new_node;
    } else {
        new_node->prev = new_node;
        new_node->next = new_node;
        *list = new_node;
    }
}

struct thread *pop_thread_list(struct thread_node **list) {
    if (*list == 0) {
        panic("Empty thread list while popping\n");
    } else {
        struct thread* t = (*list)->data;
        if ((*list)->next = (*list)) {
            kfree(*list);
            *list = 0;
        } else {
            (*list)->prev->next = (*list)->next;
            (*list)->next->prev = (*list)->prev;
            kfree(*list);
        }
        return t;
    }
}

void shift_thread_list(struct thread_node **list) {
    if (*list == 0) {
        panic("Empty thread list while shifting\n");
    } else {
        *list = (*list)->next;
    }
}

struct thread *peek_thread_list(struct thread_node *list) {
    if (list == 0) {
        panic("Empty thread list while peeking\n");
    } else {
        return list->data;
    }
}

void change_thread_state(struct thread *thread, enum sched_states new_state) {
    thread->state = new_state;
}
