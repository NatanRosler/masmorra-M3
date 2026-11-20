// salvar.cpp
// -----------------------------------------------------------------------------
// Salvar e continuar a partida (ver salvar.h).
// -----------------------------------------------------------------------------
#include "salvar.h"
#include "jogo.h"
#include "dificuldade.h"
#include "som.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <type_traits>

const char* ARQUIVO_DO_SAVE = "save.dat";
static const char* ARQUIVO_TEMPORARIO = "save.tmp";

// Muda quando o formato do arquivo mudar.
// Versão 2 (v1.13): salas especiais.
// Versão 3 (v1.17): campeões, gnomo vigia e sombra (2 espécies novas antes do
// mímico), itens de suporte (4 itens novos no fim da lista), Mochila de Couro
// e Lanterna no jogador, estoque novo do Garro. Saves das versões 1 e 2
// continuam sendo lidos (ver "SAVES ANTIGOS" mais abaixo).
// Versão 4 (v1.18): no fim, as espécies já vistas na partida (bestiário).
// Versão 5 (v1.20): no fim, quantas vezes o chefe já apareceu (cena dele).
static const int VERSAO_DO_SAVE = 5;
static const char ASSINATURA[8] = { 'M', 'A', 'S', 'M', 'O', 'R', 'R', 'A' };

// =============================================================================
// GRAVAR E LER VALORES SIMPLES
// =============================================================================
// "Bruto" = copia os bytes do valor como estão na memória. Só vale para tipos
// sem ponteiros nem std::string dentro (números, enums, vetores fixos e
// structs só com isso). O static_assert garante isso na compilação.
struct Gravador {
    std::ofstream arquivo;
    template <typename T> void bruto(const T& valor) {
        static_assert(std::is_trivially_copyable<T>::value, "só tipos simples");
        arquivo.write(reinterpret_cast<const char*>(&valor), sizeof(T));
    }
    void texto(const std::string& s) {
        int tamanho = (int)s.size();
        bruto(tamanho);
        arquivo.write(s.data(), tamanho);
    }
};

struct Leitor {
    std::ifstream arquivo;
    bool ok = true;
    template <typename T> void bruto(T& valor) {
        static_assert(std::is_trivially_copyable<T>::value, "só tipos simples");
        if (ok && !arquivo.read(reinterpret_cast<char*>(&valor), sizeof(T))) ok = false;
    }
    void texto(std::string& s) {
        int tamanho = 0;
        bruto(tamanho);
        if (!ok || tamanho < 0 || tamanho > 100000) { ok = false; return; }
        s.assign((size_t)tamanho, ' ');
        if (tamanho > 0 && !arquivo.read(&s[0], tamanho)) ok = false;
    }
    // Lê um número e confere se ele está no intervalo esperado.
    int numero(int minimo, int maximo) {
        int n = 0;
        bruto(n);
        if (n < minimo || n > maximo) ok = false;
        return n;
    }
};

// =============================================================================
// SAVES ANTIGOS (versões 1 e 2)
// =============================================================================
// Como as structs eram antes da v1.17. Os campos novos foram postos no FIM
// das structs, então a parte de cima é igual: o save antigo é lido nessas
// structs e copiado para o começo das novas (memcpy), e o resto recebe o
// valor padrão.
struct MonstroAntigo {
    bool vivo; int tipo; int x, y; int nivel; int vida; Elemento elemento; int atordoadoMs;
    EstadoMonstro estado; int destinoX, destinoY; int turnosSemVerJogador; int turnosSemBatalha;
    bool carregaChave; int tempoAcumulado;
};
struct NpcAntigo {
    int tipo; int x, y; bool conversou; int estoque[2]; int pocoesAVenda[4];
};
const int ITENS_ANTIGOS = 26;     // I_CHAVE + 1: os itens de suporte vieram depois
const int ESPECIES_ANTIGAS = 9;   // do rato à naga: depois vinham o mímico e o chefe

// Tamanho das estruturas gravadas "brutas": se o jogo mudar, o save antigo
// é recusado em vez de ser lido errado.
static void tamanhos(int t[12], int versao) {
    bool antigo = versao < 3;
    t[0] = antigo ? (int)sizeof(MonstroAntigo) : (int)sizeof(Monstro);
    t[1] = (int)sizeof(ItemNoChao);
    t[2] = (int)sizeof(Armadilha);
    t[3] = antigo ? (int)sizeof(NpcAntigo) : (int)sizeof(Npc);
    t[4] = (int)sizeof(Mapa);          t[5] = (int)sizeof(Escadas);
    t[6] = (int)sizeof(SalaDoTesouro); t[7] = (int)sizeof(SalaDaLoja);
    t[8] = MAPA_LARGURA;               t[9] = MAPA_ALTURA;
    t[10] = antigo ? ITENS_ANTIGOS : NUM_TIPOS_DE_ITEM;
    t[11] = NUM_ANDARES;
}

