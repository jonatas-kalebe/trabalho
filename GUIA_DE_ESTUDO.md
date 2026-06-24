# Guia de Estudo — Compilador Cafezinho

Este guia explica **os conceitos de compiladores** por trás de cada parte do
código. A ideia é que você consiga **apresentar e defender** o projeto: para
cada fase há a teoria, onde ela está no código e as **perguntas típicas do
professor** com respostas.

---

## Índice

1. [Visão geral: as fases de um compilador](#1-visão-geral-as-fases-de-um-compilador)
2. [Fase 1 — Análise Léxica (Flex)](#2-fase-1--análise-léxica-flex)
3. [Fase 2 — Análise Sintática (Bison) e a AST](#3-fase-2--análise-sintática-bison-e-a-ast)
4. [Fase 3 — Análise Semântica e Tabela de Símbolos](#4-fase-3--análise-semântica-e-tabela-de-símbolos)
5. [Fase 4 — Geração de Código e a Máquina de Pilha](#5-fase-4--geração-de-código-e-a-máquina-de-pilha)
6. [Ambiente de Execução: Registros de Ativação](#6-ambiente-de-execução-registros-de-ativação)
7. [Mapa: qual conceito está em qual arquivo](#7-mapa-qual-conceito-está-em-qual-arquivo)
8. [Banco de perguntas do professor](#8-banco-de-perguntas-do-professor)

---

## 1. Visão geral: as fases de um compilador

Um compilador é dividido em **front-end** (entende o programa-fonte) e
**back-end** (produz o código de máquina).

```
            FRONT-END                                 BACK-END
 ┌──────────┬───────────┬───────────┐      ┌────────────────────────┐
 │  Léxica  │ Sintática │ Semântica │  →   │  Geração de Código     │
 │ (tokens) │  (AST)    │ (tipos,   │      │  (MIPS / máq. de pilha)│
 │          │           │  escopos) │      │                        │
 └──────────┴───────────┴───────────┘      └────────────────────────┘
      lexer.l   parser.y   semantic.c            codegen.c
```

- O **front-end** garante que o programa respeita as regras da linguagem.
- A "cola" entre front-end e back-end é a **Representação Intermediária (RI)**.
  Neste projeto, a RI é a **AST** (árvore sintática abstrata) — `ast.h`.
- **Não** implementamos otimização de código (faz parte do back-end de
  compiladores de produção, mas foi omitida por ser um projeto didático).

**Pergunta clássica:** *“Quais são as fases do seu compilador?”*
→ Léxica, sintática e semântica (front-end), e geração de código (back-end),
ligadas pela AST.

---

## 2. Fase 1 — Análise Léxica (Flex)

**Arquivo:** `src/lexer.l`

### O que é
O analisador léxico (ou *scanner*) lê o texto **caractere a caractere** e o
agrupa em **tokens** — as “palavras” da linguagem (palavras reservadas,
identificadores, números, operadores, delimitadores). Ele também descarta
espaços e comentários.

### Como funciona (a teoria)
- Cada padrão de token é descrito por uma **expressão regular** (ER).
  Ex.: identificador = `[a-zA-Z_][a-zA-Z0-9_]*`.
- O Flex converte essas ERs em um **Autômato Finito Determinístico (AFD)**,
  que reconhece os tokens muito rapidamente.
- Regra do **maior casamento (*maximal munch*)**: o scanner sempre pega a
  maior sequência que casa. Por isso `>=` é um único token, e não `>` seguido
  de `=`. (No `lexer.l`, os operadores de 2 caracteres vêm **antes** dos de 1.)
- Em empate de tamanho, vence a regra escrita **primeiro** no arquivo. Por isso
  as palavras reservadas (`se`, `int`, …) vêm antes da regra de identificador.

### Detalhes do nosso lexer
- `%option yylineno` mantém o número da linha atual (para mensagens de erro).
- `YY_USER_ACTION` grava a linha de cada token em `yylloc` (rastreamento de
  localização), que o parser usa nas mensagens.
- Erros léxicos tratados: caractere inválido, string em mais de uma linha,
  comentário de bloco não fechado.

**Perguntas do professor:**
- *“O que é maximal munch?”* → o scanner casa a maior cadeia possível.
- *“Como você diferencia `<` de `<=`?”* → regra do `<=` aparece antes/é maior.
- *“Palavra reservada e identificador têm a mesma ER. Como resolve?”* → a regra
  da palavra reservada vem **antes** no arquivo, então tem prioridade.

---

## 3. Fase 2 — Análise Sintática (Bison) e a AST

**Arquivos:** `src/parser.y`, `src/ast.h`, `src/ast.c`

### O que é
O analisador sintático (*parser*) recebe os tokens e verifica se eles formam
uma estrutura válida segundo a **gramática livre de contexto** da linguagem.
Ao mesmo tempo, ele **constrói a AST**.

### Como funciona (a teoria)
- O Bison gera um parser **LALR(1)**: análise **bottom-up** (ascendente) com
  **1 token de lookahead**.
- O parser faz duas ações: **shift** (empilha o próximo token) e **reduce**
  (quando reconhece o lado direito de uma regra, troca-o pelo não-terminal e
  executa a **ação semântica** em C — que cria o nó da AST).
- Por isso a AST é montada **de baixo para cima**: primeiro as folhas
  (constantes, variáveis), depois as expressões, comandos, blocos e funções.

### Precedência e associatividade
Em vez de escrever uma gramática gigante para impor precedência, declaramos:

```
%right TOK_ASSIGN          (menor precedência)
%left  TOK_OU
%left  TOK_E
...
%right TOK_BANG UMINUS     (maior precedência)
```

Assim `2 + 3 * 4` vira `2 + (3 * 4)` automaticamente, e `a = b = c` associa à
direita. Isso resolve as ambiguidades sem inflar a gramática.

### Por que não há o problema do *dangling else*?
Porque a linguagem usa **`fimse`** para fechar o `se`. Sem um terminador, um
`else` solto seria ambíguo (a que `if` ele pertence?). Com `fimse`, a gramática
fica **não-ambígua** — nosso parser tem **0 conflitos** shift/reduce.

### A AST
Cada construção vira um `struct` (`ast.h`):
- `Expr` — expressões (usa *tagged union*: um `kind` diz qual variante está
  ativa, e a `union` economiza memória).
- `Stmt` — comandos (`se`, `enquanto`, atribuição, `leia`, `escreva`…).
- `Decl`, `Param`, `Block`, `Func`, `Program` — estrutura do programa.

**Perguntas do professor:**
- *“LALR é top-down ou bottom-up?”* → bottom-up (shift-reduce).
- *“O que é uma redução?”* → reconhecer o lado direito de uma regra e
  substituí-lo pelo não-terminal, executando a ação que monta o nó.
- *“Como você trata precedência de operadores?”* → com `%left`/`%right`.
- *“Por que usar uma AST em vez de trabalhar direto com o texto?”* → porque a
  AST captura a hierarquia do programa e é percorrida facilmente pelas fases
  seguintes.

---

## 4. Fase 3 — Análise Semântica e Tabela de Símbolos

**Arquivos:** `src/symbol.h/.c`, `src/semantic.h/.c`

### O que é
A análise semântica verifica regras que a gramática **não consegue** expressar:
- variável usada sem ter sido declarada;
- variável redeclarada no mesmo escopo;
- chamada de função com número/tipo errado de argumentos;
- uso de vetor onde se espera escalar (e vice-versa).

Ela também **decora a AST**: grava em cada uso de variável **onde** ela está na
memória (categoria + posição), informação que a geração de código vai usar.

### Tabela de Símbolos e Escopos (conceito central)
A **tabela de símbolos** guarda, para cada nome, seu tipo, se é vetor, a
categoria (global/parâmetro/local) e a posição.

Os **escopos** são modelados como uma **PILHA** (`symbol.c`):
- ao **entrar** num bloco, empilhamos um novo escopo;
- ao **sair**, desempilhamos;
- a **busca** por um nome vai do topo (mais interno) para baixo (mais externo).

Isso implementa naturalmente o **shadowing**: uma variável interna “esconde”
uma externa de mesmo nome (ver `symtab_lookup`).

Níveis de escopo nesta linguagem:
```
0  → variáveis globais
1  → parâmetros + variáveis do bloco mais externo da função
2,3… → blocos aninhados
```
Observação importante: **parâmetros e o bloco externo da função compartilham o
escopo 1**. Por isso declarar uma variável local com o mesmo nome de um
parâmetro é erro (ver `analyze_function` e a checagem com `current_func`).

### Sistema de tipos
`analyze_expr` devolve, para cada expressão, um descritor `(tipo, é_vetor)`.
Com isso detectamos, por exemplo, `soma + vet` (somar um vetor inteiro) como
erro: um operando de operador binário **não pode** ser um vetor.

**Perguntas do professor:**
- *“Por que a tabela de símbolos é uma pilha?”* → para refletir o aninhamento
  de escopos e o shadowing: entra-bloco = push, sai-bloco = pop.
- *“O que a gramática não consegue verificar?”* → coisas dependentes de
  contexto: declaração prévia, tipos, número de argumentos. Isso é semântico.
- *“Como você sabe se `x` é global, parâmetro ou local?”* → a busca na tabela
  retorna a categoria; gravamos isso no nó (`VarRef`) para a geração de código.

---

## 5. Fase 4 — Geração de Código e a Máquina de Pilha

**Arquivo:** `src/codegen.c` (baseado nos slides *Geração de Código*).

### A ideia da Máquina de Pilha
A pergunta central é: *“como avaliar expressões arbitrárias usando poucos
registradores?”* A resposta é a **máquina de pilha com um acumulador**:

- **`$s0` é o ACUMULADOR**: toda expressão deixa seu resultado nele.
- Os valores **intermediários** ficam na **PILHA** (memória), não em
  registradores. Por isso o esquema funciona para expressões de qualquer
  profundidade e sobrevive a chamadas recursivas.

Para um operador binário `e1 op e2` (`cgen_expr`, caso `EX_BINARY`):

```
1. calcula e1            → resultado em $s0
2. empilha $s0           (guarda e1 na pilha)
3. calcula e2            → resultado em $s0
4. desempilha e1 em $t1
5. aplica op:  $s0 = $t1 op $s0
```

**Invariante (muito cobrado):** *avaliar uma expressão preserva a pilha* — o que
estava na pilha antes continua lá depois. Isso é o que permite compor
expressões livremente.

> **Exemplo de `n * fat(n-1)`:** calculamos `n`, **empilhamos** `n`, chamamos
> `fat(n-1)` (que usa a pilha à vontade e devolve em `$s0`), **desempilhamos**
> `n` em `$t1` e fazemos `mul`. Como `n` estava na pilha (e não num registrador),
> a chamada recursiva não o destrói. **É por isso que a recursão funciona.**

### Convenção de registradores
| Registrador | Papel |
|---|---|
| `$s0` | acumulador (resultado das expressões) |
| `$sp` | topo da pilha (cresce para endereços **menores**) |
| `$fp` | *frame pointer*: base do registro de ativação atual |
| `$s1` | base das variáveis **globais** |
| `$ra` | endereço de retorno (preenchido por `jal`) |
| `$t0,$t1` | temporários |
| `$v0,$a0` | chamadas de sistema (E/S) |

### Tradução das instruções de alto nível
- Constante: `li $s0, valor`.
- `se/enquanto`: avalia a condição em `$s0` e usa `beq $s0,$zero,rótulo` para
  desviar. Rótulos únicos (`else_N`, `endif_N`, `while_N`, `endwhile_N`).
- `escreva`/`leia`/`novalinha`: usam `syscall` (1=imprime int, 4=imprime string,
  5=lê int, 10=encerra, 11=imprime caractere).

**Perguntas do professor:**
- *“Por que máquina de pilha?”* → simplicidade: gera código de forma sistemática
  com 1 acumulador, sem alocação de registradores.
- *“Qual é o invariante da geração de expressões?”* → a avaliação preserva a
  pilha.
- *“Por que `n * fat(n-1)` não perde o `n` na recursão?”* → porque `n` é
  empilhado na memória antes da chamada.

---

## 6. Ambiente de Execução: Registros de Ativação

**Arquivo:** `src/codegen.c` (`cgen_function`, `cgen_principal`); slides
*Ambiente de Execução*.

### Organização da memória
```
   endereços altos
   ┌───────────────┐
   │  Código        │
   │  Dados (.data) │   ← strings literais
   │  ...           │
   │  Globais       │   ← base em $s1
   │  Pilha ↓       │   ← frames das funções; topo em $sp
   endereços baixos
```

### O Registro de Ativação (frame)
Cada **chamada viva** de uma função tem um **registro de ativação** na pilha,
apontado por `$fp`. A pilha de frames simula a **árvore de ativação** das
funções (percurso em pré-ordem). Layout (de endereços maiores para menores):

```
        $fp do chamador
        argumento n ... argumento 1
$fp →   $ra (endereço de retorno)
        variável local 1 ... local m
$sp →   (próxima posição livre)
```

### Sequência de chamada (quem faz o quê)
**Lado do chamador** (`cgen_expr`, caso `EX_CALL`):
1. empilha o `$fp` atual;
2. empilha os argumentos **em ordem inversa** (`arg_n` primeiro);
3. `jal funcao`.

**Lado do chamado — prólogo** (`cgen_function`):
1. `move $fp, $sp` (marca o início do frame);
2. empilha `$ra`;
3. reserva espaço para as variáveis locais.

**Lado do chamado — epílogo (retorno):**
1. restaura `$ra`;
2. `move $sp, $fp` (libera os locais);
3. ajusta `$sp` para descartar argumentos e o slot do `$fp` salvo;
4. restaura o `$fp` do chamador;
5. `jr $ra`.

Convenção: **o valor de retorno fica em `$s0`** (o acumulador).

### Acesso a variáveis (a “conta” dos deslocamentos)
| Categoria | Endereço |
|---|---|
| Global, posição *p* | `$s1 - 4*(p-1)` |
| Parâmetro, índice *i* | `$fp + 4*i` |
| Local, posição *p* | `$fp - 4*p` |

Vetores ocupam **várias posições**. Um elemento `v[i]` fica em
`base0 - 4*i`, onde `base0` é o endereço do elemento 0 (a subtração existe
porque a pilha cresce para baixo). Ver `emit_array_elem_addr`.

### Vetores como parâmetro: passagem por referência
Um vetor é **grande** e geralmente precisa ser **modificado** pela função
(ex.: `selectionSort` ordena o vetor do chamador). Por isso passamos o
**endereço base** do vetor (passagem por referência), não uma cópia. No
chamado, o parâmetro-vetor guarda esse endereço, e `v[i]` é calculado a partir
dele. Ver `cgen_argument` e o caso `CAT_PARAM` em `emit_array_elem_addr`.

**Perguntas do professor:**
- *“O que é um registro de ativação e o que ele contém?”* → ver layout acima.
- *“Por que os argumentos são empilhados em ordem inversa?”* → para que o
  argumento 1 fique em `$fp+4`, o 2 em `$fp+8`, etc. (acesso simples e uniforme).
- *“Como uma função acessa uma variável global?”* → via `$s1 - 4*(p-1)`.
- *“Vetor é passado por valor ou por referência? Por quê?”* → por referência
  (passa-se o endereço base), para permitir alteração e evitar cópia.
- *“Onde fica o valor de retorno?”* → no acumulador `$s0`.

---

## 7. Mapa: qual conceito está em qual arquivo

| Conceito | Onde estudar no código |
|---|---|
| Tokens, ER, autômato, maximal munch | `src/lexer.l` |
| Gramática, LALR, shift/reduce, precedência | `src/parser.y` |
| AST (tagged union) | `src/ast.h`, `src/ast.c` |
| Tabela de símbolos, pilha de escopos, shadowing | `src/symbol.c` |
| Checagem de tipos, escopos, decoração da AST | `src/semantic.c` |
| Máquina de pilha, acumulador, invariante | `cgen_expr` em `src/codegen.c` |
| Registro de ativação, sequência de chamada | `cgen_function`, `EX_CALL` |
| Acesso a global/parâmetro/local | `emit_scalar_load/store` |
| Vetores e passagem por referência | `emit_array_elem_addr`, `cgen_argument` |
| Organização de memória, ponto de entrada | `cgen_principal` |

---

## 8. Banco de perguntas do professor

**Geral**
- *Quais as fases do compilador e o que cada uma faz?* (§1)
- *O que é a Representação Intermediária? Qual você usou?* → a AST.
- *O seu compilador otimiza o código?* → Não; otimização foi omitida (projeto
  didático). O código gerado é correto, porém não otimizado.

**Léxico/Sintático**
- *Diferença entre análise léxica e sintática?* → léxica forma tokens (ER/AFD);
  sintática verifica a estrutura (gramática livre de contexto) e monta a AST.
- *Seu parser tem conflitos?* → Não, 0 conflitos; o `fimse` elimina o
  *dangling else* e os lookaheads após um `IDENT` são disjuntos.

**Semântico**
- *Como implementou escopos?* → pilha de tabelas de símbolos.
- *Que erros semânticos você detecta?* → não-declaração, redeclaração no mesmo
  escopo, local com nome de parâmetro, nº de argumentos, escalar×vetor em
  parâmetros, tipo de vetor incompatível, uso de vetor sem índice, etc.

**Geração de Código / Execução**
- *Por que máquina de pilha?* (§5)
- *Como a recursão funciona no seu código gerado?* → valores intermediários na
  pilha; cada chamada tem seu próprio registro de ativação.
- *Desenhe o registro de ativação.* (§6)
- *Como acessa cada tipo de variável?* → tabela de deslocamentos em §6.

> Dica de apresentação: tenha em mãos o `fat5.txt` e o `fat5.asm` gerado.
> Mostrar o MIPS da recursão do fatorial e apontar onde o `$fp`, o `$ra` e os
> argumentos entram na pilha costuma responder metade das perguntas de uma vez.
