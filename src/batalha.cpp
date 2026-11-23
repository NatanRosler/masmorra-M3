// batalha.cpp
#include "batalha.h"
#include "sala_do_chefe.h"
#include <algorithm>
#include "jogo.h"
#include "monstros.h"
#include "tela.h"
#include "terminal.h"
#include "aleatorio.h"
#include "itens.h"
#include "som.h"
#include "dificuldade.h"
#include "loja.h"
#include "dicas.h"
#include "opcoes.h"
#include "tema.h"
#include "bestiario.h"

// =============================================================================
// TABELAS
// =============================================================================
const char* NOMES_DOS_ELEMENTOS[NUM_ELEMENTOS] = {
    "Normal", "Fogo", "Água", "Planta", "Veneno", "Pedra", "Gelo", "Raio", "Trevas", "Luz"
};

const char* NOMES_DOS_STATUS[] = { "", "envenenado", "dormindo", "paralisado",
                                   "queimado", "congelado", "ofuscado", "atordoado", "preso" };

// Eficácia em METADES (1 = metade do dano, 2 = normal, 4 = dobro).
// Linha = elemento do golpe; coluna = elemento de quem recebe.
static const int TABELA_DE_EFICACIA[NUM_ELEMENTOS][NUM_ELEMENTOS] = {
    //              Normal Fogo Água Planta Veneno Pedra  Gelo Raio Trevas Luz
    /* Normal */  {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2 },
    /* Fogo   */  {   2,   1,   1,   4,   2,   1,   4,   2,   4,   2 },
    /* Água   */  {   2,   4,   1,   1,   2,   4,   1,   2,   2,   2 },
    /* Planta */  {   2,   1,   4,   1,   1,   4,   2,   2,   2,   2 },
    /* Veneno */  {   2,   2,   2,   2,   1,   1,   2,   2,   2,   4 },
    /* Pedra  */  {   2,   4,   2,   1,   4,   2,   2,   4,   2,   2 },
    /* Gelo   */  {   2,   1,   4,   2,   2,   4,   1,   2,   2,   2 },
    /* Raio   */  {   2,   2,   4,   1,   2,   2,   4,   1,   2,   2 },
    /* Trevas */  {   4,   2,   2,   2,   2,   2,   2,   2,   1,   4 },
    /* Luz    */  {   2,   2,   2,   2,   2,   2,   2,   2,   4,   1 },
};

int eficacia(Elemento ataque, Elemento defesa) {
    return TABELA_DE_EFICACIA[ataque][defesa];
}

const Golpe GOLPES[NUM_GOLPES] = {
    //  nome               elemento poder prec  PP  efeito        chance prior.
    { "Golpe",             NORMAL,  40,  100,   0, SEM_EFEITO,      0,  0, false },
    { "Escudo",            NORMAL,   0,  100,  10, SOBE_DEFESA,   100,  2, false },
    { "Golpe Forte",       NORMAL,  65,   90,  10, SEM_EFEITO,      0,  0, false },
    { "Tocha",             FOGO,    55,  100,  12, QUEIMAR,        10,  0, false },
    { "Jato d'Água",       AGUA,    60,  100,  12, SEM_EFEITO,      0,  0, false },
    { "Grito de Guerra",   NORMAL,   0,  100,  10, SOBE_ATAQUE,   100,  0, false },
    { "Pó do Sono",        PLANTA,   0,   75,   8, ADORMECER,     100,  0, false },
    { "Lâmina Venenosa",   VENENO,  55,  100,  12, ENVENENAR,      30,  0, false },
    { "Golpe Duplo",       NORMAL,  25,   90,  10, DOIS_GOLPES,   100,  0, false },
    { "Pedrada",           PEDRA,   55,   95,  12, ATORDOAR,       10,  0, false },
    // Golpes dos monstros (eles não gastam PP)
    { "Mordida",           NORMAL,  45,  100,   0, SEM_EFEITO,      0,  0, false },
    { "Rosnado",           NORMAL,   0,  100,   0, BAIXA_ATAQUE,  100,  0, false },
    { "Ataque Rápido",     NORMAL,  40,  100,   0, SEM_EFEITO,      0,  1, false },
    { "Arranhão",          NORMAL,  40,  100,   0, SEM_EFEITO,      0,  0, false },
    { "Folha Navalha",     PLANTA,  70,   95,  10, SEM_EFEITO,      0,  0, false },
    { "Língua Grudenta",   NORMAL,  30,  100,   0, PARALISAR,      30,  0, false },
    { "Ferrão Venenoso",   VENENO,  25,  100,   0, ENVENENAR,      40,  0, false },
    { "Sugar Sangue",      NORMAL,  45,  100,   0, DRENAR,        100,  0, false },
    { "Esmagar",           PEDRA,   70,   80,   0, ATORDOAR,       10,  0, false },
    { "Rugido",            NORMAL,   0,  100,   0, BAIXA_DEFESA,  100,  0, false },
    { "Chicote d'Água",    AGUA,    65,   95,   0, SEM_EFEITO,      0,  0, false },
    { "Hipnose",           NORMAL,   0,   60,   0, ADORMECER,     100,  0, false },
    { "Brasa",             FOGO,    45,  100,   0, QUEIMAR,        10,  0, false },
    // Golpe inicial de quem escolhe Planta
    { "Chicote de Vinha",  PLANTA,  60,  100,  12, PRENDER,        10,  0, false },
    // Golpes elementais dos monstros (ver golpeElementalDosMonstros)
    { "Investida",         NORMAL,  80,   90,  10, SEM_EFEITO,      0,  0, false },
    { "Labareda",          FOGO,    70,   90,  10, QUEIMAR,        20,  0, false },
    { "Bolha",             AGUA,    45,  100,   0, SEM_EFEITO,      0,  0, false },
    { "Espinhos",          PLANTA,  45,  100,   0, SEM_EFEITO,      0,  0, false },
    { "Ácido",             VENENO,  65,   90,   0, ENVENENAR,      20,  0, false },
    { "Pedregulho",        PEDRA,   45,   95,   0, SEM_EFEITO,      0,  0, false },
    // Golpe do chefe final
    { "Golpe Sombrio",     TREVAS,  80,   90,   0, SEM_EFEITO,      0,  0, false },
    // ---- v1.23 -------------------------------------------------------------
    //  nome                 elemento poder prec  PP  efeito     chance prior. área
    { "Foco",              NORMAL,   0,  100,   5, FOCO,          100,  0, false },
    { "Fúria",             NORMAL,  25,   90,   8, TRES_GOLPES,   100,  0, false },
    { "Golpe Final",       NORMAL, 100,   85,   5, SEM_EFEITO,      0,  0, false },
    { "Muralha de Fogo",   FOGO,    55,   95,   8, QUEIMAR,        10,  0, true },
    { "Inferno",           FOGO,    95,   80,   5, QUEIMAR,        30,  0, false },
    { "Bolha Curativa",    AGUA,     0,  100,   6, CURAR,          30,  0, false },
    { "Onda",              AGUA,    60,   95,   8, SEM_EFEITO,      0,  0, true },
    { "Maremoto",          AGUA,   100,   85,   5, SEM_EFEITO,      0,  0, false },
    { "Semente Sanguessuga", PLANTA, 0,   90,   8, SANGUESSUGA,   100,  0, false },
    { "Tempestade de Folhas", PLANTA, 70, 90,   6, SEM_EFEITO,      0,  0, true },
    { "Nuvem Tóxica",      VENENO,  40,   90,   8, ENVENENAR,      30,  0, true },
    { "Ácido Corrosivo",   VENENO,  65,   95,  10, BAIXA_DEFESA,   30,  0, false },
    { "Peste",             VENENO,  90,   85,   5, ENVENENAR,      50,  0, false },
    { "Muralha de Pedra",  PEDRA,    0,  100,   8, SOBE_DEFESA,   100,  0, false },
    { "Terremoto",         PEDRA,   70,   90,   6, SEM_EFEITO,      0,  0, true },
    { "Avalanche",         PEDRA,   95,   85,   5, ATORDOAR,       20,  0, false },
    { "Rajada de Gelo",    GELO,    60,  100,  12, CONGELAR,       10,  0, false },
    { "Armadura de Gelo",  GELO,     0,  100,   8, SOBE_DEFESA,   100,  0, false },
    { "Nevasca",           GELO,    60,   90,   8, CONGELAR,       10,  0, true },
    { "Zero Absoluto",     GELO,    95,   80,   5, CONGELAR,       20,  0, false },
    { "Choque",            RAIO,    55,  100,  12, PARALISAR,      10,  0, false },
    { "Passo do Raio",     RAIO,    45,  100,  12, SEM_EFEITO,      0,  1, false },
    { "Trovão",            RAIO,    80,   85,   8, PARALISAR,      20,  0, false },
    { "Tempestade",        RAIO,    70,   85,   5, PARALISAR,      10,  0, true },
    { "Garra das Trevas",  TREVAS,  50,  100,  12, DRENAR,        100,  0, false },
    { "Pavor",             TREVAS,   0,   90,   8, BAIXA_ATAQUE,  100,  0, false },
    { "Pulso das Trevas",  TREVAS,  65,   95,   8, DRENAR,        100,  0, false },
    { "Eclipse",           TREVAS,  95,   85,   5, BAIXA_DEFESA,   20,  0, false },
    { "Raio de Luz",       LUZ,     50,  100,  12, OFUSCAR,         5,  0, false },
    { "Cura de Luz",       LUZ,      0,  100,   3, CURAR,          25,  0, false },
    { "Clarão",            LUZ,     55,   95,   8, OFUSCAR,        10,  0, true },
    { "Julgamento",        LUZ,    100,   85,   5, SEM_EFEITO,      0,  0, false },
    // Golpes dos monstros (elementos novos e espécies novas)
    { "Sopro Frio",        GELO,    45,  100,   0, CONGELAR,       10,  0, false },
    { "Faísca",            RAIO,    45,  100,   0, PARALISAR,      10,  0, false },
    { "Toque Sombrio",     TREVAS,  45,  100,   0, DRENAR,        100,  0, false },
    { "Lampejo",           LUZ,     45,  100,   0, OFUSCAR,        10,  0, false },
    { "Teia",              NORMAL,   0,   90,   0, PRENDER,       100,  0, false },
    { "Esporos",           PLANTA,   0,   80,   0, ADORMECER,     100,  0, false },
};

// Golpe que o jogador já sabe ao ser criado, conforme o elemento escolhido.
int golpeInicialDoElemento(Elemento e) {
    static const int GOLPE_INICIAL[NUM_ELEMENTOS] = {
        G_GOLPE_FORTE, G_TOCHA, G_JATO_DAGUA, G_CHICOTE_DE_VINHA, G_LAMINA_VENENOSA, G_PEDRADA,
        G_RAJADA_DE_GELO, G_CHOQUE, G_GARRA_DAS_TREVAS, G_RAIO_DE_LUZ
    };
    return GOLPE_INICIAL[e];
}

// Os monstros ganham, a cada partida, um elemento sorteado. Um dos golpes da
// espécie vira o golpe deste elemento: o fraco (poder 45) para as espécies
// dos primeiros andares e o forte (poder 65-70) para as dos andares fundos.
int golpeElementalDosMonstros(Elemento e, bool forte) {
    static const int FRACO[NUM_ELEMENTOS] = { G_MORDIDA, G_BRASA, G_BOLHA, G_ESPINHOS, G_FERRAO_VENENOSO, G_PEDREGULHO,
                                              G_SOPRO_FRIO, G_FAISCA, G_TOQUE_SOMBRIO, G_LAMPEJO };
    static const int FORTE[NUM_ELEMENTOS] = { G_INVESTIDA, G_LABAREDA, G_CHICOTE_DAGUA, G_FOLHA_NAVALHA, G_ACIDO, G_ESMAGAR,
                                              G_NEVASCA, G_TROVAO, G_PULSO_DAS_TREVAS, G_CLARAO };
    return forte ? FORTE[e] : FRACO[e];
}

