// acpi functions

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "info.h"
#include "uefi_acpi.h"

EFI_STATUS validate_acpi_table(void *acpi_table) 
{
    UINT8 rsdp_version = 255;

    if (!acpi_table) {
        return EFI_INVALID_PARAMETER;
    }

    // Determine what version of ACPI this is
    {
        rsdp_descriptor_t *desc = (rsdp_descriptor_t *) acpi_table;
        if (desc->revision == 0) {
            rsdp_version = 0;
        } else if (desc->revision == 2) {
            rsdp_version = 2;
        }
    }

    // Check if this is an invalid ACPI version
    if (rsdp_version == 255) {
        return EFI_INVALID_PARAMETER;
    }

    // First checksum the acpi 1.0 portion
    {
        UINT8 *byte = (UINT8 *) acpi_table;
        UINT64 sum = 0;
        for (UINT32 i = 0; i < sizeof(rsdp_descriptor_t); i++) {
            sum += byte[i];
        }

        // We only care about the last byte of the sum, it must equal 0
        if ((sum & 0xFF) != 0) {
            return EFI_INVALID_PARAMETER;
        }
    }

    // ACPI 1.0 checksum passed, check v2 if we need to
    // v2 checksum is the same process as v1, just starting from the acpi v2 fields
    if (rsdp_version == 2) {
        rsdp_descriptor20_t *desc = (rsdp_descriptor20_t *) acpi_table;
        UINT8 *byte = (UINT8 *) &(desc->length);
        UINT64 sum = 0;
        for (UINT32 i = 0; i < (sizeof(rsdp_descriptor20_t) - sizeof(rsdp_descriptor_t)); i++) {
            sum += byte[i];
        }

        // Same as acpi v1, we only care about the last byte of the sum, must be equal to 0
        if ((sum & 0xFF) != 0) {
            return EFI_INVALID_PARAMETER;
        }
    }

    return EFI_SUCCESS;
}