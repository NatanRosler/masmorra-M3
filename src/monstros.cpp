// monstros.cpp
#include "monstros.h"
#include "tema.h"
#include "jogo.h"
#include "aleatorio.h"
#include "cenario.h"
#include "npcs.h"
#include "dificuldade.h"
#include "som.h"
#include "tela.h"

#include <cmath>
#include <cstdlib>

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
      3, 9, 25, 0 },

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
      6, 12, 20, RAPIDO | ERRATICO },

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
      9, 13, 15, QUEBRA_CAIXAS },

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
      8, 12, 10, 0 },

    // O GNOMO VIGIA: fraco, mas quando vê o jogador sopra a corneta e acorda
    // os monstros da sala dele e das salas vizinhas. Na 1a vez foge para
    // longe; na 2a, sopra de novo e fica esperando a luta (ver tocarCorneta).
    { "gnomo vigia", "o", 'G', { 215, 90, 90 }, NORMAL,
      45, 50, 40, 75,   70,
      { G_ARRANHAO, G_ROSNADO, G_ATAQUE_RAPIDO }, 3,
      { "        /\\",
        "       /  \\",
        "      /____\\    __",
        "      (o  o)  _/ /",
        "      (\\__/)=(__/",
        "      /|  |\\",
        "       d  b" },
      3, 9, 12, VIGIA },

    // A SOMBRA: só aparece de perto (2 casas) ou perto de uma tocha.
    // Na batalha é difícil de acertar, mas tem pouca vida.
    { "sombra", "a", 'W', { 120, 100, 170 }, NORMAL,
      45, 75, 40, 90,  120,
      { G_ARRANHAO, G_HIPNOSE, G_ATAQUE_RAPIDO }, 3,
      { "       .-~~-.",
        "      / O  O \\",
        "     |   ..   |",
        "     |  .  .  |",
        "      \\ ~  ~ /",
        "       ~ ~~ ~",
        "" },
      7, 12, 12, SOMBRIO },

    // ---- v1.23: um monstro novo para cada tema ----------------------------------
    // A ARANHA tece teias pelo mapa; na batalha, a Teia faz perder a vez.
    { "aranha", "a", 'a', { 150, 120, 200 }, VENENO,
      45, 65, 40, 80,   75,
      { G_FERRAO_VENENOSO, G_TEIA, G_MORDIDA }, 3,
      { "   \\  _  //",
        "  \\ (o o) //",
        "   =(  V  )=",
        "  // (___) \\",
        "   //     \\",
        "",
        "" },
      2, 6, 18, TECE_TEIA },
    // O FUNGO fica parado entre os cogumelos da caverna até o jogador chegar perto.
    { "fungo", "o", 'f', { 120, 180, 255 }, PLANTA,
      70, 50, 65, 25,   80,
      { G_ESPOROS, G_ESPINHOS, G_ACIDO }, 3,
      { "    .-'~~'-.",
        "   / o  O  o\\",
        "  '-.______.-'",
        "     | o o|",
        "     |  ~ |",
        "    _|____|_",
        "" },
      4, 6, 15, DISFARCADO },
    // O ESQUELETO anda em dupla e se monta de novo uma vez.
    { "esqueleto", "o", 'e', { 225, 225, 210 }, NORMAL,
      40, 60, 55, 45,  110,
      { G_INVESTIDA, G_GOLPE_DUPLO, G_ROSNADO }, 3,
      { "      .-.",
        "     (o.o)  |",
        "      |=|  -+-",
        "     /|=|\\/ |",
        "      |=|",
        "     _/ \\_",
        "" },
      7, 9, 24, EM_BANDO | REMONTA },
    // A SALAMANDRA é rápida e anda por cima da lava.
    { "salamandra", "a", 's', { 255, 140, 60 }, FOGO,
      60, 70, 50, 95,  140,
      { G_BRASA, G_LABAREDA, G_ATAQUE_RAPIDO }, 3,
      { "",
        "  ~)        __",
        " (~(  ____ /o \\",
        "  \\_/    \\__/",
        "    /_/  /_/",
        "",
        "" },
      10, 12, 20, RAPIDO | ANDA_NA_LAVA },

    // O MÍMICO: um baú dourado que morde. Nunca aparece por sorteio
    // (frequência 0): só nasce quando o jogador abre o baú errado (salas.cpp).
    { "mímico", "o", 'M', { 230, 190, 80 }, NORMAL,
       85,  75, 75, 45, 150,
      { G_MORDIDA, G_ESMAGAR, G_LINGUA_GRUDENTA }, 3,
      { "   ____________",
        "  /   _    _   \\",
        " |   (o)  (o)   |",
        " |\\/\\/\\/\\/\\/\\/\\|",
        " |    ~~~~~~    |",
        " |/\\/\\/\\/\\/\\/\\/|",
        "  \\____________/" },
      1, 10, 0, 0 },

    // O CHEFE FINAL: nunca aparece por sorteio (frequência 0); é colocado no
    // último andar por colocarChefe().
    { "Senhor das Profundezas", "o", 'S', { 200, 60, 70 }, NORMAL,
      100, 85, 75, 70,  400,       // (a vida do chefe é fixa desde a v1.23.1: Dificuldade::vidaDoChefe)
      { G_ESMAGAR, G_GOLPE_SOMBRIO, G_RUGIDO, G_HIPNOSE }, 4,
      { "     /\\_/\\____/\\_/\\",
        "    (  @        @  )",
        "     \\   ______   /",
        "    __\\ |VVVVVV| /__",
        "   /   \\|^^^^^^|/   \\",
        "  /  /\\  \\____/  /\\  \\",
        "    /  \\        /  \\" },
      13, 13, 0, CHEFE | QUEBRA_CAIXAS },
};
const int NUM_TIPOS_DE_MONSTRO = sizeof(TIPOS_DE_MONSTRO) / sizeof(TIPOS_DE_MONSTRO[0]);

