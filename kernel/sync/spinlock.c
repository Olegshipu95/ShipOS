//
// Created by untitled-os developers on 28.10.23.
// Copyright (c) 2023 untitled-os. All rights reserved.
//

#include "spinlock.h"
// Eflags register
#define FL_INT           0x00000200      // Interrupt Enable

void init_spinlock(struct spinlock *lock, char *name) {
    lock->is_locked = 0;
    lock->name = name;
}

//bool function
void acquire_spinlock(struct spinlock *lk) {
    pushcli(); // disable interrupts to avoid deadlock.
//    if (holding_spinlock(lk)) {
//        popcli();
//        return 1;
//    }

    // The xchg is atomic.
    while (xchg(&lk->is_locked, 1) != 0);

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen after the lock is acquire_spinlockd.
    __sync_synchronize();
    return;
}

void release_spinlock(struct spinlock *lk) {
    if (!holding_spinlock(lk))
        panic("release_spinlock");

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that all the stores in the critical
    // section are visible to other cores before the lock is release_spinlockd.
    // Both the C compiler and the hardware may re-order loads and
    // stores; __sync_synchronize() tells them both not to.
    __sync_synchronize();

    // release_spinlock the lock, equivalent to lk->locked = 0.
    // This code can't use a C assignment, since it might
    // not be atomic. A real OS would use C atomics here.
    asm volatile("movl $0, %0" : "+m" (lk->is_locked) : );

    popcli();
}

int holding_spinlock(struct spinlock *lock) {
    int r;
    pushcli();
    r = lock->is_locked;
    popcli();
    return r;
}

void pushcli(void) {
    int eflags;

    eflags = readeflags();
    cli();
    if (current_cpu.ncli == 0)
        current_cpu.intena = eflags & FL_INT;
    current_cpu.ncli += 1;
}

void popcli(void) {
    if (readeflags() & FL_INT)
        panic("popcli - interruptible");
    if (--current_cpu.ncli < 0)
        panic("popcli");
    if (current_cpu.ncli == 0 && current_cpu.intena)
        sti();
}
