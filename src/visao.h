// visao.h
// -----------------------------------------------------------------------------
// Campo de visão (o que o jogador enxerga) e iluminação.
//
// VISÃO: uma célula é vista se existe uma linha reta, sem paredes, entre ela
//        e o jogador. Calculado com "shadowcasting recursivo".
//
// LUZ:   cada fonte (o próprio jogador, tochas) espalha uma cor RGB que fica
//        mais fraca com a distância e não atravessa paredes. As luzes de
//        várias fontes se SOMAM. Só aparece na tela o que está na linha de
//        visão E iluminado o bastante.
// -----------------------------------------------------------------------------
#pragma once

#include "mapa.h"

// Cor de luz: cada componente vai de 0 (nada) a 1 (forte). Pode passar de 1
// quando duas luzes se somam.
struct Luz {
    float r, g, b;
};

// Cor de um objeto, em RGB (0 a 255 em cada componente).
struct CorRGB {
    int r, g, b;
};

// Abaixo disso, a célula é escura demais para ser vista.
const float LUZ_MINIMA_PARA_VER = 0.08f;

// Marca em "visivel" todas as células que podem ser vistas a partir de
// (origemX, origemY), até a distância "raio".
void calcularCampoDeVisao(const Mapa& mapa, int origemX, int origemY, int raio,
                          bool visivel[MAPA_ALTURA][MAPA_LARGURA]);

void apagarLuzes(Luz luz[MAPA_ALTURA][MAPA_LARGURA]);

// Soma no mapa de luz uma fonte de luz em (x, y), com a cor e o alcance dados.
void adicionarLuz(const Mapa& mapa, int x, int y, int raio, Luz cor,
                  Luz luz[MAPA_ALTURA][MAPA_LARGURA]);

// Intensidade de uma luz (o maior dos três componentes).
float brilho(Luz luz);
