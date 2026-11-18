// jogo.h
// -----------------------------------------------------------------------------
// Estado completo de uma partida e as regras de cada turno.
// -----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "mapa.h"
#include "gerador.h"
#include "visao.h"
#include "aleatorio.h"
#include "monstros.h"
#include "itens.h"
#include "cenario.h"
#include "npcs.h"
#include "dificuldade.h"

// Quantos andares a masmorra tem. O chefe final ficará no último.
const int NUM_ANDARES = 10;

// Atributos-base do jogador (como os de um monstro): os valores de verdade
// crescem com o nível. Ver recalcularAtributos().
const int BASE_VIDA_JOGADOR       = 70;
const int BASE_ATAQUE_JOGADOR     = 50;
const int BASE_DEFESA_JOGADOR     = 65;
const int BASE_VELOCIDADE_JOGADOR = 65;
const int NIVEL_INICIAL = 5;
const int NIVEL_MAXIMO = 50;

// ---- Mensagens importantes ----------------------------------------------------
// Mensagens BOAS (dourado) e de PERIGO (vermelho) ganham destaque: no mapa
// aparecem numa faixa no topo; na batalha, as teclas ficam travadas por
// TEMPO_DE_LEITURA_MS, para o jogador não pular a mensagem sem ler.
enum TipoMensagem { MSG_NORMAL, MSG_BOA, MSG_PERIGO };
const int TEMPO_DE_LEITURA_MS = 900;
const int TEMPO_DA_FAIXA_MS = 3000;

// ---- Atributos que o jogador distribui (Etapa 6) ----------------------------
enum Atributo {
    FORCA,          // +1 de ataque por ponto: mais dano
    CONSTITUICAO,   // +3 de vida máxima e +1 de defesa (armadura) por ponto
    DESTREZA,       // +3% de precisão e +1% de chance de crítico por ponto
    AGILIDADE,      // +2% de esquiva e +1 de velocidade (andar mais rápido) por ponto
    NUM_ATRIBUTOS
};
extern const char* NOMES_DOS_ATRIBUTOS[NUM_ATRIBUTOS];
extern const char* EFEITOS_DOS_ATRIBUTOS[NUM_ATRIBUTOS];

const int PONTOS_INICIAIS = 5;    // na criação do personagem
const int PONTOS_POR_NIVEL = 1;   // a cada nível novo
const int TAMANHO_MAXIMO_DO_NOME = 12;

// Cores que o jogador pode escolher para o personagem.
const int NUM_CORES_DO_JOGADOR = 6;
extern const char* NOMES_DAS_CORES_DO_JOGADOR[NUM_CORES_DO_JOGADOR];
extern const CorRGB CORES_DO_JOGADOR[NUM_CORES_DO_JOGADOR];

struct Jogador {
    // Criação do personagem (Etapa 6)
    std::string nome;
    Elemento elemento;
    int cor;                   // índice em CORES_DO_JOGADOR
    int atributos[NUM_ATRIBUTOS];
    int pontosLivres;          // ganhos ao subir de nível, ainda não distribuídos
    // Efeitos dos atributos em % (calculados por recalcularAtributos)
    int precisaoExtra, esquiva, criticoExtra;

    int x, y;
    int nivel;
    int experiencia;           // total acumulado
    int vida, vidaMaxima;
    int ataque, defesa, velocidade;

    int golpes[MAX_GOLPES];    // índices na tabela GOLPES (batalha.h)
    int pp[MAX_GOLPES];        // usos restantes de cada golpe
    int numGolpes;

    // Mochila: quantos itens de cada tipo o jogador carrega.
    int mochila[NUM_TIPOS_DE_ITEM];
    // Equipamento: PONTEIROS para a ficha do item na tabela TIPOS_DE_ITEM
    // (nullptr = mão vazia). Assim, o bônus vem direto da ficha.
    const TipoItem* arma;
    const TipoItem* escudo;
    // Bônus permanentes dos cristais (power-ups).
    int bonusVida, bonusAtaque, bonusDefesa, bonusVelocidade;

