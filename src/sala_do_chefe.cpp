// sala_do_chefe.cpp - v1.24: as salas feitas à mão dos chefes (ver o .h).
#include "sala_do_chefe.h"
#include "jogo.h"
#include "dificuldade.h"
#include "som.h"
#include <string>
#include <vector>
#include <queue>
#include <cstdlib>

// -----------------------------------------------------------------------------
// AS SALAS, DESENHADAS EM TEXTO
// -----------------------------------------------------------------------------
// Legenda:
//   #  parede            .  chão             +  portão (a única entrada)
//   >  escada de descer  T  tocha na parede  t  lápide
//   %  teia              ~  lava             w  poça d'água
//   K  o chefe           h  um ajudante      O  o chefe, em cima do círculo do ritual
// Enfeites que ninguém atravessa: R trono do rei, X trono do chefe final,
//   C casulo, c coluna, F forja, B bigorna, b braseiro.
// Enfeites no chão (dá para pisar): $ ouro, o ovos, v velas, = tapete.
// A escada fica sempre na parede de cima, com chão livre embaixo dela.
static const char* const SALAS[NUM_SALAS_DE_CHEFE][11] = {
    {   // Salão do Rei Goblin: o ouro roubado das minas, o trono e a escada no canto.
        "####T###T###T#>##",
        "#$$.....R.....$.#",
        "T$......K......$T",
        "#...............#",
        "#..h.........h..#",
        "T...............T",
        "#$$...........$$#",
        "####T###+###T####",
        nullptr },
    {   // Ninho da Rainha Aranha: teias por todo lado, casulos e ovos.
        "#####T#####>#######",
        "##%%C....%..%C%%###",
        "#%%.....oo....%%%##",
        "#%......K.......%%#",
        "TC..h..........h.CT",
        "#%%.....ww.....%%%#",
        "##%%...wwww...%%###",
        "####%%.......%%####",
        "#########+#########",
        nullptr },
    {   // Altar do Necromante: lápides, colunas, velas e o círculo do ritual.
        "####T###>###T####",
        "#v.t.t.....t.t.v#",
        "#.c...........c.#",
        "T.......O.......T",
        "#.c..h.....h..c.#",
        "#t.t.........t.t#",
        "#v.............v#",
        "########+########",
        nullptr },
    {   // Forja do Golem: a lava em volta, a forja, a bigorna e uma ponte só.
        "#####T###>###T#####",
        "#~~~~..F......~~~~#",
        "#~~.............~~#",
        "#~~...h..K..h...~~#",
        "#~~.B...........~~#",
        "#~~~~~~~...~~~~~~~#",
        "#~~~~~~~...~~~~~~~#",
        "T.................T",
        "#########+#########",
        nullptr },
    {   // Salão do Trono: colunas, braseiros, o tapete até o trono e os 2 ogros.
        "#####T##T###T##T#####",
        "#c.......bXb.......c#",
        "#........hKh........#",
        "#c........=........c#",
        "T.........=.........T",
        "#c........=........c#",
        "T.........=.........T",
        "#c........=........c#",
        "#.........=.........#",
        "##########+##########",
        nullptr },
};

int salaDoAndar(int numeroDoAndar) {
    switch (numeroDoAndar) {
        case 3: return SALA_DO_REI;
        case 6: return NINHO_DA_RAINHA;
        case 9: return ALTAR_DO_NECROMANTE;
        case 12: return FORJA_DO_GOLEM;
        case NUM_ANDARES: return SALAO_DO_TRONO;
    }
    return -1;
}

bool enfeiteSolido(char e) {
    return e == 'R' || e == 'X' || e == 'C' || e == 'c' || e == 'F' || e == 'B' || e == 'b';
}

static int tipoPeloSimbolo(char simbolo) {
    for (int i = 0; i < NUM_TIPOS_DE_MONSTRO; i++) if (TIPOS_DE_MONSTRO[i].simbolo == simbolo) return i;
    return 0;
}

static const char SIMBOLO_DO_CHEFE[NUM_SALAS_DE_CHEFE] = { 'R', 'A', 'n', 'L', 'S' };
static const char SIMBOLO_DO_AJUDANTE[NUM_SALAS_DE_CHEFE] = { 'g', 'a', 'e', 's', 'O' };