const CorRGB CORES_DOS_ELEMENTOS[NUM_ELEMENTOS] = {
    { 205, 200, 185 },  // Normal
    { 240, 110,  60 },  // Fogo
    {  80, 150, 240 },  // Água
    {  90, 200,  90 },  // Planta
    { 185,  95, 215 },  // Veneno
    { 175, 140, 100 },  // Pedra
    { 150, 225, 250 },  // Gelo
    { 245, 210,  40 },  // Raio
    { 120, 112, 175 },  // Trevas
    { 255, 245, 205 },  // Luz
};

int tipoDoChefe() {
    return NUM_TIPOS_DE_MONSTRO - 1;
}

int tipoDoOgro() {
    for (int i = 0; i < NUM_TIPOS_DE_MONSTRO; i++) if (TIPOS_DE_MONSTRO[i].simbolo == 'O') return i;
    return 0;
}

int tipoDoMimico() {
    return NUM_TIPOS_DE_MONSTRO - 2;   // logo antes do chefe na tabela
}


// =============================================================================
// ELEMENTOS SORTEADOS (Etapa 6)
// =============================================================================
// v1.23: o elemento de cada espécie puxa para o tema dos andares onde ela
// vive (7 em 10 vezes um elemento "da casa"; senão, qualquer um). Assim as
// minas têm mais Pedra e Raio, a caverna mais Água e Gelo, a cripta mais
// Trevas, o vulcão mais Fogo. O chefe final é sempre de Trevas.
void sortearElementosDasEspecies(Jogo& jogo, uint32_t semente) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    static const Elemento DO_TEMA[NUM_TEMAS][4] = {
        { PEDRA, RAIO, NORMAL, VENENO },    // minas
        { AGUA, PLANTA, GELO, NORMAL },     // caverna
        { TREVAS, VENENO, GELO, NORMAL },   // cripta
        { FOGO, PEDRA, TREVAS, RAIO },      // vulcão
        { TREVAS, TREVAS, FOGO, PEDRA },    // trono
    };
    for (int tipo = 0; tipo < NUM_TIPOS_DE_MONSTRO; tipo++) {
        const TipoMonstro& t = TIPOS_DE_MONSTRO[tipo];
        int tema = temaDoAndar((t.andarMinimo + t.andarMaximo) / 2);
        Elemento e = chance(g, 70) ? DO_TEMA[tema][sortear(g, 0, 3)] : (Elemento)sortear(g, 0, NUM_ELEMENTOS - 1);
        jogo.elementoDaEspecie[tipo] = e;
    }
    jogo.elementoDaEspecie[tipoDoChefe()] = TREVAS;
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
    if (t.especial & SOMBRIO) esquiva += 20;              // a sombra quase não tem corpo
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
// Os monstros acompanham o jogador (v1.22): nunca ficam mais que a "folga"
// da dificuldade abaixo do nível dele (os campeões, 2 a mais). Quem explora
// tudo sobe de nível rápido; sem isso, os monstros do andar ficavam fracos.
// A vida sobe na mesma proporção (um monstro ferido continua ferido).
void acompanharJogador(Jogo& jogo) {
    int folga = dificuldadeAtual().folgaDeNivel;
    if (folga < 0) return;          // Fácil: sem regra
    if (jogo.andar < 3) return;     // os andares 1 e 2 são de aprendizado
    Andar& andar = andarAtual(jogo);
    for (int i = 0; i < andar.numMonstros; i++) {
        Monstro& m = andar.monstros[i];
        if (!m.vivo || (tipoDe(m).especial & CHEFE)) continue;   // o chefe tem a regra dele
        int minimo = jogo.jogador.nivel - folga + (m.campeao ? 2 : 0) - (m.guarda ? FOLGA_DOS_GUARDAS : 0);
        if (minimo > NIVEL_MAXIMO) minimo = NIVEL_MAXIMO;
        if (m.nivel >= minimo) continue;
        int maximaAntes = vidaMaximaDoMonstro(m);
        m.nivel = minimo;
        m.vida = (int)((long long)m.vida * vidaMaximaDoMonstro(m) / maximaAntes);
        if (m.vida < 1) m.vida = 1;
    }
}

