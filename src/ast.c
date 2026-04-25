#include <stdio.h>
#include "ast.h"

void die_alloc(void) {
    fprintf(stderr, "ERRO: FALHA DE MEMORIA\n");
    exit(1);
}

char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *r = (char *)malloc(n + 1);
    if (!r) die_alloc();
    memcpy(r, s, n + 1);
    return r;
}

static Expr *alloc_expr(ExprKind k, int line) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    if (!e) die_alloc();
    e->kind = k;
    e->line = line;
    e->inferred_type = TYPE_INT;
    return e;
}

Expr *new_expr_var(char *name, int line) {
    Expr *e = alloc_expr(EX_VAR, line);
    e->as.name = name;
    return e;
}

Expr *new_expr_int(int value, int line) {
    Expr *e = alloc_expr(EX_INT, line);
    e->as.int_value = value;
    return e;
}

Expr *new_expr_char(int value, int line) {
    Expr *e = alloc_expr(EX_CHAR, line);
    e->as.char_value = value;
    return e;
}

Expr *new_expr_assign(char *name, Expr *value, int line) {
    Expr *e = alloc_expr(EX_ASSIGN, line);
    e->as.assign.name = name;
    e->as.assign.value = value;
    return e;
}

Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line) {
    Expr *e = alloc_expr(EX_BINARY, line);
    e->as.bin.op = op;
    e->as.bin.left = left;
    e->as.bin.right = right;
    return e;
}

Expr *new_expr_unary(OpKind op, Expr *expr, int line) {
    Expr *e = alloc_expr(EX_UNARY, line);
    e->as.un.op = op;
    e->as.un.expr = expr;
    return e;
}

Decl *new_decl(char *name, Type type, int line) {
    Decl *d = (Decl *)calloc(1, sizeof(Decl));
    if (!d) die_alloc();
    d->name = name;
    d->type = type;
    d->line = line;
    d->next = NULL;
    return d;
}

static Stmt *alloc_stmt(StmtKind kind, int line) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    if (!s) die_alloc();
    s->kind = kind;
    s->line = line;
    s->next = NULL;
    return s;
}

Stmt *new_stmt_empty(int line) {
    return alloc_stmt(ST_EMPTY, line);
}

Stmt *new_stmt_expr(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_EXPR, line);
    s->as.expr = expr;
    return s;
}

Stmt *new_stmt_read(char *name, int line) {
    Stmt *s = alloc_stmt(ST_READ, line);
    s->as.name = name;
    return s;
}

Stmt *new_stmt_write_expr(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_EXPR, line);
    s->as.expr = expr;
    return s;
}

Stmt *new_stmt_write_str(char *text, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_STR, line);
    s->as.write_str.text = text;
    s->as.write_str.label = NULL;
    return s;
}

Stmt *new_stmt_newline(int line) {
    return alloc_stmt(ST_NEWLINE, line);
}

Stmt *new_stmt_if(Expr *cond, Stmt *then_branch, Stmt *else_branch, int line) {
    Stmt *s = alloc_stmt(ST_IF, line);
    s->as.if_stmt.cond = cond;
    s->as.if_stmt.then_branch = then_branch;
    s->as.if_stmt.else_branch = else_branch;
    return s;
}

Stmt *new_stmt_while(Expr *cond, Stmt *body, int line) {
    Stmt *s = alloc_stmt(ST_WHILE, line);
    s->as.while_stmt.cond = cond;
    s->as.while_stmt.body = body;
    return s;
}

Stmt *new_stmt_block(Block *block, int line) {
    Stmt *s = alloc_stmt(ST_BLOCK, line);
    s->as.block = block;
    return s;
}

Block *new_block(Decl *decls, Stmt *commands, int line) {
    Block *b = (Block *)calloc(1, sizeof(Block));
    if (!b) die_alloc();
    b->decls = decls;
    b->commands = commands;
    b->line = line;
    return b;
}
