#pragma once
#include "raylib.h"
#include <string>

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
void finalizarRecursos();
void iniciarQuadro();
void terminarQuadro();
void texto(const std::string& s, float x, float y, float tam = 22, Color cor = TEXTO);
void textoCentro(const std::string& s, float x, float y, float tam = 22, Color cor = TEXTO);
void painel(Rectangle r, Color cor = PAINEL, Color borda = {65,88,113,255});
void barra(float x, float y, float w, float h, int atual, int maximo, Color cor = VIDA);
Vector2 mouseVirtual();
void fundoEstrelado();
void sprite(const char* const linhas[16], float x, float y, float pixel, float brilho = 1.0f);
// Cor da roupa do herói (letra 'J' nos sprites), escolhida na criação.
void definirCorDoHeroi(Color cor);
}

// Auxilia a integrar os cliques às seleções antigas por teclado.
void registrarOpcao(Rectangle retangulo, int indice, int cursor);
void limparOpcoes();
void registrarAtalho(Rectangle retangulo, int tecla);
