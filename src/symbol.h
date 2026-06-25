#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"

typedef struct Symbol {
    char *name;
    Type  type;
    int   is_array;
    int   array_size;
    int   category;
    int   position;
    int   scope_level;
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol *symbols;
    int     level;
    struct Scope *parent;
} Scope;

typedef struct {
    Scope *top;
} SymTab;

void    symtab_init       (SymTab *st);
void    symtab_enter_scope(SymTab *st, int level);
void    symtab_exit_scope (SymTab *st);

Symbol *symtab_insert(SymTab *st, char *name, Type type, int is_array,
                      int array_size, int category, int position);

Symbol *symtab_lookup(SymTab *st, const char *name);

Symbol *symtab_lookup_current(SymTab *st, const char *name);

#endif
