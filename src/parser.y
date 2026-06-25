/* ============================================================================
 *  parser.y  --  Analisador SINTATICO da linguagem Cafezinho, em Bison.
 *
 *  CONCEITO (2a fase do compilador):
 *  ---------------------------------
 *  O analisador sintatico recebe a sequencia de tokens do lexer e verifica se
 *  ela obedece a GRAMATICA da linguagem, ao mesmo tempo em que CONSTROI a AST.
 *  O Bison gera um parser LALR(1) (bottom-up, "shift-reduce"): ele empilha
 *  simbolos (shift) e, quando reconhece o lado direito de uma regra, reduz
 *  (reduce) executando a acao em C que monta o no correspondente da arvore.
 *
 *  A precedencia e a associatividade dos operadores (declaradas com %left /
 *  %right abaixo) resolvem as ambiguidades das expressoes SEM precisar inflar
 *  a gramatica com regras auxiliares.
 * ==========================================================================*/
%{
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int  yylex(void);
extern int  yylineno;
void yyerror(const char *s);

/* O no raiz da AST, preenchido ao final da analise. */
Program root_program;

/* Estrutura auxiliar SO do parser: acumula os nomes de uma declaracao
 * multipla (ex.: "v1[5], v2[5], soma : int;") junto com info de vetor.
 * Depois, na regra DeclVar, cada item vira um Decl com o tipo informado. */
typedef struct IdItem {
    char *name;
    int   is_array;
    int   size;
    struct IdItem *next;
} IdItem;

static IdItem *make_iditem(char *name, int is_array, int size) {
    IdItem *it = (IdItem *)malloc(sizeof(IdItem));
    if (!it) die_alloc();
    it->name = name;
    it->is_array = is_array;
    it->size = size;
    it->next = NULL;
    return it;
}
%}

%locations   /* habilita rastreamento de linha (@1, @$) para mensagens de erro */

%union {
    int    ival;
    int    cval;
    char  *sval;
    int    type;        /* Type (TYPE_INT / TYPE_CAR)                         */
    Expr  *expr;
    Arg   *arg;
    Stmt  *stmt;
    Block *block;
    Decl  *decl;
    Param *param;
    Func  *func;
    struct IdItem *iditem;
}

/* ---- Tokens ------------------------------------------------------------- */
%token TOK_GLOBAL TOK_FUNCAO TOK_PRINCIPAL
%token TOK_SE TOK_ENTAO TOK_SENAO TOK_FIMSE TOK_ENQUANTO TOK_RETORNE
%token TOK_LEIA TOK_ESCREVA TOK_NOVALINHA
%token TOK_INT TOK_CAR
%token TOK_OU TOK_E TOK_IGUAL TOK_DIFERENTE TOK_MAIORIGUAL TOK_MENORIGUAL
%token TOK_LT TOK_GT TOK_PLUS TOK_MINUS TOK_STAR TOK_SLASH TOK_BANG TOK_ASSIGN
%token TOK_LBRACE TOK_RBRACE TOK_LPAREN TOK_RPAREN TOK_LBRACK TOK_RBRACK
%token TOK_COLON TOK_SEMICOLON TOK_COMMA

%token <ival> TOK_INTCONST
%token <cval> TOK_CHARCONST
%token <sval> TOK_STRING
%token <sval> TOK_IDENT

/* ---- Tipos dos nao-terminais ------------------------------------------- */
%type <decl>   Globais DeclVarList DeclVar
%type <func>   Funcoes FuncList Funcao
%type <param>  Params ParamList Param
%type <block>  Principal Bloco
%type <stmt>   ComandoList Comando
%type <expr>   Expr
%type <arg>    ArgListOpt ArgList
%type <iditem> IdentList IdentItem
%type <type>   Tipo

/* ---- Precedencia (da MENOR para a MAIOR) ------------------------------- */
%right TOK_ASSIGN
%left  TOK_OU
%left  TOK_E
%left  TOK_IGUAL TOK_DIFERENTE
%left  TOK_LT TOK_GT TOK_MAIORIGUAL TOK_MENORIGUAL
%left  TOK_PLUS TOK_MINUS
%left  TOK_STAR TOK_SLASH
%right TOK_BANG UMINUS

/* Observacao: esta gramatica e LALR(1) SEM conflitos. Apos um IDENT, a
 * decisao entre variavel / indexacao '[' / chamada '(' / atribuicao '=' e
 * tomada apenas pelo proximo token (lookahead), cujos conjuntos sao
 * disjuntos -- por isso nao ha ambiguidade. */

%%

/* ===========================================================================
 *  Estrutura geral do programa:  [global] [funcao] principal
 *
 *  [PARTE 2 - NOVO] Na linguagem-base (G-V1) o programa era SO o 'principal'.
 *  Agora ele tem duas secoes OPCIONAIS antes dele: 'global' (variaveis globais)
 *  e 'funcao' (funcoes). Por isso a regra ganhou os nao-terminais Globais e
 *  Funcoes (ambos podem ser vazios).
 * =========================================================================*/
Programa:
    Globais Funcoes Principal {
        root_program.globals   = $1;
        root_program.functions = $2;
        root_program.main_block= $3;
    }
