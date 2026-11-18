// itens.cpp
#include "itens.h"
#include "som.h"
#include "tela.h"
#include "jogo.h"
#include "aleatorio.h"
#include "npcs.h"
#include <cstdlib>

// =============================================================================
// FICHAS DOS ITENS
// =============================================================================
// Cada linha preenche só os efeitos da sua categoria; o resto fica 0 / -1.
//   cura%  status PP  | atq def vel vida | elem golpe | andares freq
const TipoItem TIPOS_DE_ITEM[NUM_TIPOS_DE_ITEM] = {
    // ---- Poções -----------------------------------------------------------------
    { "Poção de Vida", "uma", '!', { 235, 70, 90 }, POCAO,
      50, false, false,   0, 0, 0, 0,   -1, -1,   1, 10, 30,
      "Cura metade da vida máxima." },
    { "Poção Grande", "uma", '!', { 255, 120, 170 }, POCAO,
      100, false, false,  0, 0, 0, 0,   -1, -1,   3, 10, 5,
      "Cura toda a vida." },
    { "Antídoto", "um", '!', { 120, 225, 120 }, POCAO,
      0, true, false,     0, 0, 0, 0,   -1, -1,   1, 10, 12,
      "Tira veneno, sono e paralisia (na batalha)." },
    { "Éter", "um", '!', { 110, 170, 255 }, POCAO,
      0, false, true,     0, 0, 0, 0,   -1, -1,   2, 10, 10,
      "Recupera os PP de todos os golpes." },

    // ---- Armas ------------------------------------------------------------------
    { "Adaga", "uma", ')', { 200, 200, 215 }, ARMA,
      0, false, false,    1, 0, 0, 0,   -1, -1,   1, 10, 0,
      "Ataque +1." },
    { "Espada Curta", "uma", ')', { 210, 210, 225 }, ARMA,
      0, false, false,    3, 0, 0, 0,   -1, -1,   1, 4, 5,
      "Ataque +3." },
    { "Machado", "um", ')', { 220, 220, 230 }, ARMA,
      0, false, false,    4, 0, 0, 0,   -1, -1,   3, 7, 4,
      "Ataque +4." },
    { "Espada Longa", "uma", ')', { 235, 235, 245 }, ARMA,
      0, false, false,    3, 0, 0, 0,   -1, -1,   6, 10, 3,
      "Ataque +6." },
    { "Espada de Fogo", "uma", ')', { 255, 130, 40 }, ARMA,
      0, false, false,    3, 0, 0, 0,   FOGO, -1,   2, 10, 2,
      "Ataque +3. Golpes de Fogo causam +25%." },
    { "Tridente", "um", ')', { 60, 160, 255 }, ARMA,
      0, false, false,    3, 0, 0, 0,   AGUA, -1,   2, 10, 2,
      "Ataque +3. Golpes de Água causam +25%." },
    { "Chicote de Espinhos", "um", ')', { 90, 200, 90 }, ARMA,
      0, false, false,    3, 0, 0, 0,   PLANTA, -1, 2, 10, 2,
      "Ataque +3. Golpes de Planta causam +25%." },
    { "Adaga Venenosa", "uma", ')', { 175, 115, 225 }, ARMA,
      0, false, false,    3, 0, 0, 0,   VENENO, -1, 2, 10, 2,
      "Ataque +3. Golpes de Veneno causam +25%." },
    { "Martelo de Pedra", "um", ')', { 190, 150, 90 }, ARMA,
      0, false, false,    3, 0, 0, 0,   PEDRA, -1,  2, 10, 2,
      "Ataque +3. Golpes de Pedra causam +25%." },

    // ---- Escudos ----------------------------------------------------------------
    { "Escudo de Madeira", "um", ']', { 175, 130, 80 }, ESCUDO,
      0, false, false,    0, 2, 0, 0,   -1, -1,   1, 4, 5,
      "Defesa +2." },
    { "Escudo de Ferro", "um", ']', { 190, 190, 205 }, ESCUDO,
      0, false, false,    0, 4, 0, 0,   -1, -1,   3, 8, 4,
      "Defesa +4." },
    { "Escudo de Torre", "um", ']', { 150, 150, 175 }, ESCUDO,
      0, false, false,    0, 6, -3, 0,  -1, -1,   6, 10, 2,
      "Defesa +6, mas Velocidade -3 (é pesado)." },

    // ---- Cristais (power-ups: efeito permanente ao pegar) -----------------------
    { "Cristal de Vida", "um", '*', { 255, 100, 140 }, CRISTAL,
      0, false, false,    0, 0, 0, 4,   -1, -1,   1, 10, 3,
      "Vida máxima +4 para sempre." },
    { "Cristal de Força", "um", '*', { 255, 130, 60 }, CRISTAL,
      0, false, false,    2, 0, 0, 0,   -1, -1,   1, 10, 3,
      "Ataque +2 para sempre." },
    { "Cristal de Ferro", "um", '*', { 165, 185, 210 }, CRISTAL,
      0, false, false,    0, 2, 0, 0,   -1, -1,   1, 10, 3,
      "Defesa +2 para sempre." },
    { "Pena do Vento", "uma", '*', { 180, 255, 230 }, CRISTAL,
      0, false, false,    0, 0, 2, 0,   -1, -1,   1, 10, 3,
      "Velocidade +2 para sempre." },

    // ---- Pergaminhos (ensinam um golpe) -----------------------------------------
    { "Pergaminho de Fogo", "um", '?', { 255, 170, 110 }, PERGAMINHO,
      0, false, false,    0, 0, 0, 0,   -1, G_TOCHA,           2, 10, 3,
      "Ensina o golpe Tocha (Fogo)." },
    { "Pergaminho de Água", "um", '?', { 140, 190, 255 }, PERGAMINHO,
      0, false, false,    0, 0, 0, 0,   -1, G_JATO_DAGUA,      2, 10, 3,
      "Ensina o golpe Jato d'Água (Água)." },
    { "Pergaminho de Planta", "um", '?', { 150, 220, 140 }, PERGAMINHO,
      0, false, false,    0, 0, 0, 0,   -1, G_PO_DO_SONO,      2, 10, 3,
      "Ensina o golpe Pó do Sono (Planta)." },
    { "Pergaminho de Veneno", "um", '?', { 195, 160, 230 }, PERGAMINHO,
      0, false, false,    0, 0, 0, 0,   -1, G_LAMINA_VENENOSA, 2, 10, 3,
      "Ensina o golpe Lâmina Venenosa (Veneno)." },
    { "Pergaminho de Pedra", "um", '?', { 210, 180, 130 }, PERGAMINHO,
      0, false, false,    0, 0, 0, 0,   -1, G_PEDRADA,         2, 10, 3,
      "Ensina o golpe Pedrada (Pedra)." },

    // ---- Chave ------------------------------------------------------------------
    { "Chave", "uma", '-', { 255, 215, 0 }, CHAVE,
      0, false, false,    0, 0, 0, 0,   -1, -1,   1, 10, 0,
      "Abre a porta trancada da sala do tesouro." },
};

