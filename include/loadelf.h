#pragma once

#ifndef LOADELF_H
#define LOADELF_H

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

#define EM_X86_64	62	/* AMD x86-64 architecture */

EFI_STATUS EFIAPI elf_verify_hdr_mem(void *elf_bin); 
EFI_STATUS EFIAPI elf_load_mem(void *elf_bin);
EFI_STATUS EFIAPI elf_load_mem_relo(void *elf_bin);
EFI_STATUS EFIAPI elf_verify_hdr_file(EFI_FILE *elf_file);
EFI_STATUS EFIAPI elf_load_file(EFI_FILE *elf_file);
EFI_STATUS EFIAPI elf_load_file_relo(EFI_FILE *elf_file);

#endif
