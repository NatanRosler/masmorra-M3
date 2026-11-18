// monstros.cpp
#include "monstros.h"
#include "jogo.h"
#include "aleatorio.h"
#include "cenario.h"
#include "npcs.h"
#include "dificuldade.h"

#include <cmath>

// =============================================================================
// FICHAS DAS ESPÉCIES
// =============================================================================
// Os atributos-base funcionam como nos Pokémon: o valor de verdade cresce com o
// nível. Ex.: ataque = 2 x base x nível / 100 + 5.
const TipoMonstro TIPOS_DE_MONSTRO[] = {
    { "rato", "o", 'r', { 170, 140, 110 }, NORMAL,
      35, 50, 35, 70,   45,
      { G_MORDIDA, G_ROSNADO, G_ATAQUE_RAPIDO }, 3,
      { "",
        "     _     _",
        "    (o)___(o)",
        "     ( o.o )____",
        "      (=Y=)     )~~~~",
        "      ^^  ^^^^^^",
        "" },
      1, 4, 30, 0 },

    { "kobold", "o", 'k', { 210, 140, 80 }, FOGO,
      40, 55, 50, 40,   55,
      { G_BRASA, G_ARRANHAO, G_ROSNADO }, 3,
      { "          (",
        "     /\\  )*)",
        "    (o>\\ |",
        "    /|_|\\|",
        "     |_|  |",
        "     / \\",
        "    ^   ^" },
      1, 4, 30, 0 },

    { "chacal", "o", 'j', { 215, 175, 105 }, NORMAL,
      40, 60, 40, 85,   60,
      { G_MORDIDA, G_ATAQUE_RAPIDO }, 2,
      { "",
        "    /\\___/\\",
        "   /  o o  \\_____",
        "   \\   w   /     \\~",
        "    \\_____/|_|_|_|",
        "",
        "" },
      2, 6, 20, RAPIDO | EM_BANDO },

    { "macaco", "o", 'm', { 175, 125, 80 }, PLANTA,
      50, 55, 45, 75,   70,
      { G_ARRANHAO, G_FOLHA_NAVALHA, G_ROSNADO }, 3,
      { "      .--.",
        "    _/ oo \\_",
        "   (  (__)  )",
        "    \\  ==  /",
        "    /|    |\\",
        "   ' |____| '",
        "" },
      2, 6, 15, MEDROSO },

    { "sapo", "o", 't', { 110, 170, 200 }, AGUA,
      60, 50, 55, 40,   75,
      { G_JATO_DAGUA, G_LINGUA_GRUDENTA }, 2,
      { "",
        "     (o)    (o)",
        "    /  ------  \\",
        "   (   \\____/   )",
        "    \\_/      \\_/",
        "",
        "" },
      2, 6, 15, 0 },

    { "goblin", "o", 'g', { 120, 190, 90 }, VENENO,
      55, 70, 55, 55,   90,
      { G_FERRAO_VENENOSO, G_GOLPE, G_GRITO_DE_GUERRA }, 3,
      { "    \\/\\/\\/",
        "   <( > < )>   /",
        "     ( ^^ )   /",
        "    --|##|---+",
        "      |##|",
        "      /  \\",
        "     ^    ^" },
      3, 10, 25, 0 },

    { "morcego vampiro", "o", 'v', { 165, 120, 190 }, NORMAL,
      55, 60, 45, 95,   95,
      { G_SUGAR_SANGUE, G_ATAQUE_RAPIDO, G_HIPNOSE }, 3,
      { "",
        "  /\\_          _/\\",
        " /   \\_ (\\_/) _/   \\",
        " \\_    \\(o o)/    _/",
        "   \\___/ \\v/ \\___/",
        "",
        "" },
      4, 9, 20, RAPIDO | ERRATICO },

    { "ogro", "o", 'O', { 175, 135, 75 }, PEDRA,
       90,  80, 70, 35, 160,
      { G_ESMAGAR, G_RUGIDO, G_GOLPE }, 3,
      { "      ______",
        "     / o  o \\",
        "    |  (__)  |",
        "    |  \\==/  |",
        "   /|________|\\",
        "  (_|  |  |  |_)",
        "     _|    |_" },
      6, 10, 15, QUEBRA_CAIXAS },

    { "naga", "a", 'N', { 70, 185, 165 }, AGUA,
      95, 75, 80, 70,  180,
      { G_CHICOTE_DAGUA, G_HIPNOSE, G_FERRAO_VENENOSO }, 3,
      { "      .----.",
        "     / o  o \\",
        "     \\  ~~  /___",
        "      \\____/    \\",
        "       \\___   ___)",
        "           \\_/~~",
        "" },
      8, 10, 10, 0 },

    // O CHEFE FINAL: nunca aparece por sorteio (frequência 0); é colocado no
    // último andar por colocarChefe().
    { "Senhor das Profundezas", "o", 'S', { 200, 60, 70 }, NORMAL,
      100, 85, 75, 70,  400,
      { G_ESMAGAR, G_GOLPE_SOMBRIO, G_RUGIDO, G_HIPNOSE }, 4,
      { "     /\\_/\\____/\\_/\\",
        "    (  @        @  )",
        "     \\   ______   /",
        "    __\\ |VVVVVV| /__",
        "   /   \\|^^^^^^|/   \\",
        "  /  /\\  \\____/  /\\  \\",
        "    /  \\        /  \\" },
      10, 10, 0, CHEFE | QUEBRA_CAIXAS },
};
const int NUM_TIPOS_DE_MONSTRO = sizeof(TIPOS_DE_MONSTRO) / sizeof(TIPOS_DE_MONSTRO[0]);

