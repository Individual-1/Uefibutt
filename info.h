// UEFI info structures
#pragma once

#ifndef INFO_H
#define INFO_H

#include <Uefi.h>
#include <Library/UefiLib.h>

#define EFI_MAXIMUM_VARIABLE_SIZE   1024

typedef struct mem_map {
    EFI_MEMORY_DESCRIPTOR   *memory_map;
    UINT32                  desc_version;
    UINTN                   desc_size;
    UINTN                   map_key;
    UINTN                   num_entries; 
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
