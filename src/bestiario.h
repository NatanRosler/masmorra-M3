// bestiario.h
// -----------------------------------------------------------------------------
// Bestiário: o "álbum" dos monstros que o jogador já encontrou. Vale para
// todas as partidas (fica em bestiario.txt, ao lado do jogo).
//
//   - visto:       já apareceu na sua frente (antes disso, só a silhueta "???")
//   - derrotados:  quantos você já venceu (com 1 vitória aparecem os atributos)
//   - campeões:    quantos campeões (elites) você já venceu
//   - golpes:      cada golpe aparece na ficha depois que o monstro o usa
//   - elementos:   com quais elementos a espécie já apareceu (é sorteado a
//                  cada partida)
//
// Os testes automáticos e os bots nunca gravam: o arquivo só é gravado
// depois que o jogo chama carregarBestiario() (os dois main.cpp fazem isso).
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>

struct Jogo;
struct Monstro;

struct FichaDoBestiario {
    bool visto;
    int derrotados;
    int campeoesDerrotados;
    uint64_t golpesVistos;     // bit i = GOLPES[i] já foi visto
    int elementosVistos;       // bit e = já apareceu com o elemento e
};

// A ficha da espécie (índice em TIPOS_DE_MONSTRO).
const FichaDoBestiario& fichaDoBestiario(int tipo);

// Lê bestiario.txt e passa a gravar cada novidade nele.
void carregarBestiario();
// Apaga tudo (Opções > Zerar bestiário).
void zerarBestiario();

// Quantas espécies já foram vistas (e quantas existem no bestiário).
int especiesVistas();
int especiesDoBestiario();

// Chamadas pela lógica do jogo:
void registrarAvistado(Jogo& jogo, const Monstro& m);      // apareceu na tela
void registrarGolpe(int tipo, int golpe);                   // usou um golpe na batalha
void registrarDerrota(const Monstro& m);                    // foi vencido

// A espécie já apareceu NESTA partida? (Só então o bestiário mostra o
// elemento dela nesta partida.)
bool vistaNaPartida(const Jogo& jogo, int tipo);

// Texto curto de cada espécie e em que andares ela aparece.
std::string descricaoDaEspecie(int tipo);
std::string andaresDaEspecie(int tipo);
