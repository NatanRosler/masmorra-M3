// Substitui apenas o terminal por entrada/saída da raylib. As regras do jogo
// continuam nos arquivos jogo.cpp, batalha.cpp, monstros.cpp, itens.cpp etc.
#include "terminal.h"
#include "visual.h"
#include "audio.h"
#include "efeitos.h"
#include "salvar.h"
#include "raylib.h"
#include <functional>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <string>
#include <thread>
#include <vector>
#include <utility>

namespace {
struct Opcao { Rectangle r; int indice; int cursor; };
struct Atalho { Rectangle r; int tecla; };
std::vector<Atalho> atalhos;
std::vector<Opcao> opcoes;
std::deque<int> pendentes;
bool modoTexto = false;   // digitando o nome: W A S D não viram movimento
std::function<void()> redesenho;   // a última tela desenhada (ver definirRedesenho)

// Redesenha a última tela. Usa uma CÓPIA da função: assim a tela pode
// registrar a si mesma de novo (definirRedesenho) sem apagar quem está rodando.
void redesenhar() {
    if (!redesenho) { std::this_thread::sleep_for(std::chrono::milliseconds(7)); PollInputEvents(); return; }
    std::function<void()> copia = redesenho;
    copia();
}

// Mostra o botão "afundando" por um instante antes de executar a escolha.
void mostrarClique(Rectangle r) {
    Efeitos::marcarClique(r);
    double inicio = GetTime();
    while (GetTime() - inicio < 0.11) {
        redesenhar();
    }
}
}
void definirRedesenho(std::function<void()> funcao) { redesenho = std::move(funcao); }
void definirModoTexto(bool ligado) { modoTexto = ligado; }
void registrarOpcao(Rectangle r, int indice, int cursor) { opcoes.push_back({r,indice,cursor}); }
void registrarAtalho(Rectangle r, int tecla) { atalhos.push_back({r,tecla}); }
void limparOpcoes() { opcoes.clear(); atalhos.clear(); }

long long relogioMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
void iniciarTerminal() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(Ui::LARGURA, Ui::ALTURA, "MASMORRA | Exploracao original em 2D");
    SetWindowMinSize(960,600);
    SetExitKey(KEY_NULL); // Esc é usada nos menus; fechar pela janela continua funcionando.
    SetTargetFPS(60);
    Ui::iniciarRecursos();
    iniciarAudio();
}
void finalizarTerminal() {
    if (!IsWindowReady()) return;
    Ui::finalizarRecursos();
    finalizarAudio();
    CloseWindow();
}
static int teclaDoFrame() {
    if (WindowShouldClose()) {
        // A batalha e os menus antigos esperam por teclas em loops internos.
        // Encerrar aqui evita ficar preso caso a janela seja fechada neles.
        // Antes, a partida é salva para o "Continuar" do menu.
        salvarPartidaEmAndamento();
        finalizarTerminal();
        std::exit(0);
    }
    if(!pendentes.empty()) {int t=pendentes.front();pendentes.pop_front();return t;}
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) || IsKeyPressed(KEY_KP_8)) return TECLA_CIMA;
    if (IsKeyPressed(KEY_DOWN)|| IsKeyPressedRepeat(KEY_DOWN) || IsKeyPressed(KEY_KP_2)) return TECLA_BAIXO;
    if (IsKeyPressed(KEY_LEFT)|| IsKeyPressedRepeat(KEY_LEFT) || IsKeyPressed(KEY_KP_4)) return TECLA_ESQUERDA;
    if (IsKeyPressed(KEY_RIGHT)|| IsKeyPressedRepeat(KEY_RIGHT) || IsKeyPressed(KEY_KP_6)) return TECLA_DIREITA;
    if (IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_KP_7)) return TECLA_HOME;
    if (IsKeyPressed(KEY_PAGE_UP) || IsKeyPressed(KEY_KP_9)) return TECLA_PAGE_UP;
    if (IsKeyPressed(KEY_END) || IsKeyPressed(KEY_KP_1)) return TECLA_END;
    if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressed(KEY_KP_3)) return TECLA_PAGE_DOWN;
    if (IsKeyPressed(KEY_ESCAPE)) return TECLA_ESC;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        // Enter num menu: a opção selecionada "afunda" antes de executar.
        for (const auto& o:opcoes) if (o.indice>=0 && o.indice==o.cursor) { mostrarClique(o.r); break; }
        return TECLA_ENTER;
    }
    if (IsKeyPressed(KEY_BACKSPACE)) return 8;
    // WASD opcional para exploracao. A logica do jogo continua recebendo HJKL.
    if (!modoTexto) {
        if (IsKeyPressed(KEY_W) || IsKeyPressedRepeat(KEY_W)) return 'k';
        if (IsKeyPressed(KEY_A) || IsKeyPressedRepeat(KEY_A)) return 'h';
        if (IsKeyPressed(KEY_S) || IsKeyPressedRepeat(KEY_S)) return 'j';
        if (IsKeyPressed(KEY_D) || IsKeyPressedRepeat(KEY_D)) return 'l';
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 p=Ui::mouseVirtual();
        for (const auto& a:atalhos) if (CheckCollisionPointRec(p,a.r)) { Rectangle r=a.r; int t=a.tecla; mostrarClique(r); return t; }
        for (const auto& o:opcoes) {
            if (CheckCollisionPointRec(p,o.r)) {
                Opcao escolhida=o;
                mostrarClique(escolhida.r);
                if (escolhida.indice>=0) return TECLA_CLIQUE_MENU+escolhida.indice;
                return TECLA_ENTER;
            }
        }
    }
    int ch=GetCharPressed();
    if(ch>=32 && ch<=126) return ch;
    return TECLA_NENHUMA;
}
int lerTeclaComEspera(int esperaMs) {
    long long inicio=relogioMs();
    do {
        int t=teclaDoFrame();
        if (t!=TECLA_NENHUMA) return t;
        if (esperaMs>=0 && relogioMs()-inicio>=esperaMs) return TECLA_NENHUMA;
        // Enquanto espera, redesenha a tela: as animações continuam rodando.
        // (EndDrawing já lê o teclado e segura o ritmo em 60 quadros/s.)
        redesenhar();
    } while (true);
}
int lerTecla() {return lerTeclaComEspera(-1);}
void esperarMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); PollInputEvents(); }
bool tamanhoDoTerminal(int& colunas,int& linhas) {colunas=80;linhas=24;return true;}
void escreverNaTela(const std::string&) {} // compatibilidade com terminal.h antigo
std::string ansiPosicao(int,int) {return "";}
std::string ansiCor(int,int) {return "";}
std::string ansiResetar() {return "";}
