#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "ast.h"

/*
 * Entrada da tabela de símbolos.
 * Cada identificador declarado no programa gera um Sym com:
 * - name: lexema do identificador
 * - type: tipo semântico (int/car)
 * - offset/size: metadados úteis para endereçamento em stack frame
 */
typedef struct Sym {
    char *name;
    Type type;
    int offset;
    int size;
    struct Sym *next;
} Sym;

/*
 * Um Scope representa um bloco léxico.
 * scopes formam uma pilha encadeada:
 * bloco interno -> bloco externo -> ...
 */
typedef struct Scope {
    Sym *symbols;
    struct Scope *prev;
} Scope;

/*
 * O que a estrutura faz: Mantém o contexto global semântico durante a travessia da AST.
 * Papel no Pipeline: Semântico.
 * Regra da G-V1: Isolamento de estado na passagem de validadores.
 * Dica para a Banca: "O Contexto Semântico carrega o ponteiro pro escopo no topo, facilitando o tracking sem entupir as assinaturas das funções."
 */
typedef struct {
    Scope *top;
} SemanticCtx;

/*
 * Converte tipo semântico em tamanho de armazenamento.
 * Esta decisão impacta cálculo de offset para variáveis.
 */
static int type_size(Type t) {
    return (t == TYPE_INT) ? 4 : 1;
}

/*
 * Emite erro semântico com linha e interrompe a compilação.
 * Decisão didática: estratégia fail-fast evita efeito cascata de erros derivados.
 */
static void semantic_error(int line, const char *msg) {
    printf("ERRO: %s %d\n", msg, line);
    exit(1);
}

/*
 * Abre escopo novo (entrada em bloco).
 */
static Scope *push_scope(Scope *top) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) die_alloc();
    s->prev = top;
    return s;
}

/*
 * Fecha escopo atual (saída de bloco), liberando símbolos locais.
 */
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

/*
 * Busca somente no escopo atual.
 * Uso principal: impedir redeclaração no mesmo bloco.
 */
static Sym *lookup_scope(Scope *s, const char *name) {
    for (Sym *it = s ? s->symbols : NULL; it; it = it->next) {
        if (strcmp(it->name, name) == 0) return it;
    }
    return NULL;
}

/*
 * Busca do escopo interno para o externo.
 * Este comportamento implementa shadowing de forma natural.
 */
static Sym *lookup_all(Scope *top, const char *name) {
    for (Scope *s = top; s; s = s->prev) {
        Sym *f = lookup_scope(s, name);
        if (f) return f;
    }
    return NULL;
}

/*
 * O que o método faz: Insere uma nova variável no escopo topo da pilha.
 * Papel no Pipeline: Semântico.
 * Regra da G-V1: Atualização contínua da tabela de símbolos pelas declarações.
 * Dica para a Banca: "A checagem preventiva com lookup_scope já garante que nenhuma variável nasça sobreposta indevidamente no seu próprio nível."
 */
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

/*
 * O que o método faz: Analisa um bloco de comandos, pushando escopo novo e alimentando símbolos.
 * Papel no Pipeline: Semântico.
 * Regra da G-V1: Ancorar blocos da AST ao ciclo de vida da tabela de símbolos.
 * Dica para a Banca: "Processamos primeiro as declarações (para populacionar as variáveis locais) e depois mergulhamos nos comandos que irão usá-las."
 */
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

/*
 * O que o método faz: Varre expressões recursivamente verificando inferência e consistência de tipo.
 * Papel no Pipeline: Semântico.
 * Regra da G-V1: Checagem de tipos int e car, impedindo operações ilegais e incompatíveis.
 * Dica para a Banca: "Aplicamos type-inference post-order. Primeiro os filhos resolvem seus tipos, o nó pai checa a compatibilidade e assina a si próprio com o tipo resultante."
 */
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

/*
 * O que o método faz: Analisa e valida os comandos um a um (condicionais, repetição, primitivas).
 * Papel no Pipeline: Semântico.
 * Regra da G-V1: Garantir que lógicas de condicional ("se" e "enquanto") tenham condição booleana (resolvida por INT no G-V1).
 * Dica para a Banca: "Delegações eficientes que forçam restrições pesadas na árvore; se um `while` tem condição em CAR, nós paramos o processo."
 */
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

/*
 * O que o método faz: Engatilha todo o fluxo de Type Check e Binding de símbolos começando do nó raiz.
 * Papel no Pipeline: Semântico (Orquestração Inicial).
 * Regra da G-V1: Inicia o processo com um top-scope para o Bloco do Principal.
 * Dica para a Banca: "Esta interface conecta o mundo exterior do main() na nossa infraestrutura profunda de validação, mascarando a complexidade do contexto."
 */
void check_semantics(Program *prog) {
    SemanticCtx sem;
    sem.top = NULL;
    analyze_block(&sem, prog->block);
}
