//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023-2026 SHIPOS. All rights reserved.
//

#include "proc.h"
#include "../lib/include/panic.h"
#include "../lib/include/logging.h"
#include "sched_states.h"
#include "../lib/include/memset.h"

struct spinlock pid_lock;
struct spinlock proc_lock;
struct proc_node *proc_list;

pid_t generate_pid() {
    acquire_spinlock(&pid_lock);
    static pid_t current_pid = 0;
    pid_t local_pid = current_pid;
    current_pid++;
    release_spinlock(&pid_lock);
    return local_pid;
}

struct proc *allocproc(void) {
    struct proc *proc = (struct proc *)kalloc();

    if (proc == 0) {
        panic("Failed to alloc proc\n");
    }
    memset(proc, 0, PGSIZE);

    proc->pid = generate_pid();
    proc->threads = 0;
    proc->killed = 0;

    acquire_spinlock(&proc_lock);
    push_proc_list(&proc_list, proc);
    release_spinlock(&proc_lock);

    return proc;
}

struct proc_node *procinit(void) {
    init_spinlock(&pid_lock, "pid_lock");
    init_spinlock(&proc_lock, "proc_lock");

    struct proc *init_proc = allocproc();
    LOG("Init proc allocated");

    static uint32_t arg_value1 = 1;
    static uint32_t arg_value2 = 2;
    static struct argument arg1;
    static struct argument arg2;
    
    arg1.arg_size = sizeof(uint32_t);
    arg1.value = (char *)&arg_value1;
    arg2.arg_size = sizeof(uint32_t);
    arg2.value = (char *)&arg_value2;
    
    LOG("arg initialized");
    
    // Cast thread_function to the expected signature (void (*)(void *))
    struct thread *new_thread1 = create_thread((void (*)(void *))thread_function, 1, &arg1);
    struct thread *new_thread2 = create_thread((void (*)(void *))thread_function, 1, &arg2);
    
    LOG("thread initialized");
    change_thread_state(new_thread1, RUNNABLE);
    change_thread_state(new_thread2, RUNNABLE);
    
    LOG("thread state initialized");
    push_thread_list(&(init_proc->threads), new_thread1);
    push_thread_list(&(init_proc->threads), new_thread2);
    
    LOG("thread pushed into list");
    LOG("Init proc node %p", proc_list);
    
    return proc_list;
}

void push_proc_list(struct proc_node **list, struct proc *proc) {
    struct proc_node *new_node = (struct proc_node *)kalloc();
    if (new_node == 0) panic("push_proc_list: kalloc failed");
    memset(new_node, 0, PGSIZE);
    
    new_node->data = proc;
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

struct proc *pop_proc_list(struct proc_node **list) {
    if (*list == 0) {
        panic("Empty proc list while popping\n");
        return 0; // Unreachable
    }
    
    struct proc* p = (*list)->data;
    struct proc_node *to_free = *list;

    if (to_free->next == to_free) {
        *list = 0;
    } else {
        to_free->prev->next = to_free->next;
        to_free->next->prev = to_free->prev;
        *list = to_free->next;
    }
    
    kfree(to_free);
    return p;
}

void shift_proc_list(struct proc_node **list) {
    if (*list == 0) {
        panic("Empty proc list while shifting\n");
    } else {
        *list = (*list)->next;
    }
}

struct proc *peek_proc_list(struct proc_node *list) {
    if (list == 0) {
        panic("Empty proc list while peeking\n");
        return 0; // Unreachable
    }
    return list->data;
}