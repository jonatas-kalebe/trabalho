#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/*
 * O que a estrutura faz: Mapeia uma variável no contexto do gerador de código.
 * Papel no Pipeline: Gerador de Código MIPS (Manutenção de Memória Local).
 * Regra da G-V1: Cálculo de offsets no MIPS baseados no frame pointer ($fp).

 */
typedef struct CGSym {
    char *name;
    Type type;
    int offset;
    int size;
    struct CGSym *next;
} CGSym;

/*
 * O que a estrutura faz: Representa uma literal de string (para comandos de escreva).
 * Papel no Pipeline: Gerador de Código MIPS (Seção .data).
 * Regra da G-V1: Suporte a print de literais.

 */
typedef struct CGString {
    char *text;
    char *label;
    struct CGString *next;
} CGString;

/*
 * O que a estrutura faz: Representa o ambiente léxico momentâneo para alocação no MIPS.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: O controle de escopo deve respeitar variáveis locais sobrepondo globais ao bloco (shadowing).

 */
typedef struct CGScope {
    CGSym *symbols;
    int alloc_size;
    struct CGScope *prev;
} CGScope;

/*
 * O que a estrutura faz: Carrega o estado global (labels, contadores, out file) do assembly.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Contexto de arquitetura para a fase de emissão.

 */
typedef struct {
    FILE *out;
    CGString *strings;
    int string_count;
    int label_count;
    CGScope *top;
    int depth;
} CodegenCtx;

/*
 * O que o método faz: Retorna 4 para INT e 1 para CAR.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Suporte aos tamanhos exatos das variáveis na pilha ($sp).

 */
static int cg_type_size(Type t) {
    return (t == TYPE_INT) ? 4 : 1;
}

/*
 * O que o método faz: Busca as meta-informações de MIPS de uma variável já validada.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Resolução de endereços locais por shadowing.

 */
static CGSym *cg_lookup(CodegenCtx *cg, const char *name) {
    for (CGScope *s = cg->top; s; s = s->prev) {
        for (CGSym *it = s->symbols; it; it = it->next) {
            if (strcmp(it->name, name) == 0) return it;
        }
    }
    return NULL;
}

/*
 * O que o método faz: Fornece nomes de labels únicos e sequenciais (ex: while_start_3).
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Implementação de branches para 'se' e 'enquanto'.

 */
static char *new_label(CodegenCtx *cg, const char *prefix) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s_%d", prefix, cg->label_count++);
    return xstrdup(buf);
}

/*
 * O que o método faz: Adiciona escapes corretos para a seção .asciiz do MIPS.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Formatação de strings no assembly.

 */
static char *mips_escape_string(const char *s) {
    size_t n = 0;
    for (const char *p = s; *p; p++) {
        if (*p == '\\' || *p == '"') n += 2;
        else if (*p == '\n' || *p == '\t' || *p == '\r') n += 2;
        else n += 1;
    }
    char *out = (char *)malloc(n + 1);
    if (!out) die_alloc();
    char *w = out;
    for (const char *p = s; *p; p++) {
        if (*p == '\\') {
            *w++ = '\\';
            *w++ = '\\';
        } else if (*p == '"') {
            *w++ = '\\';
            *w++ = '"';
        } else if (*p == '\n') {
            *w++ = '\\';
            *w++ = 'n';
        } else if (*p == '\t') {
            *w++ = '\\';
            *w++ = 't';
        } else if (*p == '\r') {
            *w++ = '\\';
            *w++ = 'r';
        } else {
            *w++ = *p;
        }
    }
    *w = '\0';
    return out;
}

/*
 * O que o método faz: Associa uma label (.data) para uma string, reaproveitando se já existir.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Otimização básica de tabelas de constantes.

 */
