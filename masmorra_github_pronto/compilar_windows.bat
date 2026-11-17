@echo off
cd /d "%~dp0"
where cmake >nul 2>&1
if errorlevel 1 (
  echo CMake nao encontrado. Instale o CMake e abra um novo terminal.
  pause
  exit /b 1
)
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
if errorlevel 1 goto falhou
cmake --build build --parallel 4
if errorlevel 1 goto falhou
if exist "build\masmorra_2d.exe" (
  echo Compilacao OK. Abrindo Masmorra 2D...
  "build\masmorra_2d.exe"
)
exit /b 0
:falhou
echo Ocorreu um erro na compilacao. Veja as mensagens acima.
pause
exit /b 1
