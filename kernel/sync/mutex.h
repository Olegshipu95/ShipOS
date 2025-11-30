//
// Created by ShipOS developers on 03.01.24.
// Copyright (c) 2024 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_MUTEX_H
#define UNTITLED_OS_MUTEX_H

#include "spinlock.h"
#include "../sched/threads.h"
#include "../kalloc/kalloc.h"
#include "../sched/scheduler.h"

struct mutex {
    int locked;
    struct spinlock lock;
    struct thread_node *wait_list;
    char *name;
};

int init_mutex(struct mutex *lk, char *name);

void acquire_mutex(struct mutex *lk);

void release_mutex(struct mutex *lk);


#endif //UNTITLED_OS_MUTEX_H
