#include "codegen.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct {
    FILE *out;
    int   label_id;

    char **str_text;
    char **str_label;
    int    str_count;
    int    str_cap;

    const char *cur_epilogue;
    int   cur_param_count;
} Gen;

static void emit(Gen *g, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g->out, fmt, ap);
    va_end(ap);
    fputc('\n', g->out);
}

static int new_label(Gen *g) { return g->label_id++; }

static void cgen_expr(Gen *g, Expr *e);
static void cgen_stmt(Gen *g, Stmt *st);
static void cgen_block(Gen *g, Block *b, int allocate_locals);

static void add_string(Gen *g, Stmt *st) {
    if (g->str_count == g->str_cap) {
        g->str_cap = g->str_cap ? g->str_cap * 2 : 8;
        g->str_text  = realloc(g->str_text,  g->str_cap * sizeof(char*));
        g->str_label = realloc(g->str_label, g->str_cap * sizeof(char*));
        if (!g->str_text || !g->str_label) die_alloc();
    }
    char buf[32];
    snprintf(buf, sizeof buf, "str%d", g->str_count);
    st->as.wstr.label = xstrdup(buf);
    g->str_text[g->str_count]  = st->as.wstr.text;
    g->str_label[g->str_count] = st->as.wstr.label;
    g->str_count++;
}

static void collect_strings_stmt(Gen *g, Stmt *st) {
    for (; st; st = st->next) {
        switch (st->kind) {
        case ST_WRITE_STR: add_string(g, st); break;
        case ST_IF:
            collect_strings_stmt(g, st->as.if_s.then_branch);
            collect_strings_stmt(g, st->as.if_s.else_branch);
            break;
        case ST_WHILE:
            collect_strings_stmt(g, st->as.while_s.body);
            break;
        case ST_BLOCK:
            if (st->as.block) collect_strings_stmt(g, st->as.block->commands);
            break;
        default: break;
        }
    }
}

static void collect_strings_program(Gen *g, Program *prog) {
    for (Func *f = prog->functions; f; f = f->next)
        if (f->body) collect_strings_stmt(g, f->body->commands);
    if (prog->main_block) collect_strings_stmt(g, prog->main_block->commands);
}

static void emit_escaped_string(Gen *g, const char *s) {
    fputc('"', g->out);
    for (const char *p = s; *p; p++) {
        if (*p == '"' || *p == '\\') fputc('\\', g->out);
        fputc(*p, g->out);
    }
    fputc('"', g->out);
}

static void emit_scalar_load(Gen *g, VarRef *r, const char *reg) {
    if (r->category == CAT_GLOBAL)
        emit(g, "    lw %s, %d($s1)", reg, -4 * (r->position - 1));
    else if (r->category == CAT_PARAM)
        emit(g, "    lw %s, %d($fp)", reg, 4 * r->position);
    else
        emit(g, "    lw %s, %d($fp)", reg, -4 * r->position);
}

static void emit_scalar_store(Gen *g, VarRef *r, const char *reg) {
    if (r->category == CAT_GLOBAL)
        emit(g, "    sw %s, %d($s1)", reg, -4 * (r->position - 1));
    else if (r->category == CAT_PARAM)
        emit(g, "    sw %s, %d($fp)", reg, 4 * r->position);
    else
        emit(g, "    sw %s, %d($fp)", reg, -4 * r->position);
}

static void emit_array_elem_addr(Gen *g, VarRef *r, Expr *index) {
    cgen_expr(g, index);
    emit(g, "    sll $s0, $s0, 2");
    if (r->category == CAT_GLOBAL)
        emit(g, "    addiu $t0, $s1, %d", -4 * (r->position - 1));
    else if (r->category == CAT_LOCAL)
        emit(g, "    addiu $t0, $fp, %d", -4 * r->position);
    else
        emit(g, "    lw $t0, %d($fp)", 4 * r->position);
    emit(g, "    sub $t0, $t0, $s0");
}

static void cgen_argument(Gen *g, Expr *arg) {
    if (arg->kind == EX_VAR && arg->as.var.ref.is_array) {
        VarRef *r = &arg->as.var.ref;
        if (r->category == CAT_GLOBAL)
            emit(g, "    addiu $s0, $s1, %d", -4 * (r->position - 1));
        else if (r->category == CAT_LOCAL)
            emit(g, "    addiu $s0, $fp, %d", -4 * r->position);
        else
            emit(g, "    lw $s0, %d($fp)", 4 * r->position);
    } else {
        cgen_expr(g, arg);
    }

    emit(g, "    sw $s0, 0($sp)");
    emit(g, "    addiu $sp, $sp, -4");
}