// Golpes que o jogador aprende ao chegar em cada nível.
struct GolpeAprendido {
    int nivel;
    int golpe;
};
// v1.23: os gerais (todo mundo) e a linha de cada elemento.
static const GolpeAprendido APRENDIZADO[] = {
    {  9, G_GRITO_DE_GUERRA },
    { 18, G_GOLPE_DUPLO },
    { 27, G_FOCO },
};
static const int NUM_APRENDIZADOS = sizeof(APRENDIZADO) / sizeof(APRENDIZADO[0]);
static const int NIVEIS_DA_LINHA[3] = { 12, 24, 30 };
static const int LINHA_DO_ELEMENTO[NUM_ELEMENTOS][3] = {
    { G_INVESTIDA,        G_FURIA,               G_GOLPE_FINAL },          // Normal
    { G_LABAREDA,         G_MURALHA_DE_FOGO,     G_INFERNO },              // Fogo
    { G_BOLHA_CURATIVA,   G_ONDA,                G_MAREMOTO },             // Água
    { G_FOLHA_NAVALHA,    G_TEMPESTADE_DE_FOLHAS, G_SEMENTE_SANGUESSUGA }, // Planta
    { G_NUVEM_TOXICA,     G_ACIDO_CORROSIVO,     G_PESTE },                // Veneno
    { G_MURALHA_DE_PEDRA, G_TERREMOTO,           G_AVALANCHE },            // Pedra
    { G_ARMADURA_DE_GELO, G_NEVASCA,             G_ZERO_ABSOLUTO },        // Gelo
    { G_PASSO_DO_RAIO,    G_TROVAO,              G_TEMPESTADE },           // Raio
    { G_PAVOR,            G_PULSO_DAS_TREVAS,    G_ECLIPSE },              // Trevas
    { G_CURA_DE_LUZ,      G_CLARAO,              G_JULGAMENTO },           // Luz
};

std::string resumoDoElemento(Elemento e, bool curto) {
    auto lista = [&](bool atacando, int valor) {
        std::string s;
        for (int o = 0; o < NUM_ELEMENTOS; o++) {
            int v = atacando ? eficacia(e, (Elemento)o) : eficacia((Elemento)o, e);
            if (v != valor) continue;
            if (!s.empty()) s += ", ";
            s += NOMES_DOS_ELEMENTOS[o];
        }
        return s.empty() ? std::string("nenhum") : s;
    };
    if (curto) return "Golpes de " + std::string(NOMES_DOS_ELEMENTOS[e]) + " causam +" +
                      std::to_string(BONUS_DO_PROPRIO_ELEMENTO) + "% de dano. Forte contra: " + lista(true, 4) + ".";
    return "Golpe inicial: " + std::string(GOLPES[golpeInicialDoElemento(e)].nome) + ". Golpes de " +
           NOMES_DOS_ELEMENTOS[e] + " causam +" + std::to_string(BONUS_DO_PROPRIO_ELEMENTO) + "% de dano. " +
           "Forte contra: " + lista(true, 4) + ". Leva o dobro de: " + lista(false, 4) +
           ". Resiste a: " + lista(false, 1) + ".";
}

// v1.24.1: a sequência de cada elemento é o golpe inicial e depois a linha.
// Na de Planta, o Pó do Sono vem logo depois do Chicote de Vinha (só se
// aprende por pergaminho: subindo de nível, ele é pulado).
int sequenciaDoElemento(Elemento e, int sequencia[5]) {
    int n = 0;
    sequencia[n++] = golpeInicialDoElemento(e);
    if (e == PLANTA) sequencia[n++] = G_PO_DO_SONO;
    for (int k = 0; k < 3; k++) sequencia[n++] = LINHA_DO_ELEMENTO[e][k];
    return n;
}

int proximoGolpeDaSequencia(Elemento e, const int golpes[], int numGolpes, bool doPergaminho) {
    int sequencia[5];
    int n = sequenciaDoElemento(e, sequencia);
    // O que vem depois do golpe mais adiantado que ele sabe (quem esqueceu um
    // golpe para abrir espaço não recebe o mesmo de volta).
    int proximo = 0;
    for (int k = 0; k < n; k++)
        for (int s = 0; s < numGolpes; s++) if (golpes[s] == sequencia[k]) proximo = k + 1;
    if (!doPergaminho) {
        // Subindo de nível: o golpe inicial veio na criação, e o Pó do Sono é
        // só de pergaminho.
        if (proximo == 0) proximo = 1;
        while (proximo < n && sequencia[proximo] == G_PO_DO_SONO) proximo++;
    }
    return proximo < n ? sequencia[proximo] : -1;
}

int golpesDoNivel(Elemento e, const int golpesQueSabe[], int numGolpes, int nivel, int golpes[2]) {
    int n = 0;
    for (int i = 0; i < NUM_APRENDIZADOS; i++) if (APRENDIZADO[i].nivel == nivel) golpes[n++] = APRENDIZADO[i].golpe;
    // Nos níveis 12, 24 e 30: o próximo golpe da sequência do elemento (se um
    // pergaminho já adiantou a linha, vem o seguinte; v1.24.1).
    for (int k = 0; k < 3 && n < 2; k++) {
        if (NIVEIS_DA_LINHA[k] != nivel) continue;
        int g = proximoGolpeDaSequencia(e, golpesQueSabe, numGolpes, false);
        if (g >= 0) golpes[n++] = g;
    }
    return n;
}

// =============================================================================
// FÓRMULAS
// =============================================================================
int vidaPeloNivel(int base, int nivel) {
    return (2 * base * nivel) / 100 + nivel + 10;
}

int atributoPeloNivel(int base, int nivel) {
    return (2 * base * nivel) / 100 + 5;
}

int experienciaParaNivel(int nivel) {
    return 4 * nivel * nivel * nivel / 5;
}

// Estágios, como no Pokémon: +1 = x1,5; +2 = x2; -1 = x0,67; -2 = x0,5...
static int aplicarEstagio(int valor, int estagio) {
    if (estagio >= 0) return valor * (2 + estagio) / 2;
    return valor * 2 / (2 - estagio);
}

static int velocidadeEfetiva(const Lutador& l) {
    return (l.status == PARALISADO) ? l.velocidade / 4 : l.velocidade;
}

// Fórmula de dano no estilo dos Pokémon:
//   dano = ((2 x nível / 5 + 2) x poder x ataque / defesa) / 50 + 2
// depois multiplicado por: mesmo elemento do golpe (x1,5), eficácia do
// elemento (x0,5 / x1 / x2), crítico (x1,5, chance de 1/16) e um sorteio
// entre 85% e 100%.
// O golpe ganha o bônus do próprio elemento? (Os golpes que um monstro
// trocou para o elemento dele não ganham: v1.24.1.)
static bool bonusDoElemento(const Lutador& atacante, const Golpe& golpe) {
    if (golpe.elemento != atacante.elemento) return false;
    for (int k = 0; k < 2; k++)
        if (atacante.semBonusDoElemento[k] >= 0 && &GOLPES[atacante.semBonusDoElemento[k]] == &golpe) return false;
    return true;
}

static int calcularDano(Jogo& jogo, const Lutador& atacante, const Lutador& alvo,
                        const Golpe& golpe, bool& critico, int& efic) {
    int ataque = aplicarEstagio(atacante.ataque, atacante.estagioAtaque);
    int defesa = aplicarEstagio(alvo.defesa, alvo.estagioDefesa);
    if (defesa < 1) defesa = 1;

    int dano = ((2 * atacante.nivel / 5 + 2) * golpe.poder * ataque / defesa) / 50 + 2;
    if (bonusDoElemento(atacante, golpe)) dano = dano * (100 + BONUS_DO_PROPRIO_ELEMENTO) / 100;
    // Queimado: o ataque cai para 3/4.
    if (atacante.status == QUEIMADO) dano = dano * 3 / 4;

    efic = eficacia(golpe.elemento, alvo.elemento);
    dano = dano * efic / 2;

    // Arma elemental: golpes do mesmo elemento da arma causam +25%.
    if (golpe.elemento == atacante.elementoDaArma) dano = dano * 5 / 4;

    // Crítico: 6% de base (como o 1/16 dos Pokémon) + o bônus da Destreza.
    critico = sortear(jogo.sorte, 1, 100) <= 6 + atacante.criticoExtra || atacante.focado;
    if (critico) dano = dano * 3 / 2;

    dano = dano * sortear(jogo.sorte, 85, 100) / 100;
    if (dano < 1) dano = 1;
    return dano;
}

// =============================================================================
// TELA: narração, animação e menus
// =============================================================================
static std::string comMaiuscula(std::string texto) {
    if (!texto.empty() && texto[0] >= 'a' && texto[0] <= 'z') texto[0] = texto[0] - 'a' + 'A';
    return texto;
}

// Batalha automática (Opções): quanto tempo a frase fica na tela antes de
// passar sozinha. Depende do tamanho dela e da velocidade do texto.
static int tempoDaFrase(const std::string& texto, TipoMensagem tipo) {
    static const int BASE[4] = { 900, 650, 450, 300 };      // lenta, normal, rápida, na hora
    static const int POR_LETRA[4] = { 30, 22, 14, 9 };
    int v = opcoes().velocidadeDoTexto;
    if (v < 0) v = 0;
    if (v > 3) v = 3;
    int ms = BASE[v] + POR_LETRA[v] * (int)texto.size();
    if (tipo != MSG_NORMAL) ms += 400;                       // crítico, super eficaz...
    if (opcoes().animacaoRapida) ms = ms * 3 / 4;
    return ms;
}

// Mostra um texto na caixa e espera o jogador apertar uma tecla. Na batalha
// automática (o padrão), a frase passa sozinha, a não ser que "esperaEnter"
// seja true: subir de nível, golpe novo, dicas, vida baixa, desmaio e o fim
// da luta sempre esperam o Enter, para ninguém perder o que aconteceu.
static void narrar(Jogo& jogo, Batalha& b, const std::string& texto, TipoMensagem tipo = MSG_NORMAL,
                   bool esperaEnter = false) {
    b.texto = texto;
    b.tipoDoTexto = tipo;
    b.textoDesde = relogioMs();
    b.menu = MENU_NENHUM;
    b.passaSozinhoEm = 0;
    if (jogo.modoAutomatico) return;
    // Mensagem importante: durante tempoDeLeituraMs() as teclas são lidas e
    // jogadas fora. Quem aperta Enter sem parar não pula o aviso sem ver.
    if (tipo != MSG_NORMAL) {
        b.travado = true;
        while (relogioMs() - b.textoDesde < tempoDeLeituraMs()) {
            desenharBatalha(jogo, b);
            lerTeclaComEspera(30);
        }
        b.travado = false;
    }
    if (opcoes().batalhaAutomatica && !esperaEnter) {
        b.passaSozinhoEm = b.textoDesde + tempoDaFrase(texto, tipo);
        while (relogioMs() < b.passaSozinhoEm) {
            desenharBatalha(jogo, b);
            int tecla = lerTeclaComEspera(30);
            if (tecla != TECLA_NENHUMA && tecla != TECLA_REDIMENSIONOU) break;   // Enter adianta
        }
        b.passaSozinhoEm = 0;
        return;
    }
    b.passaSozinhoEm = -1;   // agora sim: esperando o Enter
    int tecla;
    do {
        desenharBatalha(jogo, b);
        tecla = lerTecla();
    } while (tecla == TECLA_REDIMENSIONOU);
    b.passaSozinhoEm = 0;
}

// Ponteiro para a vida de quem está na posição "quem" (0 = jogador, 1..3 = inimigos).
static int* vidaDe(Batalha& b, int quem) {
    return (quem == 0) ? b.jogador.vida : b.inimigos[quem - 1].vida;
}

// A barra de vida "escorre" até o valor novo, como nos jogos antigos.
static void animarVida(Jogo& jogo, Batalha& b, int quem) {
    int alvo = *vidaDe(b, quem);
    if (jogo.modoAutomatico) {
        b.vidaMostrada[quem] = alvo;
        return;
    }
    int inicio = b.vidaMostrada[quem];
    const int PASSOS = opcoes().animacaoRapida ? 6 : 12;
    for (int i = 1; i <= PASSOS; i++) {
        b.vidaMostrada[quem] = inicio + (alvo - inicio) * i / PASSOS;
        desenharBatalha(jogo, b);
        esperarMs(25);
    }
}

static bool teclaConfirma(int t) { return t == TECLA_ENTER || t == 'z' || t == ' '; }
static bool teclaVolta(int t)    { return t == TECLA_ESC || t == 'x' || t == 8 || t == 127; }

