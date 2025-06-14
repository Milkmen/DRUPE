[BITS 32]

section .text

global idt_load

; void idt_load(uint32_t idt_ptr)
idt_load:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp + 8]  ; Get IDT pointer parameter
    lidt [eax]          ; Load IDT
    
    pop ebp
    ret

; Exception handlers - these create a common stack frame
; and call a C handler function

global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8
global isr10, isr11, isr12, isr13, isr14, isr16, isr128

; Handlers that don't push error codes
isr0:
    push dword 0    ; Dummy error code
    push dword 0    ; Interrupt number
    jmp isr_common

isr1:
    push dword 0
    push dword 1
    jmp isr_common

isr2:
    push dword 0
    push dword 2
    jmp isr_common

isr3:
    push dword 0
    push dword 3
    jmp isr_common

isr4:
    push dword 0
    push dword 4
    jmp isr_common

isr5:
    push dword 0
    push dword 5
    jmp isr_common

isr6:
    push dword 0
    push dword 6
    jmp isr_common

isr7:
    push dword 0
    push dword 7
    jmp isr_common

; Handlers that automatically push error codes
isr8:
    push dword 8    ; Double fault pushes error code
    jmp isr_common

isr10:
    push dword 10   ; Invalid TSS pushes error code
    jmp isr_common

isr11:
    push dword 11   ; Segment not present pushes error code
    jmp isr_common

isr12:
    push dword 12   ; Stack fault pushes error code
    jmp isr_common

isr13:
    push dword 13   ; GPF pushes error code
    jmp isr_common

isr14:
    push dword 14   ; Page fault pushes error code
    jmp isr_common

isr16:
    push dword 0
    push dword 16
    jmp isr_common

; System call handler
isr128:
    push dword 0
    push dword 128
    jmp isr_common

; Hardware interrupt handlers
global irq0, irq1

irq0:
    push dword 0
    push dword 32
    jmp irq_common

irq1:
    push dword 0  
    push dword 33
    jmp irq_common

; Common interrupt handler
extern interrupt_handler
isr_common:
    ; Save all registers
    pushad          ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C handler
    push esp        ; Pass pointer to stack frame
    call interrupt_handler
    add esp, 4      ; Clean up stack
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore general registers
    popad
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret

; Common IRQ handler  
extern irq_handler
irq_common:
    ; Save all registers
    pushad
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C handler
    push esp
    call irq_handler
    add esp, 4
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore general registers
    popad
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt  
    iret