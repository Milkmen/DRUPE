#ifndef K_IDT_H
#define K_IDT_H

#include <stdbool.h>
#include <stdint.h>

// Include GDT constants
#include "../gdt/gdt.h"

// IDT Entry structure
typedef struct __attribute__((packed))
{
    uint16_t handler_low;    // Lower 16 bits of handler address
    uint16_t selector;       // Kernel segment selector
    uint8_t reserved;        // Always 0
    uint8_t type_attr;       // Type and attributes
    uint16_t handler_high;   // Upper 16 bits of handler address
} idt_entry_t;

// IDT Pointer structure  
typedef struct __attribute__((packed))
{
    uint16_t limit;          // Size of IDT - 1
    uint32_t base;           // Address of IDT
} idt_ptr_t;

// Task State Segment structure
typedef struct __attribute__((packed))
{
    uint32_t prev_tss;   // Previous TSS selector
    uint32_t esp0;       // Ring 0 stack pointer
    uint32_t ss0;        // Ring 0 stack segment
    uint32_t esp1;       // Ring 1 stack pointer  
    uint32_t ss1;        // Ring 1 stack segment
    uint32_t esp2;       // Ring 2 stack pointer
    uint32_t ss2;        // Ring 2 stack segment
    uint32_t cr3;        // Page directory base
    uint32_t eip;        // Instruction pointer
    uint32_t eflags;     // Flags register
    uint32_t eax, ecx, edx, ebx;  // General purpose registers
    uint32_t esp, ebp, esi, edi;  // More general purpose registers
    uint32_t es, cs, ss, ds, fs, gs;  // Segment selectors
    uint32_t ldt;        // LDT selector
    uint16_t trap;       // Trap flag
    uint16_t iomap_base; // I/O map base address
} tss_entry_t;

// IDT Type and Attribute flags
#define IDT_TYPE_TASK_GATE        0x85    // Task gate
#define IDT_TYPE_INTERRUPT_GATE   0x8E    // Interrupt gate
#define IDT_TYPE_TRAP_GATE        0x8F    // Trap gate
#define IDT_FLAG_PRESENT          0x80    // Present bit
#define IDT_FLAG_RING0            0x00    // Ring 0
#define IDT_FLAG_RING3            0x60    // Ring 3

// IDT constants
#define IDT_ENTRIES_COUNT         256

// Updated GDT entries to include TSS
#define GDT_TSS_ENTRY             5
#define GDT_TSS_SEL               0x28    // TSS selector

// Function declarations
bool idt_setup(void);
bool idt_verify_integrity(void);
void idt_set_entry(int index, uint32_t handler, uint16_t selector, uint8_t type_attr);
void tss_setup(void);
void tss_set_kernel_stack(uint32_t stack);

// Assembly functions (you'll need to implement these)
extern void idt_load(uint32_t idt_ptr);

// Exception handlers (ISR = Interrupt Service Routine)
extern void isr0(void);   // Division by zero
extern void isr1(void);   // Debug
extern void isr2(void);   // NMI
extern void isr3(void);   // Breakpoint
extern void isr4(void);   // Overflow
extern void isr5(void);   // Bound range exceeded
extern void isr6(void);   // Invalid opcode
extern void isr7(void);   // Device not available
extern void isr8(void);   // Double fault
extern void isr10(void);  // Invalid TSS
extern void isr11(void);  // Segment not present
extern void isr12(void);  // Stack fault
extern void isr13(void);  // General protection fault
extern void isr14(void);  // Page fault
extern void isr16(void);  // x87 FPU error
extern void isr128(void); // System call

// Hardware interrupt handlers (IRQ)
extern void irq0(void);   // Timer
extern void irq1(void);   // Keyboard

#endif