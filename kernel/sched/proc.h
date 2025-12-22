//
// Created by untitled-os developers on 28.10.23.
// Copyright (c) 2023 untitled-os. All rights reserved.
//


#ifndef UNTITLED_OS_PROC_H
#define UNTITLED_OS_PROC_H
#define MAXPROCS 100
//#include "../lib/include/stdint.h"
#include <inttypes.h>
#include <stddef.h>
#include "../tty/tty.h"
#include "../sync/spinlock.h"
#include "../../kernel/kalloc/kalloc.h"
#include "threads.h"
#include "sched_states.h"

typedef size_t pid_t;

struct proc {
    pid_t pid;
    int killed;
    struct thread_node *threads;
};

struct cpu {
    int ncli;                        // Depth of pushcli nesting.
    int intena;                      // Were interrupts enabled before pushcli?
    struct thread *current_thread;   // The thread running on this cpu or null
};

struct proc_node {
    struct proc *data;
    struct proc_node *next;
    struct proc_node *prev;
};

extern struct cpu current_cpu;
extern struct proc_node *proc_list;

void push_proc_list(struct proc_node **list, struct proc *proc);

struct proc *pop_proc_list(struct proc_node **list);

void shift_proc_list(struct proc_node **list);

struct proc *peek_proc_list(struct proc_node *list);

struct proc_node *procinit(void);

int exec(char *file, char *argv[]);

char *sbrk(int n);

#endif //UNTITLED_OS_PROC_H
