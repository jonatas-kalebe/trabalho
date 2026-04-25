#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

// Gera código Assembly MIPS a partir da AST no arquivo de saída
void generate_code(Program *prog, FILE *out);

#endif
