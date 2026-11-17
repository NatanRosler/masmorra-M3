// cenario.cpp
#include "cenario.h"
#include "jogo.h"
#include "aleatorio.h"

// -----------------------------------------------------------------------------
// COLOCAR CAIXAS E ARMADILHAS
// -----------------------------------------------------------------------------
// A célula está livre de escadas, itens, monstros e NPCs?
static bool celulaVazia(const Andar& andar, int x, int y) {
    if (andar.mapa.celulas[y][x] != CHAO) return false;
    if (itemEm(andar, x, y) >= 0 || monstroEm(andar, x, y) >= 0) return false;
    if (armadilhaEm(andar, x, y) >= 0) return false;
    return true;
}

// Uma caixa só vai para o meio de uma sala: as 8 vizinhas são chão. Assim
// ela nunca fecha um corredor nem a passagem para a escada.
static bool cercadaDeChao(const Mapa& mapa, int x, int y) {
    for (int d = 0; d < 8; d++) {
        int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
        if (!dentroDoMapa(nx, ny) || mapa.celulas[ny][nx] != CHAO) return false;
    }
    return true;
}

void colocarCenario(Andar& andar, int numeroDoAndar, uint32_t semente, int chegadaX, int chegadaY) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    andar.numArmadilhas = 0;

    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, chegadaX, chegadaY, distancia);

    // Caixas: 3 a 6 por andar.
    int caixas = 3 + sortear(g, 0, 3);
    for (int tentativa = 0; tentativa < 400 && caixas > 0; tentativa++) {
        int x = sortear(g, 1, MAPA_LARGURA - 2);
        int y = sortear(g, 1, MAPA_ALTURA - 2);
        if (!celulaVazia(andar, x, y) || !cercadaDeChao(andar.mapa, x, y)) continue;
        if (distancia[y][x] == INALCANCAVEL || distancia[y][x] < 3) continue;
        andar.mapa.celulas[y][x] = CAIXA;
        caixas--;
    }

    // Armadilhas: mais numerosas nos andares fundos, longe da chegada.
    int armadilhas = 2 + numeroDoAndar / 3;
    for (int tentativa = 0; tentativa < 400 && armadilhas > 0 && andar.numArmadilhas < MAX_ARMADILHAS; tentativa++) {
        int x = sortear(g, 1, MAPA_LARGURA - 2);
        int y = sortear(g, 1, MAPA_ALTURA - 2);
        if (!celulaVazia(andar, x, y)) continue;
        if (distancia[y][x] == INALCANCAVEL || distancia[y][x] < 8) continue;
        Armadilha& a = andar.armadilhas[andar.numArmadilhas++];
        a.x = x;
        a.y = y;
        a.ativa = true;
        a.descoberta = false;
        armadilhas--;
    }
}

int armadilhaEm(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numArmadilhas; i++) {
        const Armadilha& a = andar.armadilhas[i];
        if (a.ativa && a.x == x && a.y == y) return i;
    }
    return -1;
}

// -----------------------------------------------------------------------------
// ARMADILHAS EM AÇÃO
// -----------------------------------------------------------------------------
// Dano da armadilha: uma fração da vida máxima de quem pisou.
static int danoDaArmadilha(int vidaMaxima, int porcento) {
    int dano = vidaMaxima * porcento / 100;
    return dano < 1 ? 1 : dano;
}

void armadilhaPegaJogador(Jogo& jogo) {
    Andar& andar = andarAtual(jogo);
    Jogador& j = jogo.jogador;

    int i = armadilhaEm(andar, j.x, j.y);
    if (i >= 0) {
        Armadilha& a = andar.armadilhas[i];
        a.descoberta = true;
        int dano = danoDaArmadilha(j.vidaMaxima, 15);
        j.vida -= dano;
        adicionarMensagem(jogo, "Espinhos saltam do chão! Você perde " + std::to_string(dano) + " de vida.");
        if (j.vida <= 0) {
            j.vida = 0;
            desmaiar(jogo, "ferido por uma armadilha de espinhos");
            return;
        }
    }

    // Procura armadilhas escondidas em volta. Destreza ajuda a perceber.
    int chanceDePerceber = 20 + 5 * j.atributos[DESTREZA];
    for (int k = 0; k < andar.numArmadilhas; k++) {
        Armadilha& a = andar.armadilhas[k];
        if (!a.ativa || a.descoberta) continue;
        int dx = a.x - j.x, dy = a.y - j.y;
        if (dx < -1 || dx > 1 || dy < -1 || dy > 1) continue;
        if (chance(jogo.sorte, chanceDePerceber)) {
            a.descoberta = true;
            adicionarMensagem(jogo, "Você percebe uma armadilha escondida no chão!");
        }
    }
}

