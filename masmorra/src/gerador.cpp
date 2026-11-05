// gerador.cpp
#include "gerador.h"
#include "aleatorio.h"

// Direções retas usadas aqui: 0 = cima, 1 = direita, 2 = baixo, 3 = esquerda
// (são as 4 primeiras de DIRECOES_DX / DIRECOES_DY).
static int direcaoOposta(int d) {
    return (d + 2) % 4;
}

const int CENTRO_X = MAPA_LARGURA / 2;
const int CENTRO_Y = MAPA_ALTURA / 2;
const int TOTAL_CELULAS = MAPA_LARGURA * MAPA_ALTURA;

// =============================================================================
// MOLDE: uma sala desenhada "no papel", antes de ir para o mapa.
// Tem o mesmo tamanho do mapa; a sala é desenhada perto do centro.
// =============================================================================
enum TipoSala { SALA_RETANGULAR, SALA_CRUZ, SALA_CIRCULAR, SALA_CAVERNA };

struct Molde {
    bool chao[MAPA_ALTURA][MAPA_LARGURA];
    // Posição da porta em cada direção (-1 = não tem porta desse lado).
    int portaX[4];
    int portaY[4];
    TipoSala tipo;
};

// Retângulo que envolve todo o chão do molde (para não percorrer o molde inteiro).
struct Caixa {
    int x1, y1, x2, y2;
};

static void limparMolde(Molde& m, TipoSala tipo) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            m.chao[y][x] = false;
        }
    }
    for (int d = 0; d < 4; d++) {
        m.portaX[d] = -1;
        m.portaY[d] = -1;
    }
    m.tipo = tipo;
}

static void desenharRetangulo(Molde& m, int x, int y, int largura, int altura) {
    for (int linha = y; linha < y + altura; linha++) {
        for (int coluna = x; coluna < x + largura; coluna++) {
            if (dentroDoMapa(coluna, linha)) m.chao[linha][coluna] = true;
        }
    }
}

static Caixa caixaDoMolde(const Molde& m) {
    Caixa c = { MAPA_LARGURA, MAPA_ALTURA, -1, -1 };
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (!m.chao[y][x]) continue;
            if (x < c.x1) c.x1 = x;
            if (y < c.y1) c.y1 = y;
            if (x > c.x2) c.x2 = x;
            if (y > c.y2) c.y2 = y;
        }
    }
    return c;
}

// ---- Tipos de sala ---------------------------------------------------------

static void projetarRetangular(Molde& m, GeradorAleatorio& g) {
    limparMolde(m, SALA_RETANGULAR);
    int largura = sortear(g, 4, 12);
    int altura = sortear(g, 3, 6);
    desenharRetangulo(m, CENTRO_X - largura / 2, CENTRO_Y - altura / 2, largura, altura);
}

// Dois retângulos sobrepostos: um "deitado" e um "em pé".
static void projetarCruz(Molde& m, GeradorAleatorio& g) {
    limparMolde(m, SALA_CRUZ);
    int largH = sortear(g, 6, 14), altH = sortear(g, 2, 3);       // deitado
    int largV = sortear(g, 3, 5),  altV = sortear(g, altH + 2, altH + 5); // em pé

    int xH = CENTRO_X - largH / 2;
    int yH = CENTRO_Y - altH / 2;
    int xV = sortear(g, xH, xH + largH - largV);   // o "em pé" fica dentro da largura do deitado
    int yV = sortear(g, yH + altH - altV, yH);     // e cobre toda a altura dele

    desenharRetangulo(m, xH, yH, largH, altH);
    desenharRetangulo(m, xV, yV, largV, altV);
}