    int vidas;                 // chances que ainda restam (começa com 3)
};

// Cada andar é gerado na primeira visita e guardado, para que o jogador
// encontre o mesmo andar (e os mesmos monstros) se subir de volta.
struct Andar {
    bool gerado;
    Mapa mapa;
    Escadas escadas;
    // Memória do jogador: células que ele já viu alguma vez neste andar.
    bool descoberto[MAPA_ALTURA][MAPA_LARGURA];

    Monstro monstros[MAX_MONSTROS];
    int numMonstros;

    ItemNoChao itens[MAX_ITENS_NO_CHAO];
    int numItens;
    SalaDoTesouro tesouro;     // sala trancada (se este andar tiver uma)

    Armadilha armadilhas[MAX_ARMADILHAS];   // Etapa 7
    // Item escondido em cada caixa (-1 = vazia). Acompanha a caixa quando
    // ela é empurrada e cai no chão quando ela quebra.
    int conteudoDasCaixas[MAPA_ALTURA][MAPA_LARGURA];
    int numArmadilhas;
    Npc npcs[MAX_NPCS];                     // Etapa 8
    int numNpcs;

    // Rastro de cheiro que o jogador deixa por onde passa. Quanto maior o
    // número, mais recente o cheiro. Monstros que perderam o jogador de
    // vista seguem o cheiro mais forte.
    int cheiro[MAPA_ALTURA][MAPA_LARGURA];
};

struct Jogo {
    uint32_t semente;               // semente da partida (mostrada na ajuda)
    GeradorAleatorio sorte;         // sorteios do combate
    bool comMonstros;               // false só nos testes automáticos
    bool modoAutomatico;            // true só nos testes: batalhas sem tela
    int batalhaPendente;            // monstro que vai lutar (-1 = nenhum)
    Andar andares[NUM_ANDARES];     // andares[0] é o andar 1
    int andar;                      // andar atual (1 = primeiro)

    Jogador jogador;

    // Recalculados a cada turno por atualizarVisao():
    bool linhaDeVisao[MAPA_ALTURA][MAPA_LARGURA]; // em linha reta do jogador (com ou sem luz)
    bool visivel[MAPA_ALTURA][MAPA_LARGURA];      // o que o jogador vê agora (linha + luz)
    Luz luz[MAPA_ALTURA][MAPA_LARGURA];           // quanta luz chega em cada célula
    int distanciaAteJogador[MAPA_ALTURA][MAPA_LARGURA]; // BFS a partir do jogador

    // Tempo real.
    long long tempoMs;              // tempo de jogo no mapa (não conta menus e batalhas)
    int esperaDoJogadorMs;          // quanto falta para o jogador poder dar outro passo
    bool descansando;               // tecla Z: recupera vida (e faz menos barulho)
    int msDeDescanso;
    int segundosDeDescanso;

    Elemento elementoDaEspecie[MAX_ESPECIES]; // sorteado a cada partida (Etapa 6)

    int passos;                     // quantos passos o jogador já deu
    int monstrosMortos;
    int itensColetados;
    int andarMaisFundo;             // para a pontuação
    int rodadasDaUltimaBatalha;     // estatística (usada nos testes de equilíbrio)

    std::vector<std::string> mensagensDoTurno; // aparecem no topo da tela
    std::vector<std::string> historico;        // últimas mensagens (tecla M)
    std::vector<int> tiposDoHistorico;         // TipoMensagem de cada uma
    int tipoDoTurno;                // a mensagem mais importante do turno
    std::string faixa;              // destaque no topo do mapa
    int tipoDaFaixa;
    long long faixaAte;             // relógio (ms) em que a faixa some

    bool morto;                     // as vidas acabaram: fim de jogo
    std::string causaDaMorte;       // "derrotado por um ogro"
    bool venceu;                    // derrotou o chefe final
    bool rodando;                   // false = sair do jogo

    // Fim da partida (Etapa 9): preenchidos por registrarNoRanking().
    bool rankingRegistrado;
    int pontuacaoFinal;
    int posicaoNoRanking;           // 0 = primeiro; -1 = ficou fora do top 10
};

