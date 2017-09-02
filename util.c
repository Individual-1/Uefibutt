#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "util.h"

const CHAR16 *mem_types[] = {
    L"EfiReservedMemoryType",
    L"EfiLoaderCode",
    L"EfiLoaderData",
    L"EfiBootServicesCode",
    L"EfiBootServicesData",
    L"EfiRuntimeServicesCode",
    L"EfiRuntimeServicesData",
    L"EfiConventionalMemory",
    L"EfiUnusableMemory",
    L"EfiACPIReclaimMemory",
    L"EfiACPIMemoryNVS",
    L"EfiMemoryMappedIO",
    L"EfiMemoryMappedIOPortSpace",
    L"EfiPalCode"
};

int memcmp(const void *a, const void *b, UINTN size) {
    const UINT8 *ap = a;
    const UINT8 *bp = b;

    for (UINTN i = 0; i < size; i++) {
        if (ap[i] < bp[i]) {
            return -1;
        }
        else if (ap[i] > bp[i]) {
            return 1;    
        }
    }

    return 0;
}

// Make a little helper function in case the firmware returns some weird type number
static const CHAR16 * mem_type_to_str(UINT32 type)
{
    // Sanity check
    if (type > sizeof(mem_types) / sizeof(CHAR16 *)) {
        return L"Out of bounds type";
    }

    return mem_types[type];
}

// Print out memory map information, given a mem_map struct
void print_memory_map(mem_map_t *mem_map)
{
    EFI_MEMORY_DESCRIPTOR *cur;

    Print(L"Memory map info\n");
    Print(L"Number of Entries: %d\n", mem_map->num_entries);
    Print(L"Map key: %d\n", mem_map->map_key);
    Print(L"Descriptor Size: %d\n", mem_map->desc_size);
    Print(L"Descriptor Version: %d\n", mem_map->desc_version);
    Print(L"\n");

    cur = mem_map->memory_map;
    for (int i = 0; i < mem_map->num_entries; i++) {
        // Get size of this entry in bytes
        UINTN entry_size = cur->NumberOfPages * 4096;

        // Something is wrong, type doesn't print properly if I try to read it from the array
        Print(L"Type: %s Size: %d\n", mem_type_to_str(cur->Type), entry_size);
        Print(L"Attribute: 0x%x\n", cur->Attribute);
        Print(L"Physical: %016llx-%016llx\n", cur->PhysicalStart, cur->PhysicalStart + entry_size);
        Print(L" Virtual: %016llx-%016llx\n", cur->VirtualStart, cur->VirtualStart + entry_size);
        Print(L"\n");

        cur = (void  *) cur + mem_map->desc_size;
    }

    return;
}

void efi_waitforkey()
{
    EFI_INPUT_KEY key;
    EFI_STATUS status;

    gST->ConIn->Reset(gST->ConIn, FALSE);
    while ((status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key)) == EFI_NOT_READY) ;
}

EFI_STATUS efivar_set(CHAR16 *name, UINTN *size, VOID *data, BOOLEAN persist)
{
    UINT32 attr = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

    if (persist) {
        attr |= EFI_VARIABLE_NON_VOLATILE;
    }

    if (*size > EFI_MAXIMUM_VARIABLE_SIZE) {
        // input is too large for storage
        return EFI_INVALID_PARAMETER;
    }

    return gRT->SetVariable(name, &gUefibuttGuid, attr, *size, data);
}

// Pass in an empty ptr for size and data, they will be populated by the function
// *data must be freed by FreePool, responsibility of caller
EFI_STATUS efivar_get(CHAR16 *name, OUT UINTN *size, OUT VOID **data)
{
    EFI_STATUS status;
    UINT8 dump;

    *size = 0;

    // We don't know how big the resulting efivar will be, will this work?
    // try a failed read to get correct size, will it fail with 0?
    status = gRT->GetVariable(name, &gUefibuttGuid, NULL, size, &dump); 
    if (status == EFI_BUFFER_TOO_SMALL) {
        *data = AllocateZeroPool(*size);
        if (!*data) {
            return EFI_OUT_OF_RESOURCES;
        }

        status = gRT->GetVariable(name, &gUefibuttGuid, NULL, size, *data);

        if (status != EFI_SUCCESS) {
            FreePool(*data);
            *data = NULL;
        }
    }

    return status;
}