std::string nomeComArtigo(int tipo) {
    return std::string(TIPOS_DE_ITEM[tipo].artigo) + " " + TIPOS_DE_ITEM[tipo].nome;
}

int itemEm(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numItens; i++) {
        const ItemNoChao& item = andar.itens[i];
        if (item.ativo && item.x == x && item.y == y) return i;
    }
    return -1;
}

// =============================================================================
// ESPALHAR ITENS PELO ANDAR
// =============================================================================
// Sorteio com pesos entre os itens que aparecem neste andar.
static int sortearTipoDeItem(GeradorAleatorio& g, int numeroDoAndar) {
    int total = 0;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        const TipoItem& t = TIPOS_DE_ITEM[i];
        if (numeroDoAndar >= t.andarMinimo && numeroDoAndar <= t.andarMaximo) total += t.frequencia;
    }
    int r = sortear(g, 1, total);
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        const TipoItem& t = TIPOS_DE_ITEM[i];
        if (numeroDoAndar < t.andarMinimo || numeroDoAndar > t.andarMaximo) continue;
        r -= t.frequencia;
        if (r <= 0) return i;
    }
    return I_POCAO_DE_VIDA;
}

// Uma arma ou um escudo bom para o tesouro: do andar atual até 2 andares à frente.
static int sortearEquipamento(GeradorAleatorio& g, int numeroDoAndar) {
    int candidatos[NUM_TIPOS_DE_ITEM];
    int qtd = 0;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        const TipoItem& t = TIPOS_DE_ITEM[i];
        bool ehEquipamento = (t.categoria == ARMA || t.categoria == ESCUDO);
        if (ehEquipamento && t.frequencia > 0 &&
            t.andarMinimo <= numeroDoAndar + 2 && t.andarMaximo >= numeroDoAndar) {
            candidatos[qtd++] = i;
        }
    }
    return candidatos[sortear(g, 0, qtd - 1)];
}

