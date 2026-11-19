// main.cpp (versão de terminal)
// -----------------------------------------------------------------------------
// Masmorra — roguelike inspirado no Brogue.
// Trabalho de Algoritmos e Programação II — UNIVALI.
//
// Menu principal -> criação do personagem -> partida em tempo real:
//   1. desenha a tela
//   2. espera uma tecla por no máximo 50 ms
//   3. executa a ação e faz o tempo do mapa andar (os monstros se mexem)
// -----------------------------------------------------------------------------
#include "terminal.h"
#include "tela.h"
#include "jogo.h"
#include "aleatorio.h"
#include "pontuacao.h"
#include "salvar.h"
#include "opcoes.h"
#include "dicas.h"

#include <cstdint>
#include <cstdlib>
#include <string>

// Espera até a janela ter pelo menos 80 x 24. Devolve false se o jogador
// preferir sair. É chamada antes de cada desenho, então também funciona se
// a janela for diminuída no meio da partida.
static bool garantirTamanhoDaJanela() {
    int colunas, linhas;
    while (tamanhoDoTerminal(colunas, linhas) &&
           (colunas < TELA_COLUNAS || linhas < TELA_LINHAS)) {
        desenharAviso(
            "Janela pequena: " + std::to_string(colunas) + " x " + std::to_string(linhas)
                + ". O jogo precisa de pelo menos 80 x 24.",
            "Aumente a janela (ou aperte Q para sair).");
        // Volta sozinho quando a janela for aumentada (TECLA_REDIMENSIONOU).
        if (lerTecla() == 'Q') return false;
    }
    return true;
}

// Lê a semente da linha de comando (ex.: "masmorra 1234").
// Sem número, sorteia uma semente pelo relógio.
static uint32_t lerSemente(int argc, char* argv[]) {
    if (argc >= 2) {
        char* fim = nullptr;
        unsigned long valor = std::strtoul(argv[1], &fim, 10);
        if (fim != argv[1] && *fim == '\0') return (uint32_t)valor;
    }
    return sementeDoRelogio();
}

enum OpcaoDoMenu { CONTINUAR, JOGAR, COMO_JOGAR, ITENS, PONTUACAO, OPCOES, SAIR };

static int menuPrincipal() {
    std::vector<std::string> opcoes = { "Novo jogo", "Como jogar", "Itens", "Pontuação e ranking", "Opções", "Sair" };
    std::vector<std::string> descricoes = {
        "Crie seu personagem e desça a masmorra.",
        "Objetivo, comandos, batalha e atributos.",
        "Poções, armas, escudos, cristais, pergaminhos e chave.",
        "Como os pontos são contados e as melhores partidas.",
        "Ligar ou desligar as dicas, ou ver todas de novo.",
        "Fechar o jogo.",
    };
    // Com partida salva, "Continuar" aparece em primeiro.
    std::string resumo;
    bool temSave = existePartidaSalva(&resumo);
    if (temSave) {
        opcoes.insert(opcoes.begin(), "Continuar");
        descricoes.insert(descricoes.begin(), "Partida salva: " + resumo);
        descricoes[1] += " (apaga a partida salva)";
    }
    int total = (int)opcoes.size();
    int cursor = 0;
    while (true) {
        desenharAviso("  MASMORRA  -  um roguelike nas profundezas de Valdoria", "");
        desenharMenuEmCaixa("MENU PRINCIPAL", opcoes, descricoes[cursor], cursor);
        int t = lerTecla();
        if (t == TECLA_CIMA || t == 'k') cursor = (cursor + total - 1) % total;
        if (t == TECLA_BAIXO || t == 'j') cursor = (cursor + 1) % total;
        if (t == TECLA_ENTER || t == ' ') return temSave ? cursor : cursor + 1;
        if (t == TECLA_ESC || t == 'Q') return SAIR;
    }
}

