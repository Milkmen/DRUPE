#ifndef K_STREAM_H
#define K_STREAM_H

#define STREAM_BUF_SIZE 1024

typedef struct __attribute__((packed))
{
    char data[STREAM_BUF_SIZE];
    int head;  // write pointer
    int tail;  // read pointer
    int count; // number of bytes in buffer
}
basic_stream_t;

int stream_write(basic_stream_t* s, const char* buf, int len);
int stream_read(basic_stream_t* s, char* buf, int len);

#endif