static void colocarItem(Andar& andar, int tipo, int x, int y) {
    if (andar.numItens >= MAX_ITENS_NO_CHAO) return;
    ItemNoChao& item = andar.itens[andar.numItens++];
    item.ativo = true;
    item.tipo = tipo;
    item.x = x;
    item.y = y;
    item.visto = false;
}

// Sorteia uma célula de chão livre cuja distância (no mapa de distâncias dado)
// esteja entre "minima" e infinito. Devolve false se não achar.
static bool sortearLugar(const Andar& andar, GeradorAleatorio& g,
                         const int distancia[MAPA_ALTURA][MAPA_LARGURA], int minima,
                         int& x, int& y) {
    for (int tentativa = 0; tentativa < 500; tentativa++) {
        x = sortear(g, 1, MAPA_LARGURA - 2);
        y = sortear(g, 1, MAPA_ALTURA - 2);
        if (andar.mapa.celulas[y][x] != CHAO || andar.zonaSegura[y][x]) continue;
        if (distancia[y][x] == INALCANCAVEL || distancia[y][x] < minima) continue;
        if (itemEm(andar, x, y) >= 0 || monstroEm(andar, x, y) >= 0) continue;
        return true;
    }
    return false;
}

void espalharItens(Andar& andar, int numeroDoAndar, uint32_t semente) {
    GeradorAleatorio g;
    iniciarGerador(g, semente);
    andar.numItens = 0;

    // Distâncias a partir da chegada. A porta trancada bloqueia o caminho,
    // então nada daqui cai dentro da sala do tesouro.
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, andar.escadas.chegadaSobeX, andar.escadas.chegadaSobeY, distancia);

    // 1. Itens comuns pelo andar.
    int quantos = sortear(g, 2, 4);
    for (int i = 0; i < quantos; i++) {
        int tipo = sortearTipoDeItem(g, numeroDoAndar);
        int x, y;
        if (sortearLugar(andar, g, distancia, 4, x, y)) colocarItem(andar, tipo, x, y);
    }

    // 2. Sala do tesouro: um equipamento bom, um cristal e às vezes uma Poção Grande.
    if (!andar.tesouro.existe) return;
    static int dentro[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, andar.tesouro.dentroX, andar.tesouro.dentroY, dentro);

    int premios[3];
    int numPremios = 0;
    premios[numPremios++] = sortearEquipamento(g, numeroDoAndar);
    premios[numPremios++] = I_CRISTAL_DE_VIDA + sortear(g, 0, 3);
    if (chance(g, 35)) premios[numPremios++] = I_POCAO_GRANDE;
    for (int i = 0; i < numPremios; i++) {
        int x, y;
        if (sortearLugar(andar, g, dentro, 0, x, y)) colocarItem(andar, premios[i], x, y);
    }

    // 3. A chave: metade das vezes fica com um monstro (que a deixa ao ser
    //    derrotado); senão, fica no chão, longe da chegada. No andar do
    //    Borin não há chave: ele mesmo destranca a porta.
    if (numeroDoAndar == FICHAS_DOS_NPCS[NPC_MINEIRO].andar) return;
    if (andar.numMonstros > 0 && chance(g, 50)) {
        andar.monstros[sortear(g, 0, andar.numMonstros - 1)].carregaChave = true;
        return;
    }
    int x, y;
    if (sortearLugar(andar, g, distancia, 8, x, y) || sortearLugar(andar, g, distancia, 0, x, y)) {
        colocarItem(andar, I_CHAVE, x, y);
    }
}