static void lerMonstro(Leitor& l, Monstro& m, int versao) {
    if (versao >= 3) { l.bruto(m); return; }
    MonstroAntigo antigo;
    l.bruto(antigo);
    static_assert(sizeof(MonstroAntigo) <= sizeof(Monstro), "a struct nova só cresce");
    std::memcpy(&m, &antigo, sizeof(antigo));
    m.campeao = false;
    m.cornetas = 0;
}

static void lerNpc(Leitor& l, Npc& n, int versao) {
    if (versao >= 3) { l.bruto(n); return; }
    NpcAntigo antigo;
    l.bruto(antigo);
    static_assert(sizeof(NpcAntigo) <= sizeof(Npc), "a struct nova só cresce");
    std::memcpy(&n, &antigo, sizeof(antigo));
    n.estoqueExtra = -1;
    n.extrasAVenda[0] = n.extrasAVenda[1] = 0;
}

// =============================================================================
// CADA PARTE DO JOGO
// =============================================================================
static void gravarJogador(Gravador& g, const Jogador& j) {
    g.texto(j.nome);
    g.bruto(j.elemento);
    g.bruto(j.cor);
    g.bruto(j.atributos);
    g.bruto(j.pontosLivres);
    g.bruto(j.x);
    g.bruto(j.y);
    g.bruto(j.nivel);
    g.bruto(j.experiencia);
    g.bruto(j.vida);
    g.bruto(j.golpes);
    g.bruto(j.pp);
    g.bruto(j.numGolpes);
    g.bruto(j.mochila);
    // Ponteiros não podem ir para o arquivo: grava o número do item (-1 = nada).
    int arma = (j.arma != nullptr) ? (int)(j.arma - TIPOS_DE_ITEM) : -1;
    int escudo = (j.escudo != nullptr) ? (int)(j.escudo - TIPOS_DE_ITEM) : -1;
    g.bruto(arma);
    g.bruto(escudo);
    g.bruto(j.bonusVida);
    g.bruto(j.bonusAtaque);
    g.bruto(j.bonusDefesa);
    g.bruto(j.bonusVelocidade);
    g.bruto(j.vidas);
    g.bruto(j.moedas);
    g.bruto(j.mochilaDeCouro);
    g.bruto(j.lanterna);
}

static void lerJogador(Leitor& l, Jogador& j, int versao) {
    l.texto(j.nome);
    l.bruto(j.elemento);
    l.bruto(j.cor);
    l.bruto(j.atributos);
    l.bruto(j.pontosLivres);
    l.bruto(j.x);
    l.bruto(j.y);
    l.bruto(j.nivel);
    l.bruto(j.experiencia);
    l.bruto(j.vida);
    l.bruto(j.golpes);
    l.bruto(j.pp);
    l.bruto(j.numGolpes);
    if (versao >= 3) {
        l.bruto(j.mochila);
    } else {
        int antiga[ITENS_ANTIGOS];
        l.bruto(antiga);
        for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) j.mochila[i] = (i < ITENS_ANTIGOS) ? antiga[i] : 0;
    }
    int arma = l.numero(-1, NUM_TIPOS_DE_ITEM - 1);
    int escudo = l.numero(-1, NUM_TIPOS_DE_ITEM - 1);
    j.arma = (arma >= 0) ? &TIPOS_DE_ITEM[arma] : nullptr;
    j.escudo = (escudo >= 0) ? &TIPOS_DE_ITEM[escudo] : nullptr;
    l.bruto(j.bonusVida);
    l.bruto(j.bonusAtaque);
    l.bruto(j.bonusDefesa);
    l.bruto(j.bonusVelocidade);
    l.bruto(j.vidas);
    l.bruto(j.moedas);
    j.mochilaDeCouro = j.lanterna = false;
    if (versao >= 3) {
        l.bruto(j.mochilaDeCouro);
        l.bruto(j.lanterna);
    }
    // Vida máxima, ataque, defesa, precisão... são calculados de novo a partir
    // do nível, dos atributos, do equipamento e dos bônus.
    int vida = j.vida;
    recalcularAtributos(j);
    // (Se a janela foi fechada no instante em que a vida zerou, volta com 1.)
    j.vida = (vida < 1) ? 1 : (vida > j.vidaMaxima ? j.vidaMaxima : vida);
}

