/* ============================================================================
 *  ast.c  --  Implementacao dos construtores da AST.
 *
 *  Cada funcao aqui apenas: (1) aloca um no, (2) preenche os campos, (3)
 *  devolve o ponteiro. Sao "fabricas" de nos. Manter isso separado deixa o
 *  parser.y limpo: nas acoes do Bison escrevemos so 'new_expr_binary(...)'.
 * ==========================================================================*/
#include "ast.h"
#include <stdio.h>

/* Aborta o programa quando malloc falha. Em um compilador real trataria
 * melhor, mas para fins didaticos basta avisar e sair. */
void die_alloc(void) {
    fprintf(stderr, "ERRO INTERNO: memoria insuficiente\n");
    exit(1);
}

/* strdup nao faz parte do C ANSI estrito; implementamos a nossa para
 * copiar com seguranca os nomes que vem do buffer do Flex (yytext). */
char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) die_alloc();
    memcpy(p, s, n);
    return p;
}

/* Helper interno: aloca um Expr ja com kind e linha preenchidos. */
static Expr *alloc_expr(ExprKind kind, int line) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    if (!e) die_alloc();
    e->kind = kind;
    e->line = line;
    e->inferred_type = TYPE_INT;   /* default; a semantica corrige           */
    e->is_array_result = 0;
    return e;
}

/* ---- Expressoes -----------------------------------------------------------*/

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

Expr *new_expr_var(char *name, int line) {
    Expr *e = alloc_expr(EX_VAR, line);
    e->as.var.name = name;
    e->as.var.ref.category = CAT_NONE;
    return e;
}

Expr *new_expr_array(char *name, Expr *index, int line) {
    Expr *e = alloc_expr(EX_ARRAY, line);
    e->as.arr.name = name;
    e->as.arr.index = index;
    e->as.arr.ref.category = CAT_NONE;
    return e;
}

Expr *new_expr_call(char *name, Arg *args, int line) {
    Expr *e = alloc_expr(EX_CALL, line);
    e->as.call.name = name;
    e->as.call.args = args;
    /* conta os argumentos uma unica vez aqui */
    int c = 0;
    for (Arg *a = args; a; a = a->next) c++;
    e->as.call.argc = c;
    return e;
}

Expr *new_expr_assign(char *name, Expr *index, Expr *value, int line) {
    Expr *e = alloc_expr(EX_ASSIGN, line);
    e->as.assign.name  = name;
    e->as.assign.index = index;   /* NULL = escalar; != NULL = elemento vetor */
    e->as.assign.value = value;
    e->as.assign.ref.category = CAT_NONE;
    return e;
}

Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line) {
    Expr *e = alloc_expr(EX_BINARY, line);
    e->as.bin.op = op;
    e->as.bin.left = left;
    e->as.bin.right = right;
    return e;
}

Expr *new_expr_unary(OpKind op, Expr *operand, int line) {
    Expr *e = alloc_expr(EX_UNARY, line);
    e->as.un.op = op;
    e->as.un.operand = operand;
    return e;
}

Arg *new_arg(Expr *expr) {
    Arg *a = (Arg *)calloc(1, sizeof(Arg));
    if (!a) die_alloc();
    a->expr = expr;
    a->next = NULL;
    return a;
}

/* ---- Declaracoes, parametros, blocos e funcoes ---------------------------*/

Decl *new_decl(char *name, Type type, int is_array, int array_size, int line) {
    Decl *d = (Decl *)calloc(1, sizeof(Decl));
    if (!d) die_alloc();
    d->name = name;
    d->type = type;
    d->is_array = is_array;
    d->array_size = array_size;
    d->line = line;
    d->category = CAT_NONE;
    d->next = NULL;
    return d;
}

Param *new_param(char *name, Type type, int is_array, int line) {
    Param *p = (Param *)calloc(1, sizeof(Param));
    if (!p) die_alloc();
    p->name = name;
    p->type = type;
    p->is_array = is_array;
    p->line = line;
    p->next = NULL;
    return p;
}

Block *new_block(Decl *decls, Stmt *commands, int line) {
    Block *b = (Block *)calloc(1, sizeof(Block));
    if (!b) die_alloc();
    b->decls = decls;
    b->commands = commands;
    b->line = line;
    return b;
}

Func *new_func(char *name, Param *params, Type return_type, Block *body, int line) {
    Func *f = (Func *)calloc(1, sizeof(Func));
    if (!f) die_alloc();
    f->name = name;
    f->params = params;
    f->return_type = return_type;
    f->body = body;
    f->line = line;
    f->next = NULL;
    /* conta os parametros */
    int c = 0;
    for (Param *p = params; p; p = p->next) c++;
    f->param_count = c;
    return f;
}

/* ---- Comandos -------------------------------------------------------------*/

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

Stmt *new_stmt_return(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_RETURN, line);
    s->as.expr = expr;
    return s;
}

Stmt *new_stmt_read(char *name, Expr *index, int line) {
    Stmt *s = alloc_stmt(ST_READ, line);
    s->as.read.name = name;
    s->as.read.index = index;
    s->as.read.ref.category = CAT_NONE;
    return s;
}

Stmt *new_stmt_write_expr(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_EXPR, line);
    s->as.expr = expr;
    return s;
}

Stmt *new_stmt_write_str(char *text, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_STR, line);
    s->as.wstr.text = text;
    s->as.wstr.label = NULL;   /* rotulo .data atribuido na geracao de codigo */
    return s;
}

Stmt *new_stmt_newline(int line) {
    return alloc_stmt(ST_NEWLINE, line);
}

Stmt *new_stmt_if(Expr *cond, Stmt *then_b, Stmt *else_b, int line) {
    Stmt *s = alloc_stmt(ST_IF, line);
    s->as.if_s.cond = cond;
    s->as.if_s.then_branch = then_b;
    s->as.if_s.else_branch = else_b;
    return s;
}

Stmt *new_stmt_while(Expr *cond, Stmt *body, int line) {
    Stmt *s = alloc_stmt(ST_WHILE, line);
    s->as.while_s.cond = cond;
    s->as.while_s.body = body;
    return s;
}

Stmt *new_stmt_block(Block *block, int line) {
    Stmt *s = alloc_stmt(ST_BLOCK, line);
    s->as.block = block;
    return s;
}
