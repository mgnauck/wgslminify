#ifndef WGSLMINIFY_TOKENIZE_H
#define WGSLMINIFY_TOKENIZE_H

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

enum token_type {
  COMMENT,
  KEYWORD,
  IDENTIFIER,
  LITERAL,
  SYMBOL,
  WHITESPACE,
  SUBSTITUTION, // Non-WGSL ${expr} javascript template literal
};

typedef struct token_node {
  void *token;
  enum token_type type;
  struct token_node *prev;
  struct token_node *next;
} token_node;

typedef struct token {
  char *value;
} token;

typedef struct identifier_token {
  char *value;
  void *data;
} identifier_token;

bool tokenize(FILE *file, token_node **head);
void print_tokens(const token_node *head);
void print_tokens_as_text(const token_node *head);
void free_token_node(token_node *node);
void free_token_nodes(token_node *head);
bool is_name(char c, size_t pos);

#endif
