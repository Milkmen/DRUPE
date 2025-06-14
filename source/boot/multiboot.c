#include "multiboot.h"

uint64_t mb2_get_memory(uint32_t magic, uint32_t addr)
{
    if (magic != 0x36d76289) 
    {
        return 0;
    }

    multiboot_tag_t *tag = (multiboot_tag_t*)(addr + 8);
    uint64_t available_memory = 0;

    while (tag->type != MULTIBOOT_TAG_TYPE_END) 
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) 
        {
            multiboot_tag_mmap_t *mmap_tag = 
                (multiboot_tag_mmap_t*)tag;
            uint8_t *entry_ptr = (uint8_t*)mmap_tag + sizeof(*mmap_tag);
            uint8_t *end = (uint8_t*)mmap_tag + mmap_tag->size;

            while (entry_ptr < end) 
            {
                multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t*)entry_ptr;

                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) 
                {
                    available_memory += entry->len;
                }
                entry_ptr += mmap_tag->entry_size;
            }
            break;
        }
        // Move to next tag (8-byte aligned)
        tag = (multiboot_tag_t*)
        (
            (uint8_t*)tag + ((tag->size + 7) & ~7)
        );
    }

    return available_memory;
}