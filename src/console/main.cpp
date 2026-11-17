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

enum OpcaoDoMenu { JOGAR, COMO_JOGAR, ITENS, PONTUACAO, SAIR };

static int menuPrincipal() {
    const std::vector<std::string> opcoes = { "Jogar", "Como jogar", "Itens", "Pontuação e ranking", "Sair" };
    const std::vector<std::string> descricoes = {
        "Crie seu personagem e desça a masmorra.",
        "Objetivo, comandos, batalha e atributos.",
        "Poções, armas, escudos, cristais, pergaminhos e chave.",
        "Como os pontos são contados e as melhores partidas.",
        "Fechar o jogo.",
    };
    int cursor = 0;
    while (true) {
        desenharAviso("  MASMORRA  -  um roguelike nas profundezas de Valdoria", "");
        desenharMenuEmCaixa("MENU PRINCIPAL", opcoes, descricoes[cursor], cursor);
        int t = lerTecla();
        if (t == TECLA_CIMA || t == 'k') cursor = (cursor + 4) % 5;
        if (t == TECLA_BAIXO || t == 'j') cursor = (cursor + 1) % 5;
        if (t == TECLA_ENTER || t == ' ') return cursor;
        if (t == TECLA_ESC || t == 'Q') return SAIR;
    }
}

// Uma partida inteira. Devolve false se o jogador quiser fechar o jogo.
static bool jogarPartida(Jogo& jogo, uint32_t semente) {
    if (!novaPartida(jogo, semente)) return true;   // desistiu na criação: volta ao menu

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
    iniciarTerminal();

    // "static" porque o Jogo é grande (10 andares de 79 x 21 células) e fica
    // melhor fora da pilha.
    static Jogo jogo;
    bool primeira = true;
    bool continuar = true;
    while (continuar) {
        int opcao = menuPrincipal();
        if (opcao == JOGAR) {
            continuar = jogarPartida(jogo, primeira ? sementeInicial : sementeDoRelogio());
            primeira = false;
        }
        if (opcao == COMO_JOGAR) mostrarPagina("COMO JOGAR", textoComoJogar());
        if (opcao == ITENS)      mostrarPagina("ITENS", textoDosItens());
        if (opcao == PONTUACAO)  mostrarPagina("PONTUAÇÃO", textoDaPontuacao());
        if (opcao == SAIR)       continuar = false;
    }

    finalizarTerminal();
    return 0;
}