// Elipse. As letras do terminal são mais altas que largas, então o raio
// horizontal é quase o dobro do vertical para a sala parecer redonda.
static void projetarCircular(Molde& m, GeradorAleatorio& g) {
    limparMolde(m, SALA_CIRCULAR);
    int raioY = sortear(g, 2, 4);
    int raioX = raioY * 2 - 1 + sortear(g, 0, 1);
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            double dx = (double)(x - CENTRO_X) / raioX;
            double dy = (double)(y - CENTRO_Y) / raioY;
            if (dx * dx + dy * dy <= 1.1) m.chao[y][x] = true;
        }
    }
}

// Caverna com AUTÔMATO CELULAR:
//   1. sorteia cada célula: 55% de chance de ser chão;
//   2. repete 5 vezes: cada célula olha seus 8 vizinhos —
//        parede vira chão se tiver 6 ou mais vizinhos chão;
//        chão continua chão se tiver 4 ou mais vizinhos chão;
//   3. fica só a maior região conectada (flood fill com BFS).
// Devolve false se a caverna ficou pequena demais.
static bool projetarCaverna(Molde& m, GeradorAleatorio& g,
                            int largMin, int largMax, int altMin, int altMax) {
    int largura = sortear(g, largMin, largMax);
    int altura = sortear(g, altMin, altMax);

    static bool celula[MAPA_ALTURA][MAPA_LARGURA];
    static bool proxima[MAPA_ALTURA][MAPA_LARGURA];

    // 1. sorteio inicial (a borda é sempre parede)
    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) {
            bool borda = (x == 0 || y == 0 || x == largura - 1 || y == altura - 1);
            celula[y][x] = !borda && chance(g, 55);
        }
    }

    // 2. cinco gerações do autômato
    for (int geracao = 0; geracao < 5; geracao++) {
        for (int y = 0; y < altura; y++) {
            for (int x = 0; x < largura; x++) {
                int vizinhosChao = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < largura && ny >= 0 && ny < altura && celula[ny][nx]) {
                            vizinhosChao++;
                        }
                    }
                }
                bool borda = (x == 0 || y == 0 || x == largura - 1 || y == altura - 1);
                if (borda)            proxima[y][x] = false;
                else if (celula[y][x]) proxima[y][x] = (vizinhosChao >= 4);
                else                   proxima[y][x] = (vizinhosChao >= 6);
            }
        }
        for (int y = 0; y < altura; y++) {
            for (int x = 0; x < largura; x++) {
                celula[y][x] = proxima[y][x];
            }
        }
    }

    // 3. maior região conectada. "regiao" guarda o número da região de cada
    //    célula (0 = ainda não visitada). Usamos só as 4 direções retas para
    //    que nenhuma parte fique ligada apenas pela diagonal.
    static int regiao[MAPA_ALTURA][MAPA_LARGURA];
    static int filaX[TOTAL_CELULAS], filaY[TOTAL_CELULAS];
    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) regiao[y][x] = 0;
    }

    int numRegioes = 0, maiorRegiao = 0, maiorTamanho = 0;
    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) {
            if (!celula[y][x] || regiao[y][x] != 0) continue;

            numRegioes++;
            int inicio = 0, fim = 0, tamanho = 0;
            regiao[y][x] = numRegioes;
            filaX[fim] = x; filaY[fim] = y; fim++;
            while (inicio < fim) {
                int cx = filaX[inicio], cy = filaY[inicio];
                inicio++;
                tamanho++;
                for (int d = 0; d < 4; d++) {
                    int nx = cx + DIRECOES_DX[d], ny = cy + DIRECOES_DY[d];
                    if (nx < 0 || nx >= largura || ny < 0 || ny >= altura) continue;
                    if (!celula[ny][nx] || regiao[ny][nx] != 0) continue;
                    regiao[ny][nx] = numRegioes;
                    filaX[fim] = nx; filaY[fim] = ny; fim++;
                }
            }
            if (tamanho > maiorTamanho) {
                maiorTamanho = tamanho;
                maiorRegiao = numRegioes;
            }
        }
    }

    if (maiorTamanho < (largura * altura) / 4) return false;

    // Copia a maior região para o centro do molde.
    limparMolde(m, SALA_CAVERNA);
    int inicioX = CENTRO_X - largura / 2;
    int inicioY = CENTRO_Y - altura / 2;
    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) {
            if (regiao[y][x] == maiorRegiao) m.chao[inicioY + y][inicioX + x] = true;
        }
    }
    return true;
}

