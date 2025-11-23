#ifndef UNTITLED_OS_SEMAPHORE_H
#define UNTITLED_OS_SEMAPHORE_H

#include "spinlock.h"
#include "../sched/threads.h"

struct semaphore {
    int value;
    struct spinlock lock;
    struct thread_node *wait_list;
};

void sem_init(struct semaphore *s, int value, char *name);
void sem_wait(struct semaphore *s);
void sem_post(struct semaphore *s);

#endif