// npcs.h
// -----------------------------------------------------------------------------
// Etapa 8: personagens que não lutam. Eles contam a história, dão itens e
// mudam o mapa (o mineiro destranca a sala do tesouro).
// Para conversar, basta andar na direção do NPC.
// -----------------------------------------------------------------------------
#pragma once

#include "visao.h"

struct Jogo;
struct Andar;

enum TipoNpc {
    NPC_EXPLORADORA,   // andar 1: conta como a masmorra funciona e dá uma poção
    NPC_MINEIRO,       // andar 4: destranca a sala do tesouro
    NPC_ESPIRITO,      // andar 7: avisa sobre o chefe e cura o jogador
    NPC_COMERCIANTE,   // andares 3, 6 e 9: vende e compra itens (loja.cpp)
    NUM_TIPOS_DE_NPC
};

struct FichaNpc {
    const char* nome;
    char simbolo;
    CorRGB cor;
    int andar;         // em que andar aparece (a primeira vez)
    int repeteACada;   // 0 = só nesse andar; 3 = de novo a cada 3 andares
};

extern const FichaNpc FICHAS_DOS_NPCS[NUM_TIPOS_DE_NPC];

struct Npc {
    int tipo;          // TipoNpc
    int x, y;
    bool conversou;    // a ajuda (item, cura, porta) só acontece na 1a conversa
    int estoque[2];    // comerciante: itens especiais à venda (-1 = vendido/nenhum)
    int pocoesAVenda[4];   // comerciante: quantas de cada poção ele ainda tem (loja.cpp)
};

const int MAX_NPCS = 2;

// A lojinha do comerciante: uma sala com uma porta só, perto da chegada.
// Dentro dela (Andar::zonaSegura) nenhum monstro nasce, entra ou ataca.
struct SalaDaLoja {
    bool existe;           // false = o andar não tinha uma sala assim (só o tapete)
    int portaX, portaY;
    int balcaoX, balcaoY;  // onde o comerciante fica
};

// O comerciante aparece neste andar?
bool temComercianteNoAndar(int numeroDoAndar);

// Escolhe a sala da loja logo depois de gerar o mapa (antes dos monstros).
void prepararSalaDaLoja(Andar& andar, int numeroDoAndar);

// Coloca os NPCs deste andar perto da chegada do jogador.
void colocarNpcs(Andar& andar, int numeroDoAndar, int chegadaX, int chegadaY);

// Índice do NPC em (x, y), ou -1.
int npcEm(const Andar& andar, int x, int y);

// Mostra a conversa e aplica a ajuda do NPC.
void conversarComNpc(Jogo& jogo, int indice);