// Deixa o jogador escolher uma opção do menu aberto. As opções ficam numa
// grade com "colunas" colunas. Devolve o índice escolhido, ou -1 se voltou.
static int escolherOpcao(Jogo& jogo, Batalha& b, int menu, int numOpcoes, int colunas,
                         const std::string& texto) {
    b.menu = menu;
    b.texto = texto;
    if (b.cursor >= numOpcoes) b.cursor = 0;
    while (true) {
        desenharBatalha(jogo, b);
        int t = lerTecla();
        if (t == TECLA_REDIMENSIONOU) continue;
        if (teclaConfirma(t)) { tocarSom(SOM_MENU); return b.cursor; }
        if (t >= TECLA_CLIQUE_MENU && t < TECLA_CLIQUE_MENU+numOpcoes) { tocarSom(SOM_MENU); return t-TECLA_CLIQUE_MENU; }
        if (teclaVolta(t)) return -1;
        if (t >= '1' && t < '1' + numOpcoes) return t - '1';

        int linha = b.cursor / colunas, coluna = b.cursor % colunas;
        if (t == TECLA_CIMA)     linha--;
        if (t == TECLA_BAIXO)    linha++;
        if (t == TECLA_ESQUERDA) coluna--;
        if (t == TECLA_DIREITA)  coluna++;
        if (coluna < 0) coluna = colunas - 1;
        if (coluna >= colunas) coluna = 0;
        int linhas = (numOpcoes + colunas - 1) / colunas;
        if (linha < 0) linha = linhas - 1;
        if (linha >= linhas) linha = 0;
        int novo = linha * colunas + coluna;
        if (novo < numOpcoes && novo != b.cursor) {
            b.cursor = novo;
            tocarSom(SOM_MENU);
        }
    }
}

// =============================================================================
// EFEITOS DOS GOLPES
// =============================================================================
static void mudarEstagio(Jogo& jogo, Batalha& b, Lutador& quem, int& estagio, int mudanca,
                         const std::string& atributo) {
    std::string nome = comMaiuscula(quem.nome);
    if ((mudanca > 0 && estagio >= 6) || (mudanca < 0 && estagio <= -6)) {
        narrar(jogo, b, nome + ": " + atributo + " não muda mais!");
        return;
    }
    estagio += mudanca;
    if (mudanca > 0) narrar(jogo, b, nome + ": " + atributo + " aumentou!");
    else             narrar(jogo, b, nome + ": " + atributo + " diminuiu!");
}

// Dica na batalha: aparece na própria caixa de texto, uma vez só (dicas.h).
static void dicaNaBatalha(Jogo& jogo, Batalha& b, Dica dica) {
    if (jogo.modoAutomatico || !dicaPendente(dica)) return;
    marcarDicaVista(dica);
    narrar(jogo, b, std::string("DICA - ") + tituloDaDica(dica) + ": " + textoDaDica(dica), MSG_BOA, true);
}

static int ladoDe(const Lutador& l) { return l.indice; }

// Cada elemento é imune ao "seu" estado: Veneno não se envenena, Fogo não
// queima, Gelo não congela, Raio não paralisa e Luz não fica ofuscada.
static Status statusDoEfeito(EfeitoGolpe e) {
    switch (e) {
        case ENVENENAR: return ENVENENADO; case ADORMECER: return ADORMECIDO; case PARALISAR: return PARALISADO;
        case QUEIMAR: return QUEIMADO; case CONGELAR: return CONGELADO; case OFUSCAR: return OFUSCADO;
        case ATORDOAR: return ATORDOADO; case PRENDER: return PRESO; default: return SEM_STATUS;
    }
}

static bool imuneAoStatus(const Lutador& alvo, Status status) {
    return (status == ENVENENADO && alvo.elemento == VENENO) || (status == QUEIMADO && alvo.elemento == FOGO) ||
           (status == CONGELADO && alvo.elemento == GELO) || (status == PARALISADO && alvo.elemento == RAIO) ||
           (status == OFUSCADO && alvo.elemento == LUZ);
}

static void aplicarStatus(Jogo& jogo, Batalha& b, Lutador& alvo, Status status, bool avisarFalha) {
    std::string nome = comMaiuscula(alvo.nome);
    if (alvo.status != SEM_STATUS || imuneAoStatus(alvo, status)) {
        if (avisarFalha) narrar(jogo, b, "Não teve efeito em " + alvo.nome + ".");
        return;
    }
    if ((status == ADORMECIDO || status == CONGELADO) && alvo.ehChefe && b.chefeFurioso) {
        narrar(jogo, b, "A fúria não deixa " + alvo.nome + (status == ADORMECIDO ? " dormir!" : " congelar!"));
        return;
    }
    alvo.status = status;
    Som som = SOM_PARALISIA;
    if (status == ENVENENADO) som = SOM_VENENO;
    if (status == ADORMECIDO) som = SOM_SONO;
    if (status == QUEIMADO) som = SOM_QUEIMADO;
    if (status == CONGELADO) som = SOM_CONGELADO;
    if (status == OFUSCADO) som = SOM_OFUSCADO;
    if (status == ATORDOADO || status == PRESO) som = SOM_ATORDOADO;
    tocarSom(som);
    // Status no jogador é perigo; no inimigo, é uma boa notícia.
    TipoMensagem tipo = alvo.ehJogador ? MSG_PERIGO : MSG_BOA;
    bool fem = !alvo.ehJogador && alvo.nome.compare(0, 2, "a ") == 0;   // "a aranha" / "o goblin"
    std::string o = fem ? "a" : "o";
    if (status == ENVENENADO) narrar(jogo, b, nome + " foi envenenad" + o + "!", tipo);
    if (status == PARALISADO) narrar(jogo, b, nome + " ficou paralisad" + o + "!", tipo);
    if (status == QUEIMADO)   narrar(jogo, b, nome + " pegou fogo! Está queimad" + o + ".", tipo);
    if (status == OFUSCADO)   narrar(jogo, b, nome + " ficou ofuscad" + o + "! Vai errar mais.", tipo);
    if (status == ATORDOADO)  narrar(jogo, b, nome + " ficou atordoad" + o + "!", tipo);
    if (status == PRESO)      narrar(jogo, b, nome + " ficou pres" + o + "!", tipo);
    if (status == ADORMECIDO) {
        alvo.rodadasDormindo = sortear(jogo.sorte, 1, 3);
        narrar(jogo, b, nome + " caiu no sono!", tipo);
    }
    if (status == CONGELADO) {
        alvo.rodadasDormindo = sortear(jogo.sorte, 1, 2);
        narrar(jogo, b, nome + " congelou!", tipo);
    }
    if (alvo.ehJogador) dicaNaBatalha(jogo, b, DICA_STATUS);
}

static void curar(Jogo& jogo, Batalha& b, Lutador& quem, int porcento) {
    std::string nome = comMaiuscula(quem.nome);
    if (*quem.vida >= quem.vidaMaxima) { narrar(jogo, b, nome + " já está com a vida cheia."); return; }
    int antes = *quem.vida;
    *quem.vida += quem.vidaMaxima * porcento / 100;
    if (*quem.vida > quem.vidaMaxima) *quem.vida = quem.vidaMaxima;
    tocarSom(SOM_CURA);
    animarBatalha(jogo, b, ANIM_CURA, ladoDe(quem), ladoDe(quem), 0, *quem.vida - antes, false);
    animarVida(jogo, b, ladoDe(quem));
    narrar(jogo, b, nome + " recuperou " + std::to_string(*quem.vida - antes) + " de vida!", quem.ehJogador ? MSG_BOA : MSG_NORMAL);
}

static void aplicarEfeito(Jogo& jogo, Batalha& b, Lutador& atacante, Lutador& alvo,
                          const Golpe& golpe, bool golpeDeStatus) {
    switch (golpe.efeito) {
        case ENVENENAR:    aplicarStatus(jogo, b, alvo, ENVENENADO, golpeDeStatus); break;
        case ADORMECER:    aplicarStatus(jogo, b, alvo, ADORMECIDO, golpeDeStatus); break;
        case PARALISAR:    aplicarStatus(jogo, b, alvo, PARALISADO, golpeDeStatus); break;
        case QUEIMAR:      aplicarStatus(jogo, b, alvo, QUEIMADO, golpeDeStatus); break;
        case CONGELAR:     aplicarStatus(jogo, b, alvo, CONGELADO, golpeDeStatus); break;
        case OFUSCAR:      aplicarStatus(jogo, b, alvo, OFUSCADO, golpeDeStatus); break;
        case ATORDOAR:     aplicarStatus(jogo, b, alvo, ATORDOADO, golpeDeStatus); break;
        case PRENDER:      aplicarStatus(jogo, b, alvo, PRESO, golpeDeStatus); break;
        case SOBE_ATAQUE:  mudarEstagio(jogo, b, atacante, atacante.estagioAtaque, +1, "ataque"); break;
        case SOBE_DEFESA:  mudarEstagio(jogo, b, atacante, atacante.estagioDefesa, +1, "defesa"); break;
        case BAIXA_ATAQUE: mudarEstagio(jogo, b, alvo, alvo.estagioAtaque, -1, "ataque"); break;
        case BAIXA_DEFESA: mudarEstagio(jogo, b, alvo, alvo.estagioDefesa, -1, "defesa"); break;
        case CURAR:        curar(jogo, b, atacante, golpe.chanceDoEfeito); break;
        case FOCO:
            atacante.focado = true;
            narrar(jogo, b, comMaiuscula(atacante.nome) + " se concentra: o próximo golpe vai ser crítico!", MSG_BOA);
            break;
        case SANGUESSUGA:
            if (alvo.semente || alvo.elemento == PLANTA) { narrar(jogo, b, "Não teve efeito em " + alvo.nome + "."); break; }
            alvo.semente = true;
            tocarSom(SOM_FOLHAS);
            narrar(jogo, b, "Uma semente brota em " + alvo.nome + "! Ela vai sugar a vida a cada rodada.",
                   atacante.ehJogador ? MSG_BOA : MSG_PERIGO);
            break;
        default: break;
    }
}

// Golpes que só mexem em quem usa (Escudo, Grito de Guerra, curas, Foco).
static bool golpeEmSiMesmo(const Golpe& g) {
    return g.poder == 0 && (g.efeito == SOBE_ATAQUE || g.efeito == SOBE_DEFESA || g.efeito == CURAR || g.efeito == FOCO);
}

// Efeito extra da arma elemental do jogador (v1.23): depois de um golpe com dano.
static void efeitoDaArma(Jogo& jogo, Batalha& b, Lutador& atacante, Lutador& alvo, const Golpe& golpe, int dano) {
    if (!atacante.ehJogador || atacante.elementoDaArma < 0 || dano <= 0) return;
    int e = atacante.elementoDaArma;
    if (e == PLANTA) {   // golpes de Planta recuperam 5% da vida
        if (golpe.elemento == PLANTA && *atacante.vida < atacante.vidaMaxima) {
            int cura = atacante.vidaMaxima / 20; if (cura < 1) cura = 1;
            *atacante.vida = std::min(atacante.vidaMaxima, *atacante.vida + cura);
            animarVida(jogo, b, ladoDe(atacante));
            narrar(jogo, b, "O Chicote de Espinhos devolve um pouco da sua vida.", MSG_BOA);
        }
        return;
    }
    if (e == TREVAS) {   // drena 10% do dano
        int cura = dano / 10; if (cura < 1) cura = 1;
        if (*atacante.vida < atacante.vidaMaxima) {
            *atacante.vida = std::min(atacante.vidaMaxima, *atacante.vida + cura);
            animarVida(jogo, b, ladoDe(atacante));
            narrar(jogo, b, "A Foice das Trevas suga um pouco de vida para você.", MSG_BOA);
        }
        return;
    }
    if (*alvo.vida <= 0 || sortear(jogo.sorte, 1, 100) > 10) return;
    switch (e) {
        case FOGO:   aplicarStatus(jogo, b, alvo, QUEIMADO, false); break;
        case VENENO: aplicarStatus(jogo, b, alvo, ENVENENADO, false); break;
        case PEDRA:  aplicarStatus(jogo, b, alvo, ATORDOADO, false); break;
        case GELO:   aplicarStatus(jogo, b, alvo, CONGELADO, false); break;
        case RAIO:   aplicarStatus(jogo, b, alvo, PARALISADO, false); break;
        case LUZ:    aplicarStatus(jogo, b, alvo, OFUSCADO, false); break;
        case AGUA:   mudarEstagio(jogo, b, alvo, alvo.estagioDefesa, -1, "defesa"); break;
        default: break;
    }
}

