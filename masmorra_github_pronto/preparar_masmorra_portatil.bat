@echo off
setlocal EnableExtensions
REM MASMORRA ORIGINAL 2D - empacotador para Windows
REM Coloque este arquivo na mesma pasta de CMakeLists.txt e da pasta build.
REM Execute depois de compilar o jogo com CMake.

cd /d "%~dp0"
if not exist "build\masmorra_2d.exe" (
    echo ERRO: Nao encontrei build\masmorra_2d.exe
    echo Abra o terminal nesta pasta e execute: cmake --build build --parallel 4
    echo Depois execute este arquivo novamente.
    pause
    exit /b 1
)

set "SAIDA=dist\Masmorra_2D_Windows"
if not exist "%SAIDA%" mkdir "%SAIDA%"
copy /Y "build\masmorra_2d.exe" "%SAIDA%\masmorra_2d.exe" >nul
if errorlevel 1 (
    echo ERRO: Nao consegui copiar o executavel. Feche o jogo e tente novamente.
    pause
    exit /b 1
)

REM Copia DLLs produzidas na pasta build, caso haja alguma.
for %%D in ("build\*.dll") do if exist "%%~fD" copy /Y "%%~fD" "%SAIDA%\" >nul

REM Descobre a pasta do mesmo compilador GCC usado no terminal.
set "GCC_BIN="
for /f "delims=" %%G in ('where g++ 2^>nul') do if not defined GCC_BIN set "GCC_BIN=%%~dpG"

REM Algumas versoes do MinGW precisam destas DLLs em outros PCs.
if defined GCC_BIN (
    for %%D in (libstdc++-6.dll libgcc_s_seh-1.dll libgcc_s_dw2-1.dll libgcc_s_sjlj-1.dll libwinpthread-1.dll libssp-0.dll) do (
        if exist "%GCC_BIN%%%D" copy /Y "%GCC_BIN%%%D" "%SAIDA%\" >nul
    )
) else (
    echo AVISO: Nao localizei g++. Se o jogo pedir DLLs em outro PC,
    echo copie as DLLs correspondentes do compilador usado na compilacao.
)

echo.
echo Pasta pronta: %CD%\%SAIDA%
echo Copie a PASTA INTEIRA para outro computador Windows 64 bits.
echo.
where powershell.exe >nul 2>nul
if not errorlevel 1 (
    powershell.exe -NoProfile -Command "Compress-Archive -Path 'dist\Masmorra_2D_Windows\*' -DestinationPath 'dist\Masmorra_2D_Windows.zip' -Force"
    if not errorlevel 1 echo ZIP criado em: %CD%\dist\Masmorra_2D_Windows.zip
)
echo.
echo IMPORTANTE: teste o ZIP em outro Windows. Pode ser necessario
 echo instalar os componentes visuais do Windows ou outras dependencias.
echo.
pause
