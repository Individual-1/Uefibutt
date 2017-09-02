// Uefi boot stub using EDKII

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

// ACPI
#include <Guid/Acpi.h>

// Filesystem
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>

// Multiprocessing includes
#include <Pi/PiDxeCis.h>
#include <Protocol/MpService.h>

#include "loadelf.h"
#include "graphics.h"
#include "info.h"
#include "util.h"

mem_map_t mem_map;
gfx_info_t gfx_info;
void *acpi_table = NULL;

// define this to copy full elf into memory then parse, unset to read straight from file
#define USE_BUFFER

/*
 * EFI stub
 *
 * Gather information into efi vars then load our kernel into memory and switch execution to it (after exiting boot services)
 *
 * * Memory Map
 * * Video modes
 * * Modules
 *
 * TODO before we exit bootservices:
 * * setup graphics (do mode detection in boot services)
 * * Get handles to pci devices
 * * Populate the memory map
 * * Load in kernel and put it in memory somewhere
 * * Set up GDT and disable interrupts - firmware handles this, but we can also just do it ourselves an set up non-identity mapping
 * * exit boot services then jump to kernel
 *
 * Windows does it in a few stages:
 * * Bootmgr picks OS loader and uses UEFI firmware provided stuff
 * * OS loader reads in windows kernel and sets up
 *  - GDT/IDT (initialized on entry)
 *  - Page tables
 *  - non-identity mapping (virtual addressing not == physical addressing)
 *
 * We should do something along the lines of their OS loader, and do a lot of the mapping ourselves (before we exit boot services)
 *
 * Misc:
 *  * UEFI boot services reclaims memory used (that isnt marked EfiRuntimeServicesCode or EfiRuntimeServicesData) on call to ExitBootServices
 */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) 
{
    EFI_STATUS status;
    UINTN size;

    EFI_MP_SERVICES_PROTOCOL *mps;
    
    // Load up global variables
    if (!(gST = SystemTable)) {
        return EFI_LOAD_ERROR;
    }

    if (!(gBS = SystemTable->BootServices)) {
        return EFI_LOAD_ERROR;
    }

    if (!(gRT = SystemTable->RuntimeServices)) {
        return EFI_LOAD_ERROR;
    }

    /*
     * Read memory map from UEFI
     */

    size = 0;
    status = gBS->GetMemoryMap(&size, mem_map.memory_map, &mem_map.map_key, &mem_map.desc_size, &mem_map.desc_version);
    if (status == EFI_BUFFER_TOO_SMALL) {
        // This is expected, add a little headroom on the size requested and allocate pool
        size += SIZE_1KB;
        mem_map.memory_map = AllocateZeroPool(size);
        status = gBS->GetMemoryMap(&size, mem_map.memory_map, &mem_map.map_key, &mem_map.desc_size, &mem_map.desc_version);
        if (EFI_ERROR(status)) {
            return status;
        }
        mem_map.num_entries = size / mem_map.desc_size;
    } else {
        // Something is wrong, just exit
        return status;
    }

    /*
     * Load acpi tables from firmware
     */
    {
        // How do I EDK2
        EFI_GUID gEfiAcpiTableGuid = EFI_ACPI_TABLE_GUID;
        for (size = 0; size < SystemTable->NumberOfTableEntries; size++) {
            EFI_CONFIGURATION_TABLE *cfg = &(gST->ConfigurationTable[size]);
            if (memcmp(&cfg->VendorGuid, &gEfiAcpiTableGuid, sizeof(cfg->VendorGuid)) == 0) {
                // Matches the ACPI table guid
                acpi_table = cfg->VendorTable;
                break;
            }
        }
    }
    /*
     * Load Pi MpService protocol
     */

    status = gBS->LocateProtocol(&gEfiMpServiceProtocolGuid, NULL, (void **) &mps);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate MPService");
        return status;
    }

    /*
     * Initialize graphics
     */

    status = init_graphics(&gfx_info);
    
    CHAR16 str[] = L"string";
    UINTN ts = sizeof(str);

    status = efivar_set(L"test", &ts, str, FALSE);

    /*
     * load in kernel from filesystem then parse ELF headers and relocate it into memory
     */

    CHAR16 kpath[] = L"\\test\\info.h";
    {
        EFI_LOADED_IMAGE_PROTOCOL *ld_image = NULL;
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
        EFI_FILE *root = NULL;
        EFI_FILE *kfile = NULL;
        EFI_FILE_INFO *finfo = NULL;
        void *kernel = NULL;
        EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;

        status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void **) &ld_image);
        if (EFI_ERROR(status)) {
            Print(L"Failed to load LoadedImageProtocol\n");
            efi_waitforkey();
            return status;
        }

        status = gBS->HandleProtocol(ld_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **) &fs);
        if (EFI_ERROR(status)) {
            Print(L"Failed to load SimpleFileSystemProtocol\n");
            efi_waitforkey();
            return status;
        }

        fs->OpenVolume(fs, &root);

        status = root->Open(root, &kfile, kpath, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
        if (EFI_ERROR(status)) {
            Print(L"Failed to open file %s\n", kpath);
            efi_waitforkey();
            return status;
        }

#ifdef USE_BUFFER
        size = 0;
        status = kfile->GetInfo(kfile, &gEfiFileInfoGuid, &size, NULL);
        if (status == EFI_BUFFER_TOO_SMALL) {
            finfo = AllocateZeroPool(size);
            status = kfile->GetInfo(kfile, &gEfiFileInfoGuid, &size, finfo);
            if (EFI_ERROR(status)) {
                Print(L"GetInfo failed\n");
                efi_waitforkey();
                return status;
            }
        } else {
            // Something is wrong, just exit
            efi_waitforkey();
            return status;
        }
        size = finfo->FileSize;
        kernel = AllocateZeroPool(size);
        status = kfile->Read(kfile, &size, kernel);
        if (EFI_ERROR(status)) {
            Print(L"Read failed\n");
            efi_waitforkey();
            return status;
        }

        FreePool(finfo);
        finfo = NULL;

        status = elf_verify_hdr_mem(kernel);
        if (EFI_ERROR(status)) {
            Print(L"ELF failed to verify\n");
            FreePool(kernel);
            kernel = NULL;
            return status;
        }

        elf_load_mem(kernel);
        FreePool(kernel);
        kernel = NULL;
#else
        status = elf_verify_hdr_file(kfile);
        if (EFI_ERROR(status)) {
            Print(L"ELF failed to verify\n");
            return status;
        }

        elf_load_file(kfile);
#endif

    }

    status = gBS->ExitBootServices(ImageHandle, mem_map.map_key);
    //TODO error handling
    
    /*
     * OLD: ignore this, left it here for now for reference
     * Now that we have exited bootservices, we have a much more limited set of commands available
     * At this point, we should have the following set up:
     *  * Paging
     *  * Non-identity mapping
     *  * GDT/IDT for our paging context
     */

    /*
     * Now that we have exited boot services, lets unpack part of the kernel from our archive and jump to it
     * letting it set up IDT/GDT. Be sure to pass it some things like gfx_info and mem_map though
     */
    
    status = gRT->SetVirtualAddressMap(mem_map.num_entries, mem_map.desc_size, mem_map.desc_version, mem_map.memory_map);
    //TODO error handling

    //print_memory_map(&mem_map);
    draw_triangle(&gfx_info);

    return status;
}