;

/* [PARTE 2 - NOVO] secao de variaveis globais: opcional. */
Globais:
    /* vazio */                                  { $$ = NULL; }
  | TOK_GLOBAL TOK_LBRACK DeclVarList TOK_RBRACK { $$ = $3; }
;

/* [PARTE 2 - NOVO] secao de funcoes: opcional, contem uma lista de funcoes. */
Funcoes:
    /* vazio */                                  { $$ = NULL; }
  | TOK_FUNCAO TOK_LBRACK FuncList TOK_RBRACK    { $$ = $3; }
;

Principal:
    TOK_PRINCIPAL Bloco                          { $$ = $2; }
;

/* [PARTE 2 - NOVO] ---- Funcoes (tudo abaixo nao existia na G-V1) --------- */
FuncList:
    Funcao                                       { $$ = $1; }
  | Funcao FuncList                              { $1->next = $2; $$ = $1; }
;

/* [PARTE 2 - NOVO] uma funcao: nome(parametros): tipo  + corpo (Bloco). */
Funcao:
    TOK_IDENT TOK_LPAREN Params TOK_RPAREN TOK_COLON Tipo Bloco {
        $$ = new_func($1, $3, $6, $7, @1.first_line);
    }
;

/* [PARTE 2 - NOVO] lista de parametros formais (pode ser vazia). */
Params:
    /* vazio */                                  { $$ = NULL; }
  | ParamList                                    { $$ = $1; }
;

ParamList:
    Param                                        { $$ = $1; }
  | Param TOK_COMMA ParamList                    { $1->next = $3; $$ = $1; }
;

/* [PARTE 2 - NOVO] um parametro pode ser ESCALAR (n:int) ou VETOR (v[]:int). */
Param:
    TOK_IDENT TOK_COLON Tipo {
        $$ = new_param($1, $3, 0, @1.first_line);              /* escalar */
    }
  | TOK_IDENT TOK_LBRACK TOK_RBRACK TOK_COLON Tipo {
        $$ = new_param($1, $5, 1, @1.first_line);              /* vetor   */
    }
;

/* ---- Bloco: declaracoes opcionais + comandos --------------------------- */
/* [PARTE 2 - NOVO] As declaracoes agora ficam entre COLCHETES "[ ]" (na G-V1
 * eram entre chaves). O bloco de comandos continua entre CHAVES "{ }". */
Bloco:
    TOK_LBRACK DeclVarList TOK_RBRACK TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block($2, $5, @1.first_line);
    }
  | TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block(NULL, $2, @1.first_line);
    }
;

/* ---- Declaracoes de variaveis ------------------------------------------ */
DeclVarList:
    DeclVar                                      { $$ = $1; }
  | DeclVar DeclVarList {
        Decl *t = $1; while (t->next) t = t->next; t->next = $2; $$ = $1;
    }
;

DeclVar:
    IdentList TOK_COLON Tipo TOK_SEMICOLON {
        /* transforma cada item da lista de nomes em um Decl com o tipo $3 */
        Decl *head = NULL, *tail = NULL;
        for (IdItem *it = $1; it; ) {
            Decl *d = new_decl(it->name, $3, it->is_array, it->size,
                               @1.first_line);
            if (!head) head = tail = d; else { tail->next = d; tail = d; }
            IdItem *tmp = it; it = it->next; free(tmp);
        }
        $$ = head;
    }
;

IdentList:
    IdentItem                                    { $$ = $1; }
  | IdentItem TOK_COMMA IdentList                { $1->next = $3; $$ = $1; }
;

IdentItem:
    TOK_IDENT                                    { $$ = make_iditem($1, 0, 0); }
    /* [PARTE 2 - NOVO] declaracao de VETOR com tamanho fixo: vet[10] */
  | TOK_IDENT TOK_LBRACK TOK_INTCONST TOK_RBRACK { $$ = make_iditem($1, 1, $3); }
;

Tipo:
    TOK_INT                                      { $$ = TYPE_INT; }
  | TOK_CAR                                      { $$ = TYPE_CAR; }
;

/* ---- Comandos ---------------------------------------------------------- */
ComandoList:
    /* vazio */                                  { $$ = NULL; }
  | Comando ComandoList                          { $1->next = $2; $$ = $1; }
;

