// terminal.cpp
// -----------------------------------------------------------------------------
// Implementação das funções de terminal para cada sistema operacional.
//
// O bloco "#ifdef _WIN32" só é compilado no Windows; o bloco "#else" só é
// compilado no Linux e no Mac. O compilador escolhe sozinho.
// -----------------------------------------------------------------------------
#include "terminal.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

// Sequências ANSI usadas ao entrar e sair do jogo.
//   ?1049h / ?1049l : liga / desliga a "tela alternativa" (ao sair, o terminal
//                     volta a mostrar o que tinha antes de abrir o jogo)
//   ?25l   / ?25h   : esconde / mostra o cursor piscando
//   2J              : limpa a tela
static const char* ANSI_ENTRAR = "\x1b[?1049h\x1b[?25l\x1b[0m\x1b[48;5;16m\x1b[2J\x1b[H";
static const char* ANSI_SAIR   = "\x1b[0m\x1b[2J\x1b[H\x1b[?25h\x1b[?1049l";

static bool terminalAtivo = false;

// No terminal as letras já chegam como letras: nada a fazer.
void definirModoTexto(bool) {}

bool shiftSegurado() { return false; }

long long relogioMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

int lerTecla() {
    return lerTeclaComEspera(-1);
}

void escreverNaTela(const std::string& texto) {
    std::fwrite(texto.data(), 1, texto.size(), stdout);
    std::fflush(stdout);
}

std::string ansiPosicao(int linha, int coluna) {
    return "\x1b[" + std::to_string(linha) + ";" + std::to_string(coluna) + "H";
}

std::string ansiCor(int frente, int fundo) {
    return "\x1b[38;5;" + std::to_string(frente) + ";48;5;" + std::to_string(fundo) + "m";
}

std::string ansiResetar() {
    return "\x1b[0m";
}

// =============================================================================
#ifdef _WIN32
// =============================================================================
// WINDOWS: usa a API do console (windows.h) e _getch() da conio.h.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>

// Compiladores MinGW antigos não definem esta constante.
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

void irParaAPastaDoPrograma() {
    char caminho[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, caminho, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return;
    std::string pasta(caminho, n);
    size_t barra = pasta.find_last_of("\\/");
    if (barra != std::string::npos) SetCurrentDirectoryA(pasta.substr(0, barra).c_str());
}

static DWORD modoOriginalSaida = 0;
static bool temModoOriginal = false;
static UINT paginaDeCodigoOriginal = 0;

// Chamado pelo Windows quando o jogador aperta Ctrl+C ou fecha a janela.
static BOOL WINAPI aoFecharConsole(DWORD /*tipo*/) {
    finalizarTerminal();
    return FALSE; // FALSE = deixa o Windows encerrar o programa normalmente
}

void iniciarTerminal() {
    if (terminalAtivo) return;

    HANDLE saida = GetStdHandle(STD_OUTPUT_HANDLE);

    // UTF-8 permite usar símbolos especiais nas próximas etapas.
    paginaDeCodigoOriginal = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);

    // Liga os códigos ANSI (cores e cursor) no console do Windows 10/11.
    if (GetConsoleMode(saida, &modoOriginalSaida)) {
        temModoOriginal = true;
        DWORD novoModo = modoOriginalSaida | ENABLE_PROCESSED_OUTPUT
                                           | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(saida, novoModo)) {
            std::fprintf(stderr,
                "Este console nao suporta cores ANSI. Use o Windows 10 ou mais novo.\n");
        }
    }

    SetConsoleTitleA("Masmorra");
    SetConsoleCtrlHandler(aoFecharConsole, TRUE);
    std::atexit(finalizarTerminal);

    // Buffer grande: a tela inteira sai numa escrita só.
    std::setvbuf(stdout, nullptr, _IOFBF, 1 << 16);

    terminalAtivo = true;
    escreverNaTela(ANSI_ENTRAR);
}

