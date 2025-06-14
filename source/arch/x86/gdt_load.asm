[BITS 32]

section .text

global gdt_load

; void gdt_load(uint32_t gdt_ptr)
; Load GDT and update segment registers - SAFE VERSION
gdt_load:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    
    ; Load GDT
    mov eax, [ebp + 8]  ; Get GDT pointer parameter
    lgdt [eax]          ; Load GDT
    
    ; Update data segment registers first
    mov ax, 0x10        ; Kernel data selector (entry 2)
    mov ds, ax          ; Data segment
    mov es, ax          ; Extra segment  
    mov fs, ax          ; F segment
    mov gs, ax          ; G segment
    mov ss, ax          ; Stack segment
    
    ; Far jump to update CS register - this is critical
    ; We need to jump to reload CS with new selector
    jmp 0x08:.reload_cs ; Kernel code selector (entry 1)
    
.reload_cs:
    ; If we get here, GDT loading was successful
    pop ebx
    pop eax
    pop ebp
    ret