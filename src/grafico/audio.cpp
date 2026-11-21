// audio.cpp
// -----------------------------------------------------------------------------
// Sons e músicas GERADOS NO CÓDIGO (nenhum arquivo de áudio), no estilo dos
// videogames de 8 bits:
//   - onda QUADRADA: o "bip" clássico (melodias, golpes)
//   - onda TRIANGULAR: som mais macio (baixo, cura)
//   - RUÍDO: números aleatórios (pancadas, espinhos)
//
// Cada efeito é uma lista de notas (frequência, duração, onda e volume). A
// música é tocada por um "sequenciador": a cada passo ele lê a nota da melodia
// e a do baixo e gera as amostras na hora, dentro do callback da raylib.
// -----------------------------------------------------------------------------
#include "som.h"
#include "audio.h"
#include "opcoes.h"
#include "raylib.h"
#include "sintese.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <vector>

namespace {
using Sintese::TAXA;               // 22050 amostras por segundo
const float PI2 = 6.2831853f;

enum Onda { QUADRADA, QUADRADA_FINA, TRIANGULAR, RUIDO, SENOIDE };

struct Nota {
    float frequencia;     // Hz (0 = silêncio)
    float frequenciaFim;  // para "deslizar" (0 = igual à inicial)
    int ms;
    Onda onda;
    float volume;         // 0 a 1
};

bool audioPronto = false;
bool ligado = true;
Sound sons[NUM_SONS];
AudioStream fluxoDaMusica;

// Uma amostra (-1 a 1) da onda na fase dada (0 a 1).
float amostraDaOnda(Onda onda, float fase, uint32_t& ruido) {
    switch (onda) {
        case QUADRADA:      return fase < 0.5f ? 1.0f : -1.0f;
        case QUADRADA_FINA: return fase < 0.25f ? 1.0f : -1.0f;
        case TRIANGULAR:    return fase < 0.5f ? 4.0f * fase - 1.0f : 3.0f - 4.0f * fase;
        case SENOIDE:       return std::sin(PI2 * fase);
        case RUIDO:
            ruido ^= ruido << 13; ruido ^= ruido >> 17; ruido ^= ruido << 5;   // xorshift
            return (float)(ruido & 0xFFFF) / 32768.0f - 1.0f;
    }
    return 0.0f;
}

// Monta um som a partir das notas, uma depois da outra.
Sound gerarSom(const std::vector<Nota>& notas) {
    int total = 0;
    for (const Nota& n : notas) total += n.ms * TAXA / 1000;
    short* dados = (short*)MemAlloc(total * sizeof(short));
    int i = 0;
    uint32_t ruido = 12345;
    float fase = 0.0f;
    for (const Nota& n : notas) {
        int amostras = n.ms * TAXA / 1000;
        float fim = n.frequenciaFim > 0 ? n.frequenciaFim : n.frequencia;
        for (int k = 0; k < amostras; k++) {
            float t = (float)k / amostras;                       // 0 a 1 dentro da nota
            float freq = n.frequencia + (fim - n.frequencia) * t;
            // Envelope: entra rápido e vai sumindo, para não dar "estalo".
            float envelope = (k < 40 ? k / 40.0f : 1.0f) * (1.0f - 0.7f * t);
            if (amostras - k < 60) envelope *= (amostras - k) / 60.0f;
            float valor = 0.0f;
            if (n.frequencia > 0) {
                fase += freq / TAXA;
                fase -= std::floor(fase);
                valor = amostraDaOnda(n.onda, fase, ruido) * n.volume * envelope;
            }
            dados[i++] = (short)(valor * 20000);
        }
    }
    Wave onda = { (unsigned int)total, (unsigned int)TAXA, 16, 1, dados };
    Sound som = LoadSoundFromWave(onda);
    UnloadWave(onda);
    return som;
}

// ---- Vozes -------------------------------------------------------------------
// Um "bip" de voz tem mais enfeites que as notas dos efeitos: vibrato (o tom
// balança), tremido (o volume balança), um pouco de ruído (voz rouca), uma
// segunda onda levemente desafinada (som "grosso") e eco.
struct Bip {
    float frequencia, frequenciaFim;
    int ms;
    Onda onda;
    float volume;
    float vibratoHz = 0, vibrato = 0;     // vibrato: velocidade e quanto o tom varia (0.05 = 5%)
    float tremidoHz = 0, tremido = 0;     // tremido do volume
    float ruido = 0;                      // 0 a 1: quanto de ruído misturado
    float desafino = 0;                   // 0 = sem segunda onda; 1.06 = segunda onda 6% mais aguda
    int ecoMs = 0, ecos = 0;              // eco: atraso e quantas repetições
    float ecoGanho = 0;                   // volume de cada repetição
};

// Monta um som com uma sequência de bips (com "pausaMs" entre eles).
Sound gerarBips(const std::vector<Bip>& bips, int pausaMs = 0) {
    std::vector<float> s;
    uint32_t ruido = 777;
    for (const Bip& b : bips) {
        int n = b.ms * TAXA / 1000;
        float fim = b.frequenciaFim > 0 ? b.frequenciaFim : b.frequencia;
        float fase = 0, fase2 = 0;
        std::vector<float> bip(n);
        for (int k = 0; k < n; k++) {
            float t = (float)k / n, seg = (float)k / TAXA;
            float f = b.frequencia + (fim - b.frequencia) * t;
            if (b.vibrato > 0) f *= 1.0f + b.vibrato * std::sin(PI2 * b.vibratoHz * seg);
            fase += f / TAXA; fase -= std::floor(fase);
            float v = amostraDaOnda(b.onda, fase, ruido);
            if (b.desafino > 0) {
                fase2 += f * b.desafino / TAXA; fase2 -= std::floor(fase2);
                v = 0.6f * v + 0.6f * amostraDaOnda(b.onda, fase2, ruido);
            }
            if (b.ruido > 0) v = (1 - b.ruido) * v + b.ruido * amostraDaOnda(RUIDO, 0, ruido);
            float envelope = (k < 40 ? k / 40.0f : 1.0f) * (1.0f - 0.7f * t);
            if (n - k < 60) envelope *= (n - k) / 60.0f;
            if (b.tremido > 0) envelope *= 1.0f - b.tremido * (0.5f + 0.5f * std::sin(PI2 * b.tremidoHz * seg));
            bip[k] = v * envelope * b.volume;
        }
        // Eco: o mesmo bip repetido mais baixo, um pouco depois.
        int atraso = b.ecoMs * TAXA / 1000;
        std::vector<float> comEco(n + atraso * b.ecos, 0.0f);
        float ganho = 1.0f;
        for (int e = 0; e <= b.ecos; e++, ganho *= b.ecoGanho)
            for (int k = 0; k < n; k++) comEco[e * atraso + k] += bip[k] * ganho;
        s.insert(s.end(), comEco.begin(), comEco.end());
        s.insert(s.end(), pausaMs * TAXA / 1000, 0.0f);
    }
    short* dados = (short*)MemAlloc((unsigned int)(s.size() * sizeof(short)));
    for (size_t i = 0; i < s.size(); i++) dados[i] = (short)(std::clamp(s[i], -1.6f, 1.6f) * 20000);
    Wave onda = { (unsigned int)s.size(), (unsigned int)TAXA, 16, 1, dados };
    Sound som = LoadSoundFromWave(onda);
    UnloadWave(onda);
    return som;
}

// Cada voz: até 2 bips diferentes (o Garro e o macaco alternam), a cada
// quantas letras toca e quanto o tom varia de um bip para o outro. Cada bip
// tem 3 "cópias" (aliases), para um eco longo não ser cortado pelo próximo.
struct VozPronta {
    Sound bip[2][3];
    int variantes = 1;
    int cada = 2;
    float variacao = 0.06f;
};
VozPronta vozes[NUM_VOZES];

void criarVoz(Voz v, int cada, float variacao, const Bip& a) {
    vozes[v].bip[0][0] = gerarBips({a});
    for (int c = 1; c < 3; c++) vozes[v].bip[0][c] = LoadSoundAlias(vozes[v].bip[0][0]);
    vozes[v].variantes = 1; vozes[v].cada = cada; vozes[v].variacao = variacao;
}
void criarVoz(Voz v, int cada, float variacao, const Bip& a, const Bip& b) {
    criarVoz(v, cada, variacao, a);
    vozes[v].bip[1][0] = gerarBips({b});
    for (int c = 1; c < 3; c++) vozes[v].bip[1][c] = LoadSoundAlias(vozes[v].bip[1][0]);
    vozes[v].variantes = 2;
}

void criarVozes() {
    Bip b;
    //                 freq  fim   ms  onda           vol
    criarVoz(VOZ_NARRADOR, 3, 0.04f, {210,   0, 55, TRIANGULAR,    0.30f});
    criarVoz(VOZ_HEROI,    2, 0.06f, {330, 320, 34, TRIANGULAR,    0.30f});
    criarVoz(VOZ_ELDRA,    2, 0.08f, {640,   0, 28, QUADRADA_FINA, 0.16f});
    b = {140, 125, 58, QUADRADA, 0.17f}; b.ruido = 0.18f;                 // rouco
    criarVoz(VOZ_BORIN, 2, 0.05f, b);
    b = {300, 0, 70, TRIANGULAR, 0.30f}; b.vibratoHz = 7; b.vibrato = 0.03f;
    b.ecoMs = 90; b.ecos = 3; b.ecoGanho = 0.45f;                         // fantasma
    criarVoz(VOZ_ALDRIC, 2, 0.03f, b);
    criarVoz(VOZ_GARRO, 2, 0.03f, {470, 0, 30, QUADRADA, 0.15f}, {390, 0, 30, QUADRADA, 0.15f});
    b = {520, 0, 40, TRIANGULAR, 0.28f}; b.tremidoHz = 28; b.tremido = 0.6f;   // nervoso
    criarVoz(VOZ_TOBIAS, 2, 0.10f, b);
    // Monstros (gritos curtos, uma letra por bip).
    criarVoz(VOZ_RATO,    1, 0.06f, {1300, 1700, 35, QUADRADA_FINA, 0.12f});
    criarVoz(VOZ_KOBOLD,  1, 0.06f, {760, 560, 40, QUADRADA, 0.13f});
    b = {260, 200, 60, QUADRADA, 0.15f}; b.ruido = 0.35f;
    criarVoz(VOZ_CHACAL, 1, 0.06f, b);
    criarVoz(VOZ_MACACO,  1, 0.03f, {820, 0, 35, TRIANGULAR, 0.25f}, {1040, 0, 35, TRIANGULAR, 0.25f});
    criarVoz(VOZ_SAPO,    1, 0.06f, {170, 110, 80, QUADRADA, 0.15f});
    b = {520, 0, 40, QUADRADA_FINA, 0.13f}; b.vibratoHz = 30; b.vibrato = 0.08f;
    criarVoz(VOZ_GOBLIN, 1, 0.06f, b);
    criarVoz(VOZ_MORCEGO, 1, 0.06f, {2100, 2600, 18, QUADRADA_FINA, 0.10f});
    b = {85, 70, 85, QUADRADA, 0.20f}; b.ruido = 0.25f;
    criarVoz(VOZ_OGRO, 1, 0.04f, b);
    b = {330, 0, 70, TRIANGULAR, 0.18f}; b.ruido = 0.55f; b.vibratoHz = 5; b.vibrato = 0.05f;   // sibilo
    criarVoz(VOZ_NAGA, 1, 0.06f, b);
    criarVoz(VOZ_GNOMO,   1, 0.06f, {980, 1100, 30, QUADRADA_FINA, 0.12f});
    b = {250, 0, 70, TRIANGULAR, 0.14f}; b.ruido = 0.85f; b.ecoMs = 120; b.ecos = 2; b.ecoGanho = 0.5f;   // sussurro
    criarVoz(VOZ_SOMBRA, 1, 0.06f, b);
    criarVoz(VOZ_MIMICO,  1, 0.06f, {160, 120, 50, QUADRADA, 0.16f});
    // O chefe: duas ondas muito graves e desafinadas, rouco e com eco longo.
    b = {52, 46, 120, QUADRADA, 0.24f}; b.desafino = 1.06f; b.ruido = 0.12f;
    b.ecoMs = 160; b.ecos = 3; b.ecoGanho = 0.5f;
    criarVoz(VOZ_CHEFE, 1, 0.03f, b);
    // v1.23
    b = {1500, 1100, 22, QUADRADA_FINA, 0.10f}; b.ruido = 0.5f;           // aranha: chiado de patinhas
    criarVoz(VOZ_ARANHA, 1, 0.06f, b);
    criarVoz(VOZ_FUNGO, 1, 0.08f, {210, 260, 70, TRIANGULAR, 0.25f});      // fungo: "bloop" mole
    b = {420, 380, 30, QUADRADA, 0.14f}; b.tremidoHz = 40; b.tremido = 0.7f;   // esqueleto: ossos batendo
    criarVoz(VOZ_ESQUELETO, 1, 0.05f, b);
    b = {700, 500, 45, TRIANGULAR, 0.16f}; b.ruido = 0.45f;               // salamandra: sibilo quente
    criarVoz(VOZ_SALAMANDRA, 1, 0.06f, b);
}

// ---- Efeitos com "instrumentos" (v1.21) ---------------------------------------
using namespace Sintese;
Sound somDeAmostras(Amostras x, float pico = 0.75f) {
    normalizar(x, pico);
    short* dados = (short*)MemAlloc((unsigned int)(x.size() * sizeof(short)));
    // Mesma escala dos efeitos de 8 bits (gerarSom), para ficarem no mesmo volume.
    for (size_t i = 0; i < x.size(); i++) dados[i] = (short)(std::clamp(x[i], -1.0f, 1.0f) * 20000);
    Wave onda = { (unsigned int)x.size(), (unsigned int)TAXA, 16, 1, dados };
    Sound som = LoadSoundFromWave(onda);
    UnloadWave(onda);
    return som;
}
// Vários sons começando um pouco depois do outro (arpejo de harpa, de sinos...).
Amostras arpejo(const std::vector<float>& notas, float intervalo, bool deHarpa, float duracao, float volume) {
    Amostras x;
    for (size_t i = 0; i < notas.size(); i++)
        misturar(x, deHarpa ? corda(notas[i], duracao, volume, 0.997f) : sino(notas[i], duracao, volume, 2.0f, 1.5f, 3.0f),
                 i * intervalo);
    return x;
}
Amostras passosCorrendo(int quantos, float intervalo, float volume) {
    Amostras x;
    for (int i = 0; i < quantos; i++) {
        Amostras p = passaBaixa(ruido(0.06f), 900);
        p = envelope(p, 0, 60);
        misturar(p, envelope(sino(90, 0.06f, 1.0f, 1.0f, 0.0f, 50), 0, 0), 0, 0.6f);
        misturar(x, p, i * intervalo, volume);
    }
    return x;
}

void criarEfeitosComInstrumentos() {
    sons[SOM_MOEDAS] = somDeAmostras(reverb(juntar({ sino(2093, 0.12f, 0.3f, 2.0f, 1.2f, 6), sino(2637, 0.6f, 0.3f, 2.0f, 1.2f, 4) }), 0.8f, 0.25f), 0.55f);
    {
        Amostras x = arpejo({ 523, 659, 784, 1047, 1319, 1568 }, 0.08f, true, 1.4f, 0.35f);
        misturar(x, sino(2093, 1.5f, 0.15f, 2.0f, 1.5f, 2.0f), 0.5f);
        sons[SOM_CRISTAL] = somDeAmostras(reverb(x, 1.2f, 0.4f));
    }
    {
        Amostras x = envelope(passaAlta(ruido(0.25f), 2500), 0, 14);
        misturar(x, sino(1800, 1.0f, 0.7f, 2.7f, 3.0f, 4.0f), 0.05f);
        sons[SOM_EQUIPAR_ARMA] = somDeAmostras(reverb(x, 0.8f, 0.3f), 0.6f);
    }
    {
        Amostras x = envelope(passaBaixa(ruido(0.1f), 1500), 0, 30);
        misturar(x, sino(320, 0.8f, 0.8f, 1.48f, 2.5f, 6.0f), 0);
        sons[SOM_EQUIPAR_ESCUDO] = somDeAmostras(reverb(x, 0.8f, 0.3f), 0.7f);
    }
    {
        Amostras x;
        for (int i = 0; i < 10; i++) misturar(x, bolha(acaso(400, 900), 0.18f), acaso(0, 0.5f));
        misturar(x, sino(1319, 1.2f, 0.18f, 2.0f, 1.5f, 2.5f), 0.35f);
        sons[SOM_ETER] = somDeAmostras(reverb(x, 1.0f, 0.4f), 0.6f);
    }
    sons[SOM_ANTIDOTO] = somDeAmostras(reverb(juntar({ corda(880, 0.25f, 0.4f), corda(1175, 0.9f, 0.4f) }), 1.0f, 0.35f), 0.6f);
    {
        Amostras x = envelope(passaBaixa(ruido(0.12f), 2000), 0, 25);
        misturar(x, envelope(passaBanda(ruido(1.2f), 1500, 6000), 0, 1.8f), 0.05f, 0.5f);
        sons[SOM_FUMACA] = somDeAmostras(reverb(x, 1.0f, 0.3f), 0.7f);
    }
    {
        Amostras papel = passaBanda(ruido(0.35f), 2000, 7000);
        for (size_t i = 0; i < papel.size(); i++) papel[i] *= 0.25f * (0.3f + 0.7f * std::fabs(std::sin(9.0f * i / papel.size())));
        sons[SOM_PERGAMINHO] = somDeAmostras(reverb(juntar({ papel, arpejo({ 659, 784, 988, 1319 }, 0.07f, true, 1.0f, 0.3f) }), 1.0f, 0.35f), 0.6f);
    }
    sons[SOM_ITEM_ESPECIAL] = somDeAmostras(reverb(arpejo({ 523, 659, 784, 1047 }, 0.11f, false, 1.2f, 0.22f), 1.0f, 0.35f), 0.6f);
    {
        Amostras x = passaBaixa(estalo(0.5f), 3000);
        misturar(x, envelope(sino(180, 0.12f, 0.5f, 1.0f, 0.0f, 40), 0, 0), 0);
        sons[SOM_LARGAR] = somDeAmostras(x, 0.5f);
    }
    {
        Amostras x;
        for (int i = 0; i < 9; i++) misturar(x, bolha(acaso(150, 350), 0.3f), acaso(0, 0.6f));
        sons[SOM_VENENO] = somDeAmostras(reverb(x, 1.0f, 0.3f), 0.6f);
    }
    sons[SOM_SONO] = somDeAmostras(reverb(arpejo({ 1568, 1319, 1047, 880, 784, 659 }, 0.09f, true, 1.2f, 0.3f), 1.4f, 0.5f), 0.6f);
    {
        Amostras x = passaBanda(ruido(0.5f), 2000, 6000);
        for (size_t i = 0; i < x.size(); i++) {
            float t = (float)i / TAXA;
            x[i] = x[i] * (std::fabs(std::sin(PI2 * 23 * t / 2)) > 0.7f ? 0.4f : 0.0f) + std::sin(PI2 * 120 * t) * 0.15f * (1 - t / 0.5f);
        }
        sons[SOM_PARALISIA] = somDeAmostras(reverb(x, 0.8f, 0.25f), 0.6f);
    }
    sons[SOM_FUGA] = somDeAmostras(reverb(passosCorrendo(6, 0.13f, 0.5f), 1.2f, 0.3f), 0.6f);
    {
        Amostras x = passosCorrendo(2, 0.14f, 0.5f);
        misturar(x, envelope(passaBaixa(ruido(0.3f), 400), 0, 12), 0.4f, 1.2f);
        sons[SOM_FUGA_FALHOU] = somDeAmostras(reverb(x, 1.2f, 0.3f), 0.6f);
    }
    {
        Amostras x = passaBanda(ruido(0.35f), 800, 3000);
        for (size_t i = 0; i < x.size(); i++) { float s = std::sin(3.14159f * i / x.size()); x[i] *= 0.4f * s * s; }
        sons[SOM_FURTIVO] = somDeAmostras(x, 0.35f);
    }
    {
        Amostras x = envelope(passaBanda(ruido(0.35f), 300, 3000), 0, 9);
        for (int i = 0; i < 5; i++) misturar(x, gota(acaso(700, 1500), 0.25f), acaso(0.05f, 0.3f));
        sons[SOM_AGUA] = somDeAmostras(reverb(x, 1.0f, 0.35f), 0.55f);
    }
    {
        Amostras x = envelope(passaAlta(ruido(0.8f), 2500), 0, 3);
        for (int i = 0; i < 12; i++) misturar(x, estalo(0.3f), acaso(0, 0.5f));
        sons[SOM_LAVA] = somDeAmostras(reverb(x, 0.8f, 0.2f), 0.6f);
    }
    sons[SOM_PONTO] = somDeAmostras(reverb(juntar({ estalo(0.4f), sino(1319, 0.6f, 0.25f, 2.0f, 1.5f, 5) }), 0.8f, 0.25f), 0.5f);
    sons[SOM_DICA] = somDeAmostras(reverb(juntar({ sino(1568, 0.3f, 0.25f, 2.0f, 1.5f, 5), sino(2093, 1.0f, 0.25f, 2.0f, 1.5f, 3) }), 1.0f, 0.4f), 0.45f);
    sons[SOM_MIMICO] = somDeAmostras(reverb(juntar({ envelope(passaBaixa(ruido(0.08f), 2500), 0, 25), silencio(0.05f),
                                                     sino(140, 0.25f, 0.5f, 1.3f, 4, 12), silencio(0.04f),
                                                     sino(120, 0.3f, 0.5f, 1.3f, 4, 12) }), 1.0f, 0.3f));
    {
        Amostras x = sino(70, 0.5f, 0.6f, 1.05f, 0.3f, 4);
        Amostras sussurro = ressonancia(ruido(1.2f), 700, 6);
        Amostras s2 = ressonancia(ruido(1.2f), 1150, 6);
        for (size_t i = 0; i < sussurro.size(); i++) {
            float t = (float)i / TAXA;
            sussurro[i] = (sussurro[i] + s2[i]) * 0.08f * std::sin(3.14159f * t / 1.2f);
        }
        misturar(x, sussurro, 0.3f);
        sons[SOM_ALTAR] = somDeAmostras(reverb(x, 1.5f, 0.5f), 0.7f);
    }
    {
        Amostras x(TAXA * 4);
        for (size_t i = 0; i < x.size(); i++) {
            float t = (float)i / TAXA, e = std::exp(-t * 0.9f);
            x[i] = (std::sin(PI2 * 110 * t + 1.6f * e * std::sin(PI2 * 110 * 1.41f * t)) +
                    0.5f * std::sin(PI2 * 233 * t) * std::exp(-t * 1.5f) + 0.3f * std::sin(PI2 * 391 * t) * std::exp(-t * 2.5f)) * e * 0.4f;
        }
        misturar(x, envelope(ruido(0.02f), 0, 100), 0, 0.5f);
        sons[SOM_GONGO] = somDeAmostras(reverb(x, 1.8f, 0.4f), 0.8f);
    }
    {   // Espinhos: três lâminas de metal saltando quase juntas ("shhk") e a pancada no chão.
        Amostras x;
        for (int i = 0; i < 3; i++) {
            Amostras lamina = envelope(passaAlta(ruido(0.12f), 3500), 0, 30);
            misturar(lamina, sino(acaso(1900, 2600), 0.35f, 0.35f, 2.7f, 2.5f, 9), 0.005f);
            misturar(x, lamina, i * 0.025f, 0.7f);
        }
        misturar(x, baque(70, 0.3f, 0.9f), 0.02f);
        sons[SOM_ESPINHOS] = somDeAmostras(reverb(x, 0.9f, 0.3f), 0.7f);
    }
    // Ponto de atributo: duas notas de harpa subindo e um brilho de sino.
    {
        Amostras x = arpejo({ 784, 1175, 1568 }, 0.09f, true, 0.9f, 0.35f);
        misturar(x, sino(2349, 0.9f, 0.18f, 2.0f, 1.2f, 3.5f), 0.2f);
        sons[SOM_PONTO_GANHO] = somDeAmostras(reverb(x, 1.0f, 0.35f), 0.55f);
    }
    // ---- v1.23: estados novos ----
    {   // Queimado: um sopro de fogo que cresce e uns estalos.
        Amostras x = envelope(passaBanda(ruido(0.6f), 300, 2500), 0.08f, 5);
        for (int i = 0; i < 7; i++) misturar(x, estalo(0.5f), acaso(0.05f, 0.5f));
        sons[SOM_QUEIMADO] = somDeAmostras(reverb(x, 0.8f, 0.25f), 0.6f);
    }
    {   // Congelado: cristais tilintando subindo e um chiado frio.
        Amostras x = envelope(passaAlta(ruido(0.5f), 5000), 0.02f, 6);
        for (size_t i = 0; i < x.size(); i++) x[i] *= 0.3f;
        const float notas[] = { 2093, 2637, 3136, 3951, 4186 };
        for (int i = 0; i < 5; i++) misturar(x, sino(notas[i], 0.5f, 0.22f, 3.5f, 2.0f, 6), i * 0.05f);
        sons[SOM_CONGELADO] = somDeAmostras(reverb(x, 1.2f, 0.4f), 0.6f);
    }
    {   // Degelo: o gelo racha e pinga.
        Amostras x = envelope(passaAlta(ruido(0.08f), 2500), 0, 40);
        for (int i = 0; i < 4; i++) misturar(x, gota(acaso(1400, 2200), 0.5f), 0.12f + i * acaso(0.08f, 0.14f));
        sons[SOM_DEGELO] = somDeAmostras(reverb(x, 1.0f, 0.3f), 0.55f);
    }
    {   // Ofuscado: um brilho de sinos agudos que some.
        Amostras x;
        const float notas[] = { 2637, 3520, 4186, 3520 };
        for (int i = 0; i < 4; i++) misturar(x, sino(notas[i], 0.7f, 0.2f, 2.0f, 0.8f, 4), i * 0.04f);
        sons[SOM_OFUSCADO] = somDeAmostras(reverb(x, 1.4f, 0.45f), 0.55f);
    }
    {   // Atordoado: a pancada e três "piu" girando.
        Amostras x = baque(110, 0.25f, 0.8f);
        for (int i = 0; i < 3; i++) misturar(x, sino(1800 + i * 300, 0.18f, 0.25f, 1.0f, 0.3f, 10), 0.15f + i * 0.12f);
        sons[SOM_ATORDOADO] = somDeAmostras(reverb(x, 0.9f, 0.3f), 0.55f);
    }
    {   // Folhas: farfalhar (ruído em rajadas curtas).
        Amostras x;
        for (int i = 0; i < 6; i++) misturar(x, envelope(passaBanda(ruido(0.12f), 2500, 7000), 0.01f, 25), i * 0.07f, 0.6f);
        sons[SOM_FOLHAS] = somDeAmostras(reverb(x, 0.7f, 0.2f), 0.5f);
    }
    {   // Ossos se juntando: vários "toc" secos acelerando e um sino grave.
        Amostras x;
        float t = 0;
        for (int i = 0; i < 9; i++) {
            misturar(x, sino(acaso(600, 1000), 0.08f, 0.3f, 3.1f, 2.0f, 40), t);
            t += 0.11f - i * 0.009f;
        }
        misturar(x, sino(110, 1.2f, 0.25f, 1.4f, 2.5f, 2.5f), t);
        sons[SOM_REMONTA] = somDeAmostras(reverb(x, 1.3f, 0.4f), 0.6f);
    }
}

void criarEfeitos() {
    sons[SOM_MENU]         = gerarSom({ {880, 0, 40, QUADRADA_FINA, 0.25f} });
    sons[SOM_GOLPE]        = gerarSom({ {900, 200, 60, RUIDO, 0.5f}, {220, 110, 70, QUADRADA, 0.35f} });
    sons[SOM_CRITICO]      = gerarSom({ {1400, 300, 60, RUIDO, 0.55f}, {660, 0, 80, QUADRADA, 0.35f}, {990, 0, 140, QUADRADA, 0.35f} });
    sons[SOM_SUPER_EFICAZ] = gerarSom({ {523, 0, 50, QUADRADA, 0.3f}, {659, 0, 50, QUADRADA, 0.3f},
                                        {784, 0, 50, QUADRADA, 0.3f}, {1047, 0, 120, QUADRADA, 0.3f} });
    sons[SOM_ERROU]        = gerarSom({ {420, 180, 170, TRIANGULAR, 0.5f} });
    sons[SOM_DANO]         = gerarSom({ {500, 0, 40, RUIDO, 0.4f}, {320, 70, 180, QUADRADA, 0.4f} });
    sons[SOM_DESMAIO]      = gerarSom({ {660, 90, 420, TRIANGULAR, 0.55f} });
    sons[SOM_NIVEL]        = gerarSom({ {523, 0, 80, QUADRADA, 0.3f}, {659, 0, 80, QUADRADA, 0.3f}, {784, 0, 80, QUADRADA, 0.3f},
                                        {1047, 0, 80, QUADRADA, 0.3f}, {0, 0, 40, QUADRADA, 0}, {1047, 0, 320, QUADRADA, 0.3f} });
    sons[SOM_ITEM]         = gerarSom({ {988, 0, 60, QUADRADA_FINA, 0.3f}, {1319, 0, 160, QUADRADA_FINA, 0.3f} });
    sons[SOM_CURA]         = gerarSom({ {523, 0, 90, TRIANGULAR, 0.5f}, {659, 0, 90, TRIANGULAR, 0.5f}, {784, 0, 200, TRIANGULAR, 0.5f} });
    sons[SOM_PORTA]        = gerarSom({ {300, 0, 40, RUIDO, 0.4f}, {330, 0, 80, QUADRADA, 0.3f}, {440, 0, 160, QUADRADA, 0.3f} });
    sons[SOM_ESCADA]       = gerarSom({ {392, 0, 90, TRIANGULAR, 0.5f}, {330, 0, 90, TRIANGULAR, 0.5f}, {262, 0, 200, TRIANGULAR, 0.5f} });
    sons[SOM_ARMADILHA]    = gerarSom({ {2200, 400, 130, RUIDO, 0.55f}, {150, 0, 120, QUADRADA, 0.4f} });
    sons[SOM_CAIXA]        = gerarSom({ {300, 80, 140, RUIDO, 0.5f} });
    sons[SOM_VITORIA]      = gerarSom({ {523, 0, 150, QUADRADA, 0.3f}, {659, 0, 150, QUADRADA, 0.3f}, {784, 0, 150, QUADRADA, 0.3f},
                                        {1047, 0, 400, QUADRADA, 0.3f}, {784, 0, 150, QUADRADA, 0.3f}, {1047, 0, 700, QUADRADA, 0.3f} });
    sons[SOM_DERROTA]      = gerarSom({ {392, 0, 250, TRIANGULAR, 0.55f}, {370, 0, 250, TRIANGULAR, 0.55f},
                                        {349, 0, 250, TRIANGULAR, 0.55f}, {330, 0, 700, TRIANGULAR, 0.55f} });
    // Corneta do gnomo: duas notas longas de metal, a segunda mais aguda.
    sons[SOM_CORNETA]      = gerarSom({ {294, 300, 380, QUADRADA_FINA, 0.38f}, {0, 0, 40, TRIANGULAR, 0.0f},
                                        {392, 400, 650, QUADRADA_FINA, 0.38f} });
    // Bloqueio: "clang" de metal (um estalo agudo e um tom que fica soando).
    sons[SOM_BLOQUEIO]     = gerarSom({ {2400, 1800, 30, RUIDO, 0.45f}, {1560, 1500, 90, QUADRADA_FINA, 0.3f},
                                        {1180, 1150, 260, TRIANGULAR, 0.4f} });
    // Portas pesadas se fechando: rangido descendo e uma pancada surda.
    sons[SOM_PORTA_FECHANDO] = gerarSom({ {180, 120, 500, QUADRADA_FINA, 0.12f}, {90, 40, 60, RUIDO, 0.6f},
                                          {70, 50, 420, TRIANGULAR, 0.55f} });
    // Risada do gnomo: "hi-hi-hi-hi-hi-hi", cada vez um pouco mais grave.
    {
        std::vector<Bip> risada;
        for (int k = 0; k < 6; k++) {
            float f = 1250.0f * (1.0f - 0.06f * k);
            Bip hi = {f, f * 0.85f, 55, QUADRADA_FINA, 0.16f};
            hi.vibratoHz = 25; hi.vibrato = 0.05f;
            risada.push_back(hi);
        }
        sons[SOM_RISADA_GNOMO] = gerarBips(risada, 35);
    }
    // Rugido do chefe: grave, desafinado, rouco e com eco.
    {
        Bip r = {70, 38, 1100, QUADRADA, 0.30f};
        r.desafino = 1.07f; r.ruido = 0.3f; r.vibratoHz = 9; r.vibrato = 0.06f;
        r.ecoMs = 220; r.ecos = 2; r.ecoGanho = 0.45f;
        sons[SOM_RUGIDO] = gerarBips({r});
    }
    sons[SOM_TOCHA_APAGA]  = gerarSom({ {600, 150, 260, RUIDO, 0.22f} });
    sons[SOM_RACHADURA]    = gerarSom({ {3000, 900, 70, RUIDO, 0.6f}, {1800, 300, 160, RUIDO, 0.5f},
                                        {110, 60, 300, QUADRADA, 0.35f} });
    criarVozes();
    criarEfeitosComInstrumentos();
}

// -----------------------------------------------------------------------------
// AMBIENTE E PASSOS (v1.21): um segundo fluxo de áudio, com o seu próprio
// volume (Opções > Ambiente), que mistura:
//   - o som do andar em loop (vento, gotas, lava...), trocando com fade;
//   - os sons da fonte e do altar, mais altos quanto mais perto;
//   - os passos (quantos houver ao mesmo tempo, sem limite).
// -----------------------------------------------------------------------------
Amostras ambientes[NUM_AMBIENTES];
Amostras somDaFonte, somDoAltar;
const int VARIANTES_DE_PASSO = 3;
Amostras passos[NUM_PASSOS][VARIANTES_DE_PASSO];
AudioStream fluxoDoAmbiente;

Amostras ambienteDasMinas() {
    const float D = 10;
    Amostras x = passaBanda(ruido(D), 200, 900);              // vento nos túneis
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA;
        x[i] *= 0.5f * (0.35f + 0.25f * std::sin(PI2 * 0.1f * t) + 0.15f * std::sin(PI2 * 0.3f * t + 1));
    }
    Amostras fogo = passaBaixa(ruido(D), 500);                // chiado das tochas
    for (size_t i = 0; i < x.size(); i++) x[i] += fogo[i] * 0.06f;
    for (int i = 0; i < 140; i++) misturar(x, estalo(acaso(0.05f, 0.25f)), acaso(0, D - 0.1f));   // crepitar
    misturar(x, gota(1000, 0.18f), 2.3f);
    misturar(x, gota(1150, 0.15f), 6.8f);
    return emLoop(reverb(x, 1.0f, 0.3f), 0.8f);
}
Amostras ambienteDaCaverna() {
    const float D = 10;
    Amostras x = passaBaixa(ruido(D), 700);                   // água correndo longe
    for (size_t i = 0; i < x.size(); i++) x[i] *= 0.10f * (0.8f + 0.2f * std::sin(PI2 * 0.2f * i / TAXA));
    for (int i = 0; i < 22; i++) misturar(x, gota(acaso(700, 1500), acaso(0.15f, 0.4f)), acaso(0, D - 0.2f));
    return emLoop(reverb(x, 1.6f, 0.5f), 0.8f);
}
Amostras ambienteDoVulcao() {
    const float D = 10;
    Amostras x = passaBaixa(passaBaixa(ruido(D), 90), 90);    // ronco grave da lava
    Amostras chiado = passaAlta(ruido(D), 3000);
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA, s = std::sin(PI2 * 0.4f * t);
        x[i] = x[i] * 1.8f * (0.7f + 0.3f * std::sin(PI2 * 0.2f * t)) + chiado[i] * 0.03f * (0.5f + 0.5f * s * s);
    }
    for (int i = 0; i < 45; i++) misturar(x, bolha(acaso(90, 220), acaso(0.15f, 0.35f)), acaso(0, D - 0.2f));
    for (int i = 0; i < 30; i++) misturar(x, estalo(acaso(0.05f, 0.15f)), acaso(0, D - 0.1f));
    return emLoop(reverb(x, 1.2f, 0.3f), 0.8f);
}
Amostras ambienteDoTrono() {
    const float D = 10;
    Amostras x(TAXA * 10);
    Amostras vento = passaBanda(ruido(D), 300, 1400);
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA, v = 0.5f + 0.5f * std::sin(PI2 * 0.1f * t);
        x[i] = (std::sin(PI2 * 55 * t) + std::sin(PI2 * 55.6f * t) + 0.5f * std::sin(PI2 * 82.4f * t)) * 0.12f *
               (0.8f + 0.2f * std::sin(PI2 * 0.1f * t)) + vento[i] * 0.15f * v * v;
    }
    misturar(x, sino(196, 4, 0.12f, 2.76f, 3, 0.9f), 3.5f);    // sino distante
    return emLoop(reverb(x, 2.0f, 0.55f), 1.0f);
}
// Cripta (v1.23): vento frio por corredores de pedra, sussurros longe, uma
// corrente balançando e o "plic" de uma gota de vez em quando.
Amostras ambienteDaCripta() {
    const float D = 10;
    Amostras x(TAXA * 10);
    Amostras vento = passaBanda(ruido(D), 180, 900);
    Amostras a = ressonancia(ruido(D), 620, 7), b = ressonancia(ruido(D), 980, 7);   // sussurros
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA;
        float v = 0.5f + 0.5f * std::sin(PI2 * 0.07f * t + 1.3f);
        float m = 0.5f + 0.5f * std::sin(PI2 * 0.4f * t + 2 * std::sin(PI2 * 0.13f * t));
        x[i] = vento[i] * 0.22f * v * v + (a[i] + b[i]) * 0.035f * m * m * m +
               std::sin(PI2 * 49 * t) * 0.05f * (0.7f + 0.3f * std::sin(PI2 * 0.05f * t));   // zumbido grave
    }
    for (float p = 1.2f; p < 9.0f; p += 3.7f) {   // corrente balançando: três tilintares metálicos
        for (int k = 0; k < 3; k++) misturar(x, sino(acaso(900, 1300), 0.4f, 0.06f, 3.2f, 2.0f, 7), p + k * 0.13f);
    }
    misturar(x, gota(acaso(900, 1300), 0.12f), 6.4f);
    return emLoop(reverb(x, 2.2f, 0.6f), 1.0f);
}
Amostras ambienteDaFonte() {
    Amostras x = passaBanda(ruido(6), 400, 2500);
    for (float& v : x) v *= 0.10f;
    for (int i = 0; i < 60; i++) misturar(x, gota(acaso(1200, 2400), acaso(0.05f, 0.12f)), acaso(0, 5.9f));
    misturar(x, sino(1568, 1.5f, 0.08f, 2, 1, 2.5f), 1.0f);
    misturar(x, sino(2093, 1.5f, 0.08f, 2, 1, 2.5f), 2.6f);
    misturar(x, sino(1760, 1.5f, 0.08f, 2, 1, 2.5f), 4.3f);
    return emLoop(reverb(x, 1.3f, 0.45f), 0.6f);
}
Amostras ambienteDoAltar() {
    Amostras x(TAXA * 6);
    for (float p = 0.3f; p < 5.6f; p += 1.1f) misturar(x, baque(55, 0.35f, 0.6f), p);   // batida grave lenta
    Amostras a = ressonancia(ruido(6), 700, 6), b = ressonancia(ruido(6), 1150, 6);   // sussurros
    for (size_t i = 0; i < x.size() && i < a.size(); i++) {
        float t = (float)i / TAXA, m = 0.5f + 0.5f * std::sin(PI2 * 0.6f * t + 3 * std::sin(PI2 * 0.23f * t));
        x[i] += (a[i] + b[i]) * 0.05f * m * m * m;
    }
    return emLoop(reverb(x, 1.5f, 0.5f), 0.6f);
}

