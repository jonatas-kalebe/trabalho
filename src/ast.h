#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>

/*
 * O que a estrutura faz: Enumera os tipos de dados básicos suportados pela linguagem.
 * Papel no Pipeline: Árvore (AST) -> Semântico.
 * Regra da G-V1: Necessário para a checagem de tipos int e car.

 */
typedef enum {
    TYPE_INT,
    TYPE_CAR
} Type;

/*
 * O que a estrutura faz: Define os tipos de expressões possíveis na AST.
 * Papel no Pipeline: Árvore (AST) -> Semântico -> Gerador de Código MIPS.
 * Regra da G-V1: Distinção clara entre operadores e literais para simplificar o caminhamento da árvore.

 */
typedef enum {
    EX_VAR,
    EX_INT,
    EX_CHAR,
    EX_ASSIGN,
    EX_BINARY,
    EX_UNARY
} ExprKind;

/*
 * O que a estrutura faz: Lista todas as operações unárias e binárias válidas.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Suporte a operações aritméticas, relacionais e lógicas do G-V1.

 */
typedef enum {
    OP_OR,
    OP_AND,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_GE,
    OP_LE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    OP_NOT
} OpKind;

typedef struct Expr Expr;

/*
 * O que a estrutura faz: Representa uma expressão binária com dois operandos.
 * Papel no Pipeline: Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Resolução de expressões matemáticas e booleanas.

 */
typedef struct {
    OpKind op;
    Expr *left;
    Expr *right;
} BinaryExpr;

/*
 * O que a estrutura faz: Representa uma expressão unária, como negação aritmética ou lógica.
 * Papel no Pipeline: Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Suporte a operadores unários de sinal e negação.

 */
typedef struct {
    OpKind op;
    Expr *expr;
} UnaryExpr;

/*
 * O que a estrutura faz: Estrutura genérica (variant/union) que modela qualquer nó de expressão na AST.
 * Papel no Pipeline: Árvore (AST) -> Semântico -> Gerador de Código MIPS.
 * Regra da G-V1: A gestão de memória usa uma AST compacta.

 */
struct Expr {
    ExprKind kind;
    int line;
    Type inferred_type;
    union {
        char *name;
        int int_value;
        int char_value;
        struct {
            char *name;
            Expr *value;
        } assign;
        BinaryExpr bin;
        UnaryExpr un;
    } as;
};

/*
 * O que a estrutura faz: Representa a declaração de uma variável em um bloco.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Semântico.
 * Regra da G-V1: Definição de escopo e variáveis locais.

 */
typedef struct Decl {
    char *name;
    Type type;
    int line;
    struct Decl *next;
} Decl;

typedef struct Block Block;

/*
 * O que a estrutura faz: Enumera os tipos de comandos/statements da linguagem.
 * Papel no Pipeline: Árvore (AST) -> Semântico -> Gerador de Código MIPS.
 * Regra da G-V1: Controle de fluxo e comandos nativos como leia e escreva.

 */
typedef enum {
    ST_EMPTY,
    ST_EXPR,
    ST_READ,
    ST_WRITE_EXPR,
    ST_WRITE_STR,
    ST_NEWLINE,
    ST_IF,
    ST_WHILE,
    ST_BLOCK
} StmtKind;

/*
 * O que a estrutura faz: Estrutura genérica que modela comandos lógicos, de I/O ou controle de fluxo.
 * Papel no Pipeline: Árvore (AST) -> Semântico -> Gerador de Código MIPS.
 * Regra da G-V1: A gestão de memória usa uma AST compacta para as estruturas de controle.

 */
typedef struct Stmt {
    StmtKind kind;
    int line;
    struct Stmt *next;
    union {
        Expr *expr;
        char *name;
        struct {
            Expr *cond;
            struct Stmt *then_branch;
            struct Stmt *else_branch;
        } if_stmt;
        struct {
            Expr *cond;
            struct Stmt *body;
        } while_stmt;
        Block *block;
        struct {
            char *text;
            char *label;
        } write_str;
    } as;
} Stmt;

/*
 * O que a estrutura faz: Representa um bloco de código (escopo) com declarações e comandos.
 * Papel no Pipeline: Árvore (AST) -> Semântico -> Gerador de Código MIPS.
 * Regra da G-V1: O controle de escopo deve respeitar variáveis locais sobrepondo globais ao bloco (shadowing).

 */
struct Block {
    Decl *decls;
    Stmt *commands;
    int line;
};

/*
 * O que a estrutura faz: Define o nó raiz de todo o programa compilado.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Ponto de partida ('principal') da estrutura G-V1.

 */
typedef struct {
    Block *block;
} Program;

// Function declarations for AST node creation

/*
 * O que o método faz: Instancia um nó de expressão referenciando uma variável.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Acesso a variáveis definidas no escopo.

 */
Expr *new_expr_var(char *name, int line);

/*
 * O que o método faz: Cria um nó de literal inteiro.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Conversão de lexemas númericos.

 */
Expr *new_expr_int(int value, int line);

/*
 * O que o método faz: Cria um nó de literal do tipo car (caractere).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Checagem de tipos int e car e gerenciamento de valores.

 */
Expr *new_expr_char(int value, int line);

/*
 * O que o método faz: Cria nó de atribuição (x = exp).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Garantir corretude na atualização de variáveis na memória (stack/MIPS).

 */
Expr *new_expr_assign(char *name, Expr *value, int line);

/*
 * O que o método faz: Constroi um nó de operação binária (+, -, *, etc).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Formação correta de precedências matemáticas vindas do Bison.

 */
Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line);

/*
 * O que o método faz: Constroi nó de operação unária.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Modelagem de operadores unários na lógica do G-V1.

 */
Expr *new_expr_unary(OpKind op, Expr *expr, int line);

/*
 * O que o método faz: Aloca um registro de declaração de variável.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Controle de variáveis e tabela de símbolos.

 */
Decl *new_decl(char *name, Type type, int line);

/*
 * O que o método faz: Criações de statements e nós de comando da linguagem.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Mapear comandos como leia, escreva, loops e condicional se-entao-senao.

 */
Stmt *new_stmt_empty(int line);
Stmt *new_stmt_expr(Expr *expr, int line);
Stmt *new_stmt_read(char *name, int line);
Stmt *new_stmt_write_expr(Expr *expr, int line);
Stmt *new_stmt_write_str(char *text, int line);
Stmt *new_stmt_newline(int line);
Stmt *new_stmt_if(Expr *cond, Stmt *then_branch, Stmt *else_branch, int line);
Stmt *new_stmt_while(Expr *cond, Stmt *body, int line);
Stmt *new_stmt_block(Block *block, int line);

/*
 * O que o método faz: Associa uma lista de declarações a uma lista de comandos no mesmo bloco.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Implementação de escopo modular baseado em blocos.

 */
Block *new_block(Decl *decls, Stmt *commands, int line);

// Utility functions
/*
 * O que o método faz: Trata de erros de alocação (out-of-memory).
 * Papel no Pipeline: Global -> Aborta o processo.
 * Regra da G-V1: Manter a integridade caso o host perca recursos de heap.

 */
void die_alloc(void);

/*
 * O que o método faz: Criação customizada da rotina strdup (que nem sempre está na libc padrão com strict ANSI).
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Clonagem segura de nomes de variáveis vindas do buffer do Lexer (Bison/Flex).

 */
char *xstrdup(const char *s);

#endif