// =============================================================================
// UMA AÇÃO DE GOLPE
// =============================================================================
// Estados que impedem de agir. Devolve false se o lutador perde a vez.
static bool consegueAgir(Jogo& jogo, Batalha& b, Lutador& atacante) {
    std::string nome = comMaiuscula(atacante.nome);
    if (atacante.status == ADORMECIDO) {
        atacante.rodadasDormindo--;
        if (atacante.rodadasDormindo > 0) {
            narrar(jogo, b, nome + " está dormindo...");
            return false;
        }
        atacante.status = SEM_STATUS;
        narrar(jogo, b, nome + " acordou!");
    }
    if (atacante.status == CONGELADO) {
        atacante.rodadasDormindo--;
        if (atacante.rodadasDormindo > 0) {
            narrar(jogo, b, nome + " está congelado e não consegue se mexer!");
            return false;
        }
        atacante.status = SEM_STATUS;
        tocarSom(SOM_DEGELO);
        narrar(jogo, b, "O gelo derreteu! " + nome + " pode lutar de novo.");
    }
    if (atacante.status == ATORDOADO || atacante.status == PRESO) {
        bool preso = atacante.status == PRESO;
        atacante.status = SEM_STATUS;
        narrar(jogo, b, nome + (preso ? " está preso e perde a vez!" : " está atordoado e perde a vez!"));
        return false;
    }
    if (atacante.status == PARALISADO && sortear(jogo.sorte, 1, 4) == 1) {
        narrar(jogo, b, nome + " está paralisado e não consegue se mexer!");
        return false;
    }
    return true;
}

// O golpe em UM alvo: acerto, bloqueio, dano e efeitos.
static void acertarAlvo(Jogo& jogo, Batalha& b, Lutador& atacante, Lutador& alvo, int idGolpe) {
    const Golpe& golpe = GOLPES[idGolpe];
    std::string nome = comMaiuscula(atacante.nome);
    // Chance de acertar = precisão do golpe + Destreza de quem ataca - esquiva
    // (Agilidade) de quem defende. Golpes que só mexem em quem usa não erram.
    int chanceDeAcertar = golpe.precisao;
    bool contraOAlvo = !golpeEmSiMesmo(golpe);
    if (contraOAlvo) chanceDeAcertar += atacante.precisaoExtra - alvo.esquiva;
    if (contraOAlvo && atacante.status == OFUSCADO) chanceDeAcertar -= 15;
    if (chanceDeAcertar > 100) chanceDeAcertar = 100;
    if (chanceDeAcertar < 10) chanceDeAcertar = 10;
    if (sortear(jogo.sorte, 1, 100) > chanceDeAcertar) {
        tocarSom(SOM_ERROU);
        animarBatalha(jogo, b, ANIM_ERRO, ladoDe(atacante), ladoDe(alvo), golpe.elemento, 0, false, idGolpe);
        if (golpe.precisao >= 100 && contraOAlvo && atacante.status != OFUSCADO) narrar(jogo, b, comMaiuscula(alvo.nome) + " se esquivou!");
        else narrar(jogo, b, "Mas errou!");
        return;
    }

    // Bloqueio: um ataque com dano contra o jogador pode parar no escudo
    // (erguido com o golpe Escudo, ou só o escudo equipado, com chance menor).
    if (alvo.ehJogador && golpe.poder > 0) {
        int chanceBloquear = b.escudoErguido ? b.chanceDoBloqueio : chanceDeBloqueio(jogo.jogador.escudo, false);
        if (chanceBloquear > 0 && sortear(jogo.sorte, 1, 100) <= chanceBloquear) {
            tocarSom(SOM_BLOQUEIO);
            animarBatalha(jogo, b, ANIM_BLOQUEIO, ladoDe(atacante), ladoDe(alvo), golpe.elemento, 0, false, idGolpe);
            narrar(jogo, b, b.escudoErguido ? "Você bloqueou o ataque com o escudo!"
                                            : "Seu escudo aparou o golpe sozinho!", MSG_BOA);
            return;
        }
    }

    // Golpes de status: só o efeito.
    if (golpe.poder == 0) {
        if (contraOAlvo) animarBatalha(jogo, b, ANIM_GOLPE, ladoDe(atacante), ladoDe(alvo), golpe.elemento, 0, false, idGolpe);
        else animarBatalha(jogo, b, ANIM_EFEITO_PROPRIO, ladoDe(atacante), ladoDe(atacante), golpe.elemento, 0, false, idGolpe);
        aplicarEfeito(jogo, b, atacante, alvo, golpe, true);
        return;
    }

    int vezes = (golpe.efeito == DOIS_GOLPES) ? 2 : (golpe.efeito == TRES_GOLPES) ? 3 : 1;
    int danoTotal = 0, efic = 2, acertos = 0;
    for (int v = 0; v < vezes && *alvo.vida > 0; v++) {
        bool critico;
        int dano = calcularDano(jogo, atacante, alvo, golpe, critico, efic);
        atacante.focado = false;
        if (!alvo.ehJogador && alvo.indice >= 1) b.ultimoElemento[alvo.indice - 1] = golpe.elemento;
        *alvo.vida -= dano;
        if (*alvo.vida < 0) *alvo.vida = 0;
        danoTotal += dano;
        acertos++;
        tocarSom(critico ? SOM_CRITICO : (alvo.ehJogador ? SOM_DANO : SOM_GOLPE));
        animarBatalha(jogo, b, ANIM_GOLPE, ladoDe(atacante), ladoDe(alvo), golpe.elemento, dano, critico, idGolpe);
        animarVida(jogo, b, ladoDe(alvo));
        if (critico) narrar(jogo, b, "Um golpe crítico!", atacante.ehJogador ? MSG_BOA : MSG_PERIGO);
    }
    if (efic > 2) {
        tocarSom(SOM_SUPER_EFICAZ);
        narrar(jogo, b, "É super eficaz!", atacante.ehJogador ? MSG_BOA : MSG_PERIGO);
    }
    if (efic < 2) narrar(jogo, b, "Não é muito eficaz...");
    if (vezes > 1) narrar(jogo, b, "Acertou " + std::to_string(acertos) + " vezes!");
    // Fogo derrete o gelo na hora.
    if (golpe.elemento == FOGO && alvo.status == CONGELADO && *alvo.vida > 0) {
        alvo.status = SEM_STATUS;
        tocarSom(SOM_DEGELO);
        narrar(jogo, b, "O fogo derreteu o gelo de " + alvo.nome + "!");
    }
    // Vida do jogador abaixo de 25%: aviso (uma vez por batalha).
    if (alvo.ehJogador && !b.avisouVidaBaixa && *alvo.vida > 0 && *alvo.vida * 4 <= alvo.vidaMaxima) {
        b.avisouVidaBaixa = true;
        narrar(jogo, b, "Cuidado: sua vida está baixa! Use uma poção ou fuja.", MSG_PERIGO, true);
    }

    if (golpe.efeito == DRENAR && *atacante.vida > 0 && *atacante.vida < atacante.vidaMaxima) {
        int antes = *atacante.vida;
        *atacante.vida += danoTotal / 2;
        if (*atacante.vida > atacante.vidaMaxima) *atacante.vida = atacante.vidaMaxima;
        animarBatalha(jogo, b, ANIM_CURA, ladoDe(atacante), ladoDe(atacante), 0, *atacante.vida - antes, false);
        animarVida(jogo, b, ladoDe(atacante));
        narrar(jogo, b, nome + " sugou a vida de " + alvo.nome + "!");
    }

    // Efeito extra (ex.: 30% de chance de envenenar), se o alvo ainda está de pé.
    if (*alvo.vida > 0 && golpe.efeito != SEM_EFEITO && golpe.efeito != DRENAR &&
        golpe.efeito != DOIS_GOLPES && golpe.efeito != TRES_GOLPES && sortear(jogo.sorte, 1, 100) <= golpe.chanceDoEfeito) {
        aplicarEfeito(jogo, b, atacante, alvo, golpe, false);
    }
    efeitoDaArma(jogo, b, atacante, alvo, golpe, danoTotal);
}

// Uma ação: o golpe do "slot" em "alvo" (ou em todos os inimigos, se for em área).
static void usarGolpe(Jogo& jogo, Batalha& b, Lutador& atacante, Lutador& alvo, int slot) {
    std::string nome = comMaiuscula(atacante.nome);
    if (!consegueAgir(jogo, b, atacante)) return;

    int idGolpe = atacante.golpes[slot];
    const Golpe& golpe = GOLPES[idGolpe];
    // O golpe que o monstro usou passa a aparecer no bestiário.
    if (!atacante.ehJogador && atacante.indice >= 1) registrarGolpe(b.monstros[atacante.indice - 1]->tipo, idGolpe);
    if (atacante.pp != nullptr && golpe.ppMaximo > 0) atacante.pp[slot]--;
    narrar(jogo, b, nome + " usa " + golpe.nome + "!");

    // Escudo: age antes de todos (prioridade 2) e fica erguido até o fim da
    // rodada. A chance depende do escudo equipado e cai pela metade se ele
    // já estava erguido na rodada anterior (senão dava para bloquear sempre).
    if (atacante.ehJogador && idGolpe == G_ESCUDO) {
        b.escudoErguido = true;
        b.chanceDoBloqueio = chanceDeBloqueio(jogo.jogador.escudo, true);
        if (b.escudoNaRodadaAnterior) {
            b.chanceDoBloqueio /= 2;
            narrar(jogo, b, "Você ergue o escudo de novo, já cansado: " + std::to_string(b.chanceDoBloqueio) +
                            "% de chance de bloquear.");
        } else {
            narrar(jogo, b, "Você ergue o escudo: " + std::to_string(b.chanceDoBloqueio) +
                            "% de chance de bloquear os ataques desta rodada.");
        }
    }

    // Golpe em área do jogador: acerta cada inimigo que ainda está na luta.
    if (golpe.area && atacante.ehJogador) {
        for (int i = 0; i < b.numInimigos; i++) {
            if (b.naLuta[i] && *b.inimigos[i].vida > 0) acertarAlvo(jogo, b, atacante, b.inimigos[i], idGolpe);
        }
        return;
    }
    acertarAlvo(jogo, b, atacante, alvo, idGolpe);
}

// =============================================================================
// ESCOLHAS
// =============================================================================
enum TipoAcao { ACAO_GOLPE, ACAO_ITEM, ACAO_FUGIR };

struct Acao {
    TipoAcao tipo;
    int slot;   // qual golpe (ACAO_GOLPE) ou qual item (ACAO_ITEM, índice em TIPOS_DE_ITEM)
    int alvo;   // qual inimigo (índice em b.inimigos), para ACAO_GOLPE
};

static int primeiroNaLuta(const Batalha& b) {
    for (int i = 0; i < b.numInimigos; i++) {
        if (b.naLuta[i]) return i;
    }
    return -1;
}

static int inimigosNaLuta(const Batalha& b) {
    int n = 0;
    for (int i = 0; i < b.numInimigos; i++) {
        if (b.naLuta[i]) n++;
    }
    return n;
}

// Golpes que só mexem em quem usa (ex.: Escudo) e os golpes em área não
// precisam de alvo.
static bool precisaDeAlvo(const Golpe& g) {
    return !golpeEmSiMesmo(g) && !g.area;
}

// Monta a lista das poções (e da bomba de fumaça) que o jogador tem na mochila.
static void listarItensUsaveis(const Jogador& j, Batalha& b) {
    b.numItensUsaveis = 0;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        bool serve = TIPOS_DE_ITEM[i].categoria == POCAO || i == I_BOMBA_DE_FUMACA;
        if (serve && j.mochila[i] > 0 && b.numItensUsaveis < 16) {
            b.itensUsaveis[b.numItensUsaveis++] = i;
        }
    }
}

// O item faria alguma coisa agora? Se não, devolve o motivo (senão, "").
static bool contraOChefe(const Batalha& b) {
    for (int i = 0; i < b.numInimigos; i++) {
        if (b.naLuta[i] && b.inimigos[i].ehChefe) return true;
    }
    return false;
}
// v1.24: contra um mini-chefe o portão está fechado (não dá para fugir).
static bool contraUmMiniChefe(const Batalha& b) {
    for (int i = 0; i < b.numInimigos; i++) {
        if (b.naLuta[i] && b.inimigos[i].ehMiniChefe) return true;
    }
    return false;
}

