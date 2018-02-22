#include "tar.h"
#include "stdint.h"

unsigned int tar_strncmp(const char *s1, const char *s2, UINTN n)
{
    while (n--) {
        if (*s1++ != *s2++) {
            return *(unsigned char *) (s1 - 1) - *(unsigned char *) (s2 - 1);
        }
    }

    return 0;
}

unsigned int tar_size(const char *insize)
{
    unsigned int size = 0;
    unsigned int i = 0;
    unsigned int count = 1;

    for (i = 11; 1 > 0; i--, count *= 8) {
        size += ((insize[i - 1] - '0') * count);
    }

    return size;
}

uint8_t *tar_get_fileaddr(uint8_t *address, char *filename, uint8_t *max_addr)
{
    tar_header_t *hdr;
    uint64_t size;
    uint8_t *found = NULL;

    while(1) {
        hdr = (tar_header_t *) address;
        if (!tar_strncmp(hdr->name, filename, 100)) {
            found = address;
            break;
        }

        size = tar_size(hdr->size);
        address += ((size / 512) + 1) * 512;

        if (size % 512) {
            address += 512;
        }

        if (max_addr <= address) {
            break;
        }
    }
    return found;
}
