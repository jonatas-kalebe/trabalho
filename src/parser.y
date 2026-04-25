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

// We need a structure for IdentList to build declarations
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

Programa:
    TOK_PRINCIPAL Block {
        root_program.block = $2;
    }
;

Block:
    TOK_LBRACE VarDeclList TOK_RBRACE TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block($2, $5, @1.first_line); // line number logic needs adaptation if not using locations, we use yylineno for simplicity or just 0
        $$->line = yylineno; // approximate
    }
  | TOK_LBRACE ComandoList TOK_RBRACE {
        $$ = new_block(NULL, $2, yylineno);
  }
;

VarDeclList:
    VarDecl { $$ = $1; }
  | VarDeclList VarDecl {
        Decl *tail = $1;
        while(tail->next) tail = tail->next;
        tail->next = $2;
        $$ = $1;
  }
;

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

extern int token_line;

void yyerror(const char *s) {
    printf("ERRO: %s %d\n", "SINTATICO", token_line);
    exit(1);
}
