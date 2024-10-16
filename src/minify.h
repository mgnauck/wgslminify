#ifndef MINIFY_H
#define MINIFY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct token_node token_node;

bool minify(token_node **head);
bool mangle(token_node **head, const char **exclude_names,
    size_t exclude_count, bool print_unused);

#endif
