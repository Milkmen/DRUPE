#include "idt.h"

// IDT table - statically allocated and aligned
static idt_entry_t idt_entries[IDT_ENTRIES_COUNT] __attribute__((aligned(8)));
static idt_ptr_t idt_ptr __attribute__((aligned(4)));

// TSS structure for hardware task switching support
static tss_entry_t tss __attribute__((aligned(4)));

// Integrity check
static bool idt_initialized = false;

void idt_set_entry(int index, uint32_t handler, uint16_t selector, uint8_t type_attr)
{
    if (index < 0 || index >= IDT_ENTRIES_COUNT)
        return;
        
    idt_entries[index].handler_low = handler & 0xFFFF;
    idt_entries[index].handler_high = (handler >> 16) & 0xFFFF;
    idt_entries[index].selector = selector;
    idt_entries[index].reserved = 0;
    idt_entries[index].type_attr = type_attr;
}

void tss_setup(void)
{
    // Clear TSS structure
    for (int i = 0; i < (int)sizeof(tss_entry_t); i++)
        ((uint8_t*)&tss)[i] = 0;
    
    // Set up basic TSS fields
    tss.ss0 = GDT_KERNEL_DATA_SEL;  // Kernel stack segment
    tss.esp0 = 0;  // Will be set when switching to user mode
    tss.cs = GDT_KERNEL_CODE_SEL | 0;  // Kernel code segment
    tss.ss = GDT_KERNEL_DATA_SEL | 0;  // Kernel stack segment  
    tss.ds = GDT_KERNEL_DATA_SEL | 0;  // Kernel data segment
    tss.es = GDT_KERNEL_DATA_SEL | 0;
    tss.fs = GDT_KERNEL_DATA_SEL | 0;
    tss.gs = GDT_KERNEL_DATA_SEL | 0;
    
    // Add TSS descriptor to GDT (you'll need to modify your GDT)
    // This goes in gdt.c - TSS descriptor at index 5
    gdt_set_entry(GDT_TSS_ENTRY,
                  (uint32_t)&tss,           // Base address of TSS
                  sizeof(tss_entry_t) - 1,  // Limit
                  0x89,                     // Present, Ring 0, TSS Available
                  0x00);                    // Byte granularity
}

bool idt_setup(void)
{
    // Clear IDT entries
    for (int i = 0; i < IDT_ENTRIES_COUNT; i++)
    {
        idt_entries[i] = (idt_entry_t){0, 0, 0, 0, 0};
    }
    
    // Set up IDT pointer
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES_COUNT) - 1;
    idt_ptr.base = (uint32_t)&idt_entries;
    
    // Set up basic exception handlers (you'll need to implement these)
    idt_set_entry(0, (uint32_t)isr0, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Division by zero
    idt_set_entry(1, (uint32_t)isr1, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Debug
    idt_set_entry(2, (uint32_t)isr2, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // NMI
    idt_set_entry(3, (uint32_t)isr3, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Breakpoint
    idt_set_entry(4, (uint32_t)isr4, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Overflow
    idt_set_entry(5, (uint32_t)isr5, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Bound range exceeded
    idt_set_entry(6, (uint32_t)isr6, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Invalid opcode
    idt_set_entry(7, (uint32_t)isr7, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Device not available
    idt_set_entry(8, (uint32_t)isr8, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);   // Double fault
    idt_set_entry(10, (uint32_t)isr10, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE); // Invalid TSS
    idt_set_entry(11, (uint32_t)isr11, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE); // Segment not present
    idt_set_entry(12, (uint32_t)isr12, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE); // Stack fault
    idt_set_entry(13, (uint32_t)isr13, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE); // General protection fault
    idt_set_entry(14, (uint32_t)isr14, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE); // Page fault
    idt_set_entry(16, (uint32_t)isr16, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE); // x87 FPU error
    
    // Set up hardware interrupt handlers (IRQs)
    idt_set_entry(32, (uint32_t)irq0, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);  // Timer
    idt_set_entry(33, (uint32_t)irq1, GDT_KERNEL_CODE_SEL, IDT_TYPE_INTERRUPT_GATE);  // Keyboard
    
    // Set up system call handler
    idt_set_entry(0x80, (uint32_t)isr128, GDT_KERNEL_CODE_SEL, IDT_TYPE_TRAP_GATE | IDT_FLAG_RING3);
    
    // Set up TSS
    tss_setup();
    
    // Load IDT
    idt_load((uint32_t)&idt_ptr);
    
    idt_initialized = true;
    return true;
}

bool idt_verify_integrity(void)
{
    if (!idt_initialized)
        return false;
        
    // Basic integrity checks
    if (idt_ptr.base != (uint32_t)&idt_entries ||
        idt_ptr.limit != (sizeof(idt_entry_t) * IDT_ENTRIES_COUNT) - 1)
        return false;
        
    // Check that critical handlers are set
    if (idt_entries[0].handler_low == 0 && idt_entries[0].handler_high == 0)
        return false;  // Division by zero handler not set
        
    if (idt_entries[13].handler_low == 0 && idt_entries[13].handler_high == 0)
        return false;  // GPF handler not set
        
    return true;
}

void tss_set_kernel_stack(uint32_t stack)
{
    tss.esp0 = stack;
}