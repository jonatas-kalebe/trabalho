# O que foi adicionado na Parte 2 (explicado a fundo)

Na **Parte 1** a linguagem era a versão **base** (no código: *G-V1*): só o
`principal`, com **variáveis escalares** (`int`/`car`), e **tudo entre chaves
`{ }`**. Sem funções, sem globais, sem vetores.

A **Parte 2** acrescentou **quatro coisas**:

| # | Novidade | Em uma frase |
|---|---|---|
| 1 | **Colchetes `[ ]` nas declarações** | a área de declarações deixou de ser `{ }` e virou `[ ]` |
| 2 | **Variáveis globais** (`global [ … ]`) | variáveis visíveis no programa inteiro |
| 3 | **Funções** (`funcao [ … ]`) | sub-rotinas com parâmetros, `retorne`, chamadas e **recursão** |
| 4 | **Vetores / arrays** | `vet[10]`, acesso `vet[i]`, e vetores como parâmetro |

Este documento explica **só essas quatro novidades**, sempre em **dois níveis**:
o **termo técnico** (para a banca) e o **“em miúdos”** 💡 (linguagem simples, com
analogias). Ao final há um **glossário**.

---

## 0. Como acompanhar no próprio código

Marquei **cada trecho novo** com a etiqueta **`[PARTE 2 - NOVO]`**. Para ver
todos os pontos de uma vez, rode na raiz do projeto:

```powershell
Select-String -Path src\*.l, src\*.y, src\*.c, src\*.h -Pattern "PARTE 2 - NOVO"
```

Seguindo a etiqueta você percorre **o fluxo da novidade pelas 4 fases**:
`lexer.l` (vira token) → `parser.y` (vira regra/AST) → `semantic.c` (é checado) →
`codegen.c` (vira MIPS).

---

## Conceito-base: a PILHA (você vai precisar dele o tempo todo)

Quase tudo que foi adicionado usa a **pilha** (*stack*). Vale fixar:

- **Termo técnico:** a pilha é uma região de memória gerenciada pelo registrador
  **`$sp`** (*stack pointer*). Nesta arquitetura ela **cresce para baixo** (cada
  coisa empilhada fica num endereço **menor**). “Empilhar” = `sw` (gravar) +
  `addiu $sp,$sp,-4` (descer o topo); “desempilhar” = o contrário.
- 💡 **Em miúdos:** imagine uma **pilha de pratos**. Você só mexe no prato do
  **topo**: coloca um em cima (empilhar) ou tira o de cima (desempilhar). O `$sp`
  é o seu dedo apontando para o topo. Quando uma função chama outra, é como
  colocar um prato novo em cima; quando ela termina, tira o prato.

Outros registradores que aparecem abaixo:
`$s0` (acumulador, onde toda conta deixa o resultado), `$fp` (marca o início do
“espaço” da função atual), `$s1` (aponta as globais), `$ra` (endereço de
retorno), `$t0/$t1` (rascunho).

---

## 1. Declarações com `[ ]` em vez de `{ }`

### Antes (G-V1) → Agora
```
ANTES:  principal { x, y : int; } { x = 1; escreva x; }
AGORA:  principal [ x, y : int; ] { x = 1; escreva x; }
```

### O que é
- **Termo técnico:** foram introduzidos dois **tokens** novos (`TOK_LBRACK` e
  `TOK_RBRACK`) e a regra do `Bloco` na gramática passou a usar
  `[ DeclVarList ] { ComandoList }`.
- 💡 **Em miúdos:** os **colchetes `[ ]`** são a **“lista do que existe”** (as
  variáveis); as **chaves `{ }`** são a **“lista do que fazer”** (os comandos).
  Separar os dois deixa o programa mais organizado e fácil de ler.

**No código** (siga a etiqueta): `lexer.l` (tokens `[` `]`), `parser.y` (regra
`Bloco`). Essa mesma ideia de `[ … ]` é reaproveitada nas seções `global [ … ]`
e `funcao [ … ]`.

---

## 2. Variáveis globais (`global [ … ]`)

```
global [
    n : int;
    vet[10] : int;
]
```

### O conceito
- **Termo técnico:** variáveis globais têm **alocação estática** e **tempo de
  vida** igual ao do programa inteiro. Ficam na **área de dados estáticos** (nos
  slides *Ambiente de Execução*), separada da pilha de execução. Têm **escopo 0**
  (o mais externo): são visíveis em qualquer função e no `principal`.
- 💡 **Em miúdos:** uma global é como um **quadro de avisos da sala**: qualquer
  pessoa (qualquer função) pode ler e escrever nele, e ele **fica pendurado o
  tempo todo**, do começo ao fim do programa. Já uma variável **local** é como um
  **bilhetinho no seu bolso**: só você (aquela função) vê, e ele é jogado fora
  quando você sai.

