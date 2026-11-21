// efeitos.h
// -----------------------------------------------------------------------------
// Efeitos visuais da versão gráfica: partículas, números flutuantes, tremor e
// piscada da tela, escurecimento entre telas e o "afundar" dos botões.
//
// Tudo é calculado pelo RELÓGIO (GetTime): cada efeito guarda quando começou
// e quanto dura, e é desenhado na posição certa para aquele instante. Quando
// o tempo acaba, ele sai da lista.
// -----------------------------------------------------------------------------
#pragma once
#include "raylib.h"
#include <string>

namespace Efeitos {

// Onde a partícula vive: na tela (batalha) ou no mapa (anda junto com a câmera).
enum Camada { CAMADA_TELA, CAMADA_MAPA };

// Formatos de partícula.
enum Forma { BOLINHA, ANEL, QUADRADO, ESTRELA, FOLHA, FAISCA, RELAMPAGO };

struct Particula {
    float x, y, vx, vy, gravidade;
    Color cor;
    float tamanho;
    double inicio, vida;
    int forma;
    float giro;
    int camada;
};

// Partículas do elemento de um golpe (0 = Normal, 1 = Fogo, 2 = Água,
// 3 = Planta, 4 = Veneno, 5 = Pedra, 6 = Gelo, 7 = Raio, 8 = Trevas,
// 9 = Luz), espalhadas a partir de (x, y).
void explosaoDoElemento(int elemento, float x, float y, int camada, float forca = 1.0f);
// Brilho subindo (cura em verde, nível em dourado...).
void brilhoSubindo(float x, float y, Color cor, int quantidade, int camada);
// Lascas de madeira (caixa quebrando) e espinhos (armadilha).
void lascas(float x, float y, int camada);
void espinhos(float x, float y, int camada);
// Uma partícula avulsa.
void criarParticula(const Particula& p);
void desenharParticulas(int camada, float deslocX = 0, float deslocY = 0);
void limparParticulas(int camada);

// Texto que sobe e some (ex.: "-12", "ERROU", "+20").
void textoFlutuante(const std::string& texto, float x, float y, Color cor, float tamanho, int camada,
                    double duracao = 0.9);
void desenharTextosFlutuantes(int camada, float deslocX = 0, float deslocY = 0);

// Tremor da tela inteira (golpe crítico, armadilha).
void tremer(float intensidade, double duracao);
Vector2 deslocamentoDoTremor();

// Piscada da tela inteira com uma cor (branco no início da batalha, vermelho
// na armadilha).
void piscar(Color cor, double duracao);

// Escurecimento rápido ao trocar de tela. "idDaTela" identifica a tela: quando
// muda, a nova tela aparece saindo do escuro.
void trocarDeTela(int idDaTela);
void escurecer();   // força o efeito (ex.: ao descer uma escada)

// Botão apertado: flash rápido sobre o retângulo.
void marcarClique(Rectangle r);

// Desenhado no fim de cada quadro (piscada, escurecimento e clique).
void desenharSobreposicoes();

// Destaque dourado que desliza entre as opções de um menu.
Rectangle destaqueDeslizante(Rectangle alvo);

// Suaviza um valor na direção do alvo (velocidade em "por segundo").
float aproximar(float atual, float alvo, float velocidade, float dt);

}
