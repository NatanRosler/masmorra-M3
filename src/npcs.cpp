// npcs.cpp
#include "npcs.h"
#include "jogo.h"
#include "pontuacao.h"
#include "som.h"
#include "loja.h"
#include <cstdlib>

const FichaNpc FICHAS_DOS_NPCS[NUM_TIPOS_DE_NPC] = {
    { "Eldra, a exploradora",   '&', { 120, 210, 230 }, 1, 0 },
    { "Borin, o velho mineiro", '&', { 230, 170,  90 }, 4, 0 },
    { "O espírito do guardião", '&', { 200, 200, 255 }, 7, 0 },
    { "Garro, o comerciante",   '$', { 255, 205,  70 }, 3, 3 },   // andares 3, 6 e 9
};

// O NPC deste tipo aparece neste andar?
static bool apareceNoAndar(const FichaNpc& ficha, int numeroDoAndar) {
    if (numeroDoAndar == ficha.andar) return true;
    if (ficha.repeteACada <= 0 || numeroDoAndar < ficha.andar || numeroDoAndar >= NUM_ANDARES) return false;
    return (numeroDoAndar - ficha.andar) % ficha.repeteACada == 0;
}

bool temComercianteNoAndar(int numeroDoAndar) {
    return apareceNoAndar(FICHAS_DOS_NPCS[NPC_COMERCIANTE], numeroDoAndar);
}

// =============================================================================
// A LOJINHA DO COMERCIANTE
// =============================================================================
// Mesma ideia da sala do tesouro (gerador.cpp): testa cada porta como se fosse
// parede. Se isso isola uma salinha (de 9 a 80 casas, sem escada e sem outra
// porta), ela é candidata. Fica a mais perto da chegada.
void prepararSalaDaLoja(Andar& andar, int numeroDoAndar) {
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++) andar.zonaSegura[y][x] = false;
    andar.loja.existe = false;
    if (!temComercianteNoAndar(numeroDoAndar)) return;

    Mapa& mapa = andar.mapa;
    const Escadas& e = andar.escadas;
    static int antes[MAPA_ALTURA][MAPA_LARGURA], depois[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, antes);

    int melhorX = -1, melhorY = -1, melhorDistancia = 1000000;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (mapa.celulas[y][x] != PORTA || antes[y][x] == INALCANCAVEL) continue;
            if (antes[y][x] >= melhorDistancia) continue;

            mapa.celulas[y][x] = PAREDE;                     // e se fosse parede?
            calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, depois);
            mapa.celulas[y][x] = PORTA;                      // desfaz o teste
            if (e.temDescida && depois[e.chegadaDesceY][e.chegadaDesceX] == INALCANCAVEL) continue;

            int isoladas = 0;
            bool problema = false;
            for (int cy = 1; cy < MAPA_ALTURA - 1 && !problema; cy++) {
                for (int cx = 1; cx < MAPA_LARGURA - 1; cx++) {
                    if (antes[cy][cx] == INALCANCAVEL || depois[cy][cx] != INALCANCAVEL) continue;
                    if (cx == x && cy == y) continue;
                    isoladas++;
                    TipoCelula c = mapa.celulas[cy][cx];
                    if (c != CHAO) { problema = true; break; }     // escada ou outra porta lá dentro
                    for (int d = 0; d < 4; d++) {
                        if (mapa.celulas[cy + DIRECOES_DY[d]][cx + DIRECOES_DX[d]] == PORTA_TRANCADA) problema = true;
                    }
                }
            }
            if (problema || isoladas < 9 || isoladas > 80) continue;
            melhorX = x;
            melhorY = y;
            melhorDistancia = antes[y][x];
        }
    }
    if (melhorX < 0) return;   // sem salinha: o comerciante ganha só um tapete (colocarNpcs)

    // Marca a sala (e a porta) como zona segura.
    mapa.celulas[melhorY][melhorX] = PAREDE;
    calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, depois);
    mapa.celulas[melhorY][melhorX] = PORTA;
    long somaX = 0, somaY = 0;
    int qtd = 0;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (antes[y][x] != INALCANCAVEL && depois[y][x] == INALCANCAVEL) {
                andar.zonaSegura[y][x] = true;
                somaX += x; somaY += y; qtd++;
            }
        }
    }
    andar.zonaSegura[melhorY][melhorX] = true;
    andar.loja.existe = true;
    andar.loja.portaX = melhorX;
    andar.loja.portaY = melhorY;

    // Balcão: a casa da sala mais perto do meio dela, longe da porta.
    float meioX = (float)somaX / qtd, meioY = (float)somaY / qtd;
    float melhor = 1e9f;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (!andar.zonaSegura[y][x] || mapa.celulas[y][x] != CHAO) continue;
            if (std::abs(x - melhorX) <= 1 && std::abs(y - melhorY) <= 1) continue;
            float d = (x - meioX) * (x - meioX) + (y - meioY) * (y - meioY);
            if (d < melhor) { melhor = d; andar.loja.balcaoX = x; andar.loja.balcaoY = y; }
        }
    }

    // Lampião: a parede da sala mais longe da porta vira uma tocha.
    int lampiaoX = -1, lampiaoY = -1, maisLonge = -1;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (mapa.celulas[y][x] != PAREDE) continue;
            bool encosta = false;
            for (int d = 0; d < 4; d++) {
                int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
                if (andar.zonaSegura[ny][nx] && mapa.celulas[ny][nx] == CHAO) encosta = true;
            }
            int distancia = std::abs(x - melhorX) + std::abs(y - melhorY);
            if (encosta && distancia > maisLonge) { maisLonge = distancia; lampiaoX = x; lampiaoY = y; }
        }
    }
    if (lampiaoX >= 0) mapa.celulas[lampiaoY][lampiaoX] = TOCHA;
}

