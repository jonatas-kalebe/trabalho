#include <stdio.h>
#include "ast.h"

/*
 * O que o método faz: Encerra o compilador relatando falha em alocação de memória.
 * Papel no Pipeline: Auxiliar / Árvore (AST).
 * Regra da G-V1: A gestão de memória usa uma AST compacta e exige resiliência.
 * Dica para a Banca: "Usamos um modelo fail-fast que corta a execução imediatamente se o heap do sistema esgotar, evitando segmentation faults ocultos."
 */
void die_alloc(void) {
    fprintf(stderr, "ERRO: FALHA DE MEMORIA\n");
    exit(1);
}

/*
 * O que o método faz: Copia de forma segura uma string (lexema) para uma nova área de memória.
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Isolar o ciclo de vida do buffer do Lexer das strings alocadas na árvore.
 * Dica para a Banca: "Garante que não dependemos das referências efêmeras do Flex na nossa AST permanente."
 */
char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *r = (char *)malloc(n + 1);
    if (!r) die_alloc();
    memcpy(r, s, n + 1);
    return r;
}

/*
 * O que o método faz: Aloca dinamicamente um nó genérico da árvore de expressões.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: A gestão de memória usa uma AST compacta (função central de alocação de expressões).
 * Dica para a Banca: "Centralizar o malloc de expressões com calloc garante que flags e uniões lixo sejam perfeitamente zeradas."
 */
static Expr *alloc_expr(ExprKind k, int line) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    if (!e) die_alloc();
    e->kind = k;
    e->line = line;
    e->inferred_type = TYPE_INT;
    return e;
}

/*
 * O que o método faz: Retorna um novo nó representando uma variável.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: O controle de escopo precisará deste identificador na tabela depois.
 * Dica para a Banca: "Armazenar o identificador via union minimiza overhead na AST."
 */
Expr *new_expr_var(char *name, int line) {
    Expr *e = alloc_expr(EX_VAR, line);
    e->as.name = name;
    return e;
}

/*
 * O que o método faz: Cria um nó primitivo na AST para lidar com literais do tipo inteiro.
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Tipo base de literais inteiros da G-V1.
 * Dica para a Banca: "Guardamos o inteiro diretamente no nó em vez de alocar um objeto extra, tornando o footprint menor."
 */
Expr *new_expr_int(int value, int line) {
    Expr *e = alloc_expr(EX_INT, line);
    e->as.int_value = value;
    return e;
}

/*
 * O que o método faz: Cria um nó primitivo na AST para literal de caractere.
 * Papel no Pipeline: Léxico -> Sintático -> Árvore (AST).
 * Regra da G-V1: Tipo base de literais caracteres da G-V1.
 * Dica para a Banca: "Tratamos car como tipos base já prevendo otimizações em memória (ocupando só os bits necessários)."
 */
Expr *new_expr_char(int value, int line) {
    Expr *e = alloc_expr(EX_CHAR, line);
    e->as.char_value = value;
    return e;
}

/*
 * O que o método faz: Fabrica nó da AST correspondente a uma atribuição (ex: a = b + 1).
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Semântico.
 * Regra da G-V1: Construção da l-value para o gerador e update no escopo.
 * Dica para a Banca: "Separamos l-value do r-value claramente na AST para que a checagem semântica descubra rápido a variável de destino."
 */
Expr *new_expr_assign(char *name, Expr *value, int line) {
    Expr *e = alloc_expr(EX_ASSIGN, line);
    e->as.assign.name = name;
    e->as.assign.value = value;
    return e;
}

/*
 * O que o método faz: Anexa dois nós operandos (left, right) a uma operação binária.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Implementação hierárquica das precedências aritméticas do parser Bison.
 * Dica para a Banca: "Cria a ramificação da árvore. O gerador de código usará isso para saber o que desempilhar depois de avaliar os dois lados."
 */
Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line) {
    Expr *e = alloc_expr(EX_BINARY, line);
    e->as.bin.op = op;
    e->as.bin.left = left;
    e->as.bin.right = right;
    return e;
}

/*
 * O que o método faz: Empacota um nó operando com um modificador lógico ou sinal.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Aplicação de modificadores sobre expressões existentes (como negativo unário).
 * Dica para a Banca: "Utiliza um layout unificado que suporta escalabilidade caso a G-V1 expanda seus operadores unários no futuro."
 */
Expr *new_expr_unary(OpKind op, Expr *expr, int line) {
    Expr *e = alloc_expr(EX_UNARY, line);
    e->as.un.op = op;
    e->as.un.expr = expr;
    return e;
}

/*
 * O que o método faz: Inicializa as meta-informações de uma nova declaração formal de variável.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Semântico.
 * Regra da G-V1: O controle de escopo deve respeitar variáveis locais; isto modela a variável.
 * Dica para a Banca: "Nós deixamos `next` pronto (como ponteiro nulo inicial), facilitando compor uma LinkedList simples de variáveis declaradas."
 */
Decl *new_decl(char *name, Type type, int line) {
    Decl *d = (Decl *)calloc(1, sizeof(Decl));
    if (!d) die_alloc();
    d->name = name;
    d->type = type;
    d->line = line;
    d->next = NULL;
    return d;
}

/*
 * O que o método faz: Aloca genericamente um bloco de Comando (Statement) vazio/virgem na memória.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: A gestão de memória usa uma AST compacta para ramificações lógicas.
 * Dica para a Banca: "Esta função é o coração do polimorfismo dos comandos, garantindo zero garbage data antes da sub-alocação dos statements específicos."
 */
