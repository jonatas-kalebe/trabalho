@echo off
REM ===========================================================================
REM  build.bat  --  Compila o compilador Cafezinho no Windows usando:
REM      win_flex / win_bison  (gera lex.yy.c e parser.tab.c/.h)
REM      cl.exe (MSVC)         (compila tudo em compilador.exe)
REM ===========================================================================
setlocal
set "ROOT=%~dp0"
set "SRC=%ROOT%src"
set "FLEX=%ROOT%tools\winflexbison\win_flex.exe"
set "BISON=%ROOT%tools\winflexbison\win_bison.exe"

REM o vcvars64 precisa achar o vswhere (fica na pasta do Installer)
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul

if not exist "%ROOT%build" mkdir "%ROOT%build"

echo [1/3] Bison: gerando o parser a partir de parser.y ...
"%BISON%" -d -o "%SRC%\parser.tab.c" "%SRC%\parser.y"
if errorlevel 1 goto :erro

echo [2/3] Flex: gerando o lexer a partir de lexer.l ...
"%FLEX%" -o "%SRC%\lex.yy.c" "%SRC%\lexer.l"
if errorlevel 1 goto :erro

echo [3/3] cl: compilando o codigo C ...
cd /d "%SRC%"
cl /nologo /W3 /D_CRT_SECURE_NO_WARNINGS ^
   main.c ast.c symbol.c semantic.c codegen.c parser.tab.c lex.yy.c ^
   /Fe:"%ROOT%compilador.exe" /Fo:"%ROOT%build\\"
if errorlevel 1 goto :erro

echo.
echo OK -- gerado: %ROOT%compilador.exe
goto :fim

:erro
echo.
echo *** FALHA na compilacao. ***
endlocal
exit /b 1

:fim
endlocal
