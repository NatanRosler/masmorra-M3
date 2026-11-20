// salas.cpp
// -----------------------------------------------------------------------------
// Salas especiais (ver salas.h).
// -----------------------------------------------------------------------------
#include "salas.h"
#include "jogo.h"
#include "som.h"
#include "tela.h"
#include "loja.h"

#include <cstdlib>
#include <string>
#include <vector>

// =============================================================================
// ONDE COLOCAR
// =============================================================================
// Casa boa para um objeto: chão livre, no meio de uma sala (as 8 vizinhas são
// chão), fora da lojinha, sem nada em cima e longe da chegada.
// Cada surpresa tem um pedaço de chão decorado em volta (3x3 casas): para os
// enfeites não se misturarem, ela fica longe de outra surpresa e dos NPCs.
static bool longeDasOutras(const Andar& a, int x, int y) {
    for (int i = 0; i < a.numEspeciais; i++) {
        if (std::abs(a.especiais[i].x - x) < 4 && std::abs(a.especiais[i].y - y) < 4) return false;
    }
    for (int i = 0; i < a.numNpcs; i++) {
        if (std::abs(a.npcs[i].x - x) < 4 && std::abs(a.npcs[i].y - y) < 4) return false;
    }
    return true;
}

static bool lugarLivre(const Andar& a, int x, int y, const int distancia[MAPA_ALTURA][MAPA_LARGURA]) {
    if (x < 2 || y < 2 || x >= MAPA_LARGURA - 2 || y >= MAPA_ALTURA - 2) return false;
    if (a.mapa.celulas[y][x] != CHAO || a.zonaSegura[y][x]) return false;
    if (distancia[y][x] == INALCANCAVEL || distancia[y][x] < 6) return false;
    for (int d = 0; d < 8; d++) {
        int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
        if (a.mapa.celulas[ny][nx] != CHAO || a.zonaSegura[ny][nx]) return false;
    }
    if (!longeDasOutras(a, x, y)) return false;
    if (monstroEm(a, x, y) >= 0 || itemEm(a, x, y) >= 0 || armadilhaEm(a, x, y) >= 0) return false;
    if (npcEm(a, x, y) >= 0 || especialEm(a, x, y) >= 0) return false;
    return true;
}

static bool sortearLugar(const Andar& a, GeradorAleatorio& g, const int distancia[MAPA_ALTURA][MAPA_LARGURA],
                         int& x, int& y) {
    for (int t = 0; t < 400; t++) {
        x = sortear(g, 2, MAPA_LARGURA - 3);
        y = sortear(g, 2, MAPA_ALTURA - 3);
        if (lugarLivre(a, x, y, distancia)) return true;
    }
    return false;
}

