// dificuldade.h
// -----------------------------------------------------------------------------
// Nível de dificuldade da partida, escolhido na criação do personagem.
// Cada nível é só uma tabela de números: as regras do jogo leem os valores
// de dificuldadeAtual() em vez de usar números fixos.
// -----------------------------------------------------------------------------
#pragma once

enum NivelDeDificuldade { FACIL, MEDIO, DIFICIL, NUM_DIFICULDADES };

struct Dificuldade {
    const char* nome;
    const char* descricao;
    int vidaDosMonstros;        // % a mais (ou a menos) de vida nos monstros
    int ataqueDosMonstros;      // % do ataque normal dos monstros
    int lentidaoDosMonstros;    // % do intervalo entre os passos (maior = mais lentos)
    int danoDasArmadilhas;      // % da vida máxima que a armadilha tira
    int experiencia;            // % da experiência ganha nas batalhas
    int vidas;                  // vidas no começo
    int pocoes;                 // poções de vida no começo
    int pontuacao;              // % da pontuação final (mais difícil vale mais)
};

extern const Dificuldade DIFICULDADES[NUM_DIFICULDADES];

// A dificuldade da partida atual (MÉDIO até alguém escolher outra).
void definirDificuldade(int nivel);
const Dificuldade& dificuldadeAtual();
int nivelDaDificuldade();
