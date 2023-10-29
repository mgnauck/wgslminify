CC=gcc
CFLAGS=-g -std=c17 -Wall -Wextra -pedantic
LDFLAGS=-g
LDLIBS=
SRCS=main.c tokenize.c minify.c buffer.c keywords.c
OBJS=$(subst .c,.o,$(SRCS))
OUT=wgslminify

$(OUT): $(OBJS)
	$(CC) $(LDFLAGS) -o $(OUT) $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(OUT)
