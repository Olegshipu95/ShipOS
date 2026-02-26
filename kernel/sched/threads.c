//
// Created by ShipOS developers on 20.12.23.
// Copyright (c) 2023-2026 SHIPOS. All rights reserved.
//

#include "threads.h"
#include "sched_states.h"
#include "../lib/include/panic.h"
#include "smp_sched.h"

struct thread *current_thread = 0;

void init_thread(struct thread *thread, void (*start_function)(void *), int argc, struct argument *args) {
    thread->stack = (uint64_t)kalloc();
    thread->kstack = (uint64_t)kalloc();
    
    memset((void *)thread->stack, 0, PGSIZE);
    memset((void *)thread->kstack, 0, PGSIZE);
    
    thread->kstack += PGSIZE;
    thread->stack += PGSIZE;
    thread->start_function = start_function;
    thread->argc = argc;
    thread->args = args;
    
    char *sp = (char *)thread->stack;
    sp -= sizeof(uint64_t);     
    *(uint64_t *)(sp) = (uint64_t)start_function;
    
    sp -= sizeof(struct context) - sizeof(uint64_t);
    memset(sp, 0, sizeof(struct context) - sizeof(uint64_t));
    
    thread->context = (struct context *) sp;
    thread->context->rdi = (uint64_t)argc;
    thread->context->rsi = (uint64_t)args;
}

struct thread *create_thread(void (*start_function)(void *), int argc, struct argument *args) {
    struct thread *new_thread = (struct thread *) kalloc();
    if (new_thread == 0) panic("create_thread: kalloc failed");
    memset(new_thread, 0, PGSIZE);
    init_thread(new_thread, start_function, argc, args);
    return new_thread;
}

void push_thread_list(struct thread_node **list, struct thread *thread) {
    struct thread_node *new_node = (struct thread_node *)kalloc();
    if (new_node == 0) panic("push_thread_list: kalloc failed");
    memset(new_node, 0, PGSIZE);
    
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
        return 0; // Unreachable
    }
    
    struct thread* t = (*list)->data;
    struct thread_node *to_free = *list;

    if (to_free->next == to_free) {
        *list = 0;
    } else {
        to_free->prev->next = to_free->next;
        to_free->next->prev = to_free->prev;
        *list = to_free->next;
    }
    
    kfree(to_free);
    return t;
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
        return 0; // Unreachable
    }
    return list->data;
}

void change_thread_state(struct thread *thread, enum sched_states new_state) {
    thread->state = new_state;
}