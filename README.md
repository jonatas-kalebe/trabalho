# Compilador da linguagem Cafezinho (Goianinha)

Projeto didático de um **compilador completo** escrito em **C**, usando **Flex**
(análise léxica) e **Bison** (análise sintática), que traduz programas da
linguagem *Cafezinho* para **código de montagem MIPS** (executável no simulador
SPIM/MARS).

O compilador implementa todas as fases clássicas:

```
texto-fonte → [Léxico/Flex] → [Sintático/Bison] → AST
            → [Semântico] → AST verificada e decorada
            → [Geração de Código] → MIPS
```

> 📚 Para entender **o que foi adicionado na Parte 2** (vetores, funções,
> variáveis globais e a troca de `{}` por `[]`), leia
> **[O_QUE_FOI_ADICIONADO.md](O_QUE_FOI_ADICIONADO.md)**.
> Para os **conceitos gerais** (pilha, registros de ativação, escopos, LALR),
> veja **[GUIA_DE_ESTUDO.md](GUIA_DE_ESTUDO.md)**.
> Para a análise de **cada arquivo de teste**, veja **[TESTES.md](TESTES.md)**.

---

## Estrutura do projeto

| Arquivo | Fase | Papel |
|---|---|---|
| `src/lexer.l`    | Léxica     | Scanner Flex: transforma texto em *tokens*. |
| `src/parser.y`   | Sintática  | Gramática Bison (LALR): reconhece a estrutura e monta a AST. |
| `src/ast.h/.c`   | —          | Definição da Árvore Sintática Abstrata (AST) e seus construtores. |
| `src/symbol.h/.c`| Semântica  | Tabela de símbolos com pilha de escopos. |
| `src/semantic.h/.c` | Semântica | Verificação de tipos, escopos e regras; decora a AST. |
| `src/codegen.h/.c` | Geração  | Percorre a AST e emite MIPS (máquina de pilha). |
| `src/main.c`     | —          | Orquestra as fases. |

---

## Como compilar o compilador

### Windows (já configurado nesta máquina)

Usa o `win_flex`/`win_bison` (em `tools/winflexbison/`) e o compilador C da
Microsoft (`cl.exe`). Basta executar:

```bat
build.bat
```

Isso gera o executável **`compilador.exe`** na raiz do projeto.

### Linux / macOS

Requer `flex`, `bison` e `gcc` instalados:

```sh
make
```

Gera o executável **`compilador`**.

---

## Como usar

```bat
:: imprime o MIPS na tela
compilador.exe  programa.txt

:: grava o MIPS em um arquivo .asm
compilador.exe  programa.txt  saida.asm
```

- Se houver **erro léxico, sintático ou semântico**, o compilador imprime a
  mensagem com o número da linha e **não** gera código.
- Se estiver tudo certo, gera o código MIPS.

### Executando o MIPS gerado

O `.asm` gerado roda no simulador **MARS** ou **SPIM**:

```sh
java -jar Mars.jar  saida.asm
```

Para **testar sem instalar um simulador**, incluímos um mini-simulador em
Python (`tools/sim.py`) que cobre exatamente o subconjunto de instruções que
geramos:

```bat
:: 2o argumento = valores de entrada (para comandos "leia"), separados por espaço
python tools\sim.py  saida.asm  "1 2 3 4 5 10 20 30 40 50"
```

---

## Exemplo rápido

`fat5.txt`:

```
funcao [
    fat(n:int):int {
        se (n==0) entao retorne 1; senao retorne n * fat(n-1); fimse
    }
]
principal {
    escreva "fatorial de 5 = ";
    escreva fat(5);
    novalinha;
}
```

```bat
compilador.exe fat5.txt fat5.asm
python tools\sim.py fat5.asm
```

Saída:

```
fatorial de 5 = 120
```

---

## A linguagem Cafezinho (resumo)

```
programa     →  [ global [ decls ] ]  [ funcao [ funcoes ] ]  principal bloco
funcao       →  nome ( params ) : tipo  bloco
bloco        →  [ [ decls ] ]  { comandos }
decl         →  id { , id }  : tipo ;        (id pode ser  nome  ou  nome[tam])
param        →  nome : tipo   |   nome[] : tipo
tipo         →  int | car
comando      →  atribuição ;  |  leia lv ;  |  escreva (expr|"str") ;
             |  novalinha ;   |  retorne expr ;
             |  se ( expr ) entao comando [ senao comando ] fimse
             |  enquanto ( expr ) comando
             |  bloco  |  ;
```

Operadores (precedência crescente): `=` · `||` · `&&` · `== !=` ·
`< > <= >=` · `+ -` · `* /` · `! -unário`.
