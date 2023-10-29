#ifndef WGSLMINIFY_MINIFY_H
#define WGSLMINIFY_MINIFY_H

#include "tokenize.h"

bool minify(token_node **head);
bool mangle(token_node **head, char **exclude_names, size_t exclude_count);

#endif