const CorRGB CORES_DOS_ELEMENTOS[NUM_ELEMENTOS] = {
    { 205, 200, 185 },  // Normal
    { 240, 110,  60 },  // Fogo
    {  80, 150, 240 },  // Água
    {  90, 200,  90 },  // Planta
    { 185,  95, 215 },  // Veneno
    { 175, 140, 100 },  // Pedra
};

int tipoDoChefe() {
    return NUM_TIPOS_DE_MONSTRO - 1;
}

// =============================================================================
// ELEMENTOS SORTEADOS (Etapa 6)
// =============================================================================
void sortearElementosDasEspecies(Jogo& jogo, uint32_t semente) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    int saco[NUM_ELEMENTOS];
    int restantes = 0;
    // A tabela já está em ordem de andar (do rato ao chefe).
    for (int tipo = 0; tipo < NUM_TIPOS_DE_MONSTRO; tipo++) {
        if (restantes == 0) {
            // Saco novo com os 6 elementos embaralhados (Fisher-Yates).
            for (int e = 0; e < NUM_ELEMENTOS; e++) saco[e] = e;
            for (int e = NUM_ELEMENTOS - 1; e > 0; e--) {
                int k = sortear(g, 0, e);
                int tmp = saco[e]; saco[e] = saco[k]; saco[k] = tmp;
            }
            restantes = NUM_ELEMENTOS;
        }
        jogo.elementoDaEspecie[tipo] = (Elemento)saco[--restantes];
    }
}

void golpesDoMonstro(const Monstro& m, int golpes[MAX_GOLPES], int& numGolpes) {
    const TipoMonstro& t = tipoDe(m);
    numGolpes = t.numGolpes;
    for (int s = 0; s < t.numGolpes; s++) golpes[s] = t.golpes[s];
    // O primeiro golpe de dano "simples" (sem efeito extra) vira o golpe do
    // elemento sorteado. Espécies dos andares fundos ganham a versão forte.
    bool forte = t.andarMinimo >= 4;
    for (int s = 0; s < numGolpes; s++) {
        const Golpe& g = GOLPES[golpes[s]];
        if (g.poder > 0 && g.efeito == SEM_EFEITO) {
            golpes[s] = golpeElementalDosMonstros(m.elemento, forte);
            return;
        }
    }
}

int esquivaDoMonstro(const Monstro& m) {
    const TipoMonstro& t = tipoDe(m);
    int esquiva = 5;
    if (t.especial & (RAPIDO | ERRATICO)) esquiva += 10;   // chacais e morcegos são ágeis
    return esquiva;
}

// Até onde um monstro acordado percebe o jogador (em passos). Quando o
// jogador descansa, faz menos barulho e o alcance cai pela metade.
const int ALCANCE_DE_PERCEPCAO = 14;

