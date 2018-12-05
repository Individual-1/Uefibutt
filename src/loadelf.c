#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>

#include <elf.h>

#include "info.h"
#include "util.h"

/*
 * TODO:
 * * Implement relocations
 * * 
 */

EFI_STATUS EFIAPI elf_verify_hdr_mem(void *elf_bin) 
{
    Elf64_Ehdr *hdr = (Elf64_Ehdr *) elf_bin;

    // Sanity check the ELF header, e_machine value from Wikipedia for x86-64
    if (memcmp(&(hdr->e_ident[EI_MAG0]), ELFMAG, SELFMAG) != 0 ||
            hdr->e_ident[EI_CLASS] != ELFCLASS64 ||
            hdr->e_ident[EI_DATA] != ELFDATANONE ||
            hdr->e_type != ET_DYN ||
            hdr->e_machine != 0x3E ||
            hdr->e_version != EV_CURRENT
       ) {
        return EFI_LOAD_ERROR;
    }

    return EFI_SUCCESS;
}

// Parse the ELF in memory and load it into the desired memory sections
// Returns the entry point address
EFI_PHYSICAL_ADDRESS EFIAPI elf_load_mem(void *elf_bin)
{
    Elf64_Ehdr *hdr = (Elf64_Ehdr *) elf_bin;
    Elf64_Phdr *phdrs = (Elf64_Phdr *) ((UINT8 *) elf_bin + hdr->e_phoff);
    Elf64_Phdr *phdr = NULL;
    EFI_STATUS status;

    for (phdr = phdrs; 
            (UINT8 *) phdr < (UINT8 *) phdrs + (hdr->e_phnum * hdr->e_phentsize); 
            phdr = (Elf64_Phdr *) ((UINT8 *) phdr + hdr->e_phentsize)) {
        switch (phdr->p_type) {
            case PT_LOAD: {
                // 4KB pages, get number of pages this segment takes, rounding up
                UINT64 pages = (phdr->p_memsz + 4096 - 1) & (-4096);
                EFI_PHYSICAL_ADDRESS segment = phdr->p_paddr;
                status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);

                if (EFI_ERROR(status)) {
                    Print(L"Failed to allocate pages for elf load\n");
                    return 0;
                }

                gBS->CopyMem((void *) segment, (void *) ((UINT8 *) elf_bin + phdr->p_offset), phdr->p_filesz);
                break;
            }
        }
    }

    return (EFI_PHYSICAL_ADDRESS) hdr->e_entry;
}

// Returns entry point address
EFI_PHYSICAL_ADDRESS EFIAPI elf_load_mem_relo(void *elf_bin)
{
    Elf64_Ehdr *hdr = (Elf64_Ehdr *) elf_bin;
    Elf64_Phdr *phdrs = (Elf64_Phdr *) ((UINT8 *) elf_bin + hdr->e_phoff);
    UINT64 vsize = 0;
    UINT64 vmin = -1; // UINT64_MAX
    UINTN size;
    EFI_STATUS status;

    for (UINT64 i = 0; i < hdr->e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type == PT_LOAD) {
            if (vsize < (phdr->p_vaddr + phdr->p_memsz)) {
                vsize = phdr->p_vaddr + phdr->p_memsz;
            }

            if (vmin > phdr->p_vaddr) {
                vmin = phdr->p_vaddr;
            }            
        }
    }

    UINT64 pages = (vsize - vmin + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT;

    EFI_PHYSICAL_ADDRESS allocmem = 0x400000; // Default ELF load base
    status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &allocmem);
    if (EFI_ERROR(status)) {
        Print(L"Failed to allocate pages for elf load\n");
        return 0;
    }

    for (UINT64 i = 0; i < hdr->e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        UINTN segsize = phdr->p_filesz;
        EFI_PHYSICAL_ADDRESS segment = allocmem + phdr->p_vaddr;

        if (phdr->p_type == PT_LOAD) {
            gBS->CopyMem((void *) segment, (void *) ((UINT8 *) elf_bin + phdr->p_offset), phdr->p_filesz);
        }
    }

    return (EFI_PHYSICAL_ADDRESS) (allocmem + hdr->e_entry);
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

    // Sanity check the ELF header
    if (memcmp(&(hdr.e_ident[EI_MAG0]), ELFMAG, SELFMAG) != 0 ||
            hdr.e_ident[EI_CLASS] != ELFCLASS64 ||
            hdr.e_ident[EI_DATA] != ELFDATANONE ||
            hdr.e_type != ET_DYN ||
            hdr.e_machine != EM_X86_64 ||
            hdr.e_version != EV_CURRENT
       ) {
        return EFI_LOAD_ERROR;
    }

    return EFI_SUCCESS;
}

