CCFLAGS=-Wall -Wextra -pedantic -std=c11
LDFLAGS=-g
SRC=main.c tokenize.c minify.c buffer.c keywords.c
OBJ=$(patsubst %.c,obj/%.o,$(SRC))

.PHONY: clean

wgslminify: $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

obj/%.o: src/%.c
	@mkdir -p `dirname $@`
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf obj wgslminify