// Quantos turnos um monstro caçando segue o rastro antes de desistir.
const int PACIENCIA_DA_CACADA = 20;

const TipoMonstro& tipoDe(const Monstro& m) {
    return TIPOS_DE_MONSTRO[m.tipo];
}

// A vida dos monstros segue a mesma fórmula do jogador, mais o ajuste da
// dificuldade (ex.: +30% no difícil, -15% no fácil).
int vidaMaximaDoMonstro(const Monstro& m) {
    // A dificuldade aumenta (ou diminui) a vida de todos os monstros.
    return vidaPeloNivel(tipoDe(m).baseVida, m.nivel) * (100 + dificuldadeAtual().vidaDosMonstros) / 100;
}

int monstroEm(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numMonstros; i++) {
        const Monstro& m = andar.monstros[i];
        if (m.vivo && m.x == x && m.y == y) return i;
    }
    return -1;
}

std::string nomeDoMonstro(const Jogo& jogo, const Monstro& m) {
    if (!jogo.visivel[m.y][m.x]) return "algo";
    const TipoMonstro& t = tipoDe(m);
    return std::string(t.artigo) + " " + t.nome;
}

// "o rato" -> "O rato" (para começar frases).
static std::string comMaiuscula(std::string texto) {
    if (!texto.empty() && texto[0] >= 'a' && texto[0] <= 'z') texto[0] = texto[0] - 'a' + 'A';
    return texto;
}

// =============================================================================
// POVOAMENTO
// =============================================================================
// Sorteia uma espécie entre as que aparecem neste andar. Espécies com
// frequência maior têm mais chance (sorteio "com pesos").
static int sortearTipo(GeradorAleatorio& g, int numeroDoAndar) {
    int total = 0;
    for (int i = 0; i < NUM_TIPOS_DE_MONSTRO; i++) {
        const TipoMonstro& t = TIPOS_DE_MONSTRO[i];
        if (numeroDoAndar >= t.andarMinimo && numeroDoAndar <= t.andarMaximo) total += t.frequencia;
    }
    if (total == 0) return -1;

    int r = sortear(g, 1, total);
    for (int i = 0; i < NUM_TIPOS_DE_MONSTRO; i++) {
        const TipoMonstro& t = TIPOS_DE_MONSTRO[i];
        if (numeroDoAndar < t.andarMinimo || numeroDoAndar > t.andarMaximo) continue;
        r -= t.frequencia;
        if (r <= 0) return i;
    }
    return -1;
}

static void criarMonstro(Andar& andar, int tipo, int x, int y, int nivel, EstadoMonstro estado) {
    Monstro& m = andar.monstros[andar.numMonstros];
    andar.numMonstros++;
    m.vivo = true;
    m.tipo = tipo;
    m.x = x;
    m.y = y;
    m.nivel = nivel;
    m.vida = vidaMaximaDoMonstro(m);
    m.elemento = TIPOS_DE_MONSTRO[tipo].elemento;   // trocado pelo sorteado em jogo.cpp
    m.turnosSemBatalha = 0;
    m.atordoadoMs = 0;
    m.carregaChave = false;
    // Começam com relógios diferentes, para não andarem todos juntos.
    m.tempoAcumulado = (andar.numMonstros * 137) % 400;
    m.estado = estado;
    m.destinoX = x;
    m.destinoY = y;
    m.turnosSemVerJogador = 0;
}

