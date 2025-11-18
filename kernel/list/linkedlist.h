//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Generic typed linked list for ShipOS kernel.
//

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "../kalloc/kalloc.h"
#include "../lib/include/types.h"

/**
 * @brief Generates a linked list struct and its implementation
 * @param PFX Prefix for function names (e.g., list, proc_list)
 * @param NAME Name of the linked list type
 * @param DATA_TYPE Type of data stored in each node
 * 
 * Example usage:
 *   LINKEDLIST_GENERATE(mylist, mynode, int)
 *   mylist *l = mylist_new();
 */
#define LINKEDLIST_GENERATE(PFX, NAME, DATA_TYPE)    \
    LINKEDLIST_GENERATE_STRUCT(PFX, NAME, DATA_TYPE) \
    LINKEDLIST_GENERATE_SOURCE(PFX, NAME, DATA_TYPE)

#define SIZE_OF_NODE_POINTERS (3 * sizeof(void*))

/*--------------------------------------
  Structures: list, node, iterator
---------------------------------------*/
#define LINKEDLIST_GENERATE_STRUCT(PFX, NAME, DATA_TYPE) \
                                                         \
    struct NAME##_s                                      \
    {                                                    \
        struct NAME##_node_s *head;                      \
        struct NAME##_node_s *tail;                      \
        size_t count;                                    \
    };                                                   \
                                                         \
    struct NAME##_node_s                                 \
    {                                                    \
        DATA_TYPE data;                                  \
        struct NAME##_s *owner;                          \
        struct NAME##_node_s *next;                      \
        struct NAME##_node_s *prev;                      \
    };                                                   \
                                                         \
    struct NAME##_iter_s                                 \
    {                                                    \
        struct NAME##_s *target;                         \
        struct NAME##_node_s *cursor;                    \
        size_t index;                                    \
        bool start;                                      \
        bool end;                                        \
    };                                                   \
                                                         \
