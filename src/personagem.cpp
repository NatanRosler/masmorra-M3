// personagem.cpp
// -----------------------------------------------------------------------------
// Etapa 6: criação do personagem (nome, elemento, cor e atributos) e a ficha
// onde o jogador distribui os pontos que ganha a cada nível.
// -----------------------------------------------------------------------------
#include "jogo.h"
#include "tela.h"
#include "terminal.h"
#include "pontuacao.h"
#include "som.h"
#include "aleatorio.h"

const char* NOMES_DOS_ATRIBUTOS[NUM_ATRIBUTOS] = {
    "Força", "Constituição", "Destreza", "Agilidade"
};

const char* EFEITOS_DOS_ATRIBUTOS[NUM_ATRIBUTOS] = {
    "+1 de ataque por ponto: seus golpes causam mais dano.",
    "+3 de vida máxima por ponto e +1 de armadura (defesa) a cada 2 pontos.",
    "+3% de precisão e +1% de chance de golpe crítico por ponto.",
    "+2% de esquiva e +1 de velocidade por ponto: anda e age mais rápido.",
};

const char* NOMES_DAS_CORES_DO_JOGADOR[NUM_CORES_DO_JOGADOR] = {
    "Azul", "Vermelho", "Verde", "Roxo", "Dourado", "Branco"
};

const CorRGB CORES_DO_JOGADOR[NUM_CORES_DO_JOGADOR] = {
    {  95, 170, 202 },
    { 215,  80,  80 },
    {  90, 185, 100 },
    { 155, 105, 205 },
    { 235, 190,  80 },
    { 225, 225, 230 },
};

// -----------------------------------------------------------------------------
// CRIAÇÃO
// -----------------------------------------------------------------------------
void criacaoPadrao(Criacao& c) {
    c.nome = "";
    c.nomeSorteado = false;
    c.elemento = NORMAL;
    c.cor = 0;
    c.dificuldade = nivelDaDificuldade();   // lembra a da última partida
    for (int a = 0; a < NUM_ATRIBUTOS; a++) c.atributos[a] = 0;
    c.pontosLivres = PONTOS_INICIAIS;
    c.linha = LINHA_NOME;
}

// Muda um atributo em +1 ou -1, respeitando os pontos disponíveis.
static void mudarAtributo(Criacao& c, int atributo, int mudanca) {
    if (mudanca > 0 && c.pontosLivres > 0) {
        c.atributos[atributo]++;
        c.pontosLivres--;
    } else if (mudanca < 0 && c.atributos[atributo] > 0) {
        c.atributos[atributo]--;
        c.pontosLivres++;
    }
}

// Nomes do botão Aleatório (inventados para o jogo).
static const char* NOMES_ALEATORIOS[] = {
    "Arin", "Tessa", "Brom", "Lira", "Kael", "Mira", "Doran", "Sela", "Tobias", "Yara",
    "Iko", "Nara", "Vale", "Ravi", "Odete", "Bento", "Cora", "Elias", "Fen", "Gaia",
    "Hugo", "Ivo", "Juno", "Lia", "Nilo", "Otto", "Pia", "Rui", "Saga", "Teo",
};
static const int NUM_NOMES_ALEATORIOS = sizeof(NOMES_ALEATORIOS) / sizeof(NOMES_ALEATORIOS[0]);

// Botão Aleatório: sorteia nome, elemento, cor e os 5 pontos de atributo.
// A dificuldade não muda (é uma escolha de quanto desafio o jogador quer), e
// um nome digitado pelo jogador também fica.
static void sortearPersonagem(Criacao& c) {
    static GeradorAleatorio sorte;
    static bool iniciado = false;
    if (!iniciado) {
        iniciarGerador(sorte, sementeDoRelogio());
        iniciado = true;
    }
    if (c.nome.empty() || c.nomeSorteado) {
        c.nome = NOMES_ALEATORIOS[sortear(sorte, 0, NUM_NOMES_ALEATORIOS - 1)];
        c.nomeSorteado = true;
    }
    c.elemento = sortear(sorte, 0, NUM_ELEMENTOS - 1);
    c.cor = sortear(sorte, 0, NUM_CORES_DO_JOGADOR - 1);
    for (int a = 0; a < NUM_ATRIBUTOS; a++) c.atributos[a] = 0;
    for (int p = 0; p < PONTOS_INICIAIS; p++) c.atributos[sortear(sorte, 0, NUM_ATRIBUTOS - 1)]++;
    c.pontosLivres = 0;
}