void povoarAndar(Andar& andar, int numeroDoAndar, uint32_t semente,
                 int chegadaX, int chegadaY) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    andar.numMonstros = 0;

    // Monstros nascem a pelo menos 12 passos da chegada do jogador.
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, chegadaX, chegadaY, distancia);

    int quantidade = 4 + numeroDoAndar / 2 + sortear(g, 0, 2);
    for (int i = 0; i < quantidade && andar.numMonstros < MAX_MONSTROS; i++) {
        int tipo = sortearTipo(g, numeroDoAndar);
        if (tipo < 0) break;
        // 2 em cada 3 monstros começam dormindo; os outros vagam.
        EstadoMonstro estado = chance(g, 67) ? DORMINDO : VAGANDO;
        // Nível: cerca de 2 por andar (andar 1: níveis 1 a 3; andar 10: 19 a 21).
        int nivel = 2 * numeroDoAndar + sortear(g, -1, 1);

        for (int tentativa = 0; tentativa < 300; tentativa++) {
            int x = sortear(g, 1, MAPA_LARGURA - 2);
            int y = sortear(g, 1, MAPA_ALTURA - 2);
            if (andar.mapa.celulas[y][x] != CHAO) continue;
            if (distancia[y][x] == INALCANCAVEL || distancia[y][x] < 12) continue;
            if (monstroEm(andar, x, y) >= 0) continue;

            criarMonstro(andar, tipo, x, y, nivel, estado);

            // Espécies em bando ganham um companheiro ao lado (às vezes dois).
            if (TIPOS_DE_MONSTRO[tipo].especial & EM_BANDO) {
                int extras = chance(g, 25) ? 2 : 1;
                for (int d = 0; d < 8 && extras > 0 && andar.numMonstros < MAX_MONSTROS; d++) {
                    int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
                    if (andar.mapa.celulas[ny][nx] != CHAO) continue;
                    if (distancia[ny][nx] == INALCANCAVEL || distancia[ny][nx] < 12) continue;
                    if (monstroEm(andar, nx, ny) >= 0) continue;
                    criarMonstro(andar, tipo, nx, ny, nivel, estado);
                    extras--;
                }
            }
            break;
        }
    }
}

void colocarChefe(Andar& andar, int chegadaX, int chegadaY, Elemento elemento) {
    // O chefe espera no ponto mais distante da chegada.
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, chegadaX, chegadaY, distancia);
    int melhorX = -1, melhorY = -1, maior = -1;
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (andar.mapa.celulas[y][x] != CHAO || distancia[y][x] == INALCANCAVEL) continue;
            if (monstroEm(andar, x, y) >= 0) continue;
            if (distancia[y][x] > maior) { maior = distancia[y][x]; melhorX = x; melhorY = y; }
        }
    }
    if (melhorX < 0 || andar.numMonstros >= MAX_MONSTROS) return;
    criarMonstro(andar, tipoDoChefe(), melhorX, melhorY, 24, DORMINDO);
    andar.monstros[andar.numMonstros - 1].elemento = elemento;
}

// =============================================================================
// INTELIGÊNCIA ARTIFICIAL
// =============================================================================

// O monstro pode pisar em (x, y)? (sem paredes, escadas, outros monstros ou o jogador)
static bool celulaLivre(const Jogo& jogo, int x, int y) {
    const Andar& andar = andarAtual(jogo);
    if (!ehPassavel(andar.mapa, x, y) || ehEscada(andar.mapa, x, y)) return false;
    if (x == jogo.jogador.x && y == jogo.jogador.y) return false;
    if (npcEm(andar, x, y) >= 0) return false;
    return monstroEm(andar, x, y) < 0;
}

static bool podeAtacarJogador(const Jogo& jogo, const Monstro& m) {
    int dx = jogo.jogador.x - m.x;
    int dy = jogo.jogador.y - m.y;
    if (dx < -1 || dx > 1 || dy < -1 || dy > 1 || (dx == 0 && dy == 0)) return false;
    // Mesma regra do jogador: não ataca "cortando a quina" de uma parede.
    return podeMover(mapaAtual(jogo), m.x, m.y, dx, dy);
}