int vidaMaximaDoMonstro(const Monstro& m) {
    // v1.23.1: o chefe tem vida fixa (uma por dificuldade). Pela fórmula ele
    // tinha só ~140 no nível 40, e um golpe forte acabava com a luta final.
    if (tipoDe(m).especial & CHEFE) return dificuldadeAtual().vidaDoChefe;
    // A dificuldade aumenta (ou diminui) a vida de todos os monstros.
    int vida = vidaPeloNivel(tipoDe(m).baseVida, m.nivel) * (100 + dificuldadeAtual().vidaDosMonstros) / 100;
    if (m.campeao) vida = vida * 3 / 2;   // campeão: +50% de vida
    return vida;
}

int monstroEm(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numMonstros; i++) {
        const Monstro& m = andar.monstros[i];
        if (m.vivo && m.x == x && m.y == y) return i;
    }
    return -1;
}

std::string nomeDaEspecie(const Monstro& m) {
    const TipoMonstro& t = tipoDe(m);
    std::string nome = t.nome;
    if (m.campeao) nome += (t.artigo[0] == 'a') ? " campeã" : " campeão";
    return nome;
}

std::string nomeDoMonstro(const Jogo& jogo, const Monstro& m) {
    if (!monstroAVista(jogo, m)) return "algo";
    return std::string(tipoDe(m).artigo) + " " + nomeDaEspecie(m);
}

bool monstroAVista(const Jogo& jogo, const Monstro& m) {
    if (!m.vivo || !jogo.visivel[m.y][m.x]) return false;
    if (!(tipoDe(m).especial & SOMBRIO)) return true;
    // A sombra só aparece de perto ou na luz de uma tocha.
    int dx = std::abs(m.x - jogo.jogador.x), dy = std::abs(m.y - jogo.jogador.y);
    return (dx <= 2 && dy <= 2) || jogo.luzDeTocha[m.y][m.x];
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
    m.campeao = false;   // antes da vida: a vida máxima depende disso
    m.guarda = false;
    m.cornetas = 0;
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
        // Nível: cerca de 2 por andar (andar 1: níveis 1 a 3; andar 12: 23 a 25).
        int nivel = 2 * numeroDoAndar + sortear(g, -1, 1);

        for (int tentativa = 0; tentativa < 300; tentativa++) {
            int x = sortear(g, 1, MAPA_LARGURA - 2);
            int y = sortear(g, 1, MAPA_ALTURA - 2);
            if (andar.mapa.celulas[y][x] != CHAO || andar.zonaSegura[y][x]) continue;
            if (distancia[y][x] == INALCANCAVEL || distancia[y][x] < 12) continue;
            if (monstroEm(andar, x, y) >= 0) continue;

            criarMonstro(andar, tipo, x, y, nivel, estado);
            // A aranha já começa com umas teias em volta.
            if (TIPOS_DE_MONSTRO[tipo].especial & TECE_TEIA) {
                for (int d = 0; d < 8; d += 3) tecerTeia(andar, x + 2 * DIRECOES_DX[d], y + 2 * DIRECOES_DY[d]);
            }
            // O fungo nasce sempre parado (disfarçado).
            if (TIPOS_DE_MONSTRO[tipo].especial & DISFARCADO) andar.monstros[andar.numMonstros - 1].estado = DORMINDO;

            // Espécies em bando ganham um companheiro ao lado (às vezes dois).
            if (TIPOS_DE_MONSTRO[tipo].especial & EM_BANDO) {
                int extras = chance(g, 25) ? 2 : 1;
                if (TIPOS_DE_MONSTRO[tipo].especial & REMONTA) extras = 1;   // esqueletos andam sempre em dupla
                for (int d = 0; d < 8 && extras > 0 && andar.numMonstros < MAX_MONSTROS; d++) {
                    int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
                    if (andar.mapa.celulas[ny][nx] != CHAO || andar.zonaSegura[ny][nx]) continue;
                    if (distancia[ny][nx] == INALCANCAVEL || distancia[ny][nx] < 12) continue;
                    if (monstroEm(andar, nx, ny) >= 0) continue;
                    criarMonstro(andar, tipo, nx, ny, nivel, estado);
                    extras--;
                }
            }
            break;
        }
    }

    // Campeões (elite): do andar 2 em diante, cada monstro tem uma chance
    // (que depende da dificuldade) de nascer mais forte, até um máximo por
    // andar. O gnomo vigia nunca é campeão (ele foge, não luta).
    if (numeroDoAndar < 2) return;
    const Dificuldade& d = dificuldadeAtual();
    int campeoes = 0;
    int primeiro = andar.numMonstros > 0 ? sortear(g, 0, andar.numMonstros - 1) : 0;
    for (int k = 0; k < andar.numMonstros && campeoes < d.maximoDeCampeoes; k++) {
        Monstro& m = andar.monstros[(primeiro + k) % andar.numMonstros];
        if ((tipoDe(m).especial & VIGIA) || !chance(g, d.chanceDeCampeao)) continue;
        tornarCampeao(m);
        campeoes++;
    }
}

