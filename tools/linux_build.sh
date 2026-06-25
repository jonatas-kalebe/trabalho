#!/usr/bin/env bash
# ===========================================================================
#  linux_build.sh -- compila o compilador Cafezinho no Linux/WSL.
#
#  Nao usa o 'make' porque o Makefile do projeto esta salvo em CRLF (Windows);
#  chama flex/bison/gcc diretamente. NAO modifica nenhum arquivo-fonte.
#
#  Uso:  bash tools/linux_build.sh
# ===========================================================================
set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "[1/3] bison  -> src/parser.tab.c / parser.tab.h"
bison -d -o src/parser.tab.c src/parser.y

echo "[2/3] flex   -> src/lex.yy.c"
flex -o src/lex.yy.c src/lexer.l

echo "[3/3] gcc    -> compilador"
gcc -Wall -g -I./src -std=c99 -D_POSIX_C_SOURCE=200809L \
    src/parser.tab.c src/lex.yy.c src/ast.c src/symbol.c \
    src/semantic.c src/codegen.c src/main.c \
    -o compilador

echo "OK -> $ROOT/compilador"
