// som.h
// -----------------------------------------------------------------------------
// Sons e músicas. A lógica do jogo só diz QUAL som tocar; quem toca é a
// versão gráfica (grafico/audio.cpp, com a raylib). A versão de terminal
// não tem som (console/sem_som.cpp não faz nada).
//
// Nenhum arquivo de áudio é usado: os sons são GERADOS no código, como nos
// videogames antigos (ondas quadradas, triangulares e ruído).
// -----------------------------------------------------------------------------
#pragma once

enum Som {
    SOM_MENU,            // mover o cursor / confirmar
    SOM_GOLPE,           // um golpe acertou
    SOM_CRITICO,
    SOM_SUPER_EFICAZ,
    SOM_ERROU,
    SOM_DANO,            // o jogador levou dano
    SOM_DESMAIO,         // um inimigo desmaiou
    SOM_NIVEL,           // subiu de nível
    SOM_ITEM,            // pegou um item
    SOM_CURA,            // bebeu uma poção / descansou
    SOM_PORTA,           // destrancou uma porta
    SOM_ESCADA,
    SOM_ARMADILHA,
    SOM_CAIXA,
    SOM_VITORIA,
    SOM_DERROTA,         // o jogador desmaiou
    SOM_BLOQUEIO,        // o escudo bloqueou um ataque
    NUM_SONS
};

enum Musica {
    SEM_MUSICA,
    MUSICA_MENU,
    MUSICA_MASMORRA,
    MUSICA_BATALHA,
    MUSICA_CHEFE,
    NUM_MUSICAS
};

void tocarSom(Som som);
void tocarMusica(Musica musica);   // troca a música (ou continua, se for a mesma)

// Tecla O: liga e desliga o som. Devolve true se o som ficou ligado.
bool alternarSom();
bool somLigado();
