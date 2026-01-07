#include <efi.h>
#include <efilib.h>

#define PAGE_SIZE 4096
#define KERNEL_STACK_SIZE (64 * 1024)

#define PTE_PRESENT (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_PAGE_SIZE (1ULL << 7)

#define ELF_MAGIC 0x464C457F
#define PT_LOAD 1

typedef void (*kernel_entry_t)(void);
typedef UINT64 pte_t;

struct gdt_descriptor
{
    UINT16 limit;
    UINT64 base;
} __attribute__((packed));

typedef struct
{
    UINT8 e_ident[16];
    UINT16 e_type;
    UINT16 e_machine;
    UINT32 e_version;
    UINT64 e_entry;
    UINT64 e_phoff;
    UINT64 e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize;
    UINT16 e_phentsize;
    UINT16 e_phnum;
    UINT16 e_shentsize;
    UINT16 e_shnum;
    UINT16 e_shstrndx;
} Elf64_Ehdr;

typedef struct
{
    UINT32 p_type;
    UINT32 p_flags;
    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;
    UINT64 p_filesz;
    UINT64 p_memsz;
    UINT64 p_align;
} Elf64_Phdr;

static pte_t *pml4_table;
static pte_t *pdpt_table;
static pte_t *pd_tables;
static UINT64 *gdt;

static inline void cli(void) { __asm__ volatile("cli"); }
static inline void hlt(void) { __asm__ volatile("hlt"); }
static inline void load_cr3(UINT64 addr) { __asm__ volatile("mov %0, %%cr3" : : "r"(addr) : "memory"); }
static inline void load_gdt(struct gdt_descriptor *gdtr) { __asm__ volatile("lgdt %0" : : "m"(*gdtr)); }

static inline void jump_to_kernel(kernel_entry_t entry, UINT64 stack_top)
{
    __asm__ volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "mov %1, %%rsp\n"
        "pushq $0x08\n"
        "pushq %0\n"
        "retfq\n"
        : : "r"((UINT64)entry), "r"(stack_top) : "memory", "rax");
}

static EFI_STATUS alloc_pages(UINTN num_pages, EFI_PHYSICAL_ADDRESS *addr)
{
    EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, num_pages, addr);
    if (!EFI_ERROR(status))
        SetMem((VOID *)*addr, num_pages * PAGE_SIZE, 0);
    return status;
}

static EFI_STATUS setup_page_tables(void)
{
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS addr;

    status = alloc_pages(1, &addr);
    if (EFI_ERROR(status))
        return status;
    pml4_table = (pte_t *)addr;

    status = alloc_pages(1, &addr);
    if (EFI_ERROR(status))
        return status;
    pdpt_table = (pte_t *)addr;

    status = alloc_pages(4, &addr);
    if (EFI_ERROR(status))
        return status;
    pd_tables = (pte_t *)addr;

    pml4_table[0] = (UINT64)pdpt_table | PTE_PRESENT | PTE_WRITABLE;

    for (int i = 0; i < 4; i++)
        pdpt_table[i] = ((UINT64)pd_tables + i * PAGE_SIZE) | PTE_PRESENT | PTE_WRITABLE;

    UINT64 phys = 0;
    for (int pd = 0; pd < 4; pd++)
    {
        pte_t *table = (pte_t *)((UINT64)pd_tables + pd * PAGE_SIZE);
        for (int e = 0; e < 512; e++, phys += 0x200000)
            table[e] = phys | PTE_PRESENT | PTE_WRITABLE | PTE_PAGE_SIZE;
    }
    return EFI_SUCCESS;
}

static EFI_STATUS setup_gdt(void)
{
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS addr;
    status = alloc_pages(1, &addr);
    if (EFI_ERROR(status))
        return status;
    gdt = (UINT64 *)addr;
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFULL; // Code segment
    gdt[2] = 0x00CF92000000FFFFULL; // Data segment
    return EFI_SUCCESS;
}

