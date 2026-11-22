// opcoes.cpp
// -----------------------------------------------------------------------------
// Lê e grava as opções em opcoes.txt. Formato (uma por linha):
//   musica=8
//   efeitos=10
//   ambiente=7               (sons dos andares, das salas e os passos)
//   tela_cheia=0
//   animacao_rapida=0
//   zoom=1                    (zoom do mapa: 0 longe, 1 médio, 2 perto)
//   texto=1                   (falas e história: 0 lenta, 1 normal, 2 rápida, 3 na hora)
//   impossivel=0              (1 = já venceu o Difícil: o modo Impossível está liberado)
//   dicas=1
//   dicas_vistas=0110000...   (um 0 ou 1 para cada dica, na ordem do enum)
// Linhas desconhecidas são ignoradas e valores fora da faixa são corrigidos,
// então um arquivo de outra versão (ou editado à mão) não quebra o jogo.
// -----------------------------------------------------------------------------
#include "opcoes.h"
#include "jogo.h"

#include <fstream>
#include <string>

static const char* ARQUIVO_DAS_OPCOES = "opcoes.txt";
static bool podeGravar = true;

static Opcoes padrao() {
    Opcoes o;
    o.volumeMusica = 10;
    o.volumeEfeitos = 10;
    o.volumeAmbiente = 7;
    o.telaCheia = false;
    o.animacaoRapida = false;
    o.zoomDoMapa = 1;
    o.velocidadeDoTexto = 1;
    o.impossivelLiberado = false;
    o.mostrarDicas = true;
    o.batalhaAutomatica = true;
    for (int i = 0; i < NUM_DICAS; i++) o.dicaVista[i] = false;
    return o;
}

Opcoes& opcoes() {
    static Opcoes atuais = padrao();
    return atuais;
}

static int limitar(int valor, int minimo, int maximo) {
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

// Converte o texto em número; se não for número, devolve "seNaoDer".
static int numero(const std::string& texto, int seNaoDer) {
    if (texto.empty()) return seNaoDer;
    int valor = 0;
    for (char c : texto) {
        if (c < '0' || c > '9') return seNaoDer;
        valor = valor * 10 + (c - '0');
        if (valor > 1000) return seNaoDer;
    }
    return valor;
}

void carregarOpcoes() {
    Opcoes& o = opcoes();
    o = padrao();
    std::ifstream arquivo(ARQUIVO_DAS_OPCOES);
    std::string linha;
    while (std::getline(arquivo, linha)) {
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();   // arquivo do Windows
        size_t igual = linha.find('=');
        if (linha.empty() || linha[0] == '#' || igual == std::string::npos) continue;
        std::string chave = linha.substr(0, igual), valor = linha.substr(igual + 1);
        if (chave == "musica")          o.volumeMusica = limitar(numero(valor, 10), 0, 10);
        if (chave == "efeitos")         o.volumeEfeitos = limitar(numero(valor, 10), 0, 10);
        if (chave == "ambiente")        o.volumeAmbiente = limitar(numero(valor, 7), 0, 10);
        if (chave == "tela_cheia")      o.telaCheia = (valor == "1");
        if (chave == "animacao_rapida") o.animacaoRapida = (valor == "1");
        if (chave == "zoom")            o.zoomDoMapa = limitar(numero(valor, 1), 0, 2);
        if (chave == "texto")           o.velocidadeDoTexto = limitar(numero(valor, 1), 0, 3);
        if (chave == "impossivel")      o.impossivelLiberado = (valor == "1");
        if (chave == "dicas")           o.mostrarDicas = (valor != "0");
        if (chave == "batalha_auto")    o.batalhaAutomatica = (valor != "0");
        if (chave == "dicas_vistas") {
            for (int i = 0; i < NUM_DICAS && i < (int)valor.size(); i++) o.dicaVista[i] = (valor[i] == '1');
        }
    }
}

void salvarOpcoes() {
    if (!podeGravar) return;
    const Opcoes& o = opcoes();
    std::ofstream arquivo(ARQUIVO_DAS_OPCOES);
    if (!arquivo) return;   // sem permissão para gravar: segue sem salvar
    arquivo << "# Opções do Masmorra (pode apagar este arquivo: tudo volta ao padrão)\n";
    arquivo << "musica=" << o.volumeMusica << "\n";
    arquivo << "efeitos=" << o.volumeEfeitos << "\n";
    arquivo << "ambiente=" << o.volumeAmbiente << "\n";
    arquivo << "tela_cheia=" << (o.telaCheia ? 1 : 0) << "\n";
    arquivo << "animacao_rapida=" << (o.animacaoRapida ? 1 : 0) << "\n";
    arquivo << "zoom=" << o.zoomDoMapa << "\n";
    arquivo << "texto=" << o.velocidadeDoTexto << "\n";
    arquivo << "impossivel=" << (o.impossivelLiberado ? 1 : 0) << "\n";
    arquivo << "dicas=" << (o.mostrarDicas ? 1 : 0) << "\n";
    arquivo << "batalha_auto=" << (o.batalhaAutomatica ? 1 : 0) << "\n";
    arquivo << "dicas_vistas=";
    for (int i = 0; i < NUM_DICAS; i++) arquivo << (o.dicaVista[i] ? '1' : '0');
    arquivo << "\n";
}

void naoGravarOpcoes() { podeGravar = false; }

int tempoDeLeituraMs() {
    return opcoes().animacaoRapida ? TEMPO_DE_LEITURA_MS * 6 / 10 : TEMPO_DE_LEITURA_MS;
}
