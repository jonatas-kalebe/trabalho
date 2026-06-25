# O que foi adicionado na Parte 2

Na **Parte 1** (que você já apresentou) a linguagem era a versão **base**
(chamada no código de *G-V1*): só existia o `principal`, com **variáveis
escalares** (`int`/`car`), e **tudo ficava entre chaves `{ }`**. Não havia
funções, nem variáveis globais, nem vetores.

A **Parte 2** acrescentou **quatro coisas**:

| # | Novidade | Em uma frase |
|---|---|---|
| 1 | **Colchetes `[ ]` nas declarações** | a seção de declarações deixou de ser `{ }` e passou a ser `[ ]` |
| 2 | **Variáveis globais** (`global [ … ]`) | variáveis visíveis no programa inteiro |
| 3 | **Funções** (`funcao [ … ]`) | sub-rotinas com parâmetros, `retorne`, chamadas e **recursão** |
| 4 | **Vetores / arrays** | `vet[10]`, acesso `vet[i]`, e vetores como parâmetro |

Este documento explica **só essas quatro novidades**, mostrando *como era
antes*, *como ficou* e *como cada uma funciona na memória em tempo de execução*
(usando os conceitos dos slides **Ambiente de Execução** e **Geração de
Código**).

> Os outros documentos ([GUIA_DE_ESTUDO.md](GUIA_DE_ESTUDO.md),
> [TESTES.md](TESTES.md)) continuam valendo para a visão geral e os testes.

---

## 1. Declarações com `[ ]` em vez de `{ }`

### Antes (base / G-V1)
As declarações ficavam **num bloco de chaves**, e os comandos em **outro**:

```
principal { x, y : int; } { x = 1; escreva x; }
```
(no `parser.y` antigo: `Block: { VarDeclList } { ComandoList }`)

### Agora
A seção de **declarações** usa **colchetes `[ ]`**; o **bloco de comandos**
continua com **chaves `{ }`**:

```
principal [ x, y : int; ] { x = 1; escreva x; }
```

### Por quê / o que mudou no código
A ideia é separar visualmente duas coisas diferentes:
- `[ … ]` = a **lista de declarações** (o que existe na memória);
- `{ … }` = o **bloco de comandos** (o que é executado).

No **léxico** (`lexer.l`) foram criados dois tokens novos:
```
"[" { return TOK_LBRACK; }
"]" { return TOK_RBRACK; }
```
No **sintático** (`parser.y`), a regra do bloco passou a ser:
```
Bloco : TOK_LBRACK DeclVarList TOK_RBRACK TOK_LBRACE ComandoList TOK_RBRACE
      | TOK_LBRACE ComandoList TOK_RBRACE          /* bloco sem declaracoes */
```
Esses mesmos colchetes são reaproveitados nas seções `global [ … ]` e
`funcao [ … ]` (novidades 2 e 3).

---

## 2. Variáveis globais (`global [ … ]`)

### Como se escreve
```
global [
    n : int;
    vet[10] : int;
]
```

### O conceito (slides *Ambiente de Execução*)
A memória de um programa é dividida em áreas. Além da **pilha** (onde ficam as
funções), existe a área de **Dados Estáticos** — alocada em **tempo de
compilação** e que **vive durante toda a execução**. As variáveis **globais**
moram nessa ideia de área fixa (no nosso caso, numa região reservada no início
da pilha, apontada por um registrador fixo).

### Como funciona na geração de código (slides *Geração de Código*)
- Reservamos o registrador **`$s1`** para apontar a **base das globais**. No
  início do programa fazemos `move $s1, $sp` e descemos o `$sp` para reservar o
  espaço das globais. **`$s1` nunca muda** durante a execução.
- Toda global tem uma **posição `p`** (1ª global é `p=1`, etc.; um vetor de
  tamanho *n* ocupa *n* posições). O acesso é:
  ```
  endereço da global p  =  $s1 - 4*(p-1)
  ```
- O **escopo** das globais é **0** (o mais externo): elas são enxergadas por
  qualquer função e pelo `principal`.

### Exemplo de código MIPS gerado
Para `n = 1;` onde `n` é a 1ª global (`p = 1`, logo deslocamento `0`):
```mips
    li $s0, 1
    sw $s0, 0($s1)      # grava em $s1 - 4*(1-1) = $s1
```

