//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//
// SMP-safe counting semaphore implementation for ShipOS kernel.
//

#include "semaphore.h"
#include "../sched/smp_sched.h"

void sem_init(struct semaphore *s, int value, const char *name) {
    init_spinlock(&s->lock, name);
    s->value = value;
    s->name = name;
}

void sem_wait(struct semaphore *s) {
    acquire_spinlock(&s->lock);

    // Sleep until the counter is greater than 0
    while (s->value <= 0) {
        sleep(s, &s->lock);
    }

    s->value--;
    
    release_spinlock(&s->lock);
}

void sem_post(struct semaphore *s) {
    acquire_spinlock(&s->lock);
    
    s->value++;
    
    // Wake up threads waiting on this semaphore
    wakeup(s);
    
    release_spinlock(&s->lock);
}