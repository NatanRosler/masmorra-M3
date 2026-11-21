// golpes_visuais.h
// -----------------------------------------------------------------------------
// v1.23: o desenho de cada TIPO de golpe na batalha (corte, mordida, bola de
// fogo, feixe, chicote...) e dos estados que ficam no lutador (queimado,
// congelado, ofuscado...). A lógica (batalha.cpp) só avisa qual golpe foi
// usado; tela_grafica.cpp chama estas funções em cada quadro da animação.
// -----------------------------------------------------------------------------
#pragma once
#include "raylib.h"

namespace GolpesVisuais {

enum Forma {
    F_IMPACTO,    // onda de choque (Golpe, Investida, Esmagar...)
    F_CORTE,      // três riscos (Arranhão, Lâmina Venenosa, Garra das Trevas...)
    F_MORDIDA,    // mandíbulas fechando
    F_PROJETIL,   // algo voando até o alvo (bola de fogo, pedra, estilhaços, folhas...)
    F_COLUNA,     // sobe do chão embaixo do alvo (Labareda, Nevasca, Onda...)
    F_FEIXE,      // linha contínua até o alvo (Jato d'Água, Raio de Luz...)
    F_CHICOTE,    // estica, acerta e volta (Chicote de Vinha, Língua Grudenta, Teia)
    F_NUVEM,      // pó ou gás voando até o alvo (Pó do Sono, Nuvem Tóxica...)
    F_RELAMPAGO,  // raio caindo do alto (Choque, Trovão...)
    F_GRITO,      // ondas de som (Rosnado, Rugido, Pavor)
    F_ESPIRAL     // espirais (Hipnose)
};

int formaDoGolpe(int golpe);
// Fração da animação (0 a 1) em que o golpe chega no alvo.
float momentoDoAcerto(int forma);
// O atacante dá o bote até o alvo (golpes de contato)?
bool encosta(int forma);
// Quanto dura a animação de um golpe desta forma (segundos, antes das Opções).
double duracaoDaForma(int forma);

// Desenha o golpe no instante t (0 a 1). A = quem ataca, B = o alvo,
// metade = meia altura do desenho do alvo.
void desenharGolpe(int golpe, Vector2 A, Vector2 B, float t, float metade);

// Desenho do estado que fica no lutador (c = centro do desenho).
void desenharEstado(int status, bool semente, Vector2 c, float metade);

}  // namespace GolpesVisuais
