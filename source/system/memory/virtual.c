#include "virtual.h"
#include "physical.h" // for mem_phys_alloc
#include <stdint.h>

static uint8_t mem_virt_map[VMM_TOTAL_SECTORS / 8];
static void* mem_virt_start;
static size_t mem_virt_sectors;

void mem_virt_init(void* virt_start, size_t virt_size)
{
    mem_virt_start = virt_start;
    mem_virt_sectors = virt_size / VMM_SECTOR_SIZE;

    for (size_t i = 0; i < VMM_TOTAL_SECTORS / 8; i++) 
        mem_virt_map[i] = 0xFF;
}
/*
void* mem_virt_alloc() 
{
    for (size_t i = 0; i < VMM_TOTAL_SECTORS / 8; i++) 
    {
        uint8_t byte = mem_virt_map[i];
        if (byte == 0) continue;

        for (int b = 0; b < 8; b++) 
        {
            if (byte & (1 << b)) 
            {
                mem_virt_map[i] &= ~(1 << b);

                void* virt = (void*)((uintptr_t)mem_virt_start + VMM_SECTOR_SIZE * (i * 8 + b));
                
                void* phys1 = mem_phys_alloc();
                void* phys2 = mem_phys_alloc();
                if (!phys1 || !phys2) return NULL;

                map_page(virt, phys1, 0x3); // Present + Writable
                map_page((void*)((uintptr_t)virt + 0x1000), phys2, 0x3);

                return virt;
            }
        }
    }
    return NULL;
}

void* mem_virt_alloc_sectors(size_t num_sectors) 
{
    if (num_sectors == 0 || num_sectors > mem_virt_sectors)
        return NULL;

    size_t max = mem_virt_sectors - num_sectors + 1;

    for (size_t i = 0; i < max; i++) 
    {
        size_t j;
        for (j = 0; j < num_sectors; j++) 
        {
            size_t index = i + j;
            size_t byte = index / 8;
            size_t bit = index % 8;

            if (!(mem_virt_map[byte] & (1 << bit))) 
                break;
        }

        if (j == num_sectors) 
        {
            for (j = 0; j < num_sectors; j++) 
            {
                size_t index = i + j;
                size_t byte = index / 8;
                size_t bit = index % 8;
                mem_virt_map[byte] &= ~(1 << bit);
            }

            void* virt = (void*)((uintptr_t)mem_virt_start + VMM_SECTOR_SIZE * i);

            for (size_t k = 0; k < num_sectors; k++) 
            {
                void* vaddr = (void*)((uintptr_t)virt + VMM_SECTOR_SIZE * k);
                void* phys1 = mem_phys_alloc();
                void* phys2 = mem_phys_alloc();
                if (!phys1 || !phys2) return NULL;

                map_page(vaddr, phys1, 0x3);
                map_page((void*)((uintptr_t)vaddr + 0x1000), phys2, 0x3);
            }

            return virt;
        }
    }

    return NULL; // no space found
}

void mem_virt_free(void* addr) 
{
    size_t offset = ((uintptr_t)addr - (uintptr_t)mem_virt_start) / VMM_SECTOR_SIZE;
    size_t byte = offset / 8;
    size_t bit = offset % 8;

    mem_virt_map[byte] |= (1 << bit);
    
    // NOTE: optional: unmap_page(addr); unmap_page(addr + 0x1000);
}*/