#pragma once
// v1.24: o fundo da batalha de cada sala de chefe (na ordem de TipoDeSala,
// sala_do_chefe.h). Tudo desenhado com formas simples da raylib, dentro da
// arena (x 42..1398, y 125..587). "t" é o tempo, para o que se mexe.
#include "raylib.h"
#include <cmath>

namespace FundosDeBatalha {


const float X0 = 42, Y0 = 125, X1 = 1398, Y1 = 587;
const float CHAO = 430;   // onde a parede do fundo encontra o chão

inline Color mistura(Color a, Color b, float f) {
    return { (unsigned char)(a.r + (b.r - a.r) * f), (unsigned char)(a.g + (b.g - a.g) * f),
             (unsigned char)(a.b + (b.b - a.b) * f), (unsigned char)(a.a + (b.a - a.a) * f) };
}
inline Color alfa(Color c, float a) { c.a = (unsigned char)(255 * a); return c; }
inline float ruido(float x) { return std::sin(x * 12.9898f) * 43758.5453f - std::floor(std::sin(x * 12.9898f) * 43758.5453f); }

// Luz quente redonda (tocha, lanterna, braseiro).
inline void brilho(float x, float y, float raio, Color c, float forca) {
    Color dentro = alfa(c, forca), fora = alfa(c, 0);
    DrawCircleGradient((int)x, (int)y, raio, dentro, fora);
}

// Pedras da parede: manchas um pouco mais claras e mais escuras.
inline void textura(Color base, int semente, int quantas) {
    for (int i = 0; i < quantas; i++) {
        float x = X0 + ruido(semente + i * 1.7f) * (X1 - X0);
        float y = Y0 + ruido(semente + i * 3.1f) * (CHAO - Y0);
        float w = 30 + ruido(semente + i * 5.3f) * 70, h = 14 + ruido(semente + i * 7.9f) * 26;
        float f = 0.8f + ruido(semente + i * 2.3f) * 0.4f;
        Color c = { (unsigned char)std::fmin(255, base.r * f), (unsigned char)std::fmin(255, base.g * f),
                    (unsigned char)std::fmin(255, base.b * f), 255 };
        DrawEllipse((int)x, (int)y, w / 2, h / 2, c);
    }
}

inline void chamaPequena(float x, float y, float t, float tam) {
    for (int k = 0; k < 3; k++) {
        float f = std::sin(t * 9 + k * 2.1f) * 0.15f + 1;
        Color c = k == 0 ? Color{255, 120, 40, 230} : k == 1 ? Color{255, 190, 70, 230} : Color{255, 245, 190, 240};
        float s = tam * (1 - k * 0.3f) * f;
        DrawTriangle({x - s * 0.5f, y}, {x + s * 0.5f, y}, {x + std::sin(t * 7 + k) * 2, y - s * 1.6f}, c);
        DrawCircleV({x, y - s * 0.1f}, s * 0.5f, c);
    }
}

// ---------------------------------------------------------------------------
// 1. Salão do Rei Goblin (minas): escoras de madeira, lanternas, trilhos e o ouro roubado.
inline void salaoDoRei(float t) {
    DrawRectangleGradientV(X0, Y0, X1 - X0, CHAO - Y0, {46, 34, 28, 255}, {36, 28, 24, 255});
    textura({58, 44, 36, 255}, 11, 70);
    DrawRectangleGradientV(X0, CHAO, X1 - X0, Y1 - CHAO, {52, 40, 30, 255}, {34, 26, 22, 255});
    // Trono tosco de madeira e ouro atrás do rei (o encosto aparece em volta dele).
    float bx = 1010;
    DrawRectangle(bx - 120, 250, 240, 200, {92, 58, 32, 255});
    DrawRectangle(bx - 120, 250, 240, 12, {150, 104, 46, 255});
    for (int k = -2; k <= 2; k++) DrawTriangle({bx + k * 48 - 16, 250}, {bx + k * 48 + 16, 250}, {bx + k * 48, 222}, {226, 176, 62, 255});
    DrawRectangle(bx - 136, 380, 34, 70, {92, 58, 32, 255});
    DrawRectangle(bx + 102, 380, 34, 70, {92, 58, 32, 255});
    // Escoras: dois postes e uma viga em cima, de lado a lado.
    Color madeira = {112, 74, 42, 255}, escura = {72, 46, 26, 255};
    DrawRectangle(X0, Y0 + 18, X1 - X0, 26, madeira);
    DrawRectangle(X0, Y0 + 40, X1 - X0, 5, escura);
    for (float px : {120.0f, 560.0f, 1300.0f}) {
        DrawRectangle(px - 16, Y0 + 18, 32, CHAO - Y0 - 10, madeira);
        DrawRectangle(px + 8, Y0 + 18, 8, CHAO - Y0 - 10, escura);
        DrawTriangle({px - 16, Y0 + 44}, {px - 60, Y0 + 44}, {px - 16, Y0 + 88}, escura);
        DrawTriangle({px + 16, Y0 + 44}, {px + 16, Y0 + 88}, {px + 60, Y0 + 44}, escura);
    }
    // Lanternas penduradas na viga.
    for (float lx : {330.0f, 780.0f, 1180.0f}) {
        float bal = std::sin(t * 1.3f + lx) * 3;
        DrawLineEx({lx, Y0 + 44}, {lx + bal, Y0 + 92}, 2, {40, 30, 24, 255});
        brilho(lx + bal, Y0 + 108, 120, {255, 190, 90, 255}, 0.35f + 0.05f * std::sin(t * 7 + lx));
        DrawRectangle(lx + bal - 10, Y0 + 92, 20, 28, {60, 46, 36, 255});
        DrawRectangle(lx + bal - 6, Y0 + 97, 12, 18, {255, 214, 120, 255});
    }
    // Pilhas de ouro e moedas brilhando.
    auto pilha = [&](float x, float y, float r) {
        for (int i = 0; i < 26; i++) {
            float a = ruido(x + i) * 3.14159f, d = ruido(y + i * 2) * r;
            float cx = x + std::cos(a) * d * 1.6f, cy = y - std::sin(a) * d * 0.9f;
            DrawCircleV({cx, cy}, 7, i % 3 ? Color{226, 176, 62, 255} : Color{250, 214, 110, 255});
            DrawCircleLinesV({cx, cy}, 7, {150, 104, 30, 255});
        }
        float pisca = std::fmax(0, std::sin(t * 2.5f + x));
        DrawCircleV({x + 12, y - r * 0.6f}, 3 + 3 * pisca, {255, 250, 220, (unsigned char)(200 * pisca)});
    };
    pilha(560, CHAO + 6, 46);
    pilha(1345, CHAO + 4, 40);
    // Trilhos da mina atravessando o chão.
    Color ferro = {120, 120, 132, 255};
    for (int i = 0; i < 16; i++) DrawRectangle(X0 + 30 + i * 86, 548, 46, 12, escura);
    DrawRectangle(X0, 546, X1 - X0, 5, ferro);
    DrawRectangle(X0, 562, X1 - X0, 5, ferro);
}

// ---------------------------------------------------------------------------
// 2. Ninho da Rainha Aranha (caverna): estalactites, teias enormes, casulos e ovos.
inline void ninhoDaRainha(float t) {
    DrawRectangleGradientV(X0, Y0, X1 - X0, CHAO - Y0, {16, 32, 38, 255}, {22, 30, 40, 255});
    textura({26, 46, 54, 255}, 23, 60);
    DrawRectangleGradientV(X0, CHAO, X1 - X0, Y1 - CHAO, {28, 42, 48, 255}, {18, 26, 32, 255});
    // Estalactites.
    for (int i = 0; i < 22; i++) {
        float x = X0 + 20 + i * 63 + ruido(i * 3.3f) * 20, h = 30 + ruido(i * 7.1f) * 70;
        DrawTriangle({x - 16, Y0}, {x, Y0 + h}, {x + 16, Y0}, {34, 56, 64, 255});
        DrawTriangle({x - 4, Y0}, {x + 3, Y0 + h * 0.8f}, {x + 8, Y0}, {48, 74, 82, 255});
    }
    // Teia: raios saindo do canto e anéis ligando os raios.
    auto teia = [&](float cx, float cy, float r, float a0, float a1) {
        Color fio = {210, 220, 225, 120};
        const int RAIOS = 7;
        for (int k = 0; k <= RAIOS; k++) {
            float a = a0 + (a1 - a0) * k / RAIOS;
            DrawLineEx({cx, cy}, {cx + std::cos(a) * r, cy + std::sin(a) * r}, 1.5f, fio);
        }
        for (int anel = 1; anel <= 6; anel++) {
            float d = r * anel / 6.5f;
            for (int k = 0; k < RAIOS; k++) {
                float a = a0 + (a1 - a0) * k / RAIOS, b = a0 + (a1 - a0) * (k + 1) / RAIOS;
                DrawLineEx({cx + std::cos(a) * d, cy + std::sin(a) * d}, {cx + std::cos(b) * d, cy + std::sin(b) * d}, 1.2f, fio);
            }
        }
    };
    teia(X0, Y0, 290, 0.0f, 1.5708f);
    teia(X1, Y0, 260, 1.5708f, 3.1416f);
    teia(1010, Y0 - 40, 230, 0.35f, 2.8f);   // atrás da rainha
    // Casulos pendurados (balançando de leve).
    for (float cx : {520.0f, 640.0f, 1350.0f}) {
        float bal = std::sin(t * 0.9f + cx) * 4, comp = 40 + ruido(cx) * 60;
        DrawLineEx({cx, Y0}, {cx + bal, Y0 + comp}, 1.5f, {210, 220, 225, 160});
        DrawEllipse(cx + bal, Y0 + comp + 30, 18, 34, {200, 205, 195, 255});
        for (int k = 0; k < 4; k++) DrawLineEx({cx + bal - 17, Y0 + comp + 10 + k * 14}, {cx + bal + 17, Y0 + comp + 18 + k * 14}, 1.5f, {150, 155, 150, 255});
    }
    // Ovos no chão, alguns brilhando.
    for (int i = 0; i < 9; i++) {
        float x = 470 + i * 20 + (i % 2) * 8, y = CHAO + 18 - (i % 3) * 10;
        DrawEllipse(x, y, 13, 16, {214, 222, 200, 255});
        if (i % 3 == 0) brilho(x, y, 20, {170, 255, 160, 255}, 0.25f + 0.15f * std::sin(t * 3 + i));
    }
    // Esporos verdes flutuando.
    for (int i = 0; i < 18; i++) {
        float f = std::fmod(t * 0.05f + ruido(i * 9.1f), 1.0f);
        float x = X0 + ruido(i * 4.7f) * (X1 - X0) + std::sin(t + i) * 10, y = Y1 - f * (Y1 - Y0);
        DrawCircleV({x, y}, 2.5f, {170, 255, 170, (unsigned char)(160 * std::sin(f * 3.1416f))});
    }
}

// ---------------------------------------------------------------------------
// 3. Altar do Necromante (cripta): colunas, nichos com crânios, velas e o círculo do ritual.
inline void altarDoNecromante(float t) {
    DrawRectangleGradientV(X0, Y0, X1 - X0, CHAO - Y0, {22, 30, 30, 255}, {26, 26, 34, 255});
    // Blocos da parede.
    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 20; x++) {
            float bx = X0 + x * 72 + (y % 2) * 36 - 36, by = Y0 + y * 31;
            Color c = mistura({34, 42, 42, 255}, {44, 52, 52, 255}, ruido(x * 7 + y * 13.0f));
            DrawRectangle(bx + 2, by + 2, 68, 27, c);
        }
    DrawRectangleGradientV(X0, CHAO, X1 - X0, Y1 - CHAO, {36, 40, 44, 255}, {20, 22, 28, 255});
    // Nichos com crânios.
    for (float nx : {250.0f, 470.0f, 1230.0f}) {
        DrawRectangle(nx - 34, Y0 + 70, 68, 60, {14, 18, 20, 255});
        DrawCircle(nx - 34 + 34, Y0 + 70, 34, {14, 18, 20, 255});
        DrawCircle(nx, Y0 + 104, 14, {220, 220, 200, 255});
        DrawRectangle(nx - 9, Y0 + 112, 18, 10, {220, 220, 200, 255});
        DrawCircle(nx - 5, Y0 + 102, 4, {14, 18, 20, 255});
        DrawCircle(nx + 5, Y0 + 102, 4, {14, 18, 20, 255});
    }
    // Colunas.
    for (float cx : {120.0f, 690.0f, 1330.0f}) {
        DrawRectangle(cx - 26, Y0, 52, CHAO - Y0 + 20, {58, 64, 66, 255});
        DrawRectangle(cx + 10, Y0, 16, CHAO - Y0 + 20, {42, 48, 50, 255});
        DrawRectangle(cx - 34, CHAO + 4, 68, 18, {66, 72, 74, 255});
    }
    // Círculo do ritual no chão, girando devagar, com runas.
    float cx = 1010, cy = 492;
    float pulso = 0.6f + 0.4f * std::sin(t * 2);
    Color verde = {120, 255, 150, 255};
    brilho(cx, cy, 260, verde, 0.10f * pulso);
    DrawEllipseLines(cx, cy, 250, 56, alfa(verde, 0.8f * pulso));
    DrawEllipseLines(cx, cy, 226, 48, alfa(verde, 0.6f * pulso));
    for (int k = 0; k < 10; k++) {
        float a = t * 0.3f + k * 0.628f;
        float rx = cx + std::cos(a) * 238, ry = cy + std::sin(a) * 52;
        DrawRectangle(rx - 4, ry - 6, 8, 12, alfa(verde, 0.9f * pulso));
        DrawRectangle(rx - 7, ry - 1, 14, 3, alfa(verde, 0.9f * pulso));
    }
    // Velas no chão e na parede.
    for (float vx : {330.0f, 560.0f, 1160.0f, 1290.0f}) {
        float vy = vx > 1000 ? CHAO + 30 : CHAO + 10;
        DrawRectangle(vx - 6, vy - 26, 12, 26, {220, 214, 190, 255});
        brilho(vx, vy - 32, 70, {150, 255, 170, 255}, 0.25f);
        chamaPequena(vx, vy - 26, t + vx, 8);
    }
    // Névoa rasteira esverdeada.
    for (int k = 0; k < 6; k++) {
        float x = std::fmod(t * 18 + k * 260, X1 - X0 + 400) + X0 - 200;
        DrawEllipse(x, 560 - (k % 2) * 16, 180, 22, {150, 210, 170, 22});
    }
}

