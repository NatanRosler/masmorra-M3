#include "conquistas.h"
#include "jogo.h"
#include "npcs.h"
#include "salas.h"
#include "batalha.h"
#include "bestiario.h"
#include "som.h"
#include <fstream>
#include <string>

const FichaDaConquista CONQUISTAS[NUM_CONQUISTAS] = {
    { "primeiros_passos", "Primeiros passos",       "Vencer a primeira batalha",                              GRUPO_JORNADA },
    { "rei_deposto",      "Rei deposto",            "Vencer o Rei Goblin",                                    GRUPO_JORNADA },
    { "fim_do_ninho",     "Fim do ninho",           "Vencer a Rainha Aranha",                                 GRUPO_JORNADA },
    { "ossos_no_chao",    "Ossos no chão",          "Vencer o Necromante",                                    GRUPO_JORNADA },
    { "coracao_apagado",  "Coração apagado",        "Vencer o Golem de Magma",                                GRUPO_JORNADA },
    { "libertador",       "Libertador de Valdoria", "Vencer o Senhor das Profundezas",                        GRUPO_JORNADA },
    { "coragem",          "Coragem",                "Vencer o jogo no Difícil",                               GRUPO_DESAFIO },
    { "lenda",            "Lenda",                  "Vencer o jogo no Impossível",                            GRUPO_DESAFIO },
    { "sem_tropecos",     "Sem tropeços",           "Chegar ao andar 13 sem perder nenhuma vida",             GRUPO_DESAFIO },
    { "relampago",        "Relâmpago",              "Vencer o jogo em menos de 45 minutos",                   GRUPO_DESAFIO },
    { "tesouro",          "Caçador de tesouros",    "Abrir uma sala do tesouro",                              GRUPO_EXPLORACAO },
    { "gladiador",        "Gladiador",              "Vencer uma arena",                                       GRUPO_EXPLORACAO },
    { "desconfiado",      "Desconfiado",            "Derrotar um mímico",                                     GRUPO_EXPLORACAO },
    { "bom_coracao",      "Bom coração",            "Ajudar o Tobias",                                        GRUPO_EXPLORACAO },
    { "velhos_amigos",    "Velhos amigos",          "Conversar com Eldra, Borin e Aldric na mesma partida",   GRUPO_EXPLORACAO },
    { "bolso_cheio",      "Bolso cheio",            "Ter 500 moedas ao mesmo tempo",                          GRUPO_EXPLORACAO },
    { "muitos_elementos", "Muitos elementos",       "Saber golpes de 3 elementos (fora Normal)",   GRUPO_EXPLORACAO },
    { "na_surdina",       "Na surdina",             "Pegar um monstro dormindo ou de surpresa",         GRUPO_EXPLORACAO },
    { "naturalista",      "Naturalista",            "Ver todas as espécies no bestiário",                     GRUPO_EXPLORACAO },
};
const char* NOMES_DOS_GRUPOS[NUM_GRUPOS_DE_CONQUISTA] = { "JORNADA", "DESAFIO", "EXPLORAÇÃO" };

static const char* ARQUIVO_DAS_CONQUISTAS = "conquistas.txt";
static bool ganhas[NUM_CONQUISTAS];
static bool podeGravar = false;   // só depois de carregarConquistas()

void carregarConquistas() {
    for (bool& g : ganhas) g = false;
    std::ifstream arquivo(ARQUIVO_DAS_CONQUISTAS);
    std::string linha;
    while (std::getline(arquivo, linha)) {
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();   // arquivo do Windows
        for (int i = 0; i < NUM_CONQUISTAS; i++) if (linha == CONQUISTAS[i].chave) ganhas[i] = true;
    }
    podeGravar = true;
}

static void gravar() {
    if (!podeGravar) return;
    std::ofstream arquivo(ARQUIVO_DAS_CONQUISTAS);
    if (!arquivo) return;   // sem permissão para gravar: segue sem salvar
    arquivo << "# Conquistas do Masmorra (pode apagar este arquivo: tudo volta a zero)\n";
    for (int i = 0; i < NUM_CONQUISTAS; i++) if (ganhas[i]) arquivo << CONQUISTAS[i].chave << "\n";
}

bool conquistaGanha(int id) { return id >= 0 && id < NUM_CONQUISTAS && ganhas[id]; }

int conquistasGanhas() {
    int n = 0;
    for (bool g : ganhas) n += g;
    return n;
}

bool ganhouNaPartida(const Jogo& jogo, int id) { return (jogo.conquistasDaPartida >> id) & 1u; }
bool novaNaPartida(const Jogo& jogo, int id) { return (jogo.conquistasNovas >> id) & 1u; }

void ganharConquista(Jogo& jogo, int id) {
    if (jogo.modoAutomatico || id < 0 || id >= NUM_CONQUISTAS) return;
    jogo.conquistasDaPartida |= 1u << id;
    if (ganhas[id]) return;   // já tinha de outra partida: só marca na desta
    ganhas[id] = true;
    jogo.conquistasNovas |= 1u << id;
    gravar();
    tocarSom(SOM_PONTO_GANHO);
    adicionarMensagem(jogo, std::string("Conquista: ") + CONQUISTAS[id].nome + "!  (" + CONQUISTAS[id].comoGanhar + ")", MSG_BOA);
}

void conferirConquistas(Jogo& jogo) {
    if (jogo.modoAutomatico) return;
    const Jogador& j = jogo.jogador;
    if (j.moedas >= 500) ganharConquista(jogo, C_BOLSO_CHEIO);

    // Golpes de 3 elementos diferentes (fora Normal) ao mesmo tempo.
    int elementos = 0;
    for (int s = 0; s < j.numGolpes; s++) {
        int e = GOLPES[j.golpes[s]].elemento;
        if (e != NORMAL) elementos |= 1 << e;
    }
    int quantos = 0;
    for (int e = 0; e < NUM_ELEMENTOS; e++) quantos += (elementos >> e) & 1;
    if (quantos >= 3) ganharConquista(jogo, C_MUITOS_ELEMENTOS);

    // Conversas, arena e tesouro (em qualquer andar já criado).
    bool eldra = false, borin = false, aldric = false;
    for (int a = 0; a < NUM_ANDARES; a++) {
        const Andar& andar = jogo.andares[a];
        if (!andar.gerado) continue;
        for (int i = 0; i < andar.numNpcs; i++) {
            const Npc& p = andar.npcs[i];
            if (!p.conversou) continue;
            if (p.tipo == NPC_EXPLORADORA) eldra = true;
            if (p.tipo == NPC_MINEIRO) borin = true;
            if (p.tipo == NPC_ESPIRITO) aldric = true;
            if (p.tipo == NPC_VIAJANTE) ganharConquista(jogo, C_BOM_CORACAO);
        }
        for (int i = 0; i < andar.numEspeciais; i++)
            if (andar.especiais[i].tipo == ESP_GONGO && andar.especiais[i].usado) ganharConquista(jogo, C_GLADIADOR);
        // Tesouro: a porta aberta e o jogador já viu lá dentro.
        if (andar.tesouro.existe && andar.mapa.celulas[andar.tesouro.portaY][andar.tesouro.portaX] != PORTA_TRANCADA &&
            andar.descoberto[andar.tesouro.dentroY][andar.tesouro.dentroX])
            ganharConquista(jogo, C_TESOURO);
    }
    if (eldra && borin && aldric) ganharConquista(jogo, C_VELHOS_AMIGOS);
    if (especiesVistas() >= especiesDoBestiario()) ganharConquista(jogo, C_NATURALISTA);
}