// Um passo de cada tipo (cada variante sai um pouco diferente).
Amostras umPasso(int tipo) {
    auto pedra = [](float v, float f) {
        Amostras x = envelope(passaBaixa(ruido(0.07f), acaso(900, 1300)), 0, 55);
        misturar(x, envelope(sino(f, 0.07f, 1.0f, 1.0f, 0.0f, 45), 0, 0), 0, 0.6f);
        for (float& a : x) a *= v;
        return x;
    };
    Amostras x;
    switch (tipo) {
        case PASSO_PEDRA:   x = pedra(0.5f, acaso(80, 110)); break;
        case PASSO_AGUA:    x = envelope(passaBanda(ruido(0.18f), 400, 2500), 0, 16); misturar(x, gota(acaso(500, 900), 0.2f), 0.03f); break;
        case PASSO_TAPETE:  x = envelope(passaBaixa(ruido(0.09f), 400), 0, 35); for (float& a : x) a *= 0.65f; break;
        case PASSO_FURTIVO: x = envelope(passaBaixa(ruido(0.06f), 500), 0, 70); for (float& a : x) a *= 0.25f; break;
        case PASSO_RATO:    x = juntar({ estalo(0.25f), silencio(0.03f), estalo(0.2f) }); break;
        case PASSO_KOBOLD:  x = envelope(passaAlta(ruido(0.03f), 2500), 0, 90); for (float& a : x) a *= 0.4f; break;
        case PASSO_CHACAL:  x = juntar({ envelope(passaBaixa(ruido(0.03f), 1500), 0, 90), silencio(0.04f),
                                         envelope(passaBaixa(ruido(0.03f), 1500), 0, 90) }); for (float& a : x) a *= 0.4f; break;
        case PASSO_MACACO:  x = pedra(0.35f, 140); misturar(x, estalo(0.15f), 0.02f); break;
        case PASSO_SAPO:    x = bolha(acaso(120, 160), 0.35f); misturar(x, envelope(passaBanda(ruido(0.08f), 300, 1500), 0, 40), 0, 0.25f); break;
        case PASSO_GOBLIN:  x = envelope(passaBanda(ruido(0.12f), 300, 2000), 0, 20); for (float& a : x) a *= 0.35f; break;
        case PASSO_MORCEGO: {
            x = passaBanda(ruido(0.07f), 200, 1200);
            for (size_t i = 0; i < x.size(); i++) x[i] *= 0.5f * std::sin(3.14159f * i / x.size());
            break;
        }
        case PASSO_OGRO:    x = baque(55, 0.35f, 0.9f); break;
        case PASSO_NAGA: {
            x = passaBanda(ruido(0.4f), 2500, 7000);
            for (size_t i = 0; i < x.size(); i++) x[i] *= 0.25f * std::sin(3.14159f * i / x.size());
            break;
        }
        case PASSO_GNOMO:   x = pedra(0.25f, 160); misturar(x, sino(2637, 0.15f, 0.05f, 2, 1.5f, 20), 0.01f); break;
        case PASSO_MIMICO:  x = sino(180, 0.15f, 0.4f, 1.3f, 3, 25); misturar(x, envelope(passaBaixa(ruido(0.05f), 2000), 0, 60), 0, 0.5f); break;
        case PASSO_ARANHA:      // oito patinhas: estalinhos rápidos
            x = juntar({ estalo(0.12f), silencio(0.015f), estalo(0.1f), silencio(0.015f), estalo(0.12f), silencio(0.015f), estalo(0.1f) });
            break;
        case PASSO_FUNGO:       // "plof" mole
            x = bolha(acaso(90, 120), 0.3f); misturar(x, envelope(passaBaixa(ruido(0.08f), 600), 0, 30), 0, 0.3f);
            break;
        case PASSO_ESQUELETO:   // osso batendo na pedra
            x = pedra(0.35f, 180); misturar(x, sino(acaso(700, 900), 0.08f, 0.25f, 3.1f, 2, 40), 0.005f);
            break;
        case PASSO_SALAMANDRA:  // passos rápidos e um chiado de brasa
            x = juntar({ envelope(passaAlta(ruido(0.03f), 1800), 0, 90), silencio(0.03f), envelope(passaAlta(ruido(0.03f), 1800), 0, 90) });
            for (float& a : x) a *= 0.4f;
            misturar(x, estalo(0.12f), 0.02f);
            break;
        case PASSO_CHEFE: {
            x = baque(38, 0.6f, 1.0f);
            misturar(x, envelope(passaBaixa(ruido(0.4f), 300), 0, 10), 0, 0.6f);
            x = reverb(x, 1.5f, 0.4f);
            break;
        }
    }
    return reverb(x, 0.8f, 0.2f);
}