void tornarCampeao(Monstro& m) {
    m.campeao = true;
    m.nivel += 2;
    m.vida = vidaMaximaDoMonstro(m);
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
    criarMonstro(andar, tipoDoChefe(), melhorX, melhorY, NIVEL_MINIMO_DO_CHEFE, DORMINDO);
    andar.monstros[andar.numMonstros - 1].elemento = elemento;
    colocarGuardasDoChefe(andar, elemento);   // o elemento certo do ogro vem depois (jogo.cpp)
}

// Os guarda-costas dormem nas casas livres mais perto do chefe (de chão
// comum e alcançáveis a partir dele). Nível: FOLGA_DOS_GUARDAS abaixo dos
// outros monstros (do andar e da regra que acompanha o jogador).
void colocarGuardasDoChefe(Andar& andar, Elemento elementoDoOgro) {
    int chefe = -1, guardas = 0;
    for (int i = 0; i < andar.numMonstros; i++) {
        if (!andar.monstros[i].vivo) continue;
        if (tipoDe(andar.monstros[i]).especial & CHEFE) chefe = i;
        if (andar.monstros[i].guarda) guardas++;
    }
    if (chefe < 0) return;
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, andar.monstros[chefe].x, andar.monstros[chefe].y, distancia);
    for (int passos = 1; passos <= 6 && guardas < GUARDAS_DO_CHEFE; passos++) {
        for (int y = 0; y < MAPA_ALTURA && guardas < GUARDAS_DO_CHEFE; y++) {
            for (int x = 0; x < MAPA_LARGURA && guardas < GUARDAS_DO_CHEFE; x++) {
                if (distancia[y][x] != passos || andar.mapa.celulas[y][x] != CHAO) continue;
                if (andar.zonaSegura[y][x] || monstroEm(andar, x, y) >= 0) continue;
                if (andar.numMonstros >= MAX_MONSTROS) return;
                criarMonstro(andar, tipoDoOgro(), x, y, 2 * NUM_ANDARES - FOLGA_DOS_GUARDAS, DORMINDO);
                Monstro& g = andar.monstros[andar.numMonstros - 1];
                g.guarda = true;
                g.elemento = elementoDoOgro;
                guardas++;
            }
        }
    }
}

void ajustarNivelDoChefe(Monstro& chefe, int nivelDoJogador) {
    int novo = nivelDoJogador + dificuldadeAtual().nivelExtraDoChefe;   // +1 (+3 no Impossível)
    if (novo < NIVEL_MINIMO_DO_CHEFE) novo = NIVEL_MINIMO_DO_CHEFE;
    if (novo <= chefe.nivel) return;   // só sobe
    int maximaAntes = vidaMaximaDoMonstro(chefe);
    chefe.nivel = novo;
    chefe.vida = (int)((long long)chefe.vida * vidaMaximaDoMonstro(chefe) / maximaAntes);
    if (chefe.vida < 1) chefe.vida = 1;
}

// =============================================================================
// INTELIGÊNCIA ARTIFICIAL
// =============================================================================

// O monstro pode pisar em (x, y)? (sem paredes, escadas, outros monstros ou o jogador)
static bool celulaLivre(const Jogo& jogo, const Monstro& m, int x, int y) {
    const Andar& andar = andarAtual(jogo);
    if (!ehPassavel(andar.mapa, x, y) || ehEscada(andar.mapa, x, y)) return false;
    if (x == jogo.jogador.x && y == jogo.jogador.y) return false;
    if (npcEm(andar, x, y) >= 0) return false;
    if (andar.zonaSegura[y][x]) return false;   // a lojinha do comerciante
    // Monstros não pisam na lava (a salamandra pisa).
    if (andar.mapa.celulas[y][x] == LAVA && !(tipoDe(m).especial & ANDA_NA_LAVA)) return false;
    if (especialEm(andar, x, y) >= 0) return false;       // fonte, altar, baú, gongo
    return monstroEm(andar, x, y) < 0;
}

