CC=cc
CFLAGS=-W -Wall -Wextra -pedantic -std=c2x
LDFLAGS=-g
LDLIBS=
SRC=main.c tokenize.c minify.c buffer.c keywords.c
OBJ=$(patsubst %.c,obj/%.o,$(SRC))

.PHONY: wgslminify

all: wgslminify

obj/%.o: src/%.c
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

wgslminify: $(OBJ)
	$(CC) $^ $(CFLAGS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJ) wgslminify
	rmdir obj