### Como é implementado na geração de código
- **Termo técnico:** reservamos um **registrador-base fixo, `$s1`**, apontando a
  base das globais. No início do programa: `move $s1, $sp` e depois descemos o
  `$sp` para reservar o espaço. **`$s1` nunca mais muda.** A global de **posição
  `p`** (1ª global = `p=1`; um vetor de tamanho *n* ocupa *n* posições) é
  acessada por **endereçamento com deslocamento**:
  ```
  endereço da global p  =  $s1 - 4*(p-1)
  ```
- 💡 **Em miúdos:** `$s1` é o **“endereço do quadro de avisos”**, anotado uma vez
  e nunca mais mexido. Cada global tem um **lugar fixo** no quadro; para chegar
  nela, você parte do `$s1` e anda um tanto certo (`-4*(p-1)`). Como o endereço
  é fixo, **qualquer** função alcança a global — diferente das locais, que
  dependem de “onde a função está” na pilha.

**Exemplo de MIPS gerado** (para `n = 1;`, sendo `n` a 1ª global, `p=1`):
```mips
    li $s0, 1
    sw $s0, 0($s1)      # grava em $s1 - 4*(1-1) = $s1
```

**No código:** `cgen_principal` (define `$s1`) e `emit_scalar_load/store`
(o caso `CAT_GLOBAL`) em `codegen.c`; registro com escopo 0 em `semantic.c`.

---

## 3. Funções (`funcao [ … ]`)

A maior novidade. Usa pesado os slides de *Ambiente de Execução*.

```
funcao [
    fatorial(n:int) : int {
        se (n==0) entao retorne 1;
        senao retorne n * fatorial(n-1); fimse
    }
]
```

### 3.1 O conceito: registro de ativação e árvore de ativação
- **Termo técnico:** chamar uma função é **ativá-la**. Cada ativação viva tem um
  **Registro de Ativação (RA / *frame*)** na pilha, contendo: **endereço de
  retorno** (`$ra`), **argumentos**, **ligação de controle** (o `$fp` de quem
  chamou) e **variáveis locais**. A sequência de ativações forma a **árvore de
  ativação**, e a **pilha de RAs simula essa árvore** (percurso em pré-ordem):
  empilha ao chamar, desempilha ao retornar.
- 💡 **Em miúdos:** chamar uma função é como fazer uma **ligação telefônica**:
  você **pausa** o que estava fazendo, a outra pessoa resolve o assunto dela, e
  quando ela desliga você **volta exatamente de onde parou**. Para conseguir
  voltar, você anota num **bloquinho** (o registro de ativação): *“para onde eu
  volto”* (`$ra`), *“quais valores recebi”* (argumentos), *“onde está o bloquinho
  de quem me ligou”* (`$fp` do chamador) e *“minhas anotações de rascunho”* (as
  variáveis locais). A **pilha de bloquinhos** é o histórico de chamadas.

Layout do RA que o nosso compilador usa (endereços maiores em cima):
```
        $fp do chamador     <- ligação de controle ("bloquinho de quem me chamou")
        argumento n .. 1
$fp ->  $ra                 <- endereço de retorno ("para onde eu volto")
        local 1 .. local m
$sp ->  (livre)
```

### 3.2 A sequência de chamada (o “protocolo” da ligação)
- **Quem chama (chamador)** — em `codegen.c`, caso `EX_CALL`:
  1. empilha o `$fp` atual (a ligação de controle);
  2. empilha os argumentos **em ordem inversa** (o arg 1 fica logo acima do `$ra`);
  3. `jal nome_funcao` — *jump and link*: desvia para a função **e** salva em
     `$ra` o endereço da instrução seguinte.
- **Quem é chamado — prólogo** (`cgen_function`):
  1. `move $fp, $sp` (marca o início do frame);
  2. salva `$ra` na pilha;
  3. reserva espaço para as variáveis locais.
- **Quem é chamado — epílogo (retorno):** restaura `$ra`, devolve o `$sp` ao
  ponto inicial, restaura o `$fp` do chamador e faz `jr $ra`. O **valor de
  retorno** sai no acumulador **`$s0`**.

Acesso aos dados, por **deslocamento a partir do `$fp`**:
```
parâmetro i  ->  $fp + 4*i        (argumentos ficam ACIMA do $fp)
local p      ->  $fp - 4*p        (locais ficam ABAIXO do $fp)
```
💡 **Em miúdos:** o `$fp` é um **“marcador de página”** fincado no começo do
espaço da função. Tudo dela é encontrado “a tantos passos do marcador”: os
argumentos um pouco **acima**, as variáveis locais um pouco **abaixo**.