static std::string motivoParaNaoUsar(const Jogador& j, const Batalha& b, int tipo) {
    const TipoItem& t = TIPOS_DE_ITEM[tipo];
    if (tipo == I_BOMBA_DE_FUMACA) {
        if (contraOChefe(b)) return "A fumaça não engana o Senhor das Profundezas.";
        if (contraUmMiniChefe(b)) return "O portão está fechado: não há para onde fugir.";
        if (b.naArena) return "Na arena não há para onde fugir: a porta está trancada.";
        return "";
    }
    if (t.curaPorcento > 0 && j.vida >= j.vidaMaxima) return "Sua vida já está cheia.";
    if (t.curaStatus && b.jogador.status == SEM_STATUS) return "Você não está sob nenhum efeito.";
    if (t.restauraPP) {
        bool cheio = true;
        for (int s = 0; s < j.numGolpes; s++) {
            if (j.pp[s] < GOLPES[j.golpes[s]].ppMaximo) cheio = false;
        }
        if (cheio) return "Seus golpes já estão com os PP cheios.";
    }
    return "";
}

// Valor de um golpe para o "piloto automático" dos testes.
static int valorDoGolpe(const Batalha& b, const Lutador& atacante, const Lutador& alvo, int slot) {
    const Golpe& g = GOLPES[atacante.golpes[slot]];
    if (atacante.pp != nullptr && g.ppMaximo > 0 && atacante.pp[slot] <= 0) return -1;
    // Curas: só com pouca vida (e aí valem mais que qualquer golpe).
    if (g.efeito == CURAR) return (*atacante.vida * 100 < atacante.vidaMaxima * 40) ? 100000000 : -1;
    if (g.poder == 0) {
        // Status no alvo: vale como um golpe de poder 70 enquanto o alvo está
        // sem status e com mais da metade da vida. Subir atributo: no começo.
        bool alvoInteiro = *alvo.vida * 2 > alvo.vidaMaxima;
        switch (g.efeito) {
            case ADORMECER: case PARALISAR: case CONGELAR: case OFUSCAR: case ATORDOAR: case PRENDER: case ENVENENAR: case QUEIMAR:
                return (alvo.status == SEM_STATUS && alvoInteiro && !imuneAoStatus(alvo, statusDoEfeito(g.efeito))) ? 70 * g.precisao * 2 : 0;
            case SANGUESSUGA: return (!alvo.semente && alvoInteiro && alvo.elemento != PLANTA) ? 75 * g.precisao * 2 : 0;
            case SOBE_DEFESA: return (atacante.estagioDefesa < 1 && alvoInteiro) ? 35 * 100 * 2 : 0;
            case SOBE_ATAQUE: return (atacante.estagioAtaque < 1 && alvoInteiro) ? 35 * 100 * 2 : 0;
            case BAIXA_ATAQUE: return (alvo.estagioAtaque > -1 && alvoInteiro) ? 50 * g.precisao * 2 : 0;
            case FOCO: return (!atacante.focado && alvoInteiro) ? 60 * 100 * 2 : 0;
            default: return 0;
        }
    }
    int v = g.poder * g.precisao * eficacia(g.elemento, alvo.elemento);
    if (bonusDoElemento(atacante, g)) v = v * (100 + BONUS_DO_PROPRIO_ELEMENTO) / 100;
    if (g.efeito == DOIS_GOLPES) v *= 2;
    if (g.efeito == TRES_GOLPES) v *= 3;
    if (g.elemento == atacante.elementoDaArma) v = v * 5 / 4;
    if (g.area) {   // em área: soma um pouco por inimigo a mais
        int n = 0;
        for (int i = 0; i < b.numInimigos; i++) if (b.naLuta[i]) n++;
        v = v * (2 + n - 1) / 2;
    }
    return v;
}

static Acao escolherAcaoAutomatica(Jogo& jogo, Batalha& b) {
    Jogador& j = jogo.jogador;
    // Piloto automático dos testes: cura com pouca vida, antídoto se envenenado.
    if (j.vida * 3 < j.vidaMaxima) {
        if (j.mochila[I_POCAO_GRANDE] > 0 && j.vida * 5 < j.vidaMaxima) return { ACAO_ITEM, I_POCAO_GRANDE, 0 };
        if (j.mochila[I_POCAO_DE_VIDA] > 0) return { ACAO_ITEM, I_POCAO_DE_VIDA, 0 };
        if (j.mochila[I_POCAO_GRANDE] > 0) return { ACAO_ITEM, I_POCAO_GRANDE, 0 };
    }
    if (b.jogador.status == ENVENENADO && j.mochila[I_ANTIDOTO] > 0 && j.vida * 2 < j.vidaMaxima) {
        return { ACAO_ITEM, I_ANTIDOTO, 0 };
    }
    // Sem poção e quase desmaiando: a bomba de fumaça tira o bot da luta.
    if (j.vida * 4 < j.vidaMaxima && j.mochila[I_BOMBA_DE_FUMACA] > 0 &&
        motivoParaNaoUsar(j, b, I_BOMBA_DE_FUMACA).empty()) {
        return { ACAO_ITEM, I_BOMBA_DE_FUMACA, 0 };
    }
    // Melhor par (golpe, alvo); empatando, o alvo com menos vida.
    int melhorSlot = 0, melhorAlvo = primeiroNaLuta(b), melhorValor = -1;
    for (int i = 0; i < b.numInimigos; i++) {
        if (!b.naLuta[i]) continue;
        for (int s = 0; s < b.jogador.numGolpes; s++) {
            int v = valorDoGolpe(b, b.jogador, b.inimigos[i], s);
            bool melhor = v > melhorValor ||
                          (v == melhorValor && *b.inimigos[i].vida < *b.inimigos[melhorAlvo].vida);
            if (melhor) { melhorValor = v; melhorSlot = s; melhorAlvo = i; }
        }
    }
    return { ACAO_GOLPE, melhorSlot, melhorAlvo };
}

static Acao escolherAcaoDoJogador(Jogo& jogo, Batalha& b) {
    if (jogo.modoAutomatico) return escolherAcaoAutomatica(jogo, b);
    Jogador& j = jogo.jogador;

    while (true) {
        b.cursor = 0;
        int opcao = escolherOpcao(jogo, b, MENU_PRINCIPAL, 4, 2, "O que você vai fazer?");
        if (opcao == 0) {                                   // LUTAR
            b.cursor = 0;
            int slot = escolherOpcao(jogo, b, MENU_GOLPES, b.jogador.numGolpes, 1,
                                     "Escolha um golpe:");
            if (slot < 0) continue;
            const Golpe& g = GOLPES[b.jogador.golpes[slot]];
            if (g.ppMaximo > 0 && b.jogador.pp[slot] <= 0) {
                narrar(jogo, b, "Não há PP para este golpe!");
                continue;
            }
            // Com mais de um inimigo, escolhe o alvo.
            int alvo = primeiroNaLuta(b);
            if (precisaDeAlvo(g) && inimigosNaLuta(b) > 1) {
                b.numAlvos = 0;
                for (int i = 0; i < b.numInimigos; i++) {
                    if (b.naLuta[i]) b.alvos[b.numAlvos++] = i;
                }
                b.cursor = 0;
                int escolha = escolherOpcao(jogo, b, MENU_ALVO, b.numAlvos, 1,
                                            "Usar " + std::string(g.nome) + " em quem?");
                if (escolha < 0) continue;
                alvo = b.alvos[escolha];
            }
            return { ACAO_GOLPE, slot, alvo };
        }
        if (opcao == 1) {                                   // ITENS
            listarItensUsaveis(j, b);
            if (b.numItensUsaveis == 0) {
                narrar(jogo, b, "Você não tem poções na mochila.");
                continue;
            }
            b.cursor = 0;
            int escolha = escolherOpcao(jogo, b, MENU_ITENS, b.numItensUsaveis, 1, "Usar qual item?");
            if (escolha < 0) continue;
            int tipo = b.itensUsaveis[escolha];
            std::string motivo = motivoParaNaoUsar(j, b, tipo);
            if (!motivo.empty()) {
                narrar(jogo, b, motivo);
                continue;
            }
            return { ACAO_ITEM, tipo, 0 };
        }
        if (opcao == 2) {                                   // STATUS
            b.menu = MENU_STATUS;
            int t;
            do {
                desenharBatalha(jogo, b);
                t = lerTecla();
            } while (t == TECLA_REDIMENSIONOU);
            continue;
        }
        if (opcao == 3) return { ACAO_FUGIR, 0, 0 };        // FUGIR
    }
}

// O monstro sorteia um golpe, evitando os que não fariam nada agora.
static int escolherGolpeDoInimigo(Jogo& jogo, const Batalha& b, int qual) {
    const Lutador& eu = b.inimigos[qual];
    const Lutador& alvo = b.jogador;
    int pesos[MAX_GOLPES];
    int total = 0;
    for (int s = 0; s < eu.numGolpes; s++) {
        const Golpe& g = GOLPES[eu.golpes[s]];
        int peso = 3;                                         // golpes de dano
        if (g.poder == 0) {
            peso = 1;
            bool causaStatus = (g.efeito == ENVENENAR || g.efeito == ADORMECER || g.efeito == PARALISAR ||
                                g.efeito == QUEIMAR || g.efeito == CONGELAR || g.efeito == OFUSCAR ||
                                g.efeito == ATORDOAR || g.efeito == PRENDER);
            if (causaStatus && alvo.status != SEM_STATUS) peso = 0;
            if (g.efeito == SOBE_ATAQUE && eu.estagioAtaque >= 2) peso = 0;
            if (g.efeito == BAIXA_ATAQUE && alvo.estagioAtaque <= -2) peso = 0;
            if (g.efeito == BAIXA_DEFESA && alvo.estagioDefesa <= -2) peso = 0;
        }
        pesos[s] = peso;
        total += peso;
    }
    if (total == 0) return 0;
    int r = sortear(jogo.sorte, 1, total);
    for (int s = 0; s < eu.numGolpes; s++) {
        r -= pesos[s];
        if (r <= 0) return s;
    }
    return 0;
}

// Fuga no estilo Pokémon: chance = velocidade do jogador x 128 / velocidade do
// inimigo mais rápido + 30 por tentativa anterior, em 256. Velocidades iguais: 50%.
static bool tentarFugir(Jogo& jogo, Batalha& b) {
    int vJ = velocidadeEfetiva(b.jogador);
    int vI = 1;
    for (int i = 0; i < b.numInimigos; i++) {
        if (b.naLuta[i] && velocidadeEfetiva(b.inimigos[i]) > vI) vI = velocidadeEfetiva(b.inimigos[i]);
    }
    if (vI < 1) vI = 1;
    int chance = vJ * 128 / vI + 30 * b.tentativasDeFuga;
    b.tentativasDeFuga++;
    return chance > 255 || sortear(jogo.sorte, 0, 255) < chance;
}

// =============================================================================
// EXPERIÊNCIA E NÍVEIS
// =============================================================================
// A luta usa uma cópia dos golpes do jogador (b.jogador). Ao aprender ou
// trocar um golpe no meio dela, a cópia precisa mudar junto.
static void copiarGolpesParaALuta(const Jogador& j, Batalha& b) {
    b.jogador.numGolpes = j.numGolpes;
    for (int s = 0; s < j.numGolpes; s++) b.jogador.golpes[s] = j.golpes[s];
}

