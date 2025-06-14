; multiboot.asm
[BITS 32]
[GLOBAL _start]
[EXTERN kernel_entry]

; Multiboot2 header
section .multiboot_header
align 8
header_start:
    dd 0xE85250D6       ; magic
    dd 0                 ; architecture
    dd header_end - header_start ; length
    dd -(0xE85250D6 + 0 + (header_end - header_start)) ; checksum

    ; request memory map (tag 6)
    dw 6
    dw 0
    dd 8

    ; end tag
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
_start:
    ; Disable interrupts
    cli
    
    ; Set up stack
    mov esp, stack_top
    
    ; Reset EFLAGS
    push dword 0
    popf
    
    ; Push multiboot arguments
    push ebx    ; multiboot info pointer
    push eax    ; multiboot magic
    
    ; Call kernel
    call kernel_entry
    
    ; Hang if kernel returns
    cli
.hang:
    hlt
    jmp .hang