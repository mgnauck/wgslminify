#include "tokenize.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "keywords.h"

typedef bool (*func_is)(char, size_t);

bool is_name(char c, size_t pos)
{
  return isalpha(c) != 0 || c == '_' || (pos > 0 && isdigit(c) != 0);
}

bool is_number(char c, size_t pos)
{
  if(pos == 0 && !(isdigit(c) != 0 || c == '.'))
    return false;

  if(pos == 1 && (c == 'x' || c == 'X'))
    return true;

  return isdigit(c) != 0 || c == '.' || c == 'i' || c == 'u' || c == 'h' ||
    (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'p' || c == 'P' ||
    c == '+' || c == '-';
}

bool is_symbol(const char *symbol)
{
  for(size_t i=0; i<symbols_count; i++)
    if(strcmp(symbols[i], symbol) == 0)
      return true;
  return false;
}

bool is_keyword(const char *name)
{
  for(size_t i=0; i<keywords_count; i++)
    if(strcmp(keywords[i], name) == 0)
      return true;
  return false;
}

int peek(FILE *file)
{
  return ungetc(fgetc(file), file);
}

bool read_until(FILE *file, char **dst, const char *delimiter)
{
  size_t delimiter_size = strlen(delimiter);
  size_t delimiter_pos = 0;
  int c;
  bool error = false;
  buffer buf = { NULL, 0, 0 };
  
  while(!error && delimiter_pos < delimiter_size && (c = fgetc(file)) != EOF) {
    error = write_buf(&buf, c);
    if(c != delimiter[delimiter_pos])
      delimiter_pos = 0;
    if(c == delimiter[delimiter_pos])
      delimiter_pos++;
  }
  
  if(error) {
    free(buf.ptr);
    return true;
  }
  
  if(delimiter_size == 1 && delimiter[0] == '\n' && delimiter_pos == 1) {
    ungetc(c, file);
    buf.pos--;
  }
  
  *dst = buf_to_str(&buf, true);
  
  return !*dst;
}

bool read_until_is(FILE *file, char **dst, func_is is)
{ 
  int c;
  bool error = false;
  buffer buf = { NULL, 0, 0 };
  
  while(!error && (c = fgetc(file)) != EOF && is(c, buf.pos))
    error = write_buf(&buf, c);
  
  if(error) {
    free(buf.ptr);
    return true;
  }
  
  if(c != EOF)
    ungetc(c, file);
  
  *dst = buf_to_str(&buf, true);
  
  return !*dst;
}

bool read_symbol(FILE *file, char **symbol)
{
  int c;
  buffer buf = { NULL, 0, 0 };
  
  while(buf.pos < max_symbol_len && (c = fgetc(file)) != EOF &&
      ispunct(c) != 0) {
    if(write_buf(&buf, c)) {
      free(buf.ptr);
      return true;
    }
  }

  if(buf.pos < max_symbol_len && c != EOF)
    ungetc(c, file);

  *symbol = buf_to_str(&buf, true);
  if(!*symbol)
    return true;

  size_t symbol_len = strlen(*symbol);
  while(symbol_len > 0 && !is_symbol(*symbol)) {
    symbol_len--;
    ungetc((*symbol)[symbol_len], file);
    (*symbol)[symbol_len] = '\0';
  }
  
  if(symbol_len == 0) {
    free(*symbol);
    *symbol = NULL;
  }

  return false;
}

bool read_block_comment(FILE *file, char **comment)
{
  int c;
  int comment_beg_cnt = 0;
  bool error = false;
  buffer buf = { NULL, 0, 0 };
  
  do {
    c = fgetc(file);
    if(c == '/' && peek(file) == '*')
      comment_beg_cnt++;
    else if(c == '*' && peek(file) == '/')
      comment_beg_cnt--;
    error = write_buf(&buf, c);
  } while(!error && c != EOF && comment_beg_cnt > 0);

  if(!error && c != EOF) {
    c = fgetc(file);
    if(c != EOF)
      error = write_buf(&buf, c);
  }

  if(error) {
    free(buf.ptr);
    *comment = NULL;
    return true;
  }

  return (*comment = buf_to_str(&buf, true)) == NULL;
}

token_node *create_token_node(token_node *last, enum token_type type, void *token)
{
  token_node *tn = malloc(sizeof *tn);
  if(!tn) {
    fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
    return NULL;
  }

  tn->type = type;
  tn->token = token;
  tn->prev = last;
  tn->next = NULL;
  
  if(last)
    last->next = tn;
  
  return tn;
}

token *create_token(const char *value)
{
  token *t = malloc(sizeof *t);
  if(!t) {
    fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
    return NULL;
  }

  t->value = strdup(value);
  if(!t->value) {
    fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
    free(t);
    return NULL;
  }
  
  return t;
}

identifier_token *create_identifier_token(const char *value)
{
  identifier_token *t = malloc(sizeof *t);
  if(!t) {
    fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
    return NULL;
  }

  t->value = strdup(value);
  if(!t->value) {
    fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
    free(t);
    return NULL;
  }

  t->data = NULL;
  
  return t;
}

bool create_token_node_with_token(token_node **last, enum token_type type, const char *value)
{
  void *t = NULL;
  if(value) {
    t = (type == IDENTIFIER) ? (void *)create_identifier_token(value) : (void *)create_token(value);
    if(!t)
      return true;
  }

  token_node *tn = create_token_node(*last, type, t);
  if(!tn) {
    free(t);
    return true;
  }

  *last = tn;

  return false;
}

void print_tokens(const token_node *head)
{
  while(head) {
    switch(head->type) {
      case WHITESPACE:
        printf("whitespace\n");
        break;
      case SUBSTITUTION:
        printf("substitution: %s\n", ((token *)head->token)->value);
        break;
      case COMMENT:
        printf("comment: %s\n", ((token *)head->token)->value);
        break;
      case KEYWORD:
        printf("keyword: %s\n", ((token *)head->token)->value);
        break;
      case IDENTIFIER:
        printf("identifier: %s\n", ((identifier_token *)head->token)->value);
        break;
      case LITERAL:
        printf("number: %s\n", ((token *)head->token)->value);
        break;
      case SYMBOL:
        printf("symbol: %s\n", ((token *)head->token)->value);
        break;
      default:
        printf("UNKNOWN TOKEN\n");
    }
    head = head->next;
  }
}

void print_tokens_as_text(const token_node *head)
{
  while(head) {
     switch(head->type) {
      case WHITESPACE:
      case COMMENT:
      case SUBSTITUTION:
      case KEYWORD:
      case LITERAL:
      case SYMBOL:
        printf("%s", ((token *)head->token)->value);
        break;
      case IDENTIFIER:
        printf("%s", ((identifier_token *)head->token)->value);
        break;
      default:
        printf("<< UNKNOWN TOKEN >>");
    }
    head = head->next;
  }
  printf("\n");
}

void free_token_node(token_node *node)
{
  if(node->type == IDENTIFIER)
    free(((identifier_token *)node->token)->value);
  else
    free(((token *)node->token)->value);
  
  free(node->token);
  free(node);
}

void free_token_nodes(token_node *head)
{
  while(head) {
    token_node *next = head->next;
    free_token_node(head);
    head = next;
  }
}

bool tokenize(FILE *file, token_node **head)
{
  bool error = false;
  token_node *last = *head;
  int c;

  while(!error && (c = fgetc(file)) != EOF) {

    if(isspace(c) != 0) {
      error = create_token_node_with_token(&last, WHITESPACE, " ");
      continue;
    }
   
    if(c == '$' && peek(file) == '{') {
      ungetc(c, file);
      char *subst;
      error = read_until(file, &subst, "}");
      if(!error)
        error = create_token_node_with_token(&last, SUBSTITUTION, subst);
      free(subst);
      continue;
    }

    if(c == '/' && peek(file) == '/') {
      ungetc(c, file);
      char *comment = NULL;
      error = read_until(file, &comment, "\n");
      if(comment) {
        error = create_token_node_with_token(&last, COMMENT, comment);
        free(comment);
        continue;
      }
      if(error)
        continue;
    }

    if(c == '/' && peek(file) == '*') {
      ungetc(c, file);
      char *comment = NULL;
      error = read_block_comment(file, &comment);
      if(comment) {
        error = create_token_node_with_token(&last, COMMENT, comment);
        free(comment);
        continue;
      }
      if(error)
        continue;
    }

    if(c == '_' && !is_name(peek(file), 1)) {
      error = create_token_node_with_token(&last, KEYWORD, "_");
      continue;
    }

    if(is_name(c, 0)) {
      ungetc(c, file);
      char *name;
      error = read_until_is(file, &name, is_name);
      if(!error) {
        if(is_keyword(name))
          error = create_token_node_with_token(&last, KEYWORD, name);
        else
          error = create_token_node_with_token(&last, IDENTIFIER, name);
      }
      free(name);
      continue;
    }

    if(isdigit(c) || (c == '.' && isdigit(peek(file)))) {
      ungetc(c, file);
      char *num;
      error = read_until_is(file, &num, is_number);
      if(!error) {
        error = create_token_node_with_token(&last, LITERAL, num);
      }
      free(num);
      continue;
    }

    if(ispunct(c) != 0) {
      ungetc(c, file);
      char *symbol;
      error = read_symbol(file, &symbol);
      if(symbol) {
        error = create_token_node_with_token(&last, SYMBOL, symbol);
        free(symbol);
        continue;
      }
      if(error)
        continue;
    }

    printf(">>> UNKNOWN TOKEN: '%c'\n", c);
  }

  if(ferror(file) != 0) {
    fprintf(stderr, "Failed to read file: %s\n", strerror(errno));
    error = true;
  }

  if(!error && last) {
    while(last->prev)
      last = last->prev;
    *head = last;
  } else {
    free_token_nodes(last);
    *head = NULL;
  }

  return error;
}
