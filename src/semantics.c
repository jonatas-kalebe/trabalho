#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "ast.h"

typedef struct Sym {
    char *name;
    Type type;
    int offset;
    int size;
    struct Sym *next;
} Sym;

typedef struct Scope {
    Sym *symbols;
    struct Scope *prev;
} Scope;

typedef struct {
    Scope *top;
} SemanticCtx;

static int type_size(Type t) {
    return (t == TYPE_INT) ? 4 : 1;
}

static void semantic_error(int line, const char *msg) {
    printf("ERRO: %s %d\n", msg, line);
    exit(1);
}

static Scope *push_scope(Scope *top) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) die_alloc();
    s->prev = top;
    return s;
}

static Scope *pop_scope(Scope *top) {
    Scope *p = top->prev;
    while (top->symbols) {
        Sym *n = top->symbols->next;
        free(top->symbols);
        top->symbols = n;
    }
    free(top);
    return p;
}

static Sym *lookup_scope(Scope *s, const char *name) {
    for (Sym *it = s ? s->symbols : NULL; it; it = it->next) {
        if (strcmp(it->name, name) == 0) return it;
    }
    return NULL;
}

static Sym *lookup_all(Scope *top, const char *name) {
    for (Scope *s = top; s; s = s->prev) {
        Sym *f = lookup_scope(s, name);
        if (f) return f;
    }
    return NULL;
}

static void add_symbol(Scope *scope, char *name, Type type, int line) {
    if (lookup_scope(scope, name)) semantic_error(line, "IDENTIFICADOR JA DECLARADO");
    Sym *s = (Sym *)calloc(1, sizeof(Sym));
    if (!s) die_alloc();
    s->name = name;
    s->type = type;
    s->size = type_size(type);
    s->next = scope->symbols;
    scope->symbols = s;
}

static Type analyze_expr(SemanticCtx *ctx, Expr *e);
static void analyze_stmt(SemanticCtx *ctx, Stmt *s);

static void analyze_block(SemanticCtx *ctx, Block *b) {
    if (!b) return;
    ctx->top = push_scope(ctx->top);

    for (Decl *d = b->decls; d; d = d->next) {
        add_symbol(ctx->top, d->name, d->type, d->line);
    }

    for (Stmt *s = b->commands; s; s = s->next) {
        analyze_stmt(ctx, s);
    }

    ctx->top = pop_scope(ctx->top);
}

static Type analyze_expr(SemanticCtx *ctx, Expr *e) {
    if (!e) return TYPE_INT;
    switch (e->kind) {
        case EX_VAR: {
            Sym *sym = lookup_all(ctx->top, e->as.name);
            if (!sym) semantic_error(e->line, "IDENTIFICADOR NAO DECLARADO");
            e->inferred_type = sym->type;
            return e->inferred_type;
        }
        case EX_INT:
            e->inferred_type = TYPE_INT;
            return TYPE_INT;
        case EX_CHAR:
            e->inferred_type = TYPE_CAR;
            return TYPE_CAR;
        case EX_ASSIGN: {
            Sym *sym = lookup_all(ctx->top, e->as.assign.name);
            if (!sym) semantic_error(e->line, "IDENTIFICADOR NAO DECLARADO");
            Type rhs = analyze_expr(ctx, e->as.assign.value);
            if (rhs != sym->type) semantic_error(e->line, "ATRIBUICAO COM TIPOS DIFERENTES");
            e->inferred_type = sym->type;
            return e->inferred_type;
        }
        case EX_UNARY: {
            Type t = analyze_expr(ctx, e->as.un.expr);
            if (t != TYPE_INT) semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
            e->inferred_type = TYPE_INT;
            return TYPE_INT;
        }
        case EX_BINARY: {
            Type l = analyze_expr(ctx, e->as.bin.left);
            Type r = analyze_expr(ctx, e->as.bin.right);
            switch (e->as.bin.op) {
                case OP_ADD:
                case OP_SUB:
                case OP_MUL:
                case OP_DIV:
                    if (l != TYPE_INT || r != TYPE_INT) {
                        semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
                    }
                    e->inferred_type = TYPE_INT;
                    return TYPE_INT;
                case OP_OR:
                case OP_AND:
                    if (l != TYPE_INT || r != TYPE_INT) {
                        semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
                    }
                    e->inferred_type = TYPE_INT;
                    return TYPE_INT;
                case OP_EQ:
                case OP_NE:
                case OP_LT:
                case OP_GT:
                case OP_GE:
                case OP_LE:
                    if (l != r) semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
                    e->inferred_type = TYPE_INT;
                    return TYPE_INT;
                default:
                    break;
            }
        }
    }
    semantic_error(e->line, "ERRO SEMANTICO");
    return TYPE_INT;
}

static void analyze_stmt(SemanticCtx *ctx, Stmt *s) {
    if (!s) return;
    switch (s->kind) {
        case ST_EMPTY:
            break;
        case ST_EXPR:
            (void)analyze_expr(ctx, s->as.expr);
            break;
        case ST_READ: {
            Sym *sym = lookup_all(ctx->top, s->as.name);
            if (!sym) semantic_error(s->line, "IDENTIFICADOR NAO DECLARADO");
            break;
        }
        case ST_WRITE_EXPR:
            (void)analyze_expr(ctx, s->as.expr);
            break;
        case ST_WRITE_STR:
        case ST_NEWLINE:
            break;
        case ST_IF: {
            Type t = analyze_expr(ctx, s->as.if_stmt.cond);
            if (t != TYPE_INT) semantic_error(s->line, "CONDICAO DEVE SER INT");
            analyze_stmt(ctx, s->as.if_stmt.then_branch);
            if (s->as.if_stmt.else_branch) analyze_stmt(ctx, s->as.if_stmt.else_branch);
            break;
        }
        case ST_WHILE: {
            Type t = analyze_expr(ctx, s->as.while_stmt.cond);
            if (t != TYPE_INT) semantic_error(s->line, "CONDICAO DEVE SER INT");
            analyze_stmt(ctx, s->as.while_stmt.body);
            break;
        }
        case ST_BLOCK:
            analyze_block(ctx, s->as.block);
            break;
    }
}

void check_semantics(Program *prog) {
    SemanticCtx sem;
    sem.top = NULL;
    analyze_block(&sem, prog->block);
}
