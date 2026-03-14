//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//

#include "completion.h"
#include "../sched/smp_sched.h"

void init_completion(struct completion *c, const char *name) {
    c->done = 0;
    init_spinlock(&c->lock, name);
}

void wait_for_completion(struct completion *c) {
    acquire_spinlock(&c->lock);
    
    // sleep() atomically releases c->lock, puts thread to sleep, 
    // and reacquires c->lock when woken up!
    while (c->done == 0) {
        sleep(c, &c->lock);
    }
    
    release_spinlock(&c->lock);
}

void complete(struct completion *c) {
    acquire_spinlock(&c->lock);
    
    c->done = 1;
    wakeup(c); // Wakes up all threads sleeping on 'c'
    
    release_spinlock(&c->lock);
}

void complete_all(struct completion *c) {
    complete(c);
}