// Andares nunca visitados não são gravados: eles serão gerados quando o
// jogador chegar lá (a semente da partida garante que sairão iguais).
static void gravarAndar(Gravador& g, const Andar& a) {
    g.bruto(a.gerado);
    if (!a.gerado) return;
    g.bruto(a.mapa);
    g.bruto(a.escadas);
    g.bruto(a.descoberto);
    g.bruto(a.numMonstros);
    for (int i = 0; i < a.numMonstros; i++) g.bruto(a.monstros[i]);
    g.bruto(a.numItens);
    for (int i = 0; i < a.numItens; i++) g.bruto(a.itens[i]);
    g.bruto(a.tesouro);
    g.bruto(a.chavesGuardadas);
    g.bruto(a.loja);
    g.bruto(a.zonaSegura);
    g.bruto(a.numArmadilhas);
    for (int i = 0; i < a.numArmadilhas; i++) g.bruto(a.armadilhas[i]);
    g.bruto(a.conteudoDasCaixas);
    g.bruto(a.numNpcs);
    for (int i = 0; i < a.numNpcs; i++) g.bruto(a.npcs[i]);
    g.bruto(a.cheiro);
    g.bruto(a.numEspeciais);
    for (int i = 0; i < a.numEspeciais; i++) g.bruto(a.especiais[i]);
}

static void lerAndar(Leitor& l, Andar& a, int versao) {
    a.numEspeciais = 0;
    l.bruto(a.gerado);
    if (!l.ok || !a.gerado) return;
    l.bruto(a.mapa);
    l.bruto(a.escadas);
    l.bruto(a.descoberto);
    a.numMonstros = l.numero(0, MAX_MONSTROS);
    for (int i = 0; i < a.numMonstros && l.ok; i++) lerMonstro(l, a.monstros[i], versao);
    a.numItens = l.numero(0, MAX_ITENS_NO_CHAO);
    for (int i = 0; i < a.numItens && l.ok; i++) l.bruto(a.itens[i]);
    l.bruto(a.tesouro);
    l.bruto(a.chavesGuardadas);
    l.bruto(a.loja);
    l.bruto(a.zonaSegura);
    a.numArmadilhas = l.numero(0, MAX_ARMADILHAS);
    for (int i = 0; i < a.numArmadilhas && l.ok; i++) l.bruto(a.armadilhas[i]);
    l.bruto(a.conteudoDasCaixas);
    a.numNpcs = l.numero(0, MAX_NPCS);
    for (int i = 0; i < a.numNpcs && l.ok; i++) lerNpc(l, a.npcs[i], versao);
    l.bruto(a.cheiro);
    if (versao >= 2) {
        a.numEspeciais = l.numero(0, MAX_ESPECIAIS);
        for (int i = 0; i < a.numEspeciais && l.ok; i++) l.bruto(a.especiais[i]);
    }
}

