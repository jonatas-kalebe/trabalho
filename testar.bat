@echo off
REM ===========================================================================
REM  testar.bat -- compila e roda a suite de testes do compilador no WSL Linux.
REM  Basta dar 2 cliques neste arquivo (ou rodar no terminal).
REM ===========================================================================
echo Rodando build + testes no WSL Ubuntu-24.04 ...
echo.
wsl -d Ubuntu-24.04 -u root bash -lc "cd /mnt/c/Users/jonatas.mfreitas/Desktop/compilador && sed -i 's/\r$//' tools/*.sh && bash tools/linux_build.sh && echo. && bash tools/run_tests.sh"
echo.
pause
