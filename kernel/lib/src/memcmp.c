#include "../include/memcmp.h"

int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *p1 = (const uint8_t *) a;
    const uint8_t *p2 = (const uint8_t *) b;

    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
            return (int) p1[i] - (int) p2[i];
    }
    return 0;
}