// A arena precisa de uma sala com uma porta só (a mesma ideia da sala do
// tesouro e da lojinha): testa cada porta como se fosse parede e vê se isso
// isola uma sala de 12 a 80 casas, sem escada, sem loja e sem outra porta.
static bool acharSalaDaArena(Andar& a, int& portaX, int& portaY, int& meioX, int& meioY,
                             GeradorAleatorio& g) {
    Mapa& mapa = a.mapa;
    const Escadas& e = a.escadas;
    static int antes[MAPA_ALTURA][MAPA_LARGURA], depois[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, antes);
    int candidatas[64][2], n = 0;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (mapa.celulas[y][x] != PORTA || antes[y][x] == INALCANCAVEL || a.zonaSegura[y][x]) continue;
            mapa.celulas[y][x] = PAREDE;
            calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, depois);
            mapa.celulas[y][x] = PORTA;
            if (e.temDescida && depois[e.chegadaDesceY][e.chegadaDesceX] == INALCANCAVEL) continue;
            int isoladas = 0;
            bool problema = false;
            for (int cy = 1; cy < MAPA_ALTURA - 1 && !problema; cy++) {
                for (int cx = 1; cx < MAPA_LARGURA - 1; cx++) {
                    if (antes[cy][cx] == INALCANCAVEL || depois[cy][cx] != INALCANCAVEL) continue;
                    if (cx == x && cy == y) continue;
                    isoladas++;
                    TipoCelula c = mapa.celulas[cy][cx];
                    if ((c != CHAO && c != CAIXA && c != POCA && c != LAVA) || a.zonaSegura[cy][cx] ||
                        npcEm(a, cx, cy) >= 0) { problema = true; break; }
                }
            }
            if (problema || isoladas < 12 || isoladas > 80 || n >= 64) continue;
            candidatas[n][0] = x;
            candidatas[n][1] = y;
            n++;
        }
    }
    // Sorteia uma das candidatas que tenha um meio livre para o gongo.
    for (int tentativa = 0; tentativa < 6 && n > 0; tentativa++) {
        int k = sortear(g, 0, n - 1);
        int x = candidatas[k][0], y = candidatas[k][1];
        mapa.celulas[y][x] = PAREDE;
        calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, depois);
        mapa.celulas[y][x] = PORTA;
        long somaX = 0, somaY = 0;
        int qtd = 0;
        for (int cy = 1; cy < MAPA_ALTURA - 1; cy++)
            for (int cx = 1; cx < MAPA_LARGURA - 1; cx++)
                if (antes[cy][cx] != INALCANCAVEL && depois[cy][cx] == INALCANCAVEL && !(cx == x && cy == y)) {
                    somaX += cx; somaY += cy; qtd++;
                }
        float mx = (float)somaX / qtd, my = (float)somaY / qtd;
        float melhor = 1e9f;
        int bx = -1, by = -1;
        for (int cy = 2; cy < MAPA_ALTURA - 2; cy++)
            for (int cx = 2; cx < MAPA_LARGURA - 2; cx++) {
                if (antes[cy][cx] == INALCANCAVEL || depois[cy][cx] != INALCANCAVEL) continue;
                if (mapa.celulas[cy][cx] != CHAO || monstroEm(a, cx, cy) >= 0 || itemEm(a, cx, cy) >= 0 ||
                    armadilhaEm(a, cx, cy) >= 0) continue;
                bool meio = true;
                for (int d = 0; d < 8; d++)
                    if (mapa.celulas[cy + DIRECOES_DY[d]][cx + DIRECOES_DX[d]] != CHAO) meio = false;
                if (!meio) continue;
                float dd = (cx - mx) * (cx - mx) + (cy - my) * (cy - my);
                if (dd < melhor) { melhor = dd; bx = cx; by = cy; }
            }
        if (bx < 0) continue;
        portaX = x; portaY = y; meioX = bx; meioY = by;
        return true;
    }
    return false;
}

static Especial& novoEspecial(Andar& a, int tipo, int x, int y) {
    Especial& e = a.especiais[a.numEspeciais++];
    e.tipo = tipo;
    e.x = x;
    e.y = y;
    e.usado = false;
    e.mimico = false;
    e.onda = 0;
    e.portaX = e.portaY = -1;
    e.numMonstros = 0;
    return e;
}