Comando:
    TOK_SEMICOLON                                { $$ = new_stmt_empty(@1.first_line); }
    /* [PARTE 2 - NOVO] 'retorne expr;' so existe porque agora ha funcoes. */
  | TOK_RETORNE Expr TOK_SEMICOLON               { $$ = new_stmt_return($2, @1.first_line); }
  | TOK_LEIA TOK_IDENT TOK_SEMICOLON             { $$ = new_stmt_read($2, NULL, @1.first_line); }
    /* [PARTE 2 - NOVO] 'leia vet[i];' -- ler para um elemento de vetor. */
  | TOK_LEIA TOK_IDENT TOK_LBRACK Expr TOK_RBRACK TOK_SEMICOLON
                                                 { $$ = new_stmt_read($2, $4, @1.first_line); }
  | TOK_ESCREVA TOK_STRING TOK_SEMICOLON         { $$ = new_stmt_write_str($2, @1.first_line); }
  | TOK_ESCREVA Expr TOK_SEMICOLON               { $$ = new_stmt_write_expr($2, @1.first_line); }
  | TOK_NOVALINHA TOK_SEMICOLON                  { $$ = new_stmt_newline(@1.first_line); }
  | TOK_SE TOK_LPAREN Expr TOK_RPAREN TOK_ENTAO Comando TOK_FIMSE {
        $$ = new_stmt_if($3, $6, NULL, @1.first_line);
    }
  | TOK_SE TOK_LPAREN Expr TOK_RPAREN TOK_ENTAO Comando TOK_SENAO Comando TOK_FIMSE {
        $$ = new_stmt_if($3, $6, $8, @1.first_line);
    }
  | TOK_ENQUANTO TOK_LPAREN Expr TOK_RPAREN Comando {
        $$ = new_stmt_while($3, $5, @1.first_line);
    }
  | Bloco                                        { $$ = new_stmt_block($1, @1.first_line); }
  | Expr TOK_SEMICOLON                           { $$ = new_stmt_expr($1, @1.first_line); }
;

/* ---- Expressoes -------------------------------------------------------- */
Expr:
    Expr TOK_OU Expr          { $$ = new_expr_binary(OP_OR,  $1, $3, @2.first_line); }
  | Expr TOK_E Expr           { $$ = new_expr_binary(OP_AND, $1, $3, @2.first_line); }
  | Expr TOK_IGUAL Expr       { $$ = new_expr_binary(OP_EQ,  $1, $3, @2.first_line); }
  | Expr TOK_DIFERENTE Expr   { $$ = new_expr_binary(OP_NE,  $1, $3, @2.first_line); }
  | Expr TOK_LT Expr          { $$ = new_expr_binary(OP_LT,  $1, $3, @2.first_line); }
  | Expr TOK_GT Expr          { $$ = new_expr_binary(OP_GT,  $1, $3, @2.first_line); }
  | Expr TOK_MAIORIGUAL Expr  { $$ = new_expr_binary(OP_GE,  $1, $3, @2.first_line); }
  | Expr TOK_MENORIGUAL Expr  { $$ = new_expr_binary(OP_LE,  $1, $3, @2.first_line); }
  | Expr TOK_PLUS Expr        { $$ = new_expr_binary(OP_ADD, $1, $3, @2.first_line); }
  | Expr TOK_MINUS Expr       { $$ = new_expr_binary(OP_SUB, $1, $3, @2.first_line); }
  | Expr TOK_STAR Expr        { $$ = new_expr_binary(OP_MUL, $1, $3, @2.first_line); }
  | Expr TOK_SLASH Expr       { $$ = new_expr_binary(OP_DIV, $1, $3, @2.first_line); }
  | TOK_MINUS Expr %prec UMINUS { $$ = new_expr_unary(OP_NEG, $2, @1.first_line); }
  | TOK_BANG Expr             { $$ = new_expr_unary(OP_NOT, $2, @1.first_line); }
  | TOK_LPAREN Expr TOK_RPAREN { $$ = $2; }
  | TOK_INTCONST              { $$ = new_expr_int($1, @1.first_line); }
  | TOK_CHARCONST            { $$ = new_expr_char($1, @1.first_line); }
  | TOK_IDENT                 { $$ = new_expr_var($1, @1.first_line); }
    /* [PARTE 2 - NOVO] acesso a elemento de vetor: vet[i] */
  | TOK_IDENT TOK_LBRACK Expr TOK_RBRACK {
        $$ = new_expr_array($1, $3, @1.first_line);
    }
    /* [PARTE 2 - NOVO] chamada de funcao: nome(args) */
  | TOK_IDENT TOK_LPAREN ArgListOpt TOK_RPAREN {
        $$ = new_expr_call($1, $3, @1.first_line);
    }
  | TOK_IDENT TOK_ASSIGN Expr {
        $$ = new_expr_assign($1, NULL, $3, @1.first_line);     /* x = e     */
    }
    /* [PARTE 2 - NOVO] atribuicao a elemento de vetor: vet[i] = e */
  | TOK_IDENT TOK_LBRACK Expr TOK_RBRACK TOK_ASSIGN Expr {
        $$ = new_expr_assign($1, $3, $6, @1.first_line);       /* v[i] = e  */
    }
;

/* [PARTE 2 - NOVO] lista de argumentos de uma chamada de funcao (pode ser vazia) */
ArgListOpt:
    /* vazio */                                  { $$ = NULL; }
  | ArgList                                      { $$ = $1; }
;

ArgList:
    Expr                                         { $$ = new_arg($1); }
  | Expr TOK_COMMA ArgList {
        Arg *a = new_arg($1); a->next = $3; $$ = a;
    }
;

%%

/* Chamada pelo Bison quando encontra um erro sintatico. Usamos a localizacao
 * (yylloc) capturada pelo lexer para informar a linha. */
void yyerror(const char *s) {
    printf("ERRO SINTATICO (linha %d): %s\n", yylloc.first_line, s);
    exit(1);
}
