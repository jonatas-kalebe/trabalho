#include "semantic.h"
#include "symbol.h"
#include <stdio.h>

typedef struct {
    Type type;
    int  is_array;
    int  is_error;
} ExprType;

typedef struct {
    SymTab   st;
    Program *prog;
    int      error_count;
    int      local_count;
    int      global_count;
    Func    *current_func;
} Sem;

static void sem_error(Sem *s, int line, const char *msg) {
    s->error_count++;
    printf("ERRO SEMANTICO (linha %d): %s\n", line, msg);
}

static Func *find_func(Program *prog, const char *name) {
    for (Func *f = prog->functions; f; f = f->next)
        if (strcmp(f->name, name) == 0)
            return f;
    return NULL;
}

static ExprType make_type(Type t, int is_array) {
    ExprType r; r.type = t; r.is_array = is_array; r.is_error = 0; return r;
}
static ExprType error_type(void) {
    ExprType r; r.type = TYPE_INT; r.is_array = 0; r.is_error = 1; return r;
}

static ExprType analyze_expr(Sem *s, Expr *e);
static void     analyze_stmt(Sem *s, Stmt *st);
static void     analyze_decls(Sem *s, Decl *list, int category);
static void     analyze_block_body(Sem *s, Block *b);
static void     analyze_nested_block(Sem *s, Block *b);

static void analyze_decls(Sem *s, Decl *list, int category) {

    for (Decl *d = list; d; d = d->next) {
        int size = d->is_array ? d->array_size : 1;
        int pos;
        if (category == CAT_GLOBAL) {
            pos = s->global_count + 1;
            s->global_count += size;
        } else {
            pos = s->local_count + 1;
            s->local_count += size;
        }
        d->category = category;
        d->position = pos;

        Symbol *sym = symtab_insert(&s->st, d->name, d->type, d->is_array,
                                    d->array_size, category, pos);
        if (sym == NULL) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "variavel '%s' ja declarada neste escopo", d->name);
            sem_error(s, d->line, buf);
        } else if (category == CAT_LOCAL && s->current_func) {

            for (Param *p = s->current_func->params; p; p = p->next)
                if (strcmp(p->name, d->name) == 0) {
                    char buf[128];
                    snprintf(buf, sizeof buf,
                        "variavel '%s' tem o mesmo nome de um parametro da funcao",
                        d->name);
                    sem_error(s, d->line, buf);
                    break;
                }
        }
    }
}

