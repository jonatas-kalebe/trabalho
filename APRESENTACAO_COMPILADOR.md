# Manual Completo de Estudo + Roteiro de Defesa do Compilador G-V1

> Objetivo deste documento: te levar do **zero em compiladores** até o ponto de conseguir explicar com segurança cada etapa do seu projeto, justificar decisões e responder perguntas técnicas do professor.

---

## PARTE 1 — Fundamentos de Compiladores (do zero)

## 1. O que é um compilador?
Um compilador é um programa que transforma código-fonte escrito em uma linguagem A (alto nível) para uma linguagem B (baixo nível), normalmente assembly ou código de máquina.

No seu caso:
- **Entrada**: linguagem G-V1 (`.g`)
- **Saída**: código MIPS (`.asm`)

## 2. As 4 fases clássicas que seu projeto implementa

1. **Análise Léxica** (Lexer):
   - Lê caracteres.
   - Agrupa em tokens (identificador, número, palavra-chave, símbolo).
2. **Análise Sintática** (Parser):
   - Verifica se os tokens obedecem à gramática.
   - Constrói a AST (árvore sintática abstrata).
3. **Análise Semântica**:
   - Verifica regras de significado (tipo, escopo, uso de variável).
4. **Geração de Código**:
   - Traduz AST válida para MIPS.

## 3. Diferença essencial (pergunta clássica)
- Léxico valida “palavras” (tokenização).
- Sintático valida “frases” (estrutura gramatical).
- Semântico valida “sentido” (regras de tipo/escopo).

Exemplo:
- `int x; x = 'a' + 2;`
  - Léxico: ok
  - Sintático: ok
  - Semântico: pode falhar se regra não permitir operação mista

---

## PARTE 2 — Arquitetura real do seu projeto

## 4. Mapa de arquivos (o que cada arquivo faz)

- `src/lexer.l`: regras do Flex (tokens da linguagem).
- `src/parser.y`: gramática Bison e ações semânticas que montam AST.
- `src/ast.h` e `src/ast.c`: definição e construção dos nós da AST.
- `src/semantics.h` e `src/semantics.c`: checagem semântica (tipos, escopos, símbolos).
- `src/codegen.h` e `src/codegen.c`: geração do assembly MIPS.
- `src/main.c`: orquestra pipeline completo.

## 5. Fluxo de execução real (o que acontece quando roda)

1. `main` abre arquivo de entrada e aponta `yyin` para o lexer.
2. `yyparse()` chama lexer + parser e monta `root_program`.
3. Se parsing passou, `check_semantics(&root_program)` valida significado.
4. Se semântica passou, `generate_code(&root_program, out)` gera MIPS.

Mensagem para banca:
> “Meu compilador é **fail-fast por fase**: erro sintático para sintaxe; erro semântico para significado; só gera código quando as fases anteriores estão corretas.”

---

## 5.1 Entendendo o Parser do zero (Bison sem mistério)

Se o professor perguntar do parser, você pode explicar assim:

1. O parser trabalha com **tokens** que vêm do lexer (`yylex()`).
2. Em `parser.y`, cada regra tem formato:
   - `NaoTerminal: producao1 | producao2 ... ;`
3. Quando uma produção casa, o bloco `{ ... }` executa e cria/encadeia nós.

Conceitos fundamentais de Bison no seu arquivo:
- **`%token`**: declara quais tokens existem.
- **`%union`**: define os tipos de valor semântico que cada símbolo pode carregar.
- **`%type`**: diz qual campo da union cada não-terminal usa.
- **`$1`, `$2`, ...**: valores dos itens da direita da regra.
- **`$$`**: valor produzido pela regra (lado esquerdo).
- **`%left`, `%right` e `%prec`**: resolvem precedência e associatividade.

Exemplo explicado:
- Regra: `Expr TOK_PLUS Expr`
- Ação: cria `new_expr_binary(OP_ADD, $1, $3, ...)`
- Interpretação: se reconheci `expressão + expressão`, monto um nó de soma na AST.

### Como explicar as principais regras do seu parser

1. `Programa`: regra inicial; ancora resultado em `root_program`.
2. `Block`: reconhece bloco com/sem declarações e cria `new_block`.
3. `VarDeclList`/`VarDecl`: converte `a, b, c : int;` em lista de `Decl`.
4. `Comando`: reconhece estruturas imperativas (`if`, `while`, `leia`, `escreva` etc.).
5. `Expr`: reconhece expressões e cria nós respeitando precedência.

### Ponto que professor costuma cobrar
“Como você diferencia `-x` de `a - b`?”
- Resposta: usando `%prec UMINUS` no parser para tratar o menos unário com precedência adequada.

---

## PARTE 3 — AST explicada como se você fosse ensinar

## 6. O que é AST?
AST (Abstract Syntax Tree) é uma árvore que representa a estrutura lógica do programa, sem detalhes supérfluos de pontuação.

