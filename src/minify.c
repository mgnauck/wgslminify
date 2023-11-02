#include "minify.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"

typedef struct identifier {
  char *value;
  size_t count;
  struct identifier *prev;
  struct identifier *next;
} identifier;

token_node* delete_node(token_node *node)
{
  if(node->prev)
    node->prev->next = node->next;
  if(node->next)
    node->next->prev = node->prev;

  token_node *res = node->prev ? node->prev : node->next;

  free_token_node(node);

  return res;
}

void remove_comments(token_node **head)
{
  token_node *curr = *head;
  while(curr) {
    if(curr->type == COMMENT) {
      curr = delete_node(curr);
      if(!curr || !curr->prev)
        *head = curr;
    }
    if(curr)
      curr = curr->next;
  }
}

void compress_whitespaces(token_node **head)
{
  token_node *curr = *head;
  while(curr) {
    if(curr->type == WHITESPACE && 
        (!curr->prev || !curr->next ||
         ((curr->next &&
           (curr->next->type == WHITESPACE || curr->next->type == SYMBOL)) ||
          (curr->prev &&
          (curr->prev->type == WHITESPACE || curr->prev->type == SYMBOL))))) {
      curr = delete_node(curr);
      if(!curr || !curr->prev)
        *head = curr;
    }
    if(curr)
      curr = curr->next;
  }
}

char *omit_leading_zeros(char *value)
{
  size_t len = strlen(value);
  size_t i = 0;
  while(i < len - 1 && value[i] == '0') {
    if(i + 1 <= len - 1 && isdigit(value[i + 1]) == 0 &&
      !(value[i + 1] == '.' && i + 2 <= len - 1 && isdigit(value[i + 2]) != 0))
      // 0u, 0i, 0.e+4f, 0e+4f, 0h, 0f, 0.h, 0.f
      break;
    i++;
  }
  return value + i;
}

char *omit_trailing_zeros(char *value)
{
  if(strchr(value, '.') == NULL)
    return value;

  size_t len = strlen(value);
  size_t i = len - 1;
  size_t min = value[0] == '.' ? 1 : 0; // .0
  while(i > min && value[i] == '0')
    i--;
  return i == len - 1 ? value : value + i + 1;
}

bool compress_literals(token_node *head)
{
  token_node *curr = head;
  bool error = false;
  
  while(!error && curr) {
    if(curr->type == LITERAL) {
      char *value = ((token *)curr->token)->value;
      if(strchr(value, 'x') == NULL && strchr(value, 'X') == NULL) {  
        char *value_new = omit_leading_zeros(value);
        if(value_new != value) {
          ((token *)curr->token)->value = strdup(value_new);
          if(!((token *)curr->token)->value) {
            fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
            error = true;
          }
          free(value);
          value = ((token *)curr->token)->value;
        } 
        
        if(error)
          continue;

        value_new = omit_trailing_zeros(value);
        if(value_new != value) {
          *value_new = '\0';
          ((token *)curr->token)->value = strdup(value);
          if(!((token *)curr->token)->value) {
            fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
            error = true;
          }
          free(value);
        }
      }
    }
    curr = curr->next;
  }

  return error;
}

bool minify(token_node **head)
{
  remove_comments(head);
  compress_whitespaces(head);
  return compress_literals(*head);
}

void move_identifier(identifier *nominee, identifier *next)
{
  identifier *nnext = nominee->next;
  identifier *nprev = nominee->prev;

  if(nnext)
    nnext->prev = nprev;
  if(nprev)
    nprev->next = nnext;

  nominee->next = next;
  nominee->prev = next->prev;
  if(next->prev)
    next->prev->next = nominee;
  next->prev = nominee;
}

identifier *add_identifier(identifier **first, const char *value)
{
  identifier *curr = *first;
  identifier *prev = *first ? (*first)->prev : NULL;
  while(curr && strcmp(curr->value, value) != 0) {
    prev = curr;
    curr = curr->next;
  }

  if(!curr) {
    curr = malloc(sizeof *curr);
    if(!curr) {
      fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
      return NULL;
    }
    
    curr->value = strdup(value);
    if(!curr->value) {
      fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
      free(curr);
      return NULL;
    }

    curr->count = 0;
    curr->prev = prev;
    curr->next = NULL;

    if(prev)
      prev->next = curr;
    else
      *first = curr;
  }

  curr->count++;

  while(prev && prev->count < curr->count)
    prev = prev->prev;

  if(prev != curr->prev) {
    move_identifier(curr, prev ? prev->next : *first);
    if(!prev)
      *first = curr;
  }

  return curr;
}

