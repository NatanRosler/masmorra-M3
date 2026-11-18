// tela.cpp
#include "tela.h"
#include "jogo.h"
#include "terminal.h"
#include "batalha.h"
#include "itens.h"
#include "pontuacao.h"

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

static Aparencia aparenciaDaCelula(const Mapa& mapa, int x, int y) {
    switch (mapa.celulas[y][x]) {
        case PAREDE:
            if (ehVisivelComoParede(mapa, x, y)) {
                return { '#', { 170, 155, 140 }, { 105, 95, 85 }, false };
            }
            return { ' ', { 0, 0, 0 }, { 0, 0, 0 }, false }; // rocha maciça
        case CHAO:         return { '.', { 185, 175, 160 }, { 62, 56, 50 },  false };
        case PORTA:        return { '+', { 235, 185, 95 },  { 115, 72, 30 }, false };
        case ESCADA_DESCE: return { '>', { 255, 255, 255 }, { 110, 110, 120 }, false };
        case ESCADA_SOBE:  return { '<', { 255, 255, 255 }, { 110, 110, 120 }, false };
        case TOCHA:        return { '#', { 255, 215, 90 },  { 150, 80, 25 }, true };
        case PORTA_TRANCADA: return { '+', { 255, 215, 0 }, { 120, 30, 30 }, false };
        case CAIXA:        return { '=', { 225, 170, 95 },  { 95, 60, 30 },  false };
    }
    return { '?', { 255, 0, 0 }, { 0, 0, 0 }, true };
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
        if (!m.vivo || !jogo.visivel[m.y][m.x]) continue;
        int dx = m.x - jogo.jogador.x, dy = m.y - jogo.jogador.y;
        int d = dx * dx + dy * dy;
        if (d < melhorDistancia) {
            melhorDistancia = d;
            melhor = &m;
        }
    }
    return melhor;
}

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

    // Tabela rápida: que monstro está em cada célula (-1 = nenhum).
    static int monstroNaCelula[MAPA_ALTURA][MAPA_LARGURA];
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) monstroNaCelula[y][x] = -1;
    }
    for (int i = 0; i < andar.numMonstros; i++) {
        if (andar.monstros[i].vivo) monstroNaCelula[andar.monstros[i].y][andar.monstros[i].x] = i;
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
        for (int coluna = 0; coluna < CAMERA_LARGURA; coluna++) {
            // Converte a posição na tela para a posição no mapa.
            int x = cameraX + coluna;
            int y = cameraY + linha;

            char simbolo = ' ';
            Cor cor = { FUNDO_PRETO, FUNDO_PRETO };

            if (dentroDoMapa(x, y)) {
                Aparencia a = aparenciaDaCelula(mapa, x, y);
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
                    } else if (monstroNaCelula[y][x] >= 0) {
                        // Monstro: só aparece se a célula está visível (no
                        // escuro ou fora da visão, ele fica escondido). A cor
                        // mostra o elemento sorteado.
                        const Monstro& m = andar.monstros[monstroNaCelula[y][x]];
                        a.simbolo = tipoDe(m).simbolo;
                        frente = CORES_DOS_ELEMENTOS[m.elemento];
                        // Atordoado: pisca em amarelo.
                        if (m.atordoadoMs > 0 && (relogioMs() / 300) % 2 == 0) frente = { 255, 226, 90 };
                    } else if (npc >= 0) {
                        a.simbolo = FICHAS_DOS_NPCS[andar.npcs[npc].tipo].simbolo;
                        frente = FICHAS_DOS_NPCS[andar.npcs[npc].tipo].cor;
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
                    if (item >= 0 && andar.itens[item].visto) {
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
            p.letra(' ', cor);
        }
    }

    // Linha 23: status
    const Jogador& j = jogo.jogador;
    p.irPara(23, 1);
    std::string inicio = " Andar " + std::to_string(jogo.andar) + "  Nv " + std::to_string(j.nivel) + "  ";
    p.texto(inicio, COR_TEXTO);
    desenharBarra(p, "Vida", j.vida, j.vidaMaxima, 16, 124, 52);
    std::string resto = "  Vidas " + std::to_string(j.vidas) + "  Pts " + std::to_string(calcularPontuacao(jogo))
                      + "  " + textoDoTempo(jogo.tempoMs) + (jogo.descansando ? " zZz " : " ")
                      + (j.pontosLivres > 0 ? "[C] " : "");
    p.texto(resto, COR_TEXTO);
    int usadas = contarLetras(inicio) + 16 + contarLetras(resto);

    // Monstro visível mais próximo, como na barra lateral do Brogue.
    const Monstro* perto = monstroVisivelMaisProximo(jogo);
    int sobra = TELA_COLUNAS - 1 - usadas;
    if (perto != nullptr && sobra > 2) {
        const TipoMonstro& t = tipoDe(*perto);
        p.letra(t.simbolo, { rgbPara256(CORES_DOS_ELEMENTOS[perto->elemento]), FUNDO_PRETO });
        std::string info = std::string(" ") + t.nome + " Nv" + std::to_string(perto->nivel)
                         + " " + NOMES_DOS_ELEMENTOS[perto->elemento];
        if (perto->estado == DORMINDO) info += " (dormindo)";
        if (perto->estado == FUGINDO)  info += " (fugindo)";
        p.texto(ajustarLargura(info, sobra - 1), COR_TEXTO);
    } else {
        p.texto(std::string(sobra > 0 ? sobra : 0, ' '), COR_TEXTO);
    }

    // Linha 24: lembrete de comandos
    p.irPara(24, 1);
    p.texto(ajustarLargura(" Mover: setas/hjkl  i: mochila  C: ficha  Z: descansar  M: mensagens  ?: ajuda",
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
        "Mover          setas  ou  hjkl",
        "Diagonais      Home PgUp End PgDn  ou  y u b n",
        "Teclado num.   7 8 9 / 4 6 / 1 2 3",
        "Batalhar       ande na direção do monstro",
        "Descansar      z  ou  Z   (+1 de vida por segundo)",
        "               qualquer tecla para de descansar",
        "Mochila        i          Ficha (atributos)   C",
        "Mensagens      M          Sair   Q",
        "",
        "Na batalha: setas escolhem, Enter confirma, Esc volta.",
        "Pegue monstros dormindo: eles começam a luta dormindo.",
        "Escadas: ande para dentro de  >  (desce)  ou  <  (sobe)",
        "Itens:  ! poção  ) arma  ] escudo  * cristal  ? pergaminho  - chave",
        "Mapa:   = caixa (empurre num monstro: atordoa; pode ter item)",
        "        ^ armadilha   & pessoa (converse)",
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

void desenharCriacao(const Criacao& c) {
    std::vector<std::string> linhas;
    linhas.push_back("CRIE SEU PERSONAGEM");
    linhas.push_back("");
    std::vector<std::string> rotulos(NUM_LINHAS_DA_CRIACAO);
    rotulos[LINHA_NOME] = "Nome:         " + c.nome + (c.linha == LINHA_NOME ? "_" : "");
    rotulos[LINHA_ELEMENTO] = "Elemento:   < " + std::string(NOMES_DOS_ELEMENTOS[c.elemento]) + " >";
    rotulos[LINHA_COR] = "Cor:        < " + std::string(NOMES_DAS_CORES_DO_JOGADOR[c.cor]) + " >";
    rotulos[LINHA_DIFICULDADE] = "Dificuldade: < " + std::string(DIFICULDADES[c.dificuldade].nome) + " >";
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
        dica = "Golpe inicial: " + std::string(GOLPES[golpeInicialDoElemento(e)].nome);
    }
    if (c.linha == LINHA_COR) dica = "A cor do seu @ no mapa.";
    if (c.linha == LINHA_DIFICULDADE) dica = DIFICULDADES[c.dificuldade].descricao;
    if (c.linha >= LINHA_FORCA && c.linha <= LINHA_AGILIDADE) dica = EFEITOS_DOS_ATRIBUTOS[c.linha - LINHA_FORCA];
    if (c.linha == LINHA_ALEATORIO) dica = "Enter sorteia nome, elemento, cor e atributos.";
    if (c.linha == LINHA_COMECAR) dica = "Enter para começar.";
    linhas.push_back(dica);
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
    for (char& c : texto) {
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
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
        p.texto(b.travado ? "    " : "[ok]", APAGADO);
        dica = b.travado ? " Leia com atenção..." : " Aperte qualquer tecla para continuar";
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
    p.texto("  " + ajustarLargura("Setas: escolher   Enter: usar/equipar   Esc: voltar", LARGURA - 4) + "  ", DICA);
    p.saida += ansiResetar();
    escreverNaTela(p.saida);
}