void criarAmbientes() {
    ambientes[AMBIENTE_MINAS] = ambienteDasMinas();
    ambientes[AMBIENTE_CAVERNA] = ambienteDaCaverna();
    ambientes[AMBIENTE_VULCAO] = ambienteDoVulcao();
    ambientes[AMBIENTE_TRONO] = ambienteDoTrono();
    ambientes[AMBIENTE_CRIPTA] = ambienteDaCripta();
    // O ambiente fica baixinho, por baixo da música (pico perto de 1/3 da música).
    for (int a = 1; a < NUM_AMBIENTES; a++) normalizar(ambientes[a], 0.2f);
    somDaFonte = ambienteDaFonte(); normalizar(somDaFonte, 0.25f);
    somDoAltar = ambienteDoAltar(); normalizar(somDoAltar, 0.28f);
    for (int t = 0; t < NUM_PASSOS; t++)
        for (int v = 0; v < VARIANTES_DE_PASSO; v++) {
            passos[t][v] = umPasso(t);
            normalizar(passos[t][v], t == PASSO_CHEFE || t == PASSO_OGRO ? 0.5f : t == PASSO_FURTIVO ? 0.12f : 0.3f);
        }
}

// Passos esperando para tocar (o jogo coloca; o fluxo de áudio, em outra
// thread, tira daqui; a trava evita que os dois mexam ao mesmo tempo).
struct PassoTocando {
    const Amostras* som;
    float posicao;      // em amostras (com "velocidade" para mudar o tom)
    float velocidade;   // 0.94 a 1.06: cada passo sai num tom um pouco diferente
    float volume;
    int atraso;         // amostras de espera (passos juntos saem desencontrados)
};
std::mutex travaDosPassos;
std::vector<PassoTocando> passosNovos;
std::atomic<int> ambientePedido{SEM_AMBIENTE};
std::atomic<int> volumeDaFonteMil{0}, volumeDoAltarMil{0};   // 0 a 1000
std::vector<double> passosRecentes;                          // (só a thread do jogo usa)

