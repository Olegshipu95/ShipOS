#include "acpi.h"

/**
 * @brief Checks if ACPI checksum is valid for the table
 *
 * @param p Pointer to ACPI table
 * @param len Length of ACPI table in bytes
 *
 * @return `true` if checksums match `false` otherwise
 */
bool acpi_checksum_ok(void *p, uint32_t len)
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len; i++)
        sum += ((uint8_t *) p)[i];
    return sum == 0;
}