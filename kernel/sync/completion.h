#ifndef UNTITLED_OS_COMPLETION_H
#define UNTITLED_OS_COMPLETION_H

#include "spinlock.h"
#include "../sched/threads.h"

struct completion {
    int done;
    struct spinlock lock;
    struct thread_node *wait_list;
};

void init_completion(struct completion *c, char *name);
void wait_for_completion(struct completion *c);
void complete(struct completion *c);  // Будит всех
void complete_all(struct completion *c);  // Алиас для комплита

#endif