// Escolhas da tela de criação do personagem (Etapa 6).
struct Criacao {
    std::string nome;
    int elemento;                   // Elemento
    int cor;                        // índice em CORES_DO_JOGADOR
    int dificuldade;                // NivelDeDificuldade
    bool nomeSorteado;              // o nome veio do botão Aleatório
    int atributos[NUM_ATRIBUTOS];
    int pontosLivres;
    int linha;                      // linha selecionada na tela
};

// Linhas da tela de criação.
enum LinhaDaCriacao {
    LINHA_NOME, LINHA_ELEMENTO, LINHA_COR, LINHA_DIFICULDADE,
    LINHA_FORCA, LINHA_CONSTITUICAO, LINHA_DESTREZA, LINHA_AGILIDADE,
    LINHA_ALEATORIO, LINHA_COMECAR,
    NUM_LINHAS_DA_CRIACAO
};

// Valores iniciais da criação (usados também pelos testes automáticos).
void criacaoPadrao(Criacao& c);

// Mostra a tela de criação. Devolve false se o jogador apertou Esc.
bool criarPersonagem(Criacao& c);

// Aplica as escolhas da criação no jogador (chamar logo depois de iniciarJogo).
void aplicarCriacao(Jogo& jogo, const Criacao& c);

// Começa uma partida de verdade: criação do personagem, masmorra nova e a
// história. Devolve false se o jogador desistiu (Esc) na criação.
bool novaPartida(Jogo& jogo, uint32_t semente);

// Gasta "quantos" pontos de atributo; nos testes automáticos são
// distribuídos sozinhos.
void ganharPontosDeAtributo(Jogo& jogo, int quantos);

// Tecla C: ficha do personagem, onde se distribuem os pontos livres.
void abrirFicha(Jogo& jogo);

// O jogador desmaiou (batalha perdida ou armadilha): gasta uma vida e volta
// para a entrada do andar. Sem vidas, é o fim do jogo.
void desmaiar(Jogo& jogo, const std::string& causa);

// comMonstros = false serve só para os testes automáticos.
void iniciarJogo(Jogo& jogo, uint32_t semente, bool comMonstros = true);

// Mapa e andar onde o jogador está.
Mapa& mapaAtual(Jogo& jogo);
const Mapa& mapaAtual(const Jogo& jogo);
Andar& andarAtual(Jogo& jogo);
const Andar& andarAtual(const Jogo& jogo);

// Recalcula luz e visão a partir da posição do jogador e atualiza a memória.
void atualizarVisao(Jogo& jogo);

// Alcance da luz que o jogador carrega (diminui nos andares mais fundos).
int raioDaLuzDoJogador(int andar);

// Mostra uma mensagem no topo da tela (e guarda no histórico).
void adicionarMensagem(Jogo& jogo, const std::string& texto, TipoMensagem tipo = MSG_NORMAL);

// Recalcula ataque, defesa, velocidade e vida máxima a partir do nível,
// somando os bônus da arma, do escudo e dos cristais.
void recalcularAtributos(Jogador& jogador);

// Mostra uma caixa com opções e deixa o jogador escolher uma (setas + Enter).
// "descricoes" (pode ficar vazio) aparece embaixo da opção selecionada.
// Devolve o índice escolhido ou -1 se ele apertar Esc.
int escolherEmCaixa(const Jogo& jogo, const std::string& titulo,
                    const std::vector<std::string>& opcoes,
                    const std::vector<std::string>& descricoes);

// Recebe a tecla apertada e executa a ação correspondente.
void processarTecla(Jogo& jogo, int tecla);

// Faz o tempo do mapa andar "ms" milissegundos: monstros agem, o descanso
// recupera vida, batalhas começam. Chamada pelo laço principal o tempo todo.
void avancarTempo(Jogo& jogo, int ms);

// Intervalo mínimo entre dois passos do jogador (depende da velocidade).
int intervaloDoJogador(const Jogador& j);
