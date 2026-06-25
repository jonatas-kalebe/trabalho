/* ============================================================================
 *  ast.h  --  Definicao da Arvore Sintatica Abstrata (AST) da linguagem
 *             Cafezinho / Goianinha.
 *
 *  CONCEITO (o que o professor pergunta):
 *  --------------------------------------
 *  A AST e a "Representacao Intermediaria" (RI) do programa. Depois que a
 *  analise lexica (Flex) quebra o texto em tokens e a analise sintatica (Bison)
 *  reconhece a estrutura gramatical, NAO trabalhamos mais com texto: montamos
 *  esta arvore de structs em C. Todas as fases seguintes (analise semantica e
 *  geracao de codigo MIPS) apenas "caminham" (percorrem) esta arvore.
 *
 *  Por que uma arvore?  Porque um programa e naturalmente hierarquico:
 *  um programa contem funcoes, que contem blocos, que contem comandos, que
 *  contem expressoes, que contem subexpressoes. A arvore captura exatamente
 *  esse aninhamento.
 * ==========================================================================*/
#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------------------------------
 *  Tipos basicos da linguagem. So existem dois: inteiro e caractere.
 *  (Em Cafezinho, 'car' e essencialmente um inteiro de 1 caractere ASCII.)
 * --------------------------------------------------------------------------*/
typedef enum {
    TYPE_INT,
    TYPE_CAR
} Type;

/* ----------------------------------------------------------------------------
 *  CATEGORIA de uma variavel -- onde ela "mora" na memoria em tempo de
 *  execucao. Isso e decidido na analise semantica e usado na geracao de codigo
 *  para saber COMO acessar a variavel (via $s1, via $fp+, ou via $fp-).
 *
 *    CAT_GLOBAL : variavel global  -> acessada a partir do registrador $s1.
 *    CAT_PARAM  : parametro de funcao -> acessado a partir de $fp + 4*indice.
 *    CAT_LOCAL  : variavel local de bloco -> acessada a partir de $fp - 4*pos.
 * --------------------------------------------------------------------------*/
enum {
    CAT_NONE = 0,   /* ainda nao resolvida pela semantica */
    CAT_GLOBAL,
    CAT_PARAM,
    CAT_LOCAL
};

/* ----------------------------------------------------------------------------
 *  VarRef -- "anotacao" que a analise semantica grava em cada uso de variavel.
 *  Quando o parser cria um no de variavel ele so conhece o NOME. A semantica
 *  procura esse nome na tabela de simbolos e preenche aqui a categoria, a
 *  posicao/indice, o tipo e se e vetor. A geracao de codigo le essas anotacoes.
 *
 *  Esse e um exemplo classico de "decoracao da AST": a arvore comeca crua
 *  (so com a estrutura sintatica) e vai sendo enriquecida com informacao
 *  semantica.
 * --------------------------------------------------------------------------*/
typedef struct {
    int  category;   /* CAT_GLOBAL / CAT_PARAM / CAT_LOCAL                    */
    int  position;   /* posicao (global/local) OU indice do parametro        */
    Type type;       /* tipo do elemento                                     */
    int  is_array;   /* 1 se a variavel declarada e um vetor                 */
} VarRef;

/* ----------------------------------------------------------------------------
 *  Operadores unarios e binarios. A ordem nao importa para a semantica, mas
 *  cada um vira uma (ou poucas) instrucoes MIPS na geracao de codigo.
 * --------------------------------------------------------------------------*/
typedef enum {
    OP_OR,   /* ||  */
    OP_AND,  /* &&  */
    OP_EQ,   /* ==  */
    OP_NE,   /* !=  */
    OP_LT,   /* <   */
    OP_GT,   /* >   */
    OP_GE,   /* >=  */
    OP_LE,   /* <=  */
    OP_ADD,  /* +   */
    OP_SUB,  /* -   */
    OP_MUL,  /* *   */
    OP_DIV,  /* /   */
    OP_NEG,  /* -  (menos unario) */
    OP_NOT   /* !  (nao logico)   */
} OpKind;

/* ----------------------------------------------------------------------------
 *  Os diferentes "formatos" de expressao. Usamos a tecnica de "tagged union":
 *  um enum (kind) diz qual variante esta ativa, e uma union economiza memoria
 *  guardando apenas os campos daquela variante.
 * --------------------------------------------------------------------------*/