// =============================================================================
// PEGAR, EQUIPAR E USAR
// =============================================================================
static std::string textoDosBonus(const TipoItem& t) {
    std::string s;
    if (t.bonusVida)       s += " Vida máxima +" + std::to_string(t.bonusVida) + ".";
    if (t.bonusAtaque)     s += " Ataque +" + std::to_string(t.bonusAtaque) + ".";
    if (t.bonusDefesa)     s += " Defesa +" + std::to_string(t.bonusDefesa) + ".";
    if (t.bonusVelocidade) s += " Velocidade +" + std::to_string(t.bonusVelocidade) + ".";
    return s;
}

// =============================================================================
// LIMITES DA MOCHILA
// =============================================================================
// "uma poção" -> "Uma poção" (os artigos começam com letra sem acento).
static std::string comMaiusculaInicial(std::string texto) {
    if (!texto.empty() && texto[0] >= 'a' && texto[0] <= 'z') texto[0] = (char)(texto[0] - 'a' + 'A');
    return texto;
}

int limiteDoItem(int tipo) {
    switch (tipo) {
        case I_POCAO_DE_VIDA: return 6;
        case I_POCAO_GRANDE:
        case I_ANTIDOTO:
        case I_ETER:          return 3;
        case I_CHAVE:         return 99;   // a chave não ocupa espaço
    }
    if (TIPOS_DE_ITEM[tipo].categoria == CRISTAL) return 99;   // nunca fica na mochila
    return 1;   // armas, escudos e pergaminhos de reserva: um de cada
}

int itensNaMochila(const Jogador& j) {
    int total = 0;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        if (i != I_CHAVE) total += j.mochila[i];
    }
    return total;
}

bool cabeNaMochila(const Jogador& j, int tipo) {
    if (tipo == I_CHAVE || TIPOS_DE_ITEM[tipo].categoria == CRISTAL) return true;
    return j.mochila[tipo] < limiteDoItem(tipo) && itensNaMochila(j) < LIMITE_DA_MOCHILA;
}

std::string motivoDeNaoCaber(const Jogador& j, int tipo) {
    if (j.mochila[tipo] >= limiteDoItem(tipo)) {
        return "Você já carrega o máximo de " + std::string(TIPOS_DE_ITEM[tipo].nome) + " (" +
               std::to_string(j.mochila[tipo]) + "/" + std::to_string(limiteDoItem(tipo)) + ").";
    }
    return "A mochila está cheia (" + std::to_string(itensNaMochila(j)) + "/" +
           std::to_string(LIMITE_DA_MOCHILA) + ").";
}

// Deixa um item no chão em (x, y) ou, se já houver outro ali, numa célula vizinha.
static void largarNoChao(Jogo& jogo, int tipo, int x, int y) {
    Andar& andar = andarAtual(jogo);
    for (int d = -1; d < 8; d++) {
        int nx = (d < 0) ? x : x + DIRECOES_DX[d], ny = (d < 0) ? y : y + DIRECOES_DY[d];
        if (!dentroDoMapa(nx, ny) || andar.mapa.celulas[ny][nx] != CHAO || itemEm(andar, nx, ny) >= 0) continue;
        // Reaproveita uma vaga de item já pego; senão, usa uma nova.
        int vaga = -1;
        for (int i = 0; i < andar.numItens && vaga < 0; i++) if (!andar.itens[i].ativo) vaga = i;
        if (vaga < 0) {
            if (andar.numItens >= MAX_ITENS_NO_CHAO) return;
            vaga = andar.numItens++;
        }
        ItemNoChao& item = andar.itens[vaga];
        item.ativo = true;
        item.tipo = tipo;
        item.x = nx;
        item.y = ny;
        item.visto = true;
        return;
    }
}

