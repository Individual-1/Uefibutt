// Graphics header
#pragma once

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <efi.h>
#include <efilib.h>

#include "info.h"

#define DESIRED_H_RES 1024
#define DESIRED_V_RES 768

EFI_STATUS init_graphics(OUT gfx_info_t *gfx_info);

EFI_STATUS find_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx, OUT UINT32 *mode);

void draw_triangle(const gfx_info_t *gfx_info);

#endif
