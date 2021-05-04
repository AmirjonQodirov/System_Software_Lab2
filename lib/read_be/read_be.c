#include <stdint.h>
#include <byteswap.h>
#include <unistd.h>
#include <bits/types/FILE.h>
#include <stdint.h>
#include "read_be.h"

#define SIZE_64 sizeof(uint64_t)
#define SIZE_32 sizeof(uint32_t)
#define SIZE_16 sizeof(uint16_t)
#define SIZE_8 sizeof(uint8_t)


void read_uint64(uint64_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_64);
    *ptr = bswap_64(*ptr);
}

void read_uint32(uint32_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_32);
    *ptr = bswap_32(*ptr);
}

void read_uint16(uint16_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_16);
    *ptr = bswap_16(*ptr);
}

void read_uint8(uint8_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_8);
}

void read_char(char *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_8);
}

void read_int64(int64_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_64);
    *ptr = bswap_64(*ptr);
}

void read_int32(int32_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_32);
    *ptr = bswap_32(*ptr);
}

void read_int16(int16_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_16);
    *ptr = bswap_16(*ptr);
}

void read_int8(int8_t *ptr, uint64_t fd) {
    read(fd, ptr, SIZE_8);
}