// Tira da lojinha o que não combina com ela: caixas, armadilhas e monstros.
static void protegerSalaDaLoja(Andar& andar) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (!andar.zonaSegura[y][x]) continue;
            if (andar.mapa.celulas[y][x] == CAIXA) {
                andar.mapa.celulas[y][x] = CHAO;
                andar.conteudoDasCaixas[y][x] = -1;
            }
        }
    }
    for (int i = 0; i < andar.numArmadilhas; i++) {
        Armadilha& a = andar.armadilhas[i];
        if (andar.zonaSegura[a.y][a.x]) {
            andar.armadilhas[i] = andar.armadilhas[--andar.numArmadilhas];
            i--;
        }
    }
    for (int i = 0; i < andar.numMonstros; i++) {
        Monstro& m = andar.monstros[i];
        if (!m.vivo || !andar.zonaSegura[m.y][m.x]) continue;
        m.vivo = false;
        // Se ele carregava a chave do andar, ela fica no chão.
        if (m.carregaChave && andar.numItens < MAX_ITENS_NO_CHAO) {
            ItemNoChao& chave = andar.itens[andar.numItens++];
            chave.ativo = true;
            chave.tipo = I_CHAVE;
            chave.x = m.x;
            chave.y = m.y;
            chave.visto = false;
        }
    }
}

