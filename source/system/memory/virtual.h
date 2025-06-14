#ifndef K_MEM_VIRT_H
#define K_MEM_VIRT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER 0x4

#define VMM_TOTAL_SECTORS 65536
#define VMM_SECTOR_SIZE (2 * 4096)

void mem_virt_init(void* virt_start, size_t virt_size);
void* mem_virt_alloc();
void* mem_virt_alloc_sectors(size_t num_sectors);
void mem_virt_free(void* addr);
bool mem_virt_map_to_phys(void* virt_addr, void* phys_addr, int flags);

#endif
