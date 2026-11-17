@echo off
REM ---------------------------------------------------------------------------
REM Compila o jogo no Windows com MinGW (g++).
REM
REM -static : coloca as bibliotecas do C++ DENTRO do .exe. Sem isso, o jogo
REM           pede DLLs (libstdc++-6.dll etc.) que nao existem em outros PCs.
REM ---------------------------------------------------------------------------
setlocal enabledelayedexpansion
cd /d "%~dp0"

set ARQUIVOS=
for %%f in (src\*.cpp) do set ARQUIVOS=!ARQUIVOS! %%f

g++ -std=c++17 -O2 -Wall -Wextra -static -o masmorra.exe !ARQUIVOS!
if errorlevel 1 (
    echo.
    echo *** Erro na compilacao ***
    exit /b 1
)
echo Compilado: masmorra.exe
