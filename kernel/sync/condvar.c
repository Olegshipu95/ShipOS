//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//

#include "condvar.h"
#include "../sched/smp_sched.h"
#include "../sched/percpu.h"

void init_condvar(struct condvar *cv, const char *name) {
    cv->name = name;
}

void cv_wait(struct condvar *cv, struct mutex *m) {
    // We must hold the mutex's internal spinlock to ensure that 
    // the release of the mutex and the sleep are atomic.
    acquire_spinlock(&m->lock);

    // Release the mutex
    m->locked = 0;
    m->owner = (void *)0;

    // Wake up any threads that might be waiting for this mutex
    wakeup(m);

    // Sleep on the condition variable address.
    // The sleep() function in smp_sched.c will release m->lock 
    // and switch context.
    sleep(cv, &m->lock);

    // After waking up, we must re-acquire the mutex
    acquire_mutex(m);
}

void cv_signal(struct condvar *cv) {
    // Wake up threads sleeping on the condvar channel
    wakeup(cv);
}

void cv_broadcast(struct condvar *cv) {
    wakeup(cv);
}