int tipoDoAjudante(int tipoDeSala) { return tipoPeloSimbolo(SIMBOLO_DO_AJUDANTE[tipoDeSala]); }

int salaDoMiniChefe(int tipoDoMonstro) {
    if (tipoDoMonstro < 0 || tipoDoMonstro >= NUM_TIPOS_DE_MONSTRO) return -1;
    if (!(TIPOS_DE_MONSTRO[tipoDoMonstro].especial & MINI_CHEFE)) return -1;
    for (int s = 0; s < SALAO_DO_TRONO; s++) if (TIPOS_DE_MONSTRO[tipoDoMonstro].simbolo == SIMBOLO_DO_CHEFE[s]) return s;
    return -1;
}

// Os números dos mini-chefes (acertados com lutas simuladas, ver
// docs/COMO_FUNCIONA.md). Como o chefe final, a vida é fixa (uma por
// dificuldade): pela fórmula dos monstros, um golpe forte acabava com a luta.
// O nível acompanha o do jogador (o "+1" do chefe, mais o ajuste) e o ataque
// é uma porcentagem do normal: no começo do jogo o jogador tem pouca vida.
// (No Difícil e no Impossível a vida é a do Médio: lá eles já batem mais forte.)
struct NumerosDoMiniChefe {
    int vida[NUM_DIFICULDADES];   // Fácil, Médio, Difícil, Impossível
    int ajusteDeNivel;            // somado ao nível do jogador + nivelExtraDoChefe
    int ataque;                   // % do ataque normal
};
static const NumerosDoMiniChefe NUMEROS[SALAO_DO_TRONO] = {
    { {  60, 110, 110, 110 }, -3,  80 },   // Rei Goblin (andar 3)
    { { 140, 250, 250, 250 }, -3,  80 },   // Rainha Aranha (andar 6)
    { { 200, 360, 360, 360 }, -1, 100 },   // Necromante (andar 9)
    { { 205, 370, 370, 370 }, -3,  80 },   // Golem de Magma (andar 12)
};

int vidaDoMiniChefe(int tipoDeSala) {
    if (tipoDeSala < 0 || tipoDeSala >= SALAO_DO_TRONO) return 100;
    return NUMEROS[tipoDeSala].vida[nivelDaDificuldade()];
}
int ajusteDeNivelDoMiniChefe(int tipoDeSala) {
    return (tipoDeSala < 0 || tipoDeSala >= SALAO_DO_TRONO) ? 0 : NUMEROS[tipoDeSala].ajusteDeNivel;
}
int ataqueDoMiniChefe(int tipoDeSala) {
    return (tipoDeSala < 0 || tipoDeSala >= SALAO_DO_TRONO) ? 100 : NUMEROS[tipoDeSala].ataque;
}

bool dentroDaSalaDoChefe(const Andar& a, int x, int y) {
    const SalaDoChefe& s = a.salaDoChefe;
    if (!s.existe) return false;
    return x > s.x && x < s.x + s.largura - 1 && y > s.y && y < s.y + s.altura - 1;
}

int chefeDaSala(const Andar& a) {
    if (!a.salaDoChefe.existe) return -1;
    for (int i = 0; i < a.numMonstros; i++) {
        const Monstro& m = a.monstros[i];
        if (m.vivo && (tipoDe(m).especial & (CHEFE | MINI_CHEFE))) return i;
    }
    return -1;
}

