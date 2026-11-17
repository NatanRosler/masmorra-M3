// jogo.h
// -----------------------------------------------------------------------------
// Estado completo de uma partida e as regras de cada turno.
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "mapa.h"
#include "gerador.h"
#include "visao.h"
#include "aleatorio.h"
#include "monstros.h"
#include "itens.h"

// Quantos andares a masmorra tem. O chefe final ficará no último.
const int NUM_ANDARES = 10;

// Atributos-base do jogador (como os de um monstro): os valores de verdade
// crescem com o nível. Ver recalcularAtributos().
const int BASE_VIDA_JOGADOR       = 70;
const int BASE_ATAQUE_JOGADOR     = 50;
const int BASE_DEFESA_JOGADOR     = 65;
const int BASE_VELOCIDADE_JOGADOR = 65;
const int NIVEL_INICIAL = 5;
const int NIVEL_MAXIMO = 50;
const int VIDAS_INICIAIS = 3;
const int POCOES_INICIAIS = 3;

struct Jogador {
    int x, y;
    int nivel;
    int experiencia;           // total acumulado
    int vida, vidaMaxima;
    int ataque, defesa, velocidade;

    int golpes[MAX_GOLPES];    // índices na tabela GOLPES (batalha.h)
    int pp[MAX_GOLPES];        // usos restantes de cada golpe
    int numGolpes;

    // Mochila: quantos itens de cada tipo o jogador carrega.
    int mochila[NUM_TIPOS_DE_ITEM];
    // Equipamento: PONTEIROS para a ficha do item na tabela TIPOS_DE_ITEM
    // (nullptr = mão vazia). Assim, o bônus vem direto da ficha.
    const TipoItem* arma;
    const TipoItem* escudo;
    // Bônus permanentes dos cristais (power-ups).
    int bonusVida, bonusAtaque, bonusDefesa, bonusVelocidade;

    int vidas;                 // chances que ainda restam (começa com 3)
};

// Cada andar é gerado na primeira visita e guardado, para que o jogador
// encontre o mesmo andar (e os mesmos monstros) se subir de volta.
struct Andar {
    bool gerado;
    Mapa mapa;
    Escadas escadas;
    // Memória do jogador: células que ele já viu alguma vez neste andar.
    bool descoberto[MAPA_ALTURA][MAPA_LARGURA];

    Monstro monstros[MAX_MONSTROS];
    int numMonstros;

    ItemNoChao itens[MAX_ITENS_NO_CHAO];
    int numItens;
    SalaDoTesouro tesouro;     // sala trancada (se este andar tiver uma)

    // Rastro de cheiro que o jogador deixa por onde passa. Quanto maior o
    // número, mais recente o cheiro. Monstros que perderam o jogador de
    // vista seguem o cheiro mais forte.
    int cheiro[MAPA_ALTURA][MAPA_LARGURA];
};

struct Jogo {
    uint32_t semente;               // semente da partida (mostrada na ajuda)
    GeradorAleatorio sorte;         // sorteios do combate
    bool comMonstros;               // false só nos testes automáticos
    bool modoAutomatico;            // true só nos testes: batalhas sem tela
    int batalhaPendente;            // monstro que vai lutar (-1 = nenhum)
    Andar andares[NUM_ANDARES];     // andares[0] é o andar 1
    int andar;                      // andar atual (1 = primeiro)

    Jogador jogador;

    // Recalculados a cada turno por atualizarVisao():
    bool linhaDeVisao[MAPA_ALTURA][MAPA_LARGURA]; // em linha reta do jogador (com ou sem luz)
    bool visivel[MAPA_ALTURA][MAPA_LARGURA];      // o que o jogador vê agora (linha + luz)
    Luz luz[MAPA_ALTURA][MAPA_LARGURA];           // quanta luz chega em cada célula
    int distanciaAteJogador[MAPA_ALTURA][MAPA_LARGURA]; // BFS a partir do jogador

    // Tempo real.
    long long tempoMs;              // tempo de jogo no mapa (não conta menus e batalhas)
    int esperaDoJogadorMs;          // quanto falta para o jogador poder dar outro passo
    bool descansando;               // tecla Z: recupera vida (e faz menos barulho)
    int msDeDescanso;
    int segundosDeDescanso;

    int passos;                     // quantos passos o jogador já deu
    int monstrosMortos;
    int itensColetados;
    int rodadasDaUltimaBatalha;     // estatística (usada nos testes de equilíbrio)

    std::vector<std::string> mensagensDoTurno; // aparecem no topo da tela
    std::vector<std::string> historico;        // últimas mensagens (tecla M)

    bool morto;                     // as vidas acabaram: fim de jogo
    std::string causaDaMorte;       // "derrotado por um ogro"
    bool rodando;                   // false = sair do jogo
};

// comMonstros = false serve só para os testes automáticos.
void iniciarJogo(Jogo& jogo, uint32_t semente, bool comMonstros = true);

// Mapa e andar onde o jogador está.
Mapa& mapaAtual(Jogo& jogo);
const Mapa& mapaAtual(const Jogo& jogo);
Andar& andarAtual(Jogo& jogo);
const Andar& andarAtual(const Jogo& jogo);

// Recalcula luz e visão a partir da posição do jogador e atualiza a memória.
void atualizarVisao(Jogo& jogo);

// Alcance da luz que o jogador carrega (diminui nos andares mais fundos).
int raioDaLuzDoJogador(int andar);

// Mostra uma mensagem no topo da tela (e guarda no histórico).
void adicionarMensagem(Jogo& jogo, const std::string& texto);

// Recalcula ataque, defesa, velocidade e vida máxima a partir do nível,
// somando os bônus da arma, do escudo e dos cristais.
void recalcularAtributos(Jogador& jogador);

// Mostra uma caixa com opções e deixa o jogador escolher uma (setas + Enter).
// "descricoes" (pode ficar vazio) aparece embaixo da opção selecionada.
// Devolve o índice escolhido ou -1 se ele apertar Esc.
int escolherEmCaixa(const Jogo& jogo, const std::string& titulo,
                    const std::vector<std::string>& opcoes,
                    const std::vector<std::string>& descricoes);

// Recebe a tecla apertada e executa a ação correspondente.
void processarTecla(Jogo& jogo, int tecla);

// Faz o tempo do mapa andar "ms" milissegundos: monstros agem, o descanso
// recupera vida, batalhas começam. Chamada pelo laço principal o tempo todo.
void avancarTempo(Jogo& jogo, int ms);

// Intervalo mínimo entre dois passos do jogador (depende da velocidade).
int intervaloDoJogador(const Jogador& j);