void gerarAmbiente(void* buffer, unsigned int quadros) {
    static int tocando = SEM_AMBIENTE;
    static float ganho = 0;
    static size_t posicao = 0, posFonte = 0, posAltar = 0;
    static float volFonte = 0, volAltar = 0;
    static std::vector<PassoTocando> ativos;
    short* saida = (short*)buffer;
    {
        std::lock_guard<std::mutex> trava(travaDosPassos);
        ativos.insert(ativos.end(), passosNovos.begin(), passosNovos.end());
        passosNovos.clear();
    }
    int pedido = ambientePedido.load();
    float alvoFonte = volumeDaFonteMil.load() / 1000.0f, alvoAltar = volumeDoAltarMil.load() / 1000.0f;
    const float PASSO_DO_FADE = 1.0f / (0.8f * TAXA);
    for (unsigned int i = 0; i < quadros; i++) {
        // Troca de ambiente: o atual some aos poucos e o novo entra aos poucos.
        if (pedido != tocando) {
            ganho -= PASSO_DO_FADE;
            if (ganho <= 0) { ganho = 0; tocando = pedido; posicao = 0; }
        } else if (ganho < 1) {
            ganho = std::min(1.0f, ganho + PASSO_DO_FADE);
        }
        float valor = 0;
        if (tocando != SEM_AMBIENTE && !ambientes[tocando].empty()) {
            valor += ambientes[tocando][posicao] * ganho;
            if (++posicao >= ambientes[tocando].size()) posicao = 0;
        }
        volFonte += (alvoFonte - volFonte) * 0.0005f;
        volAltar += (alvoAltar - volAltar) * 0.0005f;
        if (!somDaFonte.empty()) { valor += somDaFonte[posFonte] * volFonte; if (++posFonte >= somDaFonte.size()) posFonte = 0; }
        if (!somDoAltar.empty()) { valor += somDoAltar[posAltar] * volAltar; if (++posAltar >= somDoAltar.size()) posAltar = 0; }
        saida[i] = 0;
        for (PassoTocando& p : ativos) {
            if (p.atraso > 0) { p.atraso--; continue; }
            size_t k = (size_t)p.posicao;
            if (k + 1 >= p.som->size()) continue;
            float f = p.posicao - k;
            valor += ((*p.som)[k] * (1 - f) + (*p.som)[k + 1] * f) * p.volume;
            p.posicao += p.velocidade;
        }
        saida[i] = (short)(std::clamp(valor, -1.0f, 1.0f) * 30000);
    }
    ativos.erase(std::remove_if(ativos.begin(), ativos.end(),
                                [](const PassoTocando& p) { return (size_t)p.posicao + 1 >= p.som->size(); }),
                 ativos.end());
}

