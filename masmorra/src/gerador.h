// gerador.h
// -----------------------------------------------------------------------------
// Geração procedural de andares, no estilo do Brogue ("acreção"):
//
//   1. Coloca uma primeira sala grande em algum lugar do mapa.
//   2. Repete várias vezes: desenha uma sala nova num "molde" (sala retangular,
//      em cruz, circular ou caverna, às vezes com um corredor saindo dela) e
//      procura uma parede do mapa onde ela se encaixe sem encostar em nada.
//      Se encaixar, a sala é "carimbada" ali e ganha uma porta.
//   3. Abre atalhos em paredes finas que separam lugares muito distantes
//      (calculado com BFS), para o mapa ter voltas e não só becos.
//   4. Coloca as escadas dentro das paredes; a de descida fica longe da
//      de subida (de novo com BFS).
//   5. Prende algumas tochas nas paredes.
//   6. Escolhe uma sala sem saída para ser a sala do tesouro e tranca a porta.
//
// Como tudo vem de um GeradorAleatorio com semente, a mesma semente gera
// sempre o mesmo andar.
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include "mapa.h"

struct Escadas {
    int sobeX, sobeY;                 // escada que sobe '<' (fica na parede)
    int chegadaSobeX, chegadaSobeY;   // chão em frente a ela
    bool temDescida;                  // o último andar não tem descida
    int desceX, desceY;               // escada que desce '>'
    int chegadaDesceX, chegadaDesceY;
};

// Sala do tesouro: uma sala sem saída cuja única porta fica trancada.
struct SalaDoTesouro {
    bool existe;
    int portaX, portaY;      // a porta trancada
    int dentroX, dentroY;    // uma célula de chão do lado de dentro
};

// Gera um andar completo. Devolve quantas salas foram criadas.
// Se "tesouro" não for nullptr, também escolhe e tranca a sala do tesouro.
int gerarAndar(Mapa& mapa, Escadas& escadas, uint32_t semente, bool ultimoAndar,
               SalaDoTesouro* tesouro = nullptr);