void finalizarTerminal() {
    if (!terminalAtivo) return;
    terminalAtivo = false;

    escreverNaTela(ANSI_SAIR);
    if (temModoOriginal) {
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), modoOriginalSaida);
    }
    if (paginaDeCodigoOriginal != 0) {
        SetConsoleOutputCP(paginaDeCodigoOriginal);
    }
}

int lerTeclaComEspera(int esperaMs) {
    // Enquanto nenhuma tecla é apertada, confere de pouco em pouco se o tempo
    // acabou e se a janela mudou de tamanho (maximizar não gera tecla).
    int colunasAntes = 0, linhasAntes = 0;
    bool sabeTamanho = tamanhoDoTerminal(colunasAntes, linhasAntes);
    long long limite = relogioMs() + esperaMs;
    while (!_kbhit()) {
        if (esperaMs >= 0 && relogioMs() >= limite) return TECLA_NENHUMA;
        Sleep(5);
        int colunas, linhas;
        if (sabeTamanho && tamanhoDoTerminal(colunas, linhas) &&
            (colunas != colunasAntes || linhas != linhasAntes)) {
            return TECLA_REDIMENSIONOU;
        }
    }

    int c = _getch();

    // Setas e teclas especiais chegam em DOIS códigos: primeiro 0 ou 224,
    // depois o código da tecla.
    if (c == 0 || c == 224) {
        int c2 = _getch();
        switch (c2) {
            case 72: return TECLA_CIMA;
            case 80: return TECLA_BAIXO;
            case 75: return TECLA_ESQUERDA;
            case 77: return TECLA_DIREITA;
            case 71: return TECLA_HOME;
            case 73: return TECLA_PAGE_UP;
            case 79: return TECLA_END;
            case 81: return TECLA_PAGE_DOWN;
            default: return TECLA_NENHUMA;
        }
    }
    if (c == '\r' || c == '\n') return TECLA_ENTER;
    return c;
}

void esperarMs(int milissegundos) {
    Sleep((DWORD)milissegundos);
}

bool tamanhoDoTerminal(int& colunas, int& linhas) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        return false;
    }
    colunas = info.srWindow.Right - info.srWindow.Left + 1;
    linhas  = info.srWindow.Bottom - info.srWindow.Top + 1;
    return true;
}

// =============================================================================
#else
// =============================================================================
// LINUX e MAC: usa termios (configuração do terminal) e read().

#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

void irParaAPastaDoPrograma() {
    char caminho[4096];
#ifdef __APPLE__
    uint32_t tamanho = sizeof(caminho);
    if (_NSGetExecutablePath(caminho, &tamanho) != 0) return;
    std::string pasta(caminho);
#else
    ssize_t n = readlink("/proc/self/exe", caminho, sizeof(caminho) - 1);
    if (n <= 0) return;
    std::string pasta(caminho, (size_t)n);
#endif
    size_t barra = pasta.find_last_of('/');
    if (barra != std::string::npos && barra > 0 && chdir(pasta.substr(0, barra).c_str()) != 0) { /* fica onde está */ }
}

static struct termios configOriginal;
static bool temConfigOriginal = false;

// Chamado quando o jogador aperta Ctrl+C: arruma o terminal e sai.
static void aoReceberSinal(int /*sinal*/) {
    if (write(STDOUT_FILENO, ANSI_SAIR, std::strlen(ANSI_SAIR)) < 0) { /* ignora */ }
    if (temConfigOriginal) tcsetattr(STDIN_FILENO, TCSAFLUSH, &configOriginal);
    _exit(130);
}

