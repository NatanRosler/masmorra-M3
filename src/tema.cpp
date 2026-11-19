// tema.cpp
#include "tema.h"
#include "jogo.h"
#include "aleatorio.h"
#include <cstdlib>

const FichaDoTema FICHAS_DOS_TEMAS[NUM_TEMAS] = {
    { "AS MINAS DE VALDORIA",   "Túneis de pedra e tochas velhas dos mineiros.",
      -1,   { 1.00f, 0.68f, 0.38f } },
    { "CAVERNA ALAGADA",        "A água pinga do teto. Nas poças, cada passo é lento.",
      AGUA, { 0.62f, 0.86f, 1.00f } },
    { "PROFUNDEZAS VULCÂNICAS", "O chão racha e a lava brilha. Não pise nela!",
      FOGO, { 1.00f, 0.50f, 0.25f } },
    { "SALA DO TRONO",          "O Senhor das Profundezas espera no fim do salão.",
      -1,   { 0.55f, 0.70f, 1.00f } },
};

int temaDoAndar(int numeroDoAndar) {
    if (numeroDoAndar >= NUM_ANDARES) return TEMA_TRONO;
    if (numeroDoAndar >= 7) return TEMA_VULCAO;
    if (numeroDoAndar >= 4) return TEMA_CAVERNA;
    return TEMA_MINAS;
}

int bonusDoAmbiente(int numeroDoAndar, int elemento) {
    int forte = FICHAS_DOS_TEMAS[temaDoAndar(numeroDoAndar)].elementoFortalecido;
    return (forte >= 0 && elemento == forte) ? 120 : 100;
}

int multiplicadorDoPasso(const Andar& andar, int x, int y) {
    return andar.mapa.celulas[y][x] == POCA ? 2 : 1;
}

// -----------------------------------------------------------------------------
// DECORAÇÃO DO ANDAR
// -----------------------------------------------------------------------------
// Uma célula de chão "comum": fora da lojinha e longe (2 casas) das escadas,
// das chegadas e das portas, para nunca atrapalhar a entrada de uma sala.
static bool podeDecorar(const Andar& a, int x, int y) {
    if (x < 1 || y < 1 || x >= MAPA_LARGURA - 1 || y >= MAPA_ALTURA - 1) return false;
    if (a.mapa.celulas[y][x] != CHAO || a.zonaSegura[y][x]) return false;
    const Escadas& e = a.escadas;
    int pontos[4][2] = { { e.chegadaSobeX, e.chegadaSobeY }, { e.chegadaDesceX, e.chegadaDesceY },
                         { e.sobeX, e.sobeY }, { e.desceX, e.desceY } };
    for (auto& p : pontos) {
        if (std::abs(p[0] - x) <= 2 && std::abs(p[1] - y) <= 2) return false;
    }
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            TipoCelula c = a.mapa.celulas[y + dy][x + dx];
            if (c == PORTA || c == PORTA_TRANCADA) return false;
        }
    return true;
}

// Quantas células dá para alcançar a partir da chegada SEM pisar na lava.
static int alcancaveisSemLava(Mapa& mapa, const Escadas& e) {
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    static TipoCelula guardadas[MAPA_ALTURA][MAPA_LARGURA];
    // A lava conta como parede só para esta conta.
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++) {
            guardadas[y][x] = mapa.celulas[y][x];
            if (mapa.celulas[y][x] == LAVA) mapa.celulas[y][x] = PAREDE;
        }
    calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, distancia);
    int total = 0;
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++) {
            mapa.celulas[y][x] = guardadas[y][x];
            if (distancia[y][x] != INALCANCAVEL) total++;
        }
    return total;
}

