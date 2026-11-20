// sintese.cpp
// -----------------------------------------------------------------------------
// Os "instrumentos" do sintese.h. Os filtros são biquads (a receita clássica
// de Robert Bristow-Johnson para equalizadores de áudio).
// -----------------------------------------------------------------------------
#include "sintese.h"

#include <algorithm>
#include <cmath>

namespace Sintese {

namespace {
const float PI = 3.14159265f;
uint32_t semente = 0x9E3779B9u;
float sorteio() {   // xorshift: 0 a 1
    semente ^= semente << 13; semente ^= semente >> 17; semente ^= semente << 5;
    return (float)(semente & 0xFFFFFF) / (float)0x1000000;
}
int amostras(float segundos) { return std::max(1, (int)(segundos * TAXA)); }

// Biquad: y = (b0 x + b1 x1 + b2 x2 - a1 y1 - a2 y2) / a0
Amostras biquad(Amostras x, float b0, float b1, float b2, float a0, float a1, float a2) {
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (float& v : x) {
        float y = (b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2) / a0;
        x2 = x1; x1 = v; y2 = y1; y1 = y;
        v = y;
    }
    return x;
}
}

float acaso(float minimo, float maximo) { return minimo + (maximo - minimo) * sorteio(); }

Amostras silencio(float segundos) { return Amostras(amostras(segundos), 0.0f); }

Amostras ruido(float segundos) {
    Amostras x(amostras(segundos));
    for (float& v : x) v = sorteio() * 2 - 1;
    return x;
}

Amostras passaBaixa(Amostras x, float freq) {
    float w = 2 * PI * freq / TAXA, c = std::cos(w), alfa = std::sin(w) / (2 * 0.7071f);
    return biquad(x, (1 - c) / 2, 1 - c, (1 - c) / 2, 1 + alfa, -2 * c, 1 - alfa);
}
Amostras passaAlta(Amostras x, float freq) {
    float w = 2 * PI * freq / TAXA, c = std::cos(w), alfa = std::sin(w) / (2 * 0.7071f);
    return biquad(x, (1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alfa, -2 * c, 1 - alfa);
}
Amostras ressonancia(Amostras x, float freq, float q) {
    float w = 2 * PI * freq / TAXA, c = std::cos(w), alfa = std::sin(w) / (2 * q);
    return biquad(x, alfa, 0, -alfa, 1 + alfa, -2 * c, 1 - alfa);
}
Amostras passaBanda(Amostras x, float de, float ate) {
    float centro = std::sqrt(de * ate);
    return ressonancia(x, centro, centro / (ate - de));
}

Amostras sino(float freq, float segundos, float volume, float razao, float indice, float decaimento) {
    Amostras x(amostras(segundos));
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA, e = std::exp(-t * decaimento);
        x[i] = std::sin(2 * PI * freq * t + indice * e * std::sin(2 * PI * freq * razao * t)) * e * volume;
    }
    for (int i = 0; i < 40 && i < (int)x.size(); i++) x[i] *= i / 40.0f;
    return x;
}

Amostras corda(float freq, float segundos, float volume, float brilho) {
    // Karplus-Strong: um pedaço de ruído que se repete e vai sendo suavizado.
    int periodo = std::max(2, (int)(TAXA / freq));
    Amostras buffer(periodo);
    for (float& v : buffer) v = sorteio() * 2 - 1;
    Amostras x(amostras(segundos));
    for (size_t i = 0; i < x.size(); i++) {
        int k = (int)(i % periodo), proximo = (k + 1) % periodo;
        x[i] = buffer[k] * volume;
        buffer[k] = brilho * 0.5f * (buffer[k] + buffer[proximo]);
    }
    return x;
}

Amostras gota(float freq, float volume) {
    Amostras x(amostras(0.09f));
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA;
        x[i] = std::sin(2 * PI * (freq * t + freq * 3.2f * t * t)) * std::exp(-t * 45) * volume;   // o tom sobe: "plic"
    }
    return x;
}

Amostras bolha(float freq, float volume) {
    float dur = acaso(0.08f, 0.16f);
    Amostras x(amostras(dur));
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA;
        x[i] = std::sin(2 * PI * (freq * t + freq * 6 * t * t)) * std::sin(PI * t / dur) * volume;
    }
    return x;
}

