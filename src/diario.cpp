#include "diario.h"
#include "jogo.h"
#include "monstros.h"
#include "npcs.h"
#include "salas.h"
#include "itens.h"
#include "sala_do_chefe.h"

// O chefe que vive num andar (mini-chefe ou o chefe final), vivo ou já
// vencido. -1 = o andar não tem (ou ainda não foi criado).
static int chefeDoAndar(const Andar& a) {
    for (int i = 0; i < a.numMonstros; i++)
        if (tipoDe(a.monstros[i]).especial & (MINI_CHEFE | CHEFE)) return i;
    return -1;
}

std::vector<EtapaPrincipal> etapasDaMissaoPrincipal(const Jogo& jogo) {
    std::vector<EtapaPrincipal> etapas;
    const int ANDARES[] = { 3, 6, 9, 12, NUM_ANDARES };
    for (int n : ANDARES) {
        EtapaPrincipal e{ n, "", -1, false, false };
        const Andar& a = jogo.andares[n - 1];
        if (a.gerado) {
            int c = chefeDoAndar(a);
            if (c < 0) e.semChefe = true;
            else {
                e.nome = tipoDe(a.monstros[c]).nome;
                e.especie = a.monstros[c].tipo;
                e.vencido = !a.monstros[c].vivo;
            }
        }
        etapas.push_back(e);
    }
    return etapas;
}

std::vector<Missao> missoesDoAndar(const Jogo& jogo) {
    std::vector<Missao> lista;
    const Andar& a = andarAtual(jogo);
    const Jogador& j = jogo.jogador;
    int n = jogo.andar;
    auto nova = [&](int tipo, const std::string& titulo, const std::string& detalhe, bool feita, bool obrigatoria) {
        lista.push_back({ tipo, titulo, detalhe, feita, obrigatoria, -1, -1 });
        return &lista.back();
    };

    // ---- Obrigatórias: o chefe do andar e a escada ----
    int c = chefeDoAndar(a);
    if (c >= 0) {
        const Monstro& m = a.monstros[c];
        const TipoMonstro& t = tipoDe(m);
        bool final = t.especial & CHEFE;
        // Já viu o chefe ou a sala dele?
        bool viu = a.descoberto[m.y][m.x] ||
                   (a.salaDoChefe.existe && a.descoberto[a.salaDoChefe.portaoY][a.salaDoChefe.portaoX]);
        Missao* x = nova(MISSAO_CHEFE, viu ? t.nome : "???",
                         !m.vivo ? "vencido" : final ? "o fim da masmorra" : viu ? "lacra a escada" : "guarda a escada",
                         !m.vivo, true);
        if (viu) x->especie = m.tipo;
    }
    if (n < NUM_ANDARES) {
        bool achou = a.descoberto[a.escadas.desceY][a.escadas.desceX];
        nova(MISSAO_ESCADA, "Escada", achou ? "encontrada" : "para o andar " + std::to_string(n + 1), achou, true);
    }

    // ---- A sala do tesouro ----
    int borin = -1;
    for (int i = 0; i < a.numNpcs; i++) if (a.npcs[i].tipo == NPC_MINEIRO) borin = i;
    if (a.tesouro.existe) {
        bool aberta = a.mapa.celulas[a.tesouro.portaY][a.tesouro.portaX] != PORTA_TRANCADA;
        bool temChave = j.mochila[I_CHAVE] > 0;
        bool viu = a.descoberto[a.tesouro.portaY][a.tesouro.portaX] || temChave ||
                   (borin >= 0 && a.descoberto[a.npcs[borin].y][a.npcs[borin].x]);
        if (viu || aberta)
            nova(MISSAO_TESOURO, "Sala do tesouro",
                 aberta ? "aberta" : borin >= 0 ? "o Borin tem a chave" : temChave ? "você tem a chave" : "um monstro tem a chave",
                 aberta, false);
    }

    // ---- Salas especiais (depois de descobertas) ----
    for (int i = 0; i < a.numEspeciais; i++) {
        const Especial& e = a.especiais[i];
        if (!a.descoberto[e.y][e.x]) continue;
        switch (e.tipo) {
            case ESP_FONTE: nova(MISSAO_FONTE, "Fonte", e.usado ? "usada" : "cura a vida", e.usado, false); break;
            case ESP_ALTAR: nova(MISSAO_ALTAR, "Altar de sangue", e.usado ? "pacto feito" : "vida por poder", e.usado, false); break;
            case ESP_BAU:   nova(MISSAO_BAU, "Baú dourado", e.usado ? "aberto" : "pode ser um mímico!", e.usado, false); break;
            case ESP_GONGO: nova(MISSAO_ARENA, "Arena", e.usado ? "vencida" : "bata o gongo: 3 ondas", e.usado, false); break;
            default: break;
        }
    }

    // ---- Pessoas (depois de vistas) ----
    for (int i = 0; i < a.numNpcs; i++) {
        const Npc& p = a.npcs[i];
        if (!a.descoberto[p.y][p.x]) continue;
        if (p.tipo == NPC_MINEIRO && a.tesouro.existe) continue;   // já está na missão do tesouro
        static const char* NOMES[NUM_TIPOS_DE_NPC] = { "Eldra", "Borin", "Aldric", "Garro", "Tobias" };
        std::string detalhe = p.tipo == NPC_COMERCIANTE ? "loja" : p.tipo == NPC_VIAJANTE ? "troca por um cristal" : "conversar";
        if (p.conversou) detalhe = p.tipo == NPC_VIAJANTE ? "ajudado" : p.tipo == NPC_COMERCIANTE ? "loja visitada" : "conversou";
        Missao* x = nova(MISSAO_PESSOA, NOMES[p.tipo], detalhe, p.conversou, false);
        x->npc = p.tipo;
    }
    return lista;
}

