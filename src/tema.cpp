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
    { "A CRIPTA ESQUECIDA",     "Os mortos de Valdoria não descansam. Ande com cuidado.",
      -1,   { 0.62f, 1.00f, 0.80f } },
    { "PROFUNDEZAS VULCÂNICAS", "O chão racha e a lava brilha. Não pise nela!",
      FOGO, { 1.00f, 0.50f, 0.25f } },
    { "SALA DO TRONO",          "O Senhor das Profundezas espera no fim do salão.",
      -1,   { 0.55f, 0.70f, 1.00f } },
};

int temaDoAndar(int numeroDoAndar) {
    if (numeroDoAndar >= NUM_ANDARES) return TEMA_TRONO;
    if (numeroDoAndar >= 10) return TEMA_VULCAO;
    if (numeroDoAndar >= 7) return TEMA_CRIPTA;
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

// Quantas células dá para alcançar a partir da chegada SEM pisar na lava
// (as lápides da cripta também contam como parede).
static int alcancaveisSemLava(Mapa& mapa, const Escadas& e) {
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    static TipoCelula guardadas[MAPA_ALTURA][MAPA_LARGURA];
    // A lava conta como parede só para esta conta.
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++) {
            guardadas[y][x] = mapa.celulas[y][x];
            if (mapa.celulas[y][x] == LAVA || mapa.celulas[y][x] == TUMULO) mapa.celulas[y][x] = PAREDE;
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

// Poça (de água ou de lava): cresce a partir de uma célula, pegando vizinhas
// ao acaso. Para a lava, "alcancaveis" não é nulo: se a poça fechar alguma
// passagem (obrigando a pisar na lava), ela é desfeita.
static void criarPoca(Andar& a, GeradorAleatorio& g, int tamanho, TipoCelula tipo, int* alcancaveis) {
    int x = 0, y = 0;
    bool achou = false;
    for (int t = 0; t < 200 && !achou; t++) {
        x = sortear(g, 1, MAPA_LARGURA - 2);
        y = sortear(g, 1, MAPA_ALTURA - 2);
        achou = podeDecorar(a, x, y);
    }
    if (!achou) return;
    int fx[64], fy[64], n = 0;
    a.mapa.celulas[y][x] = tipo;
    fx[n] = x; fy[n] = y; n++;
    for (int colocadas = 1, t = 0; colocadas < tamanho && t < tamanho * 12; t++) {
        // Testa 3 vizinhas e fica com a que tem mais poça em volta: ela
        // cresce "redonda" em vez de virar um canal comprido.
        int nx = -1, ny = -1, melhor = -1;
        for (int tentativa = 0; tentativa < 3; tentativa++) {
            int k = sortear(g, 0, n - 1);
            int d = sortear(g, 0, 3);
            int cx = fx[k] + DIRECOES_DX[d], cy = fy[k] + DIRECOES_DY[d];
            if (!podeDecorar(a, cx, cy)) continue;
            int iguais = 0;
            for (int v = 0; v < 8; v++) iguais += a.mapa.celulas[cy + DIRECOES_DY[v]][cx + DIRECOES_DX[v]] == tipo;
            if (iguais > melhor) { melhor = iguais; nx = cx; ny = cy; }
        }
        if (nx < 0) continue;
        a.mapa.celulas[ny][nx] = tipo;
        colocadas++;
        if (n < 64) { fx[n] = nx; fy[n] = ny; n++; }
    }
    if (alcancaveis == nullptr) return;
    // Lava: sem caminho fechado, só as próprias casas de lava deixam de contar.
    int agora = alcancaveisSemLava(a.mapa, a.escadas);
    if (agora < *alcancaveis - n) {
        for (int i = 0; i < n; i++) a.mapa.celulas[fy[i]][fx[i]] = CHAO;   // fechava um caminho: desfaz
    } else {
        *alcancaveis = agora;
    }
}

void decorarAndar(Andar& andar, int numeroDoAndar, uint32_t semente) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    int tema = temaDoAndar(numeroDoAndar);
    if (tema == TEMA_CAVERNA) {
        // Várias poças pequenas (e não um rio): de 1 a 6 casas cada.
        int pocas = 11 + sortear(g, 0, 4);
        for (int i = 0; i < pocas; i++) criarPoca(andar, g, sortear(g, 1, 6), POCA, nullptr);
    } else if (tema == TEMA_CRIPTA) {
        // Lápides soltas pelas salas (de preferência encostadas na parede),
        // em grupinhos de 1 a 3, sem nunca fechar um caminho.
        int alcancaveis = alcancaveisSemLava(andar.mapa, andar.escadas);
        int grupos = 10 + sortear(g, 0, 4);
        for (int i = 0; i < grupos; i++) {
            int x = 0, y = 0; bool achou = false;
            for (int t = 0; t < 300 && !achou; t++) {
                x = sortear(g, 1, MAPA_LARGURA - 2); y = sortear(g, 1, MAPA_ALTURA - 2);
                if (!podeDecorar(andar, x, y)) continue;
                int paredes = 0;
                for (int v = 0; v < 4; v++) paredes += andar.mapa.celulas[y + DIRECOES_DY[v]][x + DIRECOES_DX[v]] == PAREDE;
                achou = paredes == 1 || (paredes == 0 && t > 200);
            }
            if (!achou) continue;
            int n = sortear(g, 1, 3), dx = sortear(g, 0, 1);
            for (int k = 0; k < n; k++) {
                int cx = x + (dx ? 2 * k : 0), cy = y + (dx ? 0 : 2 * k);
                if (!podeDecorar(andar, cx, cy)) break;
                andar.mapa.celulas[cy][cx] = TUMULO;
                int agora = alcancaveisSemLava(andar.mapa, andar.escadas);
                if (agora < alcancaveis - 1) { andar.mapa.celulas[cy][cx] = CHAO; break; }
                alcancaveis = agora;
            }
        }
        // Velas: mais pontos de luz (fracos e esverdeados) nas paredes retas.
        int vx[16], vy[16], velas = 0;
        for (int t = 0; t < 4000 && velas < 9; t++) {
            int x = sortear(g, 1, MAPA_LARGURA - 2), y = sortear(g, 1, MAPA_ALTURA - 3);
            if (andar.mapa.celulas[y][x] != PAREDE || andar.mapa.celulas[y + 1][x] != CHAO) continue;
            if (andar.mapa.celulas[y][x - 1] != PAREDE || andar.mapa.celulas[y][x + 1] != PAREDE) continue;
            if (andar.zonaSegura[y + 1][x]) continue;
            bool longe = true;
            for (int k = 0; k < velas; k++) if ((vx[k]-x)*(vx[k]-x) + (vy[k]-y)*(vy[k]-y) < 64) longe = false;
            for (int dy = -6; dy <= 6 && longe; dy++) for (int dx = -6; dx <= 6; dx++)
                if (x+dx > 0 && y+dy > 0 && x+dx < MAPA_LARGURA && y+dy < MAPA_ALTURA && andar.mapa.celulas[y+dy][x+dx] == TOCHA) longe = false;
            if (!longe) continue;
            andar.mapa.celulas[y][x] = TOCHA; vx[velas] = x; vy[velas] = y; velas++;
        }
    } else if (tema == TEMA_VULCAO) {
        // Poças de lava: menores, e nunca fecham um caminho.
        int alcancaveis = alcancaveisSemLava(andar.mapa, andar.escadas);
        int pocas = 9 + sortear(g, 0, 3);
        for (int i = 0; i < pocas; i++) criarPoca(andar, g, sortear(g, 1, 5), LAVA, &alcancaveis);
    }
}
