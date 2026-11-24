// tela.cpp
#include "sala_do_chefe.h"
#include "diario.h"
#include "conquistas.h"
#include "tela.h"
#include "jogo.h"
#include "tema.h"
#include "terminal.h"
#include "batalha.h"
#include "itens.h"
#include "pontuacao.h"
#include "bestiario.h"

#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// CORES
// -----------------------------------------------------------------------------
// Pensamos as cores em RGB (0 a 255 em cada componente), porque assim dá para
// "multiplicar" a cor de um objeto pela cor da luz que chega nele. Na hora de
// desenhar, cada cor é convertida para a mais parecida da paleta de 256 cores
// do terminal (funciona igual no Windows, Linux e Mac).
// Cor já convertida para a paleta: índice da frente e do fundo.
struct Cor {
    int frente;
    int fundo;
};

const int FUNDO_PRETO = 16;

// Aparência de cada tipo de célula, com as cores "em plena luz".
struct Aparencia {
    char simbolo;
    CorRGB frente;
    CorRGB fundo;
    bool brilhaSozinho; // tochas não dependem da luz para aparecer
};

// Fonte {, altar |, baú dourado %, gongo da arena 0 (apagados depois de usados).
static void aparenciaDoEspecial(const Especial& e, char& simbolo, CorRGB& cor) {
    const char SIMBOLOS[NUM_TIPOS_ESPECIAIS] = { '{', '|', '%', '0' };
    const CorRGB CORES[NUM_TIPOS_ESPECIAIS] = { { 120, 220, 255 }, { 230, 60, 70 }, { 255, 205, 60 }, { 215, 150, 70 } };
    simbolo = SIMBOLOS[e.tipo];
    cor = e.usado ? CorRGB{ 120, 120, 130 } : CORES[e.tipo];
}

// Chão decorado em volta das surpresas (as 8 casas vizinhas): pedra azulada na
// fonte, vermelho no altar, dourado no baú, areia na arena e roxo no
// acampamento do viajante. Devolve false se a casa não fica perto de nenhuma.
static bool chaoDoAmbiente(const Andar& andar, int x, int y, CorRGB& frente, CorRGB& fundo) {
    if (andar.mapa.celulas[y][x] != CHAO || andar.zonaSegura[y][x]) return false;
    const CorRGB FRENTE[5] = { { 150, 200, 230 }, { 220, 70, 80 }, { 240, 200, 90 }, { 220, 190, 140 }, { 200, 150, 100 } };
    const CorRGB FUNDO[5]  = { { 0, 95, 110 },    { 120, 0, 10 },  { 140, 95, 0 },  { 175, 135, 90 },  { 70, 0, 95 } };
    for (int d = 0; d < 8; d++) {
        int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
        if (!dentroDoMapa(nx, ny)) continue;
        int tipo = -1;
        int esp = especialEm(andar, nx, ny);
        if (esp >= 0) tipo = andar.especiais[esp].tipo;
        int npc = npcEm(andar, nx, ny);
        if (npc >= 0 && andar.npcs[npc].tipo == NPC_VIAJANTE) tipo = 4;
        if (tipo < 0) continue;
        frente = FRENTE[tipo];
        fundo = FUNDO[tipo];
        return true;
    }
    return false;
}

// Cores do chão e da parede de cada tema (minas, caverna, cripta, vulcão, trono).
static const CorRGB PAREDE_FRENTE[NUM_TEMAS] = { { 170, 155, 140 }, { 120, 165, 175 }, { 150, 160, 165 }, { 150, 95, 85 }, { 170, 175, 205 } };
static const CorRGB PAREDE_FUNDO[NUM_TEMAS]  = { { 105, 95, 85 },   { 50, 80, 95 },    { 55, 60, 66 },    { 70, 35, 35 }, { 75, 80, 110 } };
static const CorRGB CHAO_FRENTE[NUM_TEMAS]   = { { 185, 175, 160 }, { 150, 190, 195 }, { 175, 185, 180 }, { 190, 150, 130 }, { 190, 195, 215 } };
static const CorRGB CHAO_FUNDO[NUM_TEMAS]    = { { 62, 56, 50 },    { 36, 56, 62 },    { 40, 44, 46 },    { 56, 36, 34 }, { 50, 54, 72 } };

static Aparencia aparenciaDaCelula(const Mapa& mapa, int x, int y, int tema) {
    switch (mapa.celulas[y][x]) {
        case PAREDE:
            if (ehVisivelComoParede(mapa, x, y)) {
                return { '#', PAREDE_FRENTE[tema], PAREDE_FUNDO[tema], false };
            }
            return { ' ', { 0, 0, 0 }, { 0, 0, 0 }, false }; // rocha maciça
        case CHAO:         return { '.', CHAO_FRENTE[tema], CHAO_FUNDO[tema], false };
        case POCA:         return { '~', { 120, 190, 255 }, { 25, 70, 120 }, false };
        case LAVA:         return { '~', { 255, 200, 80 },  { 190, 50, 20 }, true };   // brilha sozinha
        case PORTA:        return { '+', { 235, 185, 95 },  { 115, 72, 30 }, false };
        // v1.23.1: a de descer (o caminho) é dourada; a de subir, cinza-azulada.
        case ESCADA_DESCE: return { '>', { 255, 215, 90 }, { 90, 60, 20 }, false };
        case ESCADA_SOBE:  return { '<', { 200, 215, 235 }, { 60, 66, 80 }, false };
        case TOCHA:        return { '#', { 255, 215, 90 },  { 150, 80, 25 }, true };
        case PORTA_TRANCADA: return { '+', { 255, 215, 0 }, { 120, 30, 30 }, false };
        case CAIXA:        return { '=', { 225, 170, 95 },  { 95, 60, 30 },  false };
        case TUMULO:       return { '+', { 190, 195, 200 }, CHAO_FUNDO[tema], false };   // lápide (cruz)
        case TEIA:         return { '%', { 220, 225, 235 }, CHAO_FUNDO[tema], false };
        case PORTAO:       return { '|', { 160, 165, 180 }, { 45, 48, 58 }, false };      // v1.24: grade aberta
    }
    return { '?', { 255, 0, 0 }, { 0, 0, 0 }, true };
}

// v1.24: cada enfeite das salas dos chefes vira uma letra colorida.
static void enfeiteNoConsole(char e, Aparencia& a) {
    switch (e) {
        case '$': a.simbolo = '$'; a.frente = { 255, 215, 80 }; break;    // ouro
        case 'R': case 'X': a.simbolo = 'W'; a.frente = { 230, 180, 70 }; a.fundo = { 110, 30, 40 }; break;   // trono
        case 'C': a.simbolo = '0'; a.frente = { 225, 225, 215 }; break;   // casulo
        case 'o': a.simbolo = 'o'; a.frente = { 215, 230, 200 }; break;   // ovos
        case 'O': a.simbolo = '*'; a.frente = { 120, 255, 150 }; break;   // círculo do ritual
        case 'v': a.simbolo = 'i'; a.frente = { 170, 255, 180 }; break;   // velas
        case 'c': a.simbolo = 'I'; a.frente = { 200, 205, 210 }; break;   // coluna
        case 'F': a.simbolo = 'M'; a.frente = { 255, 140, 50 }; break;    // forja
        case 'B': a.simbolo = 'n'; a.frente = { 160, 160, 170 }; break;   // bigorna
        case '=': a.simbolo = ':'; a.frente = { 220, 170, 70 }; a.fundo = { 110, 25, 40 }; break;   // tapete
        case 'b': a.simbolo = 'Y'; a.frente = { 255, 170, 60 }; break;    // braseiro
        default: break;
    }
}

static int limitar(int v, int minimo, int maximo) {
    if (v < minimo) return minimo;
    if (v > maximo) return maximo;
    return v;
}

// Cor do objeto x cor da luz. Luz 1.0 = cor original; luz 0 = preto.
static CorRGB iluminar(CorRGB c, Luz luz) {
    return { limitar((int)(c.r * luz.r), 0, 255),
             limitar((int)(c.g * luz.g), 0, 255),
             limitar((int)(c.b * luz.b), 0, 255) };
}

// Como uma célula lembrada aparece: escura e azulada, como no Brogue.
static CorRGB lembranca(CorRGB c) {
    int luminosidade = (c.r * 30 + c.g * 59 + c.b * 11) / 100;
    return { luminosidade * 35 / 100, luminosidade * 38 / 100, luminosidade * 65 / 100 };
}

// A paleta de 256 cores tem um "cubo" 6x6x6 de cores (índices 16 a 231),
// em que cada componente só pode valer um destes 6 níveis...
static const int NIVEIS_DO_CUBO[6] = { 0, 95, 135, 175, 215, 255 };

static int nivelMaisProximo(int v) {
    int melhor = 0;
    for (int i = 1; i < 6; i++) {
        int erroI = v - NIVEIS_DO_CUBO[i];
        int erroMelhor = v - NIVEIS_DO_CUBO[melhor];
        if (erroI * erroI < erroMelhor * erroMelhor) melhor = i;
    }
    return melhor;
}

// ...e uma escala de 24 cinzas (índices 232 a 255, de 8 até 238).
// Escolhemos o que ficar mais perto da cor pedida: o cubo ou um cinza.
static int rgbPara256(CorRGB c) {
    int r = limitar(c.r, 0, 255), g = limitar(c.g, 0, 255), b = limitar(c.b, 0, 255);

    int ir = nivelMaisProximo(r), ig = nivelMaisProximo(g), ib = nivelMaisProximo(b);
    int dr = r - NIVEIS_DO_CUBO[ir], dg = g - NIVEIS_DO_CUBO[ig], db = b - NIVEIS_DO_CUBO[ib];
    int erroCubo = dr * dr + dg * dg + db * db;

    int media = (r + g + b) / 3;
    int ic = limitar((media - 8 + 5) / 10, 0, 23);
    int cinza = 8 + 10 * ic;
    int erroCinza = (r - cinza) * (r - cinza) + (g - cinza) * (g - cinza) + (b - cinza) * (b - cinza);

    // Cores quase cinzas (pouca diferença entre o maior e o menor componente)
    // vão sempre para a escala de cinzas. Senão o cubo, que tem poucos níveis,
    // transforma um bege claro em rosa ou verde.
    int maior = r, menor = r;
    if (g > maior) maior = g;
    if (b > maior) maior = b;
    if (g < menor) menor = g;
    if (b < menor) menor = b;
    bool quaseCinza = (maior - menor) < 50;

    if (quaseCinza || erroCinza < erroCubo) return 232 + ic;
    return 16 + 36 * ir + 6 * ig + ib;
}