// -----------------------------------------------------------------------------
// MÚSICA: sequenciador de 2 vozes (melodia em onda quadrada + baixo triangular)
// -----------------------------------------------------------------------------
// As notas são números MIDI (60 = Dó central, 69 = Lá 440 Hz; 0 = pausa).
// Todas as melodias foram compostas para este jogo.
struct Faixa {
    int msPorPasso;
    std::vector<int> melodia;
    std::vector<int> baixo;
    // v1.21: o timbre da melodia, a percussão (0 = nada, 1 = picareta,
    // 2 = coração; um número por passo, ou vazio) e o eco (música do Aldric).
    Onda ondaDaMelodia = QUADRADA_FINA;
    std::vector<int> percussao = {};
    bool eco = false;
};

std::vector<Faixa> faixas;

void comporMusicas() {
    faixas.resize(NUM_MUSICAS);
    // Menu: calma, em Lá menor.
    faixas[MUSICA_MENU] = { 210,
        { 69,72,76,81, 79,76,72,76, 65,69,72,77, 76,72,69,72, 74,77,81,77, 76,79,83,79, 81,76,72,69, 0,0,0,0 },
        { 45,0,45,0, 48,0,48,0, 41,0,41,0, 45,0,45,0, 50,0,50,0, 52,0,52,0, 45,0,45,0, 45,0,0,0 } };
    // Masmorra: lenta e misteriosa, em Ré menor, com muitas pausas.
    faixas[MUSICA_MASMORRA] = { 320,
        { 62,0,65,0, 69,0,0,0, 72,0,69,0, 67,0,0,0, 65,0,64,0, 62,0,0,0, 57,0,60,0, 62,0,0,0 },
        { 38,0,0,0, 38,0,0,0, 36,0,0,0, 36,0,0,0, 34,0,0,0, 34,0,0,0, 33,0,0,0, 38,0,0,0 } };
    // Batalha: rápida, em Mi menor.
    faixas[MUSICA_BATALHA] = { 125,
        { 76,76,79,76, 83,81,79,78, 76,76,79,76, 86,84,83,81, 72,72,76,72, 79,78,76,74, 71,74,78,81, 79,78,76,75 },
        { 40,52,40,52, 40,52,40,52, 40,52,40,52, 40,52,40,52, 36,48,36,48, 36,48,36,48, 35,47,35,47, 35,47,35,47 } };
    // Chefe: tensa, com notas "fora do tom" (cromáticas).
    faixas[MUSICA_CHEFE] = { 135,
        { 69,69,72,69, 75,74,72,71, 69,69,72,69, 77,76,75,76, 65,65,69,65, 74,73,72,71, 64,68,71,74, 76,75,74,73 },
        { 33,45,33,45, 33,45,33,45, 33,45,33,45, 33,45,33,45, 29,41,29,41, 29,41,29,41, 28,40,28,40, 28,40,28,40 } };

    // ---- Músicas das cenas (v1.21) ----
    // História do começo: lenta e sombria, em Ré menor, o baixo "segurando" a nota.
    faixas[MUSICA_HISTORIA] = { 380,
        { 62,0,0,0, 65,0,64,0, 62,0,0,0, 57,0,0,0, 58,0,0,0, 57,0,55,0, 57,0,0,0, 0,0,0,0 },
        { 38,0,38,0, 38,0,38,0, 34,0,34,0, 33,0,33,0, 31,0,31,0, 33,0,33,0, 38,0,38,0, 38,0,0,0 }, TRIANGULAR };
    // Eldra: leve e aventureira, em Sol maior.
    faixas[MUSICA_ELDRA] = { 170,
        { 67,71,74,79, 78,74,71,74, 72,76,79,84, 83,79,76,79, 74,78,81,86, 84,81,78,81, 79,74,71,67, 0,0,0,0 },
        { 43,0,50,0, 43,0,50,0, 48,0,55,0, 48,0,55,0, 50,0,57,0, 50,0,57,0, 43,0,50,0, 43,0,0,0 } };
    // Borin: marcada, com a picareta batendo, em Mi menor.
    faixas[MUSICA_BORIN] = { 230,
        { 64,0,64,67, 64,0,62,0, 60,0,60,64, 60,0,59,0, 57,0,57,60, 62,0,64,0, 59,0,0,0, 0,0,0,0 },
        { 40,40,0,40, 40,40,0,40, 36,36,0,36, 36,36,0,36, 33,33,0,33, 35,35,0,35, 40,0,40,0, 40,0,0,0 }, QUADRADA,
        { 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0 } };
    // Aldric: etérea, notas macias com eco, em Lá menor.
    faixas[MUSICA_ALDRIC] = { 300,
        { 81,0,76,0, 72,0,76,0, 79,0,74,0, 71,0,74,0, 77,0,72,0, 69,0,72,0, 76,0,0,0, 0,0,0,0 },
        { 45,0,0,0, 45,0,0,0, 43,0,0,0, 43,0,0,0, 41,0,0,0, 41,0,0,0, 40,0,0,0, 45,0,0,0 }, SENOIDE, {}, true };
    // Garro: saltitante, a "musiquinha de loja", em Dó maior.
    faixas[MUSICA_GARRO] = { 150,
        { 72,0,76,79, 0,76,72,0, 74,0,77,81, 0,77,74,0, 76,79,84,79, 76,0,72,0, 74,76,74,71, 72,0,0,0 },
        { 48,0,55,0, 48,0,55,0, 50,0,57,0, 50,0,57,0, 48,0,55,0, 45,0,52,0, 43,0,50,0, 48,0,48,0 } };
    // Tobias: simples e melancólica, em Fá maior.
    faixas[MUSICA_TOBIAS] = { 280,
        { 69,0,67,65, 67,0,0,0, 65,0,64,62, 64,0,0,0, 62,0,60,58, 60,0,62,0, 65,0,0,0, 0,0,0,0 },
        { 41,0,48,0, 46,0,50,0, 38,0,45,0, 43,0,46,0, 34,0,41,0, 36,0,43,0, 41,0,48,0, 41,0,0,0 }, TRIANGULAR };
    // Cena do chefe: quase silêncio, notas que se arrastam e o coração batendo (tum-tum).
    faixas[MUSICA_CHEFE_CENA] = { 420,
        { 0,0,0,0, 57,0,0,0, 58,0,0,0, 57,0,0,0, 0,0,0,0, 56,0,0,0, 57,0,0,0, 0,0,0,0 },
        { 33,33,0,0, 33,33,0,0, 33,33,0,0, 33,33,0,0, 32,32,0,0, 32,32,0,0, 33,33,0,0, 33,33,0,0 }, QUADRADA,
        { 2,2,0,0, 2,2,0,0, 2,2,0,0, 2,2,0,0, 2,2,0,0, 2,2,0,0, 2,2,0,0, 2,2,0,0 } };
    // Final: triunfante, em Dó maior.
    faixas[MUSICA_FINAL] = { 190,
        { 67,72,76,79, 0,76,79,0, 84,0,83,81, 79,0,0,0, 77,81,84,81, 79,76,72,76, 74,76,77,74, 72,0,0,0 },
        { 48,0,55,0, 48,0,55,0, 45,0,52,0, 43,0,50,0, 41,0,48,0, 48,0,55,0, 43,0,50,0, 48,0,48,0 }, QUADRADA };
}