void colocarSalasEspeciais(Andar& andar, int numeroDoAndar, uint32_t semente) {
    andar.numEspeciais = 0;
    if (numeroDoAndar >= NUM_ANDARES) return;   // no andar do chefe, nada de surpresas
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, andar.escadas.chegadaSobeX, andar.escadas.chegadaSobeY, distancia);

    // Uma surpresa sempre; metade das vezes, duas. Nunca duas iguais.
    int quantas = chance(g, 50) ? 2 : 1;
    bool usado[5] = { false, false, false, false, false };   // fonte, altar, baú, arena, viajante
    const int PESOS[5] = { 25, 20, 25, 15, 15 };
    for (int s = 0; s < quantas; s++) {
        int total = 0;
        for (int k = 0; k < 5; k++) if (!usado[k]) total += PESOS[k];
        int r = sortear(g, 1, total), escolha = 0;
        for (int k = 0; k < 5; k++) {
            if (usado[k]) continue;
            r -= PESOS[k];
            if (r <= 0) { escolha = k; break; }
        }
        usado[escolha] = true;
        int x, y;
        if (escolha == 3) {
            // Arena: precisa de uma sala com uma porta só.
            int px, py;
            if (!acharSalaDaArena(andar, px, py, x, y, g) || !longeDasOutras(andar, x, y)) continue;
            Especial& e = novoEspecial(andar, ESP_GONGO, x, y);
            e.portaX = px;
            e.portaY = py;
        } else if (escolha == 4) {
            // Viajante perdido: é um NPC (a conversa fica em npcs.cpp).
            if (andar.numNpcs >= MAX_NPCS || !sortearLugar(andar, g, distancia, x, y)) continue;
            Npc& npc = andar.npcs[andar.numNpcs++];
            npc.tipo = NPC_VIAJANTE;
            npc.x = x;
            npc.y = y;
            npc.conversou = false;
            npc.estoque[0] = npc.estoque[1] = -1;
            npc.estoqueExtra = -1;
            npc.extrasAVenda[0] = npc.extrasAVenda[1] = 0;
            for (int k = 0; k < 4; k++) npc.pocoesAVenda[k] = 0;
        } else {
            if (!sortearLugar(andar, g, distancia, x, y)) continue;
            Especial& e = novoEspecial(andar, escolha == 0 ? ESP_FONTE : escolha == 1 ? ESP_ALTAR : ESP_BAU, x, y);
            if (e.tipo == ESP_BAU) e.mimico = chance(g, 40);   // 40% das vezes, o baú morde
        }
    }
}

int especialEm(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numEspeciais; i++) {
        if (andar.especiais[i].x == x && andar.especiais[i].y == y) return i;
    }
    return -1;
}

// =============================================================================
// USAR
// =============================================================================
static std::string emMoedas(int n) {
    return std::to_string(n) + (n == 1 ? " moeda" : " moedas");
}

// Um cristal sorteado (vida, força, ferro ou pena), absorvido na hora.
static void ganharCristal(Jogo& jogo) {
    int cristais[4] = { I_CRISTAL_DE_VIDA, I_CRISTAL_DE_FORCA, I_CRISTAL_DE_FERRO, I_PENA_DO_VENTO };
    absorverCristal(jogo, cristais[sortear(jogo.sorte, 0, 3)]);
}

// Um item bom para o baú de verdade: poção grande, éter, um pergaminho ou uma
// arma/escudo que costuma aparecer neste andar.
static int itemDoBau(Jogo& jogo) {
    int r = sortear(jogo.sorte, 1, 100);
    // Itens de suporte permanentes: só um de cada por partida.
    if (r <= 12 && !jogo.jogador.mochilaDeCouro) return I_MOCHILA_DE_COURO;
    if (r <= 12 && !jogo.jogador.lanterna) return I_LANTERNA;
    if (r <= 30) return I_POCAO_GRANDE;
    if (r <= 40) return I_ETER;
    if (r <= 50) return chance(jogo.sorte, 50) ? I_BOMBA_DE_FUMACA : I_PERGAMINHO_DO_MAPA;
    if (r <= 65) return I_PERGAMINHO_DE_FOGO + sortear(jogo.sorte, 0, 4);
    std::vector<int> opcoes;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        const TipoItem& t = TIPOS_DE_ITEM[i];
        if ((t.categoria == ARMA || t.categoria == ESCUDO) && i != I_ADAGA &&
            jogo.andar >= t.andarMinimo && jogo.andar <= t.andarMaximo) opcoes.push_back(i);
    }
    if (opcoes.empty()) return I_POCAO_GRANDE;
    return opcoes[sortear(jogo.sorte, 0, (int)opcoes.size() - 1)];
}