// Função usada por TODOS os movimentos: olha os 8 vizinhos num "mapa de
// valores" e vai para o melhor, se ele for melhor que a célula atual.
//   - perseguir: valores = distância até o jogador, quero o MENOR
//   - fugir:     valores = distância até o jogador, quero o MAIOR
//   - farejar:   valores = rastro de cheiro,        quero o MAIOR
//   - vagar:     valores = distância até o destino, quero o MENOR
// Devolve false se não houve movimento.
static bool moverParaMelhorVizinho(Jogo& jogo, Monstro& m,
                                   const int valores[MAPA_ALTURA][MAPA_LARGURA],
                                   bool queroMenor) {
    const Mapa& mapa = mapaAtual(jogo);
    int melhorValor = valores[m.y][m.x];
    int melhorD = -1;

    for (int d = 0; d < 8; d++) {
        int dx = DIRECOES_DX[d], dy = DIRECOES_DY[d];
        int nx = m.x + dx, ny = m.y + dy;
        if (!podeMover(mapa, m.x, m.y, dx, dy)) continue;
        if (!celulaLivre(jogo, nx, ny)) continue;

        int v = valores[ny][nx];
        if (v == INALCANCAVEL) continue;
        bool melhor = queroMenor ? (melhorValor == INALCANCAVEL || v < melhorValor)
                                 : (v > melhorValor);
        if (melhor) {
            melhorValor = v;
            melhorD = d;
        }
    }
    if (melhorD < 0) return false;
    m.x += DIRECOES_DX[melhorD];
    m.y += DIRECOES_DY[melhorD];
    return true;
}

static void moverAoAcaso(Jogo& jogo, Monstro& m) {
    const Mapa& mapa = mapaAtual(jogo);
    int primeira = sortear(jogo.sorte, 0, 7);
    for (int k = 0; k < 8; k++) {
        int d = (primeira + k) % 8;
        int dx = DIRECOES_DX[d], dy = DIRECOES_DY[d];
        if (podeMover(mapa, m.x, m.y, dx, dy) && celulaLivre(jogo, m.x + dx, m.y + dy)) {
            m.x += dx;
            m.y += dy;
            return;
        }
    }
}

static void escolherDestino(Jogo& jogo, Monstro& m) {
    const Mapa& mapa = mapaAtual(jogo);
    for (int tentativa = 0; tentativa < 100; tentativa++) {
        int x = sortear(jogo.sorte, 1, MAPA_LARGURA - 2);
        int y = sortear(jogo.sorte, 1, MAPA_ALTURA - 2);
        if (mapa.celulas[y][x] == CHAO) {
            m.destinoX = x;
            m.destinoY = y;
            return;
        }
    }
}

