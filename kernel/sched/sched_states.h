//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_SCHED_STATES_H
#define UNTITLED_OS_SCHED_STATES_H

#define NUMBER_OF_SCHED_STATES 6

/**
 * @brief Scheduler states for threads/processes
 *
 * Defines the lifecycle states a thread or process can be in.
 */
enum sched_states
{
    NEW = 0,  /**< Thread/process has been created but not yet initialized */
    RUNNABLE, /**< Thread/process is ready to run and waiting for CPU */
    ON_CPU,   /**< Thread/process is currently running on a CPU */
    WAIT,     /**< Thread/process is blocked, waiting for an event */
    EXIT,     /**< Thread/process has finished execution */
    UNUSED    /**< Unused or free thread/process slot */
};

#endif // UNTITLED_OS_SCHED_STATES_H
