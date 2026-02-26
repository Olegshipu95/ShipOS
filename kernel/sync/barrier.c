//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//

#include "barrier.h"
#include "../sched/smp_sched.h"

void init_barrier(struct barrier *b, uint32_t count, const char *name) {
    b->threshold = count;
    b->count = 0;
    b->generation = 0;
    init_spinlock(&b->lock, name);
}

void barrier_wait(struct barrier *b) {
    acquire_spinlock(&b->lock);

    // Save the current generation before we sleep
    uint32_t gen = b->generation;
    b->count++;

    if (b->count >= b->threshold) {
        // We are the last thread to arrive - trip the barrier!
        b->count = 0;
        b->generation++; // Increment generation for future reuses
        
        // Wake up all threads sleeping on the 'generation' channel
        wakeup(&b->generation);
    } else {
        // Not all threads have arrived, we must wait
        // Loop to protect against spurious wakeups
        while (gen == b->generation) {
            sleep(&b->generation, &b->lock);
        }
    }
    
    release_spinlock(&b->lock);
}