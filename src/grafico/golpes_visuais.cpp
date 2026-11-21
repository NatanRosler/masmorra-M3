// golpes_visuais.cpp
#include "golpes_visuais.h"
#include "visual.h"
#include "batalha.h"
#include "monstros.h"
#include <algorithm>
#include <cmath>

namespace GolpesVisuais {

namespace {
const float PI_F = 3.14159265f;

Color corDoElemento(int e) {
    CorRGB c = CORES_DOS_ELEMENTOS[e];
    return { (unsigned char)c.r, (unsigned char)c.g, (unsigned char)c.b, 255 };
}
Color comAlfa(Color c, float a) { c.a = (unsigned char)(std::clamp(a, 0.0f, 1.0f) * c.a); return c; }
Color clarear(Color c, float f) {
    return { (unsigned char)(c.r + (255 - c.r) * f), (unsigned char)(c.g + (255 - c.g) * f),
             (unsigned char)(c.b + (255 - c.b) * f), c.a };
}
// Quanto do trecho [a, b] da animação já passou (0 a 1).
float fase(float t, float a, float b) { return std::clamp((t - a) / (b - a), 0.0f, 1.0f); }
Vector2 entre(Vector2 a, Vector2 b, float s) { return { a.x + (b.x - a.x) * s, a.y + (b.y - a.y) * s }; }

// ---- cada forma ----------------------------------------------------------------
void corte(Vector2 B, float t, Color c) {
    for (int k = 0; k < 3; k++) {
        float a = 0.22f + k * 0.06f;
        float cresce = fase(t, a, a + 0.06f), some = 1 - fase(t, a + 0.1f, a + 0.35f);
        if (cresce <= 0 || some <= 0) continue;
        Vector2 p0 = { B.x - 80 + k * 34, B.y - 90 }, p1 = { B.x + 30 + k * 34, B.y + 80 };
        Vector2 fim = entre(p0, p1, cresce);
        DrawLineEx(p0, fim, 14, comAlfa(c, 0.45f * some));
        DrawLineEx(p0, fim, 5, comAlfa(WHITE, some));
    }
}

void mordida(Vector2 B, float t) {
    float fecha = fase(t, 0.15f, 0.32f), some = 1 - fase(t, 0.45f, 0.65f);
    if (t < 0.15f || some <= 0) return;
    float abertura = 80 * (1 - fecha) + 6;
    for (int lado = -1; lado <= 1; lado += 2) {
        float y = B.y + lado * abertura;
        DrawRectangle((int)B.x - 90, (int)(y + (lado < 0 ? -26 : 10)), 180, 16, comAlfa({ 120, 30, 40, 255 }, some));
        for (int k = 0; k < 6; k++) {
            float x = B.x - 84 + k * 30;
            Vector2 a = { x, y + (lado < 0 ? -10.0f : 10.0f) }, b = { x + 24, a.y }, ponta = { x + 12, y + (lado < 0 ? 16.0f : -16.0f) };
            if (lado < 0) DrawTriangle(a, ponta, b, comAlfa(WHITE, some));
            else DrawTriangle(a, b, ponta, comAlfa(WHITE, some));
        }
    }
}

void ondaDeChoque(Vector2 B, float t, Color c) {
    float f = fase(t, 0.28f, 0.58f);
    if (f <= 0 || f >= 1) return;
    DrawRing(B, 20 + f * 110, 26 + f * 116, 0, 360, 48, comAlfa(c, 1 - f));
    for (int k = 0; k < 8; k++) {
        float a = k * 0.785f + 0.3f, r0 = 30 + f * 60, r1 = 60 + f * 110;
        DrawLineEx({ B.x + std::cos(a) * r0, B.y + std::sin(a) * r0 }, { B.x + std::cos(a) * r1, B.y + std::sin(a) * r1 },
                   4, comAlfa(WHITE, 1 - f));
    }
}

void projetil(int golpe, int e, Vector2 A, Vector2 B, float t) {
    if (t < 0.08f || t > 0.45f) return;
    float s = fase(t, 0.08f, 0.45f);
    Vector2 p = entre(A, B, s);
    bool grande = GOLPES[golpe].poder >= 90;
    float tam = grande ? 1.6f : 1.0f;
    float agora = (float)GetTime();
    if (e == PEDRA) {   // pedra em arco, girando
        p.y -= std::sin(s * PI_F) * 150;
        DrawRectanglePro({ p.x, p.y, 30 * tam, 26 * tam }, { 15 * tam, 13 * tam }, agora * 720, { 175, 140, 100, 255 });
    } else if (e == FOGO) {   // bola de fogo com rastro
        for (int k = 6; k >= 1; k--) {
            Vector2 q = entre(A, B, std::max(0.0f, s - k * 0.03f));
            DrawCircleV(q, (14 - k * 1.6f) * tam, comAlfa({ 255, 110, 40, 255 }, 0.7f - k * 0.1f));
        }
        DrawCircleGradient((int)p.x, (int)p.y, 34 * tam, { 255, 160, 60, 140 }, { 255, 160, 60, 0 });
        DrawCircleV(p, 16 * tam, { 255, 120, 40, 255 });
        DrawCircleV(p, 9 * tam, { 255, 230, 120, 255 });
    } else if (e == GELO) {   // três estilhaços
        for (int k = -1; k <= 1; k++) {
            Vector2 q = { p.x - k * 18, p.y + k * 22 };
            DrawPoly(q, 4, 13 * tam, 45, { 180, 235, 255, 255 });
            DrawPoly(q, 4, 6 * tam, 45, WHITE);
            DrawLineEx({ q.x - 40, q.y }, { q.x - 14, q.y }, 3, { 180, 235, 255, 140 });
        }
    } else if (e == PLANTA) {   // folhas (ou uma semente) girando
        int n = GOLPES[golpe].efeito == SANGUESSUGA ? 1 : 4;
        for (int k = 0; k < n; k++) {
            Vector2 q = { p.x - k * 26, p.y + std::sin(s * 12 + k) * 22 };
            if (n == 1) { DrawEllipse((int)q.x, (int)q.y, 9, 7, { 150, 110, 60, 255 }); DrawEllipse((int)q.x + 4, (int)q.y - 7, 6, 3, { 100, 210, 100, 255 }); continue; }
            DrawEllipse((int)q.x, (int)q.y, 14, 6, { 100, 210, 100, 255 });
            DrawLineEx({ q.x - 12, q.y }, { q.x + 12, q.y }, 2, { 60, 140, 70, 255 });
        }
    } else if (e == VENENO || e == AGUA) {   // gota / bolha de ácido
        Color c = corDoElemento(e);
        DrawCircleV(p, 13 * tam, c);
        DrawCircleV({ p.x - 4, p.y - 4 }, 4 * tam, clarear(c, 0.6f));
        for (int k = 1; k <= 3; k++) DrawCircleV(entre(A, B, std::max(0.0f, s - k * 0.05f)), 6.0f - k, comAlfa(c, 0.6f));
    } else {   // agulha (Ferrão) e o resto: uma bolinha com brilho
        Color c = corDoElemento(e);
        DrawCircleGradient((int)p.x, (int)p.y, 26 * tam, comAlfa(c, 0.5f), comAlfa(c, 0));
        DrawCircleV(p, 10 * tam, c);
        DrawCircleV(p, 4 * tam, WHITE);
    }
}

void chama(float x, float base, float altura, float balanco, Color fora, Color meio, Color dentro, float alfa) {
    DrawTriangle({ x - 24, base }, { x + 24, base }, { x + balanco, base - altura }, comAlfa(fora, alfa));
    DrawTriangle({ x - 15, base }, { x + 15, base }, { x + balanco * 0.7f, base - altura * 0.7f }, comAlfa(meio, alfa));
    DrawTriangle({ x - 7, base }, { x + 7, base }, { x + balanco * 0.4f, base - altura * 0.4f }, comAlfa(dentro, alfa));
}

// Sobe do chão embaixo do alvo: fogo, pilar de gelo, onda d'água ou redemoinho de folhas.
void coluna(int e, Vector2 B, float t, float metade) {
    float cresce = fase(t, 0.2f, 0.42f), some = 1 - fase(t, 0.6f, 0.9f);
    if (t < 0.2f || some <= 0) return;
    float base = B.y + metade, agora = (float)GetTime();
    Color fora = { 235, 80, 30, 255 }, meio = { 255, 160, 50, 255 }, dentro = { 255, 235, 130, 255 };
    if (e == GELO)   { fora = { 120, 190, 240, 255 }; meio = { 180, 230, 255, 255 }; dentro = WHITE; }
    if (e == AGUA)   { fora = { 50, 110, 210, 255 };  meio = { 90, 160, 240, 255 };  dentro = { 200, 230, 255, 255 }; }
    if (e == PLANTA) { fora = { 50, 130, 60, 255 };   meio = { 100, 200, 100, 255 }; dentro = { 190, 240, 160, 255 }; }
    DrawEllipse((int)B.x, (int)base, 110, 22, comAlfa(fora, 0.5f * some));
    DrawCircleGradient((int)B.x, (int)(base - 100 * cresce), 150, comAlfa(meio, 0.3f * some), comAlfa(meio, 0));
    for (int k = 0; k < 7; k++) {
        float x = B.x - 90 + k * 30;
        float altura = (140 + 120 * std::pow(std::sin(k * 1.9f + 0.5f), 2.0f)) * cresce *
                       (0.85f + 0.15f * std::sin(agora * 18 + k * 2));
        float balanco = (e == GELO) ? 0 : std::sin(agora * 10 + k) * 10;
        chama(x, base, altura, balanco, fora, meio, dentro, some);
    }
}

void feixe(int e, Vector2 A, Vector2 B, float t) {
    float cresce = fase(t, 0.12f, 0.25f), some = 1 - fase(t, 0.5f, 0.7f);
    if (t < 0.12f || some <= 0) return;
    Vector2 fim = entre(A, B, cresce);
    float agora = (float)GetTime();
    if (e == AGUA) {   // jato ondulado
        for (int k = 0; k <= 30; k++) {
            float s = k / 30.0f;
            if (s > cresce) break;
            Vector2 q = entre(A, B, s);
            q.y += std::sin(s * 18 - agora * 30) * 8;
            DrawCircleV(q, 12, comAlfa({ 80, 150, 240, 255 }, some));
            DrawCircleV(q, 5, comAlfa({ 200, 230, 255, 255 }, some));
        }
        return;
    }
    Color c = corDoElemento(e);
    if (e == TREVAS) c = { 110, 70, 170, 255 };
    DrawLineEx(A, fim, 46 * some, comAlfa(c, 0.35f));
    DrawLineEx(A, fim, 22 * some, comAlfa(c, 0.9f));
    DrawLineEx(A, fim, 8 * some, comAlfa(e == TREVAS ? Color{ 200, 170, 255, 255 } : WHITE, 1));
    DrawCircleGradient((int)fim.x, (int)fim.y, 70 * some, comAlfa(clarear(c, 0.6f), 0.8f), comAlfa(c, 0));
}

void chicote(int golpe, int e, Vector2 A, Vector2 B, float t) {
    float vai = fase(t, 0.1f, 0.3f), volta = fase(t, 0.45f, 0.65f);
    float s = vai * (1 - volta);
    if (s <= 0) return;
    Vector2 C = { (A.x + B.x) / 2, std::min(A.y, B.y) - 180 };
    bool lingua = golpe == G_LINGUA_GRUDENTA, teia = golpe == G_TEIA;
    Color c = lingua ? Color{ 235, 120, 150, 255 } : teia ? Color{ 225, 230, 240, 230 } : corDoElemento(e);
    if (!lingua && !teia && e == PLANTA) c = { 90, 180, 80, 255 };
    float grossura = lingua ? 12 : teia ? 3 : 8;
    Vector2 anterior = A;
    for (int k = 1; k <= 24; k++) {
        float u = s * k / 24.0f;
        Vector2 p = { (1 - u) * (1 - u) * A.x + 2 * u * (1 - u) * C.x + u * u * B.x,
                      (1 - u) * (1 - u) * A.y + 2 * u * (1 - u) * C.y + u * u * B.y };
        DrawLineEx(anterior, p, grossura, c);
        if (e == PLANTA && !lingua && !teia && k % 5 == 0) DrawEllipse((int)p.x + 8, (int)p.y - 6, 9, 4, { 120, 215, 110, 255 });
        anterior = p;
    }
    if (lingua) DrawCircleV(anterior, 10, c);
    if (teia && vai >= 1 && volta <= 0) {   // a teia abre em cima do alvo
        for (int k = 0; k < 8; k++) {
            float a = k * PI_F / 4;
            DrawLineEx(B, { B.x + std::cos(a) * 70, B.y + std::sin(a) * 70 }, 2, c);
        }
        DrawCircleLines((int)B.x, (int)B.y, 35, c);
        DrawCircleLines((int)B.x, (int)B.y, 60, c);
    }
}

void nuvem(int e, Vector2 A, Vector2 B, float t) {
    float agora = (float)GetTime();
    Color c1 = { 210, 200, 255, 255 }, c2 = { 170, 220, 255, 255 };
    if (e == VENENO) { c1 = { 190, 100, 230, 255 }; c2 = { 140, 60, 180, 255 }; }
    float some = 1 - fase(t, 0.7f, 1.0f);
    for (int k = 0; k < 40; k++) {
        float atraso = (k % 10) * 0.03f, s = fase(t, 0.08f + atraso, 0.5f + atraso);
        if (s <= 0) continue;
        Vector2 p = entre(A, B, s);
        p.x += std::sin(k * 1.7f + agora * 3) * (20 + 40 * s);
        p.y += std::cos(k * 2.3f + agora * 2.4f) * (20 + 50 * s) - 30;
        DrawCircleV(p, 3.0f + (k % 3) + (e == VENENO ? 3 : 0), comAlfa(k % 2 ? c1 : c2, some * (e == VENENO ? 0.7f : 1)));
    }
}

void grito(int e, Vector2 A, Vector2 B, float t) {
    Color c = (e == TREVAS) ? Color{ 150, 110, 210, 255 } : Color{ 230, 230, 240, 255 };
    for (int k = 0; k < 3; k++) {
        float f = fase(t, 0.08f + k * 0.08f, 0.5f + k * 0.08f);
        if (f <= 0 || f >= 1) continue;
        Vector2 p = entre(A, B, f * 0.85f);
        float r = 30 + f * 40;
        float meio = (B.x > A.x) ? 0 : 180;
        DrawRing(p, r, r + 6, meio - 50, meio + 50, 24, comAlfa(c, 1 - f * 0.7f));
    }
}

void espiral(Vector2 A, Vector2 B, float t) {
    float agora = (float)GetTime();
    for (int k = 0; k < 4; k++) {
        float f = fase(t, 0.06f + k * 0.08f, 0.5f + k * 0.08f);
        if (f <= 0 || f >= 1) continue;
        Vector2 p = entre(A, B, f);
        for (int r = 0; r < 3; r++)
            DrawRing(p, 10.0f + r * 12, 14.0f + r * 12, agora * 400 + r * 90, agora * 400 + r * 90 + 220, 24,
                     comAlfa({ 230, 130, 230, 255 }, 1 - f * 0.5f));
    }
}

void relampago(float t) {
    float some = 1 - fase(t, 0.25f, 0.4f);
    if (t > 0.15f && some > 0) DrawRectangle(0, 0, Ui::LARGURA, Ui::ALTURA, { 20, 20, 40, (unsigned char)(110 * some) });
}

// Bolinhas de vida voltando do alvo para quem bateu (golpes que drenam).
void dreno(Vector2 A, Vector2 B, float t) {
    for (int k = 0; k < 7; k++) {
        float f = fase(t, 0.45f + k * 0.04f, 0.8f + k * 0.04f);
        if (f <= 0 || f >= 1) continue;
        Vector2 p = entre(B, A, f);
        p.y -= std::sin(f * PI_F) * (60 + k * 8);
        DrawCircleGradient((int)p.x, (int)p.y, 16, { 130, 230, 140, 160 }, { 130, 230, 140, 0 });
        DrawCircleV(p, 5, { 190, 255, 190, 255 });
    }
}
}  // namespace

int formaDoGolpe(int g) {
    switch (g) {
        case G_ARRANHAO: case G_LAMINA_VENENOSA: case G_GARRA_DAS_TREVAS: case G_GOLPE_SOMBRIO: case G_TOQUE_SOMBRIO:
            return F_CORTE;
        case G_MORDIDA: case G_SUGAR_SANGUE:
            return F_MORDIDA;
        case G_TOCHA: case G_BRASA: case G_INFERNO: case G_PEDRADA: case G_PEDREGULHO: case G_AVALANCHE:
        case G_RAJADA_DE_GELO: case G_SOPRO_FRIO: case G_ZERO_ABSOLUTO: case G_FOLHA_NAVALHA: case G_ESPINHOS:
        case G_SEMENTE_SANGUESSUGA: case G_FERRAO_VENENOSO: case G_ACIDO: case G_ACIDO_CORROSIVO: case G_PESTE:
        case G_BOLHA:
            return F_PROJETIL;
        case G_LABAREDA: case G_MURALHA_DE_FOGO: case G_NEVASCA: case G_ONDA: case G_MAREMOTO:
        case G_TEMPESTADE_DE_FOLHAS:
            return F_COLUNA;
        case G_JATO_DAGUA: case G_RAIO_DE_LUZ: case G_CLARAO: case G_JULGAMENTO: case G_LAMPEJO:
        case G_PULSO_DAS_TREVAS: case G_ECLIPSE:
            return F_FEIXE;
        case G_CHICOTE_DE_VINHA: case G_CHICOTE_DAGUA: case G_LINGUA_GRUDENTA: case G_TEIA:
            return F_CHICOTE;
        case G_PO_DO_SONO: case G_ESPOROS: case G_NUVEM_TOXICA:
            return F_NUVEM;
        case G_CHOQUE: case G_FAISCA: case G_TROVAO: case G_TEMPESTADE: case G_PASSO_DO_RAIO:
            return F_RELAMPAGO;
        case G_ROSNADO: case G_RUGIDO: case G_PAVOR:
            return F_GRITO;
        case G_HIPNOSE:
            return F_ESPIRAL;
        default:
            return F_IMPACTO;   // Golpe, Golpe Forte, Investida, Esmagar, Terremoto, Fúria...
    }
}

float momentoDoAcerto(int forma) {
    switch (forma) {
        case F_PROJETIL: return 0.45f;
        case F_COLUNA:   return 0.4f;
        case F_FEIXE:    return 0.25f;
        case F_CHICOTE:  return 0.3f;
        case F_NUVEM:    return 0.5f;
        case F_GRITO:    return 0.45f;
        case F_RELAMPAGO: return 0.2f;
        case F_ESPIRAL:  return 0.5f;
        case F_MORDIDA:  return 0.32f;
        default:         return 0.25f;
    }
}

bool encosta(int forma) { return forma == F_IMPACTO || forma == F_CORTE || forma == F_MORDIDA; }

double duracaoDaForma(int forma) { return encosta(forma) ? 0.55 : 0.8; }

void desenharGolpe(int golpe, Vector2 A, Vector2 B, float t, float metade) {
    if (golpe < 0 || golpe >= NUM_GOLPES) return;
    int e = GOLPES[golpe].elemento;
    Color c = corDoElemento(e);
    switch (formaDoGolpe(golpe)) {
        case F_IMPACTO:   ondaDeChoque(B, t, clarear(c, 0.3f)); break;
        case F_CORTE:     corte(B, t, e == NORMAL ? Color{ 230, 230, 240, 255 } : c); break;
        case F_MORDIDA:   mordida(B, t); break;
        case F_PROJETIL:  projetil(golpe, e, A, B, t); break;
        case F_COLUNA:    coluna(e, B, t, metade); break;
        case F_FEIXE:     feixe(e, A, B, t); break;
        case F_CHICOTE:   chicote(golpe, e, A, B, t); break;
        case F_NUVEM:     nuvem(e, A, B, t); break;
        case F_RELAMPAGO: relampago(t); break;
        case F_GRITO:     grito(e, A, B, t); break;
        case F_ESPIRAL:   espiral(A, B, t); break;
    }
    if (GOLPES[golpe].efeito == DRENAR) dreno(A, B, t);
}

void desenharEstado(int status, bool semente, Vector2 c, float metade) {
    float agora = (float)GetTime();
    float topo = c.y - metade;
    if (status == QUEIMADO) {
        for (int k = 0; k < 3; k++) {
            float balanco = std::sin(agora * 13 + k * 2.3f) * 6;
            float altura = (30 + (k == 1) * 12) * (1 + std::sin(agora * 17 + k) * 0.15f) * metade / 100;
            chama(c.x - metade * 0.5f + k * metade * 0.5f, c.y + metade - 6, altura, balanco,
                  { 255, 120, 40, 255 }, { 255, 170, 60, 255 }, { 255, 230, 120, 255 }, 0.9f);
        }
        for (int k = 0; k < 2; k++) {
            float f = std::fmod(agora * 0.7f + k * 0.5f, 1.0f);
            DrawCircleV({ c.x - 20 + k * 40 + std::sin(f * 5) * 6, topo + 20 - f * 40 }, 6 + f * 6,
                        { 90, 90, 100, (unsigned char)(120 * (1 - f)) });
        }
    } else if (status == CONGELADO) {
        Rectangle r = { c.x - metade - 10, c.y - metade - 10, metade * 2 + 20, metade * 2 + 20 };
        DrawRectangleRounded(r, 0.15f, 6, { 150, 220, 255, 75 });
        DrawRectangleRoundedLinesEx(r, 0.15f, 6, 2.5f, { 200, 240, 255, 210 });
        DrawLineEx({ r.x + 14, r.y + 18 }, { r.x + 40, r.y + 10 }, 3, { 235, 250, 255, 210 });
        DrawLineEx({ r.x + 10, r.y + 32 }, { r.x + 20, r.y + 14 }, 3, { 235, 250, 255, 170 });
        float brilho = std::fmod(agora * 0.5f, 1.0f);
        DrawCircleV({ r.x + r.width * brilho, r.y + 8 }, 2, { 255, 255, 255, (unsigned char)(200 * (1 - brilho)) });
    } else if (status == OFUSCADO) {
        DrawCircleGradient((int)c.x, (int)(topo + metade * 0.5f), metade * 0.6f, { 255, 250, 220, 90 }, { 255, 250, 220, 0 });
        for (int k = 0; k < 3; k++) {
            float a = agora * 3 + k * 2.09f;
            DrawPoly({ c.x + std::cos(a) * metade * 0.45f, topo + 8 + std::sin(a) * 12 }, 4, 7, agora * 200, { 255, 245, 190, 240 });
        }
    } else if (status == ATORDOADO) {
        for (int k = 0; k < 3; k++) {
            float a = agora * 4 + k * 2.09f;
            Vector2 p = { c.x + std::cos(a) * metade * 0.4f, topo - 4 + std::sin(a) * 9 };
            DrawPoly(p, 5, 7, agora * 120, { 255, 220, 90, 240 });
        }
    } else if (status == PRESO) {
        Color w = { 225, 230, 240, 170 };
        for (int k = -2; k <= 2; k++) {
            DrawLineEx({ c.x - metade, c.y + k * metade * 0.35f }, { c.x + metade, c.y - k * metade * 0.3f }, 2, w);
            DrawLineEx({ c.x + k * metade * 0.35f, c.y - metade }, { c.x - k * metade * 0.3f, c.y + metade }, 2, w);
        }
    }
    if (semente) {   // brotinho na cabeça
        float b = std::sin(agora * 3) * 3;
        DrawLineEx({ c.x - metade * 0.4f, topo + 6 }, { c.x - metade * 0.4f, topo - 14 }, 3, { 80, 160, 70, 255 });
        DrawEllipse((int)(c.x - metade * 0.4f - 8), (int)(topo - 14 + b), 9, 4, { 110, 210, 100, 255 });
        DrawEllipse((int)(c.x - metade * 0.4f + 8), (int)(topo - 18 - b), 9, 4, { 110, 210, 100, 255 });
    }
}

}  // namespace GolpesVisuais
