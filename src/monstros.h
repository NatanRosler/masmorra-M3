// monstros.h
// -----------------------------------------------------------------------------
// Monstros: tipos (a "ficha" de cada espécie), o estado de cada monstro vivo
// e a inteligência artificial que decide o que cada um faz no seu turno.
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>
#include "mapa.h"
#include "visao.h"
#include "batalha.h"

struct Jogo;   // definido em jogo.h
struct Andar;

// ---- Ficha de cada espécie --------------------------------------------------

// Características especiais (podem ser combinadas com |).
const int RAPIDO   = 1;  // anda duas vezes mais rápido no mapa
const int ERRATICO = 2;  // voa sem rumo: 1/3 das vezes se move ao acaso
const int MEDROSO  = 4;  // foge (no mapa e na batalha) quando está com pouca vida
const int EM_BANDO = 8;  // aparece em dupla (às vezes em trio)
const int QUEBRA_CAIXAS = 16; // despedaça as caixas que encontra no caminho
const int CHEFE    = 32; // o chefe final: só aparece no último andar
const int VIGIA    = 64; // gnomo vigia: ao ver o jogador, sopra a corneta (chama as salas vizinhas)
const int SOMBRIO  = 128; // sombra: invisível de longe (só aparece a 2 casas ou perto de uma tocha)

const int LINHAS_DA_ARTE = 7;

struct TipoMonstro {
    const char* nome;      // "rato"
    const char* artigo;    // "o" ou "a"
    char simbolo;          // como aparece no mapa
    CorRGB cor;
    Elemento elemento;     // elemento "de fábrica" (na partida vale o sorteado)
    // Atributos-base: junto com o nível, definem os atributos de verdade
    // (ver vidaPeloNivel e atributoPeloNivel em batalha.h).
    int baseVida, baseAtaque, baseDefesa, baseVelocidade;
    int experienciaBase;   // quanto de experiência vale derrotá-lo
    int golpes[MAX_GOLPES];
    int numGolpes;
    const char* arte[LINHAS_DA_ARTE]; // desenho para a tela de batalha
    int andarMinimo;       // em que andares aparece
    int andarMaximo;
    int frequencia;        // quanto maior, mais comum
    int especial;          // RAPIDO, ERRATICO, MEDROSO, EM_BANDO, VIGIA, SOMBRIO...
};

extern const TipoMonstro TIPOS_DE_MONSTRO[];
extern const int NUM_TIPOS_DE_MONSTRO;

// ---- Cada monstro no mapa ---------------------------------------------------

enum EstadoMonstro {
    DORMINDO,   // parado; pode acordar se o jogador chegar perto
    VAGANDO,    // anda pelo andar até um destino sorteado
    CACANDO,    // persegue o jogador
    FUGINDO     // foge do jogador (só os MEDROSOS, com pouca vida)
};

struct Monstro {
    bool vivo;
    int tipo;              // índice em TIPOS_DE_MONSTRO
    int x, y;
    int nivel;
    int vida;
    Elemento elemento;     // sorteado a cada partida (ver sortearElementosDasEspecies)
    int atordoadoMs;       // > 0: levou uma caixa e está tonto (não se mexe)
    EstadoMonstro estado;
    int destinoX, destinoY;    // para onde está indo quando VAGANDO
    int turnosSemVerJogador;   // quando CACANDO: desiste depois de um tempo
    int turnosSemBatalha;      // depois de uma fuga, demora a puxar briga de novo
    bool carregaChave;         // deixa a chave da sala do tesouro ao ser derrotado
    int tempoAcumulado;        // milissegundos desde a última ação (tempo real)
    // v1.17 (no fim da struct: o save antigo lê só a parte de cima, ver salvar.cpp)
    bool campeao;              // monstro elite: mais forte, aura dourada, mais prêmios
    int cornetas;              // gnomo vigia: quantas vezes já soprou a corneta (0, 1 ou 2)
};

