%{
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylex();
extern int yylineno;
extern char* yytext;
void yyerror(const char *s);

Program root_program;

/*
 * GUIA RÁPIDO (para quem nunca viu Bison):
 * - O parser recebe tokens do lexer (função yylex()).
 * - Cada "regra" abaixo diz como reconhecer partes da linguagem.
 * - Quando uma regra casa, o bloco { ... } executa e geralmente cria nós da AST.
 * - $1, $2, $3... = valores dos símbolos da regra (lado direito).
 * - $$ = valor produzido pela regra (lado esquerdo).
 *
 * Exemplo mental:
 *   Expr: Expr TOK_PLUS Expr { $$ = new_expr_binary(OP_ADD, $1, $3, yylineno); }
 * Significa:
 *   "se reconhecer expr + expr, crie um nó AST de soma com filho esquerdo e direito".
 */

/*
 * O que a estrutura faz: Nó auxiliar para acumular nomes de variáveis declaradas numa mesma linha.
 * Papel no Pipeline: Sintático (Apoio).
 * Regra da G-V1: Declarações múltiplas (ex: a, b: int;).
 * Dica para a Banca: "Isso salva a vida da recursão à esquerda no Bison, segurando os IDs em fila até bater no tipo final (int/car) para construir os decls da AST."
 */
typedef struct IdentNode {
    char *name;
    struct IdentNode *next;
} IdentNode;

%}

%union {
    int ival;
    int cval;
    char *sval;
    Expr *expr;
    Stmt *stmt;
    Block *block;
    Decl *decl;
    struct IdentNode *ident_list;
    int type;
}

%token TOK_PRINCIPAL TOK_INT TOK_CAR TOK_LEIA TOK_ESCREVA TOK_NOVALINHA
%token TOK_SE TOK_ENTAO TOK_SENAO TOK_FIMSE TOK_ENQUANTO
%token TOK_OU TOK_E TOK_IGUAL TOK_DIFERENTE TOK_MAIORIGUAL TOK_MENORIGUAL
%token TOK_LT TOK_GT TOK_PLUS TOK_MINUS TOK_STAR TOK_SLASH TOK_BANG TOK_ASSIGN
%token TOK_LBRACE TOK_RBRACE TOK_LPAREN TOK_RPAREN TOK_COLON TOK_SEMICOLON TOK_COMMA

%token <ival> TOK_INTCONST
%token <cval> TOK_CHARCONST
%token <sval> TOK_STRING
%token <sval> TOK_IDENT

%type <block> Block
%type <stmt> Comando ComandoList
%type <expr> Expr
%type <decl> VarDeclList VarDecl
%type <ident_list> IdentList
%type <type> Tipo

%left TOK_ASSIGN
%left TOK_OU
%left TOK_E
%left TOK_IGUAL TOK_DIFERENTE
%left TOK_LT TOK_GT TOK_MAIORIGUAL TOK_MENORIGUAL
%left TOK_PLUS TOK_MINUS
%left TOK_STAR TOK_SLASH
%right TOK_BANG UMINUS

%%

/*
 * Regra inicial da linguagem.
 * Lê a palavra-chave principal e um bloco completo.
 * Ao reduzir esta regra, a AST final fica guardada em root_program.
 */
Programa:
    TOK_PRINCIPAL Block {
        root_program.block = $2;
    }
;

/*
 * Bloco da linguagem.
 * Suporta duas formas:
 * 1) bloco com declarações + bloco de comandos
 * 2) bloco só com comandos
 * Cada forma produz um nó Block na AST.
 */
Block:
    TOK_LBRACE VarDeclList TOK_RBRACE TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block($2, $5, @1.first_line);
        $$->line = yylineno; // approximate
    }
  | TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block(NULL, $2, yylineno);
  }
;

/*
 * Lista de declarações de variáveis.
 * Faz encadeamento de Decl para representar quantidade arbitrária de linhas.
 */
VarDeclList:
    VarDecl { $$ = $1; }
  | VarDeclList VarDecl {
        Decl *tail = $1;
        while(tail->next) tail = tail->next;
        tail->next = $2;
        $$ = $1;
  }
;

/*
 * Declaração do tipo: a, b, c : int;
 * Passos:
 * 1) IdentList guarda os nomes temporariamente.
 * 2) Quando o tipo chega (Tipo), cria um Decl para cada nome.
 * 3) Resultado final: lista encadeada de Decl pronta para a AST.
 */
VarDecl:
    IdentList TOK_COLON Tipo TOK_SEMICOLON {
        Decl *head = NULL, *tail = NULL;
        IdentNode *curr = $1;
        while(curr) {
            Decl *d = new_decl(curr->name, $3, yylineno);
            if(!head) head = tail = d;
            else { tail->next = d; tail = d; }
            IdentNode *tmp = curr;
            curr = curr->next;
            free(tmp);
        }
        $$ = head;
    }
;

/*
 * Coleta identificadores separados por vírgula.
 * Esta estrutura é temporária: existe só até VarDecl converter em Decl.
 */
IdentList:
    TOK_IDENT {
        $$ = (IdentNode*)malloc(sizeof(IdentNode));
        $$->name = $1;
        $$->next = NULL;
    }
  | IdentList TOK_COMMA TOK_IDENT {
        IdentNode *n = (IdentNode*)malloc(sizeof(IdentNode));
        n->name = $3;
        n->next = NULL;
        IdentNode *tail = $1;
        while(tail->next) tail = tail->next;
        tail->next = n;
        $$ = $1;
  }
