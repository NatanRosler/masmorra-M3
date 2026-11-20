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
    SOM_CORNETA,         // o gnomo vigia sopra a corneta
    SOM_PORTA_FECHANDO,  // as portas da entrada se fecham (história)
    SOM_RISADA_GNOMO,    // "hihihi!" depois da corneta
    SOM_RUGIDO,          // o Senhor das Profundezas ruge
    SOM_TOCHA_APAGA,     // "fff": uma tocha se apaga (cena do chefe)
    SOM_RACHADURA,       // a tela racha antes da batalha com o chefe
    NUM_SONS
};

// Vozes: cada letra das falas toca um "bip" na voz de quem fala. Cada voz tem
// o seu tom, a sua onda e o seu jeito (eco, tremido, rouco...).
enum Voz {
    VOZ_NARRADOR, VOZ_HEROI, VOZ_ELDRA, VOZ_BORIN, VOZ_ALDRIC, VOZ_GARRO, VOZ_TOBIAS,
    VOZ_RATO, VOZ_KOBOLD, VOZ_CHACAL, VOZ_MACACO, VOZ_SAPO, VOZ_GOBLIN, VOZ_MORCEGO,
    VOZ_OGRO, VOZ_NAGA, VOZ_GNOMO, VOZ_SOMBRA, VOZ_MIMICO, VOZ_CHEFE,
    NUM_VOZES
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
// Igual, mas mais agudo (tom > 1) ou mais grave (tom < 1): a "voz" de cada
// personagem nas falas.
void tocarSomComTom(Som som, float tom);

// Toca a voz para a letra de número "letra" (contando só as que fazem som).
// Cada voz decide sozinha se toca nesta letra (umas a cada 2, outras a cada 3).
void tocarVoz(Voz voz, int letra);

// Voz de cada espécie de monstro (pelo símbolo do mapa: 'r', 'g', 'O'...).
inline Voz vozDoMonstro(char simbolo) {
    switch (simbolo) {
        case 'r': return VOZ_RATO;    case 'k': return VOZ_KOBOLD;  case 'j': return VOZ_CHACAL;
        case 'm': return VOZ_MACACO;  case 't': return VOZ_SAPO;    case 'g': return VOZ_GOBLIN;
        case 'v': return VOZ_MORCEGO; case 'O': return VOZ_OGRO;    case 'N': return VOZ_NAGA;
        case 'G': return VOZ_GNOMO;   case 'W': return VOZ_SOMBRA;  case 'M': return VOZ_MIMICO;
        case 'S': return VOZ_CHEFE;
    }
    return VOZ_NARRADOR;
}

// Velocidade de fala da voz (1 = normal; o Borin fala devagar, o Garro rápido,
// o chefe bem devagar).
inline float ritmoDaVoz(Voz voz) {
    switch (voz) {
        case VOZ_NARRADOR: return 0.8f;  case VOZ_ELDRA: return 1.05f; case VOZ_BORIN: return 0.85f;
        case VOZ_ALDRIC:   return 0.8f;  case VOZ_GARRO: return 1.15f; case VOZ_TOBIAS: return 0.95f;
        case VOZ_SAPO:     return 0.55f; case VOZ_OGRO:  return 0.6f;  case VOZ_SOMBRA: return 0.6f;
        case VOZ_CHEFE:    return 0.38f;
        default: return 1.0f;
    }
}
void tocarMusica(Musica musica);   // troca a música (ou continua, se for a mesma)

// Tecla O: liga e desliga o som. Devolve true se o som ficou ligado.
bool alternarSom();
bool somLigado();

// Aplica os volumes de música e de efeitos escolhidos em Opções (opcoes.h).
void aplicarVolumes();