void guardarOuLargar(Jogo& jogo, int tipo) {
    Jogador& j = jogo.jogador;
    if (cabeNaMochila(j, tipo)) {
        j.mochila[tipo]++;
        return;
    }
    largarNoChao(jogo, tipo, j.x, j.y);
    adicionarMensagem(jogo, motivoDeNaoCaber(j, tipo) + " " + comMaiusculaInicial(nomeComArtigo(tipo)) +
                            " ficou no chão.");
}

void equipar(Jogo& jogo, int tipo) {
    Jogador& j = jogo.jogador;
    const TipoItem* novo = &TIPOS_DE_ITEM[tipo];

    // "lugar" é um ponteiro PARA o ponteiro da mão certa (arma ou escudo):
    // assim o mesmo código serve para os dois.
    const TipoItem** lugar = (novo->categoria == ARMA) ? &j.arma : &j.escudo;
    if (*lugar == novo) {
        adicionarMensagem(jogo, "Você já está usando " + nomeComArtigo(tipo) + ".");
        return;
    }
    // O item que estava na mão volta para a mochila (ou para o chão, se não
    // couber). A subtração de ponteiros dá a posição dele na tabela (o seu IdItem).
    j.mochila[tipo]--;
    if (*lugar != nullptr) guardarOuLargar(jogo, (int)(*lugar - TIPOS_DE_ITEM));
    *lugar = novo;

    int vidaMaximaAntes = j.vidaMaxima;
    recalcularAtributos(j);
    j.vida += j.vidaMaxima - vidaMaximaAntes;
    adicionarMensagem(jogo, "Você equipou " + nomeComArtigo(tipo) + ".");
}

void absorverCristal(Jogo& jogo, int tipo) {
    Jogador& j = jogo.jogador;
    const TipoItem& t = TIPOS_DE_ITEM[tipo];
    j.bonusVida += t.bonusVida;
    j.bonusAtaque += t.bonusAtaque;
    j.bonusDefesa += t.bonusDefesa;
    j.bonusVelocidade += t.bonusVelocidade;
    int vidaMaximaAntes = j.vidaMaxima;
    recalcularAtributos(j);
    j.vida += j.vidaMaxima - vidaMaximaAntes;
    adicionarMensagem(jogo, "Você absorve " + nomeComArtigo(tipo) + "!" + textoDosBonus(t), MSG_BOA);
}