std::vector<Anotacao> anotacoesDoDiario(const Jogo& jogo) {
    std::vector<Anotacao> notas;
    bool garroAnotado = false;
    for (int n = 1; n <= NUM_ANDARES; n++) {
        const Andar& a = jogo.andares[n - 1];
        if (!a.gerado) continue;
        for (int i = 0; i < a.numNpcs; i++) {
            const Npc& p = a.npcs[i];
            if (!p.conversou) continue;
            std::vector<std::string> pontos;
            switch (p.tipo) {
                case NPC_EXPLORADORA:
                    pontos = { "Olhe o elemento antes de atacar", "Caixa empurrada deixa o monstro tonto",
                               "Procure o Borin no andar 4" };
                    break;
                case NPC_MINEIRO:
                    pontos = { "A mina era dele, antes dos goblins",
                               a.tesouro.existe ? "Abriu a sala do tesouro" : "Deu um Éter" };
                    break;
                case NPC_COMERCIANTE:
                    if (garroAnotado) continue;
                    garroAnotado = true;
                    pontos = { "Vende e compra itens", "Monstros e caixas têm moedas", "A loja volta mais abaixo" };
                    break;
                case NPC_ESPIRITO:
                    pontos = { "O chefe espera no andar 13", "Dele não se foge: só se vence", "Encheu a vida e os PP" };
                    break;
                case NPC_VIAJANTE:
                    pontos = { "Estava perdido e ferido", "Deu um cristal pela ajuda" };
                    break;
                default: continue;
            }
            notas.push_back({ n, FICHAS_DOS_NPCS[p.tipo].nome, pontos, false, -1, p.tipo });
        }
        int c = chefeDoAndar(a);
        if (c >= 0 && !a.monstros[c].vivo) {
            const TipoMonstro& t = tipoDe(a.monstros[c]);
            std::string titulo = (t.especial & CHEFE) ? "O fim da masmorra"
                                                      : tituloDoMiniChefe(salaDoMiniChefe(a.monstros[c].tipo));
            notas.push_back({ n, t.nome, { titulo }, true, a.monstros[c].tipo, -1 });
        }
    }
    return notas;
}
