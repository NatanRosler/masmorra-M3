// opcoes.h
// -----------------------------------------------------------------------------
// Opções do jogador (tela OPÇÕES do menu e da pausa), guardadas em opcoes.txt
// ao lado do jogo. O arquivo é texto simples, uma opção por linha
// ("musica=8"); se ele sumir ou estiver estragado, vale o padrão.
// -----------------------------------------------------------------------------
#pragma once

#include "dicas.h"

struct Opcoes {
    int volumeMusica;        // 0 a 10 (x 10%)
    int volumeEfeitos;       // 0 a 10 (x 10%)
    bool telaCheia;          // só na versão gráfica (F11 também troca)
    bool animacaoRapida;     // animações da batalha na metade do tempo
    bool mostrarDicas;
    bool dicaVista[NUM_DICAS];
};

// As opções atuais (começam no padrão até carregarOpcoes ser chamada).
Opcoes& opcoes();
void carregarOpcoes();
void salvarOpcoes();

// Nos testes automáticos: nada é gravado em opcoes.txt.
void naoGravarOpcoes();

// Quanto tempo as teclas ficam travadas numa mensagem importante da batalha
// (menor com as animações rápidas).
int tempoDeLeituraMs();
