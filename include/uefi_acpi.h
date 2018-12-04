#pragma once

#ifndef UEFI_ACPI_H
#define UEFI_ACPI_H

typedef struct {
    CHAR8   signature[8];
    UINT8   checksum;
    CHAR8   oemid[6];
    UINT8   revision;
    UINT32  rsdt_address;
} __attribute__((packed)) rsdp_descriptor_t;

typedef struct {
    rsdp_descriptor_t rsdp_descriptor10;
    UINT32  length;
    UINT64  xsdt_address;
    UINT8   extended_checksum;
    UINT8   reserved[3];
} __attribute__((packed)) rsdp_descriptor20_t;

EFI_STATUS validate_acpi_table(void *acpi_table);

#endif
