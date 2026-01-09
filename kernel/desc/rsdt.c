#include "rsdt.h"

#include "../lib/include/logging.h"
#include "../lib/include/memcmp.h"
#include "../paging/paging.h"
#include "../memlayout.h"

static void *rsdt_root_ptr = NULL;
static bool extended = false;

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
        LOG_SERIAL("RSDT", "RSDP pointer is NULL");
        return;
    }

    uint64_t rsdt_phys;
    bool is_xsdt_table;

    if (rsdp_ptr->Revision >= 2)
    {
        rsdt_phys = ((struct XSDP_t *) rsdp_ptr)->XsdtAddress;
        is_xsdt_table = true;
    }
    else
    {
        rsdt_phys = (uint64_t) rsdp_ptr->RsdtAddress;
        is_xsdt_table = false;
    }

    struct RSDT_t *rsdt_ptr_mapped = (struct RSDT_t *) (uintptr_t) rsdt_phys;
    uint32_t table_length = rsdt_ptr_mapped->header.Length;

    void *mapped = map_mmio(rsdt_phys, table_length);
    if (mapped == NULL)
    {
        LOG_SERIAL("RSDT", "Failed to map full RSDT (size=%d)", table_length);
        return;
    }
    rsdt_ptr_mapped = (struct RSDT_t *) mapped;

    if (!acpi_checksum_ok(&rsdt_ptr_mapped->header, table_length))
    {
        LOG_SERIAL("RSDT", "RSDT checksum failed");
        rsdt_root_ptr = NULL;
        extended = false;
        return;
    }

    uint32_t entry_size = is_xsdt_table ? sizeof(uint64_t) : sizeof(uint32_t);
    uint32_t entries = (table_length - sizeof(struct ACPISDTHeader)) / entry_size;

    extended = is_xsdt_table;
    rsdt_root_ptr = rsdt_ptr_mapped;

    LOG_SERIAL("RSDT", "Initialized: %d entries, xsdt=%s", entries, is_xsdt_table ? "yes" : "no");
}

uint32_t rsdt_get_entry_count()
{
    if (rsdt_root_ptr == NULL)
    {
        return 0;
    }

    struct RSDT_t *rsdt = (struct RSDT_t *) rsdt_root_ptr;
    uint32_t entry_size = extended ? sizeof(uint64_t) : sizeof(uint32_t);
    return (rsdt->header.Length - sizeof(struct ACPISDTHeader)) / entry_size;
}

struct ACPISDTHeader *rsdt_find_table(const char *signature)
{
    if (rsdt_root_ptr == NULL)
    {
        LOG_SERIAL("RSDT", "rsdt_find_table: RSDT not initialized");
        return NULL;
    }

    uint32_t entries = rsdt_get_entry_count();

    for (uint32_t i = 0; i < entries; i++)
    {
        uint64_t phys_addr;

        if (extended)
        {
            struct XSDT_t *xsdt = (struct XSDT_t *) rsdt_root_ptr;
            phys_addr = xsdt->PointerToOtherSDT[i];
        }
        else
        {
            struct RSDT_t *rsdt = (struct RSDT_t *) rsdt_root_ptr;
            phys_addr = (uint64_t) rsdt->PointerToOtherSDT[i];
        }

        if (phys_addr == 0 || phys_addr < PGSIZE)
        {
            continue;
        }

        struct ACPISDTHeader *header;
        void *mapped = map_mmio(phys_addr, PGSIZE);
        if (mapped == NULL)
        {
            continue;
        }
        header = (struct ACPISDTHeader *) mapped;

        if (memcmp(header->Signature, signature, 4) == 0)
        {
            uint32_t table_len = header->Length;

            void *mapped = map_mmio(phys_addr, table_len);
            if (mapped == NULL)
            {
                LOG_SERIAL("RSDT", "Failed to map full table at 0x%x", (uint32_t) phys_addr);
                return NULL;
            }
            header = (struct ACPISDTHeader *) mapped;

            if (!acpi_checksum_ok(header, table_len))
            {
                LOG_SERIAL("RSDT", "Table '%.4s' checksum failed", signature);
                return NULL;
            }

            return header;
        }
    }

    LOG_SERIAL("RSDT", "Table '%.4s' not found", signature);
    return NULL;
}
