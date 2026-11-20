// tela.h
// -----------------------------------------------------------------------------
// Desenho da tela. Layout (80 colunas x 24 linhas, o tamanho padrão dos
// terminais):
//
//   linha 1       : mensagem
//   linhas 2..22  : câmera — um pedaço do mapa (39 x 21 células) em volta do
//                   jogador, com zoom 2x (cada célula ocupa 2 colunas)
//   linha 23      : painel de status
//   linha 24      : lembrete dos comandos
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

struct Jogo;
struct Batalha;
struct Criacao;

const int TELA_COLUNAS = 80;
const int TELA_LINHAS = 24;

// Quantas células do mapa a câmera mostra. Ímpar, para o jogador ficar
// exatamente no meio. 39 células x 2 colunas = 78 colunas na tela.
const int CAMERA_LARGURA = 39;
const int CAMERA_ALTURA = 21;

void desenharJogo(const Jogo& jogo);

// Célula do mapa que aparece no canto superior esquerdo da câmera.
void posicaoDaCamera(const Jogo& jogo, int& cameraX, int& cameraY);
void desenharAjuda(const Jogo& jogo);
// Zoom do mapa (teclas + e -; só na versão gráfica, no terminal não faz nada).
void mudarZoomDoMapa(int passo);
// Bestiário (tecla E no jogo e botão do menu). Com jogo = nullptr (menu),
// não mostra o elemento de cada espécie na partida.
void mostrarBestiario(const Jogo* jogo);
void desenharHistorico(const Jogo& jogo);
// ---- Animações (só na versão gráfica; no terminal não fazem nada) ----------
// A lógica só AVISA o que aconteceu; quem desenha a animação é a tela.
enum AnimacaoDaBatalha {
    ANIM_ENTRADA,         // começo da batalha: flash e personagens entrando
    ANIM_GOLPE,           // "quem" acerta "alvo" (valor = dano, 0 = golpe de status)
    ANIM_ERRO,            // "quem" erra "alvo"
    ANIM_EFEITO_PROPRIO,  // golpe em si mesmo (ex.: Escudo)
    ANIM_CURA,            // "quem" recupera "valor" de vida
    ANIM_DESMAIO,         // "quem" desmaia
    ANIM_NIVEL,           // o jogador sobe para o nível "valor"
    ANIM_BLOQUEIO,        // "quem" ataca e o escudo de "alvo" (o jogador) bloqueia
    ANIM_FURIA,           // o chefe ("quem") entra na segunda fase
    ANIM_FUMACA,          // bomba de fumaça: a tela some numa nuvem cinza
};
// quem/alvo: 0 = jogador, 1 a 3 = inimigos.
void animarBatalha(const Jogo& jogo, const Batalha& b, int animacao, int quem, int alvo,
                   int elemento, int valor, bool critico);

enum EfeitoNoMapa { EFEITO_ARMADILHA, EFEITO_ESPINHOS, EFEITO_ITEM, EFEITO_CAIXA, EFEITO_ATORDOAR, EFEITO_CURA,
                   EFEITO_MOEDAS, EFEITO_LAVA, EFEITO_CORNETA };
// Efeito rápido numa célula do mapa (não trava o jogo). "valor" depende do
// efeito: dano da armadilha, tipo do item, vida recuperada...
void efeitoNoMapa(const Jogo& jogo, int efeito, int x, int y, int valor);

// Fim da partida: derrota (vidas acabaram) ou vitória (chefe derrotado),
// com a pontuação e o ranking.
void desenharMorte(const Jogo& jogo);

// Tela de criação do personagem (Etapa 6).
void desenharCriacao(const Criacao& criacao);

// Página de texto com título (história, conversas com NPCs, como jogar...).
void desenharPagina(const std::string& titulo, const std::vector<std::string>& linhas,
                    const std::string& rodape);

// Tela de batalha (estilo Pokémon). Desenha o estado atual da batalha.
void desenharBatalha(const Jogo& jogo, const Batalha& batalha);

// Caixa no meio da tela com uma lista de opções (a "cursor" fica destacada).
void desenharMenuEmCaixa(const std::string& titulo, const std::vector<std::string>& opcoes,
                         const std::string& descricao, int cursor);

// Faz o próximo desenho apagar a tela inteira (usado ao sair da batalha).
void forcarLimpezaDaTela();

// Caixa de dica no meio do mapa (dicas.h). "travada": as teclas ainda não
// valem (mostra uma barrinha em vez do "aperte uma tecla").
void desenharDica(const Jogo& jogo, const std::string& titulo, const std::string& texto, bool travada);

// Mostra um aviso no meio da tela (ex.: janela pequena demais).
void desenharAviso(const std::string& linha1, const std::string& linha2);
