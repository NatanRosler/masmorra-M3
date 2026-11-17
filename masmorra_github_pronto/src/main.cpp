// main.cpp
// -----------------------------------------------------------------------------
// Masmorra — roguelike de console inspirado no Brogue.
// Trabalho de Algoritmos e Programação — UNIVALI.
//
// O laço principal é simples porque o jogo é por TURNOS:
//   1. desenha a tela
//   2. espera o jogador apertar uma tecla
//   3. executa a ação (e, nas próximas etapas, os monstros agem)
// -----------------------------------------------------------------------------
#include "terminal.h"
#include "tela.h"
#include "jogo.h"
#include "aleatorio.h"
#include "menu_principal.h"

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

int main(int argc, char* argv[]) {
    uint32_t sementeInicial = lerSemente(argc, argv);
    iniciarTerminal();

    // O jogo original permanece inteiro. So adicionamos menu inicial e pausa.
    static Jogo jogo;
    bool primeiraPartida = true;
    bool sair = false;
    while (!sair && mostrarMenuPrincipal()) {
        iniciarJogo(jogo, primeiraPartida ? sementeInicial : sementeDoRelogio());
        primeiraPartida = false;
        long long ultimoInstante = relogioMs();

        while (jogo.rodando) {
            if (!garantirTamanhoDaJanela()) { sair = true; break; }
            desenharJogo(jogo);

            if (jogo.morto) {
                desenharMorte(jogo);
                int tecla = lerTecla();
                if (tecla == 'n' || tecla == 'N') {
                    iniciarJogo(jogo, sementeDoRelogio());
                } else if (tecla == 'q' || tecla == 'Q' || tecla == TECLA_ESC) {
                    jogo.rodando = false; // voltar ao menu grafico
                }
                ultimoInstante = relogioMs();
                continue;
            }

            int tecla = lerTeclaComEspera(50);
            if (tecla == TECLA_ESC) {
                int acao = mostrarMenuDePausa();
                if (acao == 1) jogo.rodando = false;
                if (acao == 2) { jogo.rodando = false; sair = true; }
                ultimoInstante = relogioMs(); // pausa nao gasta tempo no mapa
                continue;
            }
            if (tecla != TECLA_NENHUMA && tecla != TECLA_REDIMENSIONOU) {
                processarTecla(jogo, tecla); // funcao ORIGINAL
            }
            long long agora = relogioMs();
            avancarTempo(jogo, (int)(agora - ultimoInstante)); // IA ORIGINAL
            ultimoInstante = relogioMs();
        }
    }
    finalizarTerminal();
    return 0;
}
