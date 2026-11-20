// sintese.h
// -----------------------------------------------------------------------------
// "Instrumentos" gerados no código para os sons de ambiente, os efeitos e os
// passos (as músicas e as vozes continuam em 8 bits, no audio.cpp).
//
// Tudo é uma lista de amostras (float de -1 a 1, 22050 por segundo):
//   - ruído filtrado: vento, água, fogo, chiado (filtros passa-baixa,
//     passa-alta e passa-banda);
//   - corda dedilhada (algoritmo de Karplus-Strong): harpa;
//   - sino (síntese FM: uma onda modulando a outra): sinos, metal, gongo;
//   - reverb (eco de caverna, com o método de Schroeder: vários ecos curtos
//     que se realimentam).
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <vector>

namespace Sintese {

using Amostras = std::vector<float>;
const int TAXA = 22050;

// Sorteio próprio (não mexe no sorteio do jogo).
float acaso(float minimo, float maximo);

Amostras silencio(float segundos);
Amostras ruido(float segundos);
// Filtros (frequências em Hz).
Amostras passaBaixa(Amostras x, float freq);
Amostras passaAlta(Amostras x, float freq);
Amostras passaBanda(Amostras x, float de, float ate);
Amostras ressonancia(Amostras x, float freq, float q);   // "pico" estreito (vozes/sussurros)

// Instrumentos.
Amostras sino(float freq, float segundos, float volume, float razao = 2.0f, float indice = 1.5f,
              float decaimento = 3.0f);
Amostras corda(float freq, float segundos, float volume, float brilho = 0.996f);
Amostras gota(float freq, float volume);                 // "plic" de água
Amostras bolha(float freq, float volume);                // "bloop" (lava, veneno, Éter)
Amostras estalo(float volume);                           // fogo crepitando, garras
Amostras baque(float freq, float segundos, float volume);// pancada grave (passos pesados, coração)

// Montagem.
Amostras juntar(const std::vector<Amostras>& partes, float pausa = 0);   // uma depois da outra
void misturar(Amostras& destino, const Amostras& som, float emSegundos, float volume = 1.0f);
Amostras envelope(Amostras x, float ataque, float decaimento);           // entra e some (exponencial)
Amostras reverb(const Amostras& x, float tamanho, float mistura);
void normalizar(Amostras& x, float pico);
// Para tocar em loop sem "emenda": o fim se mistura com o começo.
Amostras emLoop(Amostras x, float segundosDeEmenda);

}  // namespace Sintese
