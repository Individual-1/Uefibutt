#pragma once

#ifndef TAR_H
#define TAR_H

#include "stdint.h"

typedef struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag[1];
} tar_header_t;

unsigned int tar_strncmp(const char *s1, const char *s2, UINTN n);
unsigned int tar_size(const char *insize);
uint8_t *tar_get_fileaddr(uint8_t *address, char *filename, uint8_t *max_addr);

#endif