static void cgen_args_reverse(Gen *g, Arg *a) {
    if (!a) return;
    cgen_args_reverse(g, a->next);
    cgen_argument(g, a->expr);
}

static void cgen_expr(Gen *g, Expr *e) {
    switch (e->kind) {

    case EX_INT:
        emit(g, "    li $s0, %d", e->as.int_value);
        break;

    case EX_CHAR:
        emit(g, "    li $s0, %d", e->as.char_value);
        break;

    case EX_VAR:

        emit_scalar_load(g, &e->as.var.ref, "$s0");
        break;

    case EX_ARRAY:
        emit_array_elem_addr(g, &e->as.arr.ref, e->as.arr.index);
        emit(g, "    lw $s0, 0($t0)");
        break;

    case EX_ASSIGN:
        if (e->as.assign.index == NULL) {

            cgen_expr(g, e->as.assign.value);
            emit_scalar_store(g, &e->as.assign.ref, "$s0");
        } else {

            cgen_expr(g, e->as.assign.value);
            emit(g, "    sw $s0, 0($sp)");
            emit(g, "    addiu $sp, $sp, -4");
            emit_array_elem_addr(g, &e->as.assign.ref, e->as.assign.index);
            emit(g, "    lw $s0, 4($sp)");
            emit(g, "    addiu $sp, $sp, 4");
            emit(g, "    sw $s0, 0($t0)");
        }
        break;

    case EX_CALL: {

        emit(g, "    sw $fp, 0($sp)");
        emit(g, "    addiu $sp, $sp, -4");
        cgen_args_reverse(g, e->as.call.args);
        emit(g, "    jal %s", e->as.call.name);
        break;
    }

    case EX_UNARY:
        cgen_expr(g, e->as.un.operand);
        if (e->as.un.op == OP_NEG)
            emit(g, "    sub $s0, $zero, $s0");
        else
            emit(g, "    seq $s0, $s0, $zero");
        break;

    case EX_BINARY: {

        cgen_expr(g, e->as.bin.left);
        emit(g, "    sw $s0, 0($sp)");
        emit(g, "    addiu $sp, $sp, -4");
        cgen_expr(g, e->as.bin.right);
        emit(g, "    lw $t1, 4($sp)");
        emit(g, "    addiu $sp, $sp, 4");
        switch (e->as.bin.op) {
        case OP_ADD: emit(g, "    add $s0, $t1, $s0"); break;
        case OP_SUB: emit(g, "    sub $s0, $t1, $s0"); break;
        case OP_MUL: emit(g, "    mul $s0, $t1, $s0"); break;
        case OP_DIV: emit(g, "    div $s0, $t1, $s0"); break;
        case OP_LT:  emit(g, "    slt $s0, $t1, $s0"); break;
        case OP_GT:  emit(g, "    sgt $s0, $t1, $s0"); break;
        case OP_LE:  emit(g, "    sle $s0, $t1, $s0"); break;
        case OP_GE:  emit(g, "    sge $s0, $t1, $s0"); break;
        case OP_EQ:  emit(g, "    seq $s0, $t1, $s0"); break;
        case OP_NE:  emit(g, "    sne $s0, $t1, $s0"); break;
        case OP_AND:
            emit(g, "    sne $t1, $t1, $zero");
            emit(g, "    sne $s0, $s0, $zero");
            emit(g, "    and $s0, $t1, $s0");
            break;
        case OP_OR:
            emit(g, "    sne $t1, $t1, $zero");
            emit(g, "    sne $s0, $s0, $zero");
            emit(g, "    or $s0, $t1, $s0");
            break;
        default: break;
        }
        break;
    }
    }
}