static void aprenderGolpe(Jogo& jogo, Batalha& b, int golpe) {
    Jogador& j = jogo.jogador;
    std::string nomeGolpe = GOLPES[golpe].nome;
    for (int s = 0; s < j.numGolpes; s++) {
        if (j.golpes[s] == golpe) return;   // já sabe (ex.: era o golpe inicial do elemento)
    }

    if (j.numGolpes < MAX_GOLPES) {
        j.golpes[j.numGolpes] = golpe;
        j.pp[j.numGolpes] = GOLPES[golpe].ppMaximo;
        j.numGolpes++;
        // A luta usa uma cópia dos golpes (b.jogador): ela também recebe o novo,
        // senão o menu mostra o golpe novo e sai o antigo (v1.23.1).
        copiarGolpesParaALuta(j, b);
        narrar(jogo, b, "Você aprendeu " + nomeGolpe + "!", MSG_BOA, true);
        return;
    }

    // Já sabe 4 golpes: precisa esquecer um (ou desistir do novo).
    narrar(jogo, b, "Você quer aprender " + nomeGolpe + ", mas já sabe 4 golpes.", MSG_BOA);
    int esquecer;
    if (jogo.modoAutomatico) {
        esquecer = 0; // esquece o golpe de menor poder (exceto o Golpe básico)
        int menor = 1000000;
        for (int s = 0; s < MAX_GOLPES; s++) {
            if (j.golpes[s] == G_GOLPE) continue;
            if (GOLPES[j.golpes[s]].poder < menor) { menor = GOLPES[j.golpes[s]].poder; esquecer = s; }
        }
        if (GOLPES[golpe].poder <= menor) esquecer = -1;
    } else {
        b.cursor = 0;
        b.golpeNovo = golpe;
        esquecer = escolherOpcao(jogo, b, MENU_ESQUECER, MAX_GOLPES + 1, 1,
                                 "Esquecer qual golpe para aprender " + nomeGolpe + "?");
        if (esquecer == MAX_GOLPES) esquecer = -1; // "Não aprender"
    }
    if (esquecer < 0) {
        narrar(jogo, b, "Você não aprendeu " + nomeGolpe + ".", MSG_NORMAL, true);
        return;
    }
    std::string antigo = GOLPES[j.golpes[esquecer]].nome;
    j.golpes[esquecer] = golpe;
    j.pp[esquecer] = GOLPES[golpe].ppMaximo;
    copiarGolpesParaALuta(j, b);   // a cópia da luta também (v1.23.1)
    narrar(jogo, b, "1, 2 e... pronto! Você esqueceu " + antigo + " e aprendeu " + nomeGolpe + "!", MSG_BOA, true);
}

static void ganharExperiencia(Jogo& jogo, Batalha& b, int pontos, int moedas) {
    Jogador& j = jogo.jogador;
    j.experiencia += pontos;
    j.moedas += moedas;
    if (moedas > 0) tocarSom(SOM_MOEDAS);
    narrar(jogo, b, "Você ganhou " + std::to_string(pontos) + " pontos de experiência e " +
                    std::to_string(moedas) + (moedas == 1 ? " moeda!" : " moedas!"), MSG_NORMAL,
           inimigosNaLuta(b) == 0);   // automática: só a última espera o Enter (fim da luta)

    while (j.nivel < NIVEL_MAXIMO && j.experiencia >= experienciaParaNivel(j.nivel + 1)) {
        int vidaMaximaAntes = j.vidaMaxima;
        j.nivel++;
        recalcularAtributos(j);
        j.vida += j.vidaMaxima - vidaMaximaAntes;   // o aumento vem "cheio"
        b.jogador.nivel = j.nivel;
        b.jogador.vidaMaxima = j.vidaMaxima;
        b.jogador.ataque = j.ataque;
        b.jogador.defesa = j.defesa;
        b.jogador.velocidade = j.velocidade;
        b.vidaMostrada[0] = j.vida;
        tocarSom(SOM_NIVEL);
        animarBatalha(jogo, b, ANIM_NIVEL, 0, 0, 0, j.nivel, false);
        j.moedas += MOEDAS_POR_NIVEL;
        narrar(jogo, b, "Você subiu para o nível " + std::to_string(j.nivel) + "! (+" +
                        std::to_string(MOEDAS_POR_NIVEL) + " moedas)", MSG_BOA, true);
        ganharPontosDeAtributo(jogo, PONTOS_POR_NIVEL);
        if (!jogo.modoAutomatico) {
            tocarSom(SOM_PONTO_GANHO);
            narrar(jogo, b, "Você ganhou " + std::to_string(PONTOS_POR_NIVEL) +
                            " ponto de atributo! Aperte C no mapa para distribuir.", MSG_BOA, true);
        }
        b.jogador.precisaoExtra = j.precisaoExtra;
        b.jogador.esquiva = j.esquiva;
        b.jogador.criticoExtra = j.criticoExtra;

        int novos[2];
        int n = golpesDoNivel(j.elemento, j.golpes, j.numGolpes, j.nivel, novos);
        for (int i = 0; i < n; i++) aprenderGolpe(jogo, b, novos[i]);
    }
}

// =============================================================================
// A BATALHA
// =============================================================================
static Lutador montarLutadorDoJogador(Jogador& j) {
    Lutador l;
    l.nome = j.nome;
    l.nivel = j.nivel;
    l.elemento = j.elemento;
    l.vida = &j.vida;            // ponteiro para a vida de verdade
    l.vidaMaxima = j.vidaMaxima;
    l.ataque = j.ataque;
    l.defesa = j.defesa;
    l.velocidade = j.velocidade;
    l.estagioAtaque = 0;
    l.estagioDefesa = 0;
    l.status = SEM_STATUS;
    l.rodadasDormindo = 0;
    l.semente = false;
    l.focado = false;
    l.numGolpes = j.numGolpes;
    for (int s = 0; s < j.numGolpes; s++) l.golpes[s] = j.golpes[s];
    l.pp = j.pp;                 // ponteiro para o vetor de PP do jogador
    l.elementoDaArma = (j.arma != nullptr) ? j.arma->elemento : -1;
    l.precisaoExtra = j.precisaoExtra;
    l.esquiva = j.esquiva;
    l.criticoExtra = j.criticoExtra;
    l.ehChefe = false;
    l.ehMiniChefe = false;
    l.ehCampeao = false;
    l.ehJogador = true;
    l.indice = 0;
    return l;
}

static Lutador montarLutadorDoMonstro(Monstro& m) {
    const TipoMonstro& t = tipoDe(m);
    Lutador l;
    l.nome = std::string(t.artigo) + " " + nomeDaEspecie(m);
    l.nivel = m.nivel;
    l.elemento = m.elemento;     // sorteado a cada partida (Etapa 6)
    l.vida = &m.vida;            // ponteiro para a vida do monstro no mapa
    l.vidaMaxima = vidaMaximaDoMonstro(m);
    // A força do ataque dos monstros depende da dificuldade escolhida.
    l.ataque = atributoPeloNivel(t.baseAtaque, m.nivel) * dificuldadeAtual().ataqueDosMonstros / 100;
    if (t.especial & MINI_CHEFE) l.ataque = l.ataque * ataqueDoMiniChefe(salaDoMiniChefe(m.tipo)) / 100;   // v1.24
    l.defesa = atributoPeloNivel(t.baseDefesa, m.nivel);
    l.velocidade = atributoPeloNivel(t.baseVelocidade, m.nivel);
    l.estagioAtaque = 0;
    l.estagioDefesa = 0;
    l.status = SEM_STATUS;
    l.rodadasDormindo = 0;
    l.semente = false;
    l.focado = false;
    golpesDoMonstro(m, l.golpes, l.numGolpes, l.semBonusDoElemento);
    l.pp = nullptr;              // monstros não gastam PP
    l.elementoDaArma = -1;
    l.precisaoExtra = 0;
    l.esquiva = esquivaDoMonstro(m);
    l.criticoExtra = 0;
    l.ehChefe = (t.especial & CHEFE) != 0;
    l.ehMiniChefe = (t.especial & MINI_CHEFE) != 0;
    l.ehCampeao = m.campeao;
    l.ehJogador = false;
    l.indice = 1;
    return l;
}

// No fim de cada rodada: quem está envenenado perde 1/8 da vida máxima, quem
// está queimado perde 1/16, e a Semente Sanguessuga tira 1/8 e cura o jogador.
static void danoDoVeneno(Jogo& jogo, Batalha& b, Lutador& l) {
    if (*l.vida <= 0) return;
    if (l.status == ENVENENADO || l.status == QUEIMADO) {
        bool veneno = l.status == ENVENENADO;
        int dano = l.vidaMaxima / (veneno ? 8 : 16);
        if (!l.ehJogador && l.indice >= 1) b.ultimoElemento[l.indice - 1] = veneno ? VENENO : FOGO;
        if (dano < 1) dano = 1;
        *l.vida -= dano;
        if (*l.vida < 0) *l.vida = 0;
        tocarSom(veneno ? SOM_VENENO : SOM_QUEIMADO);
        animarVida(jogo, b, ladoDe(l));
        narrar(jogo, b, comMaiuscula(l.nome) + (veneno ? " sofre com o veneno!" : " sofre com a queimadura!"),
               l.ehJogador ? MSG_PERIGO : MSG_NORMAL);
    }
    if (l.semente && *l.vida > 0 && !l.ehJogador) {
        int dano = l.vidaMaxima / 8;
        if (dano < 1) dano = 1;
        if (dano > *l.vida) dano = *l.vida;
        if (l.indice >= 1) b.ultimoElemento[l.indice - 1] = PLANTA;
        *l.vida -= dano;
        animarVida(jogo, b, ladoDe(l));
        Lutador& j = b.jogador;
        if (*j.vida > 0 && *j.vida < j.vidaMaxima) {
            *j.vida = std::min(j.vidaMaxima, *j.vida + dano);
            animarVida(jogo, b, 0);
        }
        tocarSom(SOM_FOLHAS);
        narrar(jogo, b, "A semente suga a vida de " + l.nome + "!");
    }
}

// Um inimigo desmaiou? Tira da luta e dá a experiência. Devolve true se
// a batalha acabou (não sobrou inimigo).
// Segunda fase: com metade da vida (ou menos), o chefe fica furioso uma vez
// por batalha: acorda, perde as quedas de ataque, ganha +1 de ataque e não
// cai mais no sono.
static void conferirFuriaDoChefe(Jogo& jogo, Batalha& b) {
    if (b.chefeFurioso) return;
    for (int i = 0; i < b.numInimigos; i++) {
        Lutador& l = b.inimigos[i];
        if (!b.naLuta[i] || !l.ehChefe || *l.vida <= 0 || *l.vida * 2 > l.vidaMaxima) continue;
        b.chefeFurioso = true;
        if (l.status == ADORMECIDO) l.status = SEM_STATUS;
        if (l.estagioAtaque < 0) l.estagioAtaque = 0;
        if (l.estagioAtaque < 6) l.estagioAtaque++;
        tocarSom(SOM_CRITICO);
        animarBatalha(jogo, b, ANIM_FURIA, i + 1, i + 1, 0, 0, false);
        if (!jogo.modoAutomatico)
            falaDoChefeNaBatalha(jogo, b, i, "Você... me FERIU? Agora você vai conhecer as profundezas!");
        narrar(jogo, b, comMaiuscula(l.nome) + " fica FURIOSO! O ataque dele aumenta e ele não dorme mais.",
               MSG_PERIGO);
    }
}

static bool conferirDesmaios(Jogo& jogo, Batalha& b) {
    for (int i = 0; i < b.numInimigos; i++) {
        if (!b.naLuta[i] || *b.inimigos[i].vida > 0) continue;
        // v1.23: o esqueleto se monta de novo uma vez, com 1/3 da vida, a
        // não ser que o golpe final seja de Fogo, Pedra ou Luz.
        Monstro& osso = *b.monstros[i];
        int ultimo = b.ultimoElemento[i];
        if ((tipoDe(osso).especial & REMONTA) && osso.cornetas == 0 && ultimo != FOGO && ultimo != PEDRA && ultimo != LUZ) {
            osso.cornetas = 1;
            *b.inimigos[i].vida = std::max(1, b.inimigos[i].vidaMaxima / 3);
            b.inimigos[i].status = SEM_STATUS;
            tocarSom(SOM_REMONTA);
            narrar(jogo, b, comMaiuscula(b.inimigos[i].nome) + " desmonta... e os ossos se juntam de novo!", MSG_PERIGO);
            animarBatalha(jogo, b, ANIM_CURA, i + 1, i + 1, 0, *b.inimigos[i].vida, false);
            animarVida(jogo, b, i + 1);
            dicaNaBatalha(jogo, b, DICA_ESQUELETO);
            continue;
        }
        if (b.inimigos[i].ehChefe && !jogo.modoAutomatico)
            falaDoChefeNaBatalha(jogo, b, i, "Impossível... a luz... de novo...");
        animarBatalha(jogo, b, ANIM_DESMAIO, i + 1, i + 1, 0, 0, false);
        b.naLuta[i] = false;
        Monstro& m = *b.monstros[i];
        m.vivo = false;   // derrotado: some do mapa
        // v1.24: o esqueleto do Necromante levanta de novo daqui a 2 rodadas.
        if (m.guarda && (tipoDe(m).especial & REMONTA) && b.esqueletosLevantados < 3) {
            for (int k = 0; k < b.numInimigos; k++)
                if (b.naLuta[k] && b.inimigos[k].ehMiniChefe && tipoDe(*b.monstros[k]).simbolo == 'n') b.levantaEm[i] = 2;
        }
        registrarDerrota(m);
        tocarSom(SOM_DESMAIO);
        narrar(jogo, b, comMaiuscula(b.inimigos[i].nome) + " desmaiou!");
        int experiencia = tipoDe(m).experienciaBase * m.nivel / 6 * dificuldadeAtual().experiencia / 100;
        int moedas = moedasDoMonstro(jogo, m.nivel, b.inimigos[i].ehChefe);
        if (m.campeao) { experiencia *= 2; moedas *= 3; }      // campeão: prêmio maior
        if (m.cornetas >= 2) moedas *= 2;                       // o gnomo que parou de fugir
        ganharExperiencia(jogo, b, experiencia, moedas);
    }
    return inimigosNaLuta(b) == 0;
}