typedef enum {
    EX_INT,     /* constante inteira        ex: 42                            */
    EX_CHAR,    /* constante caractere      ex: 'a'                           */
    EX_VAR,     /* uso de variavel escalar  ex: n                            */
    EX_ARRAY,   /* [PARTE 2 - NOVO] acesso a elemento de vetor   ex: vet[i]   */
    EX_CALL,    /* [PARTE 2 - NOVO] chamada de funcao            ex: fat(n-1) */
    EX_ASSIGN,  /* atribuicao  x=e  ou (NOVO na P2) vet[i]=e                  */
    EX_BINARY,  /* operacao binaria         ex: a + b                        */
    EX_UNARY    /* operacao unaria          ex: -a  ou  !a                   */
} ExprKind;

typedef struct Expr Expr;
typedef struct Arg  Arg;

/* No de expressao. 'inferred_type' e 'is_array_result' sao preenchidos pela
 * analise semantica (tipo resultante da expressao). */
struct Expr {
    ExprKind kind;
    int      line;             /* linha do codigo-fonte (para mensagens)     */
    Type     inferred_type;    /* tipo calculado pela semantica              */
    int      is_array_result;  /* 1 se a expressao denota um vetor inteiro   */
    union {
        int int_value;                                   /* EX_INT          */
        int char_value;                                  /* EX_CHAR         */
        struct { char *name; VarRef ref; } var;          /* EX_VAR          */
        struct { char *name; Expr *index; VarRef ref; } arr;   /* EX_ARRAY  */
        struct { char *name; Arg *args; int argc; } call;      /* EX_CALL   */
        struct {                                         /* EX_ASSIGN       */
            char  *name;
            Expr  *index;   /* NULL => atribuicao a escalar; senao a vet[i]  */
            Expr  *value;
            VarRef ref;
        } assign;
        struct { OpKind op; Expr *left, *right; } bin;   /* EX_BINARY       */
        struct { OpKind op; Expr *operand; } un;         /* EX_UNARY        */
    } as;
};

/* Lista ligada de argumentos de uma chamada de funcao. */
struct Arg {
    Expr *expr;
    Arg  *next;
};

/* ----------------------------------------------------------------------------
 *  Declaracao de variavel (em 'global[...]' ou no '[...]' de um bloco).
 *  'category' e 'position' sao anotados pela semantica (vide VarRef acima).
 * --------------------------------------------------------------------------*/
typedef struct Decl {
    char *name;
    Type  type;
    int   is_array;     /* [PARTE 2 - NOVO] 1 se for vetor                    */
    int   array_size;   /* [PARTE 2 - NOVO] tamanho do vetor (se is_array)    */
    int   line;
    int   category;     /* CAT_GLOBAL ou CAT_LOCAL (anotado pela semantica)  */
    int   position;     /* posicao na area de globais/locais                 */
    struct Decl *next;
} Decl;

/* ----------------------------------------------------------------------------
 *  [PARTE 2 - NOVO] Parametro formal de uma funcao (nao existia na G-V1).
 *  Pode ser escalar (n:int) ou vetor (v[]:int). Vetores sao passados POR
 *  REFERENCIA (passa-se o endereco base, nao uma copia).
 * --------------------------------------------------------------------------*/
typedef struct Param {
    char *name;
    Type  type;
    int   is_array;
    int   line;
    int   index;        /* indice do parametro (1..n), anotado pela semantica */
    struct Param *next;
} Param;

typedef struct Block Block;

/* ----------------------------------------------------------------------------
 *  Os tipos de comando (statement) da linguagem.
 * --------------------------------------------------------------------------*/
