#include "physical.h"

static uint8_t  mem_phys_map[PMM_SECTORS / 8];
static void*    mem_phys_start; /* Start of usable memory */
static size_t   mem_phys_sectors;

void mem_phys_init(void* start, size_t total_size) 
{
    mem_phys_start = start;
    mem_phys_sectors = total_size / PMM_SECTOR_SIZE;

    for (size_t i = 0; i < PMM_SECTORS / 8; i++) 
    {
        mem_phys_map[i] = 0xFF; // All sectors initially free
    }
}

void* mem_phys_alloc() 
{
    for (int i = 0; i < PMM_SECTORS / 8; ++i) 
    {
        uint8_t c = mem_phys_map[i];
        if (c == 0) continue;

        for (int b = 0; b < 8; ++b) 
        {
            if (c & (1 << b)) 
            {
                mem_phys_map[i] &= ~(1 << b);
                return (void*)((uintptr_t)mem_phys_start + PMM_SECTOR_SIZE * (i * 8 + b));
            }
        }
    }
    return 0;
}

void* mem_phys_alloc_sectors(size_t num_sectors) 
{
    if (num_sectors == 0 || num_sectors > mem_phys_sectors)
        return 0;

    size_t max = mem_phys_sectors - num_sectors + 1;

    for (size_t i = 0; i < max; ++i) 
    {
        size_t byte = i / 8;
        size_t bit = i % 8;

        // Check num_sectors sectors starting at sector i
        size_t j;
        for (j = 0; j < num_sectors; ++j) 
        {
            size_t index = i + j;
            size_t b = index / 8;
            size_t m = index % 8;

            if (!(mem_phys_map[b] & (1 << m))) 
            {
                break;
            }
        }

        // Found a valid range
        if (j == num_sectors) {
            for (j = 0; j < num_sectors; ++j) 
            {
                size_t index = i + j;
                size_t b = index / 8;
                size_t m = index % 8;
                mem_phys_map[b] &= ~(1 << m);
            }
            return (void*)((uintptr_t)mem_phys_start + PMM_SECTOR_SIZE * i);
        }
    }

    return 0; // No space found
}

void mem_phys_free(void* addr) 
{
    size_t offset = ((uintptr_t)addr - (uintptr_t)mem_phys_start) / PMM_SECTOR_SIZE;
    size_t i = offset / 8;
    size_t b = offset % 8;

    mem_phys_map[i] |= (1 << b);
}