// -----------------------------------------------------------------------------
// Texto em UTF-8: letras acentuadas ocupam mais de 1 byte, então
// texto.size() não é o número de letras na tela. Estas funções contam e
// cortam pelo número de letras.
// -----------------------------------------------------------------------------
static bool ehInicioDeLetra(unsigned char c) {
    return (c & 0xC0) != 0x80; // bytes 10xxxxxx são continuação
}

static int contarLetras(const std::string& texto) {
    int n = 0;
    for (unsigned char c : texto) {
        if (ehInicioDeLetra(c)) n++;
    }
    return n;
}

// Corta o texto em "maximo" letras e completa com espaços até "maximo".
static std::string ajustarLargura(const std::string& texto, int maximo) {
    std::string resultado;
    int letras = 0;
    for (size_t i = 0; i < texto.size(); i++) {
        unsigned char c = texto[i];
        if (ehInicioDeLetra(c)) {
            if (letras == maximo) break;
            letras++;
        }
        resultado += texto[i];
    }
    resultado.append(maximo - letras, ' ');
    return resultado;
}

// -----------------------------------------------------------------------------
// Montagem da tela: tudo vai para uma string e é escrito de uma vez só.
// Só trocamos de cor quando ela muda, para a string ficar menor.
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Janela maior que 80 x 24 (tela cheia, por exemplo): o jogo é desenhado
// CENTRALIZADO. As margens são somadas a toda posição pelo Pincel.
//
// Quando a janela muda de tamanho, a tela inteira é apagada com fundo preto.
// Sem isso, o console do Windows preenche a área nova com a última cor usada
// em cada linha e aparecem faixas coloridas.
// -----------------------------------------------------------------------------
static int margemColunas = 0;
static int margemLinhas = 0;
static int colunasDaUltimaVez = -1;
static int linhasDaUltimaVez = -1;

// Devolve o código ANSI que apaga a tela, se a janela mudou de tamanho
// desde o último desenho (ou "" se não mudou).
static std::string ajustarAoTamanhoDaJanela() {
    int colunas = TELA_COLUNAS, linhas = TELA_LINHAS;
    tamanhoDoTerminal(colunas, linhas); // se não der para saber, usa 80 x 24

    margemColunas = (colunas > TELA_COLUNAS) ? (colunas - TELA_COLUNAS) / 2 : 0;
    margemLinhas = (linhas > TELA_LINHAS) ? (linhas - TELA_LINHAS) / 2 : 0;

    if (colunas == colunasDaUltimaVez && linhas == linhasDaUltimaVez) return "";
    colunasDaUltimaVez = colunas;
    linhasDaUltimaVez = linhas;

    // Apaga ("2J") e, para não depender de como cada terminal apaga, pinta
    // a janela inteira com espaços de fundo preto, linha por linha.
    std::string s = ansiCor(231, FUNDO_PRETO) + "\x1b[2J";
    for (int linha = 1; linha <= linhas; linha++) {
        // Na última linha, deixa a última coluna de fora: escrever no canto
        // inferior direito pode fazer alguns terminais rolarem a tela.
        int largura = (linha == linhas) ? colunas - 1 : colunas;
        s += ansiPosicao(linha, 1) + std::string(largura, ' ');
    }
    return s;
}

void forcarLimpezaDaTela() {
    colunasDaUltimaVez = -1;
}

struct Pincel {
    std::string saida;
    Cor atual = { -1, -1 };

    void cor(Cor c) {
        if (c.frente != atual.frente || c.fundo != atual.fundo) {
            saida += ansiCor(c.frente, c.fundo);
            atual = c;
        }
    }
    void irPara(int linha, int coluna) {
        saida += ansiPosicao(linha + margemLinhas, coluna + margemColunas);
    }
    void texto(const std::string& t, Cor c) {
        cor(c);
        saida += t;
    }
    void letra(char ch, Cor c) {
        cor(c);
        saida += ch;
    }
};

// Barra de vida no estilo do Brogue: o texto fica por cima de uma barra colorida.
static void desenharBarra(Pincel& p, const std::string& rotulo,
                          int valor, int maximo, int largura,
                          int corCheia, int corVazia) {
    int cheias = (maximo > 0) ? (valor * largura + maximo - 1) / maximo : 0;
    if (cheias < 0) cheias = 0;
    if (cheias > largura) cheias = largura;

    std::string conteudo = rotulo + " " + std::to_string(valor) + "/" + std::to_string(maximo);
    int espacoAntes = (largura - (int)conteudo.size()) / 2;
    std::string linha = std::string(espacoAntes, ' ') + conteudo;
    linha.resize(largura, ' ');

    for (int i = 0; i < largura; i++) {
        p.letra(linha[i], { 231, i < cheias ? corCheia : corVazia });
    }
}