// Opções da versão de terminal: só as dicas (som, tela cheia e animações são
// da versão gráfica). Fica tudo em opcoes.txt.
static void menuDeOpcoes() {
    int cursor = 0;
    std::string aviso;
    while (true) {
        Opcoes& o = opcoes();
        std::vector<std::string> itens = {
            std::string("Dicas: ") + (o.mostrarDicas ? "ligadas" : "desligadas"),
            "Rever as dicas (" + std::to_string(dicasVistas()) + " de " + std::to_string((int)NUM_DICAS) + " já vistas)",
            "Voltar",
        };
        std::vector<std::string> descricoes = {
            "Uma caixa explicando cada novidade, na primeira vez que ela aparece.",
            "Todas as dicas voltam a aparecer.",
            "Volta ao menu principal.",
        };
        desenharAviso("  MASMORRA  -  um roguelike nas profundezas de Valdoria", "");
        desenharMenuEmCaixa("OPÇÕES", itens, aviso.empty() ? descricoes[cursor] : aviso, cursor);
        int t = lerTecla();
        if (t == TECLA_REDIMENSIONOU) continue;
        aviso.clear();
        if (t == TECLA_CIMA || t == 'k') cursor = (cursor + 2) % 3;
        if (t == TECLA_BAIXO || t == 'j') cursor = (cursor + 1) % 3;
        if (t == TECLA_ESC || t == 'x') return;
        if (t == TECLA_ENTER || t == ' ') {
            if (cursor == 0) { o.mostrarDicas = !o.mostrarDicas; salvarOpcoes(); }
            if (cursor == 1) { zerarDicas(); aviso = "Pronto! Todas as dicas vão aparecer de novo."; }
            if (cursor == 2) return;
        }
    }
}

// Uma partida inteira. Devolve false se o jogador quiser fechar o jogo.
static bool jogarAteSair(Jogo& jogo);

static bool jogarPartida(Jogo& jogo, uint32_t semente, bool continuar) {
    if (continuar) {
        if (!continuarPartida(jogo)) {
            desenharAviso("Não foi possível carregar a partida salva (arquivo de outra versão?).",
                          "Aperte qualquer tecla para voltar ao menu.");
            lerTecla();
            return true;
        }
    } else {
        if (!novaPartida(jogo, semente)) return true;   // desistiu na criação: volta ao menu
        apagarPartidaSalva();   // a partida nova substitui a salva
        salvarPartida(jogo);
    }
    bool continua = jogarAteSair(jogo);
    salvarPartida(jogo);   // saiu (Q): fica salva; se acabou, não salva nada
    return continua;
}

static bool jogarAteSair(Jogo& jogo) {
    long long ultimoInstante = relogioMs();
    while (jogo.rodando) {
        if (!garantirTamanhoDaJanela()) return false;
        desenharJogo(jogo);

        // Fim da partida (vitória ou derrota): pontuação e ranking.
        if (jogo.morto || jogo.venceu) {
            registrarNoRanking(jogo);
            desenharMorte(jogo);
            int tecla = lerTecla();
            if (tecla == 'n' || tecla == 'N') {
                if (!novaPartida(jogo, sementeDoRelogio())) return true;
            } else if (tecla == 'q' || tecla == 'Q' || tecla == TECLA_ESC) {
                return true;
            }
            ultimoInstante = relogioMs();
            continue;
        }

        int tecla = lerTeclaComEspera(50);
        if (tecla != TECLA_NENHUMA && tecla != TECLA_REDIMENSIONOU) {
            processarTecla(jogo, tecla);
        }

        // Quanto tempo passou desde a última volta (avancarTempo limita saltos
        // grandes, como depois de um menu ou de uma batalha).
        long long agora = relogioMs();
        avancarTempo(jogo, (int)(agora - ultimoInstante));
        ultimoInstante = relogioMs();
    }
    return true;
}

int main(int argc, char* argv[]) {
    uint32_t sementeInicial = lerSemente(argc, argv);
    carregarOpcoes();
    iniciarTerminal();

    // "static" porque o Jogo é grande (10 andares de 79 x 21 células) e fica
    // melhor fora da pilha.
    static Jogo jogo;
    bool primeira = true;
    bool continuar = true;
    while (continuar) {
        int opcao = menuPrincipal();
        if (opcao == JOGAR || opcao == CONTINUAR) {
            continuar = jogarPartida(jogo, primeira ? sementeInicial : sementeDoRelogio(), opcao == CONTINUAR);
            primeira = false;
        }
        if (opcao == COMO_JOGAR) mostrarPagina("COMO JOGAR", textoComoJogar());
        if (opcao == ITENS)      mostrarPagina("ITENS", textoDosItens());
        if (opcao == PONTUACAO)  mostrarPagina("PONTUAÇÃO", textoDaPontuacao());
        if (opcao == OPCOES)     menuDeOpcoes();
        if (opcao == SAIR)       continuar = false;
    }

    finalizarTerminal();
    return 0;
}
