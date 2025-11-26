//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Simple circular double-linked list implementation.
// Provides fast insert, remove, push and pop operations.
//

#ifndef LIST_H
#define LIST_H

// Basic node for a circular doubly-linked list
struct list
{
    struct list *prev;
    struct list *next;
};

/**
 * @brief Initialize a list node as a circular list
 * @param lst Pointer to the list head
 */
void lst_init(struct list *lst);

/**
 * @brief Check if the list is empty
 * @param lst Pointer to the list head
 * @return 1 if empty, 0 otherwise
 */
int lst_empty(struct list *lst);

/**
 * @brief Remove a node from the list
 * @param e Pointer to the node to remove
 */
void lst_remove(struct list *e);

/**
 * @brief Pop the first element from the list
 * @param lst Pointer to the list head
 * @return Pointer to the popped node
 */
void *lst_pop(struct list *lst);

/**
 * @brief Push a node to the front of the list
 * @param lst Pointer to the list head
 * @param p Pointer to the node to push
 */
void lst_push(struct list *lst, void *p);

/**
 * @brief Print all nodes in the list (for debugging)
 * @param lst Pointer to the list head
 */
void lst_print(struct list *lst);
#endif // LIST_H