// -----------------------------------------------------------------------------
// CÂMERA
// -----------------------------------------------------------------------------
// A tela mostra só um pedaço do mapa (CAMERA_LARGURA x CAMERA_ALTURA células),
// com o jogador no centro. (cameraX, cameraY) é a célula do mapa que aparece
// no canto superior esquerdo da tela.
//
// Perto das bordas a câmera para, para não mostrar o "lado de fora" do mapa:
// aí o jogador sai do centro e anda até a beirada.
// -----------------------------------------------------------------------------
static int limitarCamera(int valor, int minimo, int maximo) {
    if (maximo < minimo) return minimo; // mapa menor que a câmera
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

void posicaoDaCamera(const Jogo& jogo, int& cameraX, int& cameraY) {
    cameraX = limitarCamera(jogo.jogador.x - CAMERA_LARGURA / 2, 0, MAPA_LARGURA - CAMERA_LARGURA);
    cameraY = limitarCamera(jogo.jogador.y - CAMERA_ALTURA / 2, 0, MAPA_ALTURA - CAMERA_ALTURA);
}

// 83500 ms -> "1:23"
static std::string textoDoTempo(long long ms) {
    long long segundos = ms / 1000;
    std::string s = std::to_string(segundos % 60);
    if (s.size() < 2) s = "0" + s;
    return std::to_string(segundos / 60) + ":" + s;
}

// Junta as mensagens do turno numa linha. Se não couberem todas, mostra as
// mais recentes (as outras ficam no histórico, tecla M).
static std::string mensagensQueCabem(const Jogo& jogo, int largura) {
    std::string linha;
    for (int i = (int)jogo.mensagensDoTurno.size() - 1; i >= 0; i--) {
        std::string candidata = jogo.mensagensDoTurno[i] + (linha.empty() ? "" : " " + linha);
        if (contarLetras(candidata) > largura && !linha.empty()) break;
        linha = candidata;
    }
    return linha;
}

static const Monstro* monstroVisivelMaisProximo(const Jogo& jogo) {
    const Andar& andar = andarAtual(jogo);
    const Monstro* melhor = nullptr;
    int melhorDistancia = 1000000;
    for (int i = 0; i < andar.numMonstros; i++) {
        const Monstro& m = andar.monstros[i];
        if (!monstroAVista(jogo, m)) continue;
        int dx = m.x - jogo.jogador.x, dy = m.y - jogo.jogador.y;
        int d = dx * dx + dy * dy;
        if (d < melhorDistancia) {
            melhorDistancia = d;
            melhor = &m;
        }
    }
    return melhor;
}

// v1.25: o Tab alterna entre o mapa com zoom 2x (a câmera em volta do
// jogador) e o andar inteiro, uma coluna por célula (79 colunas cabem nas 80).
static bool mostrarMapaInteiro = false;
void alternarMapaInteiro() { mostrarMapaInteiro = !mostrarMapaInteiro; }

void desenharJogo(const Jogo& jogo) {
    Pincel p;
    p.saida = ajustarAoTamanhoDaJanela();
    const Cor COR_TEXTO = { 252, FUNDO_PRETO };
    const Cor COR_DICA  = { 244, FUNDO_PRETO };

    // Linha 1: mensagens deste turno
    p.irPara(1, 1);
    // Mensagens importantes do turno: dourado (boa) ou vermelho (perigo).
    Cor corDasMensagens = COR_TEXTO;
    if (jogo.tipoDoTurno == MSG_BOA) corDasMensagens = { 220, FUNDO_PRETO };
    if (jogo.tipoDoTurno == MSG_PERIGO) corDasMensagens = { 203, FUNDO_PRETO };
    p.texto(ajustarLargura(mensagensQueCabem(jogo, TELA_COLUNAS - 1), TELA_COLUNAS - 1), corDasMensagens);

    // Linhas 2..22: o pedaço do mapa que a câmera mostra.
    const Mapa& mapa = mapaAtual(jogo);
    const Andar& andar = jogo.andares[jogo.andar - 1];
    int cameraX, cameraY;
    posicaoDaCamera(jogo, cameraX, cameraY);
    int colunasDoMapa = CAMERA_LARGURA;
    if (mostrarMapaInteiro) { cameraX = 0; cameraY = 0; colunasDoMapa = MAPA_LARGURA; }

    // Tabela rápida: que monstro está em cada célula (-1 = nenhum).
    static int monstroNaCelula[MAPA_ALTURA][MAPA_LARGURA];
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) monstroNaCelula[y][x] = -1;
    }
    for (int i = 0; i < andar.numMonstros; i++) {
        // A sombra de longe não aparece (monstroAVista).
        if (monstroAVista(jogo, andar.monstros[i])) monstroNaCelula[andar.monstros[i].y][andar.monstros[i].x] = i;
    }
    // O mesmo para os itens no chão.
    static int itemNaCelula[MAPA_ALTURA][MAPA_LARGURA];
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) itemNaCelula[y][x] = -1;
    }
    for (int i = 0; i < andar.numItens; i++) {
        if (andar.itens[i].ativo) itemNaCelula[andar.itens[i].y][andar.itens[i].x] = i;
    }

    for (int linha = 0; linha < CAMERA_ALTURA; linha++) {
        p.irPara(linha + 2, 2); // coluna 2: sobra 1 coluna de cada lado (1 + 78 + 1)
        for (int coluna = 0; coluna < colunasDoMapa; coluna++) {
            // Converte a posição na tela para a posição no mapa.
            int x = cameraX + coluna;
            int y = cameraY + linha;

            char simbolo = ' ';
            Cor cor = { FUNDO_PRETO, FUNDO_PRETO };

            if (dentroDoMapa(x, y)) {
                Aparencia a = aparenciaDaCelula(mapa, x, y, temaDoAndar(jogo.andar));
                // v1.25: escada lacrada (mini-chefe vivo): grade vermelha.
                if (mapa.celulas[y][x] == ESCADA_DESCE && miniChefeQueLacraAEscada(andar) >= 0) {
                    a.simbolo = '#';
                    a.frente = { 230, 90, 80 };
                    a.fundo = { 80, 20, 20 };
                }
                // Lojinha do comerciante: tapete vinho no chão.
                if (andar.zonaSegura[y][x] && mapa.celulas[y][x] == CHAO) {
                    a.simbolo = ':';
                    a.frente = { 200, 150, 70 };
                    a.fundo = { 90, 25, 35 };
                }
                chaoDoAmbiente(andar, x, y, a.frente, a.fundo);
                // v1.24: os enfeites das salas dos chefes e o portão fechado.
                enfeiteNoConsole(andar.enfeites[y][x], a);
                if (mapa.celulas[y][x] == PORTAO && jogo.portaoFechado) { a.simbolo = '#'; a.frente = { 200, 205, 215 }; }
                if (jogo.visivel[y][x]) {
                    // Visível agora: cor do objeto multiplicada pela luz.
                    CorRGB frente = a.frente, fundo = a.fundo;
                    if (!a.brilhaSozinho) {
                        frente = iluminar(frente, jogo.luz[y][x]);
                        fundo = iluminar(fundo, jogo.luz[y][x]);
                    }
                    int npc = npcEm(andar, x, y);
                    int armadilha = armadilhaEm(andar, x, y);
                    if (x == jogo.jogador.x && y == jogo.jogador.y) {
                        a.simbolo = '@';                 // jogador por cima, na cor
                        frente = CORES_DO_JOGADOR[jogo.jogador.cor]; // escolhida
                        if (jogo.furtivo) frente = { 110, 110, 150 };   // furtivo: apagado
                    } else if (monstroNaCelula[y][x] >= 0) {
                        // Monstro: só aparece se a célula está visível (no
                        // escuro ou fora da visão, ele fica escondido). A cor
                        // é a da espécie (v1.23: o elemento só aparece na batalha).
                        const Monstro& m = andar.monstros[monstroNaCelula[y][x]];
                        a.simbolo = tipoDe(m).simbolo;
                        frente = tipoDe(m).cor;
                        // O fungo parado se passa por um cogumelo (v1.23).
                        if ((tipoDe(m).especial & DISFARCADO) && m.estado == DORMINDO) { a.simbolo = '"'; frente = { 110, 175, 255 }; }
                        // Atordoado: pisca em amarelo.
                        if (m.atordoadoMs > 0 && (relogioMs() / 300) % 2 == 0) frente = { 255, 226, 90 };
                        // Campeão: fundo dourado.
                        if (m.campeao) fundo = { 135, 95, 0 };
                    } else if (npc >= 0) {
                        a.simbolo = FICHAS_DOS_NPCS[andar.npcs[npc].tipo].simbolo;
                        frente = FICHAS_DOS_NPCS[andar.npcs[npc].tipo].cor;
                    } else if (especialEm(andar, x, y) >= 0) {
                        aparenciaDoEspecial(andar.especiais[especialEm(andar, x, y)], a.simbolo, frente);
                    } else if (armadilha >= 0 && andar.armadilhas[armadilha].descoberta) {
                        a.simbolo = '^';
                        frente = { 255, 90, 90 };
                    } else if (itemNaCelula[y][x] >= 0) {
                        const TipoItem& t = TIPOS_DE_ITEM[andar.itens[itemNaCelula[y][x]].tipo];
                        a.simbolo = t.simbolo;
                        frente = t.cor;
                    }
                    simbolo = a.simbolo;
                    cor = { rgbPara256(frente), rgbPara256(fundo) };
                } else if (andar.descoberto[y][x]) {
                    // Já visto antes: aparece pela memória, escuro e azulado
                    // (com os itens que o jogador viu ali).
                    int item = itemNaCelula[y][x];
                    if (especialEm(andar, x, y) >= 0) {
                        aparenciaDoEspecial(andar.especiais[especialEm(andar, x, y)], a.simbolo, a.frente);
                    } else if (item >= 0 && andar.itens[item].visto) {
                        a.simbolo = TIPOS_DE_ITEM[andar.itens[item].tipo].simbolo;
                        a.frente = TIPOS_DE_ITEM[andar.itens[item].tipo].cor;
                    }
                    simbolo = a.simbolo;
                    cor = { rgbPara256(lembranca(a.frente)), rgbPara256(lembranca(a.fundo)) };
                }
                // Nunca visto: fica a escuridão (espaço preto).
            }

            // ZOOM 2x: cada célula ocupa 2 colunas (o símbolo e um espaço com
            // o mesmo fundo). Como as letras do terminal são mais altas que
            // largas, assim a célula fica quase quadrada.
            p.letra(simbolo, cor);
            if (!mostrarMapaInteiro) p.letra(' ', cor);
        }
    }

    // Linha 23: status
    const Jogador& j = jogo.jogador;
    p.irPara(23, 1);
    std::string inicio = " Andar " + std::to_string(jogo.andar) + "  Nv " + std::to_string(j.nivel) + "  ";
    p.texto(inicio, COR_TEXTO);
    desenharBarra(p, "Vida", j.vida, j.vidaMaxima, 16, 124, 52);
    std::string resto = "  Vidas " + std::to_string(j.vidas) + "  $" + std::to_string(j.moedas)
                      + "  Pts " + std::to_string(calcularPontuacao(jogo))
                      + "  " + textoDoTempo(jogo.tempoMs) + (jogo.descansando ? " zZz " : " ")
                      + (jogo.furtivo ? "[furtivo] " : "")
                      + (j.pontosLivres > 0 ? "[C] " : "");
    p.texto(resto, COR_TEXTO);
    int usadas = contarLetras(inicio) + 16 + contarLetras(resto);

    // Monstro visível mais próximo, como na barra lateral do Brogue.
    const Monstro* perto = monstroVisivelMaisProximo(jogo);
    int sobra = TELA_COLUNAS - 1 - usadas;
    if (perto != nullptr && sobra > 2) {
        const TipoMonstro& t = tipoDe(*perto);
        p.letra(t.simbolo, { rgbPara256(t.cor), FUNDO_PRETO });
        std::string info = std::string(" ") + nomeDaEspecie(*perto) + " Nv" + std::to_string(perto->nivel);
        if (perto->estado == DORMINDO) info += " (dormindo)";
        if (perto->estado == FUGINDO)  info += " (fugindo)";
        p.texto(ajustarLargura(info, sobra - 1), COR_TEXTO);
    } else {
        p.texto(std::string(sobra > 0 ? sobra : 0, ' '), COR_TEXTO);
    }

    // Linha 24: lembrete de comandos
    p.irPara(24, 1);
    p.texto(ajustarLargura(mostrarMapaInteiro ? " MAPA INTEIRO (Tab: voltar ao zoom)  i: mochila  C: ficha  ?: ajuda"
                                              : " i: mochila  C: ficha  J: diário  E: bestiário  Z: descansar  Tab: mapa  ?: ajuda",
                           TELA_COLUNAS - 1), COR_DICA);

    p.saida += ansiResetar();
    escreverNaTela(p.saida);
}

// Desenha uma caixa com várias linhas no centro da tela.
static void desenharCaixa(const std::vector<std::string>& linhas) {
    int largura = 0;
    for (const std::string& l : linhas) {
        int n = contarLetras(l);
        if (n > largura) largura = n;
    }
    largura += 4; // margem
    if (largura > TELA_COLUNAS - 2) largura = TELA_COLUNAS - 2;
    int altura = (int)linhas.size() + 2;
    int topo = (TELA_LINHAS - altura) / 2 + 1;
    int esquerda = (TELA_COLUNAS - largura) / 2 + 1;

    const Cor COR_CAIXA = { 252, 236 };
    Pincel p;
    p.irPara(topo, esquerda);
    p.texto(std::string(largura, ' '), COR_CAIXA);
    for (size_t i = 0; i < linhas.size(); i++) {
        p.irPara(topo + 1 + (int)i, esquerda);
        p.texto("  " + ajustarLargura(linhas[i], largura - 4) + "  ", COR_CAIXA);
    }
    p.irPara(topo + altura - 1, esquerda);
    p.texto(std::string(largura, ' '), COR_CAIXA);
    p.saida += ansiResetar();
    escreverNaTela(p.saida);
}

