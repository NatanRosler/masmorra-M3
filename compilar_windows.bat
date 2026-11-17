@echo off
REM ---------------------------------------------------------------------------
REM  Compila o MASMORRA no Windows usando so o g++ (MinGW). Nao precisa de CMake.
REM
REM  1a vez: compila tambem a raylib (pasta externo\raylib-5.5). Demora ~1 min.
REM  Depois: so recompila o jogo, que e rapido.
REM
REM  Gera:  build\masmorra.exe          (versao grafica)
REM         build\masmorra_console.exe  (versao de terminal)
REM ---------------------------------------------------------------------------
setlocal enabledelayedexpansion
cd /d "%~dp0"

where g++ >nul 2>&1
if errorlevel 1 (
  echo ERRO: g++ nao encontrado. Instale o MinGW-w64 e coloque a pasta bin no PATH.
  pause
  exit /b 1
)

set RAYLIB=externo\raylib-5.5\src
set SAIDA=build
set OBJ=%SAIDA%\raylib
if not exist "%OBJ%" mkdir "%OBJ%"

REM ---- raylib (so na primeira vez) ------------------------------------------
if not exist "%OBJ%\libraylib.a" (
  echo Compilando a raylib ^(so na primeira vez, aguarde uns instantes^)...
  for %%f in (rcore rshapes rtextures rtext rmodels raudio utils rglfw) do (
    echo   %%f.c
    gcc -c -O2 -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -I%RAYLIB% -I%RAYLIB%\external\glfw\include %RAYLIB%\%%f.c -o %OBJ%\%%f.o
    if errorlevel 1 goto falhou
  )
  ar rcs %OBJ%\libraylib.a %OBJ%\rcore.o %OBJ%\rshapes.o %OBJ%\rtextures.o %OBJ%\rtext.o %OBJ%\rmodels.o %OBJ%\raudio.o %OBJ%\utils.o %OBJ%\rglfw.o
  if errorlevel 1 goto falhou
)

REM ---- arquivos do jogo -----------------------------------------------------
set LOGICA=
for %%f in (src\*.cpp) do set LOGICA=!LOGICA! %%f
set GRAFICO=
for %%f in (src\grafico\*.cpp) do set GRAFICO=!GRAFICO! %%f
set CONSOLE=
for %%f in (src\console\*.cpp) do set CONSOLE=!CONSOLE! %%f

echo Compilando o jogo (versao grafica)...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc -I%RAYLIB% %LOGICA% %GRAFICO% %OBJ%\libraylib.a -lopengl32 -lgdi32 -lwinmm -mwindows -static -o %SAIDA%\masmorra.exe
if errorlevel 1 goto falhou

echo Compilando o jogo (versao console)...
g++ -std=c++17 -O2 -Wall -Wextra -Isrc %LOGICA% %CONSOLE% -static -o %SAIDA%\masmorra_console.exe
if errorlevel 1 goto falhou

echo.
echo Compilacao OK: %SAIDA%\masmorra.exe
start "" "%SAIDA%\masmorra.exe"
exit /b 0

:falhou
echo.
echo Ocorreu um erro na compilacao. Veja as mensagens acima.
pause
exit /b 1
