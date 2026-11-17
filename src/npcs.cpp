// npcs.cpp
#include "npcs.h"
#include "jogo.h"
#include "pontuacao.h"
#include "som.h"

const FichaNpc FICHAS_DOS_NPCS[NUM_TIPOS_DE_NPC] = {
    { "Eldra, a exploradora",  '&', { 120, 210, 230 }, 1 },
    { "Borin, o velho mineiro", '&', { 230, 170,  90 }, 4 },
    { "O espírito do guardião", '&', { 200, 200, 255 }, 7 },
};

void colocarNpcs(Andar& andar, int numeroDoAndar, int chegadaX, int chegadaY) {
    andar.numNpcs = 0;
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, chegadaX, chegadaY, distancia);

    for (int tipo = 0; tipo < NUM_TIPOS_DE_NPC && andar.numNpcs < MAX_NPCS; tipo++) {
        if (FICHAS_DOS_NPCS[tipo].andar != numeroDoAndar) continue;
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
        if (melhorX < 0) continue;
        Npc& npc = andar.npcs[andar.numNpcs++];
        npc.tipo = tipo;
        npc.x = melhorX;
        npc.y = melhorY;
        npc.conversou = false;
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

    switch (npc.tipo) {
        case NPC_EXPLORADORA:
            falas = {
                "\"" + nome + "? Então você também caiu na armadilha das portas.\"",
                "\"Eu sou Eldra. Desci até o quarto andar antes de voltar ferida.\"",
                "",
                "\"Os monstros mudam a cada vez que alguém entra aqui: o rato de",
                "fogo de hoje pode ser de água amanhã. Olhe o elemento deles!\"",
                "",
                "\"O chão esconde armadilhas de espinhos. Se vir uma, empurre uma",
                "caixa para cima dela: os espinhos quebram a caixa e param.\"",
            };
            if (!npc.conversou) {
                j.mochila[I_POCAO_DE_VIDA]++;
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
                adicionarMensagem(jogo, "Borin destrancou a sala do tesouro deste andar!");
                tocarSom(SOM_PORTA);
            } else if (!npc.conversou) {
                j.mochila[I_ETER]++;
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