bool is_swizzle_comp(const char *name, size_t name_len, const char *values)
{
  for(size_t i=0; i<name_len; i++) {
    const char v = name[i];
    bool valid = false;
    for(size_t j=0; j<4; j++)
      if(v == values[j]) {
        valid = true;
        break;
      }
    if(!valid)
      return false;
  }
  return true;
}

bool is_swizzle_name(const char *name)
{
  const char vec[] = { 'x', 'y', 'z', 'w' };
  const char col[] = { 'r', 'g', 'b', 'a' };
  size_t l = strlen(name);
  if(l > 0 && l <= 4)
    return is_swizzle_comp(name, l, vec) || is_swizzle_comp(name, l, col);
  return false;
}

bool is_excluded(const char *name, char **exclude_names, size_t exclude_count)
{
  for(size_t i=0; i<exclude_count; i++)
    if(strcmp(name, exclude_names[i]) == 0)
      return true;
  return false;
}

bool create_identifier_list(identifier **first, token_node *head, char **exclude_names, size_t exclude_count)
{
  token_node *curr = head;
  while(curr) {
    if(curr->type == IDENTIFIER) {
      const char *value = ((identifier_token *)curr->token)->value;
      if(!is_swizzle_name(value) && // TODO Support non-struct vars with swizzle names
          !is_excluded(value, exclude_names, exclude_count)) {
        identifier *identifier = add_identifier(first, value);
        if(!identifier)
          return false;
        ((identifier_token *)curr->token)->data = identifier;
      }
    }
    curr = curr->next;
  }

  return false;
}

char *eval_name(size_t cnt)
{
  const size_t max_char = 26;
  size_t i = cnt;
  buffer buf = { NULL, 0, 0 };
  while(i > 0) {
    if(write_buf_inc(&buf, 'a' + (char)((i - 1) % max_char), 1)) {
      free(buf.ptr);
      return NULL;
    }
    i = floor((i - 1) / max_char);
  }
  return buf_to_str(&buf, true);
}

bool reassign_identifier_names(identifier *first, char **exclude_names,
    size_t exclude_count)
{
  size_t count = 1;
  while(first) {
    char *subst = eval_name(count++);
    if(!subst)
      return true;
    if(!is_swizzle_name(subst) &&
        !is_excluded(subst, exclude_names, exclude_count)) {
      free(first->value);
      first->value = subst;
      first = first->next;
    } else
      free(subst);
  }

  return false;
}

bool update_identifier_nodes(token_node* head)
{
  while(head) {
    if(head->type == IDENTIFIER) {
      identifier_token *token = (identifier_token *)head->token;
      if(token->data) {
        identifier *id = (identifier *)token->data;
        if(id->value) {
          char *subst = strdup(id->value);
          if(!subst) {
            fprintf(stderr, "Allocation failed: %s\n", strerror(errno));
            return true;
          }
          free(token->value);
          token->value = subst;
          token->data = NULL;
        }
      }
    }
    head = head->next;
  }

  return false;
}

void print_identifier(identifier *first)
{
  while(first) {
    printf("%s (%zu)\n", first->value, first->count);
    first = first->next;
  }
}

void free_identifier(identifier *identifier)
{
  free(identifier->value);
  free(identifier);
}

void free_identifiers(identifier *first)
{
  while(first) {
    identifier *next = first->next;
    free_identifier(first);
    first = next;
  }
}

bool mangle(token_node **head, char **exclude_names, size_t exclude_count)
{
  identifier *first = NULL;
  bool error =
    create_identifier_list(&first, *head, exclude_names, exclude_count);
  
  if(!error)
    error = reassign_identifier_names(first, exclude_names, exclude_count);

  if(!error)
    error = update_identifier_nodes(*head);
  
  free_identifiers(first);

  return error;
}
