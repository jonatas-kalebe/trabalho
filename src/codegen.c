/* ============================================================================
 *  codegen.c  --  Geracao de codigo MIPS (maquina de pilha com 1 acumulador).
 *
 *  IDEIA CENTRAL (a pergunta classica do professor):
 *  -------------------------------------------------
 *  "Como gerar codigo para uma expressao sem usar muitos registradores?"
 *  Resposta: usamos uma MAQUINA DE PILHA. Cada expressao deixa seu resultado
 *  no ACUMULADOR ($s0) e PRESERVA a pilha (invariante). Para um operador
 *  binario op(e1, e2):
 *      1. calcula e1            -> resultado em $s0
 *      2. empilha $s0           (guarda e1 na pilha)
 *      3. calcula e2            -> resultado em $s0
 *      4. desempilha e1 em $t1
 *      5. aplica op: $s0 = $t1 op $s0
 *  Como os valores intermediarios vivem na PILHA (e nao em registradores),
 *  o esquema funciona para expressoes de qualquer profundidade e sobrevive
 *  ate a chamadas de funcao recursivas.
 *
 *  ORGANIZACAO DA MEMORIA (slides "Ambiente de Execucao"):
 *      - Globais: area apontada por $s1 (fixa durante toda a execucao).
 *      - Cada funcao ativa tem um REGISTRO DE ATIVACAO (frame) na pilha,
 *        apontado por $fp, contendo: $fp do chamador, argumentos, $ra e as
 *        variaveis locais. A pilha de frames simula a "arvore de ativacao".
 * ==========================================================================*/
#include "codegen.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ---------------------------------------------------------------------------
 *  Estado global da geracao de codigo.
 * -------------------------------------------------------------------------*/
typedef struct {
    FILE *out;
    int   label_id;          /* gera rotulos unicos (if/while)               */

    /* tabela de strings literais -> rotulos no segmento .data */
    char **str_text;
    char **str_label;
    int    str_count;
    int    str_cap;

    /* contexto da funcao sendo gerada */
    const char *cur_epilogue;/* rotulo do epilogo (destino do 'retorne')     */
    int   cur_param_count;   /* numero de parametros (para limpar a pilha)    */
} Gen;

/* impressao formatada no arquivo de saida (atalho) */
static void emit(Gen *g, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g->out, fmt, ap);
    va_end(ap);
    fputc('\n', g->out);
}

/* gera um numero de rotulo unico */
static int new_label(Gen *g) { return g->label_id++; }

/* protótipos (recursao mutua) */
static void cgen_expr(Gen *g, Expr *e);
static void cgen_stmt(Gen *g, Stmt *st);
static void cgen_block(Gen *g, Block *b, int allocate_locals);

/* ===========================================================================
 *  Coleta de strings literais (comando 'escreva "..."').
 *  Cada string vira um rotulo strN no segmento .data. Fazemos uma passada
 *  previa pela AST para descobrir todas e atribuir rotulos.
 * =========================================================================*/