static EFI_STATUS load_kernel_elf(EFI_HANDLE ImageHandle, UINT64 *entry_out)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_HANDLE volume, file;
    EFI_FILE_INFO *info;
    UINTN info_size;
    UINT8 *elf;

    status = uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (VOID **)&loaded_image);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle, &FileSystemProtocol, (VOID **)&fs);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &volume);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(volume->Open, 5, volume, &file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
        return status;

    info_size = sizeof(EFI_FILE_INFO) + 256;
    uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, info_size, (VOID **)&info);
    uefi_call_wrapper(file->GetInfo, 4, file, &GenericFileInfo, &info_size, info);

    UINTN size = info->FileSize;
    uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, size, (VOID **)&elf);
    uefi_call_wrapper(file->Read, 3, file, &size, elf);
    uefi_call_wrapper(file->Close, 1, file);
    uefi_call_wrapper(volume->Close, 1, volume);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf;
    if (*(UINT32 *)ehdr->e_ident != ELF_MAGIC || ehdr->e_ident[4] != 2)
        return EFI_LOAD_ERROR;

    Elf64_Phdr *phdr = (Elf64_Phdr *)(elf + ehdr->e_phoff);
    for (UINT16 i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        EFI_PHYSICAL_ADDRESS addr = phdr[i].p_paddr;
        UINTN pages = (phdr[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;

        status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, pages, &addr);
        if (EFI_ERROR(status))
            return status;

        SetMem((VOID *)phdr[i].p_paddr, phdr[i].p_memsz, 0);
        if (phdr[i].p_filesz > 0)
            CopyMem((VOID *)phdr[i].p_paddr, elf + phdr[i].p_offset, phdr[i].p_filesz);
    }

    *entry_out = ehdr->e_entry;
    uefi_call_wrapper(BS->FreePool, 1, elf);
    return EFI_SUCCESS;
}

static EFI_STATUS exit_boot_services(EFI_HANDLE ImageHandle)
{
    EFI_STATUS status;
    UINTN map_size = 0, map_key, desc_size, buf_size;
    UINT32 desc_ver;
    EFI_MEMORY_DESCRIPTOR *map;

    uefi_call_wrapper(BS->GetMemoryMap, 5, &map_size, NULL, &map_key, &desc_size, &desc_ver);
    buf_size = map_size + 8192;
    uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, buf_size, (VOID **)&map);

    map_size = buf_size;
    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &map_size, map, &map_key, &desc_size, &desc_ver);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, map_key);
    if (EFI_ERROR(status))
    {
        map_size = buf_size;
        uefi_call_wrapper(BS->GetMemoryMap, 5, &map_size, map, &map_key, &desc_size, &desc_ver);
        status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, map_key);
    }
    return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    UINT64 kernel_entry, stack_top;
    struct gdt_descriptor gdtr;
    EFI_PHYSICAL_ADDRESS stack_addr;

    InitializeLib(ImageHandle, SystemTable);
    Print(L"ShipOS UEFI Bootloader\n\n");

    status = load_kernel_elf(ImageHandle, &kernel_entry);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to load kernel\n");
        return status;
    }

    status = setup_page_tables();
    if (EFI_ERROR(status))
    {
        Print(L"Failed to setup page tables\n");
        return status;
    }

    status = setup_gdt();
    if (EFI_ERROR(status))
    {
        Print(L"Failed to setup GDT\n");
        return status;
    }

    // Allocate stack
    status = alloc_pages(KERNEL_STACK_SIZE / PAGE_SIZE, &stack_addr);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to allocate stack\n");
        return status;
    }
    stack_top = stack_addr + KERNEL_STACK_SIZE;

    // Allocate kernel heap (2MB - 128MB range)
    EFI_PHYSICAL_ADDRESS heap = 0x8000000 - 1;
    uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData, (120 * 1024 * 1024) / PAGE_SIZE, &heap);

    status = exit_boot_services(ImageHandle);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to exit boot services\n");
        return status;
    }

    cli();
    load_cr3((UINT64)pml4_table);
    gdtr.limit = 23;
    gdtr.base = (UINT64)gdt;
    load_gdt(&gdtr);
    jump_to_kernel((kernel_entry_t)kernel_entry, stack_top);

    while (1)
        hlt();
    return EFI_SUCCESS;
}