void desenharAjuda(const Jogo& jogo) {
    std::string semente = std::to_string(jogo.semente);
    desenharCaixa({
        "COMANDOS",
        "",
        "Mover          setas  ou  W A S D",
        "Diagonais      Home PgUp End PgDn",
        "Teclado num.   7 8 9 / 4 6 / 1 2 3",
        "Batalhar       ande na direção do monstro",
        "Descansar      z  ou  Z   (cura mais a cada 5 níveis)",
        "               qualquer tecla para de descansar",
        "Furtivo        f  (liga/desliga: lento, silencioso, ataque surpresa)",
        "Mochila        i          Ficha (atributos)   C     Diário  J",
        "Mensagens      M          Bestiário  E     Sair  Q",
        "Mapa inteiro   Tab        (de novo: volta ao zoom)",
        "",
        "Na batalha: setas escolhem, Enter confirma, Esc volta.",
        "Pegue monstros dormindo: eles começam a luta dormindo.",
        "Escadas: ande para dentro de  >  (desce, dourada)  ou  <  (sobe, cinza)",
        "Itens: ! poção ) arma ] escudo * cristal ? pergaminho - chave ( suporte",
        "Mapa:   = caixa (empurre num monstro: atordoa; pode ter item)",
        "        ^ armadilha   & pessoa   # vermelho: escada lacrada pelo chefe",
        "",
        "Semente desta partida: " + semente,
        "Para jogar a mesma masmorra:  masmorra " + semente,
        "",
        "Aperte qualquer tecla para voltar.",
    });
}

void desenharAviso(const std::string& linha1, const std::string& linha2) {
    colunasDaUltimaVez = -1; // força a limpeza agora e no próximo desenho do jogo
    std::string s = ajustarAoTamanhoDaJanela() + ansiCor(252, FUNDO_PRETO)
                  + ansiPosicao(1, 1) + linha1 + ansiPosicao(2, 1) + linha2 + ansiResetar();
    escreverNaTela(s);
    colunasDaUltimaVez = -1;
}

void desenharHistorico(const Jogo& jogo) {
    std::vector<std::string> linhas;
    linhas.push_back("ÚLTIMAS MENSAGENS");
    linhas.push_back("");
    const int MAXIMO = 15;
    size_t inicio = jogo.historico.size() > (size_t)MAXIMO ? jogo.historico.size() - MAXIMO : 0;
    for (size_t i = inicio; i < jogo.historico.size(); i++) {
        linhas.push_back(jogo.historico[i]);
    }
    linhas.push_back("");
    linhas.push_back("Aperte qualquer tecla para voltar.");
    desenharCaixa(linhas);
}

void desenharMorte(const Jogo& jogo) {
    std::vector<std::string> linhas;
    if (jogo.venceu) {
        linhas.push_back("VITÓRIA!");
        linhas.push_back("");
        linhas.push_back(jogo.jogador.nome + " derrotou o Senhor das Profundezas");
        linhas.push_back("e libertou Valdoria!");
    } else {
        linhas.push_back("FIM DE JOGO");
        linhas.push_back("");
        linhas.push_back("Suas vidas acabaram no andar " + std::to_string(jogo.andar) + ",");
        linhas.push_back(jogo.causaDaMorte + ".");
    }
    linhas.push_back("");
    linhas.push_back("PONTUAÇÃO: " + std::to_string(jogo.pontuacaoFinal) + "   (andar " +
                     std::to_string(jogo.andarMaisFundo) + ", " + std::to_string(jogo.monstrosMortos) +
                     " monstros, " + std::to_string(jogo.itensColetados) + " itens, " +
                     std::to_string(jogo.passos) + " passos, " + dificuldadeAtual().nome + ")");
    if (jogo.posicaoNoRanking >= 0) {
        linhas.push_back("Você ficou em " + std::to_string(jogo.posicaoNoRanking + 1) + "o lugar no ranking!");
    }
    // v1.26: as conquistas desta partida.
    std::string medalhas;
    for (int id = 0; id < NUM_CONQUISTAS; id++) {
        if (!ganhouNaPartida(jogo, id)) continue;
        medalhas += (medalhas.empty() ? "" : ", ") + std::string(CONQUISTAS[id].nome) + (novaNaPartida(jogo, id) ? " (nova!)" : "");
    }
    linhas.push_back("Conquistas: " + (medalhas.empty() ? std::string("nenhuma nesta partida") : medalhas));
    linhas.push_back("");
    std::vector<RegistroRanking> ranking = lerRanking();
    for (int i = 0; i < (int)ranking.size() && i < 5; i++) {
        linhas.push_back(std::to_string(i + 1) + ". " + ranking[i].nome + "  " + std::to_string(ranking[i].pontos) +
                         "  " + ranking[i].dificuldade + (ranking[i].venceu ? "  (venceu)" : ""));
    }
    linhas.push_back("");
    linhas.push_back("N: nova partida      Q: menu");
    desenharCaixa(linhas);
}

void desenharPagina(const std::string& titulo, const std::vector<std::string>& linhas,
                    const std::string& rodape) {
    std::vector<std::string> todas;
    todas.push_back(titulo);
    todas.push_back("");
    for (const std::string& l : linhas) todas.push_back(l);
    todas.push_back("");
    todas.push_back(rodape);
    forcarLimpezaDaTela();
    escreverNaTela(ajustarAoTamanhoDaJanela());
    desenharCaixa(todas);
}

static std::vector<std::string> quebrarTexto(const std::string& texto, int largura);

void desenharCriacao(const Criacao& c) {
    std::vector<std::string> linhas;
    linhas.push_back("CRIE SEU PERSONAGEM");
    linhas.push_back("");
    std::vector<std::string> rotulos(NUM_LINHAS_DA_CRIACAO);
    rotulos[LINHA_NOME] = "Nome:         " + c.nome + (c.linha == LINHA_NOME ? "_" : "");
    rotulos[LINHA_ELEMENTO] = "Elemento:   < " + std::string(NOMES_DOS_ELEMENTOS[c.elemento]) + " >";
    rotulos[LINHA_COR] = "Cor:        < " + std::string(NOMES_DAS_CORES_DO_JOGADOR[c.cor]) + " >";
    rotulos[LINHA_DIFICULDADE] = "Dificuldade: < " + std::string(DIFICULDADES[c.dificuldade].nome) +
                                 (dificuldadeLiberada(c.dificuldade) ? "" : " (trancado)") + " >";
    for (int a = 0; a < NUM_ATRIBUTOS; a++) {
        std::string nome = NOMES_DOS_ATRIBUTOS[a];
        rotulos[LINHA_FORCA + a] = nome + ":" + std::string(13 - contarLetras(nome), ' ') + "- " +
                                   std::to_string(c.atributos[a]) + " +";
    }
    rotulos[LINHA_ALEATORIO] = "[ ALEATÓRIO ]";
    rotulos[LINHA_COMECAR] = "[ COMEÇAR A AVENTURA ]";
    for (int i = 0; i < NUM_LINHAS_DA_CRIACAO; i++) {
        linhas.push_back((i == c.linha ? "> " : "  ") + rotulos[i]);
        if (i == LINHA_DIFICULDADE) linhas.push_back("");
        if (i == LINHA_AGILIDADE) {
            linhas.push_back("  Pontos livres: " + std::to_string(c.pontosLivres));
            linhas.push_back("");
        }
    }
    linhas.push_back("");
    std::string dica;
    if (c.linha == LINHA_NOME) dica = "Digite o nome (Backspace apaga).";
    if (c.linha == LINHA_ELEMENTO) {
        Elemento e = (Elemento)c.elemento;
        dica = resumoDoElemento(e);
    }
    if (c.linha == LINHA_COR) dica = "A cor do seu @ no mapa.";
    if (c.linha == LINHA_DIFICULDADE) dica = dificuldadeLiberada(c.dificuldade) ? DIFICULDADES[c.dificuldade].descricao
                                                : "Trancado: vença o Difícil para liberar o modo Impossível.";
    if (c.linha >= LINHA_FORCA && c.linha <= LINHA_AGILIDADE) dica = EFEITOS_DOS_ATRIBUTOS[c.linha - LINHA_FORCA];
    if (c.linha == LINHA_ALEATORIO) dica = "Enter sorteia nome, elemento, cor e atributos.";
    if (c.linha == LINHA_COMECAR) dica = "Enter para começar.";
    for (const std::string& l : quebrarTexto(dica, 62)) linhas.push_back(l);   // v1.23: dicas longas em várias linhas
    linhas.push_back("Setas: escolher e mudar   Enter: próximo   Esc: voltar");
    forcarLimpezaDaTela();
    escreverNaTela(ajustarAoTamanhoDaJanela());
    desenharCaixa(linhas);
}

// =============================================================================
// TELA DE BATALHA (estilo Pokémon)
// =============================================================================
//   linhas  2-8  : painel do inimigo (esquerda) e desenho dele (direita)
//   linhas  9-15 : desenho do jogador (esquerda) e painel dele (direita)
//   linhas 17-23 : caixa de texto / menus
//   linha  24    : dica de teclas
// -----------------------------------------------------------------------------
static const char* ARTE_DO_JOGADOR[LINHAS_DA_ARTE] = {
    "        _",
    "       (o)      /",
    "      /|=|\\    /",
    "     / |=| \\__/",
    "       |_|",
    "       / \\",
    "      ^   ^",
};

static int corDoElemento(Elemento e) {
    switch (e) {
        case NORMAL: return 250;
        case FOGO:   return 208;
        case AGUA:   return 39;
        case PLANTA: return 77;
        case VENENO: return 141;
        case PEDRA:  return 179;
        default:     return 250;
    }
}

// Verde com mais da metade, amarelo até 1/5, vermelho abaixo disso.
static int corDaVida(int vida, int maximo) {
    if (vida * 2 > maximo) return 34;
    if (vida * 5 > maximo) return 178;
    return 160;
}

