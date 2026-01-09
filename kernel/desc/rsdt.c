#include "rsdt.h"

#include "../lib/include/logging.h"

static void *rsdt_root_ptr = NULL;
static bool extended = false;

/**
 * @brief Checks if RSDT checksum is valid for the table
 *
 * @param header Pointer to ACPI system description table header
 *
 * @return `true` if checksums match `false` otherwise
 */
bool rsdt_checksum_ok(struct ACPISDTHeader *header)
{
    unsigned char sum = 0;

    for (int i = 0; i < header->Length; i++)
    {
        sum += ((char *) header)[i];
    }

    return sum == 0;
}

/**
 * @brief Getter function to get rsdt table root pointer
 */
void *get_rsdt_root()
{
    return rsdt_root_ptr;
}

/**
 * @brief Getter function to check if RSDT pointer is extendable to XSDT
 */
bool is_xsdt()
{
    return extended;
}

/**
 * @brief Utility function that extracts and verifies RSDT pointer from RSDP table
 */
void init_rsdt(struct RSDP_t *rsdp_ptr)
{
    if (rsdp_ptr == NULL)
    {
        return;
    }

    struct RSDT_t *rsdt_ptr;
    bool is_xsdt;

    if (rsdp_ptr->Revision >= 2)
    {
        rsdt_ptr = (uint32_t *) ((struct XSDP_t *) rsdp_ptr)->XsdtAddress;
        is_xsdt = true;
    }
    else
    {
        rsdt_ptr = (uint32_t *) rsdp_ptr->RsdtAddress;
        is_xsdt = false;
    }

    if (!rsdt_checksum_ok(&rsdt_ptr->header))
    {
        rsdt_ptr = NULL;
        is_xsdt = false;
    }

    if (rsdt_ptr)
    {
        uint8_t entries = (rsdt_ptr->header.Length - sizeof(rsdt_ptr->header)) / (is_xsdt ? 8 : 4);
        LOG_SERIAL(
            "DESCRIPTORS",
            "RSDT found at 0x%p, rev=%d, xsdt=%s, entries=%d",
            rsdt_ptr,
            rsdt_ptr->header.Revision,
            is_xsdt ? "true" : "false",
            entries);
    }
    else
    {
        LOG_SERIAL("DESCRIPTORS", "RSDT not found");
    }

    extended = is_xsdt;
    rsdt_root_ptr = rsdt_ptr;
}