void pegarItensDoChao(Jogo& jogo) {
    Andar& andar = andarAtual(jogo);
    Jogador& j = jogo.jogador;
    int indice = itemEm(andar, j.x, j.y);
    if (indice < 0) return;

    ItemNoChao& item = andar.itens[indice];
    const TipoItem& t = TIPOS_DE_ITEM[item.tipo];

    // Mochila: cada item tem um máximo, e a mochila toda também.
    if (!cabeNaMochila(j, item.tipo)) {
        bool cheiaDeTudo = j.mochila[item.tipo] < limiteDoItem(item.tipo);
        if (!cheiaDeTudo || jogo.modoAutomatico) {
            adicionarMensagem(jogo, motivoDeNaoCaber(j, item.tipo) + " " +
                                    comMaiusculaInicial(nomeComArtigo(item.tipo)) + " fica no chão.");
            return;
        }
        // Mochila cheia: o jogador pode largar outra coisa aqui para pegar este item.
        std::vector<int> tipos;
        std::vector<std::string> opcoes, descricoes;
        for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
            if (i == I_CHAVE || j.mochila[i] <= 0) continue;
            tipos.push_back(i);
            opcoes.push_back("Largar " + std::string(TIPOS_DE_ITEM[i].nome) + "  (tenho " + std::to_string(j.mochila[i]) + ")");
            descricoes.push_back(std::string(TIPOS_DE_ITEM[i].descricao) + "  |  Fica no chão no lugar de " +
                                 nomeComArtigo(item.tipo) + ".");
        }
        opcoes.push_back("Deixar " + std::string(t.nome) + " no chão");
        descricoes.push_back(std::string(t.descricao) + "  |  Você pode voltar e pegar depois.");
        int escolha = escolherEmCaixa(jogo, "MOCHILA CHEIA (" + std::to_string(itensNaMochila(j)) + "/" +
                                      std::to_string(LIMITE_DA_MOCHILA) + "): pegar " + t.nome + "?",
                                      opcoes, descricoes, (int)opcoes.size() - 1);
        if (escolha < 0 || escolha >= (int)tipos.size()) {
            adicionarMensagem(jogo, comMaiusculaInicial(nomeComArtigo(item.tipo)) + " fica no chão.");
            return;
        }
        // Troca: o item escolhido vai para o chão, no mesmo lugar.
        int largado = tipos[escolha];
        int pegou = item.tipo;
        j.mochila[largado]--;
        j.mochila[pegou]++;
        item.tipo = largado;
        item.visto = true;
        jogo.itensColetados++;
        tocarSom(SOM_ITEM);
        efeitoNoMapa(jogo, EFEITO_ITEM, item.x, item.y, pegou);
        adicionarMensagem(jogo, "Você largou " + nomeComArtigo(largado) + " e pegou " + nomeComArtigo(pegou) + ".");
        const TipoItem& novo = TIPOS_DE_ITEM[pegou];
        if (novo.categoria == ARMA && (j.arma == nullptr || novo.bonusAtaque > j.arma->bonusAtaque)) equipar(jogo, pegou);
        if (novo.categoria == ESCUDO && j.escudo == nullptr) equipar(jogo, pegou);
        return;
    }
    item.ativo = false;
    jogo.itensColetados++;
    tocarSom(SOM_ITEM);
    efeitoNoMapa(jogo, EFEITO_ITEM, item.x, item.y, item.tipo);

    // Cristais fazem efeito na hora e para sempre.
    if (t.categoria == CRISTAL) {
        absorverCristal(jogo, item.tipo);
        return;
    }

    j.mochila[item.tipo]++;
    adicionarMensagem(jogo, "Você pegou " + nomeComArtigo(item.tipo) + ".");

    // Equipa sozinho se for melhor que o que está na mão (ou se a mão está vazia).
    if (t.categoria == ARMA && (j.arma == nullptr || t.bonusAtaque > j.arma->bonusAtaque)) {
        equipar(jogo, item.tipo);
    }
    if (t.categoria == ESCUDO && j.escudo == nullptr) {
        equipar(jogo, item.tipo);
    }
    if (t.categoria == CHAVE) revelarPortaDoTesouro(jogo);
}

// Direção aproximada de (dx, dy) em palavras ("ao norte", "a sudeste"...).
static std::string direcaoEmPalavras(int dx, int dy) {
    // No mapa, y cresce para baixo (sul). Um eixo só conta se for relevante.
    bool norte = dy < 0 && -dy * 2 >= std::abs(dx), sul = dy > 0 && dy * 2 >= std::abs(dx);
    bool leste = dx > 0 && dx * 2 >= std::abs(dy), oeste = dx < 0 && -dx * 2 >= std::abs(dy);
    if (norte && leste) return "a nordeste";
    if (norte && oeste) return "a noroeste";
    if (sul && leste) return "a sudeste";
    if (sul && oeste) return "a sudoeste";
    if (norte) return "ao norte";
    if (sul) return "ao sul";
    if (leste) return "a leste";
    return "a oeste";
}

