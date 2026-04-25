#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantics.h"
#include "codegen.h"

extern int yyparse();
extern Program root_program;
extern FILE *yyin;

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_entrada.g> [arquivo_saida.asm]\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("ERRO: NAO FOI POSSIVEL ABRIR ARQUIVO\n");
        return 1;
    }
    yyin = f;

    //printf("Compilador G-V1 - Iniciando...\n");
    if (yyparse() == 0) {
        //printf("Analise Sintatica: SUCESSO\n");
        check_semantics(&root_program);
        //printf("Analise Semantica: SUCESSO\n");
        
        FILE* out = stdout;
        if (argc >= 3) {
            out = fopen(argv[2], "w");
            if (!out) {
                printf("ERRO: NAO FOI POSSIVEL ABRIR SAIDA\n");
                return 1;
            }
        }

        generate_code(&root_program, out);
        
        if (out != stdout) {
            fclose(out);
            //printf("Geracao de Codigo: SUCESSO\n");
        }
        
        // Note: AST memory freeing would go here in a production compiler.
    }
    
    fclose(f);
    return 0;
}