// -----------------------------------------------------------------------------
// ONDE A SALA CABE
// -----------------------------------------------------------------------------
namespace {

const int LONGE = 1 << 28;

// Dá para andar por aqui? (para conferir os caminhos: caixas são empurradas
// e a porta do tesouro abre com a chave, então contam como caminho).
bool caminho(TipoCelula c) {
    return c == CHAO || c == PORTA || c == POCA || c == LAVA || c == TEIA || c == CAIXA ||
           c == PORTA_TRANCADA || c == PORTAO;
}

// Distância (em passos) de (x0, y0) até cada casa, andando pelo caminho.
void distancias(const Mapa& mapa, int x0, int y0, int d[MAPA_ALTURA][MAPA_LARGURA]) {
    for (int y = 0; y < MAPA_ALTURA; y++) for (int x = 0; x < MAPA_LARGURA; x++) d[y][x] = LONGE;
    std::queue<int> fila;
    d[y0][x0] = 0;
    fila.push(y0 * MAPA_LARGURA + x0);
    while (!fila.empty()) {
        int p = fila.front(); fila.pop();
        int x = p % MAPA_LARGURA, y = p / MAPA_LARGURA;
        for (int k = 0; k < 4; k++) {
            int nx = x + DIRECOES_DX[k], ny = y + DIRECOES_DY[k];
            if (!dentroDoMapa(nx, ny) || d[ny][nx] != LONGE) continue;
            TipoCelula c = mapa.celulas[ny][nx];
            // A escada de subir também é ponto de partida, mas não é passagem.
            if (!caminho(c)) continue;
            d[ny][nx] = d[y][x] + 1;
            fila.push(ny * MAPA_LARGURA + nx);
        }
    }
}

int largura(int tipo) { return (int)std::string(SALAS[tipo][0]).size(); }
int altura(int tipo) { int h = 0; while (SALAS[tipo][h]) h++; return h; }

// Casas que a sala não pode cobrir: a chegada, a loja, a sala do tesouro,
// as salas especiais e os NPCs (com uma folga em volta).
// "rigor" 0 é o normal; se a sala não couber em lugar nenhum, as folgas
// diminuem (1 e 2).
void marcarProtegidas(const Andar& a, const int dChegada[MAPA_ALTURA][MAPA_LARGURA], bool prot[MAPA_ALTURA][MAPA_LARGURA],
                      int rigor) {
    int perto = rigor == 0 ? 6 : 3;
    for (int y = 0; y < MAPA_ALTURA; y++) for (int x = 0; x < MAPA_LARGURA; x++)
        prot[y][x] = a.zonaSegura[y][x] || dChegada[y][x] <= perto;
    auto marcarEmVolta = [&](int cx, int cy, int r) {
        for (int y = cy - r; y <= cy + r; y++) for (int x = cx - r; x <= cx + r; x++)
            if (dentroDoMapa(x, y)) prot[y][x] = true;
    };
    marcarEmVolta(a.escadas.sobeX, a.escadas.sobeY, 2);
    // Sala do tesouro: tudo o que se alcança de dentro sem passar pela porta.
    if (a.tesouro.existe) {
        marcarEmVolta(a.tesouro.portaX, a.tesouro.portaY, 1);
        static bool visto[MAPA_ALTURA][MAPA_LARGURA];
        for (auto& linha : visto) for (bool& v : linha) v = false;
        std::queue<int> fila;
        fila.push(a.tesouro.dentroY * MAPA_LARGURA + a.tesouro.dentroX);
        visto[a.tesouro.dentroY][a.tesouro.dentroX] = true;
        while (!fila.empty()) {
            int p = fila.front(); fila.pop();
            int x = p % MAPA_LARGURA, y = p / MAPA_LARGURA;
            marcarEmVolta(x, y, 1);
            for (int k = 0; k < 4; k++) {
                int nx = x + DIRECOES_DX[k], ny = y + DIRECOES_DY[k];
                if (!dentroDoMapa(nx, ny) || visto[ny][nx]) continue;
                TipoCelula c = a.mapa.celulas[ny][nx];
                if (c == PORTA_TRANCADA || !caminho(c)) continue;
                visto[ny][nx] = true;
                fila.push(ny * MAPA_LARGURA + nx);
            }
        }
    }
    for (int i = 0; i < a.numEspeciais; i++) {
        marcarEmVolta(a.especiais[i].x, a.especiais[i].y, rigor == 0 ? 2 : 1);
        if (a.especiais[i].tipo == ESP_GONGO) marcarEmVolta(a.especiais[i].portaX, a.especiais[i].portaY, 2);
    }
    for (int i = 0; i < a.numNpcs; i++) marcarEmVolta(a.npcs[i].x, a.npcs[i].y, 1);
}

// A letra do desenho que cai na linha y da sala (de cabeça para baixo, as
// linhas vêm na ordem contrária).
char letra(int tipo, int x, int y, bool invertida) {
    return SALAS[tipo][invertida ? altura(tipo) - 1 - y : y][x];
}

// A casa do lado de fora, bem atrás da escada (v1.24.1): vira parede, senão
// daria para chegar na escada por fora, sem passar pelo portão. Devolve false
// se a sala não tem escada (o salão do trono).
bool atrasDaEscada(int tipo, int ox, int oy, bool invertida, int& bx, int& by) {
    const char* linha = SALAS[tipo][0];   // a escada fica na linha de cima do desenho
    for (int x = 0; linha[x]; x++) {
        if (linha[x] != '>') continue;
        bx = ox + x;
        by = invertida ? oy + altura(tipo) : oy - 1;
        return true;
    }
    return false;
}

// Põe o desenho da sala no mapa (sem os monstros).
void desenharSala(Mapa& m, int tipo, int ox, int oy, bool invertida) {
    for (int y = 0; SALAS[tipo][y]; y++) {
        const char* linha = SALAS[tipo][invertida ? altura(tipo) - 1 - y : y];
        for (int x = 0; linha[x]; x++) {
            TipoCelula c = CHAO;
            switch (linha[x]) {
                case '#': c = PAREDE; break;        case 'T': c = TOCHA; break;
                case '>': c = ESCADA_DESCE; break;  case '+': c = PORTAO; break;
                case 't': c = TUMULO; break;        case '%': c = TEIA; break;
                case '~': c = LAVA; break;          case 'w': c = POCA; break;
                default: c = enfeiteSolido(linha[x]) ? TUMULO : CHAO; break;
            }
            m.celulas[oy + y][ox + x] = c;
        }
    }
}

// Abre o corredor da frente do portão até o caminho mais perto (que já
// existia). Devolve false se não achou. "fim" = onde o corredor encontrou o
// caminho (para medir a distância até a chegada).
bool abrirCorredor(Mapa& m, int gx, int gy, int dy, int ox, int oy, int w, int h,
                   const bool prot[MAPA_ALTURA][MAPA_LARGURA], const int dChegada[MAPA_ALTURA][MAPA_LARGURA],
                   int tapadoX, int tapadoY, int& fimX, int& fimY) {
    static int pai[MAPA_ALTURA][MAPA_LARGURA];
    for (auto& linha : pai) for (int& v : linha) v = -1;
    auto naSala = [&](int x, int y) { return x >= ox && x < ox + w && y >= oy && y < oy + h; };
    int sx = gx, sy = gy + dy;   // a casa do lado de fora do portão
    if (!dentroDoMapa(sx, sy) || sy < 1 || sy >= MAPA_ALTURA - 1 || prot[sy][sx]) return false;
    std::queue<int> fila;
    fila.push(sy * MAPA_LARGURA + sx);
    pai[sy][sx] = -2;
    fimX = -1;
    while (!fila.empty()) {
        int p = fila.front(); fila.pop();
        int x = p % MAPA_LARGURA, y = p / MAPA_LARGURA;
        if (dChegada[y][x] != LONGE && caminho(m.celulas[y][x])) { fimX = x; fimY = y; break; }
        for (int k = 0; k < 4; k++) {
            int nx = x + DIRECOES_DX[k], ny = y + DIRECOES_DY[k];
            if (nx < 1 || ny < 1 || nx >= MAPA_LARGURA - 1 || ny >= MAPA_ALTURA - 1) continue;
            if (pai[ny][nx] != -1 || naSala(nx, ny) || prot[ny][nx]) continue;
            if (nx == tapadoX && ny == tapadoY) continue;   // atrás da escada (fica parede)
            TipoCelula c = m.celulas[ny][nx];
            // Não fura escadas nem a porta trancada do tesouro.
            if (c == ESCADA_SOBE || c == ESCADA_DESCE || c == PORTA_TRANCADA) continue;
            pai[ny][nx] = p;
            fila.push(ny * MAPA_LARGURA + nx);
        }
    }
    if (fimX < 0) return false;
    for (int x = fimX, y = fimY; ;) {
        int p = pai[y][x];
        if (!caminho(m.celulas[y][x])) m.celulas[y][x] = CHAO;
        if (p == -2) break;
        x = p % MAPA_LARGURA; y = p / MAPA_LARGURA;
    }
    return true;
}

// Uma casa de chão livre (sem monstro, item, armadilha ou NPC), fora da sala
// e alcançável, para onde vai o que estava dentro dela.
bool casaLivreFora(const Andar& a, GeradorAleatorio& g, const int d[MAPA_ALTURA][MAPA_LARGURA], int& rx, int& ry) {
    for (int tentativa = 0; tentativa < 2000; tentativa++) {
        int x = sortear(g, 1, MAPA_LARGURA - 2), y = sortear(g, 1, MAPA_ALTURA - 2);
        if (a.mapa.celulas[y][x] != CHAO || d[y][x] == LONGE || a.zonaSegura[y][x]) continue;
        const SalaDoChefe& s = a.salaDoChefe;
        if (x >= s.x && x < s.x + s.largura && y >= s.y && y < s.y + s.altura) continue;
        if (monstroEm(a, x, y) >= 0 || itemEm(a, x, y) >= 0) continue;
        bool ocupado = false;
        for (int i = 0; i < a.numNpcs; i++) if (a.npcs[i].x == x && a.npcs[i].y == y) ocupado = true;
        for (int i = 0; i < a.numArmadilhas; i++) if (a.armadilhas[i].x == x && a.armadilhas[i].y == y) ocupado = true;
        for (int i = 0; i < a.numEspeciais; i++) if (a.especiais[i].x == x && a.especiais[i].y == y) ocupado = true;
        if (x == a.escadas.chegadaSobeX && y == a.escadas.chegadaSobeY) ocupado = true;
        if (ocupado) continue;
        rx = x; ry = y;
        return true;
    }
    return false;
}

}  // namespace