// Quem age na rodada e em que ordem.
struct Agente {
    int quem;        // 0 = jogador, 1..3 = inimigo (índice + 1)
    int slot;        // golpe escolhido
    int prioridade;
    int velocidade;
    int desempate;   // sorteado, para empates de velocidade
};

static void ordenarAgentes(Agente* agentes, int n) {
    // Ordenação por inserção: maior prioridade primeiro; depois maior
    // velocidade; depois o desempate sorteado.
    for (int i = 1; i < n; i++) {
        Agente atual = agentes[i];
        int k = i - 1;
        while (k >= 0) {
            const Agente& a = agentes[k];
            bool atualVemAntes = atual.prioridade > a.prioridade ||
                (atual.prioridade == a.prioridade && atual.velocidade > a.velocidade) ||
                (atual.prioridade == a.prioridade && atual.velocidade == a.velocidade && atual.desempate > a.desempate);
            if (!atualVemAntes) break;
            agentes[k + 1] = agentes[k];
            k--;
        }
        agentes[k + 1] = atual;
    }
}

// =============================================================================
// v1.24: O QUE OS MINI-CHEFES FAZEM NA LUTA
// =============================================================================
// Rei Goblin: com metade da vida, chama mais um guarda. Rainha Aranha: a cada
// 3 rodadas desce um filhote (até 3). Necromante: enquanto ele está de pé, o
// esqueleto que cai se levanta inteiro 2 rodadas depois (até 3 vezes). Golem de
// Magma: com metade da vida, a casca de pedra quebra (fase 2, de Fogo).
static int miniChefeNaLuta(const Batalha& b, char simbolo) {
    for (int i = 0; i < b.numInimigos; i++)
        if (b.naLuta[i] && b.inimigos[i].ehMiniChefe && tipoDe(*b.monstros[i]).simbolo == simbolo) return i;
    return -1;
}

// Uma casa livre dentro da sala do chefe, para quem chega no meio da luta.
static bool casaLivreNaSala(const Jogo& jogo, int& x, int& y) {
    const Andar& a = andarAtual(jogo);
    const SalaDoChefe& s = a.salaDoChefe;
    if (!s.existe) return false;
    for (int yy = s.y + 1; yy < s.y + s.altura - 1; yy++) {
        for (int xx = s.x + 1; xx < s.x + s.largura - 1; xx++) {
            TipoCelula c = a.mapa.celulas[yy][xx];
            if (c != CHAO && c != TEIA && c != POCA) continue;
            if (monstroEm(a, xx, yy) >= 0 || (xx == jogo.jogador.x && yy == jogo.jogador.y)) continue;
            x = xx; y = yy;
            return true;
        }
    }
    return false;
}

// Um monstro novo entra na luta, na vaga de quem já caiu (ou numa vaga vazia).
static bool chamarReforco(Jogo& jogo, Batalha& b, int tipo, Som som, const std::string& aviso) {
    int vaga = -1;
    for (int i = 0; i < b.numInimigos && vaga < 0; i++) if (!b.naLuta[i]) vaga = i;
    if (vaga < 0 && b.numInimigos < MAX_INIMIGOS) vaga = b.numInimigos;
    int x, y;
    if (vaga < 0 || !casaLivreNaSala(jogo, x, y)) return false;
    Andar& a = andarAtual(jogo);
    int folga = dificuldadeAtual().folgaDeNivel > 0 ? dificuldadeAtual().folgaDeNivel : 0;
    int nivel = std::max(1, jogo.jogador.nivel - folga - FOLGA_DOS_GUARDAS);
    int k = criarMonstroExtra(a, tipo, x, y, nivel, jogo.elementoDaEspecie[tipo]);
    if (k < 0) return false;
    Monstro& m = a.monstros[k];
    m.guarda = true;
    m.estado = CACANDO;
    if (vaga == b.numInimigos) b.numInimigos++;
    b.monstros[vaga] = &m;
    b.inimigos[vaga] = montarLutadorDoMonstro(m);
    b.inimigos[vaga].ataque = b.inimigos[vaga].ataque * bonusDoAmbiente(jogo.andar, b.inimigos[vaga].elemento) / 100;
    b.inimigos[vaga].indice = vaga + 1;
    b.inimigos[vaga].nome += std::string(" ") + (char)('A' + 2 + b.reforcosChamados);   // "o goblin C"
    b.naLuta[vaga] = true;
    b.ultimoElemento[vaga] = -1;
    b.levantaEm[vaga] = 0;
    b.vidaMostrada[vaga + 1] = m.vida;
    registrarAvistado(jogo, m, true);
    b.reforcosChamados++;
    tocarSom(som);
    animarBatalha(jogo, b, ANIM_CHEGADA, vaga + 1, vaga + 1, 0, 0, false);
    narrar(jogo, b, aviso, MSG_PERIGO);
    return true;
}

// O Golem perde a casca: vira Fogo, fica mais rápido e mais forte, mas a
// defesa cai.
static void quebrarACascaDoGolem(Jogo& jogo, Batalha& b, int i) {
    Lutador& l = b.inimigos[i];
    b.golemQuebrou = true;
    l.elemento = FOGO;
    static const int GOLPES_DE_MAGMA[4] = { G_LABAREDA, G_INFERNO, G_ESMAGAR, G_BRASA };
    l.numGolpes = 4;
    for (int s = 0; s < 4; s++) l.golpes[s] = GOLPES_DE_MAGMA[s];
    l.defesa = l.defesa * 7 / 10;
    l.velocidade = l.velocidade * 2;
    if (l.estagioAtaque < 0) l.estagioAtaque = 0;
    if (l.estagioAtaque < 6) l.estagioAtaque++;
    if (l.status != SEM_STATUS) l.status = SEM_STATUS;
    tocarSom(SOM_RACHADURA);
    animarBatalha(jogo, b, ANIM_FURIA, i + 1, i + 1, 0, 0, false);
    narrar(jogo, b, "A casca de pedra do Golem se parte! Por dentro ele é puro magma: agora é de Fogo, "
                    "mais rápido e mais forte, mas mais fácil de ferir.", MSG_PERIGO);
}

// Chamada depois de cada ação (fimDaRodada = false) e no fim de cada rodada.
static void conferirMiniChefes(Jogo& jogo, Batalha& b, bool fimDaRodada) {
    int rei = miniChefeNaLuta(b, 'R');
    if (rei >= 0 && b.reforcosChamados == 0 && *b.inimigos[rei].vida * 2 <= b.inimigos[rei].vidaMaxima) {
        if (!chamarReforco(jogo, b, tipoDoAjudante(SALA_DO_REI), SOM_CORNETA, "O Rei Goblin sopra o apito: \"Mais guardas! AGORA!\""))
            b.reforcosChamados = 1;   // não havia vaga: ele não tenta de novo
    }
    int golem = miniChefeNaLuta(b, 'L');
    if (golem >= 0 && !b.golemQuebrou && *b.inimigos[golem].vida * 2 <= b.inimigos[golem].vidaMaxima)
        quebrarACascaDoGolem(jogo, b, golem);
    if (!fimDaRodada) return;
    int rainha = miniChefeNaLuta(b, 'A');
    if (rainha >= 0 && jogo.rodadasDaUltimaBatalha % 3 == 0 && b.reforcosChamados < 3)
        chamarReforco(jogo, b, tipoDoAjudante(NINHO_DA_RAINHA), SOM_FOLHAS, "Um filhote desce pelo fio da teia!");
    int necro = miniChefeNaLuta(b, 'n');
    for (int i = 0; i < b.numInimigos; i++) {
        if (b.levantaEm[i] <= 0) continue;
        if (necro < 0 || b.naLuta[i] || b.esqueletosLevantados >= 3) { b.levantaEm[i] = 0; continue; }
        if (--b.levantaEm[i] > 0) continue;
        Monstro& m = *b.monstros[i];
        m.vivo = true;
        m.vida = vidaMaximaDoMonstro(m);
        std::string nome = b.inimigos[i].nome;
        b.inimigos[i] = montarLutadorDoMonstro(m);
        b.inimigos[i].nome = nome;
        b.inimigos[i].indice = i + 1;
        b.naLuta[i] = true;
        b.vidaMostrada[i + 1] = m.vida;
        b.esqueletosLevantados++;
        tocarSom(SOM_REMONTA);
        animarBatalha(jogo, b, ANIM_CHEGADA, i + 1, i + 1, 0, 0, false);
        narrar(jogo, b, "O Necromante ergue o cajado: " + b.inimigos[i].nome + " se levanta de novo!", MSG_PERIGO);
    }
}

