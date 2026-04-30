#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

/*
 * O que o método faz: Ponto de entrada do Gerador de Código. Traduz a AST em Assembly MIPS.
 * Papel no Pipeline: Gerador de Código MIPS. (Última fase do compilador).
 * Regra da G-V1: A geração de código é para arquitetura MIPS baseada em pilha ($sp e $fp).
 * Dica para a Banca: "Esta função coordena a coleta de strings na .data e depois aciona a travessia post-order na .text, entregando o código executável pronto pro SPIM."
 */
void generate_code(Program *prog, FILE *out);

#endif
