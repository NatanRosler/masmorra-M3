#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace Ui {
constexpr int LARGURA = 1440;
constexpr int ALTURA = 900;
inline constexpr Color FUNDO = {12,17,32,255};
inline constexpr Color PAINEL = {23,31,50,255};
inline constexpr Color PAINEL2 = {31,42,65,255};
inline constexpr Color TEXTO = {232,238,242,255};
inline constexpr Color APAGADO = {153,170,190,255};
inline constexpr Color OURO = {250,190,91,255};
inline constexpr Color CIANO = {109,222,209,255};
inline constexpr Color VIDA = {111,200,131,255};
inline constexpr Color PERIGO = {243,104,121,255};

void iniciarRecursos();
// Ícone da janela (o baú mímico, igual ao do masmorra.exe): desenhado aqui
// mesmo, a partir do sprite, para valer também no Linux e no Mac.
void aplicarIconeDaJanela();
void finalizarRecursos();
void iniciarQuadro();
// Desenho com zoom (o mapa): (x0, y0) é onde a origem do mundo cai na tela
// virtual e z é o zoom. terminarZoom volta ao normal.
void comecarZoom(float x0, float y0, float z);
void terminarZoom();
// Tela cheia (Opções ou F11): deixa a janela como opcoes().telaCheia pede.
void aplicarTelaCheia();
void alternarTelaCheia();   // troca, grava em opcoes.txt e aplica
void terminarQuadro();
void texto(const std::string& s, float x, float y, float tam = 22, Color cor = TEXTO);
void textoCentro(const std::string& s, float x, float y, float tam = 22, Color cor = TEXTO);
void painel(Rectangle r, Color cor = PAINEL, Color borda = {65,88,113,255});
void barra(float x, float y, float w, float h, int atual, int maximo, Color cor = VIDA);
Vector2 mouseVirtual();
void fundoEstrelado();
void sprite(const char* const linhas[16], float x, float y, float pixel, float brilho = 1.0f, float alfa = 1.0f);
// Igual ao sprite, mas sem a sombra no chão (ícones do painel e dos menus).
void icone(const char* const linhas[16], float x, float y, float pixel, float brilho = 1.0f, float alfa = 1.0f);
// Só o contorno cheio do desenho, todo numa cor (bestiário: monstro ainda
// não encontrado).
void silhueta(const char* const linhas[16], float x, float y, float pixel, Color cor);
// Cor da roupa do herói (letra 'J' nos sprites), escolhida na criação.
void definirCorDoHeroi(Color cor);
// Só desenha dentro do retângulo (ex.: o mapa, quando a câmera desliza).
void recortar(Rectangle r);
void pararDeRecortar();
// O mouse está em cima do retângulo?
bool mouseSobre(Rectangle r);
// Largura do texto em pixels (no tamanho pedido).
float larguraDoTexto(const std::string& s, float tam);
// Quebra o texto em linhas que caibam na largura (corta nos espaços).
std::vector<std::string> quebrarLinhas(const std::string& s, float tam, float largura);
// Desenha já quebrando as linhas; devolve quantas linhas usou.
int textoQuebrado(const std::string& s, float x, float y, float tam, float largura, float entrelinha, Color cor);
// Desenha numa linha só; se não couber, diminui a letra até caber.
void textoQueCabe(const std::string& s, float x, float y, float tam, float largura, Color cor);
}

// Tela que deve ser redesenhada enquanto o jogo espera uma tecla (assim as
// animações continuam rodando). Cada função desenhar...() registra a sua.
#include <functional>
void definirRedesenho(std::function<void()> funcao);

// Auxilia a integrar os cliques às seleções antigas por teclado.
void registrarOpcao(Rectangle retangulo, int indice, int cursor);
void limparOpcoes();
void registrarAtalho(Rectangle retangulo, int tecla);
