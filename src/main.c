/* ============================================================================
 *  main.c  --  Driver (orquestrador) do compilador Cafezinho.
 *
 *  Liga todas as fases na ordem classica de um compilador:
 *
 *      texto-fonte
 *          | (analise lexica  -> Flex)
 *          | (analise sintatica -> Bison)  ..... constroi a AST
 *          v
 *         AST  --(analise semantica)-->  AST decorada + verificacoes
 *          |
 *          v
 *      codigo MIPS  (geracao de codigo)
 *
 *  Uso:   compilador <entrada.txt> [saida.asm]
 *  Se a saida nao for informada, o codigo MIPS sai na tela (stdout).
 * ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantic.h"
#include "codegen.h"

extern int yyparse(void);         /* gerado pelo Bison           */
extern FILE *yyin;                /* arquivo de entrada do Flex  */
extern Program root_program;      /* raiz da AST (definida no parser) */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_entrada> [arquivo_saida.asm]\n", argv[0]);
        return 1;
    }

    /* abre o arquivo-fonte e o entrega ao analisador lexico */
    FILE *in = fopen(argv[1], "r");
    if (!in) {
        printf("ERRO: nao foi possivel abrir o arquivo '%s'\n", argv[1]);
        return 1;
    }
    yyin = in;

    /* 1) e 2) analise lexica + sintatica (constroi a AST).
     *    Erros lexicos/sintaticos abortam dentro do lexer/parser. */
    if (yyparse() != 0) {
        fclose(in);
        return 1;
    }
    fclose(in);

    /* 3) analise semantica. Retorna o numero de erros encontrados. */
    int erros = check_semantics(&root_program);
    if (erros > 0) {
        printf("Compilacao abortada: %d erro(s) semantico(s).\n", erros);
        return 1;
    }

    /* 4) geracao de codigo MIPS (so chega aqui se nao houve erros) */
    FILE *out = stdout;
    if (argc >= 3) {
        out = fopen(argv[2], "w");
        if (!out) {
            printf("ERRO: nao foi possivel criar o arquivo de saida '%s'\n", argv[2]);
            return 1;
        }
    }

    generate_code(&root_program, out);

    if (out != stdout) fclose(out);
    return 0;
}
