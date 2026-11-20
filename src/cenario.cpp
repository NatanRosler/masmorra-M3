// cenario.cpp
#include "cenario.h"
#include "jogo.h"
#include "aleatorio.h"
#include "som.h"
#include "tela.h"
#include "dificuldade.h"
#include "loja.h"

// -----------------------------------------------------------------------------
// COLOCAR CAIXAS E ARMADILHAS
// -----------------------------------------------------------------------------
// A célula está livre de escadas, itens, monstros e NPCs?
static bool celulaVazia(const Andar& andar, int x, int y) {
    if (andar.mapa.celulas[y][x] != CHAO || andar.zonaSegura[y][x]) return false;
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

// O que vem dentro de uma caixa (70% das caixas têm algo):
//   Poção de Vida 35% | Arma 25% | Antídoto 12% | Éter 10% | Poção Grande 10% | Cristal 8%
// A arma é uma das que aparecem neste andar (Espada Curta no começo, Espada
// Longa no fundo, armas elementais no meio).
static int sortearConteudo(GeradorAleatorio& g, int numeroDoAndar) {
    if (!chance(g, 70)) {
        // Das caixas sem item, metade guarda um punhado de moedas.
        if (chance(g, 50)) return MOEDAS_NA_CAIXA + moedasDaCaixa(numeroDoAndar, sortear(g, 0, 5));
        return -1;
    }
    int r = sortear(g, 1, 100);
    if (r <= 35) return I_POCAO_DE_VIDA;
    if (r <= 60) {
        int armas[NUM_TIPOS_DE_ITEM];
        int quantas = 0;
        for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
            const TipoItem& t = TIPOS_DE_ITEM[i];
            if (t.categoria == ARMA && i != I_ADAGA &&
                numeroDoAndar >= t.andarMinimo && numeroDoAndar <= t.andarMaximo) armas[quantas++] = i;
        }
        if (quantas > 0) return armas[sortear(g, 0, quantas - 1)];
        return I_POCAO_DE_VIDA;
    }
    if (r <= 68) return I_ANTIDOTO;
    if (r <= 76) return I_ETER;
    if (r <= 84) return I_POCAO_GRANDE;
    if (r <= 92 && numeroDoAndar >= 2) return chance(g, 50) ? I_BOMBA_DE_FUMACA : I_PERGAMINHO_DO_MAPA;
    return I_CRISTAL_DE_VIDA + sortear(g, 0, 3);   // um dos 4 cristais
}

void colocarCenario(Andar& andar, int numeroDoAndar, uint32_t semente, int chegadaX, int chegadaY) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    andar.numArmadilhas = 0;
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++) andar.conteudoDasCaixas[y][x] = -1;

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
        andar.conteudoDasCaixas[y][x] = sortearConteudo(g, numeroDoAndar);
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

    // Lava (profundezas vulcânicas): queima a cada passo em cima dela.
    if (andar.mapa.celulas[j.y][j.x] == LAVA) {
        tocarSom(SOM_LAVA);
        int dano = danoDaArmadilha(j.vidaMaxima, dificuldadeAtual().danoDasArmadilhas);
        efeitoNoMapa(jogo, EFEITO_LAVA, j.x, j.y, dano);
        j.vida -= dano;
        adicionarMensagem(jogo, "A lava queima seus pés! Você perde " + std::to_string(dano) + " de vida.", MSG_PERIGO);
        if (j.vida <= 0) {
            j.vida = 0;
            desmaiar(jogo, "queimado pela lava");
            return;
        }
    }

    int i = armadilhaEm(andar, j.x, j.y);
    if (i >= 0) {
        Armadilha& a = andar.armadilhas[i];
        a.descoberta = true;
        tocarSom(SOM_ESPINHOS);
        int dano = danoDaArmadilha(j.vidaMaxima, dificuldadeAtual().danoDasArmadilhas);
        efeitoNoMapa(jogo, EFEITO_ARMADILHA, j.x, j.y, dano);
        j.vida -= dano;
        adicionarMensagem(jogo, "Espinhos saltam do chão! Você perde " + std::to_string(dano) + " de vida.", MSG_PERIGO);
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
            adicionarMensagem(jogo, "Você percebe uma armadilha escondida no chão!", MSG_PERIGO);
        }
    }
}

