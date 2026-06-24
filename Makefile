# ============================================================================
#  Makefile  --  compilador Cafezinho (Linux / macOS, com flex + bison + gcc)
#
#  No Windows, use o script  build.bat  (que usa win_flex/win_bison + cl.exe).
#
#  Alvos:
#    make            -> gera o executavel ./compilador
#    make clean      -> remove arquivos gerados
#    make testar     -> compila e roda os exemplos de teste
# ============================================================================
CC      = gcc
CFLAGS  = -Wall -g -I./src -std=c99 -D_POSIX_C_SOURCE=200809L
BISON   = bison
FLEX    = flex

# O nosso lexer usa "%option noyywrap", entao nao precisamos linkar -lfl.
LIBS    =

SRCS = src/parser.tab.c src/lex.yy.c src/ast.c src/symbol.c \
       src/semantic.c src/codegen.c src/main.c
OBJS = $(SRCS:.c=.o)

TARGET = compilador

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Bison gera o parser e o cabecalho com os nomes dos tokens.
src/parser.tab.c src/parser.tab.h: src/parser.y
	$(BISON) -d -o src/parser.tab.c src/parser.y

# Flex gera o scanner (depende do cabecalho do Bison).
src/lex.yy.c: src/lexer.l src/parser.tab.h
	$(FLEX) -o src/lex.yy.c src/lexer.l

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/lex.yy.c src/parser.tab.c src/parser.tab.h $(TARGET)

.PHONY: all clean