static char *intern_string(CodegenCtx *cg, const char *text) {
    for (CGString *it = cg->strings; it; it = it->next) {
        if (strcmp(it->text, text) == 0) return it->label;
    }
    CGString *s = (CGString *)calloc(1, sizeof(CGString));
    if (!s) die_alloc();
    s->text = xstrdup(text);
    s->label = new_label(cg, "str");
    s->next = cg->strings;
    cg->strings = s;
    cg->string_count++;
    return s->label;
}

static void collect_strings_stmt(CodegenCtx *cg, Stmt *s);

/*
 * O que o método faz: Visita a AST previamente procurando literais de string em blocos.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Preparo da seção .data no MIPS.

 */
static void collect_strings_block(CodegenCtx *cg, Block *b) {
    if (!b) return;
    for (Stmt *s = b->commands; s; s = s->next) collect_strings_stmt(cg, s);
}

/*
 * O que o método faz: Recurso auxiliar recursivo para achar strings dentro de comandos aninhados.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Varredura de strings na AST.

 */
static void collect_strings_stmt(CodegenCtx *cg, Stmt *s) {
    if (!s) return;
    if (s->kind == ST_WRITE_STR) {
        s->as.write_str.label = intern_string(cg, s->as.write_str.text);
    } else if (s->kind == ST_IF) {
        collect_strings_stmt(cg, s->as.if_stmt.then_branch);
        if (s->as.if_stmt.else_branch) collect_strings_stmt(cg, s->as.if_stmt.else_branch);
    } else if (s->kind == ST_WHILE) {
        collect_strings_stmt(cg, s->as.while_stmt.body);
    } else if (s->kind == ST_BLOCK) {
        collect_strings_block(cg, s->as.block);
    }
}

static void emit_expr(CodegenCtx *cg, Expr *e);
static void emit_stmt(CodegenCtx *cg, Stmt *s);

/*
 * O que o método faz: Cria um frame de variáveis locais empurrando o $sp.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Cálculo de offsets no MIPS para variáveis e alocação dinâmica.

 */
static void cg_push_scope(CodegenCtx *cg, Block *b) {
    CGScope *sc = (CGScope *)calloc(1, sizeof(CGScope));
    if (!sc) die_alloc();
    sc->prev = cg->top;
    cg->top = sc;

    int local = 0;
    for (Decl *d = b->decls; d; d = d->next) {
        local += cg_type_size(d->type);
        CGSym *sym = (CGSym *)calloc(1, sizeof(CGSym));
        if (!sym) die_alloc();
        sym->name = d->name;
        sym->type = d->type;
        sym->size = cg_type_size(d->type);
        sym->offset = -(cg->depth + local);
        sym->next = sc->symbols;
        sc->symbols = sym;
    }

    sc->alloc_size = local;
    if (local > 0) {
        fprintf(cg->out, "  addiu $sp, $sp, -%d\n", local);
        cg->depth += local;
    }
}

/*
 * O que o método faz: Recua o $sp destruindo o escopo na máquina alvo.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Garbage collection de stack frame e shadowing.

 */
static void cg_pop_scope(CodegenCtx *cg) {
    CGScope *sc = cg->top;
    if (!sc) return;
    if (sc->alloc_size > 0) {
        fprintf(cg->out, "  addiu $sp, $sp, %d\n", sc->alloc_size);
        cg->depth -= sc->alloc_size;
    }
    cg->top = sc->prev;
    while (sc->symbols) {
        CGSym *n = sc->symbols->next;
        free(sc->symbols);
        sc->symbols = n;
    }
    free(sc);
}

/*
 * O que o método faz: Emite o bloco de código controlando os pushes e pops ao redor.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Emissão de bloco sequencial.

 */
static void emit_block(CodegenCtx *cg, Block *b) {
    if (!b) return;
    cg_push_scope(cg, b);
    for (Stmt *s = b->commands; s; s = s->next) emit_stmt(cg, s);
    cg_pop_scope(cg);
}

/*
 * O que o método faz: Traduz expressões aritméticas para comandos MIPS, guardando tudo no topo da pilha temporária para operações de 2 endereços.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: A geração de código é para arquitetura MIPS baseada em pilha ($sp).

 */