// -----------------------------------------------------------------------------
// CARIMBAR
// -----------------------------------------------------------------------------
bool carimbarSalaDoChefe(Jogo& jogo, Andar& a, int numeroDoAndar, uint32_t semente) {
    a.salaDoChefe.existe = false;
    int tipo = salaDoAndar(numeroDoAndar);
    if (tipo < 0) return false;
    const int W = largura(tipo), H = altura(tipo);
    GeradorAleatorio g;
    iniciarGerador(g, semente);

    // A escada de descer antiga vira parede (a nova fica dentro da sala).
    if (a.escadas.temDescida) a.mapa.celulas[a.escadas.desceY][a.escadas.desceX] = PAREDE;

    static int dChegada[MAPA_ALTURA][MAPA_LARGURA], depois[MAPA_ALTURA][MAPA_LARGURA];
    static bool prot[MAPA_ALTURA][MAPA_LARGURA];
    distancias(a.mapa, a.escadas.chegadaSobeX, a.escadas.chegadaSobeY, dChegada);
    // O que não pode ficar sem caminho (no rigor 2, só isso importa).
    std::vector<std::pair<int,int>> importantes;
    for (int i = 0; i < a.numItens; i++) if (a.itens[i].ativo) importantes.push_back({a.itens[i].x, a.itens[i].y});
    for (int i = 0; i < a.numEspeciais; i++) importantes.push_back({a.especiais[i].x, a.especiais[i].y});
    for (int i = 0; i < a.numMonstros; i++) if (a.monstros[i].vivo && a.monstros[i].carregaChave) importantes.push_back({a.monstros[i].x, a.monstros[i].y});
    if (a.tesouro.existe) importantes.push_back({a.tesouro.dentroX, a.tesouro.dentroY});
    for (int i = 0; i < a.numNpcs; i++)   // o NPC fica parado: vale a casa ao lado dele
        for (int k = 0; k < 4; k++) {
            int nx = a.npcs[i].x + DIRECOES_DX[k], ny = a.npcs[i].y + DIRECOES_DY[k];
            if (dentroDoMapa(nx, ny) && dChegada[ny][nx] != LONGE) { importantes.push_back({nx, ny}); break; }
        }

    // Testa todos os lugares; fica com o que deixa o portão mais longe da
    // chegada (um pouco de sorteio desempata).
    int melhorX = -1, melhorY = -1, melhorNota = -1;
    bool melhorInvertida = false;
    static Mapa teste;
    for (int rigor = 0; rigor < 3 && melhorX < 0; rigor++) {
    marcarProtegidas(a, dChegada, prot, rigor);
    for (int inv = 0; inv < 2; inv++)
    for (int oy = 1; oy + H < MAPA_ALTURA; oy++) {
        for (int ox = 1; ox + W < MAPA_LARGURA; ox++) {
            bool invertida = inv == 1;
            bool pode = true;
            for (int y = oy; y < oy + H && pode; y++)
                for (int x = ox; x < ox + W && pode; x++) if (prot[y][x]) pode = false;
            if (!pode) continue;
            teste = a.mapa;
            desenharSala(teste, tipo, ox, oy, invertida);
            int bx = -1, by = -1;
            if (atrasDaEscada(tipo, ox, oy, invertida, bx, by)) {
                TipoCelula cb = teste.celulas[by][bx];
                if (cb != PAREDE && cb != TOCHA) {
                    if (prot[by][bx] || cb == ESCADA_SOBE || cb == PORTA_TRANCADA) continue;
                    teste.celulas[by][bx] = PAREDE;
                }
            }
            int gx = ox + W / 2, gy = invertida ? oy : oy + H - 1, fora = invertida ? -1 : 1, fimX, fimY;
            if (!abrirCorredor(teste, gx, gy, fora, ox, oy, W, H, prot, dChegada, bx, by, fimX, fimY)) continue;
            // Nenhum caminho que existia pode ter ficado fechado (no rigor 2,
            // só os caminhos até as coisas importantes).
            distancias(teste, a.escadas.chegadaSobeX, a.escadas.chegadaSobeY, depois);
            auto naSala = [&](int x, int y) {
                return (x >= ox && x < ox + W && y >= oy && y < oy + H) || (x == bx && y == by);
            };
            if (rigor < 2) {
                for (int y = 0; y < MAPA_ALTURA && pode; y++)
                    for (int x = 0; x < MAPA_LARGURA && pode; x++)
                        if (!naSala(x, y) && dChegada[y][x] != LONGE && depois[y][x] == LONGE) pode = false;
            } else {
                for (auto& p : importantes) if (!naSala(p.first, p.second) && dChegada[p.second][p.first] != LONGE && depois[p.second][p.first] == LONGE) pode = false;
            }
            if (!pode || depois[gy - fora][gx] == LONGE) continue;
            int nota = depois[gy - fora][gx] * 8 + sortear(g, 0, 7);
            if (nota > melhorNota) { melhorNota = nota; melhorX = ox; melhorY = oy; melhorInvertida = invertida; }
        }
    }
    }
    if (melhorX < 0) {
        // Não coube (muito raro): a escada antiga volta e o mini-chefe fica
        // parado na frente dela, com os ajudantes do lado (continua obrigatório).
        if (a.escadas.temDescida) a.mapa.celulas[a.escadas.desceY][a.escadas.desceX] = ESCADA_DESCE;
        if (jogo.comMonstros && tipo != SALAO_DO_TRONO && a.escadas.temDescida) {
            int cx = a.escadas.chegadaDesceX, cy = a.escadas.chegadaDesceY;
            for (int i = 0; i < a.numMonstros; i++)
                if (a.monstros[i].vivo && std::abs(a.monstros[i].x - cx) <= 1 && std::abs(a.monstros[i].y - cy) <= 1)
                    a.monstros[i].vivo = a.monstros[i].carregaChave;   // (quem leva a chave fica)
            int tc = tipoPeloSimbolo(SIMBOLO_DO_CHEFE[tipo]);
            int c = criarMonstroExtra(a, tc, cx, cy, 2 * numeroDoAndar + 1, TIPOS_DE_MONSTRO[tc].elemento);
            if (c >= 0) a.monstros[c].estado = DORMINDO;
            int ajudantes = 0, ta = tipoDoAjudante(tipo);
            for (int k = 0; k < 8 && ajudantes < 2; k++) {
                int x = cx + DIRECOES_DX[k], y = cy + DIRECOES_DY[k];
                if (!dentroDoMapa(x, y) || a.mapa.celulas[y][x] != CHAO || monstroEm(a, x, y) >= 0) continue;
                int h = criarMonstroExtra(a, ta, x, y, 2 * numeroDoAndar - FOLGA_DOS_GUARDAS, jogo.elementoDaEspecie[ta]);
                if (h < 0) continue;
                a.monstros[h].estado = DORMINDO;
                a.monstros[h].guarda = true;
                ajudantes++;
            }
        }
        return false;
    }

    // ---- Carimba de verdade ----
    const int ox = melhorX, oy = melhorY;
    const bool invertida = melhorInvertida;
    desenharSala(a.mapa, tipo, ox, oy, invertida);
    SalaDoChefe& s = a.salaDoChefe;
    s.existe = true; s.tipo = tipo; s.x = ox; s.y = oy; s.largura = W; s.altura = H;
    s.invertida = invertida;
    s.portaoX = ox + W / 2; s.portaoY = invertida ? oy : oy + H - 1;
    int tapadoX = -1, tapadoY = -1;
    if (atrasDaEscada(tipo, ox, oy, invertida, tapadoX, tapadoY)) {
        a.mapa.celulas[tapadoY][tapadoX] = PAREDE;
        a.conteudoDasCaixas[tapadoY][tapadoX] = -1;
    }
    int fimX, fimY;
    abrirCorredor(a.mapa, s.portaoX, s.portaoY, invertida ? -1 : 1, ox, oy, W, H, prot, dChegada, tapadoX, tapadoY, fimX, fimY);
    int chefeX = ox + W / 2, chefeY = oy + 2;
    std::vector<std::pair<int,int>> ajudantes;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            char ch = letra(tipo, x, y, invertida);
            int X = ox + x, Y = oy + y;
            a.enfeites[Y][X] = 0;
            a.conteudoDasCaixas[Y][X] = -1;
            a.descoberto[Y][X] = false;
            if (ch == '>') {
                a.escadas.temDescida = true;
                a.escadas.desceX = X; a.escadas.desceY = Y;
                a.escadas.chegadaDesceX = X; a.escadas.chegadaDesceY = invertida ? Y - 1 : Y + 1;
            } else if (ch == 'K' || ch == 'O') {
                chefeX = X; chefeY = Y;
                if (ch == 'O') a.enfeites[Y][X] = 'O';
            } else if (ch == 'h') {
                ajudantes.push_back({X, Y});
            } else if (enfeiteSolido(ch) || ch == '$' || ch == 'o' || ch == 'v' || ch == '=') {
                a.enfeites[Y][X] = ch;
            }
        }
    }

    // O que estava dentro da sala muda de lugar (monstros e itens; a chave
    // não pode sumir) ou some (armadilhas).
    distancias(a.mapa, a.escadas.chegadaSobeX, a.escadas.chegadaSobeY, depois);
    auto naSala = [&](int x, int y) {
        return (x >= ox && x < ox + W && y >= oy && y < oy + H) || (x == tapadoX && y == tapadoY);
    };
    for (int i = 0; i < a.numMonstros; i++) {
        Monstro& m = a.monstros[i];
        if (!m.vivo || (tipoDe(m).especial & CHEFE) || m.guarda) continue;
        // (também quem ficou num canto sem saída, se a sala fechou um caminho)
        if (!naSala(m.x, m.y) && (depois[m.y][m.x] != LONGE || (tipoDe(m).especial & ANDA_NA_LAVA))) continue;
        int x, y;
        if (casaLivreFora(a, g, depois, x, y)) { m.x = x; m.y = y; }
        else if (!m.carregaChave) m.vivo = false;
    }
    for (int i = 0; i < a.numItens; i++) {
        ItemNoChao& it = a.itens[i];
        if (!it.ativo || !naSala(it.x, it.y)) continue;
        int x, y;
        if (casaLivreFora(a, g, depois, x, y)) { it.x = x; it.y = y; }
        else it.ativo = false;
    }
    for (int i = 0; i < a.numArmadilhas; i++) {
        Armadilha& ar = a.armadilhas[i];
        if (naSala(ar.x, ar.y) || a.mapa.celulas[ar.y][ar.x] != CHAO) { a.armadilhas[i] = a.armadilhas[--a.numArmadilhas]; i--; }
    }

    if (!jogo.comMonstros) return true;
    if (tipo == SALAO_DO_TRONO) {
        // O chefe e os 2 ogros (colocados antes) vão para os lugares deles.
        size_t k = 0;
        for (int i = 0; i < a.numMonstros; i++) {
            Monstro& m = a.monstros[i];
            if (!m.vivo) continue;
            if (tipoDe(m).especial & CHEFE) { m.x = chefeX; m.y = chefeY; }
            else if (m.guarda && k < ajudantes.size()) { m.x = ajudantes[k].first; m.y = ajudantes[k].second; k++; }
        }
        return true;
    }
    // O mini-chefe e os 2 ajudantes, dormindo nos lugares deles.
    int tipoDoChefe = tipoPeloSimbolo(SIMBOLO_DO_CHEFE[tipo]);
    int c = criarMonstroExtra(a, tipoDoChefe, chefeX, chefeY, 2 * numeroDoAndar + 1, TIPOS_DE_MONSTRO[tipoDoChefe].elemento);
    if (c >= 0) a.monstros[c].estado = DORMINDO;
    int tipoAjudante = tipoDoAjudante(tipo);
    for (auto& p : ajudantes) {
        int k = criarMonstroExtra(a, tipoAjudante, p.first, p.second, 2 * numeroDoAndar - FOLGA_DOS_GUARDAS,
                                  jogo.elementoDaEspecie[tipoAjudante]);
        if (k < 0) continue;
        a.monstros[k].estado = DORMINDO;
        a.monstros[k].guarda = true;
    }
    return true;
}