static Stmt *alloc_stmt(StmtKind kind, int line) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    if (!s) die_alloc();
    s->kind = kind;
    s->line = line;
    s->next = NULL;
    return s;
}

/*
 * O que o método faz: Cria um placeholder para comandos vazios (ex. `;` extra).
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Resiliência contra excesso de delimitadores sem perder controle de linha.
 * Dica para a Banca: "Apoia o parser sem inflar o pipeline do code-generator indevidamente com NOPs."
 */
Stmt *new_stmt_empty(int line) {
    return alloc_stmt(ST_EMPTY, line);
}

/*
 * O que o método faz: Promove uma expressão a um statement, para suportar execução procedural de cálculos isolados.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Permite que operações como atribuição entrem num bloco de forma autônoma.
 * Dica para a Banca: "Essa ponte entre expressão e comando possibilita que os side-effects sejam executados de forma sequencial no array de instruções MIPS."
 */
Stmt *new_stmt_expr(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_EXPR, line);
    s->as.expr = expr;
    return s;
}

/*
 * O que o método faz: Prepara o nó sintático que disparará o sys call MIPS de leitura.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Interface de entrada de dados interativa.
 * Dica para a Banca: "Criamos a estrutura preparatória que fará o link com a syscall read_int do SPIM futuramente."
 */
Stmt *new_stmt_read(char *name, int line) {
    Stmt *s = alloc_stmt(ST_READ, line);
    s->as.name = name;
    return s;
}

/*
 * O que o método faz: Monta um nó para exibição condicional do resultado de expressões no console.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Comando 'escreva' atrelado a processamento.
 * Dica para a Banca: "Separa e delega ao filho (a expr) toda a carga da matemática antes da efetiva syscall de print."
 */
Stmt *new_stmt_write_expr(Expr *expr, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_EXPR, line);
    s->as.expr = expr;
    return s;
}

/*
 * O que o método faz: Nós para printagem de literais de string (mensagens diretas).
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS (seção .data).
 * Regra da G-V1: Escrever constantes estáticas (label base).
 * Dica para a Banca: "O nó reserva espaço antecipado para a label `.data` que vai abrigar a string no backend MIPS."
 */
Stmt *new_stmt_write_str(char *text, int line) {
    Stmt *s = alloc_stmt(ST_WRITE_STR, line);
    s->as.write_str.text = text;
    s->as.write_str.label = NULL;
    return s;
}

/*
 * O que o método faz: Gera a instrução para quebrar a linha no output padrão.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Formatação de I/O de acordo com o comando 'novalinha'.
 * Dica para a Banca: "Deixamos isolado da escrita de variáveis, o que facilita emitir um macro enxuto no MIPS (print char '\n')."
 */
Stmt *new_stmt_newline(int line) {
    return alloc_stmt(ST_NEWLINE, line);
}

/*
 * O que o método faz: Aloca a bifurcação de controle de fluxo condicional com ou sem ELSE.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Estruturas de decisão lógicas ('se').
 * Dica para a Banca: "Unimos IF e IF/ELSE em um mesmo tipo de nó. Se o then_branch existe, a branch no MIPS salta para ele, e se o else_branch for NULL, ele vira um jump neutro."
 */
Stmt *new_stmt_if(Expr *cond, Stmt *then_branch, Stmt *else_branch, int line) {
    Stmt *s = alloc_stmt(ST_IF, line);
    s->as.if_stmt.cond = cond;
    s->as.if_stmt.then_branch = then_branch;
    s->as.if_stmt.else_branch = else_branch;
    return s;
}

/*
 * O que o método faz: Empacota a estrutura cíclica contendo Condição e Corpo a ser repetido.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Gerador de Código MIPS.
 * Regra da G-V1: Representar o comando 'enquanto' da G-V1 com desvios (branchs).
 * Dica para a Banca: "Na AST isso é enxuto, mas em MIPS virará um rótulo no topo (para teste) e um jump no fim do corpo do loop voltando ao teste."
 */
Stmt *new_stmt_while(Expr *cond, Stmt *body, int line) {
    Stmt *s = alloc_stmt(ST_WHILE, line);
    s->as.while_stmt.cond = cond;
    s->as.while_stmt.body = body;
    return s;
}

/*
 * O que o método faz: Cria um sub-comando que encapsula um Bloco interior completo.
 * Papel no Pipeline: Sintático -> Árvore (AST) -> Semântico.
 * Regra da G-V1: Controle de sub-escopos permitindo shadowing modular.
 * Dica para a Banca: "Ao invés de estourar todos os comandos na flat list, mantemos os blocks nested, garantindo a sanidade na pilha da tabela de símbolos."
 */
Stmt *new_stmt_block(Block *block, int line) {
    Stmt *s = alloc_stmt(ST_BLOCK, line);
    s->as.block = block;
    return s;
}

/*
 * O que o método faz: Combina lista de declarações de topo de bloco com os comandos executáveis.
 * Papel no Pipeline: Sintático -> Árvore (AST).
 * Regra da G-V1: Unidade básica de organização e escopo do G-V1.
 * Dica para a Banca: "Isola formalmente o preâmbulo declarativo dos statements dinâmicos. Quando formos emitir código, é fácil calcular os offsets locais do $fp antes das lógicas."
 */
Block *new_block(Decl *decls, Stmt *commands, int line) {
    Block *b = (Block *)calloc(1, sizeof(Block));
    if (!b) die_alloc();
    b->decls = decls;
    b->commands = commands;
    b->line = line;
    return b;
}
