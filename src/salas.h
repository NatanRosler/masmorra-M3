// salas.h
// -----------------------------------------------------------------------------
// Salas especiais: uma ou duas surpresas por andar (nunca no andar do chefe).
//
//   FONTE CRISTALINA  cura toda a vida e os PP, uma vez; depois seca.
//   ALTAR DE SANGUE   pacto: perde 5 de vida máxima e ganha +2 de ataque.
//   BAÚ DOURADO       às vezes é um MÍMICO (batalha!). Vencendo: cristal e
//                     moedas. Se for um baú de verdade: um item bom.
//   GONGO DA ARENA    numa sala com uma porta só: aceitando o desafio, a porta
//                     tranca e vêm 3 ondas de monstros. Vencendo: cristal e
//                     moedas.
//   VIAJANTE PERDIDO  um NPC que troca 1 Poção de Vida (ou 30 moedas) por um
//                     cristal (ver npcs.cpp).
//
// Os objetos ficam no mapa como obstáculos: andar na direção deles usa.
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>

struct Jogo;
struct Andar;
struct Monstro;

enum TipoEspecial { ESP_FONTE, ESP_ALTAR, ESP_BAU, ESP_GONGO, NUM_TIPOS_ESPECIAIS };

const int MAX_ESPECIAIS = 3;
const int ONDAS_DA_ARENA = 3;

struct Especial {
    int tipo;               // TipoEspecial
    int x, y;
    bool usado;             // fonte seca, altar usado, baú aberto, arena vencida
    bool mimico;            // baú: é um mímico disfarçado?
    int onda;               // arena: 0 = esperando; 1 a 3 = onda em andamento
    int portaX, portaY;     // arena: a porta da sala
    int monstros[4];        // arena: os monstros da onda atual
    int numMonstros;
};

// Coloca as surpresas do andar (depois dos NPCs, nos lugares que sobraram).
void colocarSalasEspeciais(Andar& andar, int numeroDoAndar, uint32_t semente);

// Índice do objeto especial em (x, y), ou -1.
int especialEm(const Andar& andar, int x, int y);

// O jogador andou na direção do objeto. Devolve true se gastou o turno.
bool usarEspecial(Jogo& jogo, int indice);

// A arena: confere se a onda acabou (e chama a próxima ou dá o prêmio).
void atualizarSalasEspeciais(Jogo& jogo);

// A porta (x, y) é a de uma arena em andamento? (não abre nem com chave)
bool portaDaArenaFechada(const Andar& andar, int x, int y);

// O jogador desmaiou: o desafio da arena acaba (dá para tentar de novo).
void desistirDaArena(Jogo& jogo);

// Um mímico foi derrotado: ele deixa cair um cristal e moedas.
void recompensaDoMimico(Jogo& jogo, const Monstro& m);