static bool podeAtacarJogador(const Jogo& jogo, const Monstro& m) {
    int dx = jogo.jogador.x - m.x;
    int dy = jogo.jogador.y - m.y;
    if (dx < -1 || dx > 1 || dy < -1 || dy > 1 || (dx == 0 && dy == 0)) return false;
    // Dentro da lojinha o jogador está protegido.
    if (andarAtual(jogo).zonaSegura[jogo.jogador.y][jogo.jogador.x]) return false;
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
        if (!celulaLivre(jogo, m, nx, ny)) continue;

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
        if (podeMover(mapa, m.x, m.y, dx, dy) && celulaLivre(jogo, m, m.x + dx, m.y + dy)) {
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
        if (mapa.celulas[y][x] == CHAO && !andarAtual(jogo).zonaSegura[y][x]) {
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

// =============================================================================
// GNOMO VIGIA: A CORNETA
// =============================================================================
// O som atravessa as portas: acorda os monstros da "sala" do gnomo e das
// salas ligadas a ela por uma porta. "Sala" aqui é cada pedaço do mapa que
// fica entre portas (achado com flood fill, sem atravessar portas). Se do
// outro lado da porta houver um corredor, o som segue por ele até a próxima
// sala (corredor = região sem nenhuma casa com os 8 vizinhos livres).
static bool ehPortaParaOSom(const Mapa& mapa, int x, int y) {
    return mapa.celulas[y][x] == PORTA || mapa.celulas[y][x] == PORTA_TRANCADA;
}
static bool deixaOSomPassar(const Mapa& mapa, int x, int y) {
    TipoCelula c = mapa.celulas[y][x];
    return c != PAREDE && c != TOCHA && !ehPortaParaOSom(mapa, x, y);
}

void casasQueOuvemACorneta(const Mapa& mapa, int origemX, int origemY, bool ouve[MAPA_ALTURA][MAPA_LARGURA]) {
    static int regiao[MAPA_ALTURA][MAPA_LARGURA];
    const int MAX_REGIOES = 400;
    static bool corredor[MAX_REGIOES];
    int numRegioes = 0;
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++) { regiao[y][x] = -1; ouve[y][x] = false; }

    // 1. Separa o mapa em regiões (flood fill sem atravessar portas).
    static int fila[MAPA_ALTURA * MAPA_LARGURA];
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (regiao[y][x] >= 0 || !deixaOSomPassar(mapa, x, y) || numRegioes >= MAX_REGIOES) continue;
            int r = numRegioes++;
            corredor[r] = true;
            int inicio = 0, fim = 0;
            fila[fim++] = y * MAPA_LARGURA + x;
            regiao[y][x] = r;
            while (inicio < fim) {
                int cx = fila[inicio] % MAPA_LARGURA, cy = fila[inicio] / MAPA_LARGURA;
                inicio++;
                int livres = 0;
                for (int d = 0; d < 8; d++) {
                    int nx = cx + DIRECOES_DX[d], ny = cy + DIRECOES_DY[d];
                    if (!dentroDoMapa(nx, ny)) continue;
                    if (mapa.celulas[ny][nx] != PAREDE && mapa.celulas[ny][nx] != TOCHA) livres++;
                    if (d >= 4 || regiao[ny][nx] >= 0 || !deixaOSomPassar(mapa, nx, ny)) continue;
                    regiao[ny][nx] = r;
                    fila[fim++] = ny * MAPA_LARGURA + nx;
                }
                if (livres == 8) corredor[r] = false;   // tem "miolo": é uma sala
            }
        }
    }

    // 2. As portas ligam as regiões: cada porta vira uma "ligação" entre as
    //    regiões que encostam nela.
    auto regioesEmVolta = [&](int x, int y, int saida[4]) {
        int n = 0;
        for (int d = 0; d < 4; d++) {
            int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
            if (dentroDoMapa(nx, ny) && regiao[ny][nx] >= 0) saida[n++] = regiao[ny][nx];
        }
        return n;
    };
    static int ligacaoA[MAPA_ALTURA * MAPA_LARGURA], ligacaoB[MAPA_ALTURA * MAPA_LARGURA];
    int numLigacoes = 0;
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (!ehPortaParaOSom(mapa, x, y)) continue;
            int lados[4];
            int n = regioesEmVolta(x, y, lados);
            for (int i = 0; i < n; i++)
                for (int k = i + 1; k < n; k++)
                    if (lados[i] != lados[k]) { ligacaoA[numLigacoes] = lados[i]; ligacaoB[numLigacoes++] = lados[k]; }
        }
    }

    // 3. Começa pela região do gnomo (se ele estiver numa porta, pelas
    //    regiões dos dois lados). Atravessa uma porta para todas as vizinhas;
    //    as vizinhas que forem corredores atravessam mais uma.
    static bool chega[MAX_REGIOES], primeiraLeva[MAX_REGIOES], segundaLeva[MAX_REGIOES];
    for (int r = 0; r < numRegioes; r++) chega[r] = primeiraLeva[r] = segundaLeva[r] = false;
    int inicio[4], numInicio = 0;
    if (regiao[origemY][origemX] >= 0) inicio[numInicio++] = regiao[origemY][origemX];
    else numInicio = regioesEmVolta(origemX, origemY, inicio);
    for (int k = 0; k < numInicio; k++) chega[inicio[k]] = true;
    for (int i = 0; i < numLigacoes; i++) {
        if (chega[ligacaoA[i]] && !chega[ligacaoB[i]]) primeiraLeva[ligacaoB[i]] = true;
        if (chega[ligacaoB[i]] && !chega[ligacaoA[i]]) primeiraLeva[ligacaoA[i]] = true;
    }
    for (int r = 0; r < numRegioes; r++) if (primeiraLeva[r]) chega[r] = true;
    for (int i = 0; i < numLigacoes; i++) {
        int a = ligacaoA[i], b = ligacaoB[i];
        if (primeiraLeva[a] && corredor[a] && !chega[b]) segundaLeva[b] = true;
        if (primeiraLeva[b] && corredor[b] && !chega[a]) segundaLeva[a] = true;
    }
    for (int r = 0; r < numRegioes; r++) if (segundaLeva[r]) chega[r] = true;

    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (regiao[y][x] >= 0) ouve[y][x] = chega[regiao[y][x]];
            else if (ehPortaParaOSom(mapa, x, y)) {
                int lados[4];
                int n = regioesEmVolta(x, y, lados);
                for (int k = 0; k < n; k++) if (chega[lados[k]]) ouve[y][x] = true;
            }
        }
    }
}

