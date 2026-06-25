%{
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int  yylex(void);
extern int  yylineno;
void yyerror(const char *s);

Program root_program;

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

%locations

%union {
    int    ival;
    int    cval;
    char  *sval;
    int    type;
    Expr  *expr;
    Arg   *arg;
    Stmt  *stmt;
    Block *block;
    Decl  *decl;
    Param *param;
    Func  *func;
    struct IdItem *iditem;
}

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

%type <decl>   Globais DeclVarList DeclVar
%type <func>   Funcoes FuncList Funcao
%type <param>  Params ParamList Param
%type <block>  Principal Bloco
%type <stmt>   ComandoList Comando
%type <expr>   Expr
%type <arg>    ArgListOpt ArgList
%type <iditem> IdentList IdentItem
%type <type>   Tipo

%right TOK_ASSIGN
%left  TOK_OU
%left  TOK_E
%left  TOK_IGUAL TOK_DIFERENTE
%left  TOK_LT TOK_GT TOK_MAIORIGUAL TOK_MENORIGUAL
%left  TOK_PLUS TOK_MINUS
%left  TOK_STAR TOK_SLASH
%right TOK_BANG UMINUS

%%

Programa:
    Globais Funcoes Principal {
        root_program.globals   = $1;
        root_program.functions = $2;
        root_program.main_block= $3;
    }
;

Globais:
    { $$ = NULL; }
  | TOK_GLOBAL TOK_LBRACK DeclVarList TOK_RBRACK { $$ = $3; }
;

Funcoes:
    { $$ = NULL; }
  | TOK_FUNCAO TOK_LBRACK FuncList TOK_RBRACK    { $$ = $3; }
;

Principal:
    TOK_PRINCIPAL Bloco                          { $$ = $2; }
;

FuncList:
    Funcao                                       { $$ = $1; }
  | Funcao FuncList                              { $1->next = $2; $$ = $1; }
;

Funcao:
    TOK_IDENT TOK_LPAREN Params TOK_RPAREN TOK_COLON Tipo Bloco {
        $$ = new_func($1, $3, $6, $7, @1.first_line);
    }
;

Params:
    { $$ = NULL; }
  | ParamList                                    { $$ = $1; }
;

ParamList:
    Param                                        { $$ = $1; }
  | Param TOK_COMMA ParamList                    { $1->next = $3; $$ = $1; }
;

Param:
    TOK_IDENT TOK_COLON Tipo {
        $$ = new_param($1, $3, 0, @1.first_line);
    }
  | TOK_IDENT TOK_LBRACK TOK_RBRACK TOK_COLON Tipo {
        $$ = new_param($1, $5, 1, @1.first_line);
    }
;

Bloco:
    TOK_LBRACK DeclVarList TOK_RBRACK TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block($2, $5, @1.first_line);
    }
  | TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block(NULL, $2, @1.first_line);
    }
;

DeclVarList:
    DeclVar                                      { $$ = $1; }
  | DeclVar DeclVarList {
        Decl *t = $1; while (t->next) t = t->next; t->next = $2; $$ = $1;
    }
;

DeclVar:
    IdentList TOK_COLON Tipo TOK_SEMICOLON {
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
  | TOK_IDENT TOK_LBRACK TOK_INTCONST TOK_RBRACK { $$ = make_iditem($1, 1, $3); }
;

Tipo:
    TOK_INT                                      { $$ = TYPE_INT; }
  | TOK_CAR                                      { $$ = TYPE_CAR; }
;

ComandoList:
    { $$ = NULL; }
  | Comando ComandoList                          { $1->next = $2; $$ = $1; }
;

Comando:
    TOK_SEMICOLON                                { $$ = new_stmt_empty(@1.first_line); }
  | TOK_RETORNE Expr TOK_SEMICOLON               { $$ = new_stmt_return($2, @1.first_line); }
  | TOK_LEIA TOK_IDENT TOK_SEMICOLON             { $$ = new_stmt_read($2, NULL, @1.first_line); }
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
  | TOK_IDENT TOK_LBRACK Expr TOK_RBRACK {
        $$ = new_expr_array($1, $3, @1.first_line);
    }
  | TOK_IDENT TOK_LPAREN ArgListOpt TOK_RPAREN {
        $$ = new_expr_call($1, $3, @1.first_line);
    }
  | TOK_IDENT TOK_ASSIGN Expr {
        $$ = new_expr_assign($1, NULL, $3, @1.first_line);
    }
  | TOK_IDENT TOK_LBRACK Expr TOK_RBRACK TOK_ASSIGN Expr {
        $$ = new_expr_assign($1, $3, $6, @1.first_line);
    }
;

ArgListOpt:
    { $$ = NULL; }
  | ArgList                                      { $$ = $1; }
;

ArgList:
    Expr                                         { $$ = new_arg($1); }
  | Expr TOK_COMMA ArgList {
        Arg *a = new_arg($1); a->next = $3; $$ = a;
    }
;

%%

void yyerror(const char *s) {
    printf("ERRO SINTATICO (linha %d): %s\n", yylloc.first_line, s);
    exit(1);
}
