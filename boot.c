// Uefi boot stub using EDKII

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/Uefilib.h>

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

    // init efi lib for use
    InitializeLib(ImageHandle, SystemTable);

    Print(L"Press to do stuff\n");

    // test code please ignore
    ST->ConIn->Reset(ST->ConIn, FALSE);
    while ((status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key)) == EFI_NOT_READY) ;

    // populate memory map, we use the gnu-efi method so we don't have to deal with allocating memory
    mem_map.memory_map = LibMemoryMap(&mem_map.num_entries, &mem_map.map_key, &mem_map.desc_size, &mem_map.desc_version);

    // init graphics
    status = init_graphics(&gfx_info);
    
    CHAR16 str[] = L"string";
    UINTN ts = sizeof(str);

    status = efivar_set(L"test", &ts, str, FALSE);

    status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, mem_map.map_key);
    //TODO error handling
    
    /*
     * Now that we have exited bootservices, we have a much more limited set of commands available
     * At this point, we should have the following set up:
     *  * Paging
     *  * Non-identity mapping
     *  * GDT/IDT for our paging context
     */
    
    status = uefi_call_wrapper(RT->SetVirtualAddressMap, 4, mem_map.num_entries, mem_map.desc_size, mem_map.desc_version, mem_map.memory_map);
    //TODO error handling

    print_memory_map(&mem_map);
    //draw_triangle(&gfx_info);

    return status;
}
