#ifndef UNTITLED_OS_CONDVAR_H
#define UNTITLED_OS_CONDVAR_H

#include "mutex.h"
#include "spinlock.h"
#include "../sched/threads.h"

struct condvar {
    struct spinlock lock;
    struct thread_node *wait_list;
};

void init_condvar(struct condvar *cv, char *name);
void cv_wait(struct condvar *cv, struct mutex *m);
void cv_signal(struct condvar *cv);  // Разбудить одного
void cv_broadcast(struct condvar *cv);  // Разбудить всех

#endif