// Quantos turnos os monstros que ouviram a corneta seguem até o jogador,
// mesmo sem vê-lo (guardado como turnosSemVerJogador negativo).
const int TURNOS_DE_ALERTA = 25;

// O gnomo sopra a corneta: os monstros da sala dele e das vizinhas acordam e
// vêm atrás do jogador. Devolve quantos foram chamados.
static int tocarCorneta(Jogo& jogo, Monstro& gnomo) {
    Andar& andar = andarAtual(jogo);
    static bool ouve[MAPA_ALTURA][MAPA_LARGURA];
    casasQueOuvemACorneta(andar.mapa, gnomo.x, gnomo.y, ouve);
    int chamados = 0;
    for (int i = 0; i < andar.numMonstros; i++) {
        Monstro& m = andar.monstros[i];
        if (!m.vivo || &m == &gnomo || !ouve[m.y][m.x] || andar.zonaSegura[m.y][m.x]) continue;
        if (tipoDe(m).especial & (CHEFE | VIGIA)) continue;
        m.estado = CACANDO;
        m.turnosSemVerJogador = -TURNOS_DE_ALERTA;
        chamados++;
    }
    gnomo.cornetas++;
    // À vista: a cena (câmera nele, corneta, risada). Longe: só o som.
    bool aVista = monstroAVista(jogo, gnomo);
    if (aVista && !jogo.modoAutomatico) {
        cenaDaCorneta(jogo, (int)(&gnomo - andar.monstros), gnomo.cornetas == 1);
    } else {
        tocarSom(SOM_CORNETA);
        efeitoNoMapa(jogo, EFEITO_CORNETA, gnomo.x, gnomo.y, chamados);
    }
    std::string quem = aVista ? "O gnomo vigia" : "Um gnomo vigia, em algum lugar,";
    std::string resultado = chamados == 0 ? " Ninguém respondeu... desta vez."
                          : chamados == 1 ? " Você ouve passos vindo atrás de você!"
                          : " Você ouve " + std::to_string(chamados) + " monstros vindo atrás de você!";
    if (gnomo.cornetas == 1) {
        adicionarMensagem(jogo, quem + " sopra a corneta!" + resultado, MSG_PERIGO);
    } else {
        adicionarMensagem(jogo, quem + " sopra a corneta de novo e não foge mais: \"Agora você não escapa!\"" + resultado,
                          MSG_PERIGO);
    }
    return chamados;
}

// O gnomo foge para o ponto do andar mais longe do jogador.
static void fugirParaLonge(Jogo& jogo, Monstro& m) {
    const Andar& andar = andarAtual(jogo);
    int maior = -1;
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            int d = jogo.distanciaAteJogador[y][x];
            if (d == INALCANCAVEL || andar.mapa.celulas[y][x] != CHAO || andar.zonaSegura[y][x]) continue;
            if (especialEm(andar, x, y) >= 0 || npcEm(andar, x, y) >= 0) continue;
            if (d > maior) { maior = d; m.destinoX = x; m.destinoY = y; }
        }
    }
    m.estado = FUGINDO;
}

