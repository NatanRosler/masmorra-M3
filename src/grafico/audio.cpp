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

#include <algorithm>
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
    for (int v = 0; v < NUM_VOZES; v++)
        for (int k = 0; k < vozes[v].variantes; k++)
            for (int c = 0; c < 3; c++) SetSoundVolume(vozes[v].bip[k][c], opcoes().volumeEfeitos / 10.0f);
}