static bool usarFonte(Jogo& jogo, Especial& e) {
    Jogador& j = jogo.jogador;
    if (e.usado) {
        adicionarMensagem(jogo, "A fonte secou. Só sobrou um fio de água no fundo.");
        return false;
    }
    bool ppCheio = true;
    for (int s = 0; s < j.numGolpes; s++) if (j.pp[s] < GOLPES[j.golpes[s]].ppMaximo) ppCheio = false;
    if (j.vida >= j.vidaMaxima && ppCheio) {
        adicionarMensagem(jogo, "Uma fonte de água cristalina. Você está bem; melhor guardar para depois.");
        return false;
    }
    j.vida = j.vidaMaxima;
    for (int s = 0; s < j.numGolpes; s++) j.pp[s] = GOLPES[j.golpes[s]].ppMaximo;
    e.usado = true;
    tocarSom(SOM_ITEM);
    efeitoNoMapa(jogo, EFEITO_CURA, j.x, j.y, j.vidaMaxima);
    adicionarMensagem(jogo, "Você bebe da fonte cristalina: vida e PP no máximo! A fonte seca.", MSG_BOA);
    return true;
}

static bool usarAltar(Jogo& jogo, Especial& e) {
    Jogador& j = jogo.jogador;
    if (e.usado) {
        adicionarMensagem(jogo, "O altar está frio. O pacto já foi feito.");
        return false;
    }
    if (j.vidaMaxima <= 20) {
        adicionarMensagem(jogo, "O altar de sangue sussurra: \"Volte quando tiver mais vida para dar.\"");
        return false;
    }
    if (jogo.modoAutomatico) return false;   // os testes automáticos não fazem pactos
    std::vector<std::string> opcoes = { "Aceitar o pacto", "Recusar" };
    std::vector<std::string> descricoes = {
        "Perde 5 de vida máxima PARA SEMPRE e ganha +2 de ataque PARA SEMPRE.  |  Vida máxima: " +
            std::to_string(j.vidaMaxima) + " -> " + std::to_string(j.vidaMaxima - 5) + ".   Ataque: " +
            std::to_string(j.ataque) + " -> " + std::to_string(j.ataque + 2) + ".",
        "Deixar o altar em paz.",
    };
    int escolha = escolherEmCaixa(jogo, "ALTAR DE SANGUE: aceitar o pacto?", opcoes, descricoes, 1);
    if (escolha != 0) {
        adicionarMensagem(jogo, "Você se afasta do altar.");
        return false;
    }
    j.bonusVida -= 5;
    j.bonusAtaque += 2;
    recalcularAtributos(j);
    if (j.vida > j.vidaMaxima) j.vida = j.vidaMaxima;
    e.usado = true;
    tocarSom(SOM_DANO);
    efeitoNoMapa(jogo, EFEITO_ARMADILHA, e.x, e.y, 5);
    adicionarMensagem(jogo, "O altar bebe o seu sangue... -5 de vida máxima, +2 de ataque!", MSG_BOA);
    return true;
}