static void barra(Pincel& p, int valor, int maximo, int largura, int corCheia, int corVazia) {
    int cheias = (maximo > 0) ? valor * largura / maximo : 0;
    if (valor > 0 && cheias == 0) cheias = 1;     // um fiapo enquanto houver vida
    if (cheias > largura) cheias = largura;
    if (cheias < 0) cheias = 0;
    for (int i = 0; i < largura; i++) {
        p.letra(' ', { 231, i < cheias ? corCheia : corVazia });
    }
}

static std::string emMaiusculas(std::string texto) {
    for (size_t i = 0; i < texto.size(); i++) {
        unsigned char c = (unsigned char)texto[i];
        if (c >= 'a' && c <= 'z') {
            texto[i] = (char)(c - 'a' + 'A');
        } else if (c == 0xC3 && i + 1 < texto.size()) {
            // Letras acentuadas em UTF-8 (á, ã, é, í, ç...): 0xC3 + um byte de
            // 0xA0 a 0xBE; a maiúscula é esse byte menos 0x20.
            unsigned char d = (unsigned char)texto[i + 1];
            if (d >= 0xA0 && d <= 0xBE && d != 0xB7) texto[i + 1] = (char)(d - 0x20);
            i++;
        }
    }
    return texto;
}

// Quebra um texto em linhas de no máximo "largura" letras, sem cortar palavras.
static std::vector<std::string> quebrarTexto(const std::string& texto, int largura) {
    std::vector<std::string> linhas;
    std::string linha, palavra;
    for (size_t i = 0; i <= texto.size(); i++) {
        if (i == texto.size() || texto[i] == ' ') {
            std::string candidata = linha.empty() ? palavra : linha + " " + palavra;
            if (contarLetras(candidata) > largura && !linha.empty()) {
                linhas.push_back(linha);
                linha = palavra;
            } else {
                linha = candidata;
            }
            palavra.clear();
        } else {
            palavra += texto[i];
        }
    }
    if (!linha.empty()) linhas.push_back(linha);
    return linhas;
}

static void desenharPainel(Pincel& p, int linha, int coluna, const std::string& nome,
                           const Lutador& l, int vidaMostrada, bool mostrarNumeros,
                           int larguraDaBarra = 20, bool destacado = false) {
    const Cor BRANCO = { 231, FUNDO_PRETO };
    p.irPara(linha, coluna);
    p.texto(nome + "  Nv" + std::to_string(l.nivel) + "  ", destacado ? Cor{ 16, 220 } : BRANCO);
    p.texto(NOMES_DOS_ELEMENTOS[l.elemento], { corDoElemento(l.elemento), FUNDO_PRETO });
    if (l.status == ENVENENADO) p.texto("  ENV", { 141, FUNDO_PRETO });
    if (l.status == ADORMECIDO) p.texto("  DOR", { 75, FUNDO_PRETO });
    if (l.status == PARALISADO) p.texto("  PAR", { 220, FUNDO_PRETO });

    p.irPara(linha + 1, coluna);
    p.texto("VIDA ", { 250, FUNDO_PRETO });
    barra(p, vidaMostrada, l.vidaMaxima, larguraDaBarra, corDaVida(vidaMostrada, l.vidaMaxima), 238);
    if (mostrarNumeros) {
        p.texto(" " + std::to_string(vidaMostrada) + "/" + std::to_string(l.vidaMaxima), BRANCO);
    }
}

static std::string listaDeElementos(Elemento alvo, int efeitoProcurado) {
    std::string lista;
    for (int e = 0; e < NUM_ELEMENTOS; e++) {
        if (eficacia((Elemento)e, alvo) == efeitoProcurado) {
            if (!lista.empty()) lista += ", ";
            lista += NOMES_DOS_ELEMENTOS[e];
        }
    }
    return lista.empty() ? "nenhum" : lista;
}

// "o chacal A" -> "CHACAL A" (nome para os painéis).
static std::string nomeDoPainel(const Lutador& l) {
    size_t espaco = l.nome.find(' ');
    return emMaiusculas(espaco == std::string::npos ? l.nome : l.nome.substr(espaco + 1));
}

// Painel com os detalhes de um golpe (usado nos menus de golpes).
static void desenharInfoDoGolpe(Pincel& p, int linha, int coluna, int idGolpe, int ppAtual,
                                const Batalha& b) {
    const Cor TEXTO = { 252, FUNDO_PRETO };
    const Golpe& g = GOLPES[idGolpe];
    p.irPara(linha, coluna);
    p.texto("Elemento ", TEXTO);
    p.texto(NOMES_DOS_ELEMENTOS[g.elemento], { corDoElemento(g.elemento), FUNDO_PRETO });
    p.irPara(linha + 1, coluna);
    if (g.poder > 0) p.texto("Poder " + std::to_string(g.poder) + "   Precisão " + std::to_string(g.precisao) + "%", TEXTO);
    else             p.texto("Golpe de efeito   Precisão " + std::to_string(g.precisao) + "%", TEXTO);
    p.irPara(linha + 2, coluna);
    if (g.ppMaximo == 0) p.texto("PP --/--  (infinito)", TEXTO);
    else                 p.texto("PP " + std::to_string(ppAtual) + "/" + std::to_string(g.ppMaximo), TEXTO);
    // Contra quais inimigos o golpe é super eficaz (ou pouco eficaz).
    if (g.poder > 0) {
        std::string fortes, fracos;
        for (int i = 0; i < b.numInimigos; i++) {
            if (!b.naLuta[i]) continue;
            int e = eficacia(g.elemento, b.inimigos[i].elemento);
            std::string nome = nomeDoPainel(b.inimigos[i]);
            if (e > 2) fortes += (fortes.empty() ? "" : ", ") + nome;
            if (e < 2) fracos += (fracos.empty() ? "" : ", ") + nome;
        }
        int l = linha + 3;
        if (!fortes.empty()) {
            p.irPara(l++, coluna);
            p.texto(ajustarLargura("Super eficaz: " + fortes, 30), { 77, FUNDO_PRETO });
        }
        if (!fracos.empty() && l <= linha + 4) {
            p.irPara(l, coluna);
            p.texto(ajustarLargura("Pouco eficaz: " + fracos, 30), { 167, FUNDO_PRETO });
        }
    }
}

