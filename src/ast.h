#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>

/*
 * O que a estrutura faz: Enumera os tipos de dados básicos suportados pela linguagem.
 * Papel no Pipeline: Árvore (AST) -> Semântico.
 * Regra da G-V1: Necessário para a checagem de tipos int e car.
 * Dica para a Banca: "Usamos esse enum para garantir que a tipagem estática seja avaliada corretamente durante a análise semântica em tempo de compilação."
 */
typedef enum {
    TYPE_INT,
    TYPE_CAR
} Type;

/*
 * O que a estrutura faz: Define os tipos de expressões possíveis na AST.
 * Papel no Pipeline: Árvore (AST) -> Semântico -> Gerador de Código MIPS.
 * Regra da G-V1: Distinção clara entre operadores e literais para simplificar o caminhamento da árvore.
 * Dica para a Banca: "Classificar as expressões simplifica o pattern matching no visitor de geração de código."
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
 * Dica para a Banca: "Mapear cada operador para um enum garante que instruções MIPS específicas, como 'add' ou 'seq', sejam emitidas com precisão na fase final."
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
 * Dica para a Banca: "Cada nó de expressão binária resolve recursivamente seus filhos (left e right) antes de aplicar a operação na pilha do MIPS."
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
 * Dica para a Banca: "Mantemos o mesmo nó flexível tanto para - (menos unário) quanto para ! (negação)."
 */
typedef struct {
    OpKind op;
    Expr *expr;
} UnaryExpr;

/*
 * O que a estrutura faz: Estrutura genérica (variant/union) que modela qualquer nó de expressão na AST.
 * Papel no Pipeline: Árvore (AST) -> Semântico -> Gerador de Código MIPS.
 * Regra da G-V1: A gestão de memória usa uma AST compacta.
 * Dica para a Banca: "Usamos uma union de C para criar uma 'AST compacta', minimizando o footprint de memória durante a compilação."
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
 * Dica para a Banca: "A AST de declarações é desenhada como lista encadeada para facilitar o preenchimento sequencial da tabela de símbolos."
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
 * Dica para a Banca: "Separamos Statements de Expressões para refletir fielmente a gramática imperativa da G-V1."
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
 * Dica para a Banca: "Agrupamos todos os statements em uma union para poupar alocações e evitar complexidade de herança artificial em C."
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
 * Dica para a Banca: "O bloco é a unidade atômica para empilhar novos contextos na análise semântica, lidando com o shadowing naturalmente."
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
 * Dica para a Banca: "A partir da raiz, disparamos as rotinas recursivas que atravessam a AST por todo o ciclo de vida da compilação."
 */
typedef struct {
    Block *block;
} Program;

// Function declarations for AST node creation

/*
 * O que o método faz: Instancia um nó de expressão referenciando uma variável.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Acesso a variáveis definidas no escopo.
 * Dica para a Banca: "Delegações simples de alocação que ajudam a manter a gramática Bison limpa e concisa."
 */
Expr *new_expr_var(char *name, int line);

/*
 * O que o método faz: Cria um nó de literal inteiro.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Conversão de lexemas númericos.
 * Dica para a Banca: "Capturamos o valor numérico puro já do lexer para eliminar parse adicional durante a geração da AST."
 */
Expr *new_expr_int(int value, int line);

/*
 * O que o método faz: Cria um nó de literal do tipo car (caractere).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Checagem de tipos int e car e gerenciamento de valores.
 * Dica para a Banca: "O valor é embutido diretamente no nó para economizar ponteiros de dados."
 */
Expr *new_expr_char(int value, int line);

/*
 * O que o método faz: Cria nó de atribuição (x = exp).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Garantir corretude na atualização de variáveis na memória (stack/MIPS).
 * Dica para a Banca: "Nós de atribuição vinculam imediatamente o nome do target ao seu valor right-hand-side, preparando para type-checking."
 */
Expr *new_expr_assign(char *name, Expr *value, int line);

/*
 * O que o método faz: Constroi um nó de operação binária (+, -, *, etc).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Formação correta de precedências matemáticas vindas do Bison.
 * Dica para a Banca: "A estrutura de união reduz as chamadas de malloc para este tipo comum de nó na AST."
 */
Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line);

/*
 * O que o método faz: Constroi nó de operação unária.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Modelagem de operadores unários na lógica do G-V1.
 * Dica para a Banca: "Tratamento isolado que facilita o handling de negações e sinal."
 */
Expr *new_expr_unary(OpKind op, Expr *expr, int line);

/*
 * O que o método faz: Aloca um registro de declaração de variável.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Controle de variáveis e tabela de símbolos.
 * Dica para a Banca: "O layout sequencial facilita a descoberta de declarações sucessivas no topo do bloco."
 */
Decl *new_decl(char *name, Type type, int line);

/*
 * O que o método faz: Criações de statements e nós de comando da linguagem.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Mapear comandos como leia, escreva, loops e condicional se-entao-senao.
 * Dica para a Banca: "Funções de fábrica especializadas mantêm a abstração entre o construtor da gramática no Bison e o layout da árvore."
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
 * Dica para a Banca: "Essencial para garantir que cada parênteses curly do G-V1 inicie um novo namespace temporário de escopo."
 */
Block *new_block(Decl *decls, Stmt *commands, int line);

// Utility functions
/*
 * O que o método faz: Trata de erros de alocação (out-of-memory).
 * Papel no Pipeline: Global -> Aborta o processo.
 * Regra da G-V1: Manter a integridade caso o host perca recursos de heap.
 * Dica para a Banca: "Tratamento defensivo para evitar segfaults caso tentemos criar a AST inteira para um código monstruoso."
 */
void die_alloc(void);

/*
 * O que o método faz: Criação customizada da rotina strdup (que nem sempre está na libc padrão com strict ANSI).
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Clonagem segura de nomes de variáveis vindas do buffer do Lexer (Bison/Flex).
 * Dica para a Banca: "Usamos nossa própria duplicação de strings para independência dos padrões da biblioteca do C e facilitar portabilidade."
 */
char *xstrdup(const char *s);

#endif
