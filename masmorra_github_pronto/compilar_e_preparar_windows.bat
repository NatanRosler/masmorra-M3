@echo off
setlocal EnableExtensions
cd /d "%~dp0"
echo =========================================
echo MASMORRA ORIGINAL 2D - Compilar e empacotar
echo =========================================
where cmake >nul 2>&1
if errorlevel 1 (
  echo ERRO: CMake nao encontrado. Instale CMake e configure o PATH.
  pause
  exit /b 1
)
where g++ >nul 2>&1
if errorlevel 1 (
  echo ERRO: g++ nao encontrado. Abra o VS Code com o MinGW configurado.
  pause
  exit /b 1
)
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto erro
cmake --build build --parallel 4
if errorlevel 1 goto erro
call "%~dp0preparar_masmorra_portatil.bat"
if errorlevel 1 goto erro
exit /b 0
:erro
echo.
echo Falha: veja a primeira mensagem de erro acima.
pause
exit /b 1
