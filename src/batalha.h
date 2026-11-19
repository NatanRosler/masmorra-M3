// batalha.h
// -----------------------------------------------------------------------------
// Batalha em turnos, no estilo dos Pokémon clássicos.
//
// Quando o jogador encosta num monstro (ou o monstro alcança o jogador), o
// jogo sai do mapa e abre a tela de batalha. Outros monstros acordados que
// estejam perto entram junto (no máximo 3 inimigos). A cada rodada:
//   1. o jogador escolhe: LUTAR (um golpe e o alvo), ITENS, STATUS ou FUGIR;
//   2. cada monstro escolhe um golpe;
//   3. todos agem em ordem: primeiro os golpes com prioridade, depois os
//      mais rápidos;
//   4. no fim da rodada, quem está envenenado perde vida.
// A batalha termina quando o jogador desmaia, foge, ou não sobra inimigo.
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

struct Jogo;
struct Monstro;

// ---- Elementos ---------------------------------------------------------------
enum Elemento {
    NORMAL,
    FOGO,
    AGUA,
    PLANTA,
    VENENO,
    PEDRA,
    NUM_ELEMENTOS
};

extern const char* NOMES_DOS_ELEMENTOS[NUM_ELEMENTOS];

// Multiplicador do dano de um golpe do elemento "ataque" contra um lutador do
// elemento "defesa", em METADES: 1 = metade do dano, 2 = normal, 4 = dobro.
int eficacia(Elemento ataque, Elemento defesa);

// ---- Status ------------------------------------------------------------------
enum Status {
    SEM_STATUS,
    ENVENENADO,   // perde 1/8 da vida máxima no fim de cada rodada
    ADORMECIDO,   // não age por 1 a 3 rodadas
    PARALISADO    // velocidade cai para 1/4 e, em 1/4 das vezes, não consegue agir
};

extern const char* NOMES_DOS_STATUS[];

// ---- Golpes ------------------------------------------------------------------
enum EfeitoGolpe {
    SEM_EFEITO,
    ENVENENAR,
    ADORMECER,
    PARALISAR,
    SOBE_ATAQUE,     // aumenta o ataque de quem usou
    SOBE_DEFESA,     // aumenta a defesa de quem usou
    BAIXA_ATAQUE,    // diminui o ataque do alvo
    BAIXA_DEFESA,    // diminui a defesa do alvo
    DRENAR,          // quem usou recupera metade do dano causado
    DOIS_GOLPES      // acerta duas vezes
};

struct Golpe {
    const char* nome;
    Elemento elemento;
    int poder;           // 0 = golpe de status (não causa dano)
    int precisao;        // chance de acertar, em %
    int ppMaximo;        // quantas vezes pode ser usado (0 = infinito)
    EfeitoGolpe efeito;
    int chanceDoEfeito;  // em %, para golpes que causam dano e às vezes um efeito
    int prioridade;      // 1 = age antes mesmo de quem é mais rápido
};

// Os golpes do jogo. Os monstros e o jogador guardam o ÍNDICE do golpe nesta tabela.
enum IdGolpe {
    G_GOLPE, G_ESCUDO, G_GOLPE_FORTE, G_TOCHA, G_JATO_DAGUA, G_GRITO_DE_GUERRA,
    G_PO_DO_SONO, G_LAMINA_VENENOSA, G_GOLPE_DUPLO, G_PEDRADA,
    G_MORDIDA, G_ROSNADO, G_ATAQUE_RAPIDO, G_ARRANHAO, G_FOLHA_NAVALHA,
    G_LINGUA_GRUDENTA, G_FERRAO_VENENOSO, G_SUGAR_SANGUE, G_ESMAGAR, G_RUGIDO,
    G_CHICOTE_DAGUA, G_HIPNOSE, G_BRASA,
    // Etapa 6: golpe inicial do elemento Planta e golpes elementais dos monstros
    G_CHICOTE_DE_VINHA, G_INVESTIDA, G_LABAREDA, G_BOLHA, G_ESPINHOS, G_ACIDO,
    G_PEDREGULHO, G_GOLPE_SOMBRIO,
    NUM_GOLPES
};

extern const Golpe GOLPES[NUM_GOLPES];

const int MAX_GOLPES = 4;   // como no Pokémon: no máximo 4 golpes por vez

// ---- Atributos que crescem com o nível --------------------------------------
// Fórmulas no estilo dos Pokémon: o atributo cresce em linha reta com o nível,
// mais rápido para quem tem o atributo-base maior.
int vidaPeloNivel(int base, int nivel);
int atributoPeloNivel(int base, int nivel);

// Experiência total necessária para estar no nível n (4/5 de n ao cubo).
int experienciaParaNivel(int nivel);