void armadilhaPegaMonstro(Jogo& jogo, Monstro& m) {
    Andar& andar = andarAtual(jogo);
    int i = armadilhaEm(andar, m.x, m.y);
    if (i < 0) return;
    Armadilha& a = andar.armadilhas[i];
    bool visto = jogo.visivel[m.y][m.x];
    if (visto) a.descoberta = true;
    m.vida -= danoDaArmadilha(vidaMaximaDoMonstro(m), 20);
    std::string nome = nomeDoMonstro(jogo, m);
    if (!nome.empty()) nome[0] = (char)(nome[0] - 'a' + 'A');
    if (m.vida <= 0) {
        m.vida = 0;
        m.vivo = false;
        jogo.monstrosMortos++;
        if (visto) adicionarMensagem(jogo, nome + " pisa numa armadilha e é derrotado!");
        // Se carregava a chave, ela fica no chão.
        if (m.carregaChave && andar.numItens < MAX_ITENS_NO_CHAO) {
            ItemNoChao& item = andar.itens[andar.numItens++];
            item.ativo = true;
            item.tipo = I_CHAVE;
            item.x = m.x;
            item.y = m.y;
            item.visto = false;
        }
    } else if (visto) {
        adicionarMensagem(jogo, nome + " pisa numa armadilha de espinhos!");
    }
}

// -----------------------------------------------------------------------------
// CAIXAS
// -----------------------------------------------------------------------------
bool empurrarCaixa(Jogo& jogo, int dx, int dy) {
    Andar& andar = andarAtual(jogo);
    Jogador& j = jogo.jogador;
    int caixaX = j.x + dx, caixaY = j.y + dy;
    int destinoX = caixaX + dx, destinoY = caixaY + dy;

    bool podeEmpurrar = (dx == 0 || dy == 0) && dentroDoMapa(destinoX, destinoY) &&
                        andar.mapa.celulas[destinoY][destinoX] == CHAO &&
                        monstroEm(andar, destinoX, destinoY) < 0 &&
                        npcEm(andar, destinoX, destinoY) < 0 &&
                        itemEm(andar, destinoX, destinoY) < 0;
    andar.mapa.celulas[caixaY][caixaX] = CHAO;
    if (!podeEmpurrar) {
        // Presa (parede, monstro, item...) ou empurrada na diagonal: o
        // jogador quebra a caixa e o caminho fica livre.
        adicionarMensagem(jogo, "A caixa não se mexe, então você a quebra em pedaços.");
        return true;
    }

    int armadilha = armadilhaEm(andar, destinoX, destinoY);
    if (armadilha >= 0) {
        andar.armadilhas[armadilha].ativa = false;
        adicionarMensagem(jogo, "A caixa cai sobre uma armadilha e a desarma! Os espinhos a despedaçam.");
    } else {
        andar.mapa.celulas[destinoY][destinoX] = CAIXA;
    }
    j.x = caixaX;
    j.y = caixaY;
    pegarItensDoChao(jogo);
    return true;
}

void quebrarCaixasPerto(Jogo& jogo, Monstro& m) {
    Andar& andar = andarAtual(jogo);
    for (int d = 0; d < 8; d++) {
        int x = m.x + DIRECOES_DX[d], y = m.y + DIRECOES_DY[d];
        if (!dentroDoMapa(x, y) || andar.mapa.celulas[y][x] != CAIXA) continue;
        if (!chance(jogo.sorte, 50)) continue;
        andar.mapa.celulas[y][x] = CHAO;
        if (jogo.visivel[y][x]) {
            std::string nome = nomeDoMonstro(jogo, m);
            if (!nome.empty()) nome[0] = (char)(nome[0] - 'a' + 'A');
            adicionarMensagem(jogo, nome + " despedaça uma caixa!");
        }
        return;
    }
}
