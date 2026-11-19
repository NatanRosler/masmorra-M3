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

#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
const int TAXA = 22050;            // amostras por segundo
const float PI2 = 6.2831853f;

enum Onda { QUADRADA, QUADRADA_FINA, TRIANGULAR, RUIDO };

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
    // Bloqueio: "clang" de metal (um estalo agudo e um tom que fica soando).
    sons[SOM_BLOQUEIO]     = gerarSom({ {2400, 1800, 30, RUIDO, 0.45f}, {1560, 1500, 90, QUADRADA_FINA, 0.3f},
                                        {1180, 1150, 260, TRIANGULAR, 0.4f} });
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
}

std::atomic<int> musicaAtual{SEM_MUSICA};
int musicaTocando = SEM_MUSICA;   // usada só dentro do callback
int passo = 0, amostraNoPasso = 0;
float faseMelodia = 0, faseBaixo = 0;

float midiParaHz(int nota) { return 440.0f * std::pow(2.0f, (nota - 69) / 12.0f); }

// Chamada pela raylib (em outra thread) sempre que precisa de mais áudio.
void gerarMusica(void* buffer, unsigned int quadros) {
    short* saida = (short*)buffer;
    int pedida = musicaAtual.load();
    if (pedida != musicaTocando) {   // trocou de música: começa do início
        musicaTocando = pedida;
        passo = 0;
        amostraNoPasso = 0;
    }
    if (musicaTocando == SEM_MUSICA) {
        for (unsigned int i = 0; i < quadros; i++) saida[i] = 0;
        return;
    }
    const Faixa& f = faixas[musicaTocando];
    int amostrasPorPasso = f.msPorPasso * TAXA / 1000;
    int numPassos = (int)f.melodia.size();
    for (unsigned int i = 0; i < quadros; i++) {
        int notaMelodia = f.melodia[passo], notaBaixo = f.baixo[passo];
        float t = (float)amostraNoPasso / amostrasPorPasso;
        float valor = 0.0f;
        if (notaMelodia > 0) {
            faseMelodia += midiParaHz(notaMelodia) / TAXA;
            faseMelodia -= std::floor(faseMelodia);
            valor += (faseMelodia < 0.25f ? 1.0f : -1.0f) * 0.10f * (1.0f - 0.6f * t);
        }
        if (notaBaixo > 0) {
            faseBaixo += midiParaHz(notaBaixo) / TAXA;
            faseBaixo -= std::floor(faseBaixo);
            float tri = faseBaixo < 0.5f ? 4.0f * faseBaixo - 1.0f : 3.0f - 4.0f * faseBaixo;
            valor += tri * 0.22f * (1.0f - 0.4f * t);
        }
        if (amostrasPorPasso - amostraNoPasso < 80) valor *= (amostrasPorPasso - amostraNoPasso) / 80.0f;
        saida[i] = (short)(valor * 16000);
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
    SetAudioStreamBufferSizeDefault(2048);
    fluxoDaMusica = LoadAudioStream(TAXA, 16, 1);
    SetAudioStreamCallback(fluxoDaMusica, gerarMusica);
    PlayAudioStream(fluxoDaMusica);
    audioPronto = true;
    aplicarVolumes();
}

void finalizarAudio() {
    if (!audioPronto) return;
    StopAudioStream(fluxoDaMusica);
    UnloadAudioStream(fluxoDaMusica);
    for (int i = 0; i < NUM_SONS; i++) UnloadSound(sons[i]);
    CloseAudioDevice();
    audioPronto = false;
}

void tocarSom(Som som) {
    if (audioPronto && ligado && som >= 0 && som < NUM_SONS) PlaySound(sons[som]);
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
    for (int i = 0; i < NUM_SONS; i++) SetSoundVolume(sons[i], opcoes().volumeEfeitos / 10.0f);
}
