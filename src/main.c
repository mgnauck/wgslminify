/*
Minifcation and identifier mangling for WGSL (WebGPU shading language).
Licensed unter the MIT License. https://mit-license.org

Author: Markus Gnauck
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "tokenize.h"
#include "minify.h"

typedef struct arguments {
  char *filename;
  char *excludes;
  bool no_mangle;
  bool help;
} arguments;

bool handle_arguments(int argc, char *argv[], arguments *args)
{
  bool error = false, stdin_specified = false;

  for(size_t i=1; i<(size_t)argc; i++) {
    if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      args->help = true;
      continue;
    }

    if(strcmp(argv[i], "--no-mangle") == 0) {
      if(!args->excludes) {
        args->no_mangle = true;
        continue;
      } else {
        printf("%s: specify excluded identifiers or --no-mangle\n", argv[0]);
        error = true;
        break;
      }
    }

    if(strcmp(argv[i], "-e") == 0) {
      if(args->no_mangle) {
        printf("%s: specify --no-mangle or excluded identifiers\n", argv[0]);
        error = true;
        break;
      }
      if((size_t)argc >= i + 2 && strcmp(argv[i + 1], "-") != 0) {
        args->excludes = argv[++i];
        continue;
      } else {
        printf("%s: illegal value for option %s\n", argv[0], argv[i]);
        error = true;
        break;
      }
    }

    if(strcmp(argv[i], "-") != 0 && !args->filename) {
      if(!stdin_specified) {
        args->filename = argv[i];
        continue;
      } else {
        printf("%s: specify stdin or input file\n", argv[0]);
        error = true;
        break;
      }
    }
    
    if(strcmp(argv[i], "-") == 0 && !stdin_specified) {
      if(!args->filename) {
        stdin_specified = true;
        continue;
      } else {
        printf("%s: specify input file or stdin\n", argv[0]);
        error = true;
        break;
      }
    }
    
    printf("%s: illegal option %s\n", argv[0], argv[i]);
    error = true;
    break;
  }

  if(args->excludes) {
    size_t len = strlen(args->excludes);
    for(size_t i=0; i<len; i++) {
      if(args->excludes[i] != ',' && !is_name(args->excludes[i], ((i==0) || args->excludes[i - 1] == ',') ? 0 : 1)) {
        printf("%s: illegal character '%c' in excluded identifier\n", argv[0], args->excludes[i]);
        error = true;
      }
    }
  }

  if(args->help || error)
    printf("usage: wgslminify [--no-mangle | -e exclude1,exclude2,...] [file]\n");

  return error;
}

size_t get_excludes_count(const char *excl_str)
{
  size_t cnt = 0;
  while(*excl_str != '\0')
    if(*excl_str++ == ',')
      cnt++;

  return cnt + 1;
}

void free_excludes(char **exclude_names, size_t exclude_count)
{
  for(size_t i=0; i<exclude_count; i++)
    free(exclude_names[i]);
  free(exclude_names);
}

bool parse_excludes(const char *exclude_string, char **exclude_names, size_t *exclude_count)
{
  size_t len = strlen(exclude_string);
  size_t i = 0, cnt = 0;
  buffer buf = { NULL, 0, 0 };
  while(i < len && cnt < *exclude_count) {
    char c = exclude_string[i];
    if(c != ',') {
      if(write_buf(&buf, c)) {
        free(buf.ptr);
        free_excludes(exclude_names, cnt);
        return true;
      }
    }
    if((c == ',' || i + 1 == len) && buf.pos > 0) {
      char *name = buf_to_str(&buf, true);
      if(!name) {
        free_excludes(exclude_names, cnt);
        return true;
      }
      exclude_names[cnt++] = name;
    }
    i++;
  }

  *exclude_count = cnt;

  return false;
}

int main(int argc, char *argv[])
{
  arguments args = { NULL, NULL, false, false };
  if(handle_arguments(argc, argv, &args))
    return EXIT_FAILURE;

  if(args.help)
    return EXIT_SUCCESS;

  FILE *file = NULL;
  if(args.filename) {
    file = fopen(args.filename, "rt");
    if(file == NULL) {
      fprintf(stderr, "Failed to open '%s': %s\n", args.filename, strerror(errno));
      exit(EXIT_FAILURE);
    }
  } else
    file = stdin;

  bool error = false;
  char **exclude_names = NULL;
  size_t exclude_count = 0;
  if(args.excludes) {
    exclude_count = get_excludes_count(args.excludes);
    exclude_names = malloc(exclude_count * sizeof *exclude_names);
    if(exclude_names) {
      error = parse_excludes(args.excludes, exclude_names, &exclude_count);
      if(error)
        free_excludes(exclude_names, exclude_count);
    } else
      error = true;
  }

  if(!error) {
    token_node *head = NULL;
    error = tokenize(file, &head);
    if(!error && head) {
      error = minify(&head);
      if(!error && !args.no_mangle)
        error = mangle(&head, exclude_names, exclude_count); 
      if(!error)
        print_tokens_as_text(head);
      free_token_nodes(head);
      free_excludes(exclude_names, exclude_count);
    }
  }

  if(file != stdin && file != NULL) {
    if(fclose(file) != 0) {
      fprintf(stderr, "Failed to close file: %s\n", strerror(errno));
      error = true;
    }
  }

  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
