/* ============================================================================
 *  semantic.h  --  Interface da Analise Semantica.
 *
 *  A analise semantica e a 3a fase do front-end (depois de lexica e sintatica).
 *  Ela verifica regras que a gramatica NAO consegue expressar, por exemplo:
 *    - variavel usada sem ter sido declarada;
 *    - variavel declarada duas vezes no mesmo escopo;
 *    - chamada de funcao com numero/tipo de argumentos errado;
 *    - uso de um vetor onde se espera um escalar (e vice-versa).
 *
 *  Tambem DECORA a AST: grava em cada uso de variavel a sua categoria e
 *  posicao de memoria (calculadas via tabela de simbolos), informacao que a
 *  geracao de codigo vai usar.
 * ==========================================================================*/
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* Analisa o programa inteiro. Retorna o numero de erros semanticos
 * encontrados (0 = programa semanticamente correto). */
int check_semantics(Program *prog);

#endif /* SEMANTIC_H */
