// Uefi boot stub using EDKII

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "graphics.h"
#include "info.h"
#include "util.h"

mem_map_t mem_map;
gfx_info_t gfx_info;

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
    EFI_INPUT_KEY key;
    UINTN size;
    
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

    Print(L"Press to do stuff\n");

    // test code please ignore
    gST->ConIn->Reset(gST->ConIn, FALSE);
    while ((status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key)) == EFI_NOT_READY) ;

    // Grow buffer to fit memory map
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

    // init graphics
    status = init_graphics(&gfx_info);
    
    CHAR16 str[] = L"string";
    UINTN ts = sizeof(str);

    status = efivar_set(L"test", &ts, str, FALSE);

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