void iniciarTerminal() {
    if (terminalAtivo) return;

    // Modo "cru": cada tecla chega na hora, sem Enter, e sem aparecer na tela.
    if (tcgetattr(STDIN_FILENO, &configOriginal) == 0) {
        temConfigOriginal = true;
        struct termios cru = configOriginal;
        cru.c_lflag &= ~(ICANON | ECHO);   // sem esperar Enter, sem eco
        cru.c_iflag &= ~(IXON | ICRNL);    // Ctrl+S/Ctrl+Q livres; Enter = '\r'
        cru.c_cc[VMIN] = 1;                // read() espera pelo menos 1 byte
        cru.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &cru);
    }

    signal(SIGINT, aoReceberSinal);
    signal(SIGTERM, aoReceberSinal);
    signal(SIGHUP, aoReceberSinal);
    std::atexit(finalizarTerminal);

    std::setvbuf(stdout, nullptr, _IOFBF, 1 << 16);

    terminalAtivo = true;
    escreverNaTela("\x1b]0;Masmorra\x07"); // título da janela
    escreverNaTela(ANSI_ENTRAR);
}

void finalizarTerminal() {
    if (!terminalAtivo) return;
    terminalAtivo = false;

    escreverNaTela(ANSI_SAIR);
    if (temConfigOriginal) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &configOriginal);
    }
}

// Lê um byte do teclado. Se esperaMs >= 0, desiste depois desse tempo
// (devolve -1); usado para separar "ESC sozinho" de "ESC + código de seta".
static int lerByte(int esperaMs) {
    if (esperaMs >= 0) {
        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, esperaMs) <= 0) return -1;
    }
    unsigned char c;
    while (true) {
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1) return c;
        if (n < 0 && errno == EINTR) continue;
        // Terminal fechado (fim da entrada): não há mais o que ler, encerra.
        finalizarTerminal();
        std::exit(0);
    }
}

int lerTeclaComEspera(int esperaMs) {
    // Espera a tecla em pedaços de até 30 ms; entre um e outro, confere se o
    // tempo acabou e se a janela mudou de tamanho.
    int colunasAntes = 0, linhasAntes = 0;
    bool sabeTamanho = tamanhoDoTerminal(colunasAntes, linhasAntes);
    long long limite = relogioMs() + esperaMs;
    int c;
    while (true) {
        int pedaco = 30;
        if (esperaMs >= 0) {
            long long resta = limite - relogioMs();
            if (resta <= 0) return TECLA_NENHUMA;
            if (resta < pedaco) pedaco = (int)resta;
        }
        c = lerByte(pedaco);
        if (c >= 0) break;
        int colunas, linhas;
        if (sabeTamanho && tamanhoDoTerminal(colunas, linhas) &&
            (colunas != colunasAntes || linhas != linhasAntes)) {
            return TECLA_REDIMENSIONOU;
        }
    }

    // Setas chegam como ESC [ A, ESC [ B... (ou ESC O A em alguns terminais).
    if (c == 27) {
        int c1 = lerByte(30);
        if (c1 != '[' && c1 != 'O') return TECLA_ESC;
        int c2 = lerByte(30);
        switch (c2) {
            case 'A': return TECLA_CIMA;
            case 'B': return TECLA_BAIXO;
            case 'D': return TECLA_ESQUERDA;
            case 'C': return TECLA_DIREITA;
            case 'H': return TECLA_HOME;
            case 'F': return TECLA_END;
        }
        // Home/End/PageUp/PageDown: ESC [ 1 ~, ESC [ 4 ~, ESC [ 5 ~, ESC [ 6 ~
        if (c2 >= '0' && c2 <= '9') {
            int c3 = lerByte(30);
            if (c3 == '~') {
                switch (c2) {
                    case '1': case '7': return TECLA_HOME;
                    case '4': case '8': return TECLA_END;
                    case '5': return TECLA_PAGE_UP;
                    case '6': return TECLA_PAGE_DOWN;
                }
            }
        }
        return TECLA_NENHUMA;
    }
    if (c == '\r' || c == '\n') return TECLA_ENTER;
    return c;
}

void esperarMs(int milissegundos) {
    poll(nullptr, 0, milissegundos); // poll sem nada para vigiar = só espera
}

bool tamanhoDoTerminal(int& colunas, int& linhas) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) {
        return false;
    }
    colunas = ws.ws_col;
    linhas = ws.ws_row;
    return true;
}

#endif
