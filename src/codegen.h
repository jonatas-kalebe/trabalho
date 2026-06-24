/* ============================================================================
 *  codegen.h  --  Interface da Geracao de Codigo (back-end).
 *
 *  Esta fase percorre a AST (ja validada e decorada pela semantica) e emite
 *  codigo de montagem MIPS que simula uma MAQUINA DE PILHA, exatamente como
 *  descrito nos slides "Geracao de Codigo" e "Ambiente de Execucao".
 *
 *  Convencoes de registradores (combinadas com o material da disciplina):
 *      $s0  -> ACUMULADOR. Toda expressao deixa seu resultado aqui.
 *      $sp  -> topo da PILHA (cresce para enderecos menores).
 *      $fp  -> frame pointer: base do registro de ativacao da funcao atual.
 *      $s1  -> base da area de variaveis GLOBAIS.
 *      $ra  -> endereco de retorno (preenchido por 'jal').
 *      $t0,$t1 -> registradores temporarios para calculos intermediarios.
 *      $v0,$a0 -> usados nas chamadas de sistema (syscall) de E/S.
 * ==========================================================================*/
#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

/* Gera o codigo MIPS do programa inteiro, escrevendo em 'out'. */
void generate_code(Program *prog, FILE *out);

#endif /* CODEGEN_H */
