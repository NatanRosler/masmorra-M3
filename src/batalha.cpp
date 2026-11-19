// batalha.cpp
#include "batalha.h"
#include "jogo.h"
#include "monstros.h"
#include "tela.h"
#include "terminal.h"
#include "aleatorio.h"
#include "itens.h"
#include "som.h"
#include "dificuldade.h"
#include "loja.h"
#include "tema.h"

// =============================================================================
// TABELAS
// =============================================================================
const char* NOMES_DOS_ELEMENTOS[NUM_ELEMENTOS] = {
    "Normal", "Fogo", "Água", "Planta", "Veneno", "Pedra"
};

const char* NOMES_DOS_STATUS[] = { "", "envenenado", "dormindo", "paralisado" };

// Eficácia em METADES (1 = metade do dano, 2 = normal, 4 = dobro).
// Linha = elemento do golpe; coluna = elemento de quem recebe.
static const int TABELA_DE_EFICACIA[NUM_ELEMENTOS][NUM_ELEMENTOS] = {
    //              Normal Fogo Água Planta Veneno Pedra
    /* Normal */  {   2,    2,   2,    2,     2,     1  },
    /* Fogo   */  {   2,    1,   1,    4,     2,     1  },
    /* Água   */  {   2,    4,   1,    1,     2,     4  },
    /* Planta */  {   2,    1,   4,    1,     1,     4  },
    /* Veneno */  {   2,    2,   2,    4,     1,     1  },
    /* Pedra  */  {   2,    4,   2,    2,     2,     2  },
};

int eficacia(Elemento ataque, Elemento defesa) {
    return TABELA_DE_EFICACIA[ataque][defesa];
}

const Golpe GOLPES[NUM_GOLPES] = {
    //  nome               elemento poder prec  PP  efeito        chance prior.
    { "Golpe",             NORMAL,  40,  100,   0, SEM_EFEITO,      0,  0 },
    { "Escudo",            NORMAL,   0,  100,  10, SOBE_DEFESA,   100,  2 },
    { "Golpe Forte",       NORMAL,  65,   85,  10, SEM_EFEITO,      0,  0 },
    { "Tocha",             FOGO,    55,  100,  12, SEM_EFEITO,      0,  0 },
    { "Jato d'Água",       AGUA,    55,  100,  12, SEM_EFEITO,      0,  0 },
    { "Grito de Guerra",   NORMAL,   0,  100,  10, SOBE_ATAQUE,   100,  0 },
    { "Pó do Sono",        PLANTA,   0,   75,   8, ADORMECER,     100,  0 },
    { "Lâmina Venenosa",   VENENO,  60,  100,  10, ENVENENAR,      30,  0 },
    { "Golpe Duplo",       NORMAL,  25,   90,  10, DOIS_GOLPES,   100,  0 },
    { "Pedrada",           PEDRA,   65,   90,  10, SEM_EFEITO,      0,  0 },
    // Golpes dos monstros (eles não gastam PP)
    { "Mordida",           NORMAL,  45,  100,   0, SEM_EFEITO,      0,  0 },
    { "Rosnado",           NORMAL,   0,  100,   0, BAIXA_ATAQUE,  100,  0 },
    { "Ataque Rápido",     NORMAL,  40,  100,   0, SEM_EFEITO,      0,  1 },
    { "Arranhão",          NORMAL,  40,  100,   0, SEM_EFEITO,      0,  0 },
    { "Folha Navalha",     PLANTA,  65,   95,   0, SEM_EFEITO,      0,  0 },
    { "Língua Grudenta",   NORMAL,  30,  100,   0, PARALISAR,      30,  0 },
    { "Ferrão Venenoso",   VENENO,  25,  100,   0, ENVENENAR,      40,  0 },
    { "Sugar Sangue",      NORMAL,  45,  100,   0, DRENAR,        100,  0 },
    { "Esmagar",           PEDRA,   70,   80,   0, SEM_EFEITO,      0,  0 },
    { "Rugido",            NORMAL,   0,  100,   0, BAIXA_DEFESA,  100,  0 },
    { "Chicote d'Água",    AGUA,    65,   95,   0, SEM_EFEITO,      0,  0 },
    { "Hipnose",           NORMAL,   0,   60,   0, ADORMECER,     100,  0 },
    { "Brasa",             FOGO,    45,  100,   0, SEM_EFEITO,      0,  0 },
    // Golpe inicial de quem escolhe Planta
    { "Chicote de Vinha",  PLANTA,  55,  100,  12, SEM_EFEITO,      0,  0 },
    // Golpes elementais dos monstros (ver golpeElementalDosMonstros)
    { "Investida",         NORMAL,  65,   90,   0, SEM_EFEITO,      0,  0 },
    { "Labareda",          FOGO,    65,   90,   0, SEM_EFEITO,      0,  0 },
    { "Bolha",             AGUA,    45,  100,   0, SEM_EFEITO,      0,  0 },
    { "Espinhos",          PLANTA,  45,  100,   0, SEM_EFEITO,      0,  0 },
    { "Ácido",             VENENO,  65,   90,   0, ENVENENAR,      20,  0 },
    { "Pedregulho",        PEDRA,   45,   95,   0, SEM_EFEITO,      0,  0 },
    // Golpe do chefe final
    { "Golpe Sombrio",     NORMAL,  80,   90,   0, SEM_EFEITO,      0,  0 },
};

