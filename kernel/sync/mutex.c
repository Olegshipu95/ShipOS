//
// Created by ShipOS developers on 03.01.24.
// Copyright (c) 2024 SHIPOS. All rights reserved.
//
// Simple mutex implementation for ShipOS kernel.
// Uses an underlying spinlock and a thread waiting list for blocking.
//

#include "mutex.h"

int init_mutex(struct mutex *lk, char *name) {
    lk->spinlock = kalloc();

    // Initialize waiting thread list to empty
    lk->thread_list = 0;
    init_spinlock(lk->spinlock, name);
}

void acquire_mutex(struct mutex *lk) {

    if (lk == 0) {
        panic("panic in acquire_mutex");
    }

    check_mutex:
    int bool = holding_spinlock(lk->spinlock);
    if (bool == 0) {
        acquire_spinlock(lk->spinlock);
        return;
    } else {
        // Mutex is locked, add current thread to wait list
        push_thread_list(&lk->thread_list, current_cpu.current_thread);
        change_thread_state(current_cpu.current_thread, WAIT);

        yield();

        // Ensure spinlock is free before retrying
        if(holding_spinlock(lk->spinlock) != 0)
            panic("acquire_mutex: spinlock in not free");
        goto check_mutex;
    }
};

void release_mutex(struct mutex *lk) {
    if (lk->spinlock->is_locked == 0) {
        panic("release_mutex");
    }
    if (lk->thread_list == 0) {
        // No waiting threads: simply release spinlock
        release_spinlock(lk->spinlock);
        return;
    } else {
        // Wake up one waiting thread
        struct thread *thread = pop_thread_list(&lk->thread_list);
        change_thread_state(thread, RUNNABLE);
    }
}

void destroy_mutex(struct mutex *lk) {
    kfree(lk);
}