void colocarNpcs(Andar& andar, int numeroDoAndar, int chegadaX, int chegadaY) {
    andar.numNpcs = 0;
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, chegadaX, chegadaY, distancia);

    for (int tipo = 0; tipo < NUM_TIPOS_DE_NPC && andar.numNpcs < MAX_NPCS; tipo++) {
        if (!apareceNoAndar(FICHAS_DOS_NPCS[tipo], numeroDoAndar)) continue;
        // O comerciante fica no balcão da lojinha dele.
        if (tipo == NPC_COMERCIANTE && andar.loja.existe) {
            Npc& npc = andar.npcs[andar.numNpcs++];
            npc.tipo = tipo;
            npc.x = andar.loja.balcaoX;
            npc.y = andar.loja.balcaoY;
            npc.conversou = false;
            npc.estoque[0] = npc.estoque[1] = -1;
            for (int k = 0; k < 4; k++) npc.pocoesAVenda[k] = 0;
            int item = itemEm(andar, npc.x, npc.y);
            if (item >= 0) andar.itens[item].ativo = false;   // nada embaixo do balcão
            protegerSalaDaLoja(andar);
            continue;
        }
        // O lugar mais perto da chegada que esteja a pelo menos 4 passos dela.
        int melhorX = -1, melhorY = -1, melhor = 1000000;
        for (int y = 1; y < MAPA_ALTURA - 1; y++) {
            for (int x = 1; x < MAPA_LARGURA - 1; x++) {
                int d = distancia[y][x];
                if (andar.mapa.celulas[y][x] != CHAO || d == INALCANCAVEL || d < 4 || d >= melhor) continue;
                // Só no meio de uma sala (8 vizinhas livres): o NPC nunca tranca um corredor.
                bool livreEmVolta = true;
                for (int k = 0; k < 8; k++) {
                    TipoCelula v = andar.mapa.celulas[y + DIRECOES_DY[k]][x + DIRECOES_DX[k]];
                    if (v != CHAO) livreEmVolta = false;
                }
                if (!livreEmVolta) continue;
                if (monstroEm(andar, x, y) >= 0 || itemEm(andar, x, y) >= 0 || armadilhaEm(andar, x, y) >= 0) continue;
                if (npcEm(andar, x, y) >= 0) continue;
                melhor = d;
                melhorX = x;
                melhorY = y;
            }
        }
        if (melhorX < 0) {
            // Sem lugar para o Borin: a porta já fica aberta (este andar não tem chave).
            if (tipo == NPC_MINEIRO && andar.tesouro.existe) andar.mapa.celulas[andar.tesouro.portaY][andar.tesouro.portaX] = PORTA;
            continue;
        }
        Npc& npc = andar.npcs[andar.numNpcs++];
        npc.tipo = tipo;
        npc.x = melhorX;
        npc.y = melhorY;
        npc.conversou = false;
        npc.estoque[0] = npc.estoque[1] = -1;
        for (int k = 0; k < 4; k++) npc.pocoesAVenda[k] = 0;
        if (tipo == NPC_COMERCIANTE) {
            // Sem salinha: um tapete 5x5 em volta dele vira a área protegida.
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    int x = melhorX + dx, y = melhorY + dy;
                    if (dentroDoMapa(x, y) && andar.mapa.celulas[y][x] == CHAO && distancia[y][x] != INALCANCAVEL)
                        andar.zonaSegura[y][x] = true;
                }
            protegerSalaDaLoja(andar);
        }
    }
}

int npcEm(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numNpcs; i++) {
        if (andar.npcs[i].x == x && andar.npcs[i].y == y) return i;
    }
    return -1;
}

// Abre a sala do tesouro deste andar, se ela ainda estiver trancada.
// Devolve true se a porta foi aberta.
static bool abrirSalaDoTesouro(Andar& andar) {
    if (!andar.tesouro.existe) return false;
    TipoCelula& porta = andar.mapa.celulas[andar.tesouro.portaY][andar.tesouro.portaX];
    if (porta != PORTA_TRANCADA) return false;
    porta = PORTA;
    return true;
}