> **Diferença para uma variável local:** a local é acessada a partir do **frame
> pointer `$fp`** (que muda a cada função); a global é acessada a partir do
> **`$s1`** (fixo). É por isso que ela continua acessível dentro de qualquer
> função.

---

## 3. Funções (`funcao [ … ]`)

Esta é a maior novidade — e a que mais usa os slides de *Ambiente de Execução*.

### Como se escreve
```
funcao [
    fatorial(n:int) : int {
        se (n==0) entao retorne 1;
        senao retorne n * fatorial(n-1); fimse
    }
]
```
Novidades de sintaxe: a seção `funcao [ … ]`, a palavra-chave **`retorne`**, os
**parâmetros** `(n:int)`, o **tipo de retorno** `: int`, e a **chamada**
`fatorial(n-1)` (inclusive **recursiva**).

### O conceito: registro de ativação e árvore de ativação
Quando uma função é **chamada**, dizemos que ela é **ativada**. Cada ativação
viva tem um **Registro de Ativação (RA)** — também chamado *frame* — guardado na
**pilha**. A sequência de ativações forma uma **árvore de ativação**, e a pilha
de RAs **simula essa árvore** (um percurso em pré-ordem): empilha ao chamar,
desempilha ao retornar.

O RA contém (termos dos slides): o **endereço de retorno**, os **argumentos**, a
**ligação de controle** (o `$fp` de quem chamou) e as **variáveis locais**.
No nosso compilador o frame fica assim (endereços maiores em cima):

```
        $fp do chamador     <- "ligação de controle"
        argumento n .. 1
$fp ->  $ra                 <- endereço de retorno
        local 1 .. local m
$sp ->  (livre)
```

### Como funciona na geração de código (a *sequência de chamada*)
**Quem chama (chamador):**
1. empilha o `$fp` atual (a ligação de controle);
2. empilha os argumentos **em ordem inversa**;
3. `jal nome_da_funcao` (salva o retorno em `$ra` e desvia).

**Quem é chamado — prólogo:**
1. `move $fp, $sp` (marca o início do frame);
2. salva `$ra` na pilha;
3. reserva espaço para as variáveis locais.

**Quem é chamado — epílogo (retorno):** restaura `$ra`, devolve o `$sp`,
restaura o `$fp` do chamador e faz `jr $ra`. O **valor de retorno** sai no
acumulador **`$s0`**.

Acesso aos dados da função:
```
parâmetro i  -> $fp + 4*i
local p      -> $fp - 4*p
```

### Exemplo: o prólogo/epílogo de `fatorial` (gerado pelo compilador)
```mips
fatorial:
    move $fp, $sp          # inicia o frame
    sw   $ra, 0($sp)       # salva endereço de retorno
    addiu $sp, $sp, -4
    lw   $s0, 4($fp)       # carrega o parametro n (indice 1 -> $fp+4)
    ...
fatorial__epi:
    lw   $ra, 0($fp)       # restaura $ra
    move $sp, $fp          # libera os locais
    addiu $sp, $sp, 8      # descarta args + $fp salvo  ((1 param + 1)*4)
    lw   $fp, 0($sp)       # restaura $fp do chamador
    jr   $ra               # volta
```

### Por que a recursão funciona?
Porque **cada chamada cria o seu próprio RA na pilha**. Quando `fatorial(3)`
chama `fatorial(2)`, há **dois** registros de ativação empilhados, cada um com o
seu próprio `n`. Os valores intermediários ficam **na pilha** (não em
registradores), então a chamada recursiva não destrói o estado de quem chamou.
Validamos isso de verdade: `fatorial(5)` gera **120** (veja [TESTES.md](TESTES.md)).

### O que mudou na análise semântica
- Cada função abre o **escopo 1** (parâmetros + locais do bloco externo).
- A chamada é checada: **número de argumentos** e **tipo** (escalar × vetor)
  têm de bater com a assinatura. Uma local com o mesmo nome de um parâmetro é
  erro.

---

## 4. Vetores / arrays

### Como se escreve
```
[ vet[10] : int; ]          /* declaração: vetor de 10 inteiros */
vet[i] = vet[i] + 1;        /* acesso/atribuição a um elemento   */

funcao [ soma(v[]:int, n:int):int { ... } ]   /* vetor como parâmetro: v[] */
```
Na base não existia `[`, então **nada disso era possível** — só variáveis
escalares.

