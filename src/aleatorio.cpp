// aleatorio.cpp
#include "aleatorio.h"

#include <chrono>

void iniciarGerador(GeradorAleatorio& g, uint32_t semente) {
    // O xorshift trava se o estado for 0, então passamos a semente por
    // misturar() antes (e trocamos 0 por 1, por garantia).
    g.estado = misturar(semente, 0x9E3779B9u);
    if (g.estado == 0) g.estado = 1;
}

uint32_t proximoNumero(GeradorAleatorio& g) {
    uint32_t x = g.estado;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g.estado = x;
    return x;
}

int sortear(GeradorAleatorio& g, int minimo, int maximo) {
    if (maximo <= minimo) return minimo;
    uint32_t tamanho = (uint32_t)(maximo - minimo + 1);
    return minimo + (int)(proximoNumero(g) % tamanho);
}

bool chance(GeradorAleatorio& g, int porcentagem) {
    return sortear(g, 1, 100) <= porcentagem;
}

// Função de mistura do "splitmix32": pequenas mudanças na entrada geram
// saídas completamente diferentes.
uint32_t misturar(uint32_t a, uint32_t b) {
    uint32_t z = a + 0x9E3779B9u * (b + 1);
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

uint32_t sementeDoRelogio() {
    auto agora = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    uint32_t s = misturar((uint32_t)agora, (uint32_t)(agora >> 32));
    // Sementes curtas (até 6 dígitos) são mais fáceis de anotar e digitar.
    return s % 1000000;
}