Amostras estalo(float volume) {
    Amostras x = passaAlta(ruido(acaso(0.004f, 0.015f)), 1500);
    for (size_t i = 0; i < x.size(); i++) x[i] *= volume * (1 - (float)i / x.size());
    return x;
}

Amostras baque(float freq, float segundos, float volume) {
    Amostras x(amostras(segundos));
    Amostras r = passaBaixa(ruido(segundos), freq * 6);
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA;
        float tom = std::sin(2 * PI * (freq * t - freq * 0.25f * t * t)) * std::exp(-t * 9);
        x[i] = (tom * 0.9f + r[i] * 0.5f * std::exp(-t * 25)) * volume;
    }
    return x;
}

Amostras juntar(const std::vector<Amostras>& partes, float pausa) {
    Amostras x;
    for (const Amostras& p : partes) {
        x.insert(x.end(), p.begin(), p.end());
        x.insert(x.end(), amostras(pausa) * (pausa > 0), 0.0f);
    }
    return x;
}

void misturar(Amostras& destino, const Amostras& som, float emSegundos, float volume) {
    size_t inicio = (size_t)std::max(0, (int)(emSegundos * TAXA));
    if (destino.size() < inicio + som.size()) destino.resize(inicio + som.size(), 0.0f);
    for (size_t i = 0; i < som.size(); i++) destino[inicio + i] += som[i] * volume;
}

Amostras envelope(Amostras x, float ataque, float decaimento) {
    for (size_t i = 0; i < x.size(); i++) {
        float t = (float)i / TAXA;
        x[i] *= std::min(1.0f, ataque > 0 ? t / ataque : 1.0f) * std::exp(-t * decaimento);
    }
    return x;
}

Amostras reverb(const Amostras& entrada, float tamanho, float mistura) {
    // 4 ecos que se realimentam (pentes) em paralelo + 2 "passa-tudo" em série.
    Amostras x = entrada;
    x.resize(entrada.size() + TAXA * 3 / 2, 0.0f);
    Amostras saida(x.size(), 0.0f);
    const float atrasos[4] = { 0.0297f, 0.0371f, 0.0411f, 0.0437f }, ganhos[4] = { 0.78f, 0.76f, 0.74f, 0.72f };
    for (int p = 0; p < 4; p++) {
        int k = std::max(1, (int)(atrasos[p] * TAXA * tamanho));
        Amostras y(x.size(), 0.0f);
        for (size_t i = 0; i < x.size(); i++) y[i] = x[i] + (i >= (size_t)k ? ganhos[p] * y[i - k] : 0.0f);
        for (size_t i = 0; i < x.size(); i++) saida[i] += y[i];
    }
    const float atrasosPT[2] = { 0.005f, 0.0017f };
    for (int p = 0; p < 2; p++) {
        int k = std::max(1, (int)(atrasosPT[p] * TAXA));
        float g = 0.7f;
        Amostras y(saida.size(), 0.0f);
        for (size_t i = 0; i < saida.size(); i++) {
            float xk = i >= (size_t)k ? saida[i - k] : 0.0f, yk = i >= (size_t)k ? y[i - k] : 0.0f;
            y[i] = -g * saida[i] + xk + g * yk;
        }
        saida = y;
    }
    for (size_t i = 0; i < x.size(); i++) saida[i] = x[i] * (1 - mistura) + saida[i] * mistura * 0.25f;
    // Corta a cauda que ficou em silêncio.
    size_t fim = saida.size();
    while (fim > entrada.size() && std::fabs(saida[fim - 1]) < 0.0005f) fim--;
    saida.resize(fim);
    return saida;
}

void normalizar(Amostras& x, float pico) {
    float maior = 0;
    for (float v : x) maior = std::max(maior, std::fabs(v));
    if (maior < 1e-6f) return;
    for (float& v : x) v *= pico / maior;
}

Amostras emLoop(Amostras x, float segundosDeEmenda) {
    int n = amostras(segundosDeEmenda);
    if ((int)x.size() <= 2 * n) return x;
    // O pedaço que sobra no fim entra, aos poucos, por cima do começo.
    Amostras y(x.begin(), x.end() - n);
    for (int i = 0; i < n; i++) {
        float f = (float)i / n;
        y[i] = y[i] * f + x[x.size() - n + i] * (1 - f);
    }
    return y;
}

}  // namespace Sintese
