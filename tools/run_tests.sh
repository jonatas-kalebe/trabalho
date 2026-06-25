#!/usr/bin/env bash
# ===========================================================================
#  run_tests.sh -- roda o compilador Cafezinho contra os testes do professor.
#
#    Corretos/         -> devem COMPILAR sem erro (exit 0, gera .asm)
#    ErrosSemanticos/  -> devem ACUSAR erro semantico na linha indicada no
#                         nome do arquivo (padrao ...Lin<N>...)
#
#  Uso:  bash tools/run_tests.sh
# ===========================================================================
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMP="$ROOT/compilador"
TESTS="$ROOT/tests"
OUTDIR="$ROOT/build/asm"
mkdir -p "$OUTDIR"

if [ ! -x "$COMP" ]; then
  echo "ERRO: binario '$COMP' nao encontrado. Compile antes (tools/linux_build.sh)."
  exit 2
fi

ok() {   printf '  \033[32m[ OK ]\033[0m %s\n' "$1"; }
bad() {  printf '  \033[31m[FALHA]\033[0m %s\n' "$1"; }

passC=0; failC=0; passE=0; failE=0
failed_corretos=""

echo
echo "================= CORRETOS (devem compilar SEM erro) ================="
for f in "$TESTS"/Corretos/*.txt; do
  [ -e "$f" ] || continue
  name="$(basename "$f")"
  asm="$OUTDIR/${name%.txt}.asm"
  out="$("$COMP" "$f" "$asm" 2>&1)"; rc=$?
  if [ "$rc" -eq 0 ]; then
    nlines=$(wc -l < "$asm" 2>/dev/null || echo 0)
    ok "$name  ->  gerou ${name%.txt}.asm ($nlines linhas MIPS)"
    passC=$((passC+1))
  else
    bad "$name"
    echo "$out" | sed 's/^/         | /'
    failC=$((failC+1))
    failed_corretos="$failed_corretos $name"
  fi
done

echo
echo "========== ERROS SEMANTICOS (devem acusar erro na linha N) =========="
for f in "$TESTS"/ErrosSemanticos/*.txt; do
  [ -e "$f" ] || continue
  name="$(basename "$f")"
  expline="$(printf '%s' "$name" | grep -oE 'Lin[0-9]+' | head -1 | tr -dc '0-9')"
  out="$("$COMP" "$f" /dev/null 2>&1)"; rc=$?
  got="$(printf '%s' "$out" | grep -oE 'linha [0-9]+' | tr -dc '0-9\n' | paste -sd, -)"
  if [ -n "$expline" ] && printf '%s' "$out" | grep -qE "linha ${expline}\)"; then
    ok "$name"
    printf '         esperado linha %s; erros reportados nas linhas: %s\n' "$expline" "${got:-nenhum}"
    passE=$((passE+1))
  else
    bad "$name  (esperava erro na linha ${expline:-?})"
    echo "$out" | sed 's/^/         | /'
    failE=$((failE+1))
  fi
done

echo
echo "============================ RESUMO ============================"
printf 'Corretos        : %d OK / %d FALHA   (de %d)\n' "$passC" "$failC" "$((passC+failC))"
printf 'ErrosSemanticos : %d OK / %d FALHA   (de %d)\n' "$passE" "$failE" "$((passE+failE))"
[ -n "$failed_corretos" ] && printf 'Corretos que acusaram erro:%s\n' "$failed_corretos"
echo "==============================================================="
