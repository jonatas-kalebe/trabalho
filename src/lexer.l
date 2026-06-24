/* ============================================================================
 *  lexer.l  --  Analisador LEXICO (scanner) da linguagem Cafezinho, em Flex.
 *
 *  CONCEITO (1a fase do compilador):
 *  ---------------------------------
 *  O analisador lexico le o texto-fonte caractere a caractere e o agrupa em
 *  TOKENS (as "palavras" da linguagem): palavras-reservadas, identificadores,
 *  numeros, operadores, etc. Ele entrega esses tokens, um a um, ao analisador
 *  sintatico (parser.y). Tambem descarta espacos e comentarios.
 *
 *  O Flex transforma as expressoes regulares abaixo em um AUTOMATO FINITO que
 *  reconhece cada padrao usando a regra do "maior casamento" (maximal munch).
 * ==========================================================================*/
%option noyywrap
%option yylineno
%option nounistd
%option never-interactive

%{
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"   /* nomes dos tokens (TOK_*), gerados pelo Bison    */

/* A cada token casado, registramos a linha em yylloc para o parser usar nas
 * mensagens de erro (rastreamento de localizacao). */
#define YY_USER_ACTION  yylloc.first_line = yylloc.last_line = yylineno;

void yyerror(const char *s);
%}

%%

 /* ---- Palavras reservadas ---------------------------------------------- */
"global"    { return TOK_GLOBAL; }
"funcao"    { return TOK_FUNCAO; }
"principal" { return TOK_PRINCIPAL; }
"se"        { return TOK_SE; }
"entao"     { return TOK_ENTAO; }
"senao"     { return TOK_SENAO; }
"fimse"     { return TOK_FIMSE; }
"enquanto"  { return TOK_ENQUANTO; }
"retorne"   { return TOK_RETORNE; }
"leia"      { return TOK_LEIA; }
"escreva"   { return TOK_ESCREVA; }
"novalinha" { return TOK_NOVALINHA; }
"int"       { return TOK_INT; }
"car"       { return TOK_CAR; }

 /* ---- Operadores (os de 2 caracteres vem ANTES dos de 1) --------------- */
"||"        { return TOK_OU; }
"&&"        { return TOK_E; }
"=="        { return TOK_IGUAL; }
"!="        { return TOK_DIFERENTE; }
">="        { return TOK_MAIORIGUAL; }
"<="        { return TOK_MENORIGUAL; }
"<"         { return TOK_LT; }
">"         { return TOK_GT; }
"+"         { return TOK_PLUS; }
"-"         { return TOK_MINUS; }
"*"         { return TOK_STAR; }
"/"         { return TOK_SLASH; }
"!"         { return TOK_BANG; }
"="         { return TOK_ASSIGN; }

 /* ---- Delimitadores ---------------------------------------------------- */
"{"         { return TOK_LBRACE; }
"}"         { return TOK_RBRACE; }
"("         { return TOK_LPAREN; }
")"         { return TOK_RPAREN; }
"["         { return TOK_LBRACK; }
"]"         { return TOK_RBRACK; }
":"         { return TOK_COLON; }
";"         { return TOK_SEMICOLON; }
","         { return TOK_COMMA; }

 /* ---- Constante inteira ------------------------------------------------ */
[0-9]+      {
    yylval.ival = atoi(yytext);
    return TOK_INTCONST;
}

 /* ---- Constante caractere: 'a' ou escapes como '\n' -------------------- */
\'([^\'\\]|\\.)\' {
    if (yytext[1] == '\\') {
        switch (yytext[2]) {
            case 'n': yylval.cval = '\n'; break;
            case 't': yylval.cval = '\t'; break;
            case 'r': yylval.cval = '\r'; break;
            case '0': yylval.cval = '\0'; break;
            case '\\': yylval.cval = '\\'; break;
            case '\'': yylval.cval = '\''; break;
            default:  yylval.cval = yytext[2]; break;
        }
    } else {
        yylval.cval = yytext[1];
    }
    return TOK_CHARCONST;
}

 /* ---- Cadeia de caracteres (string) numa unica linha ------------------- */
\"[^\"\n]*\" {
    /* copia sem as aspas */
    yylval.sval = xstrdup(yytext + 1);
    yylval.sval[strlen(yylval.sval) - 1] = '\0';
    return TOK_STRING;
}

 /* ---- Identificador (variavel/funcao) ---------------------------------- */
[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.sval = xstrdup(yytext);
    return TOK_IDENT;
}

 /* ---- Espacos e quebras de linha (ignorados) --------------------------- */
[ \t\r]+    { /* ignora espacos em branco */ }
\n          { /* yylineno e atualizado automaticamente pelo Flex */ }

 /* ---- Comentarios de bloco -------------------------------------------- */
"/*"([^*]|(\*+[^*/]))*\*+"/"   { /* ignora comentario completo */ }

 /* ---- Tratamento de erros lexicos ------------------------------------- */
\"[^\"\n]*\n {
    printf("ERRO LEXICO (linha %d): cadeia de caracteres ocupa mais de uma linha\n", yylineno);
    exit(1);
}
"/*"([^*]|(\*+[^*/]))*  {
    printf("ERRO LEXICO (linha %d): comentario de bloco nao foi fechado\n", yylineno);
    exit(1);
}
.           {
    printf("ERRO LEXICO (linha %d): caractere invalido '%s'\n", yylineno, yytext);
    exit(1);
}

%%
