// aleatorio.h
// -----------------------------------------------------------------------------
// Gerador de números aleatórios com SEMENTE (seed).
//
// Por que não usar rand() ou <random>? Porque o resultado deles muda de um
// compilador para outro: a mesma semente geraria uma masmorra no Windows e
// outra no Mac. Com um gerador próprio, a semente 1234 gera exatamente a
// mesma masmorra em qualquer computador — ótimo para testar e para mostrar
// na apresentação.
//
// Algoritmo: xorshift32 (George Marsaglia, 2003). Três operações de
// deslocamento e XOR "embaralham" os bits do estado a cada número sorteado.
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>

struct GeradorAleatorio {
    uint32_t estado;
};

// Prepara o gerador com uma semente (qualquer número).
void iniciarGerador(GeradorAleatorio& g, uint32_t semente);

// Próximo número "cru" de 32 bits.
uint32_t proximoNumero(GeradorAleatorio& g);

// Número inteiro entre minimo e maximo (incluindo os dois).
int sortear(GeradorAleatorio& g, int minimo, int maximo);

// true com a porcentagem indicada (ex.: chance(g, 30) = 30% de chance).
bool chance(GeradorAleatorio& g, int porcentagem);

// Combina dois números num terceiro bem diferente. Usado para criar a
// semente de cada andar a partir da semente da partida.
uint32_t misturar(uint32_t a, uint32_t b);

// Uma semente "imprevisível", baseada no relógio.
uint32_t sementeDoRelogio();
