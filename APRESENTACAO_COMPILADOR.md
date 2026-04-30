# Guia de Estudo e Apresentação do Compilador (passo a passo)

## 1) Objetivo do projeto em 1 frase
Este compilador lê um programa na linguagem G-V1 e transforma em código MIPS, passando por **análise léxica**, **sintática**, **semântica** e **geração de código**.

---

## 2) Roteiro de apresentação (10 a 15 minutos)

### Bloco A — Visão geral (1 min)
Fale isso:
1. “Meu compilador recebe um arquivo `.g`.”
2. “Converte o texto em estrutura de árvore (AST).”
3. “Valida regras de tipo e escopo.”
4. “Gera assembly MIPS no final.”

### Bloco B — Pipeline completo (3 min)
Use esta sequência obrigatória:
1. **Lexer (`src/lexer.l`)**: quebra texto em tokens (identificadores, números, símbolos).
2. **Parser (`src/parser.y`)**: valida gramática e monta AST.
3. **AST (`src/ast.c` / `src/ast.h`)**: representação estruturada do programa.
4. **Semântica (`src/semantics.c`)**: checa tipos e variáveis declaradas.
5. **Codegen (`src/codegen.c`)**: emite MIPS.
6. **Main (`src/main.c`)**: orquestra tudo.

### Bloco C — Decisões técnicas explicadas de forma defensável (4 min)

#### 1. “Por que usar AST?”
Resposta curta:
- Porque o parser só valida formato; a AST guarda a **estrutura lógica** para as fases seguintes.

Resposta técnica:
- A AST elimina detalhes sintáticos supérfluos e deixa nós úteis (atribuição, operação binária, if, while, bloco).
- Sem AST, semântica e geração de código ficariam acopladas ao parser e muito mais difíceis de manter.

#### 2. “Por que lista encadeada para comandos/declarações?”
- A gramática naturalmente produz sequências de tamanho variável.
- Lista encadeada simplifica montagem incremental durante parsing.
- Evita realocações frequentes de vetor dinâmico.

#### 3. “Por que pilha de escopos na semântica?”
- Blocos `{}` entram e saem: comportamento de pilha.
- Permite shadowing correto (variável interna pode esconder externa).
- Implementação simples e coerente com regras de escopo léxico.

#### 4. “É árvore binária ou ternária?”
- **Expressões binárias** (`+`, `-`, `*`, `/`, comparações, lógicos) usam dois filhos: esquerda e direita.
- **If** pode carregar 2 ou 3 partes lógicas: condição, ramo then e opcional else.
- Portanto, o projeto usa uma AST heterogênea: cada tipo de nó tem sua própria “aridade”.

### Bloco D — Demonstração ao vivo (3 a 5 min)
1. Mostre um programa de entrada curto.
2. Compile e gere `.asm`.
3. Aponte no output onde aparecem:
   - operação aritmética,
   - desvio condicional,
   - laço.
4. Mostre 1 erro semântico proposital (ex.: variável não declarada).

---

## 3) Mapa mental para estudar de verdade

## Etapa 1 — Entender o fluxo de execução (base)
Abra `src/main.c` e memorize:
1. Abre arquivo.
2. Chama `yyparse()`.
3. Se parser ok, chama `check_semantics()`.
4. Se semântica ok, chama `generate_code()`.

Meta: você precisa conseguir desenhar esse fluxo sem olhar código.

## Etapa 2 — Entender os nós da AST
Abra `src/ast.h` e `src/ast.c` e identifique:
- tipos de expressão (`EX_INT`, `EX_VAR`, `EX_BINARY`, etc.),
- tipos de comando (`ST_IF`, `ST_WHILE`, `ST_BLOCK`, etc.).

Meta: dado um trecho fonte, dizer quais nós serão criados.

## Etapa 3 — Entender semântica (onde professor mais pressiona)
Em `src/semantics.c`, foque em:
1. `push_scope` / `pop_scope`;
2. `lookup_scope` e `lookup_all`;
3. `analyze_expr`;
4. `analyze_stmt`.

Meta: explicar 3 erros detectados automaticamente:
- variável não declarada,
- redeclaração no mesmo escopo,
- operação com tipos incompatíveis.

## Etapa 4 — Entender geração de código
Em `src/codegen.c`, encontre:
- geração para expressão binária,
- geração para if,
- geração para while,
- syscalls de entrada/saída.

Meta: explicar como um `while` vira rótulos + branch + jump.

---

## 4) Perguntas difíceis e respostas prontas

### “Como você garante escopo correto?”
“Cada bloco cria um escopo novo com push e ao sair faz pop. A busca por variáveis começa no escopo atual e sobe para os anteriores; isso implementa shadowing naturalmente.”

### “Como verifica tipos?”
“`analyze_expr` faz checagem recursiva. Primeiro infere filhos, depois valida operador. Se tipos não batem, aborta com erro semântico e linha.”

### “Por que abortar no primeiro erro?”
“Decisão de projeto para simplificar o compilador didático: evita cascata de erros derivados e facilita depuração da fase atual.”

### “Quais limitações atuais?”
- Tipos básicos restritos (int/car).
- Estratégia simples de erros (fail-fast).
- Sem otimizações avançadas.

---

## 5) Plano de treino (2 dias)

### Dia 1
1. Ler `main.c` + `semantics.c`.
2. Montar fala de 5 min sem slides.
3. Executar 3 casos: válido, tipo inválido, identificador não declarado.

### Dia 2
1. Ler `codegen.c` com foco em if/while.
2. Fazer apresentação simulada de 10 min.
3. Gravar áudio e revisar pontos de hesitação.

---

## 6) Script de fala inicial (pode decorar)
“Professor, meu compilador segue o pipeline clássico: lexer, parser, AST, semântica e geração de código MIPS. O parser garante forma sintática; a semântica garante regras de tipo e escopo com pilha de símbolos; e só depois disso o backend gera assembly. A principal decisão de arquitetura foi separar essas fases para manter corretude e facilitar manutenção.”

---

## 7) Checklist final antes de apresentar
- [ ] Consigo explicar o pipeline sem ler.
- [ ] Sei justificar AST + pilha de escopos.
- [ ] Sei mostrar 1 erro semântico e explicar por que ocorreu.
- [ ] Sei apontar no MIPS onde está if/while.
- [ ] Tenho resposta para limitações e próximos passos.
