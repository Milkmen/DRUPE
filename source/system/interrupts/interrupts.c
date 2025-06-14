#include "../../boot/idt/idt.h"
#include "../shell/shell.h"

#define SYS_EXIT    0x01
#define SYS_WRITE   0x04
#define SYS_GETPID  0x14
#define SYS_READ    0x03

// Interrupt stack frame structure
typedef struct
{
    uint32_t gs, fs, es, ds;                    // Segment registers
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // General registers (pushad)
    uint32_t int_no, err_code;                  // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;      // Pushed by CPU
} interrupt_frame_t;

// External shell instance
extern shell_instance_t* g_kernel_shell;

// Exception names for debugging
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt", 
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault"
};

// Port I/O functions
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Simple scancode to ASCII conversion for debugging
static char scancode_to_ascii(uint8_t scancode)
{
    static char keymap[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8',    // 0-9
        '9', '0', '-', '=', '\b',                          // 10-14
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // 15-28
        0,    // 29 - Control
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, // 30-42
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, // 43-53
        '*',
        0,  // Alt
        ' ', // Space bar
        0,  // Caps lock
        0,  // 59 - F1 key ... >
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  // < ... F10
        0,  // 69 - Num lock
        0,  // Scroll Lock
        0,  // Home key
        0,  // Up Arrow
        0,  // Page Up
        '-',
        0,  // Left Arrow
        0,
        0,  // Right Arrow
        '+',
        0,  // 79 - End key
        0,  // Down Arrow
        0,  // Page Down
        0,  // Insert Key
        0,  // Delete Key
        0,   0,   0,
        0,  // F11 Key
        0,  // F12 Key
        0,  // All other keys are undefined
    };
    
    if (scancode >= 128) return 0; // Key release
    return keymap[scancode];
}
void handle_syscall(interrupt_frame_t* frame);
void interrupt_handler(interrupt_frame_t* frame)
{
    if (g_kernel_shell) 
    {
        if (frame->int_no < 17) 
        {
            sh_printf(g_kernel_shell, "EXCEPTION: %s (0x%x)\r\n", 
                     exception_messages[frame->int_no], frame->int_no);
            sh_printf(g_kernel_shell, "Error Code: 0x%x\r\n", frame->err_code);
            sh_printf(g_kernel_shell, "EIP: 0x%x, CS: 0x%x\r\n", frame->eip, frame->cs);
            
            // Check if this came from user mode
            if ((frame->cs & 0x3) == 3) 
            {
                sh_puts(g_kernel_shell, "Exception occurred in user mode!\r\n");
            }
        }
        else if (frame->int_no == 0x80) 
        {
            // System call handler
            handle_syscall(frame);
        }
        else 
        {
            sh_printf(g_kernel_shell, "Unknown interrupt: 0x%x\r\n", frame->int_no);
        }
    }
    
    // For now, halt on exceptions (except system calls and breakpoints)
    if (frame->int_no < 32 && frame->int_no != 3) {
        while (1) __asm__ volatile("hlt");
    }
}

void exit_to_kernel(void) {
    extern shell_instance_t* g_kernel_shell;
    if (g_kernel_shell) 
    {
        sh_puts(g_kernel_shell, "Idling...\r\n");
    }

    // Kernel idle loop
    while (1) 
    {
        __asm__ volatile("hlt");
    }
}

