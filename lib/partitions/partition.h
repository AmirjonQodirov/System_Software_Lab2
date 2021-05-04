#ifndef SOFTWARE1LIB_PARTITION_H
#define SOFTWARE1LIB_PARTITION_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

typedef struct {
    char *name;
    uint32_t maj;
    uint32_t min;
    uint64_t blocks;
    uint64_t size_mb;
} part;

uint64_t get_parts(part *buffer, uint64_t max_length);

#endif //SOFTWARE1LIB_PARTITION_H
