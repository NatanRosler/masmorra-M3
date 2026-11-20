@echo off
REM ---------------------------------------------------------------------------
REM  Compila o MASMORRA no Windows usando so o g++ (MinGW). Nao precisa de CMake.
REM
REM  1a vez: compila tambem a raylib (pasta externo\raylib-5.5). Demora ~1 min.
REM  Depois: so recompila o jogo, que e rapido.
REM
REM  Gera, nesta mesma pasta:
REM         masmorra.exe          (versao grafica, com o icone do bau mimico)
REM         masmorra_console.exe  (versao de terminal)
REM  e, na primeira vez, um atalho "Masmorra" na Area de Trabalho.
REM
REM  Depois de compilado, para jogar basta abrir o masmorra.exe (ou o atalho).
REM  So precisa rodar este arquivo de novo quando o codigo mudar.
REM
REM  A versao grafica precisa do MinGW-w64 (o MinGW "antigo" nao tem os
REM  arquivos do Windows que a raylib usa). Se so houver o antigo, este
REM  arquivo compila a versao de console e explica como instalar o novo.
REM ---------------------------------------------------------------------------
setlocal enabledelayedexpansion
cd /d "%~dp0"
set "PASTA=%~dp0"

set "RAYLIB=externo\raylib-5.5\src"
set "OBJ=build\raylib"
if not exist "%OBJ%" mkdir "%OBJ%"
REM Versoes antigas geravam o jogo dentro de build\. Agora ele fica aqui,
REM ao lado do save e do ranking: apaga os antigos para nao confundir.
if exist "build\masmorra.exe" del "build\masmorra.exe"
if exist "build\masmorra_console.exe" del "build\masmorra_console.exe"

REM ---- 1. procurar um MinGW-w64 instalado ----------------------------------
set "MINGW="
set "PACOTES=%LOCALAPPDATA%\Microsoft\WinGet\Packages"
for /f "delims=" %%d in ('dir /b /ad "%PACOTES%\BrechtSanders.WinLibs*" 2^>nul') do if exist "%PACOTES%\%%d\mingw64\bin\g++.exe" set "MINGW=%PACOTES%\%%d\mingw64\bin"
if not defined MINGW if exist "C:\mingw64\bin\g++.exe" set "MINGW=C:\mingw64\bin"
if not defined MINGW if exist "C:\msys64\ucrt64\bin\g++.exe" set "MINGW=C:\msys64\ucrt64\bin"
if not defined MINGW if exist "C:\msys64\mingw64\bin\g++.exe" set "MINGW=C:\msys64\mingw64\bin"
if defined MINGW set "PATH=%MINGW%;%PATH%"

g++ --version >nul 2>&1
if errorlevel 1 goto semCompilador

set "ALVO="
for /f "delims=" %%m in ('g++ -dumpmachine') do set "ALVO=%%m"
REM MinGW-w64 responde algo como "x86_64-w64-mingw32"; o antigo, "mingw32".
if "!ALVO:w64=!"=="!ALVO!" goto compiladorAntigo
echo Compilador: g++ !ALVO!

REM ---- 2. raylib (so na primeira vez) ---------------------------------------
if exist "%OBJ%\libraylib.a" goto jogo
echo Compilando a raylib (so na primeira vez, aguarde uns instantes)...
for %%f in (rcore rshapes rtextures rtext rmodels raudio utils rglfw) do (
  echo   %%f.c
  gcc -c -O2 -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -I%RAYLIB% -I%RAYLIB%\external\glfw\include %RAYLIB%\%%f.c -o %OBJ%\%%f.o
  if errorlevel 1 goto falhou
)
ar rcs %OBJ%\libraylib.a %OBJ%\rcore.o %OBJ%\rshapes.o %OBJ%\rtextures.o %OBJ%\rtext.o %OBJ%\rmodels.o %OBJ%\raudio.o %OBJ%\utils.o %OBJ%\rglfw.o
if errorlevel 1 goto falhou