;

Tipo:
    TOK_INT { $$ = TYPE_INT; }
  | TOK_CAR { $$ = TYPE_CAR; }
;

/*
 * Lista de comandos.
 * Regra recursiva para aceitar zero ou mais comandos.
 * Caso vazio retorna NULL (bloco sem comandos).
 */
ComandoList:
    /* empty */ { $$ = NULL; }
  | Comando ComandoList {
        if ($1) {
            $1->next = $2;
            $$ = $1;
        } else {
            $$ = $2;
        }
  }
;

/*
 * Regras de comando da linguagem.
 * Cada alternativa gera um tipo de Stmt:
 * - leitura/escrita
 * - if / if-else
 * - while
 * - bloco aninhado
 * - expressão como comando (ex.: atribuição;)
 */
Comando:
    TOK_SEMICOLON { $$ = new_stmt_empty(yylineno); }
  | TOK_LEIA TOK_IDENT TOK_SEMICOLON { $$ = new_stmt_read($2, yylineno); }
  | TOK_ESCREVA TOK_STRING TOK_SEMICOLON { $$ = new_stmt_write_str($2, yylineno); }
  | TOK_ESCREVA Expr TOK_SEMICOLON { $$ = new_stmt_write_expr($2, yylineno); }
  | TOK_NOVALINHA TOK_SEMICOLON { $$ = new_stmt_newline(yylineno); }
  | TOK_SE TOK_LPAREN Expr TOK_RPAREN TOK_ENTAO Comando TOK_FIMSE {
        $$ = new_stmt_if($3, $6, NULL, yylineno);
  }
  | TOK_SE TOK_LPAREN Expr TOK_RPAREN TOK_ENTAO Comando TOK_SENAO Comando TOK_FIMSE {
        $$ = new_stmt_if($3, $6, $8, yylineno);
  }
  | TOK_ENQUANTO TOK_LPAREN Expr TOK_RPAREN Comando {
        $$ = new_stmt_while($3, $5, yylineno);
  }
  | Block {
        $$ = new_stmt_block($1, yylineno);
  }
  | Expr TOK_SEMICOLON {
        $$ = new_stmt_expr($1, yylineno);
  }
;

/*
 * Expressões da linguagem.
 * Precedência vem das diretivas %left/%right no topo do arquivo.
 * Pontos importantes para explicar:
 * - UMINUS resolve diferença entre "-x" e "a - b".
 * - Parênteses forçam agrupamento.
 * - Cada operação vira nó AST próprio (unário ou binário).
 */
Expr:
    TOK_IDENT { $$ = new_expr_var($1, yylineno); }
  | TOK_INTCONST { $$ = new_expr_int($1, yylineno); }
  | TOK_CHARCONST { $$ = new_expr_char($1, yylineno); }
  | TOK_LPAREN Expr TOK_RPAREN { $$ = $2; }
  | TOK_IDENT TOK_ASSIGN Expr { $$ = new_expr_assign($1, $3, yylineno); }
  | TOK_MINUS Expr %prec UMINUS { $$ = new_expr_unary(OP_NEG, $2, yylineno); }
  | TOK_BANG Expr { $$ = new_expr_unary(OP_NOT, $2, yylineno); }
  | Expr TOK_STAR Expr { $$ = new_expr_binary(OP_MUL, $1, $3, yylineno); }
  | Expr TOK_SLASH Expr { $$ = new_expr_binary(OP_DIV, $1, $3, yylineno); }
  | Expr TOK_PLUS Expr { $$ = new_expr_binary(OP_ADD, $1, $3, yylineno); }
  | Expr TOK_MINUS Expr { $$ = new_expr_binary(OP_SUB, $1, $3, yylineno); }
  | Expr TOK_LT Expr { $$ = new_expr_binary(OP_LT, $1, $3, yylineno); }
  | Expr TOK_GT Expr { $$ = new_expr_binary(OP_GT, $1, $3, yylineno); }
  | Expr TOK_MAIORIGUAL Expr { $$ = new_expr_binary(OP_GE, $1, $3, yylineno); }
  | Expr TOK_MENORIGUAL Expr { $$ = new_expr_binary(OP_LE, $1, $3, yylineno); }
  | Expr TOK_IGUAL Expr { $$ = new_expr_binary(OP_EQ, $1, $3, yylineno); }
  | Expr TOK_DIFERENTE Expr { $$ = new_expr_binary(OP_NE, $1, $3, yylineno); }
  | Expr TOK_E Expr { $$ = new_expr_binary(OP_AND, $1, $3, yylineno); }
  | Expr TOK_OU Expr { $$ = new_expr_binary(OP_OR, $1, $3, yylineno); }
;

%%

/*
 * Tratamento de erro sintático.
 * Bison chama yyerror quando a sequência de tokens não casa com a gramática.
 * Aqui o compilador emite erro padronizado com linha e encerra.
 */
void yyerror(const char *s) {
    extern int yylineno;
    printf("ERRO: %s %d\n", "SINTATICO", yylineno > 1 ? yylineno - 1 : 1);
    exit(1);
}
