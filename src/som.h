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
    // v1.21: efeitos com "instrumentos" (grafico/sintese.cpp)
    SOM_MOEDAS,          // ganhou moedas (sino)
    SOM_CRISTAL,         // absorveu um cristal (harpa subindo)
    SOM_EQUIPAR_ARMA,    // "shing" metálico
    SOM_EQUIPAR_ESCUDO,  // "tonc" de metal
    SOM_ETER,            // bolhas e sino (PP de volta)
    SOM_ANTIDOTO,
    SOM_FUMACA,          // bomba de fumaça: estouro e chiado
    SOM_PERGAMINHO,      // papel desenrolando e harpa
    SOM_ITEM_ESPECIAL,   // Mochila de Couro, Lanterna
    SOM_LARGAR,          // um item cai no chão
    SOM_VENENO,          // bolhas (envenenou / dano do veneno)
    SOM_SONO,            // harpa descendo
    SOM_PARALISIA,       // zumbido elétrico
    SOM_FUGA,            // passos correndo
    SOM_FUGA_FALHOU,     // passos e um tropeço
    SOM_FURTIVO,         // "shh": entrou ou saiu do modo furtivo
    SOM_AGUA,            // entrou numa poça
    SOM_LAVA,            // a lava queima
    SOM_PONTO,           // distribuiu um ponto na ficha
    SOM_DICA,            // apareceu uma dica
    SOM_MIMICO,          // o baú era um mímico!
    SOM_ALTAR,           // pacto no altar de sangue
    SOM_GONGO,           // desafio da arena aceito
    // v1.22
    SOM_ESPINHOS,        // armadilha de espinhos: lâminas de metal saltando e uma pancada
    SOM_PONTO_GANHO,     // ganhou um ponto de atributo (subiu de nível)
    // v1.23: estados novos da batalha
    SOM_QUEIMADO,        // fogo pegando ("fwoosh") e estalos
    SOM_CONGELADO,       // cristais de gelo se formando
    SOM_DEGELO,          // gelo rachando e pingando
    SOM_OFUSCADO,        // brilho agudo de sinos
    SOM_ATORDOADO,       // pancada e passarinhos (atordoado ou preso)
    SOM_FOLHAS,          // folhas farfalhando (Semente Sanguessuga)
    SOM_REMONTA,         // ossos se juntando (o esqueleto se monta de novo)
    NUM_SONS
};

// Vozes: cada letra das falas toca um "bip" na voz de quem fala. Cada voz tem
// o seu tom, a sua onda e o seu jeito (eco, tremido, rouco...).
enum Voz {
    VOZ_NARRADOR, VOZ_HEROI, VOZ_ELDRA, VOZ_BORIN, VOZ_ALDRIC, VOZ_GARRO, VOZ_TOBIAS,
    VOZ_RATO, VOZ_KOBOLD, VOZ_CHACAL, VOZ_MACACO, VOZ_SAPO, VOZ_GOBLIN, VOZ_MORCEGO,
    VOZ_OGRO, VOZ_NAGA, VOZ_GNOMO, VOZ_SOMBRA, VOZ_MIMICO, VOZ_CHEFE,
    VOZ_ARANHA, VOZ_FUNGO, VOZ_ESQUELETO, VOZ_SALAMANDRA,   // v1.23
    NUM_VOZES
};

enum Musica {
    SEM_MUSICA,
    MUSICA_MENU,
    MUSICA_MASMORRA,
    MUSICA_BATALHA,
    MUSICA_CHEFE,
    // v1.21: as músicas das cenas
    MUSICA_HISTORIA,     // a história do começo
    MUSICA_ELDRA, MUSICA_BORIN, MUSICA_ALDRIC, MUSICA_GARRO, MUSICA_TOBIAS,   // conversas (e a loja)
    MUSICA_CHEFE_CENA,   // o coração batendo na cena do chefe
    MUSICA_FINAL,
    NUM_MUSICAS
};

// Som de ambiente de cada tema de andar (toca em loop por baixo da música).
enum Ambiente { SEM_AMBIENTE, AMBIENTE_MINAS, AMBIENTE_CAVERNA, AMBIENTE_VULCAO, AMBIENTE_TRONO,
                AMBIENTE_CRIPTA,   // v1.23
                NUM_AMBIENTES };

// Passos: do herói (conforme o chão) e de cada espécie de monstro.
enum Passo {
    SEM_PASSO = -1,
    PASSO_PEDRA, PASSO_AGUA, PASSO_TAPETE, PASSO_FURTIVO,
    PASSO_RATO, PASSO_KOBOLD, PASSO_CHACAL, PASSO_MACACO, PASSO_SAPO, PASSO_GOBLIN, PASSO_MORCEGO,
    PASSO_OGRO, PASSO_NAGA, PASSO_GNOMO, PASSO_MIMICO, PASSO_CHEFE,
    PASSO_ARANHA, PASSO_FUNGO, PASSO_ESQUELETO, PASSO_SALAMANDRA,   // v1.23
    NUM_PASSOS
};

// Passo de cada espécie (pelo símbolo do mapa). A sombra não faz barulho.
inline Passo passoDoMonstro(char simbolo) {
    switch (simbolo) {
        case 'r': return PASSO_RATO;    case 'k': return PASSO_KOBOLD;  case 'j': return PASSO_CHACAL;
        case 'm': return PASSO_MACACO;  case 't': return PASSO_SAPO;    case 'g': return PASSO_GOBLIN;
        case 'v': return PASSO_MORCEGO; case 'O': return PASSO_OGRO;    case 'N': return PASSO_NAGA;
        case 'G': return PASSO_GNOMO;   case 'M': return PASSO_MIMICO;  case 'S': return PASSO_CHEFE;
        case 'a': return PASSO_ARANHA;  case 'f': return PASSO_FUNGO;   case 'e': return PASSO_ESQUELETO;
        case 's': return PASSO_SALAMANDRA;
    }
    return SEM_PASSO;
}

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
        case 'a': return VOZ_ARANHA;  case 'f': return VOZ_FUNGO;   case 'e': return VOZ_ESQUELETO;
        case 's': return VOZ_SALAMANDRA;
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
void tocarMusica(Musica musica);   // troca a música, com fade (ou continua, se for a mesma)

// Ambiente do andar (troca com fade; SEM_AMBIENTE desliga) e o som das salas
// especiais por perto (0 = longe/sem som, 1 = do lado).
void tocarAmbiente(Ambiente ambiente);
void volumeDasSalas(float fonte, float altar);
// Um passo, com volume de 0 a 1 (a distância). Os passos usam o volume de
// Ambiente das Opções; muitos ao mesmo tempo ficam um pouco mais baixos.
void tocarPasso(Passo passo, float volume);

// Tecla O: liga e desliga o som. Devolve true se o som ficou ligado.
bool alternarSom();
bool somLigado();

// Aplica os volumes de música, efeitos e ambiente escolhidos em Opções (opcoes.h).
void aplicarVolumes();
