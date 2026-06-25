#include "symbol.h"
#include <stdio.h>

void symtab_init(SymTab *st) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) die_alloc();
    s->level = 0;
    s->symbols = NULL;
    s->parent = NULL;
    st->top = s;
}

void symtab_enter_scope(SymTab *st, int level) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) die_alloc();
    s->level = level;
    s->symbols = NULL;
    s->parent = st->top;
    st->top = s;
}

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

Symbol *symtab_lookup_current(SymTab *st, const char *name) {
    for (Symbol *s = st->top->symbols; s; s = s->next)
        if (strcmp(s->name, name) == 0)
            return s;
    return NULL;
}

Symbol *symtab_lookup(SymTab *st, const char *name) {
    for (Scope *sc = st->top; sc; sc = sc->parent)
        for (Symbol *s = sc->symbols; s; s = s->next)
            if (strcmp(s->name, name) == 0)
                return s;
    return NULL;
}

Symbol *symtab_insert(SymTab *st, char *name, Type type, int is_array,
                      int array_size, int category, int position) {
    if (symtab_lookup_current(st, name) != NULL)
        return NULL;

    Symbol *s = (Symbol *)calloc(1, sizeof(Symbol));
    if (!s) die_alloc();
    s->name = name;
    s->type = type;
    s->is_array = is_array;
    s->array_size = array_size;
    s->category = category;
    s->position = position;
    s->scope_level = st->top->level;

    s->next = st->top->symbols;
    st->top->symbols = s;
    return s;
}