const char* tituloDoMiniChefe(int tipoDeSala) {
    static const char* const TITULOS[NUM_SALAS_DE_CHEFE] = {
        "O dono das Minas de Valdoria", "A mãe do ninho", "O guardião dos ossos", "O coração do vulcão",
        "O fim da masmorra" };
    return (tipoDeSala >= 0 && tipoDeSala < NUM_SALAS_DE_CHEFE) ? TITULOS[tipoDeSala] : "";
}

const char* falaDoMiniChefe(int tipoDeSala, bool primeira) {
    static const char* const PRIMEIRA[SALAO_DO_TRONO] = {
        "\"Mais um ladrão atrás do MEU ouro? Guardas! Peguem esse aí!\"",
        "\"Shhh... meus filhotes estão com fome. E você chegou bem na hora.\"",
        "\"Os mortos desta cripta me obedecem. Logo, você também vai.\"",
        "RRRRROOOMMM... A rocha se parte e dois olhos de fogo se acendem.",
    };
    static const char* const DE_NOVO[SALAO_DO_TRONO] = {
        "\"Você de novo? Guardas!\"",
        "\"Voltou para o jantar?\"",
        "\"Os ossos ainda lembram de você.\"",
        "O chão treme outra vez.",
    };
    if (tipoDeSala < 0 || tipoDeSala >= SALAO_DO_TRONO) return "";
    return primeira ? PRIMEIRA[tipoDeSala] : DE_NOVO[tipoDeSala];
}