void armadilhaPegaMonstro(Jogo& jogo, Monstro& m) {
    Andar& andar = andarAtual(jogo);
    int i = armadilhaEm(andar, m.x, m.y);
    if (i < 0) return;
    Armadilha& a = andar.armadilhas[i];
    bool visto = jogo.visivel[m.y][m.x];
    if (visto) {
        a.descoberta = true;
        tocarSom(SOM_ESPINHOS);
        efeitoNoMapa(jogo, EFEITO_ESPINHOS, m.x, m.y, 0);
    }
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
// A caixa que estava em (caixaX, caixaY) quebrou: o que havia dentro cai no
// chão em (x, y).
// "peloJogador" = foi o jogador quem quebrou (só ele recolhe as moedas).
static void soltarConteudo(Jogo& jogo, Andar& andar, int caixaX, int caixaY, int x, int y, bool peloJogador = true) {
    if (jogo.visivel[caixaY][caixaX]) efeitoNoMapa(jogo, EFEITO_CAIXA, caixaX, caixaY, 0);
    int item = andar.conteudoDasCaixas[caixaY][caixaX];
    andar.conteudoDasCaixas[caixaY][caixaX] = -1;
    if (item >= MOEDAS_NA_CAIXA && !peloJogador) {
        if (jogo.visivel[caixaY][caixaX]) adicionarMensagem(jogo, "Moedas rolam da caixa e somem numa fenda do chão!");
        return;
    }
    if (item >= MOEDAS_NA_CAIXA) {
        // Moedas: o jogador recolhe na hora.
        int moedas = item - MOEDAS_NA_CAIXA;
        jogo.jogador.moedas += moedas;
        tocarSom(SOM_MOEDAS);
        efeitoNoMapa(jogo, EFEITO_MOEDAS, caixaX, caixaY, moedas);
        adicionarMensagem(jogo, "A caixa se quebra e espalha " + std::to_string(moedas) + " moedas! Você recolhe todas.", MSG_BOA);
        return;
    }
    if (item < 0 || andar.numItens >= MAX_ITENS_NO_CHAO || itemEm(andar, x, y) >= 0) return;
    ItemNoChao& novo = andar.itens[andar.numItens++];
    novo.ativo = true;
    novo.tipo = item;
    novo.x = x;
    novo.y = y;
    novo.visto = jogo.visivel[y][x];
    if (jogo.visivel[y][x]) {
        adicionarMensagem(jogo, "A caixa se quebra e revela " + nomeComArtigo(item) + "!", MSG_BOA);
    }
}

bool empurrarCaixa(Jogo& jogo, int dx, int dy) {
    Andar& andar = andarAtual(jogo);
    Jogador& j = jogo.jogador;
    int caixaX = j.x + dx, caixaY = j.y + dy;
    int destinoX = caixaX + dx, destinoY = caixaY + dy;

    // Um monstro logo atrás da caixa: ela bate nele, se despedaça e o deixa
    // atordoado (parado por alguns segundos e tonto se a batalha começar).
    int alvo = dentroDoMapa(destinoX, destinoY) ? monstroEm(andar, destinoX, destinoY) : -1;
    if ((dx == 0 || dy == 0) && alvo >= 0) {
        Monstro& m = andar.monstros[alvo];
        andar.mapa.celulas[caixaY][caixaX] = CHAO;
        tocarSom(SOM_CAIXA);
        m.atordoadoMs = TEMPO_ATORDOADO_MS;
        efeitoNoMapa(jogo, EFEITO_ATORDOAR, m.x, m.y, 0);
        bool feminino = tipoDe(m).artigo[0] == 'a';
        adicionarMensagem(jogo, "A caixa acerta " + nomeDoMonstro(jogo, m) +
                                (feminino ? "! Ela fica atordoada." : "! Ele fica atordoado."), MSG_BOA);
        soltarConteudo(jogo, andar, caixaX, caixaY, caixaX, caixaY);
        return true;
    }

    bool podeEmpurrar = (dx == 0 || dy == 0) && dentroDoMapa(destinoX, destinoY) &&
                        andar.mapa.celulas[destinoY][destinoX] == CHAO &&
                        monstroEm(andar, destinoX, destinoY) < 0 &&
                        npcEm(andar, destinoX, destinoY) < 0 &&
                        especialEm(andar, destinoX, destinoY) < 0 &&
                        itemEm(andar, destinoX, destinoY) < 0;
    andar.mapa.celulas[caixaY][caixaX] = CHAO;
    tocarSom(SOM_CAIXA);
    if (!podeEmpurrar) {
        // Presa (parede, item...) ou empurrada na diagonal: o jogador quebra
        // a caixa e o caminho fica livre.
        adicionarMensagem(jogo, "A caixa não se mexe, então você a quebra em pedaços.");
        soltarConteudo(jogo, andar, caixaX, caixaY, caixaX, caixaY);
        return true;
    }

    int armadilha = armadilhaEm(andar, destinoX, destinoY);
    if (armadilha >= 0) {
        andar.armadilhas[armadilha].ativa = false;
        adicionarMensagem(jogo, "A caixa cai sobre uma armadilha e a desarma! Os espinhos a despedaçam.", MSG_BOA);
        soltarConteudo(jogo, andar, caixaX, caixaY, destinoX, destinoY);
    } else {
        // A caixa anda e leva junto o que tem dentro.
        andar.mapa.celulas[destinoY][destinoX] = CAIXA;
        andar.conteudoDasCaixas[destinoY][destinoX] = andar.conteudoDasCaixas[caixaY][caixaX];
        andar.conteudoDasCaixas[caixaY][caixaX] = -1;
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
            tocarSom(SOM_CAIXA);
            std::string nome = nomeDoMonstro(jogo, m);
            if (!nome.empty()) nome[0] = (char)(nome[0] - 'a' + 'A');
            adicionarMensagem(jogo, nome + " despedaça uma caixa!");
        }
        soltarConteudo(jogo, andar, x, y, x, y, false);
        return;
    }
}
