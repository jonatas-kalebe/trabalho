/* ============================================================================
 *  symbol.c  --  Implementacao da tabela de simbolos (pilha de escopos).
 * ==========================================================================*/
#include "symbol.h"
#include <stdio.h>

/* Cria a tabela com um unico escopo (nivel 0), que guardara as globais. */
void symtab_init(SymTab *st) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) die_alloc();
    s->level = 0;
    s->symbols = NULL;
    s->parent = NULL;
    st->top = s;
}

/* Empilha um novo escopo (entrar num bloco). */
void symtab_enter_scope(SymTab *st, int level) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) die_alloc();
    s->level = level;
    s->symbols = NULL;
    s->parent = st->top;   /* aponta para o escopo externo                   */
    st->top = s;
}

/* Desempilha o escopo do topo (sair de um bloco). Liberamos a lista de
 * simbolos daquele escopo, mas NAO os nomes (eles continuam referenciados
 * pela AST, que vive ate o fim da compilacao). */
void symtab_exit_scope(SymTab *st) {
    Scope *s = st->top;
    if (!s) return;
    st->top = s->parent;
    Symbol *sym = s->symbols;
    while (sym) {
        Symbol *tmp = sym;
        sym = sym->next;
        free(tmp);
    }
    free(s);
}

/* Procura um nome SO no escopo do topo. */
Symbol *symtab_lookup_current(SymTab *st, const char *name) {
    for (Symbol *s = st->top->symbols; s; s = s->next)
        if (strcmp(s->name, name) == 0)
            return s;
    return NULL;
}

/* Procura um nome em todos os escopos, do mais interno para o mais externo.
 * O primeiro que casar vence -> e exatamente a regra de shadowing. */
Symbol *symtab_lookup(SymTab *st, const char *name) {
    for (Scope *sc = st->top; sc; sc = sc->parent)
        for (Symbol *s = sc->symbols; s; s = s->next)
            if (strcmp(s->name, name) == 0)
                return s;
    return NULL;
}

/* Insere no escopo do topo. Devolve NULL se ja existir nesse mesmo escopo. */
Symbol *symtab_insert(SymTab *st, char *name, Type type, int is_array,
                      int array_size, int category, int position) {
    if (symtab_lookup_current(st, name) != NULL)
        return NULL;   /* redeclaracao no mesmo escopo                       */

    Symbol *s = (Symbol *)calloc(1, sizeof(Symbol));
    if (!s) die_alloc();
    s->name = name;
    s->type = type;
    s->is_array = is_array;
    s->array_size = array_size;
    s->category = category;
    s->position = position;
    s->scope_level = st->top->level;
    /* insere na cabeca da lista do escopo atual */
    s->next = st->top->symbols;
    st->top->symbols = s;
    return s;
}