void desenharBatalha(const Jogo& jogo, const Batalha& b) {
    const Cor PRETO  = { FUNDO_PRETO, FUNDO_PRETO };
    const Cor TEXTO  = { 252, FUNDO_PRETO };
    const Cor APAGADO = { 244, FUNDO_PRETO };
    const Cor DESTAQUE = { 220, FUNDO_PRETO };

    Pincel p;
    p.saida = ajustarAoTamanhoDaJanela();
    // Limpa a área do jogo inteira (a tela muda bastante entre um quadro e outro).
    for (int l = 1; l <= TELA_LINHAS; l++) {
        p.irPara(l, 1);
        p.texto(std::string(l == TELA_LINHAS ? TELA_COLUNAS - 1 : TELA_COLUNAS, ' '), PRETO);
    }

    const Lutador& eu = b.jogador;
    const Jogador& j = jogo.jogador;
    // Inimigo destacado: o que está sendo escolhido como alvo (MENU_ALVO).
    int destacado = (b.menu == MENU_ALVO && b.cursor < b.numAlvos) ? b.alvos[b.cursor] : -1;

    if (b.numInimigos == 1) {
        // ---- Um inimigo: painel à esquerda (sem números, como no Pokémon) e
        //      desenho grande à direita.
        const TipoMonstro& tipo = tipoDe(*b.monstros[0]);
        desenharPainel(p, 3, 4, nomeDoPainel(b.inimigos[0]), b.inimigos[0], b.vidaMostrada[1], false);
        for (int i = 0; i < LINHAS_DA_ARTE; i++) {
            p.irPara(2 + i, 50);
            p.texto(tipo.arte[i], { rgbPara256(tipo.cor), FUNDO_PRETO });
        }
    } else {
        // ---- Vários inimigos: um por coluna (painel em cima, desenho embaixo).
        for (int k = 0; k < b.numInimigos; k++) {
            if (!b.naLuta[k]) continue;
            int coluna = 3 + k * 26;
            const TipoMonstro& tipo = tipoDe(*b.monstros[k]);
            desenharPainel(p, 1, coluna, nomeDoPainel(b.inimigos[k]), b.inimigos[k],
                           b.vidaMostrada[k + 1], false, 12, k == destacado);
            for (int i = 0; i < LINHAS_DA_ARTE; i++) {
                p.irPara(3 + i, coluna);
                p.texto(tipo.arte[i], { rgbPara256(tipo.cor), FUNDO_PRETO });
            }
        }
    }

    // ---- Jogador: desenho e painel (com números e experiência) ------------------
    int linhaDoJogador = (b.numInimigos == 1) ? 9 : 10;
    for (int i = 0; i < LINHAS_DA_ARTE; i++) {
        p.irPara(linhaDoJogador + i, 8);
        p.texto(ARTE_DO_JOGADOR[i], { 231, FUNDO_PRETO });
    }
    desenharPainel(p, 12, 42, "VOCÊ", eu, b.vidaMostrada[0], true);
    int xpDoNivel = experienciaParaNivel(j.nivel);
    int xpDoProximo = experienciaParaNivel(j.nivel + 1);
    p.irPara(14, 42);
    p.texto("XP   ", { 250, FUNDO_PRETO });
    barra(p, j.experiencia - xpDoNivel, xpDoProximo - xpDoNivel, 20, 33, 236);

    // ---- Caixa de texto ----------------------------------------------------------
    const Cor MOLDURA = { 245, FUNDO_PRETO };
    p.irPara(17, 2);
    p.texto("+" + std::string(76, '-') + "+", MOLDURA);
    for (int l = 18; l <= 22; l++) {
        p.irPara(l, 2);  p.texto("|", MOLDURA);
        p.irPara(l, 79); p.texto("|", MOLDURA);
    }
    p.irPara(23, 2);
    p.texto("+" + std::string(76, '-') + "+", MOLDURA);

    std::string dica = " Setas: escolher    Enter (ou z): confirmar    Esc (ou x): voltar";

    if (b.menu == MENU_NENHUM) {
        // Mensagem importante: colorida, com um sinal na frente, e sem o
        // [ok] enquanto as teclas estão travadas.
        Cor corDoTexto = TEXTO;
        std::string sinal;
        if (b.tipoDoTexto == MSG_BOA)    { corDoTexto = { 220, FUNDO_PRETO }; sinal = "* "; }
        if (b.tipoDoTexto == MSG_PERIGO) { corDoTexto = { 203, FUNDO_PRETO }; sinal = "! "; }
        std::vector<std::string> linhas = quebrarTexto(sinal + b.texto, 72);
        for (size_t i = 0; i < linhas.size() && i < 4; i++) {
            p.irPara(19 + (int)i, 5);
            p.texto(linhas[i], corDoTexto);
        }
        p.irPara(22, 72);
        p.texto(b.passaSozinhoEm < 0 && !b.travado ? "[ok]" : "    ", APAGADO);
        dica = b.travado ? " Leia com atenção..."
             : b.passaSozinhoEm > 0 ? " (passa sozinho; qualquer tecla adianta)"
             : b.passaSozinhoEm < 0 ? " Aperte qualquer tecla para continuar" : "";
    }
    else if (b.menu == MENU_PRINCIPAL) {
        std::vector<std::string> linhas = quebrarTexto(b.texto, 34);
        for (size_t i = 0; i < linhas.size() && i < 3; i++) {
            p.irPara(19 + (int)i, 5);
            p.texto(linhas[i], TEXTO);
        }
        const char* opcoes[4] = { "LUTAR", "ITENS", "STATUS", "FUGIR" };
        for (int i = 0; i < 4; i++) {
            p.irPara(19 + (i / 2) * 2, 48 + (i % 2) * 16);
            bool selecionada = (i == b.cursor);
            p.texto(selecionada ? "> " : "  ", DESTAQUE);
            p.texto(opcoes[i], selecionada ? DESTAQUE : TEXTO);
        }
        int pocoes = 0;
        for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
            if (TIPOS_DE_ITEM[i].categoria == POCAO) pocoes += j.mochila[i];
        }
        p.irPara(21, 5);
        p.texto("Poções na mochila: " + std::to_string(pocoes), APAGADO);
    }
    else if (b.menu == MENU_GOLPES || b.menu == MENU_ESQUECER) {
        bool esquecer = (b.menu == MENU_ESQUECER);
        int total = esquecer ? MAX_GOLPES + 1 : eu.numGolpes;
        for (int s = 0; s < total; s++) {
            p.irPara(18 + s, 5);
            bool selecionada = (s == b.cursor);
            p.texto(selecionada ? "> " : "  ", DESTAQUE);
            if (s < MAX_GOLPES && s < j.numGolpes) {
                const Golpe& g = GOLPES[j.golpes[s]];
                p.texto(ajustarLargura(g.nome, 18), selecionada ? DESTAQUE : TEXTO);
                p.texto(ajustarLargura(NOMES_DOS_ELEMENTOS[g.elemento], 8), { corDoElemento(g.elemento), FUNDO_PRETO });
                if (g.ppMaximo == 0) p.texto("  PP --", APAGADO);
                else p.texto("  PP " + std::to_string(j.pp[s]) + "/" + std::to_string(g.ppMaximo),
                             j.pp[s] == 0 ? Cor{ 160, FUNDO_PRETO } : APAGADO);
            } else {
                p.texto("Não aprender", selecionada ? DESTAQUE : TEXTO);
            }
        }
        if (esquecer) {
            p.irPara(18, 48);
            p.texto("Novo golpe:", APAGADO);
            desenharInfoDoGolpe(p, 19, 48, b.golpeNovo, GOLPES[b.golpeNovo].ppMaximo, b);
            p.irPara(18, 60);
            p.texto(GOLPES[b.golpeNovo].nome, DESTAQUE);
        } else if (b.cursor < eu.numGolpes) {
            desenharInfoDoGolpe(p, 18, 48, j.golpes[b.cursor], j.pp[b.cursor], b);
        }
    }
    else if (b.menu == MENU_ITENS) {
        for (int i = 0; i < b.numItensUsaveis && i < 5; i++) {
            int tipo = b.itensUsaveis[i];
            bool selecionada = (i == b.cursor);
            p.irPara(18 + i, 5);
            p.texto(selecionada ? "> " : "  ", DESTAQUE);
            p.texto(ajustarLargura(TIPOS_DE_ITEM[tipo].nome, 16), selecionada ? DESTAQUE : TEXTO);
            p.texto("x" + std::to_string(j.mochila[tipo]), APAGADO);
        }
        if (b.cursor < b.numItensUsaveis) {
            std::vector<std::string> linhas = quebrarTexto(TIPOS_DE_ITEM[b.itensUsaveis[b.cursor]].descricao, 28);
            for (size_t i = 0; i < linhas.size() && i < 4; i++) {
                p.irPara(18 + (int)i, 48);
                p.texto(linhas[i], TEXTO);
            }
        }
    }
    else if (b.menu == MENU_STATUS) {
        p.irPara(18, 5);
        p.texto("VOCÊ  Nv" + std::to_string(j.nivel) + "  Normal", DESTAQUE);
        p.irPara(19, 5);
        p.texto("Vida " + std::to_string(j.vida) + "/" + std::to_string(j.vidaMaxima)
                + "   XP " + std::to_string(j.experiencia) + "/" + std::to_string(xpDoProximo), TEXTO);
        p.irPara(20, 5);
        p.texto("Ataque " + std::to_string(eu.ataque) + "  Defesa " + std::to_string(eu.defesa)
                + "  Velocidade " + std::to_string(eu.velocidade), TEXTO);
        p.irPara(21, 5);
        p.texto("Estágios: ataque " + std::to_string(eu.estagioAtaque)
                + ", defesa " + std::to_string(eu.estagioDefesa), APAGADO);

        if (b.numInimigos == 1) {
            const Lutador& ele = b.inimigos[0];
            p.irPara(18, 48);
            p.texto(nomeDoPainel(ele) + "  Nv" + std::to_string(ele.nivel) + "  ", DESTAQUE);
            p.texto(NOMES_DOS_ELEMENTOS[ele.elemento], { corDoElemento(ele.elemento), FUNDO_PRETO });
            p.irPara(19, 48);
            p.texto(ajustarLargura("Fraco contra: " + listaDeElementos(ele.elemento, 4), 30), TEXTO);
            p.irPara(20, 48);
            p.texto(ajustarLargura("Resiste a: " + listaDeElementos(ele.elemento, 1), 30), TEXTO);
            p.irPara(21, 48);
            std::string st = (ele.status == SEM_STATUS) ? "normal" : NOMES_DOS_STATUS[ele.status];
            p.texto("Estado: " + st, TEXTO);
        } else {
            // Vários inimigos: uma linha para cada um, com a fraqueza.
            int linha = 18;
            for (int k = 0; k < b.numInimigos; k++) {
                if (!b.naLuta[k]) continue;
                const Lutador& ele = b.inimigos[k];
                p.irPara(linha++, 44);
                p.texto(ajustarLargura(nomeDoPainel(ele) + " Nv" + std::to_string(ele.nivel), 14), DESTAQUE);
                p.texto(ajustarLargura(NOMES_DOS_ELEMENTOS[ele.elemento], 7), { corDoElemento(ele.elemento), FUNDO_PRETO });
                p.texto(ajustarLargura("fraco: " + listaDeElementos(ele.elemento, 4), 13), TEXTO);
            }
        }
        dica = " Aperte qualquer tecla para voltar";
    }
    else if (b.menu == MENU_ALVO) {
        p.irPara(18, 5);
        p.texto(ajustarLargura(b.texto, 40), TEXTO);
        for (int i = 0; i < b.numAlvos; i++) {
            const Lutador& ele = b.inimigos[b.alvos[i]];
            bool selecionada = (i == b.cursor);
            p.irPara(19 + i, 5);
            p.texto(selecionada ? "> " : "  ", DESTAQUE);
            p.texto(ajustarLargura(nomeDoPainel(ele), 18), selecionada ? DESTAQUE : TEXTO);
            p.texto(NOMES_DOS_ELEMENTOS[ele.elemento], { corDoElemento(ele.elemento), FUNDO_PRETO });
        }
    }

    p.irPara(24, 1);
    p.texto(ajustarLargura(dica, TELA_COLUNAS - 1), APAGADO);
    p.saida += ansiResetar();
    escreverNaTela(p.saida);
}

// =============================================================================
// CAIXA DE MENU (mochila, escolher golpe para esquecer...)
// =============================================================================
void desenharMenuEmCaixa(const std::string& titulo, const std::vector<std::string>& opcoes,
                         const std::string& descricao, int cursor) {
    const Cor CAIXA    = { 252, 236 };
    const Cor TITULO   = { 220, 236 };
    const Cor DESTAQUE = { 16, 220 };   // opção selecionada: preto no amarelo
    const Cor DICA     = { 246, 236 };
    const int LARGURA = 64;

    int visiveis = (int)opcoes.size();
    if (visiveis > 12) visiveis = 12;
    // Se houver muitas opções, mostra uma "janela" de 12 em volta do cursor.
    int primeira = cursor - visiveis / 2;
    if (primeira < 0) primeira = 0;
    if (primeira + visiveis > (int)opcoes.size()) primeira = (int)opcoes.size() - visiveis;

    int altura = visiveis + 7;
    int topo = (TELA_LINHAS - altura) / 2 + 1;
    int esquerda = (TELA_COLUNAS - LARGURA) / 2 + 1;

    Pincel p;
    int linha = topo;
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    p.irPara(linha++, esquerda); p.texto("  " + ajustarLargura(titulo, LARGURA - 4) + "  ", TITULO);
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    for (int i = primeira; i < primeira + visiveis; i++) {
        p.irPara(linha++, esquerda);
        p.texto("  ", CAIXA);
        bool selecionada = (i == cursor);
        p.texto(ajustarLargura((selecionada ? "> " : "  ") + opcoes[i], LARGURA - 4), selecionada ? DESTAQUE : CAIXA);
        p.texto("  ", CAIXA);
    }
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    p.irPara(linha++, esquerda); p.texto("  " + ajustarLargura(descricao, LARGURA - 4) + "  ", CAIXA);
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    p.irPara(linha++, esquerda);
    p.texto("  " + ajustarLargura("Setas: escolher   Enter: confirmar   Esc: voltar", LARGURA - 4) + "  ", DICA);
    p.saida += ansiResetar();
    escreverNaTela(p.saida);
}


