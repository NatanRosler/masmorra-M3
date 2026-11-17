// mapa.cpp
#include "mapa.h"

void encherDeParede(Mapa& mapa) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            mapa.celulas[y][x] = PAREDE;
        }
    }
}

bool dentroDoMapa(int x, int y) {
    return x >= 0 && x < MAPA_LARGURA && y >= 0 && y < MAPA_ALTURA;
}

void colocar(Mapa& mapa, int x, int y, TipoCelula tipo) {
    if (dentroDoMapa(x, y)) {
        mapa.celulas[y][x] = tipo;
    }
}

bool ehPassavel(const Mapa& mapa, int x, int y) {
    if (!dentroDoMapa(x, y)) return false;
    TipoCelula t = mapa.celulas[y][x];
    return t == CHAO || t == PORTA || t == ESCADA_DESCE || t == ESCADA_SOBE;
}

// Paredes, tochas e escadas (que ficam dentro da parede) bloqueiam a visão.
// Portas fechadas também — como no Brogue, só dá para ver o outro lado
// quando alguém está parado na porta (e quem está na porta é sempre a
// origem da visão, então ela não atrapalha).
bool ehOpaco(const Mapa& mapa, int x, int y) {
    if (!dentroDoMapa(x, y)) return true;
    return mapa.celulas[y][x] != CHAO;
}

bool ehEscada(const Mapa& mapa, int x, int y) {
    if (!dentroDoMapa(x, y)) return false;
    return mapa.celulas[y][x] == ESCADA_DESCE || mapa.celulas[y][x] == ESCADA_SOBE;
}

// Só desenhamos a parede que encosta (inclusive na diagonal) em alguma célula
// passável. A rocha "maciça" longe das salas fica em branco, como no Brogue.
bool ehVisivelComoParede(const Mapa& mapa, int x, int y) {
    if (mapa.celulas[y][x] != PAREDE) return false;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (ehPassavel(mapa, x + dx, y + dy)) return true;
        }
    }
    return false;
}

bool podeMover(const Mapa& mapa, int x, int y, int dx, int dy) {
    int novoX = x + dx;
    int novoY = y + dy;
    if (!ehPassavel(mapa, novoX, novoY)) return false;

    bool diagonal = (dx != 0 && dy != 0);
    if (diagonal) {
        if (!ehPassavel(mapa, x + dx, y) || !ehPassavel(mapa, x, y + dy)) return false;
        if (mapa.celulas[y][x] == PORTA) return false;
        if (mapa.celulas[novoY][novoX] == PORTA) return false;
    }
    return true;
}

void calcularDistancias(const Mapa& mapa, int origemX, int origemY,
                        int distancia[MAPA_ALTURA][MAPA_LARGURA]) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            distancia[y][x] = INALCANCAVEL;
        }
    }
    if (!ehPassavel(mapa, origemX, origemY)) return;

    // Fila simples num vetor: "inicio" aponta o próximo a sair,
    // "fim" aponta onde o próximo vai entrar. Cada célula entra no máximo
    // uma vez, então o tamanho do mapa é suficiente.
    static int filaX[MAPA_LARGURA * MAPA_ALTURA];
    static int filaY[MAPA_LARGURA * MAPA_ALTURA];
    int inicio = 0, fim = 0;

    distancia[origemY][origemX] = 0;
    filaX[fim] = origemX;
    filaY[fim] = origemY;
    fim++;

    while (inicio < fim) {
        int x = filaX[inicio];
        int y = filaY[inicio];
        inicio++;

        for (int d = 0; d < 8; d++) {
            int dx = DIRECOES_DX[d];
            int dy = DIRECOES_DY[d];
            int nx = x + dx;
            int ny = y + dy;

            if (!podeMover(mapa, x, y, dx, dy)) continue;
            if (ehEscada(mapa, nx, ny)) continue;
            if (distancia[ny][nx] != INALCANCAVEL) continue; // já visitada

            distancia[ny][nx] = distancia[y][x] + 1;
            filaX[fim] = nx;
            filaY[fim] = ny;
            fim++;
        }
    }
}
