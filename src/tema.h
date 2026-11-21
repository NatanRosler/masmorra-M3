// tema.h
// -----------------------------------------------------------------------------
// Cada trecho da masmorra tem o seu "tema": muda o visual e algumas regras.
//
//   andares 1-3  MINAS                 pedra marrom e tochas (como sempre)
//   andares 4-6  CAVERNA ALAGADA       poças d'água: andar nelas é 2x mais lento;
//                                      monstros de Água batem 20% mais forte
//   andares 7-9  A CRIPTA ESQUECIDA    lápides (obstáculos que não tapam a
//                                      visão), velas verdes e névoa (v1.23)
//   andares 10-12 PROFUNDEZAS VULCÂNICAS poças de lava que queimam (e
//                                      iluminam); monstros de Fogo +20%
//   andar 13     SALA DO TRONO         onde o Senhor das Profundezas espera
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include "visao.h"

struct Andar;

enum Tema { TEMA_MINAS, TEMA_CAVERNA, TEMA_CRIPTA, TEMA_VULCAO, TEMA_TRONO, NUM_TEMAS };

struct FichaDoTema {
    const char* nome;          // "CAVERNA ALAGADA"
    const char* frase;         // aparece ao chegar
    int elementoFortalecido;   // monstros desse elemento batem mais forte (-1 = nenhum)
    Luz luzDaTocha;            // cor da luz das tochas
};

extern const FichaDoTema FICHAS_DOS_TEMAS[NUM_TEMAS];

int temaDoAndar(int numeroDoAndar);

// % do ataque de um monstro deste elemento neste andar (100 ou 120).
int bonusDoAmbiente(int numeroDoAndar, int elemento);

// Poças d'água (caverna) ou poças de lava (vulcão), logo depois de gerar
// o mapa. A lava nunca fecha um caminho: dá sempre para desviar dela.
void decorarAndar(Andar& andar, int numeroDoAndar, uint32_t semente);

// Quanto tempo vale um passo em (x, y): 2 na água, 1 no resto.
int multiplicadorDoPasso(const Andar& andar, int x, int y);