ResultadoBatalha batalhar(Jogo& jogo, Monstro* participantes[], int quantos) {
    if (quantos > MAX_INIMIGOS) quantos = MAX_INIMIGOS;
    Batalha b;
    b.jogador = montarLutadorDoJogador(jogo.jogador);
    b.vidaMostrada[0] = *b.jogador.vida;
    b.numInimigos = quantos;
    for (int i = 0; i < quantos; i++) {
        b.monstros[i] = participantes[i];
        if (tipoDe(*participantes[i]).especial & (CHEFE | MINI_CHEFE)) ajustarNivelDoChefe(*participantes[i], jogo.jogador.nivel);
        b.inimigos[i] = montarLutadorDoMonstro(*participantes[i]);
        // O ambiente fortalece um elemento (Água na caverna, Fogo no vulcão).
        b.inimigos[i].ataque = b.inimigos[i].ataque * bonusDoAmbiente(jogo.andar, b.inimigos[i].elemento) / 100;
        b.inimigos[i].indice = i + 1;
        b.naLuta[i] = true;
        b.ultimoElemento[i] = -1;
        registrarAvistado(jogo, *participantes[i], true);
        b.vidaMostrada[i + 1] = *b.inimigos[i].vida;
    }
    // Dois monstros da mesma espécie ganham uma letra: "o chacal A", "o chacal B".
    for (int i = 0; i < quantos; i++) {
        int iguais = 0, minhaLetra = 0;
        for (int k = 0; k < quantos; k++) {
            if (participantes[k]->tipo == participantes[i]->tipo) {
                if (k < i) minhaLetra++;
                iguais++;
            }
        }
        if (iguais > 1) b.inimigos[i].nome += std::string(" ") + (char)('A' + minhaLetra);
    }
    bool contraOChefe = false;
    for (int i = 0; i < quantos; i++) {
        if (tipoDe(*participantes[i]).especial & CHEFE) contraOChefe = true;
    }
    bool contraMiniChefe = false;
    for (int i = 0; i < quantos; i++) {
        if (tipoDe(*participantes[i]).especial & MINI_CHEFE) contraMiniChefe = true;
    }
    tocarMusica(contraOChefe ? MUSICA_CHEFE : contraMiniChefe ? MUSICA_MINI_CHEFE : MUSICA_BATALHA);
    // v1.24: na sala de um chefe, a luta tem o fundo da sala.
    b.salaDaBatalha = -1;
    if ((contraOChefe || contraMiniChefe) && andarAtual(jogo).salaDoChefe.existe) b.salaDaBatalha = andarAtual(jogo).salaDoChefe.tipo;
    b.reforcosChamados = 0;
    b.esqueletosLevantados = 0;
    b.golemQuebrou = false;
    for (int i = 0; i < MAX_INIMIGOS; i++) b.levantaEm[i] = 0;
    b.tentativasDeFuga = 0;
    b.tipoDoTexto = MSG_NORMAL;
    b.textoDesde = 0;
    b.passaSozinhoEm = 0;
    b.travado = false;
    b.avisouVidaBaixa = false;
    b.menu = MENU_NENHUM;
    b.cursor = 0;
    b.golpeNovo = -1;
    b.numItensUsaveis = 0;
    b.escudoErguido = false;
    b.escudoNaRodadaAnterior = false;
    b.chanceDoBloqueio = 0;
    b.chefeFurioso = false;
    b.naArena = false;
    {
        const Andar& andar = andarAtual(jogo);
        for (int k = 0; k < andar.numEspeciais; k++) {
            const Especial& e = andar.especiais[k];
            if (e.tipo == ESP_GONGO && e.onda > 0 && !e.usado) b.naArena = true;
        }
    }
    b.numAlvos = 0;

    animarBatalha(jogo, b, ANIM_ENTRADA, 0, 0, 0, 0, false);

    // ---- Apresentação. Monstros pegos dormindo começam a luta dormindo. -------
    std::string apresentacao;
    bool tonto = false;   // algum monstro entrou tonto (levou uma caixa)
    bool surpreendido = false;   // ataque surpresa (modo furtivo)
    for (int i = 0; i < quantos; i++) {
        std::string nome = b.inimigos[i].nome + " (Nv " + std::to_string(participantes[i]->nivel) + ")";
        bool chefe = (tipoDe(*participantes[i]).especial & CHEFE) != 0;
        bool surpresa = jogo.ataqueSurpresa >= 0 && participantes[i] == &andarAtual(jogo).monstros[jogo.ataqueSurpresa];
        if (surpresa && !chefe) {
            // Modo furtivo: o monstro nem viu o jogador chegar.
            b.inimigos[i].status = ADORMECIDO;
            b.inimigos[i].rodadasDormindo = 1;
            nome += ", surpreso";
            surpreendido = true;
        } else if (participantes[i]->atordoadoMs > 0) {
            // Ainda tonto da caixa: perde a primeira rodada (até o chefe).
            b.inimigos[i].status = ADORMECIDO;
            b.inimigos[i].rodadasDormindo = 1;
            participantes[i]->atordoadoMs = 0;
            nome += ", tonto";
            tonto = true;
        } else if (participantes[i]->estado == DORMINDO && !chefe) {
            b.inimigos[i].status = ADORMECIDO;
            b.inimigos[i].rodadasDormindo = sortear(jogo.sorte, 2, 3);
            nome += ", dormindo";
        }
        if (i > 0) apresentacao += (i == quantos - 1) ? " e " : ", ";
        apresentacao += nome;
        participantes[i]->estado = CACANDO;
    }
    jogo.ataqueSurpresa = -1;
    if (quantos == 1 && surpreendido) {
        tocarSom(SOM_CRITICO);
        narrar(jogo, b, "Ataque surpresa! " + comMaiuscula(b.inimigos[0].nome) + " (Nv " +
                        std::to_string(participantes[0]->nivel) + ") nem viu você chegar e perde a primeira rodada!",
               MSG_BOA);
    } else if (quantos == 1 && tonto) {
        bool feminino = tipoDe(*participantes[0]).artigo[0] == 'a';
        narrar(jogo, b, comMaiuscula(b.inimigos[0].nome) + (feminino ? " ainda está tonta" : " ainda está tonto") +
                        " da caixa e perde a primeira rodada!", MSG_BOA);
    } else if (quantos == 1 && b.inimigos[0].status == ADORMECIDO) {
        narrar(jogo, b, comMaiuscula(b.inimigos[0].nome) + " (Nv " + std::to_string(participantes[0]->nivel)
                        + ") está dormindo. Você o pegou desprevenido!");
    } else if (quantos == 1) {
        // O chefe final merece um aviso que não dá para pular.
        narrar(jogo, b, comMaiuscula(apresentacao) + " quer lutar!", contraOChefe ? MSG_PERIGO : MSG_NORMAL);
    } else {
        narrar(jogo, b, "Um grupo ataca: " + apresentacao + "!", MSG_PERIGO);
        if (surpreendido) {
            narrar(jogo, b, "Ataque surpresa! " + comMaiuscula(b.inimigos[0].nome) + " perde a primeira rodada!", MSG_BOA);
        }
    }

    // Aviso do ambiente: algum inimigo está mais forte por causa do tema do andar?
    for (int i = 0; i < quantos; i++) {
        if (bonusDoAmbiente(jogo.andar, b.inimigos[i].elemento) > 100) {
            Elemento el = b.inimigos[i].elemento;
            const char* onde = (el == AGUA) ? "A água da caverna" : (el == TREVAS) ? "A escuridão da cripta" : "O calor da lava";
            narrar(jogo, b, std::string(onde) + " fortalece os monstros de " + NOMES_DOS_ELEMENTOS[b.inimigos[i].elemento] +
                            ": eles batem 20% mais forte aqui!", MSG_PERIGO);
            break;
        }
    }

    dicaNaBatalha(jogo, b, DICA_BATALHA);
    if (contraMiniChefe) dicaNaBatalha(jogo, b, DICA_MINI_CHEFE);   // v1.24

    jogo.rodadasDaUltimaBatalha = 0;
    while (true) {
        jogo.rodadasDaUltimaBatalha++;

        // ---- 1. Escolhas ----------------------------------------------------------
        // Dica do Escudo: contra um grupo ou com a vida pela metade.
        bool temEscudo = false;
        for (int s = 0; s < b.jogador.numGolpes; s++) {
            if (b.jogador.golpes[s] == G_ESCUDO) temEscudo = true;
        }
        if (temEscudo && (inimigosNaLuta(b) >= 2 || *b.jogador.vida * 2 < b.jogador.vidaMaxima)) {
            dicaNaBatalha(jogo, b, DICA_ESCUDO);
        }
        Acao acao = escolherAcaoDoJogador(jogo, b);
        int golpeDe[MAX_INIMIGOS];
        for (int i = 0; i < b.numInimigos; i++) {
            if (b.naLuta[i]) golpeDe[i] = escolherGolpeDoInimigo(jogo, b, i);
        }

        // Fugir e usar item acontecem antes de qualquer golpe.
        if (acao.tipo == ACAO_FUGIR) {
            bool contraOChefe = false;
            for (int i = 0; i < b.numInimigos; i++) {
                if (b.naLuta[i] && b.inimigos[i].ehChefe) contraOChefe = true;
            }
            if (contraOChefe) {
                narrar(jogo, b, "Não há como fugir do Senhor das Profundezas!", MSG_PERIGO);
            } else if (contraUmMiniChefe(b)) {
                narrar(jogo, b, "O portão está fechado: não há como fugir!", MSG_PERIGO);
            } else if (tentarFugir(jogo, b)) {
                tocarSom(SOM_FUGA);
                narrar(jogo, b, "Você fugiu da batalha!");
                return JOGADOR_FUGIU;
            } else {
                tocarSom(SOM_FUGA_FALHOU);
                narrar(jogo, b, "Não conseguiu fugir!");
            }
        }
        if (acao.tipo == ACAO_ITEM && acao.slot == I_BOMBA_DE_FUMACA) {
            // Bomba de fumaça: fuga garantida, e os monstros ficam tossindo na
            // fumaça por uns segundos (dá tempo de sumir do mapa).
            jogo.jogador.mochila[I_BOMBA_DE_FUMACA]--;
            tocarSom(SOM_FUMACA);
            animarBatalha(jogo, b, ANIM_FUMACA, 0, 0, 0, 0, false);
            narrar(jogo, b, "Você joga a Bomba de Fumaça! Ninguém enxerga nada... e você some.", MSG_BOA);
            for (int i = 0; i < b.numInimigos; i++) {
                Monstro& m = *b.monstros[i];
                if (!m.vivo) continue;
                m.atordoadoMs = 4000;
                m.turnosSemBatalha = 10;
            }
            return JOGADOR_FUGIU;
        }
        if (acao.tipo == ACAO_ITEM) {
            Jogador& j = jogo.jogador;
            const TipoItem& item = TIPOS_DE_ITEM[acao.slot];
            j.mochila[acao.slot]--;
            narrar(jogo, b, "Você usa " + nomeComArtigo(acao.slot) + ".");
            tocarSom(item.restauraPP ? SOM_ETER : item.curaStatus && item.curaPorcento == 0 ? SOM_ANTIDOTO : SOM_CURA);
            if (item.curaPorcento > 0) {
                int antes = j.vida;
                j.vida += j.vidaMaxima * item.curaPorcento / 100;
                if (j.vida > j.vidaMaxima) j.vida = j.vidaMaxima;
                animarBatalha(jogo, b, ANIM_CURA, 0, 0, 0, j.vida - antes, false);
                animarVida(jogo, b, 0);
            }
            if (item.curaStatus) {
                b.jogador.status = SEM_STATUS;
                b.jogador.semente = false;
                narrar(jogo, b, "Você se sente bem de novo!");
            }
            if (item.restauraPP) {
                for (int s = 0; s < j.numGolpes; s++) j.pp[s] = GOLPES[j.golpes[s]].ppMaximo;
                narrar(jogo, b, "Seus golpes recuperaram os PP!");
            }
        }

        // Medrosos com pouca vida podem fugir da batalha.
        for (int i = 0; i < b.numInimigos; i++) {
            Lutador& l = b.inimigos[i];
            if (!b.naLuta[i] || !(tipoDe(*b.monstros[i]).especial & MEDROSO)) continue;
            if (*l.vida * 4 <= l.vidaMaxima && l.status != ADORMECIDO && chance(jogo.sorte, 50)) {
                b.naLuta[i] = false;
                b.monstros[i]->estado = FUGINDO;
                b.monstros[i]->turnosSemBatalha = 10;
                narrar(jogo, b, comMaiuscula(l.nome) + " fugiu!");
            }
        }
        if (inimigosNaLuta(b) == 0) return VITORIA;

        // ---- 2. Ordem: prioridade do golpe, depois velocidade ----------------------
        Agente agentes[1 + MAX_INIMIGOS];
        int numAgentes = 0;
        if (acao.tipo == ACAO_GOLPE) {
            agentes[numAgentes++] = { 0, acao.slot, GOLPES[b.jogador.golpes[acao.slot]].prioridade,
                                      velocidadeEfetiva(b.jogador), sortear(jogo.sorte, 0, 999) };
        }
        for (int i = 0; i < b.numInimigos; i++) {
            if (!b.naLuta[i]) continue;
            agentes[numAgentes++] = { i + 1, golpeDe[i], GOLPES[b.inimigos[i].golpes[golpeDe[i]]].prioridade,
                                      velocidadeEfetiva(b.inimigos[i]), sortear(jogo.sorte, 0, 999) };
        }
        ordenarAgentes(agentes, numAgentes);

        // ---- 3. Ações ---------------------------------------------------------------
        // O escudo erguido na rodada passada abaixa (e fica a lembrança, para
        // a chance cair se ele for erguido de novo logo em seguida).
        b.escudoNaRodadaAnterior = b.escudoErguido;
        b.escudoErguido = false;
        for (int a = 0; a < numAgentes; a++) {
            const Agente& ag = agentes[a];
            if (ag.quem == 0) {
                // Se o alvo escolhido já caiu, ataca outro que ainda está na luta.
                int alvo = acao.alvo;
                if (alvo < 0 || !b.naLuta[alvo]) alvo = primeiroNaLuta(b);
                if (alvo >= 0) usarGolpe(jogo, b, b.jogador, b.inimigos[alvo], ag.slot);
            } else {
                int i = ag.quem - 1;
                if (b.naLuta[i]) usarGolpe(jogo, b, b.inimigos[i], b.jogador, ag.slot);
            }
            if (*b.jogador.vida <= 0) {
                animarBatalha(jogo, b, ANIM_DESMAIO, 0, 0, 0, 0, false);
                narrar(jogo, b, "Você desmaiou...", MSG_PERIGO, true);
                return DERROTA;
            }
            if (conferirDesmaios(jogo, b)) return VITORIA;
            conferirFuriaDoChefe(jogo, b);
            conferirMiniChefes(jogo, b, false);
        }

        // ---- 4. Fim da rodada: veneno ---------------------------------------------------
        danoDoVeneno(jogo, b, b.jogador);
        for (int i = 0; i < b.numInimigos; i++) {
            if (b.naLuta[i]) danoDoVeneno(jogo, b, b.inimigos[i]);
        }
        if (*b.jogador.vida <= 0) {
            animarBatalha(jogo, b, ANIM_DESMAIO, 0, 0, 0, 0, false);
                narrar(jogo, b, "Você desmaiou...", MSG_PERIGO, true);
            return DERROTA;
        }
        if (conferirDesmaios(jogo, b)) return VITORIA;
        conferirFuriaDoChefe(jogo, b);
        conferirMiniChefes(jogo, b, true);
    }
}
