// diario.h
// -----------------------------------------------------------------------------
// O Diário (tecla J, v1.26): acompanha o que já existe no jogo, para o jogador
// não esquecer o que falta. Não cria nada novo nem dá prêmios.
//
//   - Missão principal: descer até o andar 13 e vencer o Senhor das
//     Profundezas, com os 4 mini-chefes pelo caminho.
//   - Missões do andar atual: aparecem quando o jogador descobre a coisa
//     (a escada, a sala do chefe, a porta do tesouro, a fonte, uma pessoa...)
//     e ficam marcadas quando ele completa.
//   - Anotações: o que as pessoas contaram e os chefes vencidos.
//
// Tudo sai do estado do jogo que já vai no save (nada novo para guardar).
// As telas ficam em tela.h (mostrarDiario).
// -----------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

struct Jogo;

// O que é a missão (decide o desenho no cartão).
enum TipoDeMissao { MISSAO_CHEFE, MISSAO_ESCADA, MISSAO_TESOURO, MISSAO_FONTE, MISSAO_ALTAR,
                    MISSAO_BAU, MISSAO_ARENA, MISSAO_PESSOA };

struct Missao {
    int tipo;            // TipoDeMissao
    std::string titulo;  // curto: "Escada", "Rei Goblin", "Baú dourado"
    std::string detalhe; // uma ou duas palavras: "para o andar 5", "pode ser um mímico!"
    bool feita;
    bool obrigatoria;    // sem ela não dá para descer (escada, chefe)
    int especie;         // MISSAO_CHEFE: o monstro (-1 = ainda não visto)
    int npc;             // MISSAO_PESSOA: o TipoNpc
};

// Uma etapa da missão principal: um chefe e o andar dele.
struct EtapaPrincipal {
    int andar;
    std::string nome;   // vazio enquanto o jogador não chegou ao andar
    int especie;        // -1 enquanto o jogador não chegou ao andar
    bool vencido;
    bool semChefe;      // andar criado numa versão antiga (sem sala de chefe)
};

struct Anotacao {
    int andar;
    std::string quem;                 // "Eldra, a exploradora", "Rei Goblin"...
    std::vector<std::string> pontos;  // 2 ou 3 frases curtas
    bool chefe;                       // um chefe vencido (e não uma conversa)
    int especie;                      // chefe: o monstro
    int npc;                          // conversa: o TipoNpc
};

std::vector<EtapaPrincipal> etapasDaMissaoPrincipal(const Jogo& jogo);
std::vector<Missao> missoesDoAndar(const Jogo& jogo);
std::vector<Anotacao> anotacoesDoDiario(const Jogo& jogo);
