// icones_elementos.h
// -----------------------------------------------------------------------------
// v1.23: o símbolo de cada elemento (desenho 9x9), usado na batalha, no
// bestiário, na ficha e na criação. K = contorno, X = cor do elemento,
// h = mais clara, d = mais escura, Y = amarelo (miolo do fogo).
// Normal: estrela; Fogo: chama; Água: gota; Planta: folha; Veneno: caveira;
// Pedra: pedra; Gelo: floco de neve; Raio: raio; Trevas: lua; Luz: sol.
// -----------------------------------------------------------------------------
#pragma once
#include "raylib.h"
#include "monstros.h"
#include "batalha.h"
#include <cmath>

inline const char* ICONES_DOS_ELEMENTOS[NUM_ELEMENTOS][9] = {
 {"....K....","...KXK...","KKKKXKKKK","KXXXhXXXK",".KXXXXXK.","..KXXXK..",".KXXKXXK.",".KXK.KXK.",".KK...KK."},
 {"....K....","...KXK...","..KXXK.K.",".KXXhXKXK",".KXhhhXXK","KXXhYhXXK","KXXhYYhXK",".KXXhhXK.","..KKKKK.."},
 {"....K....","...KXK...","...KXK...","..KXXXK..",".KXhXXXK.",".KXhXXXK.",".KXXXXXK.","..KXXXK..","...KKK..."},
 {"......KKK","....KKXXK","...KXXXhK","..KXXXhXK",".KXXXhXK.",".KXXhXXK.",".KXhXKK..","KKhKK....","KK......."},
 {"..KKKKK..",".KXXXXXK.","KXXXXXXXK","KXKKXKKXK","KXKKXKKXK","KXXXKXXXK",".KXXXXXK.","..KXKXK..","..KKKKK.."},
 {"...KKKK..","..KXXhXK.",".KXXXXhK.","KXXdXXXXK","KXXXXdXXK","KXdXXXXXK",".KXXXXXK.","..KKKKK..","........."},
 {"....K....",".K.KXK.K.","KXK.X.KXK",".KXKXKXK.","KXXXhXXXK",".KXKXKXK.","KXK.X.KXK",".K.KXK.K.","....K...."},
 {"....KKKK.","...KXXK..","..KXXK...",".KXXXKKK.","KXXXXXXK.",".KKKXXK..","...KXK...","..KXK....","..KK....."},
 {"...KKKK..","..KXXXK..",".KXXK....","KXXK.....","KXXK.....","KXXK.....",".KXXK...K","..KXXXXK.","...KKKK.."},
 {"....K....",".K..X..K.","..KKKKK..","..KXXXK..","KXKXhXKXK","..KXXXK..","..KKKKK..",".K..X..K.","....K...."},
};
inline void iconeDoElemento(int e, float x, float y, float px, float alfa = 1.0f) {
    CorRGB rgb = CORES_DOS_ELEMENTOS[e];
    unsigned char a = (unsigned char)(255 * alfa);
    Color c = { (unsigned char)rgb.r, (unsigned char)rgb.g, (unsigned char)rgb.b, a };
    Color clara = { (unsigned char)(c.r + (255 - c.r) * 0.45f), (unsigned char)(c.g + (255 - c.g) * 0.45f),
                    (unsigned char)(c.b + (255 - c.b) * 0.45f), a };
    Color escura = { (unsigned char)(c.r * 0.6f), (unsigned char)(c.g * 0.6f), (unsigned char)(c.b * 0.6f), a };
    for (int l = 0; l < 9; l++) {
        for (int k = 0; k < 9; k++) {
            Color cor;
            switch (ICONES_DOS_ELEMENTOS[e][l][k]) {
                case 'K': cor = { 15, 22, 36, a }; break;
                case 'X': cor = c; break;
                case 'h': cor = clara; break;
                case 'd': cor = escura; break;
                case 'Y': cor = { 255, 240, 150, a }; break;
                default: continue;
            }
            DrawRectangle((int)(x + k * px), (int)(y + l * px), (int)std::ceil(px), (int)std::ceil(px), cor);
        }
    }
}
