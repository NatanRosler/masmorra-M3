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
    // v1.23
    GELO,
    RAIO,
    TREVAS,
    LUZ,
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
    PARALISADO,   // velocidade cai para 1/4 e, em 1/4 das vezes, não consegue agir
    // v1.23
    QUEIMADO,     // perde 1/16 da vida por rodada e o ataque cai para 3/4
    CONGELADO,    // não age por 1 ou 2 rodadas (um golpe de Fogo derrete o gelo na hora)
    OFUSCADO,     // a chance de acertar cai 20 pontos
    ATORDOADO,    // perde a próxima vez
    PRESO         // preso numa teia ou numa vinha: perde a próxima vez
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
    DOIS_GOLPES,     // acerta duas vezes
    // v1.23
    QUEIMAR,
    CONGELAR,
    OFUSCAR,
    ATORDOAR,
    PRENDER,
    CURAR,           // quem usou recupera "chanceDoEfeito" % da vida máxima
    SANGUESSUGA,     // planta uma semente: o alvo perde 1/8 da vida por rodada e você recupera
    FOCO,            // o próximo golpe de quem usou é crítico com certeza
    TRES_GOLPES      // acerta três vezes
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
    bool area;           // v1.23: acerta todos os inimigos de uma vez (só faz diferença para o jogador)
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
    // v1.23: a linha de golpes de cada elemento (sempre no FIM da lista: o
    // save guarda o número do golpe) e os golpes dos monstros novos
    G_FOCO,
    G_FURIA, G_GOLPE_FINAL,
    G_MURALHA_DE_FOGO, G_INFERNO,
    G_BOLHA_CURATIVA, G_ONDA, G_MAREMOTO,
    G_SEMENTE_SANGUESSUGA, G_TEMPESTADE_DE_FOLHAS,
    G_NUVEM_TOXICA, G_ACIDO_CORROSIVO, G_PESTE,
    G_MURALHA_DE_PEDRA, G_TERREMOTO, G_AVALANCHE,
    G_RAJADA_DE_GELO, G_ARMADURA_DE_GELO, G_NEVASCA, G_ZERO_ABSOLUTO,
    G_CHOQUE, G_PASSO_DO_RAIO, G_TROVAO, G_TEMPESTADE,
    G_GARRA_DAS_TREVAS, G_PAVOR, G_PULSO_DAS_TREVAS, G_ECLIPSE,
    G_RAIO_DE_LUZ, G_CURA_DE_LUZ, G_CLARAO, G_JULGAMENTO,
    G_SOPRO_FRIO, G_FAISCA, G_TOQUE_SOMBRIO, G_LAMPEJO, G_TEIA, G_ESPOROS,
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

// v1.23: golpes que o jogador aprende subindo de nível. Cada elemento tem a
// sua linha (níveis 12, 24 e 30) e todos aprendem os gerais (9, 18 e 27).
// Devolve quantos golpes ele aprende exatamente neste nível (até 2) em "golpes".
// v1.24.1: nos níveis da linha vem o próximo da sequência (ver abaixo), por
// isso precisa dos golpes que ele já sabe.
int golpesDoNivel(Elemento e, const int golpesQueSabe[], int numGolpes, int nivel, int golpes[2]);

// v1.24.1: a sequência de golpes de um elemento: o inicial e a linha (Planta
// tem o Pó do Sono depois do inicial). Devolve quantos (até 5).
int sequenciaDoElemento(Elemento e, int sequencia[5]);

// O golpe que vem depois do mais adiantado que o jogador sabe nessa sequência
// (-1 = já sabe todos). É o que o pergaminho do elemento ensina; subindo de
// nível (doPergaminho = false), o inicial e o Pó do Sono são pulados.
int proximoGolpeDaSequencia(Elemento e, const int golpes[], int numGolpes, bool doPergaminho);

// v1.23: resumo de um elemento para a tela de criação: golpe inicial,
// bônus, contra quem é forte, de quem leva o dobro e a que resiste.
// curto = só o bônus e contra quem é forte (a tela gráfica já mostra o resto).
std::string resumoDoElemento(Elemento e, bool curto = false);

// Bônus de quem usa um golpe do próprio elemento, em % (30 = +30%).
const int BONUS_DO_PROPRIO_ELEMENTO = 30;

// ---- Quem está lutando ---------------------------------------------------------
// Um Lutador é a "ficha de batalha" do jogador ou do monstro. A vida e os PP
// são PONTEIROS para os valores de verdade (guardados no Jogador ou no
// Monstro): assim, o que acontece na batalha já fica salvo neles.
const int MAX_INIMIGOS = 3;

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
    int rodadasDormindo;     // dormindo ou congelado: quantas rodadas ainda
    bool semente;            // v1.23: Semente Sanguessuga plantada neste lutador
    bool focado;             // v1.23: o próximo golpe é crítico (Foco)

    int golpes[MAX_GOLPES];  // índices na tabela GOLPES
    int numGolpes;
    int* pp;                 // aponta para os PP reais; nullptr = infinitos (monstros)
    int elementoDaArma;      // golpes deste elemento causam +25% (-1 = nenhum)
    // v1.24.1: golpes de monstro que foram trocados para o elemento dele (ver
    // golpesDoMonstro): não ganham o bônus do próprio elemento, para o dano
    // continuar o mesmo de antes (-1 = nenhum).
    int semBonusDoElemento[2] = { -1, -1 };
    // Atributos da Etapa 6 (em %): somam ou tiram da chance de acertar e de crítico.
    int precisaoExtra;       // Destreza: + chance de acertar
    int esquiva;             // Agilidade: - chance de o inimigo acertar
    int criticoExtra;        // Destreza: + chance de golpe crítico
    bool ehChefe;            // o chefe final: não dá para fugir dele
    bool ehMiniChefe;        // v1.24: mini-chefe (também não dá para fugir: o portão fechou)
    bool ehCampeao;          // monstro elite (aura dourada)
    bool ehJogador;
    int indice;              // 0 = jogador; 1, 2, 3 = inimigos (posição na tela)
};


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
    int ultimoElemento[MAX_INIMIGOS];  // v1.23: elemento do último dano em cada inimigo (esqueleto)
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
    long long passaSozinhoEm;    // > 0: a frase passa sozinha nessa hora (batalha automática);
                                 // -1: esperando o Enter; 0: animação (nada a apertar)
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
    bool naArena;                // desafio do gongo: a bomba de fumaça não funciona

    // v1.24: luta na sala de um chefe (fundo da sala; -1 = luta comum) e o
    // que cada mini-chefe já fez (reforços, esqueletos de pé de novo, fase 2).
    int salaDaBatalha;
    int reforcosChamados;
    int esqueletosLevantados;
    int levantaEm[MAX_INIMIGOS];  // esqueleto do Necromante: rodadas até levantar (0 = não)
    bool golemQuebrou;

    // Itens que podem ser usados na batalha (lista montada ao abrir ITENS).
    int itensUsaveis[16];
    int numItensUsaveis;
};