// ---- Portas e corredores do molde -------------------------------------------

// Para cada direção, escolhe uma célula da borda da sala que sirva de porta:
// ela não é chão, tem chão "atrás" dela, não tem chão dos lados, e nada de
// chão "na frente" até a borda (senão a porta abriria para dentro da sala).
static void escolherPortas(Molde& m, GeradorAleatorio& g) {
    static int candX[TOTAL_CELULAS], candY[TOTAL_CELULAS];

    for (int d = 0; d < 4; d++) {
        int dx = DIRECOES_DX[d], dy = DIRECOES_DY[d];
        int qtd = 0;

        for (int y = 1; y < MAPA_ALTURA - 1; y++) {
            for (int x = 1; x < MAPA_LARGURA - 1; x++) {
                if (m.chao[y][x]) continue;
                if (!m.chao[y - dy][x - dx]) continue;               // atrás
                if (m.chao[y + dx][x + dy] || m.chao[y - dx][x - dy]) continue; // lados

                bool frenteLivre = true;
                int fx = x + dx, fy = y + dy;
                while (dentroDoMapa(fx, fy)) {
                    if (m.chao[fy][fx]) { frenteLivre = false; break; }
                    fx += dx;
                    fy += dy;
                }
                if (!frenteLivre) continue;

                candX[qtd] = x;
                candY[qtd] = y;
                qtd++;
            }
        }

        if (qtd > 0) {
            int i = sortear(g, 0, qtd - 1);
            m.portaX[d] = candX[i];
            m.portaY[d] = candY[i];
        }
    }
}

// Estica um corredor a partir de uma das portas. Depois disso, a única
// porta do molde passa a ser a ponta do corredor.
static void adicionarCorredor(Molde& m, GeradorAleatorio& g) {
    int d = sortear(g, 0, 3);
    for (int tentativa = 0; tentativa < 4 && m.portaX[d] < 0; tentativa++) {
        d = (d + 1) % 4;
    }
    if (m.portaX[d] < 0) return;

    int dx = DIRECOES_DX[d], dy = DIRECOES_DY[d];
    // Corredores horizontais podem ser mais compridos (o mapa é largo).
    int comprimento = (dx != 0) ? sortear(g, 4, 12) : sortear(g, 2, 5);

    int x = m.portaX[d], y = m.portaY[d];
    int pontaX = x + dx * comprimento;
    int pontaY = y + dy * comprimento;
    if (pontaX < 1 || pontaX > MAPA_LARGURA - 2 || pontaY < 1 || pontaY > MAPA_ALTURA - 2) {
        return; // não cabe no molde: fica sem corredor
    }

    for (int k = 0; k < comprimento; k++) {
        m.chao[y + dy * k][x + dx * k] = true;
    }
    for (int i = 0; i < 4; i++) {
        m.portaX[i] = -1;
        m.portaY[i] = -1;
    }
    m.portaX[d] = pontaX;
    m.portaY[d] = pontaY;
}

// Sorteia o tipo da próxima sala e desenha no molde.
static void projetarSalaAleatoria(Molde& m, GeradorAleatorio& g) {
    int r = sortear(g, 1, 100);
    if (r <= 40) {
        projetarRetangular(m, g);
    } else if (r <= 65) {
        projetarCruz(m, g);
    } else if (r <= 78) {
        projetarCircular(m, g);
    } else if (!projetarCaverna(m, g, 10, 18, 5, 9)) {
        projetarRetangular(m, g); // caverna saiu pequena: usa retangular
    }

    escolherPortas(m, g);

    if (m.tipo != SALA_CAVERNA && chance(g, 35)) {
        adicionarCorredor(m, g);
    }
}