Exemplo simples:
- Fonte: `a = 2 + 3`
- AST:
  - ASSIGN(name="a")
    - BINARY(ADD)
      - INT(2)
      - INT(3)

## 7. Por que AST foi uma boa escolha?
1. Separa parsing das fases seguintes.
2. Facilita checagem semântica recursiva.
3. Facilita gerar código por travessia da árvore.
4. Escala para novas features (novos tipos de nós).

## 8. “É árvore binária, ternária ou mista?”
Resposta correta para seu projeto:
- É **mista/heterogênea**.
- Nós de expressão binária têm 2 filhos (`left`, `right`).
- Nó `if` tem até 3 componentes lógicas (`cond`, `then`, `else`).
- Alguns nós têm 1 filho (unário), outros podem ter 0 (literal).

## 9. Como a AST é montada no projeto
No `parser.y`, cada regra gramatical chama funções como:
- `new_expr_binary(...)`
- `new_stmt_if(...)`
- `new_stmt_while(...)`
- `new_decl(...)`

Essas funções estão em `ast.c` e encapsulam a criação correta dos nós.

---

## PARTE 4 — Semântica (a parte que mais cai em pergunta)

## 10. O que a semântica precisa garantir

1. Variável foi declarada antes de usar.
2. Não redeclarar mesmo nome no mesmo escopo.
3. Operações respeitam tipos.
4. Condições de `if`/`while` têm tipo válido.

## 11. Tabela de símbolos
A tabela de símbolos guarda dados de identificadores:
- nome
- tipo
- metadados úteis ao backend (offset/size)

No seu código, isso é o `struct Sym`.

## 12. Pilha de escopos
Cada bloco `{ ... }` abre um escopo novo:
- entra no bloco: `push_scope`
- sai do bloco: `pop_scope`

Busca de variável:
- `lookup_scope`: só escopo atual.
- `lookup_all`: atual e ancestrais (de dentro para fora).

Isso implementa **shadowing** naturalmente.

## 13. Inferência/checagem de tipos em expressões
`analyze_expr` percorre árvore recursivamente (pós-ordem):
1. resolve tipo dos filhos
2. valida operador
3. define tipo do nó pai

Exemplo:
- `2 + 3` -> INT
- `x < y` -> INT lógico (resultado de comparação)

## 14. Estratégia de erro
Seu projeto usa erro fatal (`semantic_error` + `exit`) no primeiro erro.
Vantagem didática:
- simplifica controle de estado;
- evita enxurrada de erros em cascata.

Desvantagem (pode falar como limitação):
- não lista múltiplos erros em uma única compilação.

---

## PARTE 5 — Geração de código MIPS (visão de banca)

## 15. Ideia geral do backend
Após AST validada:
- aloca dados/strings quando necessário;
- gera instruções para expressões;
- gera desvios para `if` e loops;
- emite syscalls para I/O.

## 16. Tradução mental de estruturas

### Atribuição
- calcula expressão da direita
- grava no endereço da variável da esquerda

### If
- avalia condição
- branch para rótulo do `else`/fim quando falsa
- bloco `then`
- jump para fim (se tiver else)
- bloco `else`

### While
- rótulo de teste
- avalia condição
- sai do loop se falsa
- executa corpo
- jump para rótulo de teste

## 16.1 Fluxo de dados completo do codegen (de onde vem e para onde vai)

Pense no codegen como uma “esteira” com entrada e saída:

- **Entrada principal**: `Program/Block/Stmt/Expr` da AST (já validada pela semântica).
- **Estado auxiliar**: tabela de símbolos do backend (`CGSym`), pilha de escopos (`CGScope`), labels e strings (`CodegenCtx`).
- **Saída final**: texto assembly MIPS escrito em `FILE *out`.

### Etapa A — Preparação da seção `.data` (strings)
1. Codegen percorre AST procurando `ST_WRITE_STR`.
2. Cada literal passa por `intern_string`.
3. Se string já existe, reutiliza label; senão cria nova label (`new_label`).
4. Na emissão, essas labels viram entradas `.asciiz`.

**Resumo de fluxo**:\ntexto no código-fonte -> nó `ST_WRITE_STR` -> lista `CGString` -> label `.data` -> `la $a0, label` no `.text`.

### Etapa B — Preparação de memória local (variáveis)
1. Ao entrar em bloco: `cg_push_scope`.
2. Para cada `Decl`, calcula `size` por tipo (`cg_type_size`).
3. Acumula bytes locais e atualiza `offset` relativo ao frame pointer.
4. Emite `addiu $sp, $sp, -N` para reservar espaço.

**Resumo de fluxo**:\n`Decl` da AST -> `CGSym` (name/type/offset/size) -> endereço de pilha -> instruções `lw/sw/lb/sb`.

