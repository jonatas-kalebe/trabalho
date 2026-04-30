# Guia Linha a Linha do Código (didático para quem está começando do zero)

> Este guia complementa `APRESENTACAO_COMPILADOR.md` e foca em **como ler o código-fonte**: o que cada bloco faz, por que existe e qual pergunta de professor ele responde.

---

## 1) Convenções de leitura em C (para não se perder)

- `#include ...`: importa bibliotecas/headers.
- `typedef struct ...`: define estruturas de dados.
- `static`: função visível só no arquivo atual (escopo interno do módulo).
- ponteiro `*`: variável que guarda endereço de memória.
- `->`: acesso a campo de struct via ponteiro.

### Símbolos que sempre confundem iniciantes
- `;` termina instrução.
- `{ ... }` delimita bloco de código.
- `(...)` agrupa parâmetros/expressões.
- `==` compara igualdade; `=` atribui valor.
- `&&` E lógico; `||` OU lógico.

---

## 2) `src/main.c` — Orquestração do compilador

## O fluxo é este (memorize):
1. valida argumentos da linha de comando;
2. abre arquivo de entrada;
3. chama `yyparse()`;
4. se parser ok -> `check_semantics()`;
5. se semântica ok -> `generate_code()`.

### Pergunta de professor
**“Por que separar em fases?”**
Resposta: para garantir correção incremental e isolamento de responsabilidades.

---

## 3) `src/parser.y` — Coração da análise sintática (Bison)

## Como ler o arquivo
1. Bloco `%{ ... %}`: código C auxiliar e includes.
2. `%union`: tipos de valores semânticos.
3. `%token`: tokens vindos do lexer.
4. `%type`: tipo de cada não-terminal.
5. `%left/%right`: precedência/associatividade.
6. `%% ... %%`: regras gramaticais.

## Sobre `$1`, `$2`, `$$`
- `$1`: primeiro item da produção.
- `$2`: segundo item.
- `$$`: resultado produzido por aquela regra.

Exemplo:
`Expr: Expr TOK_PLUS Expr { $$ = new_expr_binary(OP_ADD, $1, $3, yylineno); }`

Significa: reconheci “expr + expr”; produzo um novo nó AST de soma.

## Regras-chave que você deve saber explicar
- `Programa`: regra inicial.
- `Block`: forma bloco com declarações e comandos.
- `VarDecl`: converte `a,b,c:int;` em lista de `Decl`.
- `Comando`: if/while/leia/escreva/expr;
- `Expr`: aritmética, comparação e lógica com precedência.

### Sobre `%prec UMINUS`
Resolve ambiguidade entre:
- unário: `-x`
- binário: `a - b`

---

## 4) `src/ast.h` + `src/ast.c` — Modelo da árvore do programa

## O que é essencial entender
A AST é o “formato interno” do programa.
- Parser cria nós.
- Semântica valida nós.
- Codegen traduz nós.

## Famílias de nós
- `Expr`: expressões (int, char, variável, unária, binária, atribuição).
- `Stmt`: comandos (if, while, block, read, write, etc.).
- `Decl`: declarações de variável.
- `Block`: conjunto de declarações + comandos.

### Pergunta clássica
**“A árvore é binária?”**
Resposta: alguns nós são binários (`Expr` binária), outros têm aridade diferente (`if` possui cond/then/else opcional).

---

## 5) `src/semantics.c` — Significado correto do programa

## O que a semântica impede
- uso de variável não declarada;
- redeclaração no mesmo escopo;
- operações com tipos incompatíveis;
- condição inválida em if/while.

## Estruturas principais
- `Sym`: símbolo (nome, tipo, offset/size).
- `Scope`: escopo atual + link para escopo pai.
- `SemanticCtx`: topo da pilha de escopos.

## Funções obrigatórias para decorar
- `push_scope` / `pop_scope`.
- `lookup_scope` / `lookup_all`.
- `analyze_expr` / `analyze_stmt`.

---

## 6) `src/codegen.c` — Tradução para MIPS

## Mentalidade correta
Codegen não “entende texto”; ele “entende AST já validada”.

## Responsabilidades
1. mapear variáveis para offsets de pilha;
2. emitir instruções para expressões;
3. emitir labels e branches para fluxo (`if`, `while`);
4. preparar `.data` para strings.

## Estruturas úteis
- `CGSym`: símbolo no backend.
- `CGScope`: escopo para alocação na stack.
- `CodegenCtx`: estado global da emissão.

## Sobre labels
`new_label` garante rótulos únicos (`if_else_3`, `while_start_7`, etc.) para não haver colisão em aninhamento.

---

## 7) `src/lexer.l` — Tokenização (entrada do parser)

O lexer transforma caracteres em tokens como:
- palavras-chave,
- identificadores,
- números,
- operadores,
- delimitadores.

Sem lexer, parser não recebe tokens e não opera.

---

## 8) Mini roteiro “decore e entregue”

1. “Meu compilador vai de G-V1 para MIPS em 4 fases.”
2. “Parser Bison monta AST via ações semânticas.”
3. “Semântica usa pilha de escopos e tabela de símbolos.”
4. “Codegen percorre AST validada e emite assembly.”
5. “Escolhi arquitetura por fases para separar responsabilidades e facilitar evolução.”