typedef enum {
    ST_EMPTY,       /* ;                                                     */
    ST_EXPR,        /* expressao usada como comando (ex: chamada, atribuicao)*/
    ST_RETURN,      /* [PARTE 2 - NOVO] retorne Expr;  (so existe por causa das funcoes) */
    ST_READ,        /* leia lvalue;                                          */
    ST_WRITE_EXPR,  /* escreva Expr;                                         */
    ST_WRITE_STR,   /* escreva "texto";                                      */
    ST_NEWLINE,     /* novalinha;                                            */
    ST_IF,          /* se (..) entao .. [senao ..] fimse                     */
    ST_WHILE,       /* enquanto (..) ..                                      */
    ST_BLOCK        /* um bloco aninhado [decls]{cmds}                       */
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    int      line;
    struct Stmt *next;     /* comandos sao encadeados em sequencia           */
    union {
        Expr *expr;                                  /* EXPR/RETURN/WRITE_EXPR */
        struct { char *name; Expr *index; VarRef ref; } read; /* ST_READ      */
        struct { char *text; char *label; } wstr;    /* ST_WRITE_STR          */
        struct { Expr *cond; struct Stmt *then_branch;
                 struct Stmt *else_branch; } if_s;    /* ST_IF                 */
        struct { Expr *cond; struct Stmt *body; } while_s; /* ST_WHILE         */
        Block *block;                                /* ST_BLOCK              */
    } as;
} Stmt;

/* ----------------------------------------------------------------------------
 *  Bloco = uma area de declaracoes opcionais seguida de uma sequencia de
 *  comandos. Cada bloco aninhado abre um novo ESCOPO (conceito central).
 * --------------------------------------------------------------------------*/
struct Block {
    Decl *decls;       /* lista de declaracoes locais (pode ser NULL)        */
    Stmt *commands;    /* lista de comandos                                  */
    int   line;
};

/* ----------------------------------------------------------------------------
 *  [PARTE 2 - NOVO] Funcao (nao existia na G-V1). Lista ligada (varias funcoes
 *  dentro de 'funcao[...]').
 * --------------------------------------------------------------------------*/
typedef struct Func {
    char  *name;
    Param *params;
    int    param_count;
    Type   return_type;
    Block *body;
    int    line;
    struct Func *next;
} Func;

/* ----------------------------------------------------------------------------
 *  No raiz: o programa inteiro. Globais + funcoes + bloco principal.
 * --------------------------------------------------------------------------*/
typedef struct {
    Decl *globals;       /* [PARTE 2 - NOVO] variaveis globais (pode ser NULL) */
    Func *functions;     /* [PARTE 2 - NOVO] funcoes do programa (pode ser NULL)*/
    Block *main_block;   /* corpo de 'principal'                             */
} Program;

/* ============================================================================
 *  Construtores -- funcoes "new_*" que alocam e inicializam cada tipo de no.
 *  O parser (parser.y) chama estas funcoes nas acoes semanticas para montar
 *  a AST de baixo para cima (bottom-up), conforme reduz as regras.
 * ==========================================================================*/
Expr *new_expr_int   (int value, int line);
Expr *new_expr_char  (int value, int line);
Expr *new_expr_var   (char *name, int line);
Expr *new_expr_array (char *name, Expr *index, int line);
Expr *new_expr_call  (char *name, Arg *args, int line);
Expr *new_expr_assign(char *name, Expr *index, Expr *value, int line);
Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line);
Expr *new_expr_unary (OpKind op, Expr *operand, int line);
Arg  *new_arg        (Expr *expr);

Decl  *new_decl  (char *name, Type type, int is_array, int array_size, int line);
Param *new_param (char *name, Type type, int is_array, int line);
Block *new_block (Decl *decls, Stmt *commands, int line);
Func  *new_func  (char *name, Param *params, Type return_type, Block *body, int line);

Stmt *new_stmt_empty     (int line);
Stmt *new_stmt_expr      (Expr *expr, int line);
Stmt *new_stmt_return    (Expr *expr, int line);
Stmt *new_stmt_read      (char *name, Expr *index, int line);
Stmt *new_stmt_write_expr(Expr *expr, int line);
Stmt *new_stmt_write_str (char *text, int line);
Stmt *new_stmt_newline   (int line);
Stmt *new_stmt_if        (Expr *cond, Stmt *then_b, Stmt *else_b, int line);
Stmt *new_stmt_while     (Expr *cond, Stmt *body, int line);
Stmt *new_stmt_block     (Block *block, int line);

/* Utilitarios. */
void  die_alloc(void);            /* aborta em caso de falta de memoria       */
char *xstrdup(const char *s);     /* strdup portatil (ANSI C estrito)         */

#endif /* AST_H */
