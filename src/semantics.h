#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast.h"

/*
 * O que o método faz: Ponto de entrada do módulo semântico que percorre toda a AST validando tipos e escopo.
 * Papel no Pipeline: Semântico -> (Valida e prepara a árvore antes da Geração de Código MIPS).
 * Regra da G-V1: Validação de tipos (int, car) e verificação de variáveis em escopo antes do código rodar.

 */
void check_semantics(Program *prog);

#endif
