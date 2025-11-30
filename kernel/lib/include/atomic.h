#ifndef UNTITLED_OS_ATOMIC_H
#define UNTITLED_OS_ATOMIC_H

#include <stdint.h>

// Атомарное чтение
#define atomic_load(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)

// Атомарная запись
#define atomic_store(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)

// Атомарный обмен (возвращает старое значение)
#define atomic_xchg(ptr, val) __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST)

// Атомарное сложение (возвращает старое значение)
#define atomic_fetch_add(ptr, val) __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)

// Атомарное вычитание (возвращает старое значение)
#define atomic_fetch_sub(ptr, val) __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST)

// Compare and Swap: если *ptr == expected, то *ptr = desired. Возвращает true при успехе.
#define atomic_compare_exchange(ptr, expected, desired) \
    __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#endif //UNTITLED_OS_ATOMIC_H