// O gnomo vigia tem o seu próprio jeito de agir:
//   0 cornetas: age como os outros até perceber o jogador; aí sopra e foge.
//   1 corneta : corre até o lugar longe e fica lá, quieto. Se o jogador
//               aparecer de novo, sopra outra vez e fica para lutar.
//   2 cornetas: não sai do lugar; luta quando o jogador encostar.
// Devolve 1 se começou a batalha, 0 se já agiu, -1 para seguir o normal.
static int agirComoVigia(Jogo& jogo, Monstro& m, int indice, bool percebe) {
    if (m.cornetas >= 2) {
        if (podeAtacarJogador(jogo, m) && m.turnosSemBatalha == 0) {
            jogo.batalhaPendente = indice;
            return 1;
        }
        return 0;
    }
    if (m.cornetas == 1 && m.estado == FUGINDO) {
        if (m.x == m.destinoX && m.y == m.destinoY) {
            m.estado = VAGANDO;   // chegou: fica quieto no esconderijo
            return 0;
        }
        static int caminho[MAPA_ALTURA][MAPA_LARGURA];
        calcularDistancias(mapaAtual(jogo), m.destinoX, m.destinoY, caminho);
        if (!moverParaMelhorVizinho(jogo, m, caminho, true)) moverParaMelhorVizinho(jogo, m, jogo.distanciaAteJogador, false);
        return 0;
    }
    if (m.estado != DORMINDO && percebe) {
        tocarCorneta(jogo, m);
        if (m.cornetas == 1) fugirParaLonge(jogo, m);
        else m.estado = CACANDO;
        return 0;
    }
    if (m.cornetas == 1) return 0;   // escondido, esperando
    return -1;
}

// v1.23.1: o guarda-costas do chefe dorme e acorda junto com ele e nunca se
// afasta mais de 2 casas dele (não sai caçando sozinho). Se o jogador encostar,
// a batalha começa (e o chefe e o outro guarda entram, ver jogo.cpp).
static bool agirComoGuarda(Jogo& jogo, Monstro& m, int indice, const Monstro& chefe) {
    m.estado = chefe.estado == DORMINDO ? DORMINDO : chefe.estado == FUGINDO ? CACANDO : chefe.estado;
    if (m.estado == DORMINDO) return false;
    int dx = std::abs(m.x - chefe.x), dy = std::abs(m.y - chefe.y);
    if ((dx > dy ? dx : dy) > 2) {
        static int pertoDoChefe[MAPA_ALTURA][MAPA_LARGURA];
        calcularDistancias(mapaAtual(jogo), chefe.x, chefe.y, pertoDoChefe);
        moverParaMelhorVizinho(jogo, m, pertoDoChefe, true);
        return false;
    }
    if (podeAtacarJogador(jogo, m) && m.turnosSemBatalha == 0) {
        jogo.batalhaPendente = indice;
        return true;
    }
    return false;
}

