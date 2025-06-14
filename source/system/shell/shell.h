#ifndef K_SHELL_H
#define K_SHELL_H

#include "stream.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#include <stddef.h>
#include <stdbool.h>

typedef struct __attribute__((packed))
{
    char character;
    char vga_color;
}
shell_char_t;

#define STREAM_STDIN    0
#define STREAM_STDOUT   1
#define STREAM_STDERR   2
#define STREAM_COUNT    3

typedef struct __attribute__((packed))
{
    volatile shell_char_t* memory;
    size_t size;
    size_t cursor;
    size_t width, height;
    char color;
    basic_stream_t streams[STREAM_COUNT];
}
shell_instance_t;

#define SH_VRAM ((volatile shell_char_t*) 0xB8000)

bool sh_init(shell_instance_t* shell, volatile shell_char_t* memory, size_t width, size_t size);

void sh_putc(shell_instance_t* shell, char c);
void sh_puts(shell_instance_t* shell, const char* str);
void sh_putint(shell_instance_t* shell, int value, int base);
void sh_printf(shell_instance_t* shell, const char* fmt, ...);
void sh_clear(shell_instance_t* shell);
void sh_render(shell_instance_t* shell);
int sh_write_stdout(shell_instance_t* shell, const char* buf, int len);
int sh_write_stderr(shell_instance_t* shell, const char* buf, int len);
int sh_write_stream(shell_instance_t* shell, int stream_idx, const char* buf, int len);

#endif