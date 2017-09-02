#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>

#include <elf.h>

#include "info.h"
#include "util.h"

EFI_STATUS EFIAPI elf_verify_hdr_mem(void *elf_bin) 
{
    Elf64_Ehdr *hdr = (Elf64_Ehdr *) elf_bin;

    // Sanity check the ELF header, e_machine value from Wikipedia for x86-64
    if (memcmp(&(hdr->e_ident[EI_MAG0]), ELFMAG, SELFMAG) != 0 ||
            hdr->e_ident[EI_CLASS] != ELFCLASS64 ||
            hdr->e_ident[EI_DATA] != ELFDATANONE ||
            hdr->e_type != ET_EXEC ||
            hdr->e_machine != 0x3E ||
            hdr->e_version != EV_CURRENT
       ) {
        return EFI_LOAD_ERROR;
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI elf_load_mem(void *elf_bin)
{
    Elf64_Ehdr *hdr = (Elf64_Ehdr *) elf_bin;
    Elf64_Phdr *phdrs = (Elf64_Phdr *) ((UINT8 *) elf_bin + hdr->e_phoff);
    Elf64_Phdr *phdr = NULL;

    for (phdr = phdrs; 
            (UINT8 *) phdr < (UINT8 *) phdrs + (hdr->e_phnum * hdr->e_phentsize); 
            phdr = (Elf64_Phdr *) ((UINT8 *) phdr + hdr->e_phentsize)) {
        switch (phdr->p_type) {
            case PT_LOAD: {
                // 4KB pages, get number of pages this segment takes, rounding up
                UINT64 pages = (phdr->p_memsz + 4096 - 1) & (-4096);
                EFI_PHYSICAL_ADDRESS segment = phdr->p_paddr;
                gBS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);
                gBS->CopyMem((void *) segment, (void *) ((UINT8 *) elf_bin + phdr->p_offset), phdr->p_filesz);
                break;
            }
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI elf_verify_hdr_file(EFI_FILE *elf_file) 
{
    Elf64_Ehdr hdr;
    UINT64 pos;
    UINTN size;

    // Get current position
    elf_file->GetPosition(elf_file, &pos);

    // Read in header
    size = sizeof(Elf64_Ehdr);
    elf_file->Read(elf_file, &size, &hdr);

    // Reset position
    elf_file->SetPosition(elf_file, pos);

    // Sanity check the ELF header, e_machine value from Wikipedia for x86-64
    if (memcmp(&(hdr.e_ident[EI_MAG0]), ELFMAG, SELFMAG) != 0 ||
            hdr.e_ident[EI_CLASS] != ELFCLASS64 ||
            hdr.e_ident[EI_DATA] != ELFDATANONE ||
            hdr.e_type != ET_EXEC ||
            hdr.e_machine != 0x3E ||
            hdr.e_version != EV_CURRENT
       ) {
        return EFI_LOAD_ERROR;
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI elf_load_file(EFI_FILE *elf_file)
{
    Elf64_Ehdr hdr;
    UINT64 pos;
    Elf64_Phdr *phdrs = NULL;
    Elf64_Phdr *phdr = NULL;
    UINTN size;

    // Get current position
    elf_file->GetPosition(elf_file, &pos);

    // Read in header
    size = sizeof(Elf64_Ehdr);
    elf_file->Read(elf_file, &size, &hdr);

    elf_file->SetPosition(elf_file, hdr.e_phoff);

    size = hdr.e_phnum * hdr.e_phentsize;
    phdrs = AllocateZeroPool(size);
    elf_file->Read(elf_file, &size, phdrs); 

    for (phdr = phdrs; 
            (UINT8 *) phdr < (UINT8 *) phdrs + (hdr.e_phnum * hdr.e_phentsize); 
            phdr = (Elf64_Phdr *) ((UINT8 *) phdr + hdr.e_phentsize)) {
        switch (phdr->p_type) {
            case PT_LOAD: {
                // 4KB pages, get number of pages this segment takes, rounding up
                UINT64 pages = (phdr->p_memsz + 4096 - 1) & (-4096);
                EFI_PHYSICAL_ADDRESS segment = phdr->p_paddr;
                UINTN lsz = 0;
                gBS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);
                
                elf_file->SetPosition(elf_file, phdr->p_offset);
                lsz = phdr->p_filesz;
                elf_file->Read(elf_file, &lsz, (void *) segment);
                break;
            }
        }
    }

    // Reset position
    elf_file->SetPosition(elf_file, pos);

    return EFI_SUCCESS;
}