// ---------------------------------------------------------------------------
// 4. Forja do Golem (vulcão): rochas escuras, cascatas e rio de lava, brasas subindo.
inline void forjaDoGolem(float t) {
    DrawRectangleGradientV(X0, Y0, X1 - X0, CHAO - Y0, {34, 16, 16, 255}, {58, 24, 18, 255});
    // Cascatas de lava no fundo.
    for (float lx : {240.0f, 1290.0f}) {
        for (int k = 0; k < 5; k++) {
            float onda = std::sin(t * 3 + k + lx) * 3;
            DrawRectangleGradientV(lx - 22 + k * 9 + onda, Y0, 9, CHAO - Y0, {255, 150, 50, 255}, {230, 70, 30, 255});
        }
        brilho(lx, CHAO, 170, {255, 120, 40, 255}, 0.35f);
    }
    // Rochas recortadas (silhuetas).
    for (int i = 0; i < 12; i++) {
        float x = X0 + i * 125, h = 120 + ruido(i * 5.5f) * 140;
        DrawTriangle({x - 40, CHAO + 10}, {x + 70 + ruido(i * 1.1f) * 40, CHAO + 10}, {x + 20, CHAO - h}, {28, 14, 16, 255});
    }
    DrawRectangleGradientV(X0, CHAO, X1 - X0, Y1 - CHAO, {48, 24, 20, 255}, {30, 16, 16, 255});
    // A boca da forja atrás do golem, brilhando, e a bigorna do lado.
    float bx = 990;
    brilho(bx, CHAO - 60, 240, {255, 110, 40, 255}, 0.30f + 0.08f * std::sin(t * 4));
    DrawRectangle(bx - 150, CHAO - 210, 300, 200, {38, 26, 28, 255});
    DrawCircle(bx, CHAO - 210, 150, {38, 26, 28, 255});
    DrawRectangleGradientV(bx - 100, CHAO - 150, 200, 140, {120, 40, 20, 255}, {255, 150, 60, 255});
    DrawCircleGradient(bx, CHAO - 150, 100, {150, 50, 24, 255}, {120, 40, 20, 255});
    float ax = 1290;
    DrawRectangle(ax - 60, CHAO - 50, 120, 22, {58, 52, 56, 255});
    DrawTriangle({ax + 60, CHAO - 50}, {ax + 60, CHAO - 28}, {ax + 100, CHAO - 50}, {58, 52, 56, 255});
    DrawRectangle(ax - 24, CHAO - 28, 48, 38, {46, 42, 46, 255});
    // Rio de lava na frente, ondulando.
    for (int x = (int)X0; x < (int)X1; x += 6) {
        float y = 560 + std::sin(x * 0.03f + t * 2) * 5;
        DrawRectangle(x, y, 6, Y1 - y, {(unsigned char)(220 + 30 * std::sin(x * 0.05f + t * 3)), 90, 30, 255});
        DrawRectangle(x, y, 6, 3, {255, 200, 90, 255});
    }
    // Brasas subindo.
    for (int i = 0; i < 26; i++) {
        float f = std::fmod(t * 0.25f + ruido(i * 2.9f), 1.0f);
        float x = X0 + ruido(i * 6.1f) * (X1 - X0) + std::sin(t * 2 + i) * 14, y = 580 - f * 440;
        DrawCircleV({x, y}, 2.5f * (1 - f) + 1, {255, (unsigned char)(140 + 100 * (1 - f)), 60, (unsigned char)(230 * (1 - f))});
    }
}