static void add_string(Gen *g, Stmt *st) {
    if (g->str_count == g->str_cap) {
        g->str_cap = g->str_cap ? g->str_cap * 2 : 8;
        g->str_text  = realloc(g->str_text,  g->str_cap * sizeof(char*));
        g->str_label = realloc(g->str_label, g->str_cap * sizeof(char*));
        if (!g->str_text || !g->str_label) die_alloc();
    }
    char buf[32];
    snprintf(buf, sizeof buf, "str%d", g->str_count);
    st->as.wstr.label = xstrdup(buf);   /* grava o rotulo no proprio no */
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

/* imprime o texto da string ja escapado para .asciiz */
static void emit_escaped_string(Gen *g, const char *s) {
    fputc('"', g->out);
    for (const char *p = s; *p; p++) {
        if (*p == '"' || *p == '\\') fputc('\\', g->out);
        fputc(*p, g->out);
    }
    fputc('"', g->out);
}

/* ===========================================================================
 *  Acesso a variaveis -- traduz (categoria, posicao) em enderecos MIPS.
 *
 *    GLOBAL pos p : endereco = $s1 - 4*(p-1)
 *    PARAM  idx i : endereco = $fp + 4*i     (escalar) | guarda o ENDERECO
 *                                              base se for vetor (passado por
 *                                              referencia)
 *    LOCAL  pos p : endereco = $fp - 4*p
 * =========================================================================*/

/* carrega o valor de uma variavel ESCALAR no registrador reg */
static void emit_scalar_load(Gen *g, VarRef *r, const char *reg) {
    if (r->category == CAT_GLOBAL)
        emit(g, "    lw %s, %d($s1)", reg, -4 * (r->position - 1));
    else if (r->category == CAT_PARAM)
        emit(g, "    lw %s, %d($fp)", reg, 4 * r->position);
    else /* CAT_LOCAL */
        emit(g, "    lw %s, %d($fp)", reg, -4 * r->position);
}

/* armazena o valor de reg em uma variavel ESCALAR */
static void emit_scalar_store(Gen *g, VarRef *r, const char *reg) {
    if (r->category == CAT_GLOBAL)
        emit(g, "    sw %s, %d($s1)", reg, -4 * (r->position - 1));
    else if (r->category == CAT_PARAM)
        emit(g, "    sw %s, %d($fp)", reg, 4 * r->position);
    else /* CAT_LOCAL */
        emit(g, "    sw %s, %d($fp)", reg, -4 * r->position);
}

/* Calcula no registrador $t0 o ENDERECO do elemento vetor[indice].
 * Estrategia: endereco = base0 - 4*indice, onde base0 e o endereco do
 * elemento de indice 0. (A pilha cresce para baixo, entao indices maiores
 * ficam em enderecos menores -- por isso a subtracao.) */
static void emit_array_elem_addr(Gen *g, VarRef *r, Expr *index) {
    cgen_expr(g, index);              /* $s0 = indice                        */
    emit(g, "    sll $s0, $s0, 2");   /* $s0 = 4*indice                      */
    if (r->category == CAT_GLOBAL)
        emit(g, "    addiu $t0, $s1, %d", -4 * (r->position - 1)); /* base0  */
    else if (r->category == CAT_LOCAL)
        emit(g, "    addiu $t0, $fp, %d", -4 * r->position);       /* base0  */
    else /* CAT_PARAM: o parametro guarda o endereco base do vetor */
        emit(g, "    lw $t0, %d($fp)", 4 * r->position);
    emit(g, "    sub $t0, $t0, $s0"); /* $t0 = endereco do elemento          */
}

/* Empilha um argumento de chamada de funcao. Se for um vetor (passagem por
 * referencia), empilha o ENDERECO base; caso contrario, empilha o VALOR. */
static void cgen_argument(Gen *g, Expr *arg) {
    if (arg->kind == EX_VAR && arg->as.var.ref.is_array) {
        VarRef *r = &arg->as.var.ref;
        if (r->category == CAT_GLOBAL)
            emit(g, "    addiu $s0, $s1, %d", -4 * (r->position - 1));
        else if (r->category == CAT_LOCAL)
            emit(g, "    addiu $s0, $fp, %d", -4 * r->position);
        else /* PARAM: repassa o endereco base recebido */
            emit(g, "    lw $s0, %d($fp)", 4 * r->position);
    } else {
        cgen_expr(g, arg);
    }
    /* empilha $s0 */
    emit(g, "    sw $s0, 0($sp)");
    emit(g, "    addiu $sp, $sp, -4");
}

/* empilha os argumentos em ORDEM INVERSA (arg_n primeiro, arg_1 por ultimo).
 * Recursao: empilha a cauda da lista antes do elemento atual. */
static void cgen_args_reverse(Gen *g, Arg *a) {
    if (!a) return;
    cgen_args_reverse(g, a->next);
    cgen_argument(g, a->expr);
}

/* ===========================================================================
 *  Geracao de codigo para EXPRESSOES. Resultado sempre em $s0; pilha preservada.
 * =========================================================================*/
static void cgen_expr(Gen *g, Expr *e) {
    switch (e->kind) {

    case EX_INT:
        emit(g, "    li $s0, %d", e->as.int_value);
        break;

    case EX_CHAR:
        emit(g, "    li $s0, %d", e->as.char_value);
        break;

    case EX_VAR:
        /* na geracao de codigo so chegam aqui usos escalares (a semantica ja
         * recusou usar o nome de um vetor sem indice em expressao). */
        emit_scalar_load(g, &e->as.var.ref, "$s0");
        break;

    case EX_ARRAY:
        emit_array_elem_addr(g, &e->as.arr.ref, e->as.arr.index);
        emit(g, "    lw $s0, 0($t0)");
        break;

    case EX_ASSIGN:
        if (e->as.assign.index == NULL) {
            /* atribuicao a escalar: calcula valor e guarda */
            cgen_expr(g, e->as.assign.value);
            emit_scalar_store(g, &e->as.assign.ref, "$s0");
        } else {
            /* atribuicao a elemento de vetor: empilha valor, calcula endereco,
             * desempilha valor e grava. */
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
        /* SEQUENCIA DE CHAMADA (lado do chamador):
         *   1. empilha o $fp do chamador
         *   2. empilha os argumentos em ordem inversa
         *   3. jal <funcao>
         * O retorno (lado do chamado) restaura $sp e $fp; resultado em $s0. */
        emit(g, "    sw $fp, 0($sp)");          /* empilha $fp do chamador    */
        emit(g, "    addiu $sp, $sp, -4");
        cgen_args_reverse(g, e->as.call.args);  /* empilha argumentos         */
        emit(g, "    jal %s", e->as.call.name); /* desvia para a funcao       */
        break;
    }

    case EX_UNARY:
        cgen_expr(g, e->as.un.operand);
        if (e->as.un.op == OP_NEG)
            emit(g, "    sub $s0, $zero, $s0");     /* negacao aritmetica     */
        else /* OP_NOT */
            emit(g, "    seq $s0, $s0, $zero");     /* 1 se for 0, senao 0    */
        break;

    case EX_BINARY: {
        /* esquema da maquina de pilha (vide topo do arquivo) */
        cgen_expr(g, e->as.bin.left);
        emit(g, "    sw $s0, 0($sp)");          /* empilha e1                 */
        emit(g, "    addiu $sp, $sp, -4");
        cgen_expr(g, e->as.bin.right);
        emit(g, "    lw $t1, 4($sp)");          /* desempilha e1 em $t1       */
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
        case OP_AND: /* normaliza para 0/1 e faz E bit a bit */
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

/* ===========================================================================
 *  Geracao de codigo para COMANDOS.
 * =========================================================================*/
static void cgen_stmt(Gen *g, Stmt *st) {
    for (; st; st = st->next) {
        switch (st->kind) {

        case ST_EMPTY:
            break;

        case ST_EXPR:
            cgen_expr(g, st->as.expr);   /* resultado descartado */
            break;

        case ST_RETURN:
            if (st->as.expr) cgen_expr(g, st->as.expr); /* valor em $s0 */
            emit(g, "    b %s", g->cur_epilogue);        /* vai ao epilogo */
            break;

        case ST_READ: {
            int is_car = (st->as.read.ref.type == TYPE_CAR);
            emit(g, "    li $v0, %d", is_car ? 12 : 5);  /* read_char/read_int*/
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
                emit(g, "    li $v0, 11");   /* print_char */
            else
                emit(g, "    li $v0, 1");    /* print_int  */
            emit(g, "    syscall");
            break;

        case ST_WRITE_STR:
            emit(g, "    la $a0, %s", st->as.wstr.label);
            emit(g, "    li $v0, 4");         /* print_string */
            emit(g, "    syscall");
            break;

        case ST_NEWLINE:
            emit(g, "    li $a0, 10");        /* codigo ASCII de '\n' */
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
            cgen_block(g, st->as.block, 1);  /* bloco aninhado aloca locais */
            break;
        }
    }
}

/* soma o numero de "slots" (palavras de 4 bytes) ocupados pelas declaracoes */
static int decls_size(Decl *d) {
    int total = 0;
    for (; d; d = d->next) total += d->is_array ? d->array_size : 1;
    return total;
}

/* Gera codigo de um bloco. Se 'allocate_locals' for verdadeiro (bloco
 * aninhado), reserva espaco na pilha para as variaveis locais na entrada e
 * libera na saida. Para o bloco externo de uma funcao o prologo ja reservou,
 * entao passamos 0. */
static void cgen_block(Gen *g, Block *b, int allocate_locals) {
    if (!b) return;
    int k = decls_size(b->decls);
    if (allocate_locals && k > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * k);   /* reserva locais */
    cgen_stmt(g, b->commands);
    if (allocate_locals && k > 0)
        emit(g, "    addiu $sp, $sp, %d", 4 * k);     /* libera locais  */
}

/* ===========================================================================
 *  Geracao de uma FUNCAO completa (prologo, corpo, epilogo).
 *
 *  Registro de Ativacao (de cima/enderecos maiores para baixo):
 *      $fp do chamador
 *      argumento n ... argumento 1
 *      $ra                <- $fp aponta aqui
 *      local 1 ... local m
 *                         <- $sp
 * =========================================================================*/
static void cgen_function(Gen *g, Func *f) {
    char epilogue[128];
    snprintf(epilogue, sizeof epilogue, "%s__epi", f->name);
    g->cur_epilogue = epilogue;
    g->cur_param_count = f->param_count;

    int m = decls_size(f->body ? f->body->decls : NULL); /* locais externos */

    emit(g, "%s:", f->name);
    /* --- PROLOGO (sequencia de chamada do lado do chamado) --- */
    emit(g, "    move $fp, $sp");          /* $fp marca o inicio do frame    */
    emit(g, "    sw $ra, 0($sp)");         /* salva o endereco de retorno    */
    emit(g, "    addiu $sp, $sp, -4");
    if (m > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * m);  /* aloca locais externos */

    /* --- CORPO --- (locais externos ja alocados: passamos 0) */
    cgen_block(g, f->body, 0);

    /* --- EPILOGO (retorno de funcao) --- */
    emit(g, "%s:", epilogue);
    emit(g, "    lw $ra, 0($fp)");                  /* restaura $ra           */
    emit(g, "    move $sp, $fp");                   /* libera locais          */
    emit(g, "    addiu $sp, $sp, %d", 4 * (f->param_count + 1)); /* tira args+$fp slot */
    emit(g, "    lw $fp, 0($sp)");                  /* restaura $fp do chamador*/
    emit(g, "    jr $ra");                          /* volta ao chamador      */
    emit(g, "");
}

/* ===========================================================================
 *  Geracao do programa PRINCIPAL (ponto de entrada 'main').
 * =========================================================================*/
static void cgen_principal(Gen *g, Program *prog) {
    int gsize = decls_size(prog->globals);
    int lp    = decls_size(prog->main_block ? prog->main_block->decls : NULL);

    emit(g, "main:");
    /* base das globais: $s1 = $sp atual; depois reservamos o espaco delas */
    emit(g, "    move $s1, $sp");
    if (gsize > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * gsize);
    /* frame do principal: tratamos como funcao sem parametros e sem $ra.
     * Reservamos 1 slot ($fp+0, no lugar do $ra) para manter a MESMA formula
     * de acesso a locais das funcoes ($fp - 4*pos). */
    emit(g, "    move $fp, $sp");
    emit(g, "    addiu $sp, $sp, -4");
    if (lp > 0)
        emit(g, "    addiu $sp, $sp, %d", -4 * lp);

    g->cur_epilogue = "main__fim";
    g->cur_param_count = 0;

    cgen_block(g, prog->main_block, 0);  /* locais ja alocados acima */

    emit(g, "main__fim:");
    emit(g, "    li $v0, 10");           /* syscall 10 = exit */
    emit(g, "    syscall");
    emit(g, "");
}

/* ===========================================================================
 *  Ponto de entrada do back-end.
 * =========================================================================*/
void generate_code(Program *prog, FILE *out) {
    Gen g;
    memset(&g, 0, sizeof g);
    g.out = out;

    /* 1) descobre todas as strings literais e atribui rotulos */
    collect_strings_program(&g, prog);

    /* 2) segmento de dados (.data): as strings do programa */
    emit(&g, "    .data");
    for (int i = 0; i < g.str_count; i++) {
        fprintf(out, "%s: .asciiz ", g.str_label[i]);
        emit_escaped_string(&g, g.str_text[i]);
        fputc('\n', out);
    }

    /* 3) segmento de codigo (.text) */
    emit(&g, "    .text");
    emit(&g, "    .globl main");
    cgen_principal(&g, prog);            /* 'main' executa o principal */
    for (Func *f = prog->functions; f; f = f->next)
        cgen_function(&g, f);            /* depois, o codigo das funcoes */

    free(g.str_text);
    free(g.str_label);
}
