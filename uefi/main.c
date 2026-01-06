#include <efi.h>
#include <efilib.h>

typedef void (*kernel_entry_t)(void);

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_FILE_HANDLE Volume, KernelFile;
    EFI_LOADED_IMAGE *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    UINTN KernelSize;
    VOID *KernelBuffer;
    EFI_PHYSICAL_ADDRESS KernelAddress = 0x100000;

    InitializeLib(ImageHandle, SystemTable);

    Print(L"ShipOS EFI Bootloader\n");
    Print(L"Loading kernel...\n");

    Status = uefi_call_wrapper(BS->HandleProtocol, 3,
                               ImageHandle,
                               &LoadedImageProtocol,
                               (VOID **)&LoadedImage);
    if (EFI_ERROR(Status))
    {
        Print(L"Error:  Could not get LoadedImage protocol: %r\n", Status);
        return Status;
    }

    Status = uefi_call_wrapper(BS->HandleProtocol, 3,
                               LoadedImage->DeviceHandle,
                               &FileSystemProtocol,
                               (VOID **)&FileSystem);
    if (EFI_ERROR(Status))
    {
        Print(L"Error:  Could not get FileSystem protocol: %r\n", Status);
        return Status;
    }

    Status = uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Volume);
    if (EFI_ERROR(Status))
    {
        Print(L"Error: Could not open volume: %r\n", Status);
        return Status;
    }

    Status = uefi_call_wrapper(Volume->Open, 5,
                               Volume,
                               &KernelFile,
                               L"\\kernel.bin",
                               EFI_FILE_MODE_READ,
                               0);
    if (EFI_ERROR(Status))
    {
        Print(L"Error: Could not open kernel.bin: %r\n", Status);
        return Status;
    }

    // Получаем размер файла
    EFI_FILE_INFO *FileInfo;
    UINTN FileInfoSize = sizeof(EFI_FILE_INFO) + 200;
    Status = uefi_call_wrapper(BS->AllocatePool, 3,
                               EfiLoaderData,
                               FileInfoSize,
                               (VOID **)&FileInfo);

    Status = uefi_call_wrapper(KernelFile->GetInfo, 4,
                               KernelFile,
                               &GenericFileInfo,
                               &FileInfoSize,
                               FileInfo);
    KernelSize = FileInfo->FileSize;

    // Выделяем память для ядра
    UINTN Pages = (KernelSize + 4095) / 4096;
    Status = uefi_call_wrapper(BS->AllocatePages, 4,
                               AllocateAddress,
                               EfiLoaderData,
                               Pages,
                               &KernelAddress);
    if (EFI_ERROR(Status))
    {
        Print(L"Error: Could not allocate memory for kernel: %r\n", Status);
        return Status;
    }

    // Читаем ядро в память
    KernelBuffer = (VOID *)KernelAddress;
    Status = uefi_call_wrapper(KernelFile->Read, 3,
                               KernelFile,
                               &KernelSize,
                               KernelBuffer);
    if (EFI_ERROR(Status))
    {
        Print(L"Error: Could not read kernel: %r\n", Status);
        return Status;
    }

    Print(L"Kernel loaded at 0x%lx, size:  %lu bytes\n", KernelAddress, KernelSize);

    uefi_call_wrapper(KernelFile->Close, 1, KernelFile);
    uefi_call_wrapper(Volume->Close, 1, Volume);

    // Выходим из Boot Services
    UINTN MemoryMapSize = 0;
    UINTN MemoryMapBufferSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    // Получаем размер карты памяти
    Status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                               &MemoryMapSize,
                               MemoryMap,
                               &MapKey,
                               &DescriptorSize,
                               &DescriptorVersion);

    MemoryMapBufferSize = MemoryMapSize + 16 * 1024; // 16KB extra space
    Status = uefi_call_wrapper(BS->AllocatePool, 3,
                               EfiLoaderData,
                               MemoryMapBufferSize,
                               (VOID **)&MemoryMap);
    if (EFI_ERROR(Status))
    {
        Print(L"AllocatePool for memory map failed: %r\n", Status);
        return Status;
    }

    MemoryMapSize = MemoryMapBufferSize;
    Status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                               &MemoryMapSize,
                               MemoryMap,
                               &MapKey,
                               &DescriptorSize,
                               &DescriptorVersion);
    if (EFI_ERROR(Status))
    {
        Print(L"GetMemoryMap failed: %r\n", Status);
        return Status;
    }

    Status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, MapKey);
    if (EFI_ERROR(Status))
    {
        MemoryMapSize = MemoryMapBufferSize;
        Status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                                   &MemoryMapSize,
                                   MemoryMap,
                                   &MapKey,
                                   &DescriptorSize,
                                   &DescriptorVersion);
        if (EFI_ERROR(Status))
        {
            Print(L"GetMemoryMap retry failed: %r\n", Status);
            return Status;
        }

        Status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, MapKey);
        if (EFI_ERROR(Status))
        {
            Print(L"ExitBootServices retry failed: %r\n", Status);
            return Status;
        }
    }

    // Передаём управление ядру
    kernel_entry_t kernel_main = (kernel_entry_t)KernelAddress;
    kernel_main();

    return EFI_SUCCESS;
}