// =============================================================================
// ENCAIXE DAS SALAS NO MAPA
// =============================================================================

// O molde cabe no mapa com deslocamento (ox, oy)? Cada célula de chão precisa
// estar dentro do mapa e ter só rocha em volta (3x3), para a sala nova não
// encostar em nenhuma que já existe.
static bool cabeNoMapa(const Mapa& mapa, const Molde& m, const Caixa& c, int ox, int oy) {
    for (int y = c.y1; y <= c.y2; y++) {
        for (int x = c.x1; x <= c.x2; x++) {
            if (!m.chao[y][x]) continue;
            int mx = x + ox, my = y + oy;
            if (mx < 1 || mx > MAPA_LARGURA - 2 || my < 1 || my > MAPA_ALTURA - 2) return false;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (mapa.celulas[my + dy][mx + dx] != PAREDE) return false;
                }
            }
        }
    }
    return true;
}

static void carimbar(Mapa& mapa, const Molde& m, const Caixa& c, int ox, int oy) {
    for (int y = c.y1; y <= c.y2; y++) {
        for (int x = c.x1; x <= c.x2; x++) {
            if (m.chao[y][x]) mapa.celulas[y + oy][x + ox] = CHAO;
        }
    }
}

// Uma parede (x, y) serve de "conector" na direção d se: atrás dela tem chão
// (a área que já existe), na frente tem rocha (onde a sala nova vai) e dos
// dois lados também tem rocha.
static bool ehConector(const Mapa& mapa, int x, int y, int d) {
    if (x < 1 || x > MAPA_LARGURA - 2 || y < 1 || y > MAPA_ALTURA - 2) return false;
    if (mapa.celulas[y][x] != PAREDE) return false;

    int dx = DIRECOES_DX[d], dy = DIRECOES_DY[d];
    if (mapa.celulas[y - dy][x - dx] != CHAO) return false;   // atrás
    if (mapa.celulas[y + dy][x + dx] != PAREDE) return false; // frente
    if (mapa.celulas[y + dx][x + dy] != PAREDE) return false; // lado
    if (mapa.celulas[y - dx][x - dy] != PAREDE) return false; // outro lado
    return true;
}

// Embaralhamento de Fisher-Yates: deixa o vetor numa ordem aleatória.
static void embaralhar(int* vetor, int tamanho, GeradorAleatorio& g) {
    for (int i = tamanho - 1; i > 0; i--) {
        int j = sortear(g, 0, i);
        int t = vetor[i];
        vetor[i] = vetor[j];
        vetor[j] = t;
    }
}

// Mesmo embaralhamento, mas para dois vetores "casados" (x e y juntos).
static void embaralharPares(int* vx, int* vy, int tamanho, GeradorAleatorio& g) {
    for (int i = tamanho - 1; i > 0; i--) {
        int j = sortear(g, 0, i);
        int tx = vx[i]; vx[i] = vx[j]; vx[j] = tx;
        int ty = vy[i]; vy[i] = vy[j]; vy[j] = ty;
    }
}