void revelarPortaDoTesouro(Jogo& jogo) {
    Andar& andar = andarAtual(jogo);
    const SalaDoTesouro& t = andar.tesouro;
    if (!t.existe || andar.mapa.celulas[t.portaY][t.portaX] != PORTA_TRANCADA) {
        adicionarMensagem(jogo, "A chave abre a porta trancada deste andar!", MSG_BOA);
        return;
    }
    // A porta (e as paredes em volta) passam a aparecer no mapa.
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dentroDoMapa(t.portaX + dx, t.portaY + dy)) andar.descoberto[t.portaY + dy][t.portaX + dx] = true;
    adicionarMensagem(jogo, "A chave brilha... a porta do tesouro fica " +
                            direcaoEmPalavras(t.portaX - jogo.jogador.x, t.portaY - jogo.jogador.y) +
                            "! (marcada no mapa)", MSG_BOA);
}

// Pergaminho: ensina um golpe (se já souber 4, pergunta qual esquecer).
static bool ensinarGolpe(Jogo& jogo, int golpe) {
    Jogador& j = jogo.jogador;
    std::string nomeGolpe = GOLPES[golpe].nome;
    for (int s = 0; s < j.numGolpes; s++) {
        if (j.golpes[s] == golpe) {
            adicionarMensagem(jogo, "Você já sabe " + nomeGolpe + ".");
            return false;
        }
    }
    int lugar = j.numGolpes;
    if (j.numGolpes >= MAX_GOLPES) {
        std::vector<std::string> opcoes;
        for (int s = 0; s < j.numGolpes; s++) opcoes.push_back(GOLPES[j.golpes[s]].nome);
        opcoes.push_back("Não aprender");
        lugar = escolherEmCaixa(jogo, "Aprender " + nomeGolpe + ": esquecer qual golpe?",
                                opcoes, std::vector<std::string>());
        if (lugar < 0 || lugar >= j.numGolpes) {
            adicionarMensagem(jogo, "Você guardou o pergaminho.");
            return false;
        }
        adicionarMensagem(jogo, "Você esqueceu " + std::string(GOLPES[j.golpes[lugar]].nome) + ".");
    } else {
        j.numGolpes++;
    }
    j.golpes[lugar] = golpe;
    j.pp[lugar] = GOLPES[golpe].ppMaximo;
    adicionarMensagem(jogo, "Você aprendeu " + nomeGolpe + "!", MSG_BOA);
    return true;
}

bool usarItemForaDaBatalha(Jogo& jogo, int tipo) {
    Jogador& j = jogo.jogador;
    const TipoItem& t = TIPOS_DE_ITEM[tipo];
    if (j.mochila[tipo] <= 0) return false;

    switch (t.categoria) {
        case ARMA:
        case ESCUDO:
            equipar(jogo, tipo);
            return true;

        case PERGAMINHO:
            if (!ensinarGolpe(jogo, t.golpeEnsinado)) return false;
            j.mochila[tipo]--;
            return true;

        case CHAVE:
            adicionarMensagem(jogo, "Ande até uma porta trancada para usar a chave.");
            return false;

        case CRISTAL:
            return false; // cristais nunca ficam na mochila

        case POCAO:
            if (t.curaPorcento > 0) {
                if (j.vida >= j.vidaMaxima) {
                    adicionarMensagem(jogo, "Sua vida já está cheia.");
                    return false;
                }
                int antes = j.vida;
                tocarSom(SOM_CURA);
                j.vida += j.vidaMaxima * t.curaPorcento / 100;
                if (j.vida > j.vidaMaxima) j.vida = j.vidaMaxima;
                adicionarMensagem(jogo, "Você bebe " + nomeComArtigo(tipo) + " e recupera "
                                        + std::to_string(j.vida - antes) + " de vida.");
            } else if (t.restauraPP) {
                for (int s = 0; s < j.numGolpes; s++) j.pp[s] = GOLPES[j.golpes[s]].ppMaximo;
                adicionarMensagem(jogo, "Você bebe " + nomeComArtigo(tipo) + ". Seus golpes recuperaram os PP!");
            } else {
                adicionarMensagem(jogo, "Fora da batalha você não está sob nenhum efeito.");
                return false;
            }
            j.mochila[tipo]--;
            return true;
    }
    return false;
}
