// Graphics related functions

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "graphics.h"
#include "info.h"

/* TODO: collapse status->efi_error blocks maybe
* TODO: do i need to free the info and gfx results?? info is callee allocated 
* but with no indication that it needs to be manually freed
* unlike LibLocateHandle (wrapper around LocateHandle and OpenHandle?) which examples all show freeing it
*/
EFI_STATUS init_graphics(OUT gfx_info_t *gfx_info)
{
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx;
    UINT32 mode;
    EFI_HANDLE *handles = NULL;
    UINTN nr_handles;

    Print(L"Entering init graphics, gfx_info: %X\n", gfx_info);

    // old gnu-efi call
    //status = LibLocateHandle(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &nr_handles, &handles);

    // Get handles, resizing buffer as needed
    nr_handles = 0;
    status = gBS->LocateHandle(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &nr_handles, handles);
    if (status == EFI_BUFFER_TOO_SMALL) {
        nr_handles += SIZE_1KB;
        handles = AllocateZeroPool(nr_handles);
        status = gBS->LocateHandle(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &nr_handles, handles);
        
        if (EFI_ERROR(status)) {
            if (handles) {
                FreePool(handles);
                handles = NULL;
            }
            return status; 
        }

        nr_handles = nr_handles / sizeof(EFI_HANDLE);
    } else {
        // Why did this work with size 0?
        return status;
    }

    // iterate over list of handles and pull down information for them, how do we decide which one to use?
    // right now we just find the first handle that has our desired h and v res and use that
    for (UINTN iter = 0; iter < nr_handles; iter++) {
        EFI_HANDLE *handle = handles[iter];
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = NULL;
        UINTN size;

        status = gBS->HandleProtocol(handle, &gEfiGraphicsOutputProtocolGuid, (void **) &gfx);
        if (EFI_ERROR(status)) {
            continue;
        }

        status = find_mode(gfx, &mode);
        if (EFI_ERROR(status)) {
            Print(L"No mode found for handle %d\n", iter);
            continue;
        }

        status = gfx->SetMode(gfx, mode);
        if (EFI_ERROR(status)) {
            Print(L"Failed to set mode for handle %d mode %d\n", iter, mode);
            break;
        }

        status = gfx->QueryMode(gfx, mode, &size, &info);
        if (EFI_ERROR(status)) {
            Print(L"Failed to read set mode for handle %d mode %d\n", iter, mode);
            if (info) {
                FreePool(info);
                info = NULL;
            }
            break;
        } else {
            // Copy out all data we care about to our gfx_info struct
            gfx_info->fb_hres = info->HorizontalResolution;
            gfx_info->fb_vres = info->VerticalResolution;
            gfx_info->fb_base = gfx->Mode->FrameBufferBase;
            gfx_info->fb_size = gfx->Mode->FrameBufferSize;
            gfx_info->fb_pixfmt = info->PixelFormat;
            gfx_info->fb_pixmask = info->PixelInformation;
            gfx_info->fb_pixline = info->PixelsPerScanLine;

            if (info) {
                FreePool(info);
                info = NULL;
            }
            break;
        }
    }

    if (handles) {
        FreePool(handles);
    }

    return status;
}

EFI_STATUS init_graphics_conf(OUT gfx_config_t *gfx_config)
{
    EFI_HANDLE  *gfx_handles;
    EFI_STATUS  status;
    UINT64      size;
    UINTN       num_handles = 0
    gfx_config->num_protos = 0;

    CHAR16 *pxl_fmts[] = {
        L"RGBReserved 8Bpp",
        L"BGRReserved 8Bpp",
        L"PixelBitMask",
        L"PixelBltOnly",
        L"PixelFormatMax"
    };


    status = gBS->LocateHandleBuffer(ByProtocol, &GraphicsOutputProtocol, NULL, &num_handles, &gfx_handles);
    if (EFI_ERROR(status)) {
        Print(L"GraphicsTable LocateHandleBuffer failed.\n");
        return status;
    }

    // TODO: Finish writing the selection stuff

}

/*
 * Iterate through the available modes and find whichever one makes best use of the display
 *
 * TODO: do I need to free all these info allocations? being able to use base_info and pointing it to a later orphaned info block seems to say yes
 */
EFI_STATUS find_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx, OUT UINT32 *mode)
{
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *base_info;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    UINTN size;

    Print(L"Entering find mode, gfx: %X\n", gfx);

    // Get the base mode for comparison against later
    status = gfx->QueryMode(gfx, gfx->Mode->Mode, &size, &base_info);
    if (EFI_ERROR(status)) {
        Print(L"Query for mode %d failed\n", gfx->Mode->Mode);
        if (base_info) {
            FreePool(base_info);
            base_info = NULL;
        }
        return status;
    }

    // Set base values
    *mode = gfx->Mode->Mode;

    for (UINT32 iter = 0; iter < gfx->Mode->MaxMode; iter++) {
        status = gfx->QueryMode(gfx, iter, &size, &info);
        if (EFI_ERROR(status)) {
            Print(L"Query for mode %d failed\n", iter);
            if (info) FreePool(info);
            break;
        }

        // We want one of these pixel formats probably?
        if (info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
             info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
            FreePool(info);
            info = NULL;
            continue;
        }

        // Skip if this mode is greater than what we want
        if (info->VerticalResolution > DESIRED_V_RES &&
             info->HorizontalResolution > DESIRED_H_RES) {
            FreePool(info);
            info = NULL;
            continue;
        }

        // If this is an exact match then bail out, otherwise if this mode is taller than the old one then use it instead
        if (info->VerticalResolution == DESIRED_V_RES &&
             info->HorizontalResolution == DESIRED_H_RES) {
            *mode = iter;
            FreePool(info);
            info = NULL;
            break;
        } else if (info->VerticalResolution > base_info->VerticalResolution) {
            FreePool(base_info);
            base_info = info;
        }
    }

    Print(L"Exiting find mode\n");
    if (base_info) {
        FreePool(base_info);
        base_info = NULL;
    }

    return status;
}

// test graphics, taken from some osdev post
void draw_triangle(const gfx_info_t *gfx_info) {
    UINT32 *loc = (UINT32 *)gfx_info->fb_base;
    UINTN row, col;
    UINT32 color = 0x00ff55ff;
    UINTN width = 100;

    loc += (gfx_info->fb_hres * (gfx_info->fb_vres / 2 - 25) + (gfx_info->fb_hres / 2) - width / 2);

    for (row = 0; row < width / 2; row++) {
        for (col = 0; col < width - row * 2; col++) {
            *loc++ = color;
        }

        loc += (gfx_info->fb_hres - col);
        
        for (col = 0; col < width - row * 2; col++) {
            *loc++ = color;
        }

        loc += (gfx_info->fb_hres - col + 1);
    }

    return;
}