/*--------------------------------------
  Function implementations
---------------------------------------*/
/* Additional helper functions included:
   - push_if / pop_if
   - front/back/get node data
   - iteration functions: iter_new, iter_next, iter_prev, iter_start, iter_end
   - insert_nxt / insert_prv / remove_nxt / remove_prv / remove_cur
   (See original macro for full implementations)
*/
#define LINKEDLIST_GENERATE_SOURCE(PFX, NAME, DATA_TYPE)                            \
                                                                                    \
    /* Create a new empty list */                                                   \
    NAME *PFX##_new(void)                                                           \
    {                                                                               \
        NAME *new_list = kalloc();                                                  \
                                                                                    \
        if (!new_list)                                                              \
            return NULL;                                                            \
                                                                                    \
        new_list->count = 0;                                                        \
        new_list->head = NULL;                                                      \
        new_list->tail = NULL;                                                      \
                                                                                    \
        return new_list;                                                            \
    }                                                                               \
                                                                                    \
    void PFX##_free(NAME *list)                                                     \
    {                                                                               \
        NAME##_node *scan = list->head;                                             \
        while (list->head != NULL)                                                  \
        {                                                                           \
            list->head = list->head->next;                                          \
            kfree(scan);                                                            \
            scan = list->head;                                                      \
        }                                                                           \
        kfree(list);                                                                \
    }                                                                               \
                                                                                    \
    bool PFX##_push_front(NAME *list, DATA_TYPE element)                            \
    {                                                                               \
        NAME##_node *node = PFX##_new_node(list, element);                          \
                                                                                    \
        if (!node)                                                                  \
            return false;                                                           \
                                                                                    \
        if (PFX##_empty(list))                                                      \
        {                                                                           \
            list->head = node;                                                      \
            list->tail = node;                                                      \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            node->next = list->head;                                                \
            list->head->prev = node;                                                \
            list->head = node;                                                      \
        }                                                                           \
                                                                                    \
        list->count++;                                                              \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_push(NAME *list, DATA_TYPE element, size_t index)               \
    {                                                                               \
        if (index > list->count)                                                    \
            return false;                                                           \
                                                                                    \
        if (index == 0)                                                             \
        {                                                                           \
            return PFX##_push_front(list, element);                                 \
        }                                                                           \
        else if (index == list->count)                                              \
        {                                                                           \
            return PFX##_push_back(list, element);                                  \
        }                                                                           \
                                                                                    \
        NAME##_node *node = PFX##_new_node(list, element);                          \
                                                                                    \
        if (!node)                                                                  \
            return false;                                                           \
                                                                                    \
        NAME##_node *scan = PFX##_get_node(list, index - 1);                        \
                                                                                    \
        node->next = scan->next;                                                    \
        node->prev = scan;                                                          \
        node->next->prev = node;                                                    \
        node->prev->next = node;                                                    \
                                                                                    \
        list->count++;                                                              \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_push_back(NAME *list, DATA_TYPE element)                        \
    {                                                                               \
        NAME##_node *node = PFX##_new_node(list, element);                          \
                                                                                    \
        if (!node)                                                                  \
            return false;                                                           \
                                                                                    \
        if (PFX##_empty(list))                                                      \
        {                                                                           \
            list->head = node;                                                      \
            list->tail = node;                                                      \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            node->prev = list->tail;                                                \
            list->tail->next = node;                                                \
            list->tail = node;                                                      \
        }                                                                           \
                                                                                    \
        list->count++;                                                              \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_pop_front(NAME *list)                                           \
    {                                                                               \
        if (PFX##_empty(list))                                                      \
            return false;                                                           \
                                                                                    \
        NAME##_node *node = list->head;                                             \
        list->head = list->head->next;                                              \
                                                                                    \
        free(node);                                                                 \
                                                                                    \
        if (list->head == NULL)                                                     \
            list->tail = NULL;                                                      \
        else                                                                        \
            list->head->prev = NULL;                                                \
                                                                                    \
        list->count--;                                                              \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_pop(NAME *list, size_t index)                                   \
    {                                                                               \
        if (PFX##_empty(list))                                                      \
            return false;                                                           \
                                                                                    \
        if (index >= list->count)                                                   \
            return false;                                                           \
                                                                                    \
        if (index == 0)                                                             \
        {                                                                           \
            return PFX##_pop_front(list);                                           \
        }                                                                           \
        else if (index == list->count - 1)                                          \
        {                                                                           \
            return PFX##_pop_back(list);                                            \
        }                                                                           \
                                                                                    \
        NAME##_node *node = PFX##_get_node(list, index);                            \
                                                                                    \
        if (!node)                                                                  \
            return false;                                                           \
                                                                                    \
        node->next->prev = node->prev;                                              \
        node->prev->next = node->next;                                              \
                                                                                    \
        free(node);                                                                 \
                                                                                    \
        list->count--;                                                              \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_pop_back(NAME *list)                                            \
    {                                                                               \
        if (PFX##_empty(list))                                                      \
            return false;                                                           \
                                                                                    \
        NAME##_node *node = list->tail;                                             \
        list->tail = list->tail->prev;                                              \
                                                                                    \
        free(node);                                                                 \
                                                                                    \
        if (list->tail == NULL)                                                     \
            list->head = NULL;                                                      \
        else                                                                        \
            list->tail->next = NULL;                                                \
                                                                                    \
        list->count--;                                                              \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_push_if(NAME *list, DATA_TYPE element, size_t index, bool condition) \
    {                                                                               \
        if (condition)                                                              \
            return PFX##_push(list, element, index);                                \
                                                                                    \
        return false;                                                               \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_pop_if(NAME *list, size_t index, bool condition)                \
    {                                                                               \
        if (condition)                                                              \
            return PFX##_pop(list, index);                                          \
                                                                                    \
        return false;                                                               \
    }                                                                               \
                                                                                    \
    FMOD DATA_TYPE PFX##_front(NAME *list)                                          \
    {                                                                               \
        if (PFX##_empty(list))                                                      \
            return 0;                                                               \
                                                                                    \
        return list->head->data;                                                    \
    }                                                                               \
                                                                                    \
    FMOD DATA_TYPE PFX##_get(NAME *list, size_t index)                              \
    {                                                                               \
        if (index >= list->count)                                                   \
            return 0;                                                               \
                                                                                    \
        if (PFX##_empty(list))                                                      \
            return 0;                                                               \
                                                                                    \
        NAME##_node *scan = PFX##_get_node(list, index);                            \
                                                                                    \
        if (scan == NULL)                                                           \
            return 0;                                                               \
                                                                                    \
        return scan->data;                                                          \
    }                                                                               \
                                                                                    \
    FMOD DATA_TYPE PFX##_back(NAME *list)                                           \
    {                                                                               \
        if (PFX##_empty(list))                                                      \
            return 0;                                                               \
                                                                                    \
        return list->tail->data;                                                    \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_empty(NAME *list)                                               \
    {                                                                               \
        return list->count == 0;                                                    \
    }                                                                               \
                                                                                    \
    FMOD size_t PFX##_count(NAME *list)                                             \
    {                                                                               \
        return list->count;                                                         \
    }                                                                               \
                                                                                    \
    FMOD NAME##_node *PFX##_new_node(NAME *_owner_, DATA_TYPE element)              \
    {                                                                               \
        NAME##_node *node = malloc(sizeof(NAME##_node));                            \
                                                                                    \
        if (!node)                                                                  \
            return NULL;                                                            \
                                                                                    \
        node->owner = _owner_;                                                      \
        node->data = element;                                                       \
        node->next = NULL;                                                          \
        node->prev = NULL;                                                          \
                                                                                    \
        return node;                                                                \
    }                                                                               \
                                                                                    \
    FMOD NAME##_node *PFX##_front_node(NAME *list)                                  \
    {                                                                               \
        return list->head;                                                          \
    }                                                                               \
                                                                                    \
    FMOD NAME##_node *PFX##_get_node(NAME *list, size_t index)                      \
    {                                                                               \
        if (index >= list->count)                                                   \
            return NULL;                                                            \
                                                                                    \
        if (PFX##_empty(list))                                                      \
            return NULL;                                                            \
                                                                                    \
        NAME##_node *scan = NULL;                                                   \
                                                                                    \
        if (index <= list->count / 2)                                               \
        {                                                                           \
            scan = list->head;                                                      \
            for (size_t i = 0; i < index; i++)                                      \
            {                                                                       \
                scan = scan->next;                                                  \
            }                                                                       \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            scan = list->tail;                                                      \
            for (size_t i = list->count - 1; i > index; i--)                        \
            {                                                                       \
                scan = scan->prev;                                                  \
            }                                                                       \
        }                                                                           \
                                                                                    \
        return scan;                                                                \
    }                                                                               \
                                                                                    \
    FMOD NAME##_node *PFX##_back_node(NAME *list)                                   \
    {                                                                               \
        return list->tail;                                                          \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_insert_nxt(NAME##_node *node, DATA_TYPE element)                \
    {                                                                               \
        NAME##_node *new_node = PFX##_new_node(node->owner, element);               \
                                                                                    \
        if (!new_node)                                                              \
            return false;                                                           \
                                                                                    \
        new_node->next = node->next;                                                \
        if (node->next != NULL)                                                     \
            node->next->prev = new_node;                                            \
        else                                                                        \
            node->owner->tail = new_node;                                           \
                                                                                    \
        new_node->prev = node;                                                      \
        node->next = new_node;                                                      \
                                                                                    \
        node->owner->count++;                                                       \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_insert_prv(NAME##_node *node, DATA_TYPE element)                \
    {                                                                               \
        NAME##_node *new_node = PFX##_new_node(node->owner, element);               \
                                                                                    \
        if (!new_node)                                                              \
            return false;                                                           \
                                                                                    \
        new_node->prev = node->prev;                                                \
        if (node->prev != NULL)                                                     \
            node->prev->next = new_node;                                            \
        else                                                                        \
            node->owner->head = new_node;                                           \
                                                                                    \
        new_node->next = node;                                                      \
        node->prev = new_node;                                                      \
                                                                                    \
        node->owner->count++;                                                       \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_remove_nxt(NAME##_node *node)                                   \
    {                                                                               \
        if (node->next == NULL)                                                     \
            return false;                                                           \
                                                                                    \
        NAME##_node *tmp = node->next;                                              \
                                                                                    \
        if (node->next != NULL)                                                     \
        {                                                                           \
            node->next = node->next->next;                                          \
            node->next->prev = node;                                                \
        }                                                                           \
        else                                                                        \
            node->owner->tail = node;                                               \
                                                                                    \
        node->owner->count--;                                                       \
                                                                                    \
        free(tmp);                                                                  \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_remove_cur(NAME##_node *node)                                   \
    {                                                                               \
        if (node->prev != NULL)                                                     \
            node->prev->next = node->next;                                          \
        else                                                                        \
            node->owner->head = node->next;                                         \
                                                                                    \
        if (node->next != NULL)                                                     \
            node->next->prev = node->prev;                                          \
        else                                                                        \
            node->owner->tail = node->prev;                                         \
                                                                                    \
        node->owner->count--;                                                       \
                                                                                    \
        free(node);                                                                 \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_remove_prv(NAME##_node *node)                                   \
    {                                                                               \
        if (node->prev == NULL)                                                     \
            return false;                                                           \
                                                                                    \
        NAME##_node *tmp = node->prev;                                              \
                                                                                    \
        if (node->prev != NULL)                                                     \
        {                                                                           \
            node->prev = node->prev->prev;                                          \
            node->prev->next = node;                                                \
        }                                                                           \
        else                                                                        \
            node->owner->head = node;                                               \
                                                                                    \
        free(tmp);                                                                  \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD NAME##_node *PFX##_next_node(NAME##_node *node)                            \
    {                                                                               \
        return node->next;                                                          \
    }                                                                               \
                                                                                    \
    FMOD NAME##_node *PFX##_prev_node(NAME##_node *node)                            \
    {                                                                               \
        return node->prev;                                                          \
    }                                                                               \
                                                                                    \
    FMOD void PFX##_iter_new(NAME##_iter *iter, NAME *target)                       \
    {                                                                               \
        iter->target = target;                                                      \
        iter->cursor = target->head;                                                \
        iter->index = 0;                                                            \
        iter->start = true;                                                         \
        iter->end = PFX##_empty(target);                                            \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_iter_start(NAME##_iter *iter)                                   \
    {                                                                               \
        return iter->cursor->prev == NULL && iter->start;                           \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_iter_end(NAME##_iter *iter)                                     \
    {                                                                               \
        return iter->cursor->next == NULL && iter->end;                             \
    }                                                                               \
                                                                                    \
    FMOD void PFX##_iter_tostart(NAME##_iter *iter)                                 \
    {                                                                               \
        iter->cursor = iter->target->head;                                          \
        iter->index = 0;                                                            \
        iter->start = true;                                                         \
        iter->end = PFX##_empty(iter->target);                                      \
    }                                                                               \
                                                                                    \
    FMOD void PFX##_iter_toend(NAME##_iter *iter)                                   \
    {                                                                               \
        iter->cursor = iter->target->tail;                                          \
        iter->index = iter->target->count - 1;                                      \
        iter->start = PFX##_empty(iter->target);                                    \
        iter->end = true;                                                           \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_iter_next(NAME##_iter *iter, DATA_TYPE *result, size_t *index)  \
    {                                                                               \
        if (iter->end)                                                              \
            return false;                                                           \
                                                                                    \
        *index = iter->index;                                                       \
        *result = iter->cursor->data;                                               \
        iter->start = false;                                                        \
                                                                                    \
        if (iter->cursor->next == NULL)                                             \
            iter->end = true;                                                       \
        else                                                                        \
        {                                                                           \
            iter->cursor = iter->cursor->next;                                      \
            iter->index++;                                                          \
        }                                                                           \
                                                                                    \
        return true;                                                                \
    }                                                                               \
                                                                                    \
    FMOD bool PFX##_iter_prev(NAME##_iter *iter, DATA_TYPE *result, size_t *index)  \
    {                                                                               \
        if (iter->start)                                                            \
            return false;                                                           \
                                                                                    \
        *index = iter->index;                                                       \
        *result = iter->cursor->data;                                               \
        iter->end = false;                                                          \
                                                                                    \
        if (iter->cursor->prev == NULL)                                             \
            iter->start = true;                                                     \
        else                                                                        \
        {                                                                           \
            iter->cursor = iter->cursor->prev;                                      \
            iter->index--;                                                          \
        }                                                                           \
                                                                                    \
        return true;                                                                \
    }

#endif /* LINKEDLIST_H */
