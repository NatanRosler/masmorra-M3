#pragma once
// sala_do_chefe.h - v1.24: as salas feitas à mão dos chefes.
//
// Nos andares 3, 6, 9 e 12 um mini-chefe guarda a escada de descer, que fica
// dentro da sala dele (atrás dele). No andar 13, o Senhor das Profundezas
// espera no salão do trono. Cada sala é desenhada em texto (sala_do_chefe.cpp)
// e "carimbada" num lugar do andar já gerado, longe da chegada, sem fechar
// nenhum caminho que já existia. A única entrada é um portão.
#include <cstdint>

struct Jogo;
struct Andar;
struct Monstro;

enum TipoDeSala {
    SALA_DO_REI,          // Rei Goblin (andar 3, minas)
    NINHO_DA_RAINHA,      // Rainha Aranha (andar 6, caverna)
    ALTAR_DO_NECROMANTE,  // Necromante (andar 9, cripta)
    FORJA_DO_GOLEM,       // Golem de Magma (andar 12, vulcão)
    SALAO_DO_TRONO,       // Senhor das Profundezas (andar 13)
    NUM_SALAS_DE_CHEFE
};

// Qual sala o andar tem (-1 = nenhuma).
int salaDoAndar(int numeroDoAndar);

// Carimba a sala no andar (chamada no fim de prepararAndar). Tira de dentro
// o que estava lá (monstros e itens mudam de lugar), troca a escada de
// descer pela da sala e, com monstros, coloca o chefe e os 2 ajudantes. No
// último andar, leva o chefe e os ogros que já existem para os lugares deles.
// Devolve false se não coube em lugar nenhum (o andar fica como era).
bool carimbarSalaDoChefe(Jogo& jogo, Andar& andar, int numeroDoAndar, uint32_t semente);

// (x, y) é chão de dentro da sala (sem contar as paredes e o portão)?
bool dentroDaSalaDoChefe(const Andar& andar, int x, int y);

// Índice do chefe vivo da sala (mini-chefe ou o chefe final), ou -1.
int chefeDaSala(const Andar& andar);

// v1.25: a escada de descer fica lacrada enquanto o mini-chefe do andar está
// vivo. Devolve o índice dele (-1 = a escada está livre).
int miniChefeQueLacraAEscada(const Andar& andar);

// Enfeite "sólido" (trono, coluna, forja...): ninguém passa por cima.
bool enfeiteSolido(char enfeite);

// Tipo do ajudante de cada mini-chefe (goblin, aranha, esqueleto, salamandra).
int tipoDoAjudante(int tipoDeSala);

// Vida fixa do mini-chefe de cada sala, na dificuldade atual; o ajuste do
// nível dele (em relação ao do jogador) e o ataque, em % do normal.
int vidaDoMiniChefe(int tipoDeSala);
int ajusteDeNivelDoMiniChefe(int tipoDeSala);
int ataqueDoMiniChefe(int tipoDeSala);
// Sala do mini-chefe de uma espécie (-1 se não for mini-chefe).
int salaDoMiniChefe(int tipoDoMonstro);

// O que aparece na cena de entrada: o apelido do mini-chefe e a fala dele
// (outra, mais curta, quando o jogador volta depois de perder).
const char* tituloDoMiniChefe(int tipoDeSala);
const char* falaDoMiniChefe(int tipoDeSala, bool primeira);

// O jogador acabou de vencer um mini-chefe: moedas e o prêmio da sala.
void recompensaDoMiniChefe(Jogo& jogo, const Monstro& m);