static void emit_expr(CodegenCtx *cg, Expr *e) {
    if (!e) return;
    switch (e->kind) {
        case EX_INT:
            fprintf(cg->out, "  li $v0, %d\n", e->as.int_value);
            return;
        case EX_CHAR:
            fprintf(cg->out, "  li $v0, %d\n", e->as.char_value);
            return;
        case EX_VAR: {
            CGSym *sym = cg_lookup(cg, e->as.name);
            if (sym->type == TYPE_INT) fprintf(cg->out, "  lw $v0, %d($fp)\n", sym->offset);
            /* 'car' usa lbu para evitar extensão de sinal indevida. */
            else fprintf(cg->out, "  lbu $v0, %d($fp)\n", sym->offset);
            return;
        }
        case EX_ASSIGN: {
            CGSym *sym = cg_lookup(cg, e->as.assign.name);
            emit_expr(cg, e->as.assign.value);
            if (sym->type == TYPE_INT) fprintf(cg->out, "  sw $v0, %d($fp)\n", sym->offset);
            else fprintf(cg->out, "  sb $v0, %d($fp)\n", sym->offset);
            return;
        }
        case EX_UNARY:
            emit_expr(cg, e->as.un.expr);
            if (e->as.un.op == OP_NEG) fprintf(cg->out, "  subu $v0, $zero, $v0\n");
            else fprintf(cg->out, "  seq $v0, $v0, $zero\n");
            return;
        case EX_BINARY:
            emit_expr(cg, e->as.bin.left);
            fprintf(cg->out, "  addiu $sp, $sp, -4\n");
            fprintf(cg->out, "  sw $v0, 0($sp)\n");
            emit_expr(cg, e->as.bin.right);
            fprintf(cg->out, "  move $t1, $v0\n");
            fprintf(cg->out, "  lw $t0, 0($sp)\n");
            fprintf(cg->out, "  addiu $sp, $sp, 4\n");
            switch (e->as.bin.op) {
                case OP_ADD: fprintf(cg->out, "  addu $v0, $t0, $t1\n"); break;
                case OP_SUB: fprintf(cg->out, "  subu $v0, $t0, $t1\n"); break;
                case OP_MUL: fprintf(cg->out, "  mul $v0, $t0, $t1\n"); break;
                case OP_DIV:
                    fprintf(cg->out, "  div $t0, $t1\n");
                    fprintf(cg->out, "  mflo $v0\n");
                    break;
                case OP_LT: fprintf(cg->out, "  slt $v0, $t0, $t1\n"); break;
                case OP_GT: fprintf(cg->out, "  slt $v0, $t1, $t0\n"); break;
                case OP_GE: fprintf(cg->out, "  slt $v0, $t0, $t1\n  xori $v0, $v0, 1\n"); break;
                case OP_LE: fprintf(cg->out, "  slt $v0, $t1, $t0\n  xori $v0, $v0, 1\n"); break;
                case OP_EQ: fprintf(cg->out, "  seq $v0, $t0, $t1\n"); break;
                case OP_NE: fprintf(cg->out, "  sne $v0, $t0, $t1\n"); break;
                case OP_AND:
                    fprintf(cg->out, "  sne $t0, $t0, $zero\n");
                    fprintf(cg->out, "  sne $t1, $t1, $zero\n");
                    fprintf(cg->out, "  and $v0, $t0, $t1\n");
                    break;
                case OP_OR:
                    fprintf(cg->out, "  sne $t0, $t0, $zero\n");
                    fprintf(cg->out, "  sne $t1, $t1, $zero\n");
                    fprintf(cg->out, "  or $v0, $t0, $t1\n");
                    break;
                default:
                    break;
            }
            return;
    }
}

/*
 * O que o método faz: Emite a lógica imperativa do MIPS (branches, IO Syscalls).
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Transformar while e if em jumps (beq e j).

 */