// Dica (dicas.cpp): caixa amarela no meio do mapa, que espera uma tecla.
void desenharDica(const Jogo& jogo, const std::string& titulo, const std::string& texto, bool travada) {
    desenharJogo(jogo);
    const Cor CAIXA  = { 230, 236 };
    const Cor TITULO = { 220, 236 };
    const Cor RODAPE = { 246, 236 };
    const int LARGURA = 66;
    std::vector<std::string> linhas = quebrarTexto(texto, LARGURA - 4);
    int altura = (int)linhas.size() + 6;
    int topo = (TELA_LINHAS - altura) / 2 + 1;
    int esquerda = (TELA_COLUNAS - LARGURA) / 2 + 1;
    Pincel p;
    int linha = topo;
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    p.irPara(linha++, esquerda); p.texto("  " + ajustarLargura("DICA: " + titulo, LARGURA - 4) + "  ", TITULO);
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    for (const std::string& l : linhas) {
        p.irPara(linha++, esquerda);
        p.texto("  " + ajustarLargura(l, LARGURA - 4) + "  ", CAIXA);
    }
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    p.irPara(linha++, esquerda);
    p.texto("  " + ajustarLargura(travada ? "Leia com atenção..." : "Aperte qualquer tecla para continuar  (Opções: dicas)",
                                  LARGURA - 4) + "  ", RODAPE);
    p.irPara(linha++, esquerda); p.texto(std::string(LARGURA, ' '), CAIXA);
    p.saida += ansiResetar();
    escreverNaTela(p.saida);
}

// Animações e efeitos: só existem na versão gráfica.
void animarBatalha(const Jogo&, const Batalha&, int, int, int, int, int, bool, int) {}
void efeitoNoMapa(const Jogo&, int, int, int, int) {}

// ---------------------------------------------------------------------------
// BESTIÁRIO (tecla E no jogo e "Bestiário" no menu): lista das espécies;
// Enter abre a ficha. As que ainda não apareceram ficam como "???".
// ---------------------------------------------------------------------------
static std::string primeiraMaiuscula(std::string s) {
    if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = (char)(s[0] - 'a' + 'A');
    return s;
}

static std::vector<std::string> linhasDaFicha(const Jogo* jogo, int tipo) {
    const TipoMonstro& t = TIPOS_DE_MONSTRO[tipo];
    const FichaDoBestiario& f = fichaDoBestiario(tipo);
    std::vector<std::string> linhas;
    if (!f.visto) {
        linhas.push_back("Ainda não encontrado.");
        linhas.push_back("Aparece: " + andaresDaEspecie(tipo));
        return linhas;
    }
    linhas.push_back("Aparece: " + andaresDaEspecie(tipo));
    for (const std::string& l : quebrarTexto(descricaoDaEspecie(tipo), 60)) linhas.push_back(l);
    linhas.push_back("");
    if (jogo && enfrentadaNaPartida(*jogo, tipo)) {
        Elemento e = jogo->elementoDaEspecie[tipo];
        linhas.push_back(std::string("Nesta partida: ") + NOMES_DOS_ELEMENTOS[e] + "  (fraco contra: " +
                         listaDeElementos(e, 4) + ")");
    } else if (jogo) {
        linhas.push_back("Nesta partida: lute com ele para descobrir o elemento.");
    } else {
        linhas.push_back("O elemento é sorteado a cada partida.");
    }
    std::string elementos;
    for (int e = 0; e < NUM_ELEMENTOS; e++) {
        if (!(f.elementosVistos & (1 << e))) continue;
        elementos += (elementos.empty() ? "" : ", ") + std::string(NOMES_DOS_ELEMENTOS[e]);
    }
    linhas.push_back("Já apareceu como: " + (elementos.empty() ? std::string("-") : elementos));
    linhas.push_back("");
    if (f.derrotados > 0) {
        linhas.push_back("Atributos: Vida " + std::to_string(t.baseVida) + "  Ataque " + std::to_string(t.baseAtaque) +
                         "  Defesa " + std::to_string(t.baseDefesa) + "  Velocidade " + std::to_string(t.baseVelocidade));
    } else {
        linhas.push_back("Atributos: vença um para descobrir.");
    }
    std::string golpes;
    int vistos = 0;
    for (int g = 0; g < NUM_GOLPES; g++) {
        if (!f.golpesVistos.test(g)) continue;
        golpes += (golpes.empty() ? "" : ", ") + std::string(GOLPES[g].nome);
        vistos++;
    }
    for (int k = vistos; k < t.numGolpes; k++) golpes += (golpes.empty() ? "" : ", ") + std::string("???");
    for (const std::string& l : quebrarTexto("Golpes: " + golpes, 60)) linhas.push_back(l);
    linhas.push_back("");
    std::string conta = "Derrotados: " + std::to_string(f.derrotados);
    if (!(t.especial & CHEFE) && t.frequencia > 0) conta += "   Campeões: " + std::to_string(f.campeoesDerrotados);
    linhas.push_back(conta);
    return linhas;
}

void mostrarBestiario(const Jogo* jogo) {
    int cursor = 0;
    int total = NUM_TIPOS_DE_MONSTRO;
    while (true) {
        std::vector<std::string> opcoes;
        for (int t = 0; t < total; t++) {
            const FichaDoBestiario& f = fichaDoBestiario(t);
            if (!f.visto) { opcoes.push_back("???"); continue; }
            std::string nome = primeiraMaiuscula(TIPOS_DE_MONSTRO[t].nome);
            std::string conta = f.derrotados > 0 ? std::to_string(f.derrotados) + " derrotado" + (f.derrotados == 1 ? "" : "s") : "";
            int espacos = 30 - contarLetras(nome);
            opcoes.push_back(nome + std::string(espacos > 1 ? espacos : 1, ' ') + conta);
        }
        opcoes.push_back("Voltar");
        std::string descricao = cursor < total
            ? (fichaDoBestiario(cursor).visto ? "Aparece: " + andaresDaEspecie(cursor) + ".  Enter: ver a ficha."
                                               : "Ainda não encontrado. Aparece: " + andaresDaEspecie(cursor) + ".")
            : "Volta.";
        desenharMenuEmCaixa("BESTIÁRIO  (" + std::to_string(especiesVistas()) + " de " + std::to_string(total) +
                            " espécies vistas)", opcoes, descricao, cursor);
        int tecla = lerTecla();
        if (tecla == TECLA_REDIMENSIONOU) { forcarLimpezaDaTela(); continue; }
        if (tecla == TECLA_CIMA) cursor = (cursor + total) % (total + 1);
        if (tecla == TECLA_BAIXO) cursor = (cursor + 1) % (total + 1);
        if (tecla == TECLA_ESC || tecla == 'e' || tecla == 'E' || tecla == 'x') break;
        if (tecla == TECLA_ENTER || tecla == ' ') {
            if (cursor == total) break;
            std::string titulo = fichaDoBestiario(cursor).visto ? emMaiusculas(TIPOS_DE_MONSTRO[cursor].nome) : "???";
            desenharPagina(titulo, linhasDaFicha(jogo, cursor), "Aperte qualquer tecla para voltar à lista.");
            lerTecla();
            forcarLimpezaDaTela();
        }
    }
    forcarLimpezaDaTela();
}

// O terminal não tem zoom (o mapa já usa a janela toda).
void mudarZoomDoMapa(int) {}

// Falas e história no terminal: a página de texto de sempre.
void mostrarFala(const Jogo& jogo, int npc, const std::vector<std::string>& linhas) {
    const Andar& andar = andarAtual(jogo);
    std::string nome = (npc >= 0 && npc < andar.numNpcs) ? FICHAS_DOS_NPCS[andar.npcs[npc].tipo].nome : "";
    // "> " na frente: quem fala é o herói. Linhas compridas são quebradas.
    std::vector<std::string> texto;
    for (const std::string& l : linhas) {
        std::string linha = l.rfind("> ", 0) == 0 ? jogo.jogador.nome + ": " + l.substr(2) : l;
        if (linha.empty()) { texto.push_back(""); continue; }
        for (const std::string& parte : quebrarTexto(linha, 66)) texto.push_back(parte);
    }
    mostrarPagina(nome, texto);
}

void mostrarHistoria(const Jogo&, const std::vector<std::string>& linhas) {
    mostrarPagina("A MASMORRA DE VALDORIA", linhas);
}

// Cenas no terminal: só o texto (sem câmera nem som).
void cenaDaCorneta(const Jogo&, int, bool) {}

void cenaDoChefe(const Jogo&, int, const std::vector<std::string>& falas, bool) {
    mostrarPagina("O SENHOR DAS PROFUNDEZAS", falas);
}

// v1.24: no terminal, a entrada na sala do mini-chefe é uma página de texto.
void cenaDoMiniChefe(const Jogo& jogo, int chefe, bool primeira) {
    const Monstro& m = andarAtual(jogo).monstros[chefe];
    int sala = salaDoMiniChefe(m.tipo);
    std::string nome = TIPOS_DE_MONSTRO[m.tipo].nome;
    for (char& c : nome) if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    mostrarPagina(nome, { "O portão desce com um estrondo atrás de você!", "",
                          std::string(tituloDoMiniChefe(sala)) + ".", "", falaDoMiniChefe(sala, primeira) });
}

