#pragma once

#ifndef UTIL_H
#define UTIL_H

#include <Uefi.h>
#include <Library/UefiLib.h>

#include "info.h"

int memcmp(const void *a, const void *b, UINTN size);

void print_memory_map(mem_map_t *mem_map);

void efi_waitforkey();

EFI_STATUS efivar_set(CHAR16 *name, UINTN *size, VOID *data, BOOLEAN persist);

EFI_STATUS efivar_get(CHAR16 *name, UINTN *size, VOID **data);

#endif