// ---------------------------------------------------------------------------
// 5. Salão do Trono (chefe final): colunas, estandartes, braseiros, tapete e o trono.
inline void salaoDoTrono(float t, bool furioso) {
    DrawRectangleGradientV(X0, Y0, X1 - X0, CHAO - Y0, {28, 16, 26, 255}, {36, 20, 30, 255});
    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 20; x++) {
            float bx = X0 + x * 72 + (y % 2) * 36 - 36, by = Y0 + y * 31;
            DrawRectangle(bx + 2, by + 2, 68, 27, mistura({38, 24, 34, 255}, {48, 30, 40, 255}, ruido(x * 5 + y * 11.0f)));
        }
    DrawRectangleGradientV(X0, CHAO, X1 - X0, Y1 - CHAO, {44, 28, 34, 255}, {22, 14, 20, 255});
    // Tapete vermelho até o trono.
    DrawTriangle({900, CHAO}, {760, Y1}, {1260, Y1}, {120, 28, 40, 255});
    DrawTriangle({900, CHAO}, {1260, Y1}, {1120, CHAO}, {120, 28, 40, 255});
    DrawLineEx({900, CHAO}, {760, Y1}, 4, {220, 170, 70, 255});
    DrawLineEx({1120, CHAO}, {1260, Y1}, 4, {220, 170, 70, 255});
    // Trono de pedra atrás do chefe.
    float tx = 1010;
    DrawRectangle(tx - 110, Y0 + 30, 220, CHAO - Y0 - 20, {40, 30, 40, 255});
    DrawTriangle({tx - 110, Y0 + 30}, {tx, Y0 - 10}, {tx + 110, Y0 + 30}, {40, 30, 40, 255});
    DrawRectangle(tx - 84, Y0 + 60, 168, CHAO - Y0 - 60, {110, 24, 36, 255});
    for (int k = -1; k <= 1; k++) DrawCircle(tx + k * 70, Y0 + 30, 10, {220, 170, 70, 255});
    // Estandartes e colunas.
    for (float cx : {170.0f, 600.0f, 1340.0f}) {
        DrawRectangle(cx - 30, Y0, 60, CHAO - Y0 + 20, {56, 40, 50, 255});
        DrawRectangle(cx + 14, Y0, 16, CHAO - Y0 + 20, {42, 30, 38, 255});
    }
    for (float ex : {385.0f, 790.0f, 1230.0f}) {
        float bal = std::sin(t * 1.1f + ex) * 2;
        DrawRectangle(ex - 32 + bal, Y0 + 10, 64, 150, {130, 26, 38, 255});
        DrawTriangle({ex - 32 + bal, Y0 + 160}, {ex + bal, Y0 + 186}, {ex + 32 + bal, Y0 + 160}, {130, 26, 38, 255});
        DrawCircleLines(ex + bal, Y0 + 80, 18, {220, 170, 70, 255});
        DrawCircle(ex + bal, Y0 + 80, 6, {220, 170, 70, 255});
    }
    // Braseiros dos dois lados do trono.
    for (float bx : {800.0f, 1220.0f}) {
        brilho(bx, CHAO - 40, 150, {255, 120, 50, 255}, 0.35f + 0.08f * std::sin(t * 6 + bx));
        DrawRectangle(bx - 22, CHAO - 30, 44, 14, {70, 56, 50, 255});
        DrawRectangle(bx - 6, CHAO - 16, 12, 40, {60, 48, 44, 255});
        chamaPequena(bx, CHAO - 30, t + bx, 22);
    }
    // Segunda fase (o chefe furioso): o salão começa a desabar. Pedras caem
    // do teto e levantam poeira ao bater no chão.
    if (furioso) {
        for (int i = 0; i < 9; i++) {
            float ciclo = 1.6f + ruido(i * 3.7f) * 1.4f;
            float f = std::fmod(t / ciclo + ruido(i * 9.3f), 1.0f);
            float x = X0 + 60 + ruido(i * 5.1f) * (X1 - X0 - 120);
            float chao = 470 + ruido(i * 2.2f) * 90;
            float tam = 7 + ruido(i * 6.6f) * 9;
            if (f < 0.75f) {
                float y = Y0 - 20 + (chao - Y0 + 20) * (f / 0.75f) * (f / 0.75f);
                Vector2 p1 = {x - tam, y}, p2 = {x + tam * 0.4f, y - tam}, p3 = {x + tam, y + tam * 0.3f}, p4 = {x - tam * 0.2f, y + tam};
                DrawTriangle(p1, p3, p2, {96, 84, 92, 255});
                DrawTriangle(p1, p4, p3, {76, 66, 74, 255});
            } else {
                float u = (f - 0.75f) / 0.25f;
                for (int k = 0; k < 4; k++)
                    DrawCircleV({x + (k - 1.5f) * tam * (0.6f + u), chao - u * 10}, tam * 0.7f * (1 - u) + 2,
                                {150, 140, 150, (unsigned char)(150 * (1 - u))});
            }
        }
    }
}

// "sala" segue a ordem de TipoDeSala: rei, rainha, necromante, golem, trono.
inline void desenhar(int sala, float t, bool furioso) {
    switch (sala) {
        case 0: salaoDoRei(t); break;
        case 1: ninhoDaRainha(t); break;
        case 2: altarDoNecromante(t); break;
        case 3: forjaDoGolem(t); break;
        case 4: salaoDoTrono(t, furioso); break;
        default: break;
    }
}

}  // namespace FundosDeBatalha
