#include "stream.h"

int stream_write(basic_stream_t* s, const char *buf, int len) 
{
    // Ensure we don't write more than buffer size
    if (len > STREAM_BUF_SIZE - s->count) {
        len = STREAM_BUF_SIZE - s->count;
    }
    
    int written = 0;
    while (written < len) 
    {
        s->data[s->head] = buf[written];
        s->head = (s->head + 1) % STREAM_BUF_SIZE;
        s->count++;
        written++;
    }
    return written;
}

int stream_read(basic_stream_t* s, char *buf, int len) 
{
    int read = 0;
    while (read < len && s->count > 0) 
    {
        if(s->data[read] == '\n')
            break;
        buf[read] = s->data[s->tail];
        s->tail = (s->tail + 1) % STREAM_BUF_SIZE;
        s->count--;
        read++;
    }
    return read;
}