#include "buffer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t default_increment = 64;

bool write_buf_inc(buffer *buf, char value, size_t buf_inc)
{
  if(buf->pos == buf->size) {
    char *new_ptr = realloc(buf->ptr, buf->size + buf_inc);
  
    if(!new_ptr) {
      fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
      return true;
    }
    
    buf->ptr = new_ptr;
    buf->size += buf_inc;
  }

  buf->ptr[buf->pos++] = value;
  
  return false;
}

bool write_buf(buffer* buf, char value)
{
  return write_buf_inc(buf, value, default_increment);
}

char *buf_to_str(buffer *buf, bool free_buf)
{
  char *str = NULL;
  
  if(!write_buf_inc(buf, '\0', 1)) {
    str = strdup(buf->ptr);
    if(!str)
      fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
  }
  
  if(free_buf) {
    free(buf->ptr);
    buf->ptr = NULL;
    buf->size = 0;
    buf->pos = 0;
  }
  
  return str;
}
