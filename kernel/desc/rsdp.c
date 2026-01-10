#include "rsdp.h"
#include "acpi.h"

#include "../lib/include/memcmp.h"
#include "../lib/include/logging.h"

#define EBDA_SEG ((uint16_t *) 0x40E)
#define BIOS_MEM_START (0x000E0000)
#define BIOS_MEM_END (0x00100000)
#define ONE_KB 1024

static void *rsdp_ptr = NULL;
static bool extended = false;

/**
 * @brief Getter function to access RSDP pointer (should call `init_rsdp` first)
 */
void *get_rsdp()
{
    return rsdp_ptr;
}

/**
 * @brief Getter function to check if RSDP pointer is extendable to XSDP
 */
bool is_xsdp()
{
    return extended;
}

/**
 * @brief Scans memory region and tries to find RSDP table inside it via
 * signature and checksum match
 *
 * @param start Memory address to start search from
 * @param end Memory address till which to search
 *
 * @return address of RSDP table if found, 0 otherwise
 */
struct RSDP_t *scan_rsdp(uintptr_t start, uintptr_t end)
{
    for (uintptr_t p = start; p < end; p += 16)
    {
        struct RSDP_t *r = (struct RSDP_t *) p;

        if (memcmp(r->Signature, "RSD PTR ", 8) != 0)
            continue;

        if (!acpi_checksum_ok(r, 20))
            continue;

        if (r->Revision >= 2 && !acpi_checksum_ok(r, ((struct XSDP_t *) r)->Length))
            continue;

        return r;
    }
    return NULL;
}

/**
 * @brief Utility function that tries to find and set RSDP table pointer
 */
void init_rsdp()
{
    uintptr_t ebda_addr = ((uintptr_t) (*EBDA_SEG)) << 4;
    struct RSDP_t *found_rsdp = scan_rsdp(ebda_addr, ebda_addr + ONE_KB);

    if (found_rsdp == NULL)
    {
        found_rsdp = scan_rsdp(BIOS_MEM_START, BIOS_MEM_END);
    }

    if (found_rsdp)
    {
        LOG_SERIAL("RSDP", "Found at %p (rev=%d)", found_rsdp, found_rsdp->Revision);
    }
    else
    {
        LOG_SERIAL("RSDP", "Not found");
    }

    rsdp_ptr = found_rsdp;
}