// Uma ação do monstro. Devolve true se ele alcançou o jogador (começa a batalha).
static bool agirUmaVez(Jogo& jogo, Monstro& m, int indice) {
    const TipoMonstro& t = tipoDe(m);
    Andar& andar = andarAtual(jogo);
    int distancia = jogo.distanciaAteJogador[m.y][m.x];

    // O monstro vê o jogador se o jogador vê a célula dele (a linha de visão
    // vale para os dois lados). O jogador carrega luz, então é sempre visto.
    bool vejoJogador = jogo.linhaDeVisao[m.y][m.x] && distancia != INALCANCAVEL;
    // Descansando ou furtivo, o jogador faz menos barulho: alcance pela metade.
    bool silencioso = jogo.descansando || jogo.furtivo;
    int alcance = silencioso ? ALCANCE_DE_PERCEPCAO / 2 : ALCANCE_DE_PERCEPCAO;

    if (t.especial & VIGIA) {
        int r = agirComoVigia(jogo, m, indice, vejoJogador && distancia <= alcance);
        if (r >= 0) return r == 1;
    }
    if (m.guarda) {
        for (int i = 0; i < andar.numMonstros; i++) {
            const Monstro& c = andar.monstros[i];
            if (c.vivo && (tipoDe(c).especial & CHEFE)) return agirComoGuarda(jogo, m, indice, c);
        }
        // (sem chefe vivo, o guarda age como um ogro comum)
    }

    // ---- DORMINDO: pode acordar se o jogador estiver bem perto ----------
    // Furtivo, o jogador quase não acorda ninguém (só colado e com pouca chance).
    if (m.estado == DORMINDO) {
        bool acorda = jogo.furtivo ? (distancia <= 2 && chance(jogo.sorte, 10))
                                   : (distancia <= alcance / 2 && chance(jogo.sorte, 33));
        // O fungo disfarçado só "acorda" quando o jogador chega a 2 casas (e aí sempre).
        if (t.especial & DISFARCADO) {
            int dx = std::abs(jogo.jogador.x - m.x), dy = std::abs(jogo.jogador.y - m.y);
            acorda = (dx > dy ? dx : dy) <= 2;
            if (vejoJogador && acorda) {
                m.estado = CACANDO;
                m.turnosSemVerJogador = 0;
                tocarSom(SOM_FOLHAS);
                adicionarMensagem(jogo, "Aquele cogumelo se mexeu! Era um fungo!", MSG_PERIGO);
            }
            return false;
        }
        if (vejoJogador && acorda) {
            m.estado = CACANDO;
            m.turnosSemVerJogador = 0;
            if (monstroAVista(jogo, m)) {
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
            if (monstroAVista(jogo, m)) {
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
    } else if (m.turnosSemVerJogador < 0) {
        // Ouviu a corneta: vai até onde o jogador está, mesmo sem vê-lo.
        m.turnosSemVerJogador++;
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

// Uma teia em (x, y), se for um chão comum (longe de portas e escadas, fora
// da lojinha) e o andar ainda não estiver cheio de teias.
const int MAX_TEIAS_POR_ANDAR = 18;
void tecerTeia(Andar& andar, int x, int y) {
    if (!dentroDoMapa(x, y) || andar.mapa.celulas[y][x] != CHAO || andar.zonaSegura[y][x]) return;
    for (int d = 0; d < 8; d++) {
        TipoCelula c = andar.mapa.celulas[y + DIRECOES_DY[d]][x + DIRECOES_DX[d]];
        if (c == PORTA || c == PORTA_TRANCADA || c == ESCADA_DESCE || c == ESCADA_SOBE) return;
    }
    int teias = 0;
    for (int yy = 0; yy < MAPA_ALTURA; yy++)
        for (int xx = 0; xx < MAPA_LARGURA; xx++) teias += andar.mapa.celulas[yy][xx] == TEIA;
    if (teias >= MAX_TEIAS_POR_ANDAR) return;
    andar.mapa.celulas[y][x] = TEIA;
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
        // Na água o monstro anda na metade da velocidade (o morcego voa por cima).
        int lentidao = (tipoDe(m).especial & ERRATICO) ? 1 : multiplicadorDoPasso(andar, m.x, m.y);
        while (m.vivo && m.tempoAcumulado >= intervaloDoMonstro(m) * lentidao) {
            m.tempoAcumulado -= intervaloDoMonstro(m) * lentidao;
            if (m.turnosSemBatalha > 0) m.turnosSemBatalha--;
            // O ogro (e o chefe) despedaça as caixas que estão encostadas nele.
            if ((tipoDe(m).especial & QUEBRA_CAIXAS) && m.estado != DORMINDO) quebrarCaixasPerto(jogo, m);
            int antesX = m.x, antesY = m.y;
            if (agirUmaVez(jogo, m, i)) return; // batalha: os outros esperam
            // Andou para cima de uma armadilha?
            if (m.x != antesX || m.y != antesY) armadilhaPegaMonstro(jogo, m);
            // A aranha deixa uma teia onde estava (às vezes).
            if ((tipoDe(m).especial & TECE_TEIA) && (m.x != antesX || m.y != antesY) && chance(jogo.sorte, 30))
                tecerTeia(andar, antesX, antesY);
        }
    }
}

// =============================================================================
// MONSTROS EXTRAS (mímico e ondas da arena, ver salas.cpp)
// =============================================================================
int criarMonstroExtra(Andar& andar, int tipo, int x, int y, int nivel, Elemento elemento) {
    // Reaproveita a vaga de um monstro já derrotado, se a lista estiver cheia.
    int vaga = -1;
    if (andar.numMonstros >= MAX_MONSTROS) {
        for (int i = 0; i < andar.numMonstros && vaga < 0; i++) {
            if (!andar.monstros[i].vivo && !andar.monstros[i].carregaChave) vaga = i;
        }
        if (vaga < 0) return -1;
        int total = andar.numMonstros;
        andar.numMonstros = vaga;          // criarMonstro usa a posição numMonstros
        criarMonstro(andar, tipo, x, y, nivel, CACANDO);
        andar.numMonstros = total;
    } else {
        vaga = andar.numMonstros;
        criarMonstro(andar, tipo, x, y, nivel, CACANDO);
    }
    andar.monstros[vaga].elemento = elemento;
    return vaga;
}

int sortearTipoDoAndar(GeradorAleatorio& g, int numeroDoAndar) {
    return sortearTipo(g, numeroDoAndar);
}
