#ifndef K_MULTIBOOT_H
#define K_MULTIBOOT_H

#include <stdint.h>

typedef struct  
{
    uint32_t type;
    uint32_t size;
}
multiboot_tag_t;

typedef struct
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
}
multiboot_tag_mmap_t;

typedef struct __attribute__((packed))
{
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} 
multiboot_mmap_entry_t;

#define MULTIBOOT_TAG_TYPE_END    0
#define MULTIBOOT_TAG_TYPE_MMAP   6
#define MULTIBOOT_MEMORY_AVAILABLE 1

uint64_t mb2_get_memory(uint32_t magic, uint32_t addr);

#endif