static ExprType analyze_expr(Sem *s, Expr *e) {
    if (!e) return error_type();
    switch (e->kind) {

    case EX_INT:
        e->inferred_type = TYPE_INT; e->is_array_result = 0;
        return make_type(TYPE_INT, 0);

    case EX_CHAR:
        e->inferred_type = TYPE_CAR; e->is_array_result = 0;
        return make_type(TYPE_CAR, 0);

    case EX_VAR: {
        Symbol *sym = symtab_lookup(&s->st, e->as.var.name);
        if (!sym) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "variavel '%s' nao foi declarada", e->as.var.name);
            sem_error(s, e->line, buf);

            sym = symtab_insert(&s->st, e->as.var.name, TYPE_INT, 0, 0,
                                CAT_LOCAL, 0);
            if (!sym) return error_type();
        }

        e->as.var.ref.category = sym->category;
        e->as.var.ref.position = sym->position;
        e->as.var.ref.type     = sym->type;
        e->as.var.ref.is_array = sym->is_array;
        e->inferred_type = sym->type;
        e->is_array_result = sym->is_array;
        return make_type(sym->type, sym->is_array);
    }

    case EX_ARRAY: {
        Symbol *sym = symtab_lookup(&s->st, e->as.arr.name);
        if (!sym) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "variavel '%s' nao foi declarada", e->as.arr.name);
            sem_error(s, e->line, buf);
            return error_type();
        }
        if (!sym->is_array) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "'%s' nao e um vetor, mas esta sendo indexada com []",
                     e->as.arr.name);
            sem_error(s, e->line, buf);
        }

        ExprType it = analyze_expr(s, e->as.arr.index);
        if (!it.is_error && it.is_array)
            sem_error(s, e->line, "o indice de um vetor deve ser escalar");

        e->as.arr.ref.category = sym->category;
        e->as.arr.ref.position = sym->position;
        e->as.arr.ref.type     = sym->type;
        e->as.arr.ref.is_array = sym->is_array;
        e->inferred_type = sym->type;
        e->is_array_result = 0;
        return make_type(sym->type, 0);
    }

    case EX_CALL: {
        Func *f = find_func(s->prog, e->as.call.name);
        if (!f) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "funcao '%s' nao foi declarada", e->as.call.name);
            sem_error(s, e->line, buf);

            for (Arg *a = e->as.call.args; a; a = a->next)
                analyze_expr(s, a->expr);
            return error_type();
        }

        if (e->as.call.argc != f->param_count) {
            char buf[160];
            snprintf(buf, sizeof buf,
                "chamada de '%s' com %d argumento(s), mas a funcao espera %d",
                e->as.call.name, e->as.call.argc, f->param_count);
            sem_error(s, e->line, buf);
        }

        Arg *a = e->as.call.args;
        Param *p = f->params;
        while (a && p) {
            ExprType at = analyze_expr(s, a->expr);
            if (!at.is_error) {
                if (p->is_array) {
                    if (!at.is_array) {
                        char buf[160];
                        snprintf(buf, sizeof buf,
                          "argumento '%s' espera um vetor, mas recebeu um escalar",
                          p->name);
                        sem_error(s, a->expr->line, buf);
                    } else if (at.type != p->type) {
                        char buf[160];
                        snprintf(buf, sizeof buf,
                          "argumento '%s': vetor de tipo incompativel com o esperado",
                          p->name);
                        sem_error(s, a->expr->line, buf);
                    }
                } else {
                    if (at.is_array) {
                        char buf[160];
                        snprintf(buf, sizeof buf,
                          "argumento '%s' espera um escalar, mas recebeu um vetor",
                          p->name);
                        sem_error(s, a->expr->line, buf);
                    }

                }
            }
            a = a->next; p = p->next;
        }

        for (; a; a = a->next) analyze_expr(s, a->expr);

        e->inferred_type = f->return_type;
        e->is_array_result = 0;
        return make_type(f->return_type, 0);
    }

    case EX_ASSIGN: {
        Symbol *sym = symtab_lookup(&s->st, e->as.assign.name);
        if (!sym) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "variavel '%s' nao foi declarada", e->as.assign.name);
            sem_error(s, e->line, buf);
            sym = symtab_insert(&s->st, e->as.assign.name, TYPE_INT, 0, 0,
                                CAT_LOCAL, 0);
            if (!sym) { analyze_expr(s, e->as.assign.value); return error_type(); }
        }
        Type target_type = sym->type;
        if (e->as.assign.index != NULL) {

            if (!sym->is_array)
                sem_error(s, e->line, "indexacao de uma variavel que nao e vetor");
            ExprType it = analyze_expr(s, e->as.assign.index);
            if (!it.is_error && it.is_array)
                sem_error(s, e->line, "o indice de um vetor deve ser escalar");
        } else {

            if (sym->is_array)
                sem_error(s, e->line,
                          "nao se pode atribuir a um vetor inteiro (use indice)");
        }

        ExprType vt = analyze_expr(s, e->as.assign.value);
        if (!vt.is_error && vt.is_array)
            sem_error(s, e->line, "nao se pode atribuir um vetor a um escalar");

        e->as.assign.ref.category = sym->category;
        e->as.assign.ref.position = sym->position;
        e->as.assign.ref.type     = sym->type;
        e->as.assign.ref.is_array = sym->is_array;
        e->inferred_type = target_type;
        e->is_array_result = 0;
        return make_type(target_type, 0);
    }

    case EX_BINARY: {
        ExprType l = analyze_expr(s, e->as.bin.left);
        ExprType r = analyze_expr(s, e->as.bin.right);

        if (!l.is_error && l.is_array)
            sem_error(s, e->line, "uso de vetor sem indice em expressao");
        if (!r.is_error && r.is_array)
            sem_error(s, e->line, "uso de vetor sem indice em expressao");

        e->inferred_type = TYPE_INT;
        e->is_array_result = 0;
        return make_type(TYPE_INT, 0);
    }

    case EX_UNARY: {
        ExprType o = analyze_expr(s, e->as.un.operand);
        if (!o.is_error && o.is_array)
            sem_error(s, e->line, "uso de vetor sem indice em expressao");
        e->inferred_type = TYPE_INT;
        e->is_array_result = 0;
        return make_type(TYPE_INT, 0);
    }
    }
    return error_type();
}