bool criarPersonagem(Criacao& c) {
    while (true) {
        // Na linha do nome, as letras viram texto (W A S D não andam).
        definirModoTexto(c.linha == LINHA_NOME);
        desenharCriacao(c);
        int t = lerTecla();
        if (t == TECLA_REDIMENSIONOU) continue;
        if (t == TECLA_ESC) {
            definirModoTexto(false);
            return false;
        }
        if (t >= TECLA_CLIQUE_MENU && t < TECLA_CLIQUE_MENU + NUM_LINHAS_DA_CRIACAO) {
            c.linha = t - TECLA_CLIQUE_MENU;
            if (c.linha != LINHA_COMECAR && c.linha != LINHA_ALEATORIO) continue;
            t = TECLA_ENTER;   // clicou num dos botões
        }
        if (t == TECLA_CIMA) {
            c.linha = (c.linha + NUM_LINHAS_DA_CRIACAO - 1) % NUM_LINHAS_DA_CRIACAO;
            continue;
        }
        // Aleatório: Enter (ou clique) sorteia de novo; o cursor fica no botão.
        if (t == TECLA_ENTER && c.linha == LINHA_ALEATORIO) {
            sortearPersonagem(c);
            tocarSom(SOM_ITEM);
            continue;
        }
        // Os dois botões ficam lado a lado: esquerda/direita troca entre eles.
        if (c.linha == LINHA_ALEATORIO && t == TECLA_DIREITA) { c.linha = LINHA_COMECAR; continue; }
        if (c.linha == LINHA_COMECAR && t == TECLA_ESQUERDA) { c.linha = LINHA_ALEATORIO; continue; }
        if (t == TECLA_BAIXO || (t == TECLA_ENTER && c.linha != LINHA_COMECAR)) {
            c.linha = (c.linha + 1) % NUM_LINHAS_DA_CRIACAO;
            continue;
        }
        if (t == TECLA_ENTER) {   // linha "Começar"
            if (c.nome.empty()) c.nome = "Aventureiro";
            definirModoTexto(false);
            return true;
        }

        int mudanca = 0;
        if (t == TECLA_ESQUERDA || t == '-') mudanca = -1;
        if (t == TECLA_DIREITA || t == '+' || t == '=') mudanca = +1;

        switch (c.linha) {
            case LINHA_NOME:
                if ((t == 8 || t == 127) && !c.nome.empty()) {
                    c.nome.pop_back();
                    c.nomeSorteado = false;
                } else if (t >= 32 && t <= 126 && t != ';' &&
                           (int)c.nome.size() < TAMANHO_MAXIMO_DO_NOME) {
                    c.nome += (char)t;
                    c.nomeSorteado = false;
                }
                break;
            case LINHA_ELEMENTO:
                if (mudanca != 0) c.elemento = (c.elemento + mudanca + NUM_ELEMENTOS) % NUM_ELEMENTOS;
                break;
            case LINHA_COR:
                if (mudanca != 0) c.cor = (c.cor + mudanca + NUM_CORES_DO_JOGADOR) % NUM_CORES_DO_JOGADOR;
                break;
            case LINHA_DIFICULDADE:
                if (mudanca != 0) c.dificuldade = (c.dificuldade + mudanca + NUM_DIFICULDADES) % NUM_DIFICULDADES;
                break;
            case LINHA_FORCA: case LINHA_CONSTITUICAO: case LINHA_DESTREZA: case LINHA_AGILIDADE:
                if (mudanca != 0) mudarAtributo(c, c.linha - LINHA_FORCA, mudanca);
                break;
        }
    }
}

void aplicarCriacao(Jogo& jogo, const Criacao& c) {
    Jogador& j = jogo.jogador;
    j.nome = c.nome.empty() ? "Aventureiro" : c.nome;
    j.elemento = (Elemento)c.elemento;
    j.cor = c.cor;
    for (int a = 0; a < NUM_ATRIBUTOS; a++) j.atributos[a] = c.atributos[a];
    j.pontosLivres = c.pontosLivres;

    // Vidas e poções iniciais dependem da dificuldade.
    definirDificuldade(c.dificuldade);
    j.vidas = dificuldadeAtual().vidas;
    j.mochila[I_POCAO_DE_VIDA] = dificuldadeAtual().pocoes;

    // Golpes iniciais: os dois básicos + o golpe do elemento escolhido.
    j.numGolpes = 3;
    j.golpes[0] = G_GOLPE;
    j.golpes[1] = G_ESCUDO;
    j.golpes[2] = golpeInicialDoElemento(j.elemento);
    for (int s = 0; s < j.numGolpes; s++) j.pp[s] = GOLPES[j.golpes[s]].ppMaximo;

    recalcularAtributos(j);
    j.vida = j.vidaMaxima;
}

// -----------------------------------------------------------------------------
// PONTOS DE ATRIBUTO
// -----------------------------------------------------------------------------
// Gasta um ponto no atributo; a vida acompanha o aumento da vida máxima.
static void gastarPonto(Jogador& j, int atributo) {
    int vidaMaximaAntes = j.vidaMaxima;
    j.atributos[atributo]++;
    j.pontosLivres--;
    recalcularAtributos(j);
    j.vida += j.vidaMaxima - vidaMaximaAntes;
}

void ganharPontosDeAtributo(Jogo& jogo, int quantos) {
    Jogador& j = jogo.jogador;
    j.pontosLivres += quantos;
    if (!jogo.modoAutomatico) return;
    // Testes automáticos: põe cada ponto no atributo que tem menos.
    while (j.pontosLivres > 0) {
        int menor = 0;
        for (int a = 1; a < NUM_ATRIBUTOS; a++) {
            if (j.atributos[a] < j.atributos[menor]) menor = a;
        }
        gastarPonto(j, menor);
    }
}

void abrirFicha(Jogo& jogo) {
    Jogador& j = jogo.jogador;
    while (true) {
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
        if (escolha < 0) return;
        if (j.pontosLivres <= 0) {
            adicionarMensagem(jogo, "Você não tem pontos para distribuir. Suba de nível para ganhar mais.");
            return;
        }
        gastarPonto(j, escolha);
    }
}

bool novaPartida(Jogo& jogo, uint32_t semente) {
    Criacao c;
    criacaoPadrao(c);
    tocarMusica(MUSICA_MENU);
    if (!criarPersonagem(c)) return false;
    // A dificuldade vale antes de gerar a masmorra: a vida dos monstros do
    // primeiro andar já sai certa.
    definirDificuldade(c.dificuldade);
    iniciarJogo(jogo, semente);
    aplicarCriacao(jogo, c);
    mostrarHistoria(jogo, textoDaHistoria(jogo.jogador.nome));
    jogo.mensagensDoTurno.clear();
    jogo.tipoDoTurno = MSG_NORMAL;
    adicionarMensagem(jogo, "Boa sorte, " + jogo.jogador.nome + ". O único caminho é para baixo.");
    tocarMusica(MUSICA_MASMORRA);
    return true;
}