// Os sons da percussão da música (picareta e coração), feitos uma vez.
Amostras percussaoPicareta, percussaoCoracao;

std::atomic<int> musicaAtual{SEM_MUSICA};
int musicaTocando = SEM_MUSICA;   // usada só dentro do callback
int passo = 0, amostraNoPasso = 0;
float faseMelodia = 0, faseBaixo = 0;

float midiParaHz(int nota) { return 440.0f * std::pow(2.0f, (nota - 69) / 12.0f); }

// Chamada pela raylib (em outra thread) sempre que precisa de mais áudio.
// Ao trocar de música, a atual some em 1/4 de segundo e a nova entra (fade).
void gerarMusica(void* buffer, unsigned int quadros) {
    short* saida = (short*)buffer;
    static float ganho = 0;
    static int percussao = 0;
    static size_t posPercussao = 0;
    static std::vector<float> linhaDoEco(TAXA * 2 / 5, 0.0f);   // 0,18 s de eco (cabe com folga)
    static size_t posEco = 0;
    const float PASSO_DO_FADE = 1.0f / (0.25f * TAXA);
    const size_t ATRASO_DO_ECO = (size_t)(0.18f * TAXA);
    int pedida = musicaAtual.load();
    for (unsigned int i = 0; i < quadros; i++) {
        if (pedida != musicaTocando) {
            ganho -= PASSO_DO_FADE;
            if (ganho <= 0) {   // terminou de sumir: troca e começa do início
                ganho = 0;
                musicaTocando = pedida;
                passo = 0;
                amostraNoPasso = 0;
                percussao = 0;
            }
        } else if (ganho < 1) {
            ganho = std::min(1.0f, ganho + PASSO_DO_FADE);
        }
        if (musicaTocando == SEM_MUSICA) { saida[i] = 0; continue; }
        const Faixa& f = faixas[musicaTocando];
        int amostrasPorPasso = f.msPorPasso * TAXA / 1000;
        int numPassos = (int)f.melodia.size();
        if (amostraNoPasso == 0 && !f.percussao.empty()) {
            percussao = f.percussao[passo % f.percussao.size()];
            posPercussao = 0;
        }
        int notaMelodia = f.melodia[passo], notaBaixo = f.baixo[passo];
        float t = (float)amostraNoPasso / amostrasPorPasso;
        float valor = 0.0f;
        if (notaMelodia > 0) {
            faseMelodia += midiParaHz(notaMelodia) / TAXA;
            faseMelodia -= std::floor(faseMelodia);
            uint32_t semRuido = 1;
            // Ondas macias (triangular, senoide) soam mais baixo: um pouco mais de volume.
            float volume = (f.ondaDaMelodia == TRIANGULAR || f.ondaDaMelodia == SENOIDE) ? 0.22f : 0.10f;
            valor += amostraDaOnda(f.ondaDaMelodia, faseMelodia, semRuido) * volume * (1.0f - 0.6f * t);
        }
        if (notaBaixo > 0) {
            faseBaixo += midiParaHz(notaBaixo) / TAXA;
            faseBaixo -= std::floor(faseBaixo);
            float tri = faseBaixo < 0.5f ? 4.0f * faseBaixo - 1.0f : 3.0f - 4.0f * faseBaixo;
            valor += tri * 0.22f * (1.0f - 0.4f * t);
        }
        if (amostrasPorPasso - amostraNoPasso < 80) valor *= (amostrasPorPasso - amostraNoPasso) / 80.0f;
        if (percussao > 0) {
            const Amostras& p = percussao == 1 ? percussaoPicareta : percussaoCoracao;
            if (posPercussao < p.size()) valor += p[posPercussao++];
        }
        if (f.eco) {   // o que tocou há 0,18 s volta mais baixo
            float atrasado = linhaDoEco[(posEco + linhaDoEco.size() - ATRASO_DO_ECO) % linhaDoEco.size()];
            valor += atrasado * 0.45f;
            linhaDoEco[posEco] = valor;
            posEco = (posEco + 1) % linhaDoEco.size();
        }
        saida[i] = (short)(std::clamp(valor * ganho, -2.0f, 2.0f) * 16000);
        if (++amostraNoPasso >= amostrasPorPasso) {
            amostraNoPasso = 0;
            passo = (passo + 1) % numPassos;
        }
    }
}
}

