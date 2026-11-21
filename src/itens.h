// itens.h
// -----------------------------------------------------------------------------
// Itens: a ficha de cada tipo de item, os itens espalhados no chão, a mochila
// do jogador e o que acontece ao pegar, usar ou equipar.
//
//   !  poções       (usos limitados: cada uma some ao ser usada)
//   )  armas        (equipar: aumenta o ataque; algumas reforçam um elemento)
//   ]  escudos      (equipar: aumenta a defesa)
//   *  cristais     (power-ups: bônus permanente assim que são pegos)
//   ?  pergaminhos  (ensinam um golpe)
//   -  chave        (abre a porta trancada da sala do tesouro)
//   (  suporte      (v1.17: mochila de couro e lanterna valem a partida toda;
//                    bomba de fumaça e pergaminho do mapa são usados uma vez)
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>
#include "visao.h"
#include "batalha.h"

struct Jogo;
struct Andar;
struct Jogador;

enum CategoriaItem {
    POCAO,
    ARMA,
    ESCUDO,
    CRISTAL,
    PERGAMINHO,
    CHAVE,
    SUPORTE
};

// Índices na tabela TIPOS_DE_ITEM.
enum IdItem {
    I_POCAO_DE_VIDA, I_POCAO_GRANDE, I_ANTIDOTO, I_ETER,
    I_ADAGA, I_ESPADA_CURTA, I_MACHADO, I_ESPADA_LONGA,
    I_ESPADA_DE_FOGO, I_TRIDENTE, I_CHICOTE_DE_ESPINHOS, I_ADAGA_VENENOSA, I_MARTELO_DE_PEDRA,
    I_ESCUDO_DE_MADEIRA, I_ESCUDO_DE_FERRO, I_ESCUDO_DE_TORRE,
    I_CRISTAL_DE_VIDA, I_CRISTAL_DE_FORCA, I_CRISTAL_DE_FERRO, I_PENA_DO_VENTO,
    I_PERGAMINHO_DE_FOGO, I_PERGAMINHO_DE_AGUA, I_PERGAMINHO_DE_PLANTA,
    I_PERGAMINHO_DE_VENENO, I_PERGAMINHO_DE_PEDRA,
    I_CHAVE,
    // v1.17: no fim da lista, para os números dos itens antigos não mudarem
    // (o save guarda os itens pelo número).
    I_MOCHILA_DE_COURO, I_LANTERNA, I_BOMBA_DE_FUMACA, I_PERGAMINHO_DO_MAPA,
    // v1.23: armas e pergaminhos dos elementos novos (também no fim)
    I_LANCA_DE_GELO, I_MARTELO_DO_TROVAO, I_FOICE_DAS_TREVAS, I_CETRO_DE_LUZ,
    I_PERGAMINHO_DE_NORMAL, I_PERGAMINHO_DE_GELO, I_PERGAMINHO_DE_RAIO, I_PERGAMINHO_DE_TREVAS, I_PERGAMINHO_DE_LUZ,
    NUM_TIPOS_DE_ITEM
};

struct TipoItem {
    const char* nome;
    const char* artigo;      // "uma" / "um" (para as mensagens)
    char simbolo;
    CorRGB cor;
    CategoriaItem categoria;

    // Efeitos (cada item usa só os campos da sua categoria; os outros ficam 0).
    int curaPorcento;        // POCAO: quanto da vida máxima cura (100 = tudo)
    bool curaStatus;         // POCAO: tira veneno, sono e paralisia
    bool restauraPP;         // POCAO: recupera os PP de todos os golpes
    int bonusAtaque;         // ARMA ou CRISTAL
    int bonusDefesa;         // ESCUDO ou CRISTAL
    int bonusVelocidade;     // CRISTAL (negativo num escudo pesado)
    int bonusVida;           // CRISTAL
    int elemento;            // ARMA: golpes deste elemento causam +25% (-1 = nenhum)
    int golpeEnsinado;       // PERGAMINHO: índice em GOLPES (-1 = nenhum)

    // Onde aparece no chão.
    int andarMinimo, andarMaximo;
    int frequencia;          // 0 = nunca aparece solto no chão
    const char* descricao;
};

extern const TipoItem TIPOS_DE_ITEM[NUM_TIPOS_DE_ITEM];

// v1.23: o pergaminho de cada elemento (na ordem do enum Elemento).
extern const int PERGAMINHO_DO_ELEMENTO[NUM_ELEMENTOS];

// Um item caído num andar.
struct ItemNoChao {
    bool ativo;              // false = já foi pego
    int tipo;                // índice em TIPOS_DE_ITEM
    int x, y;
    bool visto;              // o jogador já viu (aparece na memória do mapa)
};

const int MAX_ITENS_NO_CHAO = 30;

// Espalha os itens de um andar recém-gerado: itens comuns pelo chão, o tesouro
// dentro da sala trancada e a chave (no chão ou com um dos monstros).
void espalharItens(Andar& andar, int numeroDoAndar, uint32_t semente);

// Índice do item no chão em (x, y), ou -1.
int itemEm(const Andar& andar, int x, int y);

// O jogador pega o que houver na célula onde está.
void pegarItensDoChao(Jogo& jogo);

// Chance (em %) de bloquear um ataque por inteiro. Erguendo o escudo (golpe
// Escudo): 30% sem escudo, 45% madeira, 60% ferro, 75% torre. Sem erguer, só
// o escudo equipado: 5% madeira, 8% ferro, 12% torre.
int chanceDeBloqueio(const TipoItem* escudo, bool erguido);

// Coloca a arma ou o escudo na mão (o anterior volta para a mochila).
void equipar(Jogo& jogo, int tipo);

// Limites da mochila: cada item tem um máximo (6 Poções de Vida, 3 das outras
// poções, 2 bombas e 2 pergaminhos do mapa, 1 de cada arma/escudo/pergaminho
// de reserva) e a mochila toda leva no máximo 12 itens. Com a Mochila de
// Couro: 16 itens e +1 em cada limite. A chave, os cristais e os itens
// permanentes não ocupam espaço.
const int LIMITE_DA_MOCHILA = 12;
int limiteDaMochila(const Jogador& j);
int limiteDoItem(const Jogador& j, int tipo);
int itensNaMochila(const Jogador& j);
bool cabeNaMochila(const Jogador& j, int tipo);
std::string motivoDeNaoCaber(const Jogador& j, int tipo);   // "A mochila está cheia (12/12)."
// Guarda o item; se não couber, ele fica no chão perto do jogador.
void guardarOuLargar(Jogo& jogo, int tipo);

// Pegou a chave: marca a porta do tesouro no mapa e diz para que lado ela fica.
void revelarPortaDoTesouro(Jogo& jogo);

// Cristal: aplica o bônus permanente (pego no chão ou comprado na loja).
void absorverCristal(Jogo& jogo, int tipo);

// Itens que valem na hora e não vão para a mochila: cristais, Mochila de
// Couro e Lanterna.
bool usaNaHora(int tipo);
// Mochila de Couro ou Lanterna: aplica a melhoria. Se o jogador já tinha
// uma, vira moedas (o preço de venda).
void ganharItemPermanente(Jogo& jogo, int tipo);

// Pergaminho do mapa: revela o andar inteiro (salas, escadas e surpresas).
void revelarOAndar(Jogo& jogo);

// Usa um item da mochila fora da batalha. Devolve true se gastou o turno.
bool usarItemForaDaBatalha(Jogo& jogo, int tipo);

// Frase curta sobre o item: "uma Poção de Vida".
std::string nomeComArtigo(int tipo);