static bool usarBau(Jogo& jogo, int indice) {
    Andar& andar = andarAtual(jogo);
    Especial& e = andar.especiais[indice];
    if (e.usado) {
        adicionarMensagem(jogo, "O baú está vazio.");
        return false;
    }
    e.usado = true;
    if (e.mimico) {
        // O baú tinha dentes! Ele vira um monstro e a batalha começa na hora.
        int tipo = tipoDoMimico();
        int m = criarMonstroExtra(andar, tipo, e.x, e.y, 2 * jogo.andar + 1, jogo.elementoDaEspecie[tipo]);
        // O baú some do mapa (o mímico fica no lugar dele).
        andar.especiais[indice] = andar.especiais[--andar.numEspeciais];
        if (m < 0) return false;
        tocarSom(SOM_DANO);
        adicionarMensagem(jogo, "O baú abre a boca... É um MÍMICO!", MSG_PERIGO);
        jogo.batalhaPendente = m;
        return true;
    }
    int item = itemDoBau(jogo);
    tocarSom(SOM_ITEM);
    efeitoNoMapa(jogo, EFEITO_ITEM, e.x, e.y, item);
    int moedas = 5 + 2 * jogo.andar + sortear(jogo.sorte, 0, 5);
    jogo.jogador.moedas += moedas;
    adicionarMensagem(jogo, "O baú dourado se abre: " + nomeComArtigo(item) + " e " + emMoedas(moedas) + "!", MSG_BOA);
    guardarOuLargar(jogo, item);
    jogo.itensColetados++;
    return true;
}

// ---- A arena ----------------------------------------------------------------
// Casas da sala da arena: o que fica isolado quando a porta vira parede.
static void casasDaArena(Andar& a, const Especial& e, int dentro[MAPA_ALTURA][MAPA_LARGURA]) {
    a.mapa.celulas[e.portaY][e.portaX] = PAREDE;
    calcularDistancias(a.mapa, e.x, e.y, dentro);
    a.mapa.celulas[e.portaY][e.portaX] = PORTA_TRANCADA;
}

static void chamarOnda(Jogo& jogo, Especial& e) {
    Andar& a = andarAtual(jogo);
    static int dentro[MAPA_ALTURA][MAPA_LARGURA];
    casasDaArena(a, e, dentro);
    int quantos = (e.onda == ONDAS_DA_ARENA) ? 3 : 2;
    e.numMonstros = 0;
    for (int k = 0; k < quantos; k++) {
        // Nasce longe do jogador (pelo menos 3 casas), num chão livre da sala.
        for (int t = 0; t < 300; t++) {
            int x = sortear(jogo.sorte, 1, MAPA_LARGURA - 2), y = sortear(jogo.sorte, 1, MAPA_ALTURA - 2);
            if (dentro[y][x] == INALCANCAVEL || a.mapa.celulas[y][x] != CHAO) continue;
            if (std::abs(x - jogo.jogador.x) < 3 && std::abs(y - jogo.jogador.y) < 3) continue;
            if (monstroEm(a, x, y) >= 0 || especialEm(a, x, y) >= 0 || npcEm(a, x, y) >= 0) continue;
            int tipo = sortearTipoDoAndar(jogo.sorte, jogo.andar);
            if (tipo < 0) break;
            int m = criarMonstroExtra(a, tipo, x, y, 2 * jogo.andar + 1, jogo.elementoDaEspecie[tipo]);
            if (m >= 0) e.monstros[e.numMonstros++] = m;
            break;
        }
    }
    efeitoNoMapa(jogo, EFEITO_ATORDOAR, e.x, e.y, 0);
    adicionarMensagem(jogo, "ARENA: onda " + std::to_string(e.onda) + " de " + std::to_string(ONDAS_DA_ARENA) +
                            "! Monstros surgem das sombras.", MSG_PERIGO);
}

static bool usarGongo(Jogo& jogo, Especial& e) {
    if (e.usado) {
        adicionarMensagem(jogo, "O gongo está rachado. A arena já foi vencida.");
        return false;
    }
    if (e.onda > 0) {
        adicionarMensagem(jogo, "O desafio já começou: derrote os monstros da onda " + std::to_string(e.onda) + "!");
        return false;
    }
    if (jogo.modoAutomatico) return false;   // os testes automáticos não aceitam desafios
    int premio = 20 + 5 * jogo.andar;
    std::vector<std::string> opcoes = { "Aceitar o desafio", "Agora não" };
    std::vector<std::string> descricoes = {
        "A porta tranca e vêm 3 ondas de monstros (2, 2 e 3).  |  Prêmio: um cristal e " + emMoedas(premio) + ".",
        "Você pode voltar depois.",
    };
    int escolha = escolherEmCaixa(jogo, "GONGO DA ARENA: aceitar o desafio?", opcoes, descricoes, 1);
    if (escolha != 0) return false;
    Andar& a = andarAtual(jogo);
    a.mapa.celulas[e.portaY][e.portaX] = PORTA_TRANCADA;
    a.descoberto[e.portaY][e.portaX] = true;
    tocarSom(SOM_PORTA);
    adicionarMensagem(jogo, "BONG! A porta da arena se tranca atrás de você.", MSG_PERIGO);
    e.onda = 1;
    chamarOnda(jogo, e);
    return true;
}