// Procura (em ordem aleatória) um conector onde a sala do molde caiba.
// A porta da sala que fica do lado oposto ao da direção do conector é
// colocada exatamente em cima do conector.
static bool encaixarSala(Mapa& mapa, const Molde& m, GeradorAleatorio& g) {
    Caixa c = caixaDoMolde(m);
    if (c.x2 < 0) return false; // molde vazio

    static int ordem[TOTAL_CELULAS];
    for (int i = 0; i < TOTAL_CELULAS; i++) ordem[i] = i;
    embaralhar(ordem, TOTAL_CELULAS, g);

    for (int i = 0; i < TOTAL_CELULAS; i++) {
        int x = ordem[i] % MAPA_LARGURA;
        int y = ordem[i] / MAPA_LARGURA;
        if (mapa.celulas[y][x] != PAREDE) continue;

        int primeira = sortear(g, 0, 3);
        for (int k = 0; k < 4; k++) {
            int d = (primeira + k) % 4;
            int ladoDaPorta = direcaoOposta(d);
            if (m.portaX[ladoDaPorta] < 0) continue;
            if (!ehConector(mapa, x, y, d)) continue;

            int ox = x - m.portaX[ladoDaPorta];
            int oy = y - m.portaY[ladoDaPorta];
            if (!cabeNoMapa(mapa, m, c, ox, oy)) continue;

            carimbar(mapa, m, c, ox, oy);
            if (m.tipo == SALA_CAVERNA || !chance(g, 80)) {
                mapa.celulas[y][x] = CHAO;   // passagem aberta
            } else {
                mapa.celulas[y][x] = PORTA;
            }
            return true;
        }
    }
    return false;
}

// =============================================================================
// ATALHOS (loops)
// =============================================================================
// Procura paredes finas (uma célula) com chão dos dois lados. Se andar de um
// lado até o outro exige mais de "distanciaMinima" passos (ou é impossível),
// abre a parede. Assim o jogador tem caminhos alternativos para fugir.
static void abrirAtalhos(Mapa& mapa, GeradorAleatorio& g, int distanciaMinima) {
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    static int ordem[TOTAL_CELULAS];
    for (int i = 0; i < TOTAL_CELULAS; i++) ordem[i] = i;
    embaralhar(ordem, TOTAL_CELULAS, g);

    for (int i = 0; i < TOTAL_CELULAS; i++) {
        int x = ordem[i] % MAPA_LARGURA;
        int y = ordem[i] / MAPA_LARGURA;
        if (x < 1 || x > MAPA_LARGURA - 2 || y < 1 || y > MAPA_ALTURA - 2) continue;
        if (mapa.celulas[y][x] != PAREDE) continue;

        for (int orientacao = 0; orientacao < 2; orientacao++) {
            // orientacao 0: chão à esquerda e à direita; 1: em cima e embaixo
            int dx = (orientacao == 0) ? 1 : 0;
            int dy = (orientacao == 0) ? 0 : 1;

            if (mapa.celulas[y - dy][x - dx] != CHAO) continue;
            if (mapa.celulas[y + dy][x + dx] != CHAO) continue;
            if (mapa.celulas[y + dx][x + dy] != PAREDE) continue;
            if (mapa.celulas[y - dx][x - dy] != PAREDE) continue;

            calcularDistancias(mapa, x - dx, y - dy, distancia);
            int d = distancia[y + dy][x + dx];
            if (d == INALCANCAVEL || d > distanciaMinima) {
                mapa.celulas[y][x] = chance(g, 50) ? PORTA : CHAO;
                break;
            }
        }
    }
}

// =============================================================================
// ESCADAS
// =============================================================================
// A escada fica num "nicho" da parede: exatamente uma célula de chão encostada
// (em linha reta) e as outras três são parede.
static bool lugarDeEscada(const Mapa& mapa, int x, int y, int& frenteX, int& frenteY) {
    if (x < 1 || x > MAPA_LARGURA - 2 || y < 1 || y > MAPA_ALTURA - 2) return false;
    if (mapa.celulas[y][x] != PAREDE) return false;

    int chaos = 0;
    for (int d = 0; d < 4; d++) {
        int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
        TipoCelula t = mapa.celulas[ny][nx];
        if (t == CHAO) {
            chaos++;
            frenteX = nx;
            frenteY = ny;
        } else if (t != PAREDE) {
            return false; // porta ou outra escada encostada
        }
    }
    return chaos == 1;
}