// Returns entry point address
EFI_PHYSICAL_ADDRESS EFIAPI elf_load_file(EFI_FILE *elf_file)
{
    Elf64_Ehdr hdr;
    UINT64 pos;
    Elf64_Phdr *phdrs = NULL;
    Elf64_Phdr *phdr = NULL;
    UINTN size;
    EFI_STATUS status;

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
                status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);
                
                if (EFI_ERROR(status)) {
                    Print(L"Failed to allocate pages for elf load\n");
                    return 0;
                }

                elf_file->SetPosition(elf_file, phdr->p_offset);
                lsz = phdr->p_filesz;
                elf_file->Read(elf_file, &lsz, (void *) segment);
                break;
            }
        }
    }

    // Reset position
    elf_file->SetPosition(elf_file, pos);

    return (EFI_PHYSICAL_ADDRESS) hdr.e_entry;
}

// Returns entry point address
EFI_PHYSICAL_ADDRESS EFIAPI elf_load_file_relo(EFI_FILE *elf_file)
{
    Elf64_Ehdr hdr;
    UINT64 pos;
    UINT64 vsize = 0;
    UINT64 vmin = -1; // UINT64_MAX
    UINTN size;
    EFI_STATUS status;

    // Get current position
    elf_file->GetPosition(elf_file, &pos);

    // Read in header
    size = sizeof(Elf64_Ehdr);
    elf_file->Read(elf_file, &size, &hdr);

    elf_file->SetPosition(elf_file, hdr.e_phoff);

    Elf64_Phdr phdrs[hdr.e_phnum];
    size = hdr.e_phnum * hdr.e_phentsize;

    elf_file->Read(elf_file, &size, &phdrs[0]); 

    for (UINT64 i = 0; i < hdr.e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type == PT_LOAD) {
            if (vsize < (phdr->p_vaddr + phdr->p_memsz)) {
                vsize = phdr->p_vaddr + phdr->p_memsz;
            }

            if (vmin > phdr->p_vaddr) {
                vmin = phdr->p_vaddr;
            }            
        }
    }

    UINT64 pages = (vsize - vmin + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT;

    EFI_PHYSICAL_ADDRESS allocmem = 0x400000; // Default ELF load base
    status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &allocmem);
    if (EFI_ERROR(status)) {
        Print(L"Failed to allocate pages for elf load\n");
        return 0;
    }

    for (UINT64 i = 0; i < hdr.e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        UINTN segsize = phdr->p_filesz;
        EFI_PHYSICAL_ADDRESS segment = allocmem + phdr->p_vaddr;

        if (phdr->p_type == PT_LOAD) {
            status = elf_file->SetPosition(elf_file, phdr->p_offset);
            if (EFI_ERROR(status)) {
                Print(L"Failed to set position for segment read\n");
                return 0;
            }

            status = elf_file->Read(elf_file, &segsize, (EFI_PHYSICAL_ADDRESS *) segment);
            if (EFI_ERROR(status)) {
                Print(L"Failed to read elf segment data\n");
                return 0;
            }
        }
    }

    // Reset position
    elf_file->SetPosition(elf_file, pos);

    return (EFI_PHYSICAL_ADDRESS) (allocmem + hdr.e_entry);
}