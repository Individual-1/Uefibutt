#pragma once

#ifndef LOADELF_H
#define LOADELF_H

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS EFIAPI elf_verify_hdr_mem(void *elf_bin); 
EFI_STATUS EFIAPI elf_load_mem(void *elf_bin);
EFI_STATUS EFIAPI elf_verify_hdr_file(EFI_FILE *elf_file);
EFI_STATUS EFIAPI elf_load_file(EFI_FILE *elf_file);

#endif