// -----------------------------------------------------------------------------
void iniciarAudio() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;   // computador sem saída de som: segue mudo
    criarEfeitos();
    comporMusicas();
    percussaoPicareta = reverb(juntar({ estalo(0.5f), sino(1800, 0.08f, 0.12f, 2.7f, 3, 30) }), 0.6f, 0.2f);
    normalizar(percussaoPicareta, 0.18f);
    percussaoCoracao = baque(48, 0.25f, 1.0f);
    normalizar(percussaoCoracao, 0.35f);
    criarAmbientes();
    SetAudioStreamBufferSizeDefault(2048);
    fluxoDaMusica = LoadAudioStream(TAXA, 16, 1);
    SetAudioStreamCallback(fluxoDaMusica, gerarMusica);
    PlayAudioStream(fluxoDaMusica);
    fluxoDoAmbiente = LoadAudioStream(TAXA, 16, 1);
    SetAudioStreamCallback(fluxoDoAmbiente, gerarAmbiente);
    PlayAudioStream(fluxoDoAmbiente);
    audioPronto = true;
    aplicarVolumes();
}

void finalizarAudio() {
    if (!audioPronto) return;
    StopAudioStream(fluxoDaMusica);
    UnloadAudioStream(fluxoDaMusica);
    StopAudioStream(fluxoDoAmbiente);
    UnloadAudioStream(fluxoDoAmbiente);
    for (int i = 0; i < NUM_SONS; i++) UnloadSound(sons[i]);
    for (int v = 0; v < NUM_VOZES; v++)
        for (int k = 0; k < vozes[v].variantes; k++) {
            for (int c = 1; c < 3; c++) UnloadSoundAlias(vozes[v].bip[k][c]);
            UnloadSound(vozes[v].bip[k][0]);
        }
    CloseAudioDevice();
    audioPronto = false;
}

