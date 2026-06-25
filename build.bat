@echo off
setlocal
set "ROOT=%~dp0"
set "SRC=%ROOT%src"
set "FLEX=%ROOT%tools\winflexbison\win_flex.exe"
set "BISON=%ROOT%tools\winflexbison\win_bison.exe"

set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul

if not exist "%ROOT%build" mkdir "%ROOT%build"

"%BISON%" -d -o "%SRC%\parser.tab.c" "%SRC%\parser.y"
if errorlevel 1 goto :erro

"%FLEX%" -o "%SRC%\lex.yy.c" "%SRC%\lexer.l"
if errorlevel 1 goto :erro

cd /d "%SRC%"
cl /nologo /W3 /D_CRT_SECURE_NO_WARNINGS ^
   main.c ast.c symbol.c semantic.c codegen.c parser.tab.c lex.yy.c ^
   /Fe:"%ROOT%compilador.exe" /Fo:"%ROOT%build\\"
if errorlevel 1 goto :erro

echo OK -- compilador.exe gerado
goto :fim

:erro
echo FALHA na compilacao.
endlocal
exit /b 1

:fim
endlocal