// =============================================================================
// SALVAR
// =============================================================================
bool salvarPartida(const Jogo& jogo) {
    if (jogo.modoAutomatico || jogo.morto || jogo.venceu) return false;
    Gravador g;
    // Grava num arquivo temporário e só depois troca pelo save de verdade:
    // se o computador desligar no meio, o save antigo continua inteiro.
    g.arquivo.open(ARQUIVO_TEMPORARIO, std::ios::binary | std::ios::trunc);
    if (!g.arquivo) return false;

    // Cabeçalho (também usado pelo "Continuar" do menu para mostrar o resumo).
    g.arquivo.write(ASSINATURA, sizeof(ASSINATURA));
    g.bruto(VERSAO_DO_SAVE);
    int t[12];
    tamanhos(t, VERSAO_DO_SAVE);
    g.bruto(t);
    g.texto(jogo.jogador.nome);
    g.bruto(jogo.andar);
    g.bruto(jogo.jogador.nivel);
    int dificuldade = nivelDaDificuldade();
    g.bruto(dificuldade);

    // O resto da partida.
    g.bruto(jogo.semente);
    g.bruto(jogo.sorte);
    g.bruto(jogo.elementoDaEspecie);
    for (int i = 0; i < NUM_ANDARES; i++) gravarAndar(g, jogo.andares[i]);
    gravarJogador(g, jogo.jogador);
    g.bruto(jogo.tempoMs);
    g.bruto(jogo.passos);
    g.bruto(jogo.monstrosMortos);
    g.bruto(jogo.itensColetados);
    g.bruto(jogo.andarMaisFundo);
    int mensagens = (int)jogo.historico.size();
    g.bruto(mensagens);
    for (int i = 0; i < mensagens; i++) {
        g.texto(jogo.historico[i]);
        int tipo = (i < (int)jogo.tiposDoHistorico.size()) ? jogo.tiposDoHistorico[i] : MSG_NORMAL;
        g.bruto(tipo);
    }
    g.bruto(jogo.especiesVistasNaPartida);             // versão 4 (bestiário)
    g.bruto(jogo.encontrosComOChefe);                  // versão 5 (cena do chefe)
    g.arquivo.write(ASSINATURA, sizeof(ASSINATURA));   // marca de fim: o arquivo está completo
    g.arquivo.close();
    if (!g.arquivo) return false;

    std::remove(ARQUIVO_DO_SAVE);   // no Windows, rename não substitui um arquivo existente
    return std::rename(ARQUIVO_TEMPORARIO, ARQUIVO_DO_SAVE) == 0;
}

// =============================================================================
// CARREGAR
// =============================================================================
// Lê e confere o cabeçalho. Devolve false se não for um save desta versão.
static bool lerCabecalho(Leitor& l, std::string& nome, int& andar, int& nivel, int& dificuldade, int& versao) {
    char assinatura[8];
    if (!l.arquivo.read(assinatura, sizeof(assinatura))) return false;
    for (int i = 0; i < 8; i++) if (assinatura[i] != ASSINATURA[i]) return false;
    versao = 0;
    l.bruto(versao);
    if (!l.ok || versao < 1 || versao > VERSAO_DO_SAVE) return false;
    int lidos[12], esperados[12];
    l.bruto(lidos);
    tamanhos(esperados, versao);
    for (int i = 0; i < 12; i++) if (lidos[i] != esperados[i]) return false;
    l.texto(nome);
    andar = l.numero(1, NUM_ANDARES);
    nivel = l.numero(1, NIVEL_MAXIMO);
    dificuldade = l.numero(0, NUM_DIFICULDADES - 1);
    return l.ok;
}

bool existePartidaSalva(std::string* resumo) {
    Leitor l;
    l.arquivo.open(ARQUIVO_DO_SAVE, std::ios::binary);
    if (!l.arquivo) return false;
    std::string nome;
    int andar, nivel, dificuldade, versao;
    if (!lerCabecalho(l, nome, andar, nivel, dificuldade, versao)) return false;
    if (resumo) {
        *resumo = nome + "  ·  Andar " + std::to_string(andar) + "  ·  Nível " + std::to_string(nivel) +
                  "  ·  " + DIFICULDADES[dificuldade].nome;
    }
    return true;
}

