// dificuldade.cpp
#include "dificuldade.h"

const Dificuldade DIFICULDADES[NUM_DIFICULDADES] = {
    //  nome      descrição
    //  vida  ataque lentidão armadilha  XP  vidas poções pontos loja
    { "Fácil",   "Monstros fracos e lentos, 5 vidas e 6 poções. Para conhecer o jogo.",
         -15,    75,    130,      6,     150,   5,    6,     50,   75 },
    { "Médio",   "Equilibrado: 3 vidas, 3 poções e monstros na medida certa.",
         10,    100,    110,     10,     125,   3,    3,    100,  100 },
    { "Difícil", "A masmorra da apresentação: monstros fortes, 3 vidas e 3 poções.",
         30,    125,    100,     15,     100,   3,    3,    150,  125 },
};

static int nivelAtual = MEDIO;

void definirDificuldade(int nivel) {
    if (nivel >= 0 && nivel < NUM_DIFICULDADES) nivelAtual = nivel;
}

const Dificuldade& dificuldadeAtual() {
    return DIFICULDADES[nivelAtual];
}

int nivelDaDificuldade() {
    return nivelAtual;
}