### Etapa C — Expressões (valor em registradores temporários)
1. `emit_expr` visita recursivamente a árvore.
2. Folhas (constantes/variáveis) carregam valor.
3. Nós internos (binários/unários) combinam resultados.
4. Resultado final da expressão fica no registrador/protocolo esperado pelo statement chamador.

**Pergunta de banca comum**:\n“Como sabe onde pegar variável `x`?”\nResposta: `cg_lookup` encontra `x` no escopo mais interno e retorna offset para acesso relativo ao frame.

### Etapa D — Statements (efeitos colaterais e controle)
- `ST_EXPR`: avalia expressão (ex.: atribuição) e aplica efeito.
- `ST_READ`: syscall de entrada e armazenamento no offset da variável.
- `ST_WRITE_EXPR`: avalia expressão e imprime.
- `ST_IF`: cria labels de desvio condicional.
- `ST_WHILE`: cria label de início, teste, corpo e retorno ao teste.
- `ST_BLOCK`: abre e fecha escopo com push/pop.

### Etapa E — Fechamento de bloco
1. `cg_pop_scope` recupera bytes alocados no bloco.
2. Emite `addiu $sp, $sp, +N`.
3. Remove símbolos locais para manter shadowing correto entre blocos irmãos.

### Caminho completo (1 linha para decorar)
**Texto fonte -> tokens -> AST -> semântica válida -> codegen resolve símbolos/labels -> escreve `.data` e `.text` MIPS.**

---

## PARTE 6 — Perguntas difíceis (com resposta pronta)

## 17. “Por que linked list em vez de vetor?”
“Porque comandos e declarações são construídos incrementalmente durante parsing; lista encadeada simplifica append por encadeamento sem realocação frequente.”

## 18. “Por que fail-fast?”
“Decisão de escopo didático: garante consistência interna e reduz complexidade do compilador. Em versão industrial, eu acrescentaria recuperação de erro para continuar análise.”

## 19. “Como provar que escopo funciona?”
“Porque `lookup_scope` bloqueia redeclaração local e `lookup_all` busca de dentro para fora, implementando shadowing. Além disso, cada bloco empilha/desempilha seu contexto.”

## 20. “Como provar que tipos funcionam?”
“`analyze_expr` valida por tipo de nó. Operações aritméticas exigem tipos compatíveis; comparações exigem compatibilidade entre operandos; `if/while` exigem condição inteira/lógica do modelo da linguagem.”

## 21. “Se eu pedir nova feature, onde mexe?”
- novo token: `lexer.l`
- nova regra sintática: `parser.y`
- novo nó AST: `ast.h`/`ast.c`
- nova regra semântica: `semantics.c`
- nova emissão de código: `codegen.c`

---

## PARTE 7 — Checklist completo de domínio

## 22. Checklist técnico
- [ ] Sei explicar token, gramática, AST, escopo e tipo.
- [ ] Sei desenhar pipeline do `main` sem consultar código.
- [ ] Sei explicar diferença entre erro léxico/sintático/semântico.
- [ ] Sei explicar shadowing com exemplo.
- [ ] Sei explicar como if/while viram branch/jump.
- [ ] Sei descrever ao menos 3 limitações reais do projeto.

## 23. Checklist de demo ao vivo
- [ ] Caso válido gerando `.asm`.
- [ ] Caso com variável não declarada.
- [ ] Caso com tipos incompatíveis.
- [ ] Caso com erro sintático simples.

---

## PARTE 8 — Mini glossário (rápido para decorar)

- **Token**: unidade léxica (ex: `ID`, `NUM`, `IF`).
- **Gramática**: regras formais da linguagem.
- **AST**: árvore lógica do programa.
- **Escopo**: região onde um identificador é visível.
- **Shadowing**: variável interna com mesmo nome da externa.
- **Type checking**: validação de compatibilidade de tipos.
- **Backend**: fase que gera código alvo (MIPS).

---

## PARTE 9 — Roteiro de fala de 12 minutos (pronto)

1. **(1 min)** “Vou mostrar um compilador educacional da linguagem G-V1 para MIPS.”
2. **(2 min)** “Pipeline: léxico, sintático, AST, semântico, backend.”
3. **(2 min)** “AST: por que existe, como representa expressões e controle.”
4. **(3 min)** “Semântica: tabela de símbolos, pilha de escopos, checagem de tipos.”
5. **(2 min)** “Codegen: como if/while e expressões viram instruções.”
6. **(2 min)** “Limitações + próximos passos.”

Frase final forte:
> “A arquitetura separada por fases me permite garantir corretude incremental: cada fase valida uma dimensão do problema antes da próxima.”

---

## PARTE 10 — Próximos passos (se professor pedir evolução)

1. Recuperação de erro semântico para relatar múltiplos erros.
2. Otimizações básicas (constant folding).
3. Mais tipos e coerções explícitas.
4. Funções/procedimentos e parâmetros.
5. Testes automatizados com casos positivos e negativos.