bool usarEspecial(Jogo& jogo, int indice) {
    Especial& e = andarAtual(jogo).especiais[indice];
    switch (e.tipo) {
        case ESP_FONTE: return usarFonte(jogo, e);
        case ESP_ALTAR: return usarAltar(jogo, e);
        case ESP_BAU:   return usarBau(jogo, indice);
        case ESP_GONGO: return usarGongo(jogo, e);
    }
    return false;
}

void atualizarSalasEspeciais(Jogo& jogo) {
    if (jogo.morto || jogo.venceu) return;
    Andar& a = andarAtual(jogo);
    for (int i = 0; i < a.numEspeciais; i++) {
        Especial& e = a.especiais[i];
        if (e.tipo != ESP_GONGO || e.onda <= 0) continue;
        bool algumVivo = false;
        for (int k = 0; k < e.numMonstros; k++) {
            const Monstro& m = a.monstros[e.monstros[k]];
            if (m.vivo) algumVivo = true;
        }
        if (algumVivo) continue;
        if (e.onda < ONDAS_DA_ARENA) {
            e.onda++;
            chamarOnda(jogo, e);
            continue;
        }
        // Venceu as 3 ondas!
        e.onda = 0;
        e.usado = true;
        e.numMonstros = 0;
        a.mapa.celulas[e.portaY][e.portaX] = PORTA;
        int premio = 20 + 5 * jogo.andar;
        jogo.jogador.moedas += premio;
        tocarSom(SOM_VITORIA);
        efeitoNoMapa(jogo, EFEITO_MOEDAS, e.x, e.y, premio);
        adicionarMensagem(jogo, "ARENA VENCIDA! A porta se abre. Você ganha " + emMoedas(premio) + " e um cristal!", MSG_BOA);
        ganharCristal(jogo);
    }
}

bool portaDaArenaFechada(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numEspeciais; i++) {
        const Especial& e = andar.especiais[i];
        if (e.tipo == ESP_GONGO && e.onda > 0 && e.portaX == x && e.portaY == y) return true;
    }
    return false;
}

void desistirDaArena(Jogo& jogo) {
    Andar& a = andarAtual(jogo);
    for (int i = 0; i < a.numEspeciais; i++) {
        Especial& e = a.especiais[i];
        if (e.tipo != ESP_GONGO || e.onda <= 0) continue;
        // Os monstros da onda somem e a porta abre: dá para tentar de novo.
        for (int k = 0; k < e.numMonstros; k++) a.monstros[e.monstros[k]].vivo = false;
        e.numMonstros = 0;
        e.onda = 0;
        a.mapa.celulas[e.portaY][e.portaX] = PORTA;
        adicionarMensagem(jogo, "O desafio da arena acabou. O gongo espera por uma nova tentativa.");
    }
}

void recompensaDoMimico(Jogo& jogo, const Monstro& m) {
    if (m.tipo != tipoDoMimico()) return;
    int moedas = 25 + 5 * jogo.andar;
    jogo.jogador.moedas += moedas;
    adicionarMensagem(jogo, "Dentro do mímico havia " + emMoedas(moedas) + " e um cristal!", MSG_BOA);
    ganharCristal(jogo);
}
