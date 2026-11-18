// efeitos.cpp
#include "efeitos.h"
#include "visual.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
std::vector<Efeitos::Particula> particulas;

struct Flutuante {
    std::string texto;
    float x, y;
    Color cor;
    float tamanho;
    double inicio, duracao;
    int camada;
};
std::vector<Flutuante> flutuantes;

double tremorAte = 0, tremorDuracao = 1;
float tremorForca = 0;
double piscadaInicio = -10, piscadaDuracao = 0.1;
Color piscadaCor = WHITE;
double escurecimentoInicio = -10;
const double DURACAO_DO_ESCURECIMENTO = 0.22;
int telaAtual = -1;
Rectangle cliqueRect = {0, 0, 0, 0};
double cliqueInicio = -10;

// Número aleatório simples entre a e b (só para variar as partículas).
float acaso(float a, float b) { return a + (b - a) * (float)GetRandomValue(0, 1000) / 1000.0f; }

Color comAlfa(Color c, float alfa) {
    c.a = (unsigned char)(std::clamp(alfa, 0.0f, 1.0f) * c.a);
    return c;
}
}

namespace Efeitos {

void criarParticula(const Particula& p) {
    if (particulas.size() < 600) particulas.push_back(p);
}

static void jorrar(float x, float y, int quantidade, float velMin, float velMax, float angMin, float angMax,
                   Color cor, float tamanho, double vida, int forma, float gravidade, int camada) {
    double agora = GetTime();
    for (int i = 0; i < quantidade; i++) {
        float ang = acaso(angMin, angMax);
        float vel = acaso(velMin, velMax);
        Particula p;
        p.x = x + acaso(-8, 8);
        p.y = y + acaso(-8, 8);
        p.vx = std::cos(ang) * vel;
        p.vy = std::sin(ang) * vel;
        p.gravidade = gravidade;
        p.cor = cor;
        p.tamanho = tamanho * acaso(0.7f, 1.3f);
        p.inicio = agora + acaso(0, 0.06f);
        p.vida = vida * acaso(0.8f, 1.2f);
        p.forma = forma;
        p.giro = acaso(0, 6.28f);
        p.camada = camada;
        criarParticula(p);
    }
}

void explosaoDoElemento(int elemento, float x, float y, int camada, float forca) {
    int n = (int)(12 * forca);
    switch (elemento) {
        case 1:  // Fogo: chamas laranja e vermelhas subindo
            jorrar(x, y, n, 60, 160, -PI * 0.9f, -PI * 0.1f, {255, 150, 40, 255}, 7, 0.6, BOLINHA, -120, camada);
            jorrar(x, y, n / 2, 40, 120, -PI * 0.8f, -PI * 0.2f, {240, 70, 40, 255}, 5, 0.5, BOLINHA, -150, camada);
            break;
        case 2:  // Água: bolhas azuis
            jorrar(x, y, n, 40, 140, 0, 2 * PI, {90, 170, 255, 255}, 7, 0.7, ANEL, -60, camada);
            jorrar(x, y, n / 2, 80, 180, 0, 2 * PI, {200, 230, 255, 255}, 3, 0.4, BOLINHA, 0, camada);
            break;
        case 3:  // Planta: folhas verdes girando
            jorrar(x, y, n, 60, 180, 0, 2 * PI, {100, 210, 100, 255}, 8, 0.75, FOLHA, 40, camada);
            break;
        case 4:  // Veneno: bolhas roxas
            jorrar(x, y, n, 30, 110, -PI, 0, {190, 100, 230, 255}, 7, 0.85, ANEL, -40, camada);
            jorrar(x, y, n / 2, 30, 90, -PI, 0, {140, 60, 180, 255}, 5, 0.8, BOLINHA, -30, camada);
            break;
        case 5:  // Pedra: pedrinhas que caem
            jorrar(x, y - 60, n, 20, 90, -PI * 0.9f, -PI * 0.1f, {180, 145, 105, 255}, 8, 0.7, QUADRADO, 600, camada);
            break;
        default: // Normal: estrelas de impacto
            jorrar(x, y, n, 120, 260, 0, 2 * PI, {255, 245, 200, 255}, 7, 0.4, ESTRELA, 0, camada);
            break;
    }
}

void brilhoSubindo(float x, float y, Color cor, int quantidade, int camada) {
    jorrar(x, y + 20, quantidade, 30, 90, -3.14159f * 0.75f, -3.14159f * 0.25f, cor, 5, 0.9, FAISCA, -80, camada);
}

void lascas(float x, float y, int camada) {
    jorrar(x, y, 10, 60, 170, 0, 6.28f, {170, 115, 60, 255}, 5, 0.55, QUADRADO, 350, camada);
}

void espinhos(float x, float y, int camada) {
    jorrar(x, y, 8, 80, 160, -3.14159f * 0.85f, -3.14159f * 0.15f, {230, 230, 235, 255}, 6, 0.4, FAISCA, 300, camada);
}

void limparParticulas(int camada) {
    particulas.erase(std::remove_if(particulas.begin(), particulas.end(),
                                    [camada](const Particula& p) { return p.camada == camada; }),
                     particulas.end());
    flutuantes.erase(std::remove_if(flutuantes.begin(), flutuantes.end(),
                                    [camada](const Flutuante& f) { return f.camada == camada; }),
                     flutuantes.end());
}

void desenharParticulas(int camada, float deslocX, float deslocY) {
    double agora = GetTime();
    for (const Particula& p : particulas) {
        if (p.camada != camada) continue;
        float t = (float)(agora - p.inicio);
        if (t < 0 || t > p.vida) continue;
        // Posição pelo tempo: x = x0 + v.t ; y = y0 + v.t + g.t²/2
        float x = p.x + p.vx * t + deslocX;
        float y = p.y + p.vy * t + 0.5f * p.gravidade * t * t + deslocY;
        float resta = 1.0f - t / (float)p.vida;
        Color c = comAlfa(p.cor, resta * 1.3f);
        float s = p.tamanho * (0.4f + 0.6f * resta);
        switch (p.forma) {
            case ANEL:     DrawCircleLines((int)x, (int)y, s, c); DrawCircleLines((int)x, (int)y, s - 1, c); break;
            case QUADRADO: DrawRectanglePro({x, y, s, s}, {s / 2, s / 2}, (p.giro + t * 6) * 57.3f, c); break;
            case ESTRELA:  DrawPoly({x, y}, 4, s, (p.giro + t * 8) * 57.3f, c); break;
            case FOLHA:    DrawEllipse((int)x, (int)y, s, s * 0.45f, c); break;
            case FAISCA:   DrawPoly({x, y}, 4, s * 0.8f, 45, c); break;
            default:       DrawCircleV({x, y}, s, c); break;
        }
    }
    // Tira da lista o que já acabou.
    particulas.erase(std::remove_if(particulas.begin(), particulas.end(),
                                    [agora](const Particula& p) { return agora - p.inicio > p.vida; }),
                     particulas.end());
}

void textoFlutuante(const std::string& texto, float x, float y, Color cor, float tamanho, int camada,
                    double duracao) {
    flutuantes.push_back({texto, x, y, cor, tamanho, GetTime(), duracao, camada});
}

void desenharTextosFlutuantes(int camada, float deslocX, float deslocY) {
    double agora = GetTime();
    for (const Flutuante& f : flutuantes) {
        if (f.camada != camada) continue;
        float t = (float)((agora - f.inicio) / f.duracao);
        if (t < 0 || t > 1) continue;
        // Sobe rápido no começo e devagar no fim; some na segunda metade.
        float subida = 50.0f * (1.0f - (1.0f - t) * (1.0f - t));
        float alfa = t < 0.5f ? 1.0f : 1.0f - (t - 0.5f) * 2.0f;
        float escala = t < 0.12f ? 0.6f + t / 0.12f * 0.5f : 1.1f - std::min(0.1f, t);
        float x = f.x + deslocX, y = f.y - subida + deslocY;
        Ui::textoCentro(f.texto, x + 2, y + 2, f.tamanho * escala, comAlfa({0, 0, 0, 200}, alfa));
        Ui::textoCentro(f.texto, x, y, f.tamanho * escala, comAlfa(f.cor, alfa));
    }
    flutuantes.erase(std::remove_if(flutuantes.begin(), flutuantes.end(),
                                    [agora](const Flutuante& f) { return agora - f.inicio > f.duracao; }),
                     flutuantes.end());
}

void tremer(float intensidade, double duracao) {
    tremorForca = intensidade;
    tremorDuracao = duracao;
    tremorAte = GetTime() + duracao;
}

Vector2 deslocamentoDoTremor() {
    double resta = tremorAte - GetTime();
    if (resta <= 0) return {0, 0};
    float f = tremorForca * (float)(resta / tremorDuracao);
    return {acaso(-f, f), acaso(-f, f)};
}

void piscar(Color cor, double duracao) {
    piscadaCor = cor;
    piscadaDuracao = duracao;
    piscadaInicio = GetTime();
}

void trocarDeTela(int idDaTela) {
    if (idDaTela != telaAtual) {
        telaAtual = idDaTela;
        escurecimentoInicio = GetTime();
    }
}

void escurecer() { escurecimentoInicio = GetTime(); }

void marcarClique(Rectangle r) {
    cliqueRect = r;
    cliqueInicio = GetTime();
}

void desenharSobreposicoes() {
    double agora = GetTime();
    // Botão apertado: escurece um instante ("afunda") e depois brilha.
    double c = agora - cliqueInicio;
    if (c >= 0 && c < 0.22) {
        Color cor = c < 0.06 ? Color{0, 0, 0, 90} : comAlfa({255, 255, 255, 150}, 1.0f - (float)((c - 0.06) / 0.16));
        Rectangle r = cliqueRect;
        if (c < 0.06) { r.x += 2; r.y += 2; r.width -= 4; r.height -= 4; }
        DrawRectangleRounded(r, 0.25f, 8, cor);
    }
    // Piscada da tela.
    double p = agora - piscadaInicio;
    if (p >= 0 && p < piscadaDuracao) {
        DrawRectangle(-50, -50, Ui::LARGURA + 100, Ui::ALTURA + 100,
                      comAlfa(piscadaCor, 0.85f * (1.0f - (float)(p / piscadaDuracao))));
    }
    // Saindo do escuro ao trocar de tela.
    double e = agora - escurecimentoInicio;
    if (e >= 0 && e < DURACAO_DO_ESCURECIMENTO) {
        DrawRectangle(-50, -50, Ui::LARGURA + 100, Ui::ALTURA + 100,
                      comAlfa({8, 10, 20, 255}, 1.0f - (float)(e / DURACAO_DO_ESCURECIMENTO)));
    }
}

Rectangle destaqueDeslizante(Rectangle alvo) {
    static Rectangle atual = {0, 0, 0, 0};
    static double ultimo = 0;
    double agora = GetTime();
    float dt = (float)(agora - ultimo);
    ultimo = agora;
    // Outro menu (tamanho diferente) ou muito tempo parado: começa no lugar.
    bool outroMenu = std::fabs(alvo.width - atual.width) > 2 || std::fabs(alvo.x - atual.x) > 300 || dt > 0.25f;
    if (outroMenu) {
        atual = alvo;
        return atual;
    }
    float k = 1.0f - std::exp(-dt * 22.0f);
    atual.x += (alvo.x - atual.x) * k;
    atual.y += (alvo.y - atual.y) * k;
    atual.width = alvo.width;
    atual.height = alvo.height;
    return atual;
}

float aproximar(float atual, float alvo, float velocidade, float dt) {
    return atual + (alvo - atual) * (1.0f - std::exp(-dt * velocidade));
}

}
