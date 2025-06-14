#ifndef K_GDT_H
#define K_GDT_H

#include <stdbool.h>
#include <stdint.h>

// GDT Entry structure
typedef struct __attribute__((packed))
{
    uint16_t limit_low;      // Lower 16 bits of limit
    uint16_t base_low;       // Lower 16 bits of base
    uint8_t base_middle;     // Next 8 bits of base
    uint8_t access;          // Access flags
    uint8_t granularity;     // Granularity and upper 4 bits of limit
    uint8_t base_high;       // Upper 8 bits of base
} gdt_entry_t;

// GDT Pointer structure
typedef struct __attribute__((packed))
{
    uint16_t limit;          // Size of GDT - 1
    uint32_t base;           // Address of GDT
} gdt_ptr_t;

// GDT Access byte flags
#define GDT_ACCESS_PRESENT    0x80    // Present bit
#define GDT_ACCESS_RING0      0x00    // Ring 0 (kernel)
#define GDT_ACCESS_RING3      0x60    // Ring 3 (user)
#define GDT_ACCESS_EXEC       0x08    // Executable (code segment)
#define GDT_ACCESS_RW         0x02    // Readable/Writable
#define GDT_ACCESS_ACCESSED   0x01    // Accessed bit

// GDT Granularity byte flags
#define GDT_GRAN_4K           0x80    // 4KB granularity
#define GDT_GRAN_32BIT        0x40    // 32-bit protected mode
#define GDT_GRAN_LIMIT_MASK   0x0F    // Upper 4 bits of limit

// Segment selectors
#define GDT_KERNEL_CODE_SEL   0x08    // Kernel code selector
#define GDT_KERNEL_DATA_SEL   0x10    // Kernel data selector
#define GDT_USER_CODE_SEL     0x18    // User code selector  
#define GDT_USER_DATA_SEL     0x20    // User data selector

// GDT entry indices
#define GDT_NULL_ENTRY        0
#define GDT_KERNEL_CODE_ENTRY 1
#define GDT_KERNEL_DATA_ENTRY 2
#define GDT_USER_CODE_ENTRY   3
#define GDT_USER_DATA_ENTRY   4
#define GDT_ENTRIES_COUNT     6
#define GDT_TSS_ENTRY         5
#define GDT_TSS_SEL           0x28

bool gdt_setup(void);
bool gdt_verify_integrity(void);
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

extern void gdt_load(uint32_t gdt_ptr);

#endif