### 3.3 Exemplo: prólogo/epílogo do `fatorial` (gerado pelo compilador)
```mips
fatorial:
    move $fp, $sp          # finca o marcador (inicio do frame)
    sw   $ra, 0($sp)       # guarda "para onde volto"
    addiu $sp, $sp, -4
    lw   $s0, 4($fp)       # le o parametro n  (parametro 1 -> $fp+4)
    ...
fatorial__epi:
    lw   $ra, 0($fp)       # recupera "para onde volto"
    move $sp, $fp          # joga fora as variaveis locais
    addiu $sp, $sp, 8      # joga fora os argumentos + o $fp salvo  ((1+1)*4)
    lw   $fp, 0($sp)       # recupera o marcador de quem me chamou
    jr   $ra               # volta
```

### 3.4 Por que a recursão funciona? (a pergunta de ouro)
- **Termo técnico:** cada chamada cria **seu próprio** registro de ativação na
  pilha; os valores intermediários ficam na **pilha**, não em registradores.
  Logo, `fatorial(3)` e `fatorial(2)` coexistem com `n` independentes.
- 💡 **Em miúdos:** é como **vários bloquinhos empilhados**, um por ligação.
  Quando `fatorial(3)` “liga” para `fatorial(2)`, há **dois bloquinhos**: cada um
  com o **seu próprio `n`**. Como cada um tem o seu, eles **não se misturam**.
  Quando a ligação de baixo desliga, ela devolve o resultado (em `$s0`) e o
  bloquinho some — e a de cima continua de onde parou.

Veja a pilha durante `fatorial(2)` (já dentro da chamada feita por `fatorial(3)`):
```
   ...                         (frame do principal)
   | $fp do principal |
   | n = 3            |        <- frame de fatorial(3)
   | $ra             |  <─ $fp do (3)
   |                 |
   | $fp de fatorial(3)|
   | n = 2            |        <- frame de fatorial(2)
   | $ra             |  <─ $fp do (2)   (o $fp "atual")
   $sp →
```
Cada `n` mora no seu próprio frame → recursão correta. Testado:
`fatorial(5) = 120` (ver [TESTES.md](TESTES.md)).

### 3.5 O que mudou na análise semântica
- **Termo técnico:** cada função abre o **escopo 1** (parâmetros + locais do
  bloco externo convivem nele). A **chamada** é verificada contra a **assinatura**
  da função: **aridade** (número de argumentos) e **compatibilidade de tipos**
  (escalar × vetor). Um local com nome igual a um parâmetro é erro.
- 💡 **Em miúdos:** o compilador confere se você **ligou para o número certo**
  (função existe), **falou a quantidade certa de coisas** (nº de argumentos) e
  **do tipo certo** (não mandar um vetor onde se espera um número).

**No código:** `analyze_function` e o caso `EX_CALL` em `semantic.c`;
`cgen_function` e `EX_CALL` em `codegen.c`.

---

## 4. Vetores / arrays

```
[ vet[10] : int; ]          /* declara um vetor de 10 inteiros */
vet[i] = vet[i] + 1;        /* acessa/atribui um elemento       */
funcao [ ordena(v[]:int, n:int):int { ... } ]   /* vetor como parametro: v[] */
```
Na base não havia `[`, então **nada disso existia** — só variáveis escalares.

### 4.1 Como o vetor fica na memória
- **Termo técnico:** **alocação contígua** — um vetor de tamanho *n* ocupa *n*
  posições **consecutivas**. Se o vetor começa na posição `p`, o elemento de
  índice `i` está na posição `p + i`. Como a pilha cresce para baixo, o
  **endereço** é calculado por **deslocamento** a partir do elemento 0:
  ```
  endereço de vet[i]  =  endereço_do_elemento_0  -  4*i
  ```
  (`endereço_do_elemento_0` = `$s1 - 4*(p-1)` se global; `$fp - 4*p` se local.)
- 💡 **Em miúdos:** um vetor é uma **fileira de armários numerados**, todos
  juntos. `vet[i]` é o armário número `i`. Para achar o armário `i`, você vai
  até o armário 0 e anda `i` casas. Só isso.

### 4.2 Vetor como parâmetro: **passagem por referência**
- **Termo técnico:** vetores são passados **por referência** — empilha-se o
  **endereço-base** do vetor, não uma cópia. No chamado, o parâmetro-vetor guarda
  esse endereço, e `v[i]` é calculado a partir dele. Isso evita copiar *n*
  elementos e permite **alterar o vetor do chamador** (efeito colateral
  desejado, ex.: ordenar no lugar).
- 💡 **Em miúdos:** em vez de **fotocopiar** os 10 armários e entregar as cópias
  para a função (lento, e mexer nas cópias não mudaria os seus), você só entrega
  o **endereço da fileira**. A função então mexe **nos seus armários de
  verdade**. É por isso que um `ordena(vet, 10)` realmente ordena o **seu** vetor.