void conversarComNpc(Jogo& jogo, int indice) {
    Andar& andar = andarAtual(jogo);
    Npc& npc = andar.npcs[indice];
    Jogador& j = jogo.jogador;
    const std::string nome = j.nome;
    std::vector<std::string> falas;

    // O comerciante: na primeira vez ele se apresenta; depois, abre direto a loja.
    if (npc.tipo == NPC_COMERCIANTE) {
        if (!npc.conversou) {
            prepararEstoque(jogo, npc);
            if (jogo.andar <= 3) {
                falas = {
                    "\"Um cliente! Faz semanas que ninguém passa por aqui... vivo.\"",
                    "\"Sou Garro. Vendo de tudo um pouco: poções, armas, escudos.\"",
                    "",
                    "\"Os monstros carregam moedas, sabia? Algumas caixas também.",
                    "E quem fica mais forte sempre acha umas moedinhas no bolso.\"",
                    "",
                    "\"Vou descendo junto com você. Procure por mim mais abaixo!\"",
                };
            } else if (jogo.andar < NUM_ANDARES - 1) {
                falas = {
                    "\"" + nome + "! Eu disse que a gente se via de novo.\"",
                    "",
                    "\"Trouxe mercadoria nova. Lá embaixo o ar é pesado, então",
                    "não economize nas poções.\"",
                };
            } else {
                falas = {
                    "\"Psiu! " + nome + "! O Senhor das Profundezas está logo abaixo.\"",
                    "",
                    "\"Moeda não serve de nada no estômago de um monstro.",
                    "Melhor gastar tudo agora, não acha?\"",
                };
            }
            npc.conversou = true;
            if (!jogo.modoAutomatico) mostrarPagina(FICHAS_DOS_NPCS[npc.tipo].nome, falas);
        }
        abrirLoja(jogo, npc);
        return;
    }

    switch (npc.tipo) {
        case NPC_EXPLORADORA:
            falas = {
                "\"" + nome + "? Então você também caiu na armadilha das portas.\"",
                "\"Eu sou Eldra. Desci até o quarto andar antes de voltar ferida.\"",
                "",
                "\"Os monstros mudam a cada vez que alguém entra aqui: o rato de",
                "fogo de hoje pode ser de água amanhã. Olhe o elemento deles!\"",
                "",
                "\"Está vendo essas caixas? Empurre uma contra um monstro e ele",
                "fica tonto. E quebradas, às vezes elas escondem poções e armas!\"",
            };
            if (!npc.conversou) {
                guardarOuLargar(jogo, I_POCAO_DE_VIDA);
                falas.push_back("");
                falas.push_back("\"Leve esta poção. Vai precisar mais do que eu.\"  (+1 Poção de Vida)");
            }
            break;

        case NPC_MINEIRO:
            falas = {
                "\"Ei, cuidado onde pisa! Esta mina era minha, antes do Senhor",
                "das Profundezas transformá-la nesta masmorra.\"",
                "",
            };
            if (!npc.conversou && abrirSalaDoTesouro(andar)) {
                falas.push_back("\"Ainda tenho a chave mestra. Vou destrancar a sala do tesouro");
                falas.push_back("deste andar para você. Faça bom uso do que estiver lá!\"");
                falas.push_back("");
                falas.push_back("(Você ouve uma fechadura se abrir em algum lugar do andar.)");
                adicionarMensagem(jogo, "Borin destrancou a sala do tesouro deste andar!", MSG_BOA);
                tocarSom(SOM_PORTA);
            } else if (!npc.conversou) {
                guardarOuLargar(jogo, I_ETER);
                falas.push_back("\"Não tenho mais portas para abrir, mas leve este Éter.\"  (+1 Éter)");
            } else {
                falas.push_back("\"Lá embaixo os ogros quebram tudo o que veem pela frente.\"");
            }
            break;

        case NPC_ESPIRITO:
            falas = {
                "Uma luz fria toma a forma de um velho guerreiro.",
                "",
                "\"Fui o último guardião desta mina. O Senhor das Profundezas",
                "espera no décimo andar. Dele não há como fugir: só vencer.\"",
            };
            if (!npc.conversou) {
                j.vida = j.vidaMaxima;
                for (int s = 0; s < j.numGolpes; s++) j.pp[s] = GOLPES[j.golpes[s]].ppMaximo;
                falas.push_back("");
                falas.push_back("\"Deixe-me curar suas feridas uma última vez.\"  (vida e PP cheios)");
            }
            break;
    }
    npc.conversou = true;
    if (!jogo.modoAutomatico) mostrarPagina(FICHAS_DOS_NPCS[npc.tipo].nome, falas);
}