static void analyze_stmt(Sem *s, Stmt *st) {
    if (!st) return;
    switch (st->kind) {

    case ST_EMPTY:
    case ST_NEWLINE:
    case ST_WRITE_STR:
        break;

    case ST_EXPR:
        analyze_expr(s, st->as.expr);
        break;

    case ST_RETURN:
        if (st->as.expr) {
            ExprType t = analyze_expr(s, st->as.expr);
            if (!t.is_error && t.is_array)
                sem_error(s, st->line, "nao se pode retornar um vetor");
        }
        break;

    case ST_READ: {
        Symbol *sym = symtab_lookup(&s->st, st->as.read.name);
        if (!sym) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "variavel '%s' nao foi declarada", st->as.read.name);
            sem_error(s, st->line, buf);
            sym = symtab_insert(&s->st, st->as.read.name, TYPE_INT, 0, 0,
                                CAT_LOCAL, 0);
            if (!sym) break;
        }
        if (st->as.read.index != NULL) {
            if (!sym->is_array)
                sem_error(s, st->line, "indexacao de uma variavel que nao e vetor");
            analyze_expr(s, st->as.read.index);
        } else if (sym->is_array) {
            sem_error(s, st->line, "leia exige um escalar (vetor precisa de indice)");
        }
        st->as.read.ref.category = sym->category;
        st->as.read.ref.position = sym->position;
        st->as.read.ref.type     = sym->type;
        st->as.read.ref.is_array = sym->is_array;
        break;
    }

    case ST_WRITE_EXPR: {
        ExprType t = analyze_expr(s, st->as.expr);
        if (!t.is_error && t.is_array)
            sem_error(s, st->line, "nao se pode escrever um vetor inteiro");
        break;
    }

    case ST_IF:
        analyze_expr(s, st->as.if_s.cond);
        analyze_stmt(s, st->as.if_s.then_branch);
        analyze_stmt(s, st->as.if_s.else_branch);
        break;

    case ST_WHILE:
        analyze_expr(s, st->as.while_s.cond);
        analyze_stmt(s, st->as.while_s.body);
        break;

    case ST_BLOCK:
        analyze_nested_block(s, st->as.block);
        break;
    }

    if (st->next) analyze_stmt(s, st->next);
}

static void analyze_block_body(Sem *s, Block *b) {
    if (!b) return;
    analyze_decls(s, b->decls, CAT_LOCAL);
    analyze_stmt(s, b->commands);
}

static void analyze_nested_block(Sem *s, Block *b) {
    if (!b) return;
    int saved = s->local_count;
    symtab_enter_scope(&s->st, s->st.top->level + 1);
    analyze_decls(s, b->decls, CAT_LOCAL);
    analyze_stmt(s, b->commands);
    symtab_exit_scope(&s->st);
    s->local_count = saved;
}

static void analyze_function(Sem *s, Func *f) {
    s->current_func = f;
    symtab_enter_scope(&s->st, 1);
    s->local_count = 0;

    int idx = 1;
    for (Param *p = f->params; p; p = p->next) {
        p->index = idx++;
        Symbol *sym = symtab_insert(&s->st, p->name, p->type, p->is_array,
                                    0, CAT_PARAM, p->index);
        if (sym == NULL) {
            char buf[128];
            snprintf(buf, sizeof buf,
                     "parametro '%s' declarado mais de uma vez", p->name);
            sem_error(s, p->line, buf);
        }
    }

    analyze_block_body(s, f->body);

    symtab_exit_scope(&s->st);
}

int check_semantics(Program *prog) {
    Sem s;
    s.prog = prog;
    s.error_count = 0;
    s.local_count = 0;
    s.global_count = 0;
    s.current_func = NULL;
    symtab_init(&s.st);

    analyze_decls(&s, prog->globals, CAT_GLOBAL);

    for (Func *f = prog->functions; f; f = f->next)
        for (Func *g = f->next; g; g = g->next)
            if (strcmp(f->name, g->name) == 0) {
                char buf[128];
                snprintf(buf, sizeof buf,
                         "funcao '%s' declarada mais de uma vez", g->name);
                sem_error(&s, g->line, buf);
            }

    for (Func *f = prog->functions; f; f = f->next)
        analyze_function(&s, f);

    s.current_func = NULL;
    symtab_enter_scope(&s.st, 1);
    s.local_count = 0;
    analyze_block_body(&s, prog->main_block);
    symtab_exit_scope(&s.st);

    return s.error_count;
}