static bool colocarEscadas(Mapa& mapa, Escadas& e, GeradorAleatorio& g, bool ultimoAndar) {
    static int candX[TOTAL_CELULAS], candY[TOTAL_CELULAS];
    static int frenteX[TOTAL_CELULAS], frenteY[TOTAL_CELULAS];
    int qtd = 0;

    // Escada que sobe: qualquer nicho serve.
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            int fx, fy;
            if (lugarDeEscada(mapa, x, y, fx, fy)) {
                candX[qtd] = x; candY[qtd] = y;
                frenteX[qtd] = fx; frenteY[qtd] = fy;
                qtd++;
            }
        }
    }
    if (qtd < 2) return false;

    int i = sortear(g, 0, qtd - 1);
    e.sobeX = candX[i];
    e.sobeY = candY[i];
    e.chegadaSobeX = frenteX[i];
    e.chegadaSobeY = frenteY[i];
    mapa.celulas[e.sobeY][e.sobeX] = ESCADA_SOBE;

    e.temDescida = !ultimoAndar;
    if (ultimoAndar) return true;

    // Escada que desce: entre os nichos mais longe da chegada (BFS),
    // sorteia um dos que estão a pelo menos 75% da maior distância.
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, distancia);

    qtd = 0;
    int maiorDistancia = 0;
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            int fx, fy;
            if (!lugarDeEscada(mapa, x, y, fx, fy)) continue;
            int d = distancia[fy][fx];
            if (d == INALCANCAVEL) continue;
            candX[qtd] = x; candY[qtd] = y;
            frenteX[qtd] = fx; frenteY[qtd] = fy;
            qtd++;
            if (d > maiorDistancia) maiorDistancia = d;
        }
    }
    if (qtd == 0) return false;

    int limite = maiorDistancia * 3 / 4;
    int escolhidos = 0;
    for (int k = 0; k < qtd; k++) {
        if (distancia[frenteY[k]][frenteX[k]] >= limite) {
            candX[escolhidos] = candX[k]; candY[escolhidos] = candY[k];
            frenteX[escolhidos] = frenteX[k]; frenteY[escolhidos] = frenteY[k];
            escolhidos++;
        }
    }

    i = sortear(g, 0, escolhidos - 1);
    e.desceX = candX[i];
    e.desceY = candY[i];
    e.chegadaDesceX = frenteX[i];
    e.chegadaDesceY = frenteY[i];
    mapa.celulas[e.desceY][e.desceX] = ESCADA_DESCE;
    return true;
}

// =============================================================================
// TOCHAS
// =============================================================================
// Tochas ficam presas em paredes retas (o mesmo tipo de nicho das escadas),
// longe umas das outras. Cada andar tem de 1 a 4.
static void colocarTochas(Mapa& mapa, GeradorAleatorio& g) {
    static int candX[TOTAL_CELULAS], candY[TOTAL_CELULAS];
    int qtd = 0;
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            int fx, fy;
            if (lugarDeEscada(mapa, x, y, fx, fy)) {
                candX[qtd] = x;
                candY[qtd] = y;
                qtd++;
            }
        }
    }
    embaralharPares(candX, candY, qtd, g);

    int quantas = sortear(g, 1, 4);
    int colocadas = 0;
    static int tochaX[4], tochaY[4];
    for (int i = 0; i < qtd && colocadas < quantas; i++) {
        bool longe = true;
        for (int t = 0; t < colocadas; t++) {
            int dx = candX[i] - tochaX[t], dy = candY[i] - tochaY[t];
            if (dx * dx + dy * dy < 15 * 15) longe = false;
        }
        if (!longe) continue;
        mapa.celulas[candY[i]][candX[i]] = TOCHA;
        tochaX[colocadas] = candX[i];
        tochaY[colocadas] = candY[i];
        colocadas++;
    }
}

