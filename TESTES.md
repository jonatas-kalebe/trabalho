# Análise dos arquivos de teste

Este documento explica **o que cada teste exercita** e **o resultado do nosso
compilador**, para você apresentar com segurança.

Como rodar todos de uma vez (PowerShell, na raiz do projeto):

```powershell
Get-ChildItem tests\Corretos\Corretos\*.txt, tests\ErrosSemanticos\ErrosSemanticos\*.txt |
  ForEach-Object { "`n===== $($_.Name) ====="; .\compilador.exe $_.FullName 2>&1 |
  Select-String "ERRO|abortada" }
```

---

## Pasta `ErrosSemanticos` — devem **reportar erro**

Em todos, o nosso compilador aponta o erro **na linha indicada pelo nome do
arquivo**. ✔️

| Arquivo | Erro detectado (linha) | Conceito |
|---|---|---|
| `fatorial-ChamadaComMaisParametros**Lin32**` | linha 32: chamada de `fatorial` com 2 argumentos, mas espera 1 | nº de argumentos |
| `fatorial-variavelComMesmoNome…Parametro**Lin9**` | linha 9: variável `n` tem o mesmo nome de um parâmetro | local × parâmetro (escopo) |
| `selectionSortPassagemDeIntParaArgumentoVet**Lin40**` | linha 40: argumento `vet` espera vetor, recebeu escalar | escalar × vetor |
| `selectionSortPassagemDeVetorComtipodiferente**Lin40**` | linha 40: vetor de tipo incompatível com o esperado | tipo do vetor (car × int) |
| `somatorioVetor**Lin12**…VariavelNaoVetor` | linha 12: uso de vetor sem índice em expressão (`soma+vet`) | vetor em expressão |
| `somatorioVetor**Lin14**…SemIndices` | linha 14: vetor usado sem índice / atribuição a vetor | vetor sem índice |
| `somatorioVetor**Lin8**…NaoDeclarada` | linha 8: variável `soma` não declarada | uso sem declaração |

> Vários desses arquivos têm **mais de um** problema (ex.: também usam um `i`
> não declarado). O nosso compilador reporta **todos** os erros que encontra,
> mas o erro “oficial” do nome do arquivo está sempre presente, na linha certa.

---

## Pasta `Corretos` — deveriam compilar

| Arquivo | Resultado | Observação |
|---|---|---|
| `fatorialCorreto.txt` | ✅ Gera MIPS | recursão, `se/senao`, E/S |
| `somaVetores.txt`     | ✅ Gera MIPS | 3 funções, vetores globais por referência |
| `somaVetoresV2.txt`   | ✅ Gera MIPS | idem |
| `selectionSort.txt`   | ⚠️ Reporta erro (linha 40) | **ver nota A** |
| `selectionSortV2.txt` | ⚠️ Reporta erro (linha 44) | **ver nota A** |
| `selectionSortV3.txt` | ⚠️ Reporta erro (linhas 12 e 49) | **ver notas A e B** |
| `somatorioVetor.txt`  | ⚠️ Reporta erro (linha 10) | **ver nota B** |
| `somatorioVetorV2.txt`| ⚠️ Reporta erro (linha 12) | **ver nota B** |

Os arquivos marcados com ⚠️ **contêm erros semânticos reais** que um compilador
correto deve apontar. Eles estão na pasta “Corretos”, mas têm pequenos
descuidos. Isso é ótimo para a apresentação: você mostra que o compilador
**realmente verifica** o programa.

### Nota A — `vet[10]:car` passado para parâmetro `vet[]:int`
Em `selectionSort.txt` (e V2/V3), o `principal` declara o vetor como
**`vet[10]:car`**, mas a função `selectionSort` espera **`vet[]:int`**.
Passar um vetor de `car` para um parâmetro vetor de `int` é uma
**incompatibilidade de tipo**.

O ponto interessante: este é **exatamente** o mesmo construto do arquivo de
erro `selectionSortPassagemDeVetorComtipodiferenteLin40.txt`. Ou seja, o mesmo
código aparece como “correto” em uma pasta e como “erro” na outra — uma
**inconsistência do próprio conjunto de testes**. O nosso compilador é
coerente: detecta a incompatibilidade nos dois casos.

> Se quiser ver `selectionSort.txt` compilar, basta trocar, no `principal`,
> `vet[10]:car;` por `vet[10]:int;`. Aí o tipo bate com o parâmetro e o código
> MIPS é gerado normalmente.

### Nota B — variável de laço `i` não declarada
Em `somatorioVetor.txt`, `somatorioVetorV2.txt` e `selectionSortV3.txt`, a
variável `i` usada nos laços **não é declarada** (em `V3` há até um typo: o
bloco declara `Mi` em vez de `i`). Como a linguagem **exige declaração**, o
compilador acusa `variável 'i' não declarada` — comportamento correto.

> Para compilar, basta declarar o `i` no bloco de variáveis da função, por
> exemplo trocar a abertura por `[ i:int; ]` antes do `{`.

---

## Testes próprios (pasta `tests/meus`) — validação de execução

Como os simuladores SPIM/MARS não estavam instalados, criamos
`tools/sim.py` (mini-simulador do subconjunto MIPS gerado) para **executar de
verdade** o código e conferir a saída:

| Programa | O que valida | Saída obtida |
|---|---|---|
| `fat5.txt` | recursão, registro de ativação, retorno | `fatorial de 5 = 120` ✔️ |
| `vetref.txt` | vetor local + passagem por referência | `0 1 4 9 16` ✔️ |
| `somaVetores.txt` (entrada `1 2 3 4 5 10 20 30 40 50`) | vetores globais por referência, 3 funções | somas `11 22 33 44 55` ✔️ |

Reproduza com:

```powershell
.\compilador.exe tests\meus\fat5.txt tests\meus\fat5.asm
python tools\sim.py tests\meus\fat5.asm
```
