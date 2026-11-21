// dificuldade.cpp
#include "dificuldade.h"
#include "opcoes.h"

const Dificuldade DIFICULDADES[NUM_DIFICULDADES] = {
    //  nome      descrição
    //  vida  ataque lentidão armadilha  XP  vidas poções pontos loja  campeões   folga  chefe
    //                                                                (% e máx.)  nível  +nível
    { "Fácil",   "Monstros fracos e lentos, 5 vidas e 6 poções. Para conhecer o jogo.",
         -20,    60,    130,      6,     150,   5,    6,     50,   75,   6, 1,      -1,    1 },
    { "Médio",   "3 vidas e 3 poções. Do andar 3 em diante, os monstros ficam no máximo 2 níveis abaixo de você.",
          0,    75,    110,     10,     125,   3,    3,    100,  100,  12, 2,       2,    1 },
    { "Difícil", "Monstros fortes, 3 vidas e 3 poções. Do andar 3 em diante, no máximo 2 níveis abaixo de você.",
          5,    85,    100,     15,     100,   3,    3,    150,  125,  18, 3,       2,    1 },
    { "Impossível", "Ninguém voltou. Você vai ser o primeiro? 1 vida, 1 poção e, do andar 3 em diante, monstros sempre do seu nível.",
         40,    135,     90,     20,     100,   1,    1,    200,  150,  25, 4,       0,    3 },
};

static int nivelAtual = MEDIO;

bool dificuldadeLiberada(int nivel) {
    return nivel != IMPOSSIVEL || opcoes().impossivelLiberado;
}

void definirDificuldade(int nivel) {
    if (nivel >= 0 && nivel < NUM_DIFICULDADES) nivelAtual = nivel;
}

const Dificuldade& dificuldadeAtual() {
    return DIFICULDADES[nivelAtual];
}

int nivelDaDificuldade() {
    return nivelAtual;
}
