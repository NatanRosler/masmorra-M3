// cenario.h
// -----------------------------------------------------------------------------
// Etapa 7: o "mapa vivo".
//
//   ARMADILHAS  escondidas no chão. Quem pisa (jogador ou monstro) se fere.
//               O jogador pode percebê-las ao passar perto (Destreza ajuda).
//   CAIXAS      obstáculos interativos. O jogador empurra; se não der para
//               empurrar, ele quebra a caixa. Uma caixa empurrada para cima
//               de uma armadilha a desarma. O ogro e o chefe despedaçam as
//               caixas que encontram pelo caminho.
//
// Assim o mapa muda pela ação do jogador (caixas, portas), dos monstros
// (caixas quebradas, armadilhas disparadas) e dos NPCs (ver npcs.h).
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>

struct Jogo;
struct Andar;
struct Monstro;

struct Armadilha {
    int x, y;
    bool ativa;        // false = desarmada
    bool descoberta;   // o jogador já sabe que ela está ali
};

const int MAX_ARMADILHAS = 12;
const int TEMPO_ATORDOADO_MS = 5000;   // monstro atingido por uma caixa

// Espalha caixas e armadilhas num andar recém-gerado (depois dos monstros e
// dos itens, para não cair em cima deles).
void colocarCenario(Andar& andar, int numeroDoAndar, uint32_t semente, int chegadaX, int chegadaY);

// Índice da armadilha ativa em (x, y), ou -1.
int armadilhaEm(const Andar& andar, int x, int y);

// Depois de cada passo do jogador: dispara a armadilha onde ele pisou e
// procura armadilhas escondidas em volta.
void armadilhaPegaJogador(Jogo& jogo);

// Um monstro pisou em (m.x, m.y): se houver armadilha, ele se fere.
void armadilhaPegaMonstro(Jogo& jogo, Monstro& m);

// O jogador andou para dentro de uma caixa: empurra, quebra (se estiver presa)
// ou, se houver um monstro logo atrás, a caixa bate nele e o deixa atordoado.
// Devolve true se gastou o passo.
bool empurrarCaixa(Jogo& jogo, int dx, int dy);

// O monstro despedaça as caixas encostadas nele.
void quebrarCaixasPerto(Jogo& jogo, Monstro& m);
