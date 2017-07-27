// UEFI info structures
#pragma once

#ifndef INFO_H
#define INFO_H

#include <efi.h>
#include <efilib.h>

typedef struct mem_map {
    EFI_MEMORY_DESCRIPTOR   *memory_map;
    UINT32                  desc_version;
    UINTN                   desc_size;
    UINTN                   map_key;
    UINTN                   num_entries; // This is a gnu-efi LibMemoryMap thing, UEFI function returns size in bytes, gnu-efi divides that by desc_size to get number of entries
} mem_map_t;

typedef struct gfx_info {
    UINT16                                  fb_hres; // Horizontal Resolution
    UINT16                                  fb_vres; // Vertical Resolution
    EFI_GRAPHICS_PIXEL_FORMAT               fb_pixfmt;
    EFI_PIXEL_BITMASK                       fb_pixmask; // Currently unused since we don't accept pixelpixelbitmask format
    UINT32                                  fb_pixline;
    EFI_PHYSICAL_ADDRESS                    fb_base;
    UINTN                                   fb_size;
} gfx_info_t;

#endif