// Poça: cresce a partir de uma célula, pegando vizinhas ao acaso.
static void criarPoca(Andar& a, GeradorAleatorio& g, int tamanho) {
    int x = 0, y = 0;
    bool achou = false;
    for (int t = 0; t < 200 && !achou; t++) {
        x = sortear(g, 1, MAPA_LARGURA - 2);
        y = sortear(g, 1, MAPA_ALTURA - 2);
        achou = podeDecorar(a, x, y);
    }
    if (!achou) return;
    int fx[64], fy[64], n = 0;
    a.mapa.celulas[y][x] = POCA;
    fx[n] = x; fy[n] = y; n++;
    for (int colocadas = 1, t = 0; colocadas < tamanho && t < tamanho * 12; t++) {
        // Testa 3 vizinhas e fica com a que tem mais água em volta: a poça
        // cresce "redonda" em vez de virar um canal comprido.
        int nx = -1, ny = -1, melhor = -1;
        for (int tentativa = 0; tentativa < 3; tentativa++) {
            int k = sortear(g, 0, n - 1);
            int d = sortear(g, 0, 3);
            int cx = fx[k] + DIRECOES_DX[d], cy = fy[k] + DIRECOES_DY[d];
            if (!podeDecorar(a, cx, cy)) continue;
            int agua = 0;
            for (int v = 0; v < 8; v++) agua += a.mapa.celulas[cy + DIRECOES_DY[v]][cx + DIRECOES_DX[v]] == POCA;
            if (agua > melhor) { melhor = agua; nx = cx; ny = cy; }
        }
        if (nx < 0) continue;
        a.mapa.celulas[ny][nx] = POCA;
        colocadas++;
        if (n < 64) { fx[n] = nx; fy[n] = ny; n++; }
    }
}

// Rachadura: um caminho fino e torto. Se ela fechar alguma passagem, é desfeita.
// "alcancaveis" = quantas casas dá para alcançar sem pisar na lava (atualizado).
static void criarRachadura(Andar& a, GeradorAleatorio& g, int comprimento, int& alcancaveis) {
    // Começa no meio de uma sala (as 8 vizinhas são chão), não num corredor.
    int x = 0, y = 0;
    bool achou = false;
    for (int t = 0; t < 300 && !achou; t++) {
        x = sortear(g, 1, MAPA_LARGURA - 2);
        y = sortear(g, 1, MAPA_ALTURA - 2);
        achou = podeDecorar(a, x, y);
        for (int d = 0; d < 8 && achou; d++)
            if (a.mapa.celulas[y + DIRECOES_DY[d]][x + DIRECOES_DX[d]] != CHAO) achou = false;
    }
    if (!achou) return;
    int cx[16], cy[16], n = 0;
    int direcao = sortear(g, 0, 7);
    for (int passo = 0; passo < comprimento && n < 16; passo++) {
        if (!podeDecorar(a, x, y)) break;
        a.mapa.celulas[y][x] = LAVA;
        cx[n] = x; cy[n] = y; n++;
        if (chance(g, 35)) direcao = (direcao + (chance(g, 50) ? 1 : 7)) % 8;   // entorta um pouco
        x += DIRECOES_DX[direcao];
        y += DIRECOES_DY[direcao];
    }
    // Sem caminho fechado, só as próprias casas de lava deixam de contar.
    int agora = alcancaveisSemLava(a.mapa, a.escadas);
    if (agora < alcancaveis - n) {
        for (int i = 0; i < n; i++) a.mapa.celulas[cy[i]][cx[i]] = CHAO;   // fechava um caminho: desfaz
    } else {
        alcancaveis = agora;
    }
}

void decorarAndar(Andar& andar, int numeroDoAndar, uint32_t semente) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    int tema = temaDoAndar(numeroDoAndar);
    if (tema == TEMA_CAVERNA) {
        // Várias poças pequenas (e não um rio): de 1 a 6 casas cada.
        int pocas = 11 + sortear(g, 0, 4);
        for (int i = 0; i < pocas; i++) criarPoca(andar, g, sortear(g, 1, 6));
    } else if (tema == TEMA_VULCAO) {
        int alcancaveis = alcancaveisSemLava(andar.mapa, andar.escadas);
        int rachaduras = 7 + sortear(g, 0, 3);
        for (int i = 0; i < rachaduras; i++) criarRachadura(andar, g, sortear(g, 4, 10), alcancaveis);
    }
}
