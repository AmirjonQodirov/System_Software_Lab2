#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "partition.h"

#define FILE_PATH "/proc/partitions"

part *parse_part(char *source) {
    part *result = (part *) malloc(sizeof(part));
    result->maj = strtoll(strtok(source, " \n"), NULL, 10);
    result->min = strtoll(strtok(NULL, " \n"), NULL, 10);
    result->blocks = strtoll(strtok(NULL, " \n"), NULL, 10);
    result->size_mb = result->blocks / 1024;
    char *first = strtok(NULL, " \n");
    result->name = malloc(strlen(first) * sizeof(char));
    strcpy(result->name, first);
    return result;
}

uint64_t get_parts(part *buffer, uint64_t max_length) {
    FILE *part_file = fopen(FILE_PATH, "r");
    char part_str[100];
    fgets(part_str, sizeof(part_str), part_file);
    fgets(part_str, sizeof(part_str), part_file);
    char *ptr = fgets(part_str, sizeof(part_str), part_file);
    uint64_t cnt = 0;
    while (ptr != NULL && cnt < max_length) {
        buffer[cnt++] = *parse_part(part_str);
        ptr = fgets(part_str, sizeof(part_str), part_file);
    }
    fclose(part_file);
    return cnt;
}
