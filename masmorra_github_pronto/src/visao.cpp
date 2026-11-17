// visao.cpp
#include "visao.h"

#include <cmath>

// =============================================================================
// SHADOWCASTING RECURSIVO (Björn Bergström, 2001 — descrito no RogueBasin)
// =============================================================================
// A área em volta do jogador é dividida em 8 "fatias" (octantes), como uma
// pizza. Cada octante é percorrido linha por linha, afastando-se do jogador.
//
// Em cada linha, só interessa a faixa entre duas inclinações (inclInicio e
// inclFim) — é o "cone" que ainda está iluminado. Quando o cone bate numa
// parede, a parte de trás dela fica na sombra: a função chama a SI MESMA
// (recursão) para continuar só com a parte do cone que passou ao lado da
// parede.
//
// Os 8 octantes usam o mesmo código: a tabela abaixo "gira/espelha" as
// coordenadas (dx, dy) do octante padrão para cada um dos 8.
// -----------------------------------------------------------------------------
static const int OCTANTES[8][4] = {
    //  xx  xy  yx  yy
    {   1,  0,  0,  1 },
    {   0,  1,  1,  0 },
    {   0, -1,  1,  0 },
    {  -1,  0,  0,  1 },
    {  -1,  0,  0, -1 },
    {   0, -1, -1,  0 },
    {   0,  1, -1,  0 },
    {   1,  0,  0, -1 },
};

static void lancarSombras(const Mapa& mapa, int origemX, int origemY, int raio,
                          int linhaInicial, double inclInicio, double inclFim,
                          int xx, int xy, int yx, int yy,
                          bool visivel[MAPA_ALTURA][MAPA_LARGURA]) {
    if (inclInicio < inclFim) return; // cone fechado: nada a ver

    double proximoInicio = inclInicio;

    for (int distancia = linhaInicial; distancia <= raio; distancia++) {
        bool bloqueado = false;
        int dy = -distancia;

        for (int dx = -distancia; dx <= 0; dx++) {
            // Inclinações das bordas esquerda e direita desta célula.
            double inclEsquerda = (dx - 0.5) / (dy + 0.5);
            double inclDireita  = (dx + 0.5) / (dy - 0.5);

            if (inclInicio < inclDireita) continue; // ainda antes do cone
            if (inclFim > inclEsquerda) break;      // já passou do cone

            // Converte para coordenadas do mapa conforme o octante.
            int x = origemX + dx * xx + dy * xy;
            int y = origemY + dx * yx + dy * yy;
            bool dentro = dentroDoMapa(x, y);

            if (dentro && dx * dx + dy * dy <= raio * raio) {
                visivel[y][x] = true;
            }

            bool opaco = !dentro || ehOpaco(mapa, x, y);

            if (bloqueado) {
                // Estamos andando "dentro" de uma parede.
                if (opaco) {
                    proximoInicio = inclDireita;
                } else {
                    // Saímos da parede: o cone recomeça daqui.
                    bloqueado = false;
                    inclInicio = proximoInicio;
                }
            } else if (opaco && distancia < raio) {
                // Começou uma parede: o pedaço do cone ANTES dela continua
                // nas próximas linhas (chamada recursiva)...
                bloqueado = true;
                lancarSombras(mapa, origemX, origemY, raio, distancia + 1,
                              inclInicio, inclEsquerda, xx, xy, yx, yy, visivel);
                proximoInicio = inclDireita;
            }
        }
        // ...e se a linha terminou dentro de uma parede, o resto está na sombra.
        if (bloqueado) break;
    }
}

void calcularCampoDeVisao(const Mapa& mapa, int origemX, int origemY, int raio,
                          bool visivel[MAPA_ALTURA][MAPA_LARGURA]) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            visivel[y][x] = false;
        }
    }
    if (!dentroDoMapa(origemX, origemY)) return;

    visivel[origemY][origemX] = true; // a própria posição sempre é vista
    for (int o = 0; o < 8; o++) {
        lancarSombras(mapa, origemX, origemY, raio, 1, 1.0, 0.0,
                      OCTANTES[o][0], OCTANTES[o][1], OCTANTES[o][2], OCTANTES[o][3],
                      visivel);
    }
}

// =============================================================================
// ILUMINAÇÃO
// =============================================================================
void apagarLuzes(Luz luz[MAPA_ALTURA][MAPA_LARGURA]) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            luz[y][x] = { 0.0f, 0.0f, 0.0f };
        }
    }
}

// A luz usa o MESMO cálculo da visão: ela chega onde a fonte "enxergaria".
// A força cai com o QUADRADO da distância relativa: fica forte perto da
// fonte e só enfraquece de verdade perto da borda do alcance.
void adicionarLuz(const Mapa& mapa, int x, int y, int raio, Luz cor,
                  Luz luz[MAPA_ALTURA][MAPA_LARGURA]) {
    static bool alcance[MAPA_ALTURA][MAPA_LARGURA];
    calcularCampoDeVisao(mapa, x, y, raio, alcance);

    for (int cy = 0; cy < MAPA_ALTURA; cy++) {
        for (int cx = 0; cx < MAPA_LARGURA; cx++) {
            if (!alcance[cy][cx]) continue;
            double dx = cx - x, dy = cy - y;
            double distancia = std::sqrt(dx * dx + dy * dy);
            double relativa = distancia / (raio + 1.0);   // 0 na fonte, ~1 na borda
            float forca = (float)(1.0 - relativa * relativa);
            if (forca <= 0.0f) continue;
            luz[cy][cx].r += cor.r * forca;
            luz[cy][cx].g += cor.g * forca;
            luz[cy][cx].b += cor.b * forca;
        }
    }
}

float brilho(Luz luz) {
    float maior = luz.r;
    if (luz.g > maior) maior = luz.g;
    if (luz.b > maior) maior = luz.b;
    return maior;
}
