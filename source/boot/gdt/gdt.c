#include "gdt.h"

// GDT table - statically allocated and aligned
static gdt_entry_t gdt_entries[GDT_ENTRIES_COUNT] __attribute__((aligned(8)));
static gdt_ptr_t gdt_ptr __attribute__((aligned(4)));

// Integrity check values
static uint32_t gdt_checksum = 0;
static bool gdt_initialized = false;

// Calculate simple checksum for integrity checking
static uint32_t calculate_gdt_checksum(void)
{
    uint32_t checksum = 0;
    uint8_t* gdt_bytes = (uint8_t*)gdt_entries;
    
    for (int i = 0; i < (int)sizeof(gdt_entries); i++)
    {
        checksum += gdt_bytes[i];
        checksum = (checksum << 1) | (checksum >> 31); // Rotate left
    }
    
    return checksum;
}

void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    if (index < 0 || index >= GDT_ENTRIES_COUNT)
        return;
        
    // Set base address
    gdt_entries[index].base_low = base & 0xFFFF;
    gdt_entries[index].base_middle = (base >> 16) & 0xFF;
    gdt_entries[index].base_high = (base >> 24) & 0xFF;
    
    // Set limit
    gdt_entries[index].limit_low = limit & 0xFFFF;
    gdt_entries[index].granularity = (limit >> 16) & 0x0F;
    
    // Set granularity and access
    gdt_entries[index].granularity |= gran & 0xF0;
    gdt_entries[index].access = access;
}

bool gdt_setup(void)
{
    // Clear GDT entries first
    for (int i = 0; i < GDT_ENTRIES_COUNT; i++)
    {
        gdt_entries[i] = (gdt_entry_t){0, 0, 0, 0, 0, 0};
    }
    
    // Set up GDT entries
    // Null descriptor (required)
    gdt_set_entry(GDT_NULL_ENTRY, 0, 0, 0, 0);
    
    // Kernel code segment: Base=0, Limit=4GB, Ring 0, Executable, Readable
    gdt_set_entry(GDT_KERNEL_CODE_ENTRY, 
                  0,                    // Base
                  0xFFFFF,              // Limit (4GB with 4K granularity)
                  0x9A,                 // Present, Ring 0, Code, Readable
                  0xCF);                // 4K granularity, 32-bit, limit[19:16] = 0xF
    
    // Kernel data segment: Base=0, Limit=4GB, Ring 0, Writable
    gdt_set_entry(GDT_KERNEL_DATA_ENTRY,
                  0,                    // Base
                  0xFFFFF,              // Limit (4GB with 4K granularity)  
                  0x92,                 // Present, Ring 0, Data, Writable
                  0xCF);                // 4K granularity, 32-bit, limit[19:16] = 0xF
    
    // User code segment: Base=0, Limit=4GB, Ring 3, Executable, Readable
    // Access: 0xFA = 11111010b = Present, DPL=3, Code, Readable
    gdt_set_entry(GDT_USER_CODE_ENTRY,
                  0,                    // Base
                  0xFFFFF,              // Limit (4GB with 4K granularity)
                  0xFA,                 // Present, Ring 3, Code, Readable
                  0xCF);                // 4K granularity, 32-bit, limit[19:16] = 0xF
    
    // User data segment: Base=0, Limit=4GB, Ring 3, Writable
    // Access: 0xF2 = 11110010b = Present, DPL=3, Data, Writable
    gdt_set_entry(GDT_USER_DATA_ENTRY,
                  0,                    // Base
                  0xFFFFF,              // Limit (4GB with 4K granularity)
                  0xF2,                 // Present, Ring 3, Data, Writable
                  0xCF);                // 4K granularity, 32-bit, limit[19:16] = 0xF
    
    // Set up GDT pointer with all 5 entries
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES_COUNT) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;
    
    // Load the GDT first
    gdt_load((uint32_t)&gdt_ptr);
    
    // Calculate integrity checksum AFTER loading
    // This accounts for any CPU modifications (like accessed bit)
    gdt_checksum = calculate_gdt_checksum();
    
    gdt_initialized = true;
    
    // Now verify integrity
    return gdt_verify_integrity();
}

bool gdt_verify_integrity(void)
{
    if (!gdt_initialized)
        return false;
        
    uint32_t current_checksum = calculate_gdt_checksum();
    if (current_checksum != gdt_checksum)
        return false;
    
    if (gdt_entries[GDT_NULL_ENTRY].limit_low != 0 ||
        gdt_entries[GDT_NULL_ENTRY].base_low != 0 ||
        gdt_entries[GDT_NULL_ENTRY].access != 0)
        return false;
    
    // Check kernel code segment (access = 0x9A or 0x9B with accessed bit)
    if ((gdt_entries[GDT_KERNEL_CODE_ENTRY].access & 0xFE) != 0x9A)
        return false;
    
    // Check kernel data segment (access = 0x92 or 0x93 with accessed bit)
    if ((gdt_entries[GDT_KERNEL_DATA_ENTRY].access & 0xFE) != 0x92)
        return false;
    
    // Check user code segment (access = 0xFA or 0xFB with accessed bit)
    if ((gdt_entries[GDT_USER_CODE_ENTRY].access & 0xFE) != 0xFA)
        return false;
    
    // Check user data segment (access = 0xF2 or 0xF3 with accessed bit)
    if ((gdt_entries[GDT_USER_DATA_ENTRY].access & 0xFE) != 0xF2)
        return false;
    
    if (gdt_ptr.base != (uint32_t)&gdt_entries ||
        gdt_ptr.limit != (sizeof(gdt_entry_t) * GDT_ENTRIES_COUNT) - 1)
        return false;
    
    return true;
}