void tocarSom(Som som) {
    if (audioPronto && ligado && som >= 0 && som < NUM_SONS) PlaySound(sons[som]);
}

void tocarSomComTom(Som som, float tom) {
    if (!audioPronto || !ligado || som < 0 || som >= NUM_SONS) return;
    SetSoundPitch(sons[som], tom);
    PlaySound(sons[som]);
}

void tocarVoz(Voz voz, int letra) {
    if (!audioPronto || !ligado || voz < 0 || voz >= NUM_VOZES) return;
    VozPronta& v = vozes[voz];
    if (letra % v.cada != 0) return;
    int qual = (letra / v.cada) % v.variantes;
    static int copia = 0;
    copia = (copia + 1) % 3;
    Sound& s = v.bip[qual][copia];
    SetSoundPitch(s, 1.0f + v.variacao * (GetRandomValue(-100, 100) / 100.0f));
    PlaySound(s);
}

void tocarAmbiente(Ambiente ambiente) {
    ambientePedido.store(ambiente);
}

void volumeDasSalas(float fonte, float altar) {
    volumeDaFonteMil.store((int)(std::clamp(fonte, 0.0f, 1.0f) * 1000));
    volumeDoAltarMil.store((int)(std::clamp(altar, 0.0f, 1.0f) * 1000));
}

void tocarPasso(Passo passo, float volume) {
    if (!audioPronto || !ligado || passo < 0 || passo >= NUM_PASSOS || volume <= 0.01f) return;
    // Muitos passos ao mesmo tempo: cada um fica um pouco mais baixo (a soma
    // não estoura), mas dá para perceber que são muitos.
    double agora = GetTime();
    passosRecentes.erase(std::remove_if(passosRecentes.begin(), passosRecentes.end(),
                                        [&](double t) { return agora - t > 0.4; }), passosRecentes.end());
    passosRecentes.push_back(agora);
    float divisor = std::sqrt((float)std::max<size_t>(1, passosRecentes.size() / 2));
    PassoTocando p;
    p.som = &passos[passo][GetRandomValue(0, VARIANTES_DE_PASSO - 1)];
    p.posicao = 0;
    p.velocidade = 1.0f + GetRandomValue(-60, 60) / 1000.0f;
    p.volume = volume / divisor;
    p.atraso = GetRandomValue(0, TAXA * 4 / 100);   // até 40 ms: passos juntos saem desencontrados
    std::lock_guard<std::mutex> trava(travaDosPassos);
    passosNovos.push_back(p);
}

void tocarMusica(Musica musica) {
    musicaAtual.store(musica);
}

bool alternarSom() {
    ligado = !ligado;
    if (audioPronto) SetMasterVolume(ligado ? 1.0f : 0.0f);
    return ligado;
}

bool somLigado() { return ligado; }

void aplicarVolumes() {
    if (!audioPronto) return;
    // A música já era tocada a 80% (para não cobrir os efeitos).
    SetAudioStreamVolume(fluxoDaMusica, 0.8f * opcoes().volumeMusica / 10.0f);
    SetAudioStreamVolume(fluxoDoAmbiente, opcoes().volumeAmbiente / 10.0f);
    for (int i = 0; i < NUM_SONS; i++) SetSoundVolume(sons[i], opcoes().volumeEfeitos / 10.0f);
    for (int v = 0; v < NUM_VOZES; v++)
        for (int k = 0; k < vozes[v].variantes; k++)
            for (int c = 0; c < 3; c++) SetSoundVolume(vozes[v].bip[k][c], opcoes().volumeEfeitos / 10.0f);
}
