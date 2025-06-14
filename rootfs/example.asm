BITS 32
ORG 0x400000

_start:
    ; Print message
    mov eax, 4          ; SYS_WRITE
    mov ebx, 1          ; STDOUT
    mov ecx, message
    mov edx, 15        ; buffer length
    int 0x80

    ; Print error
    mov eax, 4          ; SYS_WRITE
    mov ebx, 2          ; STDERR
    mov ecx, message
    mov edx, 15        ; buffer length
    int 0x80

    ; Exit
    mov eax, 1          ; SYS_EXIT
    mov ebx, 0
    int 0x80
message:
    db "Hello, World!", 0x0D, 0x0A