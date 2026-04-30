#include <stdio.h>
#include "ast.h"

/*
 * O que o método faz: Encerra o compilador relatando falha em alocação de memória.
 * Papel no Pipeline: Auxiliar / Árvore (AST).
 * Regra da G-V1: A gestão de memória usa uma AST compacta e exige resiliência.

 */
void die_alloc(void) {
    fprintf(stderr, "ERRO: FALHA DE MEMORIA\n");
    exit(1);
}

/*
 * O que o método faz: Copia de forma segura uma string (lexema) para uma nova área de memória.
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Isolar o ciclo de vida do buffer do Lexer das strings alocadas na árvore.

 */
char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *r = (char *)malloc(n + 1);
    if (!r) die_alloc();
    memcpy(r, s, n + 1);
    return r;
}

/*
 * O que o método faz: Aloca dinamicamente um nó genérico da árvore de expressões.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: A gestão de memória usa uma AST compacta (função central de alocação de expressões).

 */
static Expr *alloc_expr(ExprKind k, int line) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    if (!e) die_alloc();
    e->kind = k;
    e->line = line;
    e->inferred_type = TYPE_INT;
    return e;
}

/*
 * O que o método faz: Retorna um novo nó representando uma variável.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: O controle de escopo precisará deste identificador na tabela depois.

 */
Expr *new_expr_var(char *name, int line) {
    Expr *e = alloc_expr(EX_VAR, line);
    e->as.name = name;
    return e;
}

/*
 * O que o método faz: Cria um nó primitivo na AST para lidar com literais do tipo inteiro.
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Tipo base de literais inteiros da G-V1.

 */
Expr *new_expr_int(int value, int line) {
    Expr *e = alloc_expr(EX_INT, line);
    e->as.int_value = value;
    return e;
}

/*
 * O que o método faz: Cria um nó primitivo na AST para literal de caractere.
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Tipo base de literais caracteres da G-V1.

 */
Expr *new_expr_char(int value, int line) {
    Expr *e = alloc_expr(EX_CHAR, line);
    e->as.char_value = value;
    return e;
}

/*
 * O que o método faz: Fabrica nó da AST correspondente a uma atribuição (ex: a = b + 1).
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Semântico.
 * Regra da G-V1: Construção da l-value para o gerador e update no escopo.

 */
Expr *new_expr_assign(char *name, Expr *value, int line) {
    Expr *e = alloc_expr(EX_ASSIGN, line);
    e->as.assign.name = name;
    e->as.assign.value = value;
    return e;
}

/*
 * O que o método faz: Anexa dois nós operandos (left, right) a uma operação binária.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Implementação hierárquica das precedências aritméticas do parser Bison.

 */
Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line) {
    Expr *e = alloc_expr(EX_BINARY, line);
    e->as.bin.op = op;
    e->as.bin.left = left;
    e->as.bin.right = right;
    return e;
}

/*
 * O que o método faz: Empacota um nó operando com um modificador lógico ou sinal.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Aplicação de modificadores sobre expressões existentes (como negativo unário).

 */
Expr *new_expr_unary(OpKind op, Expr *expr, int line) {
    Expr *e = alloc_expr(EX_UNARY, line);
    e->as.un.op = op;
    e->as.un.expr = expr;
    return e;
}

/*
 * O que o método faz: Inicializa as meta-informações de uma nova declaração formal de variável.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Semântico.
 * Regra da G-V1: O controle de escopo deve respeitar variáveis locais; isto modela a variável.

 */
Decl *new_decl(char *name, Type type, int line) {
    Decl *d = (Decl *)calloc(1, sizeof(Decl));
    if (!d) die_alloc();
    d->name = name;
    d->type = type;
    d->line = line;
    d->next = NULL;
    return d;
}

/*
 * O que o método faz: Aloca genericamente um bloco de Comando (Statement) vazio/virgem na memória.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: A gestão de memória usa uma AST compacta para ramificações lógicas.

 */
static Stmt *alloc_stmt(StmtKind kind, int line) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    if (!s) die_alloc();
    s->kind = kind;
    s->line = line;
    s->next = NULL;
    return s;
}

/*
 * O que o método faz: Cria um placeholder para comandos vazios (ex. `;` extra).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Resiliência contra excesso de delimitadores sem perder controle de linha.

 */
Stmt *new_stmt_empty(int line) {
    return alloc_stmt(ST_EMPTY, line);
}

/*
 * O que o método faz: Promove uma expressão a um statement, para suportar execução procedural de cálculos isolados.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Permite que operações como atribuição entrem num bloco de forma autônoma.

 */
Stmt *new_stmt_expr(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_EXPR, line);
    s->as.expr = expr;
    return s;
}

/*
 * O que o método faz: Prepara o nó sintático que disparará o sys call MIPS de leitura.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Interface de entrada de dados interativa.

 */
Stmt *new_stmt_read(char *name, int line) {
    Stmt *s = alloc_stmt(ST_READ, line);
    s->as.name = name;
    return s;
}

/*
 * O que o método faz: Monta um nó para exibição condicional do resultado de expressões no console.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Comando 'escreva' atrelado a processamento.

 */
Stmt *new_stmt_write_expr(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_EXPR, line);
    s->as.expr = expr;
    return s;
}

/*
 * O que o método faz: Nós para printagem de literais de string (mensagens diretas).
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS (seção .data).
 * Regra da G-V1: Escrever constantes estáticas (label base).

 */
Stmt *new_stmt_write_str(char *text, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_STR, line);
    s->as.write_str.text = text;
    s->as.write_str.label = NULL;
    return s;
}

/*
 * O que o método faz: Gera a instrução para quebrar a linha no output padrão.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Formatação de I/O de acordo com o comando 'novalinha'.

 */
Stmt *new_stmt_newline(int line) {
    return alloc_stmt(ST_NEWLINE, line);
}

/*
 * O que o método faz: Aloca a bifurcação de controle de fluxo condicional com ou sem ELSE.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Estruturas de decisão lógicas ('se').

 */
Stmt *new_stmt_if(Expr *cond, Stmt *then_branch, Stmt *else_branch, int line) {
    Stmt *s = alloc_stmt(ST_IF, line);
    s->as.if_stmt.cond = cond;
    s->as.if_stmt.then_branch = then_branch;
    s->as.if_stmt.else_branch = else_branch;
    return s;
}

/*
 * O que o método faz: Empacota a estrutura cíclica contendo Condição e Corpo a ser repetido.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Representar o comando 'enquanto' da G-V1 com desvios (branchs).

 */
Stmt *new_stmt_while(Expr *cond, Stmt *body, int line) {
    Stmt *s = alloc_stmt(ST_WHILE, line);
    s->as.while_stmt.cond = cond;
    s->as.while_stmt.body = body;
    return s;
}

/*
 * O que o método faz: Cria um sub-comando que encapsula um Bloco interior completo.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Semântico.
 * Regra da G-V1: Controle de sub-escopos permitindo shadowing modular.

 */
Stmt *new_stmt_block(Block *block, int line) {
    Stmt *s = alloc_stmt(ST_BLOCK, line);
    s->as.block = block;
    return s;
}

/*
 * O que o método faz: Combina lista de declarações de topo de bloco com os comandos executáveis.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Unidade básica de organização e escopo do G-V1.

 */
Block *new_block(Decl *decls, Stmt *commands, int line) {
    Block *b = (Block *)calloc(1, sizeof(Block));
    if (!b) die_alloc();
    b->decls = decls;
    b->commands = commands;
    b->line = line;
    return b;
}
