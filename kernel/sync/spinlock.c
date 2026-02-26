#include "spinlock.h"
#include "../sched/percpu.h"
#include "../lib/include/panic.h"

void init_spinlock(struct spinlock *lock, const char *name) {
    lock->is_locked = 0;
    lock->name = name;
    lock->cpu = (void*)0;
}

void acquire_spinlock(struct spinlock *lk) {
    pushcli(); 

    // Deadlock check: only panic if THIS CPU already holds the lock
    if (holding_spinlock(lk)) {
        panic("acquire_spinlock: deadlock");
    }

    // Atomic swap. 32-bit swap for 32-bit is_locked.
    while (xchg(&lk->is_locked, 1) != 0) {
        asm volatile("pause");
    }

    __sync_synchronize();
    lk->cpu = mycpu();
}

void release_spinlock(struct spinlock *lk) {
    if (!holding_spinlock(lk))
        panic("release_spinlock: not holding");

    lk->cpu = (void*)0;
    __sync_synchronize();

    // Atomic release
    asm volatile("movl $0, %0" : "+m" (lk->is_locked) : );

    popcli();
}

int holding_spinlock(struct spinlock *lock) {
    return lock->is_locked && (lock->cpu == mycpu());
}