static void cgen_stmt(Gen *g, Stmt *st) {
    for (; st; st = st->next) {
        switch (st->kind) {

        case ST_EMPTY:
            break;

        case ST_EXPR:
            cgen_expr(g, st->as.expr);
            break;

        case ST_RETURN:
            if (st->as.expr) cgen_expr(g, st->as.expr);
            emit(g, "    b %s", g->cur_epilogue);
            break;

        case ST_READ: {
            int is_car = (st->as.read.ref.type == TYPE_CAR);
            emit(g, "    li $v0, %d", is_car ? 12 : 5);
            emit(g, "    syscall");
            if (st->as.read.index == NULL) {
                emit_scalar_store(g, &st->as.read.ref, "$v0");
            } else {
                emit_array_elem_addr(g, &st->as.read.ref, st->as.read.index);
                emit(g, "    sw $v0, 0($t0)");
            }
            break;
        }

        case ST_WRITE_EXPR:
            cgen_expr(g, st->as.expr);
            emit(g, "    move $a0, $s0");
            if (st->as.expr->inferred_type == TYPE_CAR)
                emit(g, "    li $v0, 11");
            else
                emit(g, "    li $v0, 1");
            emit(g, "    syscall");
            break;

        case ST_WRITE_STR:
            emit(g, "    la $a0, %s", st->as.wstr.label);
            emit(g, "    li $v0, 4");
            emit(g, "    syscall");
            break;

        case ST_NEWLINE:
            emit(g, "    li $a0, 10");
            emit(g, "    li $v0, 11");
            emit(g, "    syscall");
            break;

        case ST_IF: {
            int id = new_label(g);
            cgen_expr(g, st->as.if_s.cond);
            if (st->as.if_s.else_branch) {
                emit(g, "    beq $s0, $zero, else_%d", id);
                cgen_stmt(g, st->as.if_s.then_branch);
                emit(g, "    b endif_%d", id);
                emit(g, "else_%d:", id);
                cgen_stmt(g, st->as.if_s.else_branch);
                emit(g, "endif_%d:", id);
            } else {
                emit(g, "    beq $s0, $zero, endif_%d", id);
                cgen_stmt(g, st->as.if_s.then_branch);
                emit(g, "endif_%d:", id);
            }
            break;
        }

        case ST_WHILE: {
            int id = new_label(g);
            emit(g, "while_%d:", id);
            cgen_expr(g, st->as.while_s.cond);
            emit(g, "    beq $s0, $zero, endwhile_%d", id);
            cgen_stmt(g, st->as.while_s.body);
            emit(g, "    b while_%d", id);
            emit(g, "endwhile_%d:", id);
            break;
        }

        case ST_BLOCK:
            cgen_block(g, st->as.block, 1);
            break;
        }
    }
}

static int decls_size(Decl *d) {
    int total = 0;
    for (; d; d = d->next) total += d->is_array ? d->array_size : 1;
    return total;
}

static void cgen_block(Gen *g, Block *b, int allocate_locals) {
    if (!b) return;
    int k = decls_size(b->decls);
    if (allocate_locals && k > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * k);
    cgen_stmt(g, b->commands);
    if (allocate_locals && k > 0)
        emit(g, "    addiu $sp, $sp, %d", 4 * k);
}

static void cgen_function(Gen *g, Func *f) {
    char epilogue[128];
    snprintf(epilogue, sizeof epilogue, "%s__epi", f->name);
    g->cur_epilogue = epilogue;
    g->cur_param_count = f->param_count;

    int m = decls_size(f->body ? f->body->decls : NULL);

    emit(g, "%s:", f->name);

    emit(g, "    move $fp, $sp");
    emit(g, "    sw $ra, 0($sp)");
    emit(g, "    addiu $sp, $sp, -4");
    if (m > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * m);

    cgen_block(g, f->body, 0);

    emit(g, "%s:", epilogue);
    emit(g, "    lw $ra, 0($fp)");
    emit(g, "    move $sp, $fp");
    emit(g, "    addiu $sp, $sp, %d", 4 * (f->param_count + 1));
    emit(g, "    lw $fp, 0($sp)");
    emit(g, "    jr $ra");
    emit(g, "");
}

static void cgen_principal(Gen *g, Program *prog) {
    int gsize = decls_size(prog->globals);
    int lp    = decls_size(prog->main_block ? prog->main_block->decls : NULL);

    emit(g, "main:");

    emit(g, "    move $s1, $sp");
    if (gsize > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * gsize);

    emit(g, "    move $fp, $sp");
    emit(g, "    addiu $sp, $sp, -4");
    if (lp > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * lp);

    g->cur_epilogue = "main__fim";
    g->cur_param_count = 0;

    cgen_block(g, prog->main_block, 0);

    emit(g, "main__fim:");
    emit(g, "    li $v0, 10");
    emit(g, "    syscall");
    emit(g, "");
}

void generate_code(Program *prog, FILE *out) {
    Gen g;
    memset(&g, 0, sizeof g);
    g.out = out;

    collect_strings_program(&g, prog);

    emit(&g, "    .data");
    for (int i = 0; i < g.str_count; i++) {
        fprintf(out, "%s: .asciiz ", g.str_label[i]);
        emit_escaped_string(&g, g.str_text[i]);
        fputc('\n', out);
    }

    emit(&g, "    .text");
    emit(&g, "    .globl main");
    cgen_principal(&g, prog);
    for (Func *f = prog->functions; f; f = f->next)
        cgen_function(&g, f);

    free(g.str_text);
    free(g.str_label);
}
