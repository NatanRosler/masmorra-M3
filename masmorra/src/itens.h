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
    CHAVE
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

// Coloca a arma ou o escudo na mão (o anterior volta para a mochila).
void equipar(Jogo& jogo, int tipo);

// Usa um item da mochila fora da batalha. Devolve true se gastou o turno.
bool usarItemForaDaBatalha(Jogo& jogo, int tipo);

// Frase curta sobre o item: "uma Poção de Vida".
std::string nomeComArtigo(int tipo);