// Golpe inicial do jogador de cada elemento (Etapa 6: criação do personagem).
int golpeInicialDoElemento(Elemento e);

// Golpe elemental que um monstro ganha conforme o elemento sorteado.
// "forte" = para as espécies dos andares mais fundos.
int golpeElementalDosMonstros(Elemento e, bool forte);

// ---- Quem está lutando ---------------------------------------------------------
// Um Lutador é a "ficha de batalha" do jogador ou do monstro. A vida e os PP
// são PONTEIROS para os valores de verdade (guardados no Jogador ou no
// Monstro): assim, o que acontece na batalha já fica salvo neles.
struct Lutador {
    std::string nome;
    int nivel;
    Elemento elemento;
    int* vida;               // aponta para a vida real
    int vidaMaxima;
    int ataque, defesa, velocidade;

    // Mudanças temporárias (só valem durante a batalha), de -6 a +6.
    int estagioAtaque, estagioDefesa;

    Status status;
    int rodadasDormindo;

    int golpes[MAX_GOLPES];  // índices na tabela GOLPES
    int numGolpes;
    int* pp;                 // aponta para os PP reais; nullptr = infinitos (monstros)
    int elementoDaArma;      // golpes deste elemento causam +25% (-1 = nenhum)
    // Atributos da Etapa 6 (em %): somam ou tiram da chance de acertar e de crítico.
    int precisaoExtra;       // Destreza: + chance de acertar
    int esquiva;             // Agilidade: - chance de o inimigo acertar
    int criticoExtra;        // Destreza: + chance de golpe crítico
    bool ehChefe;            // o chefe final: não dá para fugir dele
    bool ehJogador;
    int indice;              // 0 = jogador; 1, 2, 3 = inimigos (posição na tela)
};

const int MAX_INIMIGOS = 3;

enum ResultadoBatalha {
    VITORIA,         // não sobrou inimigo (desmaiaram ou fugiram)
    DERROTA,         // o jogador desmaiou
    JOGADOR_FUGIU
};

// Abre a batalha contra 1 a 3 monstros e só volta quando ela termina.
// Monstros derrotados ficam com vivo = false; os que fogem, com estado FUGINDO.
ResultadoBatalha batalhar(Jogo& jogo, Monstro* participantes[], int quantos);

// ---- Usado pela tela ------------------------------------------------------------
// Qual menu está aberto na caixa de texto.
enum MenuBatalha {
    MENU_NENHUM,     // só um texto (aperte qualquer tecla)
    MENU_PRINCIPAL,  // LUTAR / ITENS / STATUS / FUGIR
    MENU_GOLPES,     // lista de golpes
    MENU_ITENS,      // mochila
    MENU_STATUS,     // ficha dos lutadores
    MENU_ESQUECER,   // escolher golpe para esquecer
    MENU_ALVO        // escolher qual inimigo atacar
};

struct Batalha {
    Lutador jogador;
    Lutador inimigos[MAX_INIMIGOS];
    Monstro* monstros[MAX_INIMIGOS];   // os monstros do mapa que estão lutando
    bool naLuta[MAX_INIMIGOS];         // false = desmaiou ou fugiu
    int numInimigos;
    int vidaMostrada[1 + MAX_INIMIGOS]; // [0] jogador, [1..3] inimigos: anima as barras
    int tentativasDeFuga;

    // Menu de alvo: quais inimigos podem ser escolhidos (índices em "inimigos").
    int alvos[MAX_INIMIGOS];
    int numAlvos;

    // O que a caixa de texto está mostrando.
    std::string texto;
    int tipoDoTexto;             // TipoMensagem: normal, boa ou perigo
    long long textoDesde;        // quando o texto apareceu (para piscar a borda)
    bool travado;                // mensagem importante: as teclas ainda não valem
    bool avisouVidaBaixa;        // o aviso de vida baixa só aparece uma vez
    int menu;                    // qual menu está aberto (MenuBatalha)
    int cursor;                  // opção selecionada
    int golpeNovo;               // golpe sendo aprendido (MENU_ESQUECER)

    // Golpe Escudo: o jogador ergue o escudo e pode bloquear os ataques da rodada.
    bool escudoErguido;          // erguido nesta rodada
    bool escudoNaRodadaAnterior; // duas rodadas seguidas: a chance cai pela metade
    int chanceDoBloqueio;        // chance (%) enquanto está erguido

    // Segunda fase do chefe: com metade da vida ele fica furioso.
    bool chefeFurioso;

    // Itens que podem ser usados na batalha (lista montada ao abrir ITENS).
    int itensUsaveis[16];
    int numItensUsaveis;
};