void handle_syscall(interrupt_frame_t* frame)
{
    extern uint8_t* kernel_stack_top;
    uint32_t syscall_num = frame->eax;
    uint32_t arg0 = frame->ebx; // stream id
    uint32_t arg1 = frame->ecx; // buffer
    uint32_t arg2 = frame->edx; // length
    
    // Check if call came from user mode
    if ((frame->cs & 0x3) != 3) {
        sh_puts(g_kernel_shell, "System call from kernel mode - ignoring\r\n");
        return;
    }

    switch (syscall_num) 
    {
        case SYS_WRITE:
            {
                const char* str = (const char*)arg1;
                if (g_kernel_shell && str) 
                {
                    // More permissive range check for user memory
                    if (arg1 >= 0x400000 && arg1 < 0x900000) 
                    {
                        // Use the provided length instead of searching for null terminator
                        sh_write_stream(g_kernel_shell, arg0, str, arg2);
                        //sh_puts(g_kernel_shell, str);

                        frame->eax = arg2; // Return number of bytes written
                    }
                    else 
                    {
                        sh_printf(g_kernel_shell, "Invalid string pointer: 0x%x\r\n", arg1);
                        frame->eax = -1; // Error
                    }
                } 
                else 
                {
                    frame->eax = -1; // Error
                }
            }
            break;
            
        case SYS_EXIT:
            {
                if (g_kernel_shell) 
                {
                    sh_printf(g_kernel_shell, "Exited with code %d\r\n", arg0);
                }
                
                // Set return value
                frame->eax = 0;
                
                // Switch back to kernel mode by modifying the stack frame
                frame->cs = 0x08;  // Kernel code segment
                frame->ss = 0x10;  // Kernel data segment
                frame->eip = (uint32_t)exit_to_kernel;  // Jump to kernel exit handler
                frame->useresp = (uint32_t)kernel_stack_top;  // Use kernel stack
                    
                // Clear user mode segments
                frame->ds = 0x10;
                frame->es = 0x10;
                frame->fs = 0x10;
                frame->gs = 0x10;
            }
            break;
            
        case SYS_GETPID:
            {
                frame->eax = 1; // Process ID 1
            }
            break;
            
        case SYS_READ:
            {
                char* buf = (char*)arg1;
                size_t count = arg2;
                
                if (g_kernel_shell && buf) 
                {
                    // Check buffer is in user memory
                    if (arg1 >= 0x400000 && arg1 < 0x900000) 
                    {
                        // Read from stdin stream
                        frame->eax = stream_read(&g_kernel_shell->streams[STREAM_STDIN], buf, count);
                    }
                    else 
                    {
                        sh_printf(g_kernel_shell, "Invalid buffer pointer: 0x%x\r\n", arg1);
                        frame->eax = -1;
                    }
                } 
                else 
                {
                    frame->eax = -1;
                }
            }
            break;
            
        default:
            if (g_kernel_shell) {
                sh_printf(g_kernel_shell, "Unknown system call: %d\r\n", syscall_num);
            }
            frame->eax = -1; // Error
            break;
    }
}

void irq_handler(interrupt_frame_t* frame)
{
    if (g_kernel_shell) 
    {
        switch (frame->int_no) 
        {
            case 32: // Timer IRQ0
                break;
                
            case 33: // Keyboard IRQ1
                {
                    uint8_t scancode = inb(0x60);
                    char ascii = scancode_to_ascii(scancode);
                    
                    if (scancode < 128 && ascii) // Key press with valid ASCII
                    { 
                        // Write to stdin stream
                        sh_write_stream(g_kernel_shell, STREAM_STDIN, &ascii, 1);
                    }
                }
                break;
                
            default:
                sh_printf(g_kernel_shell, "Hardware interrupt: IRQ%d (INT 0x%x)\r\n", 
                         frame->int_no - 32, frame->int_no);
                break;
        }
    }
    
    // Send EOI (End of Interrupt) to PIC
    if (frame->int_no >= 40) 
    {
        outb(0xA0, 0x20); // Send EOI to slave PIC
    }
    outb(0x20, 0x20); // Send EOI to master PIC
}

// Initialize PIC with proper debugging
void pic_init(void)
{
    // Save current masks
    uint8_t mask1 = inb(0x21);
    uint8_t mask2 = inb(0xA1);
    
    // Initialize PIC
    outb(0x20, 0x11); // Start initialization sequence for PIC1
    outb(0xA0, 0x11); // Start initialization sequence for PIC2
    
    outb(0x21, 0x20); // PIC1 vector offset (IRQ 0-7 -> INT 32-39)
    outb(0xA1, 0x28); // PIC2 vector offset (IRQ 8-15 -> INT 40-47)
    
    outb(0x21, 0x04); // Tell PIC1 that PIC2 is at IRQ2
    outb(0xA1, 0x02); // Tell PIC2 its cascade identity
    
    outb(0x21, 0x01); // 8086 mode for PIC1
    outb(0xA1, 0x01); // 8086 mode for PIC2
    
    // Enable timer (IRQ0) and keyboard (IRQ1), disable others
    outb(0x21, 0xFC); // 11111100 - Enable IRQ0 and IRQ1
    outb(0xA1, 0xFF); // Disable all IRQ8-15
}

// Function to test if keyboard is working
void keyboard_test(void)
{
    if (g_kernel_shell) {
        sh_puts(g_kernel_shell, "Testing keyboard controller...\r\n");
        
        // Check keyboard controller status
        uint8_t status = inb(0x64);
        sh_printf(g_kernel_shell, "Keyboard controller status: 0x%x\r\n", status);
        
        // Check if output buffer is full
        if (status & 0x01) {
            uint8_t data = inb(0x60);
            sh_printf(g_kernel_shell, "Data in keyboard buffer: 0x%x\r\n", data);
        }
        
        sh_puts(g_kernel_shell, "Press any key to test interrupts...\r\n");
    }
}

// Function to enable specific IRQs
void irq_enable(uint8_t irq)
{
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// Function to disable specific IRQs  
void irq_disable(uint8_t irq)
{
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}