**Exemplo de MIPS (endereço de `vet[i]`, vetor local):**
```mips
    # ... o indice i ja foi calculado em $s0 ...
    sll  $s0, $s0, 2       # i * 4   (cada inteiro ocupa 4 bytes)
    addiu $t0, $fp, -4     # endereco do elemento 0 (vetor local na posicao 1)
    sub  $t0, $t0, $s0     # endereco de vet[i] = base0 - 4*i
    lw   $s0, 0($t0)       # carrega o valor de vet[i]
```

### 4.3 O que mudou na análise semântica
- Distinção **escalar × vetor**: usar o **nome de um vetor sem índice** numa
  conta (ex.: `soma + vet`) é erro; **indexar** algo que não é vetor também é.
- Na chamada, passar um **escalar onde se espera vetor** (ou vetor de tipo
  diferente) é erro.

**No código:** `emit_array_elem_addr` e `cgen_argument` em `codegen.c`; caso
`EX_ARRAY` em `semantic.c`.

---

## Como cada novidade atravessa as fases do compilador

| Novidade | Léxico (`lexer.l`) | Sintático (`parser.y`) | Semântico (`semantic.c`) | Geração de código (`codegen.c`) |
|---|---|---|---|---|
| `[ ]` nas declarações | tokens `[` `]` | regra `Bloco` | — | — |
| Globais | `global` | seção `global[…]` | escopo 0 | base `$s1`, `$s1-4*(p-1)` |
| Funções | `funcao`, `retorne` | `Funcao`, `Params`, chamada | escopo 1, assinatura | registro de ativação, `$fp`, `jal`/`jr`, retorno em `$s0` |
| Vetores | (usa `[` `]`) | `vet[tam]`, `vet[i]`, `vet[]` | escalar × vetor | `base0 - 4*i`, passagem por referência |

---

## Glossário rápido (termos técnicos)

- **Token:** a “palavra” que o léxico reconhece (ex.: `TOK_FUNCAO`).
- **AST:** árvore que representa o programa; o parser a constrói.
- **Escopo:** região onde um nome é visível (0 = global; 1 = parâmetros/locais
  da função; 2+ = blocos aninhados).
- **Pilha / `$sp`:** memória que cresce para baixo; topo apontado por `$sp`.
- **Acumulador (`$s0`):** registrador onde toda expressão deixa seu resultado.
- **Registro de ativação (RA / frame):** o “bloquinho” de uma chamada de função
  na pilha (retorno, argumentos, ligação de controle, locais).
- **Frame pointer (`$fp`):** marcador do início do RA atual.
- **Ligação de controle:** o `$fp` do chamador, salvo no RA, para conseguir
  restaurar o frame de quem chamou.
- **Endereço de retorno (`$ra`):** para onde a função volta (gravado por `jal`).
- **Deslocamento (*offset*):** o “quanto andar” a partir de um registrador-base
  (`$fp` ou `$s1`) para achar uma variável.
- **Passagem por referência:** passar o **endereço** de um dado (não uma cópia),
  permitindo que a função altere o original.
- **Alocação contígua:** os elementos de um vetor ficam em posições vizinhas.

---

## Perguntas que o professor pode fazer (sobre as novidades)

**“Por que `[ ]` nas declarações e `{ }` nos comandos?”**
→ Para separar a **lista de declarações** (o que existe) do **bloco de comandos**
(o que executa). São tokens diferentes e a regra do `Bloco` reflete isso.

**“Onde ficam as globais e como são acessadas?”**
→ Em área de **dados estáticos**, base fixa em **`$s1`** (definido uma vez e
nunca alterado). Global de posição `p`: `$s1 - 4*(p-1)`. Escopo 0.

**“Desenhe e explique um registro de ativação.”**
→ Frame na pilha com: ligação de controle (`$fp` do chamador), argumentos, `$ra`
e locais. `$fp` marca o início; parâmetros em `$fp+4*i`, locais em `$fp-4*p`.
(Ver §3.1.)

**“Por que a recursão funciona?”**
→ Cada chamada tem **seu próprio** registro de ativação; valores intermediários
ficam na pilha, então uma chamada não atropela o estado da outra. (Ver §3.4.)

**“Vetor é passado por valor ou por referência? Por quê?”**
→ Por **referência** (passa-se o endereço-base): evita copiar o vetor e permite
**alterar** o vetor do chamador.

**“Como se calcula o endereço de `vet[i]`?”**
→ `endereço_do_elemento_0 - 4*i` (subtração porque a pilha cresce para baixo);
elemento 0 em `$s1-4*(p-1)` (global) ou `$fp-4*p` (local).
