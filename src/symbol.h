/* ============================================================================
 *  symbol.h  --  Tabela de Simbolos com escopos aninhados.
 *
 *  CONCEITO (muito cobrado pelo professor):
 *  ----------------------------------------
 *  A "tabela de simbolos" e a estrutura onde o compilador guarda, para cada
 *  nome declarado (variavel/parametro), TUDO o que precisa saber sobre ele:
 *  tipo, se e vetor, onde fica na memoria (categoria + posicao) e em que
 *  ESCOPO foi declarado.
 *
 *  ESCOPO e a regra de visibilidade: uma variavel declarada num bloco interno
 *  "esconde" (shadowing) uma de mesmo nome num bloco externo. Modelamos os
 *  escopos como uma PILHA: ao entrar num bloco empilhamos um escopo; ao sair,
 *  desempilhamos. A busca por um nome vai do topo (escopo mais interno) para
 *  baixo (mais externo), o que implementa exatamente a regra de shadowing.
 *
 *  Niveis de escopo nesta linguagem:
 *      0  -> variaveis GLOBAIS
 *      1  -> parametros + variaveis do bloco mais externo de uma funcao
 *      2,3.. -> blocos aninhados dentro da funcao
 * ==========================================================================*/
#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"

/* Um simbolo = uma variavel/parametro declarado. */
typedef struct Symbol {
    char *name;
    Type  type;
    int   is_array;
    int   array_size;
    int   category;     /* CAT_GLOBAL / CAT_PARAM / CAT_LOCAL                 */
    int   position;     /* posicao (global/local) OU indice do parametro     */
    int   scope_level;  /* nivel do escopo onde foi declarado                */
    struct Symbol *next;/* proximo simbolo do MESMO escopo                   */
} Symbol;

/* Um escopo = uma lista de simbolos + ligacao para o escopo "pai" (externo). */
typedef struct Scope {
    Symbol *symbols;
    int     level;
    struct Scope *parent;
} Scope;

/* A tabela inteira: basta guardar o escopo do topo (os demais sao alcancados
 * via ponteiro 'parent'). */
typedef struct {
    Scope *top;
} SymTab;

void    symtab_init       (SymTab *st);                 /* cria o escopo 0    */
void    symtab_enter_scope(SymTab *st, int level);      /* empilha escopo     */
void    symtab_exit_scope (SymTab *st);                 /* desempilha escopo  */

/* Insere no escopo do topo. Retorna o Symbol criado, ou NULL se ja existir
 * um simbolo com o mesmo nome NESSE escopo (erro de redeclaracao). */
Symbol *symtab_insert(SymTab *st, char *name, Type type, int is_array,
                      int array_size, int category, int position);

/* Busca em todos os escopos, do topo para baixo (respeita shadowing). */
Symbol *symtab_lookup(SymTab *st, const char *name);

/* Busca apenas no escopo do topo (usada para detectar redeclaracao). */
Symbol *symtab_lookup_current(SymTab *st, const char *name);

#endif /* SYMBOL_H */