// Golpe que o jogador já sabe ao ser criado, conforme o elemento escolhido.
int golpeInicialDoElemento(Elemento e) {
    static const int GOLPE_INICIAL[NUM_ELEMENTOS] = {
        G_GOLPE_FORTE, G_TOCHA, G_JATO_DAGUA, G_CHICOTE_DE_VINHA, G_LAMINA_VENENOSA, G_PEDRADA
    };
    return GOLPE_INICIAL[e];
}

// Os monstros ganham, a cada partida, um elemento sorteado. Um dos golpes da
// espécie vira o golpe deste elemento: o fraco (poder 45) para as espécies
// dos primeiros andares e o forte (poder 65-70) para as dos andares fundos.
int golpeElementalDosMonstros(Elemento e, bool forte) {
    static const int FRACO[NUM_ELEMENTOS] = { G_MORDIDA, G_BRASA, G_BOLHA, G_ESPINHOS, G_FERRAO_VENENOSO, G_PEDREGULHO };
    static const int FORTE[NUM_ELEMENTOS] = { G_INVESTIDA, G_LABAREDA, G_CHICOTE_DAGUA, G_FOLHA_NAVALHA, G_ACIDO, G_ESMAGAR };
    return forte ? FORTE[e] : FRACO[e];
}

// Golpes que o jogador aprende ao chegar em cada nível.
struct GolpeAprendido {
    int nivel;
    int golpe;
};
static const GolpeAprendido APRENDIZADO[] = {
    {  7, G_TOCHA },
    {  9, G_GOLPE_FORTE },
    { 11, G_JATO_DAGUA },
    { 13, G_GRITO_DE_GUERRA },
    { 15, G_PO_DO_SONO },
    { 18, G_LAMINA_VENENOSA },
    { 21, G_GOLPE_DUPLO },
    { 24, G_PEDRADA },
};
static const int NUM_APRENDIZADOS = sizeof(APRENDIZADO) / sizeof(APRENDIZADO[0]);

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
static int calcularDano(Jogo& jogo, const Lutador& atacante, const Lutador& alvo,
                        const Golpe& golpe, bool& critico, int& efic) {
    int ataque = aplicarEstagio(atacante.ataque, atacante.estagioAtaque);
    int defesa = aplicarEstagio(alvo.defesa, alvo.estagioDefesa);
    if (defesa < 1) defesa = 1;

    int dano = ((2 * atacante.nivel / 5 + 2) * golpe.poder * ataque / defesa) / 50 + 2;
    if (golpe.elemento == atacante.elemento) dano = dano * 3 / 2;

    efic = eficacia(golpe.elemento, alvo.elemento);
    dano = dano * efic / 2;

    // Arma elemental: golpes do mesmo elemento da arma causam +25%.
    if (golpe.elemento == atacante.elementoDaArma) dano = dano * 5 / 4;

    // Crítico: 6% de base (como o 1/16 dos Pokémon) + o bônus da Destreza.
    critico = sortear(jogo.sorte, 1, 100) <= 6 + atacante.criticoExtra;
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

// Mostra um texto na caixa e espera o jogador apertar uma tecla.
static void narrar(Jogo& jogo, Batalha& b, const std::string& texto, TipoMensagem tipo = MSG_NORMAL) {
    b.texto = texto;
    b.tipoDoTexto = tipo;
    b.textoDesde = relogioMs();
    b.menu = MENU_NENHUM;
    if (jogo.modoAutomatico) return;
    // Mensagem importante: durante TEMPO_DE_LEITURA_MS as teclas são lidas e
    // jogadas fora. Quem aperta Enter sem parar não pula o aviso sem ver.
    if (tipo != MSG_NORMAL) {
        b.travado = true;
        while (relogioMs() - b.textoDesde < TEMPO_DE_LEITURA_MS) {
            desenharBatalha(jogo, b);
            lerTeclaComEspera(30);
        }
        b.travado = false;
    }
    int tecla;
    do {
        desenharBatalha(jogo, b);
        tecla = lerTecla();
    } while (tecla == TECLA_REDIMENSIONOU);
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
    const int PASSOS = 12;
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
        if (t == TECLA_CIMA || t == 'k')     linha--;
        if (t == TECLA_BAIXO || t == 'j')    linha++;
        if (t == TECLA_ESQUERDA || t == 'h') coluna--;
        if (t == TECLA_DIREITA || t == 'l')  coluna++;
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

static void aplicarStatus(Jogo& jogo, Batalha& b, Lutador& alvo, Status status, bool avisarFalha) {
    std::string nome = comMaiuscula(alvo.nome);
    bool imune = (status == ENVENENADO && alvo.elemento == VENENO);
    if (alvo.status != SEM_STATUS || imune) {
        if (avisarFalha) narrar(jogo, b, "Não teve efeito em " + alvo.nome + ".");
        return;
    }
    alvo.status = status;
    // Status no jogador é perigo; no inimigo, é uma boa notícia.
    TipoMensagem tipo = alvo.ehJogador ? MSG_PERIGO : MSG_BOA;
    if (status == ENVENENADO) narrar(jogo, b, nome + " foi envenenado!", tipo);
    if (status == PARALISADO) narrar(jogo, b, nome + " ficou paralisado!", tipo);
    if (status == ADORMECIDO) {
        alvo.rodadasDormindo = sortear(jogo.sorte, 1, 3);
        narrar(jogo, b, nome + " caiu no sono!", tipo);
    }
}

static void aplicarEfeito(Jogo& jogo, Batalha& b, Lutador& atacante, Lutador& alvo,
                          const Golpe& golpe, bool golpeDeStatus) {
    switch (golpe.efeito) {
        case ENVENENAR:    aplicarStatus(jogo, b, alvo, ENVENENADO, golpeDeStatus); break;
        case ADORMECER:    aplicarStatus(jogo, b, alvo, ADORMECIDO, golpeDeStatus); break;
        case PARALISAR:    aplicarStatus(jogo, b, alvo, PARALISADO, golpeDeStatus); break;
        case SOBE_ATAQUE:  mudarEstagio(jogo, b, atacante, atacante.estagioAtaque, +1, "ataque"); break;
        case SOBE_DEFESA:  mudarEstagio(jogo, b, atacante, atacante.estagioDefesa, +1, "defesa"); break;
        case BAIXA_ATAQUE: mudarEstagio(jogo, b, alvo, alvo.estagioAtaque, -1, "ataque"); break;
        case BAIXA_DEFESA: mudarEstagio(jogo, b, alvo, alvo.estagioDefesa, -1, "defesa"); break;
        default: break;
    }
}

// =============================================================================
// UMA AÇÃO DE GOLPE
// =============================================================================
static int ladoDe(const Lutador& l) { return l.indice; }

static void usarGolpe(Jogo& jogo, Batalha& b, Lutador& atacante, Lutador& alvo, int slot) {
    std::string nome = comMaiuscula(atacante.nome);

    // Status que impedem de agir.
    if (atacante.status == ADORMECIDO) {
        atacante.rodadasDormindo--;
        if (atacante.rodadasDormindo > 0) {
            narrar(jogo, b, nome + " está dormindo...");
            return;
        }
        atacante.status = SEM_STATUS;
        narrar(jogo, b, nome + " acordou!");
    }
    if (atacante.status == PARALISADO && sortear(jogo.sorte, 1, 4) == 1) {
        narrar(jogo, b, nome + " está paralisado e não consegue se mexer!");
        return;
    }

    const Golpe& golpe = GOLPES[atacante.golpes[slot]];
    if (atacante.pp != nullptr && golpe.ppMaximo > 0) atacante.pp[slot]--;
    narrar(jogo, b, nome + " usa " + golpe.nome + "!");

    // Escudo: age antes de todos (prioridade 2) e fica erguido até o fim da
    // rodada. A chance depende do escudo equipado e cai pela metade se ele
    // já estava erguido na rodada anterior (senão dava para bloquear sempre).
    if (atacante.ehJogador && atacante.golpes[slot] == G_ESCUDO) {
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

    // Chance de acertar = precisão do golpe + Destreza de quem ataca - esquiva
    // (Agilidade) de quem defende. Golpes que só mexem em quem usa não erram.
    int chanceDeAcertar = golpe.precisao;
    bool contraOAlvo = !(golpe.poder == 0 && (golpe.efeito == SOBE_ATAQUE || golpe.efeito == SOBE_DEFESA));
    if (contraOAlvo) chanceDeAcertar += atacante.precisaoExtra - alvo.esquiva;
    if (chanceDeAcertar > 100) chanceDeAcertar = 100;
    if (chanceDeAcertar < 10) chanceDeAcertar = 10;
    if (sortear(jogo.sorte, 1, 100) > chanceDeAcertar) {
        tocarSom(SOM_ERROU);
        animarBatalha(jogo, b, ANIM_ERRO, ladoDe(atacante), ladoDe(alvo), golpe.elemento, 0, false);
        if (golpe.precisao >= 100 && contraOAlvo) narrar(jogo, b, comMaiuscula(alvo.nome) + " se esquivou!");
        else narrar(jogo, b, "Mas errou!");
        return;
    }

    // Bloqueio: um ataque com dano contra o jogador pode parar no escudo
    // (erguido com o golpe Escudo, ou só o escudo equipado, com chance menor).
    if (alvo.ehJogador && golpe.poder > 0) {
        int chanceBloquear = b.escudoErguido ? b.chanceDoBloqueio : chanceDeBloqueio(jogo.jogador.escudo, false);
        if (chanceBloquear > 0 && sortear(jogo.sorte, 1, 100) <= chanceBloquear) {
            tocarSom(SOM_BLOQUEIO);
            animarBatalha(jogo, b, ANIM_BLOQUEIO, ladoDe(atacante), ladoDe(alvo), golpe.elemento, 0, false);
            narrar(jogo, b, b.escudoErguido ? "Você bloqueou o ataque com o escudo!"
                                            : "Seu escudo aparou o golpe sozinho!", MSG_BOA);
            return;
        }
    }

    // Golpes de status: só o efeito.
    if (golpe.poder == 0) {
        if (contraOAlvo) animarBatalha(jogo, b, ANIM_GOLPE, ladoDe(atacante), ladoDe(alvo), golpe.elemento, 0, false);
        else animarBatalha(jogo, b, ANIM_EFEITO_PROPRIO, ladoDe(atacante), ladoDe(atacante), golpe.elemento, 0, false);
        aplicarEfeito(jogo, b, atacante, alvo, golpe, true);
        return;
    }

    int vezes = (golpe.efeito == DOIS_GOLPES) ? 2 : 1;
    int danoTotal = 0, efic = 2, acertos = 0;
    for (int v = 0; v < vezes && *alvo.vida > 0; v++) {
        bool critico;
        int dano = calcularDano(jogo, atacante, alvo, golpe, critico, efic);
        *alvo.vida -= dano;
        if (*alvo.vida < 0) *alvo.vida = 0;
        danoTotal += dano;
        acertos++;
        tocarSom(critico ? SOM_CRITICO : (alvo.ehJogador ? SOM_DANO : SOM_GOLPE));
        animarBatalha(jogo, b, ANIM_GOLPE, ladoDe(atacante), ladoDe(alvo), golpe.elemento, dano, critico);
        animarVida(jogo, b, ladoDe(alvo));
        if (critico) narrar(jogo, b, "Um golpe crítico!", atacante.ehJogador ? MSG_BOA : MSG_PERIGO);
    }
    if (efic > 2) {
        tocarSom(SOM_SUPER_EFICAZ);
        narrar(jogo, b, "É super eficaz!", atacante.ehJogador ? MSG_BOA : MSG_PERIGO);
    }
    if (efic < 2) narrar(jogo, b, "Não é muito eficaz...");
    if (vezes > 1) narrar(jogo, b, "Acertou " + std::to_string(acertos) + " vezes!");
    // Vida do jogador abaixo de 25%: aviso (uma vez por batalha).
    if (alvo.ehJogador && !b.avisouVidaBaixa && *alvo.vida > 0 && *alvo.vida * 4 <= alvo.vidaMaxima) {
        b.avisouVidaBaixa = true;
        narrar(jogo, b, "Cuidado: sua vida está baixa! Use uma poção ou fuja.", MSG_PERIGO);
    }

    if (golpe.efeito == DRENAR && *atacante.vida > 0) {
        int antes = *atacante.vida;
        *atacante.vida += danoTotal / 2;
        if (*atacante.vida > atacante.vidaMaxima) *atacante.vida = atacante.vidaMaxima;
        animarBatalha(jogo, b, ANIM_CURA, ladoDe(atacante), ladoDe(atacante), 0, *atacante.vida - antes, false);
        animarVida(jogo, b, ladoDe(atacante));
        narrar(jogo, b, nome + " sugou a vida de " + alvo.nome + "!");
    }

    // Efeito extra (ex.: 30% de chance de envenenar), se o alvo ainda está de pé.
    if (*alvo.vida > 0 && golpe.efeito != SEM_EFEITO && golpe.efeito != DRENAR &&
        golpe.efeito != DOIS_GOLPES && sortear(jogo.sorte, 1, 100) <= golpe.chanceDoEfeito) {
        aplicarEfeito(jogo, b, atacante, alvo, golpe, false);
    }
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

// Golpes que só mexem em quem usa (ex.: Escudo) não precisam de alvo.
static bool precisaDeAlvo(const Golpe& g) {
    return !(g.poder == 0 && (g.efeito == SOBE_ATAQUE || g.efeito == SOBE_DEFESA));
}

// Monta a lista das poções que o jogador tem na mochila.
static void listarItensUsaveis(const Jogador& j, Batalha& b) {
    b.numItensUsaveis = 0;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        if (TIPOS_DE_ITEM[i].categoria == POCAO && j.mochila[i] > 0 && b.numItensUsaveis < 16) {
            b.itensUsaveis[b.numItensUsaveis++] = i;
        }
    }
}

// O item faria alguma coisa agora? Se não, devolve o motivo (senão, "").
static std::string motivoParaNaoUsar(const Jogador& j, const Batalha& b, int tipo) {
    const TipoItem& t = TIPOS_DE_ITEM[tipo];
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
static int valorDoGolpe(const Lutador& atacante, const Lutador& alvo, int slot) {
    const Golpe& g = GOLPES[atacante.golpes[slot]];
    if (atacante.pp != nullptr && g.ppMaximo > 0 && atacante.pp[slot] <= 0) return -1;
    if (g.poder == 0) return 0;
    int v = g.poder * g.precisao * eficacia(g.elemento, alvo.elemento);
    if (g.elemento == atacante.elemento) v = v * 3 / 2;
    if (g.efeito == DOIS_GOLPES) v *= 2;
    if (g.elemento == atacante.elementoDaArma) v = v * 5 / 4;
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
    // Melhor par (golpe, alvo); empatando, o alvo com menos vida.
    int melhorSlot = 0, melhorAlvo = primeiroNaLuta(b), melhorValor = -1;
    for (int i = 0; i < b.numInimigos; i++) {
        if (!b.naLuta[i]) continue;
        for (int s = 0; s < b.jogador.numGolpes; s++) {
            int v = valorDoGolpe(b.jogador, b.inimigos[i], s);
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
            bool causaStatus = (g.efeito == ENVENENAR || g.efeito == ADORMECER || g.efeito == PARALISAR);
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
        narrar(jogo, b, "Você aprendeu " + nomeGolpe + "!", MSG_BOA);
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
        narrar(jogo, b, "Você não aprendeu " + nomeGolpe + ".");
        return;
    }
    std::string antigo = GOLPES[j.golpes[esquecer]].nome;
    j.golpes[esquecer] = golpe;
    j.pp[esquecer] = GOLPES[golpe].ppMaximo;
    narrar(jogo, b, "1, 2 e... pronto! Você esqueceu " + antigo + " e aprendeu " + nomeGolpe + "!", MSG_BOA);
}

static void ganharExperiencia(Jogo& jogo, Batalha& b, int pontos, int moedas) {
    Jogador& j = jogo.jogador;
    j.experiencia += pontos;
    j.moedas += moedas;
    narrar(jogo, b, "Você ganhou " + std::to_string(pontos) + " pontos de experiência e " +
                    std::to_string(moedas) + (moedas == 1 ? " moeda!" : " moedas!"));

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
                        std::to_string(MOEDAS_POR_NIVEL) + " moedas)", MSG_BOA);
        ganharPontosDeAtributo(jogo, PONTOS_POR_NIVEL);
        if (!jogo.modoAutomatico) {
            narrar(jogo, b, "Você ganhou " + std::to_string(PONTOS_POR_NIVEL) +
                            " ponto de atributo! Aperte C no mapa para distribuir.", MSG_BOA);
        }
        b.jogador.precisaoExtra = j.precisaoExtra;
        b.jogador.esquiva = j.esquiva;
        b.jogador.criticoExtra = j.criticoExtra;

        for (int i = 0; i < NUM_APRENDIZADOS; i++) {
            if (APRENDIZADO[i].nivel == j.nivel) aprenderGolpe(jogo, b, APRENDIZADO[i].golpe);
        }
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
    l.numGolpes = j.numGolpes;
    for (int s = 0; s < j.numGolpes; s++) l.golpes[s] = j.golpes[s];
    l.pp = j.pp;                 // ponteiro para o vetor de PP do jogador
    l.elementoDaArma = (j.arma != nullptr) ? j.arma->elemento : -1;
    l.precisaoExtra = j.precisaoExtra;
    l.esquiva = j.esquiva;
    l.criticoExtra = j.criticoExtra;
    l.ehChefe = false;
    l.ehJogador = true;
    l.indice = 0;
    return l;
}

static Lutador montarLutadorDoMonstro(Monstro& m) {
    const TipoMonstro& t = tipoDe(m);
    Lutador l;
    l.nome = std::string(t.artigo) + " " + t.nome;
    l.nivel = m.nivel;
    l.elemento = m.elemento;     // sorteado a cada partida (Etapa 6)
    l.vida = &m.vida;            // ponteiro para a vida do monstro no mapa
    l.vidaMaxima = vidaMaximaDoMonstro(m);
    // A força do ataque dos monstros depende da dificuldade escolhida.
    l.ataque = atributoPeloNivel(t.baseAtaque, m.nivel) * dificuldadeAtual().ataqueDosMonstros / 100;
    l.defesa = atributoPeloNivel(t.baseDefesa, m.nivel);
    l.velocidade = atributoPeloNivel(t.baseVelocidade, m.nivel);
    l.estagioAtaque = 0;
    l.estagioDefesa = 0;
    l.status = SEM_STATUS;
    l.rodadasDormindo = 0;
    golpesDoMonstro(m, l.golpes, l.numGolpes);
    l.pp = nullptr;              // monstros não gastam PP
    l.elementoDaArma = -1;
    l.precisaoExtra = 0;
    l.esquiva = esquivaDoMonstro(m);
    l.criticoExtra = 0;
    l.ehChefe = (t.especial & CHEFE) != 0;
    l.ehJogador = false;
    l.indice = 1;
    return l;
}

// No fim de cada rodada, quem está envenenado perde 1/8 da vida máxima.
static void danoDoVeneno(Jogo& jogo, Batalha& b, Lutador& l) {
    if (l.status != ENVENENADO || *l.vida <= 0) return;
    int dano = l.vidaMaxima / 8;
    if (dano < 1) dano = 1;
    *l.vida -= dano;
    if (*l.vida < 0) *l.vida = 0;
    animarVida(jogo, b, ladoDe(l));
    narrar(jogo, b, comMaiuscula(l.nome) + " sofre com o veneno!", l.ehJogador ? MSG_PERIGO : MSG_NORMAL);
}

// Um inimigo desmaiou? Tira da luta e dá a experiência. Devolve true se
// a batalha acabou (não sobrou inimigo).
static bool conferirDesmaios(Jogo& jogo, Batalha& b) {
    for (int i = 0; i < b.numInimigos; i++) {
        if (!b.naLuta[i] || *b.inimigos[i].vida > 0) continue;
        animarBatalha(jogo, b, ANIM_DESMAIO, i + 1, i + 1, 0, 0, false);
        b.naLuta[i] = false;
        Monstro& m = *b.monstros[i];
        m.vivo = false;   // derrotado: some do mapa
        tocarSom(SOM_DESMAIO);
        narrar(jogo, b, comMaiuscula(b.inimigos[i].nome) + " desmaiou!");
        ganharExperiencia(jogo, b, tipoDe(m).experienciaBase * m.nivel / 6 * dificuldadeAtual().experiencia / 100,
                          moedasDoMonstro(jogo, m.nivel, b.inimigos[i].ehChefe));
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

ResultadoBatalha batalhar(Jogo& jogo, Monstro* participantes[], int quantos) {
    if (quantos > MAX_INIMIGOS) quantos = MAX_INIMIGOS;
    Batalha b;
    b.jogador = montarLutadorDoJogador(jogo.jogador);
    b.vidaMostrada[0] = *b.jogador.vida;
    b.numInimigos = quantos;
    for (int i = 0; i < quantos; i++) {
        b.monstros[i] = participantes[i];
        b.inimigos[i] = montarLutadorDoMonstro(*participantes[i]);
        // O ambiente fortalece um elemento (Água na caverna, Fogo no vulcão).
        b.inimigos[i].ataque = b.inimigos[i].ataque * bonusDoAmbiente(jogo.andar, b.inimigos[i].elemento) / 100;
        b.inimigos[i].indice = i + 1;
        b.naLuta[i] = true;
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
    tocarMusica(contraOChefe ? MUSICA_CHEFE : MUSICA_BATALHA);
    b.tentativasDeFuga = 0;
    b.tipoDoTexto = MSG_NORMAL;
    b.textoDesde = 0;
    b.travado = false;
    b.avisouVidaBaixa = false;
    b.menu = MENU_NENHUM;
    b.cursor = 0;
    b.golpeNovo = -1;
    b.numItensUsaveis = 0;
    b.escudoErguido = false;
    b.escudoNaRodadaAnterior = false;
    b.chanceDoBloqueio = 0;
    b.numAlvos = 0;

    animarBatalha(jogo, b, ANIM_ENTRADA, 0, 0, 0, 0, false);

    // ---- Apresentação. Monstros pegos dormindo começam a luta dormindo. -------
    std::string apresentacao;
    bool tonto = false;   // algum monstro entrou tonto (levou uma caixa)
    for (int i = 0; i < quantos; i++) {
        std::string nome = b.inimigos[i].nome + " (Nv " + std::to_string(participantes[i]->nivel) + ")";
        bool chefe = (tipoDe(*participantes[i]).especial & CHEFE) != 0;
        if (participantes[i]->atordoadoMs > 0) {
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
    if (quantos == 1 && tonto) {
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
    }

    // Aviso do ambiente: algum inimigo está mais forte por causa do tema do andar?
    for (int i = 0; i < quantos; i++) {
        if (bonusDoAmbiente(jogo.andar, b.inimigos[i].elemento) > 100) {
            const char* onde = (b.inimigos[i].elemento == AGUA) ? "A água da caverna" : "O calor da lava";
            narrar(jogo, b, std::string(onde) + " fortalece os monstros de " + NOMES_DOS_ELEMENTOS[b.inimigos[i].elemento] +
                            ": eles batem 20% mais forte aqui!", MSG_PERIGO);
            break;
        }
    }

    jogo.rodadasDaUltimaBatalha = 0;
    while (true) {
        jogo.rodadasDaUltimaBatalha++;

        // ---- 1. Escolhas ----------------------------------------------------------
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
            } else if (tentarFugir(jogo, b)) {
                narrar(jogo, b, "Você fugiu da batalha!");
                return JOGADOR_FUGIU;
            } else {
                narrar(jogo, b, "Não conseguiu fugir!");
            }
        }
        if (acao.tipo == ACAO_ITEM) {
            Jogador& j = jogo.jogador;
            const TipoItem& item = TIPOS_DE_ITEM[acao.slot];
            j.mochila[acao.slot]--;
            narrar(jogo, b, "Você usa " + nomeComArtigo(acao.slot) + ".");
            tocarSom(SOM_CURA);
            if (item.curaPorcento > 0) {
                int antes = j.vida;
                j.vida += j.vidaMaxima * item.curaPorcento / 100;
                if (j.vida > j.vidaMaxima) j.vida = j.vidaMaxima;
                animarBatalha(jogo, b, ANIM_CURA, 0, 0, 0, j.vida - antes, false);
                animarVida(jogo, b, 0);
            }
            if (item.curaStatus) {
                b.jogador.status = SEM_STATUS;
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
                narrar(jogo, b, "Você desmaiou...", MSG_PERIGO);
                return DERROTA;
            }
            if (conferirDesmaios(jogo, b)) return VITORIA;
        }

        // ---- 4. Fim da rodada: veneno ---------------------------------------------------
        danoDoVeneno(jogo, b, b.jogador);
        for (int i = 0; i < b.numInimigos; i++) {
            if (b.naLuta[i]) danoDoVeneno(jogo, b, b.inimigos[i]);
        }
        if (*b.jogador.vida <= 0) {
            animarBatalha(jogo, b, ANIM_DESMAIO, 0, 0, 0, 0, false);
                narrar(jogo, b, "Você desmaiou...", MSG_PERIGO);
            return DERROTA;
        }
        if (conferirDesmaios(jogo, b)) return VITORIA;
    }
}
