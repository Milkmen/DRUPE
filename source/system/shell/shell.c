#include "shell.h"

#include <stdarg.h>
#include <stdint.h>

bool sh_init(shell_instance_t* shell, volatile shell_char_t* memory, size_t width, size_t size)
{
    shell->memory = memory;
    shell->width = width;
    shell->height = size / width;
    shell->size = size;
    shell->color = 0x07;
    shell->cursor = 0;
    
    // Initialize all streams
    for(int i = 0; i < STREAM_COUNT; i++) {
        shell->streams[i].head = 0;
        shell->streams[i].tail = 0;
        shell->streams[i].count = 0;
    }
    
    sh_clear(shell);
    return true;
}

int sh_write_stream(shell_instance_t* shell, int stream_idx, const char* buf, int len)
{
    if (stream_idx < 0 || stream_idx >= STREAM_COUNT) {
        return -1; // Invalid stream index
    }

    // Debug: Print stream buffer contents
    shell->color = 0x07; // Reset to default color
    int written = stream_write(&shell->streams[stream_idx], buf, len);
    sh_render(shell);

    return written;
}

int sh_write_stdout(shell_instance_t* shell, const char* buf, int len)
{
    return sh_write_stream(shell, STREAM_STDOUT, buf, len);
}

int sh_write_stderr(shell_instance_t* shell, const char* buf, int len)
{
    return sh_write_stream(shell, STREAM_STDERR, buf, len);
}

void sh_putc(shell_instance_t* shell, char c)
{
    sh_write_stdout(shell, &c, 1);
}

void sh_render(shell_instance_t* shell)
{
    char buf[64];
    int bytes;
    
    // Process stdout
    while ((bytes = stream_read(&shell->streams[STREAM_STDOUT], buf, sizeof(buf))) > 0) {
        for (int i = 0; i < bytes; i++) {
            char c = buf[i];
            if (c == '\r') {
                shell->cursor -= shell->cursor % shell->width;
            }
            else if (c == '\n') {
                shell->cursor += shell->width - (shell->cursor % shell->width);
                if (shell->cursor >= shell->size) {
                    sh_scroll(shell);
                    shell->cursor -= shell->width;
                }
            }
            else if (shell->cursor < shell->size) {
                shell->memory[shell->cursor] = (shell_char_t){ c, shell->color };
                shell->cursor++;
                if (shell->cursor >= shell->size) {
                    sh_scroll(shell);
                    shell->cursor -= shell->width;
                }
            }
        }
    }

    // Process stderr (in red)
    char saved_color = shell->color;
    shell->color = 0x04; // Red
    while ((bytes = stream_read(&shell->streams[STREAM_STDERR], buf, sizeof(buf))) > 0) {
        for (int i = 0; i < bytes; i++) {
            char c = buf[i];
            if (c == '\r') {
                shell->cursor -= shell->cursor % shell->width;
            }
            else if (c == '\n') {
                shell->cursor += shell->width - (shell->cursor % shell->width);
                if (shell->cursor >= shell->size) {
                    sh_scroll(shell);
                    shell->cursor -= shell->width;
                }
            }
            else if (shell->cursor < shell->size) {
                shell->memory[shell->cursor] = (shell_char_t){ c, shell->color };
                shell->cursor++;
                if (shell->cursor >= shell->size) {
                    sh_scroll(shell);
                    shell->cursor -= shell->width;
                }
            }
        }
    }
    shell->color = saved_color;
}

void sh_scroll(shell_instance_t* shell) 
{
    for (size_t row = 1; row < shell->height; row++) 
    {
        for (size_t col = 0; col < shell->width; col++) 
        {
            size_t dst = (row - 1) * shell->width + col;
            size_t src = row * shell->width + col;
            shell->memory[dst] = shell->memory[src];
        }
    }

    size_t last_line = (shell->height - 1) * shell->width;
    for (size_t col = 0; col < shell->width; col++) 
    {
        shell->memory[last_line + col] = (shell_char_t){ ' ', shell->color };
    }
}

void sh_puts(shell_instance_t* shell, const char* str) 
{
    while (*str) {
        sh_putc(shell, *str++);
    }
}

void sh_putint(shell_instance_t* shell, int value, int base) 
{
    char buffer[32];  // Increased buffer size for safety
    const char* digits = "0123456789abcdef";
    int i = 0;
    unsigned int uvalue;

    if (base == 10 && value < 0) 
    {
        sh_putc(shell, '-');
        uvalue = (unsigned int)(-value);
    }
    else 
    {
        uvalue = (unsigned int)value;
    }

    if (uvalue == 0) {
        sh_putc(shell, '0');
        return;
    }

    while (uvalue && i < (int)sizeof(buffer) - 1)
    {
        buffer[i++] = digits[uvalue % base];
        uvalue /= base;
    }

    while (i > 0) 
    {
        sh_putc(shell, buffer[--i]);
    }
}

void sh_printf(shell_instance_t* shell, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt) 
    {
        if (*fmt == '%') 
        {
            fmt++;
            switch (*fmt) 
            {
                case 's': 
                {
                    const char* s = va_arg(args, const char*);
                    sh_puts(shell, s ? s : "(null)");
                    break;
                }
                case 'd': 
                {
                    int d = va_arg(args, int);
                    sh_putint(shell, d, 10);
                    break;
                }
                case 'u': 
                {
                    unsigned int u = va_arg(args, unsigned int);
                    sh_putint(shell, u, 10);
                    break;
                }
                case 'x': 
                {
                    unsigned int x = va_arg(args, unsigned int);
                    sh_putint(shell, x, 16);
                    break;
                }
                case 'p': 
                {
                    uintptr_t p = (uintptr_t)va_arg(args, void*);
                    sh_puts(shell, "0x");
                    sh_putint(shell, p, 16);
                    break;
                }
                case 'c': 
                {
                    int c = va_arg(args, int);
                    sh_putc(shell, (char)c);
                    break;
                }
                case '%': 
                {
                    sh_putc(shell, '%');
                    break;
                }
                default: 
                {
                    sh_putc(shell, '%');
                    sh_putc(shell, *fmt);
                    break;
                }
            }
        }
        else 
        {
            sh_putc(shell, *fmt);
        }
        fmt++;
    }

    va_end(args);
}

void sh_clear(shell_instance_t* shell)
{
    shell->cursor = 0;

    for (unsigned int i = 0; i < shell->size; i++)
    {
        shell->memory[i] = (shell_char_t){ ' ', shell->color };
    }

    shell->cursor = 0;
}