### Como funciona na memória
- Um vetor de tamanho *n* **ocupa *n* posições consecutivas**. Se o vetor começa
  na posição `p`, o elemento de índice `i` está na posição `p + i`.
- Como a pilha **cresce para baixo**, o endereço do elemento é:
  ```
  endereço de vet[i]  =  endereço_do_elemento_0  -  4*i
  ```
  (para global, `endereço_do_elemento_0 = $s1 - 4*(p-1)`; para local,
  `= $fp - 4*p`).

### Vetores como parâmetro: **passagem por referência**
Um vetor é grande e geralmente precisa ser **alterado** pela função (ex.: um
`ordena` que ordena o vetor de quem chamou). Por isso **não copiamos o vetor** —
passamos o **endereço base** dele. No chamado, o parâmetro-vetor guarda esse
endereço, e `v[i]` é calculado a partir dele.

> Foi isso que testamos: uma função que preenche um vetor recebido por
> referência realmente altera o vetor do chamador (`0 1 4 9 16`), e o
> `selectionSort` ordena de verdade. Ver [TESTES.md](TESTES.md).

### Exemplo de código MIPS (endereço de `vet[i]`)
```mips
    # ... calcula o indice i em $s0 ...
    sll  $s0, $s0, 2       # i * 4
    addiu $t0, $fp, -4     # endereço do elemento 0 (vetor local na posicao 1)
    sub  $t0, $t0, $s0     # endereço de vet[i] = base0 - 4*i
    lw   $s0, 0($t0)       # carrega vet[i]
```

### O que mudou na análise semântica
- Distinção **escalar × vetor**: usar o nome de um vetor **sem índice** numa
  conta (ex.: `soma + vet`) é erro; indexar uma variável que **não é vetor**
  também é.
- Na chamada, passar um **escalar onde se espera vetor** (ou vetor de tipo
  diferente) é erro.

---

## Resumo: como cada novidade tocou cada fase do compilador

| Novidade | Léxico (`lexer.l`) | Sintático (`parser.y`) | Semântico (`semantic.c`) | Geração de código (`codegen.c`) |
|---|---|---|---|---|
| `[ ]` nas declarações | tokens `[` e `]` | regra `Bloco` com `[decls]{cmds}` | — | — |
| Globais | palavra `global` | seção `global [ … ]` | escopo 0 | base em `$s1`, acesso `$s1-4*(p-1)` |
| Funções | `funcao`, `retorne` | regras de `Funcao`, `Params`, chamada | escopo 1, checagem de assinatura | registro de ativação, `$fp`, `jal`/`jr`, retorno em `$s0` |
| Vetores | (usa `[` `]`) | `vet[tam]`, `vet[i]`, param `vet[]` | escalar × vetor, tipo do vetor | N posições, `base0 - 4*i`, passagem por referência |

---

## Perguntas que o professor pode fazer (sobre as novidades)

**“Por que as declarações usam `[ ]` e os comandos `{ }`?”**
→ Para separar a **lista de declarações** (o que existe) do **bloco de comandos**
(o que executa). São tokens diferentes (`TOK_LBRACK`/`TOK_RBRACE`) e a gramática
do `Bloco` reflete isso.

**“Onde ficam as variáveis globais e como são acessadas?”**
→ Numa área fixa apontada por **`$s1`** (definido uma vez no início e nunca
alterado). A global de posição `p` está em `$s1 - 4*(p-1)`. Escopo 0.

**“O que é um registro de ativação? Desenhe.”**
→ É o *frame* de uma chamada de função na pilha; contém ligação de controle
(`$fp` do chamador), argumentos, `$ra` e variáveis locais. (Ver o desenho na §3.)

**“Por que a recursão funciona?”**
→ Cada chamada empilha **seu próprio** registro de ativação; os valores
intermediários ficam na pilha, então uma chamada não destrói o estado da outra.

**“Vetor é passado por valor ou por referência? Por quê?”**
→ Por **referência** (passa-se o endereço base): evita copiar o vetor inteiro e
permite que a função **altere** o vetor do chamador.

**“Como se calcula o endereço de `vet[i]`?”**
→ `endereço_do_elemento_0 - 4*i` (a subtração porque a pilha cresce para baixo);
o elemento 0 fica em `$s1-4*(p-1)` se global, ou `$fp-4*p` se local.
