#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantic.h"
#include "codegen.h"

extern int yyparse(void);
extern FILE *yyin;
extern Program root_program;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_entrada> [arquivo_saida.asm]\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) {
        printf("ERRO: nao foi possivel abrir o arquivo '%s'\n", argv[1]);
        return 1;
    }
    yyin = in;

    if (yyparse() != 0) {
        fclose(in);
        return 1;
    }
    fclose(in);

    int erros = check_semantics(&root_program);
    if (erros > 0) {
        printf("Compilacao abortada: %d erro(s) semantico(s).\n", erros);
        return 1;
    }

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