bool carregarPartida(Jogo& jogo) {
    Leitor l;
    l.arquivo.open(ARQUIVO_DO_SAVE, std::ios::binary);
    if (!l.arquivo) return false;
    std::string nome;
    int andar, nivel, dificuldade, versao;
    if (!lerCabecalho(l, nome, andar, nivel, dificuldade, versao)) return false;

    definirDificuldade(dificuldade);
    jogo.andar = andar;
    l.bruto(jogo.semente);
    l.bruto(jogo.sorte);
    l.bruto(jogo.elementoDaEspecie);
    for (int i = 0; i < NUM_ANDARES && l.ok; i++) lerAndar(l, jogo.andares[i], versao);
    if (l.ok) lerJogador(l, jogo.jogador, versao);
    l.bruto(jogo.tempoMs);
    l.bruto(jogo.passos);
    l.bruto(jogo.monstrosMortos);
    l.bruto(jogo.itensColetados);
    l.bruto(jogo.andarMaisFundo);
    int mensagens = l.numero(0, 10000);
    jogo.historico.clear();
    jogo.tiposDoHistorico.clear();
    for (int i = 0; i < mensagens && l.ok; i++) {
        std::string texto;
        int tipo = MSG_NORMAL;
        l.texto(texto);
        l.bruto(tipo);
        jogo.historico.push_back(texto);
        jogo.tiposDoHistorico.push_back(tipo);
    }
    jogo.especiesVistasNaPartida = 0;
    if (versao >= 4) l.bruto(jogo.especiesVistasNaPartida);
    jogo.encontrosComOChefe = 0;
    jogo.chefeAVista = false;
    if (versao >= 5) l.bruto(jogo.encontrosComOChefe);
    char fim[8];
    if (!l.ok || !l.arquivo.read(fim, sizeof(fim))) return false;
    for (int i = 0; i < 8; i++) if (fim[i] != ASSINATURA[i]) return false;
    if (!jogo.andares[jogo.andar - 1].gerado) return false;
    if (versao < 3) {
        // As espécies mudaram de número: na versão 1 a tabela acabava no
        // chefe (índice 9); na 2, vinham o mímico (9) e o chefe (10); na 3,
        // o gnomo e a sombra entraram antes do mímico.
        int novoNumero[MAX_ESPECIES];
        for (int i = 0; i < MAX_ESPECIES; i++) novoNumero[i] = i;
        if (versao == 1) {
            novoNumero[ESPECIES_ANTIGAS] = tipoDoChefe();
        } else {
            novoNumero[ESPECIES_ANTIGAS] = tipoDoMimico();
            novoNumero[ESPECIES_ANTIGAS + 1] = tipoDoChefe();
        }
        Elemento antigos[MAX_ESPECIES];
        for (int i = 0; i < MAX_ESPECIES; i++) antigos[i] = jogo.elementoDaEspecie[i];
        // Espécies novas: um elemento tirado da semente da partida.
        for (int i = ESPECIES_ANTIGAS; i < MAX_ESPECIES; i++) {
            jogo.elementoDaEspecie[i] = (Elemento)((jogo.semente / (i + 1)) % NUM_ELEMENTOS);
        }
        int especiesNoSave = (versao == 1) ? ESPECIES_ANTIGAS + 1 : ESPECIES_ANTIGAS + 2;
        for (int i = ESPECIES_ANTIGAS; i < especiesNoSave; i++) jogo.elementoDaEspecie[novoNumero[i]] = antigos[i];
        if (versao == 1) jogo.elementoDaEspecie[tipoDoMimico()] = NORMAL;
        for (int i = 0; i < NUM_ANDARES; i++) {
            Andar& a = jogo.andares[i];
            if (!a.gerado) continue;
            for (int k = 0; k < a.numMonstros; k++) {
                int tipo = a.monstros[k].tipo;
                if (tipo >= 0 && tipo < MAX_ESPECIES) a.monstros[k].tipo = novoNumero[tipo];
            }
        }
    }

    // O que não vai para o arquivo volta ao normal.
    jogo.comMonstros = true;
    jogo.modoAutomatico = false;
    jogo.batalhaPendente = -1;
    jogo.esperaDoJogadorMs = 0;
    jogo.descansando = false;
    jogo.furtivo = false;
    jogo.furtivoLigado = false;
    jogo.ataqueSurpresa = -1;
    jogo.msDeDescanso = 0;
    jogo.segundosDeDescanso = 0;
    jogo.rodadasDaUltimaBatalha = 0;
    jogo.morto = false;
    jogo.venceu = false;
    jogo.causaDaMorte = "";
    jogo.rodando = true;
    jogo.rankingRegistrado = false;
    jogo.pontuacaoFinal = 0;
    jogo.posicaoNoRanking = -1;
    jogo.mensagensDoTurno.clear();
    jogo.tipoDoTurno = MSG_NORMAL;
    jogo.faixa = "";
    jogo.faixaAte = 0;
    atualizarVisao(jogo);
    return true;
}

void apagarPartidaSalva() {
    std::remove(ARQUIVO_DO_SAVE);
}

bool continuarPartida(Jogo& jogo) {
    if (!carregarPartida(jogo)) return false;
    adicionarMensagem(jogo, "Bem-vindo de volta, " + jogo.jogador.nome + "! A partida continua no andar " +
                            std::to_string(jogo.andar) + ".", MSG_BOA);
    tocarMusica(MUSICA_MASMORRA);
    return true;
}

// =============================================================================
// SALVAR AO FECHAR A JANELA
// =============================================================================
static Jogo* emAndamento = nullptr;

void definirPartidaEmAndamento(Jogo* jogo) {
    emAndamento = jogo;
}

void salvarPartidaEmAndamento() {
    if (emAndamento != nullptr) salvarPartida(*emAndamento);
}
