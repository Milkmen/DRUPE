#ifndef K_MEM_PHYS_H
#define K_MEM_PHYS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PMM_SECTORS 65536
#define PMM_SECTOR_SIZE 8192

void mem_phys_init(void* start, size_t total_size);
void* mem_phys_alloc();
void* mem_phys_alloc_sectors(size_t num_sectors);
void mem_phys_free(void* addr);

#endif