
#ifndef SOFTWARE1LIB_READ_BE_H
#define SOFTWARE1LIB_READ_BE_H

#include <bits/types/FILE.h>
#include <stdint.h>

#define SIZE64 sizeof(uint64_t)
#define SIZE32 sizeof(uint32_t)
#define SIZE16 sizeof(uint16_t)
#define SIZE8 sizeof(uint8_t)

void read_uint64(uint64_t *ptr, uint64_t fd);

void read_uint32(uint32_t *ptr, uint64_t fd);

void read_uint16(uint16_t *ptr, uint64_t fd);

void read_uint8(uint8_t *ptr, uint64_t fd);

void read_int64(int64_t *ptr, uint64_t fd);

void read_int32(int32_t *ptr, uint64_t fd);

void read_int16(int16_t *ptr, uint64_t fd);

void read_int8(int8_t *ptr, uint64_t fd);

void read_char(char *ptr, uint64_t fd);

#endif //SOFTWARE1LIB_READ_BE_H