const int MAX_MONSTROS = 40;
const int MAX_ESPECIES = 16;   // espaço reservado para a tabela de elementos sorteados

// Cor de cada elemento (usada para pintar os monstros e os textos).
extern const CorRGB CORES_DOS_ELEMENTOS[NUM_ELEMENTOS];

// Índice do chefe final na tabela TIPOS_DE_MONSTRO.
int tipoDoChefe();
int tipoDoMimico();

// Cria um monstro fora da geração do andar (mímico, ondas da arena). Ele já
// nasce caçando o jogador. Devolve o índice dele (-1 se não coube).
int criarMonstroExtra(Andar& andar, int tipo, int x, int y, int nivel, Elemento elemento);
// Sorteia uma espécie que costuma aparecer neste andar.
struct GeradorAleatorio;
int sortearTipoDoAndar(GeradorAleatorio& g, int numeroDoAndar);

// Sorteia o elemento de cada espécie nesta partida, de forma equilibrada:
// os 6 elementos são embaralhados e distribuídos em ordem de andar, então as
// espécies que aparecem juntas nos mesmos andares ficam com elementos diferentes.
void sortearElementosDasEspecies(Jogo& jogo, uint32_t semente);

// Coloca o chefe final no ponto do andar mais longe da chegada do jogador.
void colocarChefe(Andar& andar, int chegadaX, int chegadaY, Elemento elemento);

// O chefe acompanha o jogador: no começo de cada batalha ele fica com o nível
// do jogador + 1 (nunca abaixo de NIVEL_MINIMO_DO_CHEFE). A vida que ele já
// perdeu continua perdida (a fração da vida é mantida).
const int NIVEL_MINIMO_DO_CHEFE = 22;
void ajustarNivelDoChefe(Monstro& chefe, int nivelDoJogador);

// Golpes que o monstro usa na batalha: os da espécie, com um deles trocado
// pelo golpe do elemento sorteado.
void golpesDoMonstro(const Monstro& m, int golpes[MAX_GOLPES], int& numGolpes);

// Chance (em %) de o monstro se esquivar de um golpe.
int esquivaDoMonstro(const Monstro& m);

// Espalha os monstros de um andar recém-gerado, longe da chegada do jogador.
void povoarAndar(Andar& andar, int numeroDoAndar, uint32_t semente,
                 int chegadaX, int chegadaY);

// Índice do monstro vivo em (x, y) no andar, ou -1 se não houver.
int monstroEm(const Andar& andar, int x, int y);

const TipoMonstro& tipoDe(const Monstro& m);

// Nome com artigo, para as mensagens: "o rato", "a naga", "o ogro campeão".
// Se o jogador não está vendo o monstro, vira "algo".
std::string nomeDoMonstro(const Jogo& jogo, const Monstro& m);
// Só o nome: "ogro", "ogro campeão", "sombra campeã".
std::string nomeDaEspecie(const Monstro& m);

// O jogador está vendo este monstro agora? (A sombra só aparece a até 2
// casas ou na luz de uma tocha, mesmo numa casa visível.)
bool monstroAVista(const Jogo& jogo, const Monstro& m);

// Campeão (elite): +2 níveis, +50% de vida, aura dourada; vale o dobro de
// experiência, o triplo de moedas e pode soltar um item bom.
void tornarCampeao(Monstro& m);

// Vida máxima do monstro (depende da espécie e do nível).
int vidaMaximaDoMonstro(const Monstro& m);

// O tempo passou "ms" milissegundos no mapa: cada monstro age quando completa
// o seu intervalo (ver intervaloDoMonstro). Se um monstro alcançar o jogador,
// ele marca jogo.batalhaPendente e os outros esperam: a batalha começa.
void agirMonstros(Jogo& jogo, int ms);

// De quantos em quantos milissegundos o monstro age. Depende da velocidade
// (que cresce com o nível); os RAPIDOS agem no dobro do ritmo.
int intervaloDoMonstro(const Monstro& m);
