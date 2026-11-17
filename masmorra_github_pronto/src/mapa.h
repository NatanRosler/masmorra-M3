// mapa.h
// -----------------------------------------------------------------------------
// O mapa de um andar é uma matriz de células: celulas[linha][coluna],
// ou seja, celulas[y][x]. O canto superior esquerdo é (x=0, y=0).
// -----------------------------------------------------------------------------
#pragma once

const int MAPA_LARGURA = 79;
const int MAPA_ALTURA = 21;

// Valor usado nos mapas de distância para "não dá para chegar".
const int INALCANCAVEL = -1;

enum TipoCelula {
    PAREDE,
    CHAO,
    PORTA,
    ESCADA_DESCE,
    ESCADA_SOBE,
    TOCHA,         // parede com uma tocha presa: não dá para passar, mas ilumina
    PORTA_TRANCADA // só abre com uma chave
};

struct Mapa {
    TipoCelula celulas[MAPA_ALTURA][MAPA_LARGURA];
};

// As 8 direções, na ordem: as 4 retas primeiro, depois as 4 diagonais.
const int DIRECOES_DX[8] = { 0, 1, 0, -1,   1,  1, -1, -1 };
const int DIRECOES_DY[8] = { -1, 0, 1, 0,  -1,  1,  1, -1 };

// Preenche o mapa inteiro com parede (rocha sólida).
void encherDeParede(Mapa& mapa);
void colocar(Mapa& mapa, int x, int y, TipoCelula tipo);

// Consultas
bool dentroDoMapa(int x, int y);
bool ehPassavel(const Mapa& mapa, int x, int y);         // dá para pisar?
bool ehEscada(const Mapa& mapa, int x, int y);
bool ehOpaco(const Mapa& mapa, int x, int y);             // bloqueia a visão e a luz?
bool ehVisivelComoParede(const Mapa& mapa, int x, int y); // parede encostada em algo passável

// Regra de movimento do Brogue: dá para ir de (x, y) para (x+dx, y+dy)?
// Nas diagonais, não pode "cortar a quina" de uma parede nem entrar ou
// sair de uma porta.
bool podeMover(const Mapa& mapa, int x, int y, int dx, int dy);

// BFS (busca em largura): calcula quantos passos são necessários para ir de
// (origemX, origemY) até cada célula do mapa. Células que não dá para
// alcançar ficam com INALCANCAVEL.
//
// Escadas não entram no caminho (elas levam para outro andar).
void calcularDistancias(const Mapa& mapa, int origemX, int origemY,
                        int distancia[MAPA_ALTURA][MAPA_LARGURA]);