// =============================================================================
// SALA DO TESOURO
// =============================================================================
// Testa cada porta do mapa: "se esta porta fosse parede, alguma parte do mapa
// ficaria isolada?". Se a parte isolada for uma sala de bom tamanho, sem
// escadas, e as escadas continuarem ligadas, a porta é candidata. Uma delas é
// sorteada e vira PORTA_TRANCADA.
static void escolherSalaDoTesouro(Mapa& mapa, const Escadas& e, GeradorAleatorio& g,
                                  SalaDoTesouro& tesouro) {
    tesouro.existe = false;
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    static int portaX[TOTAL_CELULAS], portaY[TOTAL_CELULAS];
    static int dentroX[TOTAL_CELULAS], dentroY[TOTAL_CELULAS];
    int qtd = 0;

    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (mapa.celulas[y][x] != PORTA) continue;

            mapa.celulas[y][x] = PAREDE;                 // e se fosse parede?
            calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, distancia);

            bool descidaLigada = !e.temDescida ||
                distancia[e.chegadaDesceY][e.chegadaDesceX] != INALCANCAVEL;
            int isoladas = 0;
            for (int cy = 0; cy < MAPA_ALTURA; cy++) {
                for (int cx = 0; cx < MAPA_LARGURA; cx++) {
                    if (ehPassavel(mapa, cx, cy) && !ehEscada(mapa, cx, cy) &&
                        distancia[cy][cx] == INALCANCAVEL) {
                        isoladas++;
                    }
                }
            }
            int dx = -1, dy = -1;
            for (int d = 0; d < 4; d++) {
                int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
                if (ehPassavel(mapa, nx, ny) && distancia[ny][nx] == INALCANCAVEL) {
                    dx = nx;
                    dy = ny;
                }
            }
            mapa.celulas[y][x] = PORTA;                  // desfaz o teste

            if (descidaLigada && dx >= 0 && isoladas >= 10 && isoladas <= 120) {
                portaX[qtd] = x; portaY[qtd] = y;
                dentroX[qtd] = dx; dentroY[qtd] = dy;
                qtd++;
            }
        }
    }
    if (qtd == 0) return;

    int i = sortear(g, 0, qtd - 1);
    mapa.celulas[portaY[i]][portaX[i]] = PORTA_TRANCADA;
    tesouro.existe = true;
    tesouro.portaX = portaX[i];
    tesouro.portaY = portaY[i];
    tesouro.dentroX = dentroX[i];
    tesouro.dentroY = dentroY[i];
}

// =============================================================================
// GERAÇÃO DO ANDAR
// =============================================================================
int gerarAndar(Mapa& mapa, Escadas& escadas, uint32_t semente, bool ultimoAndar,
               SalaDoTesouro* tesouro) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    static Molde molde;

    // Quase sempre dá certo de primeira; se o andar sair pequeno demais,
    // o gerador continua sorteando e tenta de novo.
    while (true) {
        encherDeParede(mapa);

        // 1. Primeira sala: caverna grande ou sala em cruz, em lugar sorteado.
        if (!(chance(g, 50) && projetarCaverna(molde, g, 20, 34, 8, 13))) {
            projetarCruz(molde, g);
        }
        Caixa c = caixaDoMolde(molde);
        int ox = sortear(g, 1 - c.x1, MAPA_LARGURA - 2 - c.x2);
        int oy = sortear(g, 1 - c.y1, MAPA_ALTURA - 2 - c.y2);
        carimbar(mapa, molde, c, ox, oy);
        int salas = 1;

        // 2. Acreção: tenta encaixar várias salas.
        for (int tentativa = 0; tentativa < 80; tentativa++) {
            projetarSalaAleatoria(molde, g);
            if (encaixarSala(mapa, molde, g)) salas++;
        }

        // 3. Atalhos.
        abrirAtalhos(mapa, g, 20);

        // 4. Escadas.
        if (salas >= 6 && colocarEscadas(mapa, escadas, g, ultimoAndar)) {
            // 5. Tochas (depois de tudo, para não mudar o resto do andar).
            colocarTochas(mapa, g);
            // 6. Sala do tesouro.
            if (tesouro != nullptr) escolherSalaDoTesouro(mapa, escadas, g, *tesouro);
            return salas;
        }
    }
}