static void emit_stmt(CodegenCtx *cg, Stmt *s) {
    if (!s) return;
    switch (s->kind) {
        case ST_EMPTY:
            return;
        case ST_EXPR:
            emit_expr(cg, s->as.expr);
            return;
        case ST_READ: {
            CGSym *sym = cg_lookup(cg, s->as.name);
            if (sym->type == TYPE_INT) {
                fprintf(cg->out, "  li $v0, 5\n  syscall\n");
                fprintf(cg->out, "  sw $v0, %d($fp)\n", sym->offset);
            } else {
                fprintf(cg->out, "  li $v0, 12\n  syscall\n");
                fprintf(cg->out, "  sb $v0, %d($fp)\n", sym->offset);
            }
            return;
        }
        case ST_WRITE_EXPR:
            emit_expr(cg, s->as.expr);
            if (s->as.expr->inferred_type == TYPE_INT) {
                fprintf(cg->out, "  move $a0, $v0\n  li $v0, 1\n  syscall\n");
            } else {
                fprintf(cg->out, "  move $a0, $v0\n  li $v0, 11\n  syscall\n");
            }
            return;
        case ST_WRITE_STR:
            fprintf(cg->out, "  la $a0, %s\n  li $v0, 4\n  syscall\n", s->as.write_str.label);
            return;
        case ST_NEWLINE:
            fprintf(cg->out, "  li $a0, 10\n  li $v0, 11\n  syscall\n");
            return;
        case ST_IF: {
            char *l_else = new_label(cg, "else");
            char *l_end = new_label(cg, "endif");
            emit_expr(cg, s->as.if_stmt.cond);
            if (s->as.if_stmt.else_branch) {
                fprintf(cg->out, "  beq $v0, $zero, %s\n", l_else);
                emit_stmt(cg, s->as.if_stmt.then_branch);
                fprintf(cg->out, "  j %s\n", l_end);
                fprintf(cg->out, "%s:\n", l_else);
                emit_stmt(cg, s->as.if_stmt.else_branch);
                fprintf(cg->out, "%s:\n", l_end);
            } else {
                fprintf(cg->out, "  beq $v0, $zero, %s\n", l_end);
                emit_stmt(cg, s->as.if_stmt.then_branch);
                fprintf(cg->out, "%s:\n", l_end);
            }
            free(l_else);
            free(l_end);
            return;
        }
        case ST_WHILE: {
            char *l_start = new_label(cg, "while_start");
            char *l_end = new_label(cg, "while_end");
            fprintf(cg->out, "%s:\n", l_start);
            emit_expr(cg, s->as.while_stmt.cond);
            fprintf(cg->out, "  beq $v0, $zero, %s\n", l_end);
            emit_stmt(cg, s->as.while_stmt.body);
            fprintf(cg->out, "  j %s\n", l_start);
            fprintf(cg->out, "%s:\n", l_end);
            free(l_start);
            free(l_end);
            return;
        }
        case ST_BLOCK:
            emit_block(cg, s->as.block);
            return;
    }
}

/*
 * O que o método faz: Setup inicial do MIPS (bootstrapping da .data e main) gerindo o fp original.
 * Papel no Pipeline: Gerador de Código MIPS.
 * Regra da G-V1: Ponto de entrada do programa e emissão do assembler final.

 */
void generate_code(Program *prog, FILE *out) {
    CodegenCtx cg;
    memset(&cg, 0, sizeof(cg));
    cg.out = out;

    collect_strings_block(&cg, prog->block);

    fprintf(out, ".data\n");
    for (CGString *it = cg.strings; it; it = it->next) {
        char *esc = mips_escape_string(it->text);
        fprintf(out, "%s: .asciiz \"%s\"\n", it->label, esc);
        free(esc);
    }

    fprintf(out, ".text\n");
    fprintf(out, ".globl main\n");
    fprintf(out, "main:\n");
    fprintf(out, "  move $fp, $sp\n");

    emit_block(&cg, prog->block);

    fprintf(out, "  li $v0, 10\n");
    fprintf(out, "  syscall\n");
}
