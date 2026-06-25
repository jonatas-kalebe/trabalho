CC      = gcc
CFLAGS  = -Wall -g -I./src -std=c99 -D_POSIX_C_SOURCE=200809L
BISON   = bison
FLEX    = flex
LIBS    =

SRCS = src/parser.tab.c src/lex.yy.c src/ast.c src/symbol.c \
       src/semantic.c src/codegen.c src/main.c
OBJS = $(SRCS:.c=.o)

TARGET = compilador

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

src/parser.tab.c src/parser.tab.h: src/parser.y
	$(BISON) -d -o src/parser.tab.c src/parser.y

src/lex.yy.c: src/lexer.l src/parser.tab.h
	$(FLEX) -o src/lex.yy.c src/lexer.l

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/lex.yy.c src/parser.tab.c src/parser.tab.h $(TARGET)

.PHONY: all clean