// -----------------------------------------------------------------------------
// O PRÊMIO
// -----------------------------------------------------------------------------
void recompensaDoMiniChefe(Jogo& jogo, const Monstro& m) {
    int sala = salaDoMiniChefe(m.tipo);
    if (sala < 0) return;
    static const int MOEDAS[SALAO_DO_TRONO] = { 150, 200, 250, 300 };
    static const int PREMIOS[SALAO_DO_TRONO][3] = {
        { I_CRISTAL_DE_FORCA, -1, -1 },
        { I_CRISTAL_DE_VIDA, I_ANTIDOTO, I_ANTIDOTO },
        { I_CRISTAL_DE_FERRO, I_ETER, -1 },
        { I_CRISTAL_DE_FORCA, I_CRISTAL_DE_VIDA, I_POCAO_GRANDE },
    };
    jogo.jogador.moedas += MOEDAS[sala];
    tocarSom(SOM_MOEDAS);
    adicionarMensagem(jogo, "Prêmio do " + std::string(TIPOS_DE_MONSTRO[m.tipo].nome) + ": " +
                            std::to_string(MOEDAS[sala]) + " moedas!", MSG_BOA);
    for (int k = 0; k < 3; k++) {
        if (PREMIOS[sala][k] < 0) continue;
        guardarOuLargar(jogo, PREMIOS[sala][k]);
        if (TIPOS_DE_ITEM[PREMIOS[sala][k]].categoria != CRISTAL)
            adicionarMensagem(jogo, "Você pegou " + nomeComArtigo(PREMIOS[sala][k]) + ".", MSG_BOA);
    }
    adicionarMensagem(jogo, "O portão se abre. A escada atrás dele leva ao próximo andar.", MSG_BOA);
}