static void vagar(Jogo& jogo, Monstro& m) {
    if ((m.x == m.destinoX && m.y == m.destinoY) || chance(jogo.sorte, 2)) {
        escolherDestino(jogo, m);
    }
    if ((tipoDe(m).especial & ERRATICO) && chance(jogo.sorte, 33)) {
        moverAoAcaso(jogo, m);
        return;
    }
    static int distanciaAteDestino[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(mapaAtual(jogo), m.destinoX, m.destinoY, distanciaAteDestino);
    if (!moverParaMelhorVizinho(jogo, m, distanciaAteDestino, true)) {
        escolherDestino(jogo, m); // caminho bloqueado: tenta outro lugar
    }
}

// Uma ação do monstro. Devolve true se ele alcançou o jogador (começa a batalha).
static bool agirUmaVez(Jogo& jogo, Monstro& m, int indice) {
    const TipoMonstro& t = tipoDe(m);
    Andar& andar = andarAtual(jogo);
    int distancia = jogo.distanciaAteJogador[m.y][m.x];

    // O monstro vê o jogador se o jogador vê a célula dele (a linha de visão
    // vale para os dois lados). O jogador carrega luz, então é sempre visto.
    bool vejoJogador = jogo.linhaDeVisao[m.y][m.x] && distancia != INALCANCAVEL;
    int alcance = jogo.descansando ? ALCANCE_DE_PERCEPCAO / 2 : ALCANCE_DE_PERCEPCAO;

    // ---- DORMINDO: pode acordar se o jogador estiver bem perto ----------
    if (m.estado == DORMINDO) {
        if (vejoJogador && distancia <= alcance / 2 && chance(jogo.sorte, 33)) {
            m.estado = CACANDO;
            m.turnosSemVerJogador = 0;
            if (jogo.visivel[m.y][m.x]) {
                adicionarMensagem(jogo, comMaiuscula(nomeDoMonstro(jogo, m)) + " acorda!");
            }
        }
        return false;
    }

    // ---- VAGANDO: anda por aí até perceber o jogador -----------------------
    if (m.estado == VAGANDO) {
        if (vejoJogador && distancia <= alcance) {
            m.estado = CACANDO;
            m.turnosSemVerJogador = 0;
            if (jogo.visivel[m.y][m.x]) {
                adicionarMensagem(jogo, comMaiuscula(nomeDoMonstro(jogo, m)) + " percebe você!");
            }
        } else {
            vagar(jogo, m);
            return false;
        }
    }

    // Medrosos fogem quando ficam com 1/4 da vida ou menos.
    int vidaMaxima = vidaMaximaDoMonstro(m);
    if ((t.especial & MEDROSO) && m.vida * 4 <= vidaMaxima) {
        m.estado = FUGINDO;
    }

    // ---- FUGINDO: vai para onde a distância até o jogador aumenta ----------
    // Enquanto foge, se recupera aos poucos; com metade da vida, volta a caçar.
    if (m.estado == FUGINDO) {
        if (chance(jogo.sorte, 20) && m.vida < vidaMaxima) m.vida++;
        if (m.vida * 2 > vidaMaxima) {
            m.estado = CACANDO;
            m.turnosSemVerJogador = 0;
            return false;
        }
        moverParaMelhorVizinho(jogo, m, jogo.distanciaAteJogador, false);
        return false;
    }

    // ---- CACANDO -----------------------------------------------------------
    // Alcançou o jogador: começa a batalha.
    if (podeAtacarJogador(jogo, m) && m.turnosSemBatalha == 0) {
        jogo.batalhaPendente = indice;
        return true;
    }
    if ((t.especial & ERRATICO) && chance(jogo.sorte, 33)) {
        moverAoAcaso(jogo, m);
        return false;
    }
    if (vejoJogador) {
        // Vê o jogador: segue o caminho mais curto (BFS a partir do jogador).
        m.turnosSemVerJogador = 0;
        moverParaMelhorVizinho(jogo, m, jogo.distanciaAteJogador, true);
    } else {
        // Perdeu de vista: segue o rastro de cheiro. Depois de um tempo, desiste.
        m.turnosSemVerJogador++;
        moverParaMelhorVizinho(jogo, m, andar.cheiro, false);
        if (m.turnosSemVerJogador > PACIENCIA_DA_CACADA) {
            m.estado = VAGANDO;
            escolherDestino(jogo, m);
        }
    }
    return false;
}

int intervaloDoMonstro(const Monstro& m) {
    const TipoMonstro& t = tipoDe(m);
    int velocidade = atributoPeloNivel(t.baseVelocidade, m.nivel);
    int ms = 560 - 6 * velocidade;              // mais veloz = intervalo menor
    if (ms < 220) ms = 220;
    if (t.especial & RAPIDO) ms /= 2;
    if (m.estado == VAGANDO || m.estado == DORMINDO) ms = ms * 3 / 2; // passeando, vai devagar
    return ms * dificuldadeAtual().lentidaoDosMonstros / 100;
}

void agirMonstros(Jogo& jogo, int ms) {
    Andar& andar = andarAtual(jogo);
    for (int i = 0; i < andar.numMonstros && jogo.batalhaPendente < 0; i++) {
        Monstro& m = andar.monstros[i];
        if (!m.vivo) continue;
        // Atordoado por uma caixa: fica parado até passar o tempo.
        if (m.atordoadoMs > 0) {
            m.atordoadoMs -= ms;
            if (m.atordoadoMs < 0) m.atordoadoMs = 0;
            continue;
        }
        m.tempoAcumulado += ms;
        // Age quantas vezes o tempo acumulado permitir (normalmente uma).
        while (m.vivo && m.tempoAcumulado >= intervaloDoMonstro(m)) {
            m.tempoAcumulado -= intervaloDoMonstro(m);
            if (m.turnosSemBatalha > 0) m.turnosSemBatalha--;
            // O ogro (e o chefe) despedaça as caixas que estão encostadas nele.
            if ((tipoDe(m).especial & QUEBRA_CAIXAS) && m.estado != DORMINDO) quebrarCaixasPerto(jogo, m);
            int antesX = m.x, antesY = m.y;
            if (agirUmaVez(jogo, m, i)) return; // batalha: os outros esperam
            // Andou para cima de uma armadilha?
            if (m.x != antesX || m.y != antesY) armadilhaPegaMonstro(jogo, m);
        }
    }
}
