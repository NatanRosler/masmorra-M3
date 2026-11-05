#!/bin/sh
# ---------------------------------------------------------------------------
# Compila o jogo no Linux ou no Mac.
#
# Linux: -static gera um executável que roda em qualquer distribuição,
#        sem depender das bibliotecas instaladas.
# Mac:   a Apple não permite executável 100% estático. Em compensação,
#        "-arch arm64 -arch x86_64" gera um binário universal, que roda
#        nos Macs com chip Apple (M1/M2/...) e nos Macs Intel.
# ---------------------------------------------------------------------------
set -e
cd "$(dirname "$0")"

case "$(uname -s)" in
    Darwin) EXTRA="-arch arm64 -arch x86_64" ;;
    Linux)  EXTRA="-static" ;;
    *)      EXTRA="" ;;
esac

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra $EXTRA -o masmorra src/*.cpp
echo "Compilado: ./masmorra"
