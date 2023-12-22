#ifndef WGSLMINIFY_BUFFER_H
#define WGSLMINIFY_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct buffer {
  char *ptr;
  size_t size;
  size_t pos;
} buffer;

bool write_buf_inc(buffer *buf, char value, size_t buf_inc);
bool write_buf(buffer* buf, char value);
char *buf_to_str(buffer *buf, bool free_buf);

char *strdup(const char *src);

#endif