void falaDoChefeNaBatalha(const Jogo& jogo, const Batalha& b, int inimigo, const std::string& fala) {
    Batalha copia = b;
    copia.texto = primeiraMaiuscula(b.inimigos[inimigo].nome) + ": \"" + fala + "\"";
    desenharBatalha(jogo, copia);
    lerTecla();
}

void mostrarFinal(const Jogo&, const std::vector<std::string>& linhas) {
    mostrarPagina("VALDORIA ESTÁ LIVRE", linhas);
}

// v1.25: a ficha no console continua sendo a lista de atributos.
int escolherNaFicha(const Jogo& jogo, int& cursor) {
    const Jogador& j = jogo.jogador;
    std::vector<std::string> opcoes, descricoes;
    std::string resumo = "Ataque " + std::to_string(j.ataque) + "  Defesa " + std::to_string(j.defesa) +
                         "  Vida " + std::to_string(j.vidaMaxima) + "  Velocidade " + std::to_string(j.velocidade) +
                         "  Precisão +" + std::to_string(j.precisaoExtra) + "%  Esquiva " +
                         std::to_string(j.esquiva) + "%  Crítico " + std::to_string(6 + j.criticoExtra) + "%";
    for (int a = 0; a < NUM_ATRIBUTOS; a++) {
        // Alinha os números: conta letras, não bytes ("ç" e "ã" ocupam 2 bytes).
        std::string nome = NOMES_DOS_ATRIBUTOS[a];
        int letras = 0;
        for (unsigned char ch : nome) if ((ch & 0xC0) != 0x80) letras++;
        nome += std::string(letras < 16 ? 16 - letras : 1, ' ');
        opcoes.push_back(nome + std::to_string(j.atributos[a]) + (j.pontosLivres > 0 ? "   [+]" : ""));
        descricoes.push_back(std::string(EFEITOS_DOS_ATRIBUTOS[a]) + "  |  " + resumo);
    }
    std::string titulo = "FICHA DE " + j.nome + "   (" + NOMES_DOS_ELEMENTOS[j.elemento] + ")   pontos livres: " +
                         std::to_string(j.pontosLivres);
    int escolha = escolherEmCaixa(jogo, titulo, opcoes, descricoes);
    if (escolha >= 0) cursor = escolha;
    return escolha;
}

// v1.25: a mochila no console continua sendo a lista de itens.
bool mochilaMostraEquipamento() { return false; }

// v1.27: no console, a caixa de opções com o elemento, o poder e o PP embaixo.
int escolherGolpeParaEsquecer(const Jogo& jogo, int golpeNovo) {
    const Jogador& j = jogo.jogador;
    auto resumo = [](int g, int pp) {
        const Golpe& x = GOLPES[g];
        std::string r = std::string(NOMES_DOS_ELEMENTOS[x.elemento]) + "  -  " +
                        (x.poder > 0 ? "poder " + std::to_string(x.poder) : std::string("sem dano")) +
                        "  -  precisão " + std::to_string(x.precisao) + "%  -  PP ";
        r += x.ppMaximo > 0 ? std::to_string(pp) + "/" + std::to_string(x.ppMaximo) : std::string("sem limite");
        return r;
    };
    std::vector<std::string> opcoes, descricoes;
    for (int s = 0; s < j.numGolpes; s++) {
        opcoes.push_back(GOLPES[j.golpes[s]].nome);
        descricoes.push_back(resumo(j.golpes[s], j.pp[s]));
    }
    opcoes.push_back("Não aprender");
    descricoes.push_back(std::string("Novo: ") + GOLPES[golpeNovo].nome + "  -  " + resumo(golpeNovo, GOLPES[golpeNovo].ppMaximo));
    int lugar = escolherEmCaixa(jogo, "Aprender " + std::string(GOLPES[golpeNovo].nome) + ": esquecer qual golpe?",
                                opcoes, descricoes);
    return (lugar >= 0 && lugar < j.numGolpes) ? lugar : -1;
}
int escolherNaMochila(const Jogo& jogo, const std::vector<int>& tipos, int& cursor) {
    const Jogador& j = jogo.jogador;
    std::vector<std::string> opcoes, descricoes;
    for (int i : tipos) {
        if (i == I_CHAVE) {
            // A chave é deste andar e não ocupa espaço.
            opcoes.push_back("Chave do andar " + std::to_string(jogo.andar));
            descricoes.push_back("Abre a porta trancada da sala do tesouro DESTE andar.  |  Não ocupa espaço na mochila.");
            continue;
        }
        opcoes.push_back(std::string(TIPOS_DE_ITEM[i].nome) + "   " + std::to_string(j.mochila[i]) + "/" +
                         std::to_string(limiteDoItem(j, i)));
        descricoes.push_back(descricaoDoItem(j, i));
    }
    std::string arma = (j.arma != nullptr) ? j.arma->nome : "nenhuma";
    std::string escudo = (j.escudo != nullptr) ? j.escudo->nome : "nenhum";
    std::string titulo = "MOCHILA " + std::to_string(itensNaMochila(j)) + "/" + std::to_string(limiteDaMochila(j)) +
                         "   (arma: " + arma + ", escudo: " + escudo + ")";
    int escolha = escolherEmCaixa(jogo, titulo, opcoes, descricoes, cursor);
    if (escolha >= 0) cursor = escolha;
    return escolha;
}

// v1.26: o Diário no console: uma página de missões e outra de anotações.
static void quebrarEmLinhas(const std::string& texto, const std::string& recuo, int largura,
                            std::vector<std::string>& linhas) {
    std::string linha = recuo, palavra;
    auto fechar = [&]() {
        if (palavra.empty()) return;
        if (contarLetras(linha) + contarLetras(palavra) + 1 > largura && linha != recuo) { linhas.push_back(linha); linha = recuo; }
        linha += (linha == recuo ? "" : " ") + palavra;
        palavra.clear();
    };
    for (char ch : texto) { if (ch == ' ') fechar(); else palavra += ch; }
    fechar();
    if (linha != recuo) linhas.push_back(linha);
}

void mostrarDiario(const Jogo& jogo) {
    // Cada grupo de linhas fica inteiro na mesma página (cabem 18 por página).
    std::vector<std::vector<std::string>> grupos;
    std::vector<std::string> g;
    g.push_back("MISSÃO PRINCIPAL: descer até o andar 13 e vencer o Senhor das Profundezas.");
    g.push_back("Você está no andar " + std::to_string(jogo.andar) + " de " + std::to_string(NUM_ANDARES) + ".");
    for (const EtapaPrincipal& e : etapasDaMissaoPrincipal(jogo)) {
        std::string nome = e.semChefe ? "(andar de uma versão antiga, sem chefe)" : e.nome.empty() ? "???" : e.nome;
        g.push_back(std::string("  ") + (e.vencido ? "[x] " : "[ ] ") + "Andar " + std::to_string(e.andar) + ": " + nome);
    }
    g.push_back("");
    grupos.push_back(g); g.clear();
    g.push_back("ESTE ANDAR (" + std::to_string(jogo.andar) + ")");
    std::vector<Missao> m = missoesDoAndar(jogo);
    if (m.empty()) g.push_back("  Explore o andar para descobrir o que fazer aqui.");
    for (const Missao& x : m)
        quebrarEmLinhas(std::string(x.feita ? "[x] " : "[ ] ") + x.titulo + ": " + x.detalhe +
                        (x.obrigatoria && !x.feita ? "  (obrigatória)" : ""), "  ", 74, g);
    grupos.push_back(g); g.clear();
    grupos.push_back({ QUEBRA_DE_PAGINA, "ANOTAÇÕES", "" });
    std::vector<Anotacao> notas = anotacoesDoDiario(jogo);
    if (notas.empty()) grupos.push_back({ "Ainda não há anotações. Converse com as pessoas que encontrar." });
    for (const Anotacao& a : notas) {
        g.push_back("Andar " + std::to_string(a.andar) + " - " + a.quem + (a.chefe ? " (vencido)" : ""));
        for (const std::string& ponto : a.pontos) quebrarEmLinhas("* " + ponto, "  ", 74, g);
        grupos.push_back(g); g.clear();
    }
    std::vector<std::string> l;
    int naPagina = 0;
    for (const std::vector<std::string>& grupo : grupos) {
        if (!grupo.empty() && grupo[0] == QUEBRA_DE_PAGINA) naPagina = -1;   // começa página nova
        else if (naPagina + (int)grupo.size() > 18) { l.push_back(QUEBRA_DE_PAGINA); naPagina = 0; }
        for (const std::string& x : grupo) l.push_back(x);
        naPagina += (int)grupo.size();
    }
    mostrarPagina("DIÁRIO", l);
}

// v1.26: as conquistas no console (uma página por grupo).
void mostrarConquistas(const Jogo* jogo) {
    (void)jogo;
    std::vector<std::string> l;
    l.push_back("Você tem " + std::to_string(conquistasGanhas()) + " de " + std::to_string((int)NUM_CONQUISTAS) + " conquistas.");
    for (int g = 0; g < NUM_GRUPOS_DE_CONQUISTA; g++) {
        if (g > 0) l.push_back(QUEBRA_DE_PAGINA);
        l.push_back("");
        l.push_back(NOMES_DOS_GRUPOS[g]);
        for (int id = 0; id < NUM_CONQUISTAS; id++) {
            if (CONQUISTAS[id].grupo != g) continue;
            l.push_back(std::string(conquistaGanha(id) ? "  [*] " : "  [ ] ") + CONQUISTAS[id].nome);
            l.push_back(std::string("        ") + CONQUISTAS[id].comoGanhar);
        }
    }
    mostrarPagina("CONQUISTAS", l);
}