REM ---- 3. o jogo ------------------------------------------------------------
:jogo
call :listarFontes
call :compilarIcone
echo Compilando o jogo (versao grafica)...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc -I%RAYLIB% %LOGICA% %GRAFICO% %ICONE% %OBJ%\libraylib.a -lopengl32 -lgdi32 -lwinmm -mwindows -static -o masmorra.exe
if errorlevel 1 goto falhou
call :compilarConsole
if errorlevel 1 goto falhou
call :criarAtalho

echo.
echo Compilacao OK: masmorra.exe
start "" "%PASTA%masmorra.exe"
exit /b 0

REM ---- MinGW antigo: so a versao de console ---------------------------------
:compiladorAntigo
call :listarFontes
echo.
echo AVISO: o seu g++ e o MinGW antigo (!ALVO!).
echo Ele nao consegue compilar a versao grafica, so a de console.
echo.
call :compilarIcone
call :compilarConsole
if errorlevel 1 goto falhou
echo Versao de console pronta: masmorra_console.exe
echo.
echo Para compilar a versao grafica, instale o MinGW-w64 (so uma vez).
echo Abra o terminal e rode:
echo.
echo     winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
echo.
echo Depois rode este arquivo de novo. Ele encontra o compilador novo sozinho.
pause
exit /b 0

:semCompilador
echo ERRO: g++ nao encontrado. Instale o MinGW-w64 (so uma vez). No terminal:
echo.
echo     winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
echo.
echo Depois rode este arquivo de novo.
pause
exit /b 1

:falhou
echo.
echo Ocorreu um erro na compilacao. Veja as mensagens acima.
echo (Se o jogo estiver aberto, feche-o e rode este arquivo de novo.)
pause
exit /b 1

REM ---- sub-rotinas ----------------------------------------------------------
:listarFontes
set "LOGICA="
for %%f in (src\*.cpp) do set "LOGICA=!LOGICA! %%f"
set "GRAFICO="
for %%f in (src\grafico\*.cpp) do set "GRAFICO=!GRAFICO! %%f"
set "CONSOLE="
for %%f in (src\console\*.cpp) do set "CONSOLE=!CONSOLE! %%f"
exit /b 0

:compilarConsole
echo Compilando o jogo (versao console)...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc %LOGICA% %CONSOLE% %ICONE% -static -o masmorra_console.exe
exit /b %errorlevel%

REM Icone do .exe (src\grafico\icone.rc + masmorra.ico). Se o windres nao
REM existir, o jogo sai sem icone, mas funciona igual.
:compilarIcone
set "ICONE="
windres src\grafico\icone.rc -O coff -o build\icone.o >nul 2>&1
if errorlevel 1 (
  echo AVISO: nao deu para colocar o icone no .exe ^(windres nao encontrado^).
  exit /b 0
)
set "ICONE=build\icone.o"
exit /b 0

REM Atalho "Masmorra" na Area de Trabalho, so na primeira vez (se voce
REM apagar o atalho, ele nao volta; para criar de novo, apague build\atalho.ok).
:criarAtalho
if exist "build\atalho.ok" exit /b 0
powershell -NoProfile -ExecutionPolicy Bypass -Command "$l = Join-Path ([Environment]::GetFolderPath('Desktop')) 'Masmorra.lnk'; $s = (New-Object -ComObject WScript.Shell).CreateShortcut($l); $s.TargetPath = '%PASTA%masmorra.exe'; $s.WorkingDirectory = '%PASTA%'; $s.Description = 'Masmorra'; $s.Save()" >nul 2>&1
if errorlevel 1 (
  echo AVISO: nao deu para criar o atalho na Area de Trabalho.
  exit /b 0
)
echo Atalho "Masmorra" criado na Area de Trabalho.
echo ok> "build\atalho.ok"
exit /b 0
