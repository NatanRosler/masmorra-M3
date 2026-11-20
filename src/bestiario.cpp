// bestiario.cpp
// -----------------------------------------------------------------------------
// Lê e grava o bestiário em bestiario.txt. Uma linha por espécie, pelo NOME
// (assim a ordem da tabela de monstros pode mudar sem estragar o arquivo):
//   rato|visto=1|derrotados=12|campeoes=0|golpes=10,11|elementos=0,3
// Linhas estranhas são ignoradas, então um arquivo editado à mão ou de outra
// versão não quebra o jogo.
// -----------------------------------------------------------------------------
#include "bestiario.h"
#include "jogo.h"
#include "monstros.h"

#include <fstream>
#include <sstream>
#include <vector>

static const char* ARQUIVO_DO_BESTIARIO = "bestiario.txt";
static FichaDoBestiario fichas[MAX_ESPECIES];
static bool podeGravar = false;   // só depois de carregarBestiario()

const FichaDoBestiario& fichaDoBestiario(int tipo) {
    static const FichaDoBestiario vazia = { false, 0, 0, 0, 0 };
    if (tipo < 0 || tipo >= NUM_TIPOS_DE_MONSTRO) return vazia;
    return fichas[tipo];
}

static void gravar() {
    if (!podeGravar) return;
    std::ofstream arquivo(ARQUIVO_DO_BESTIARIO);
    if (!arquivo) return;   // sem permissão para gravar: segue sem salvar
    arquivo << "# Bestiário do Masmorra (pode apagar este arquivo: tudo volta a zero)\n";
    for (int t = 0; t < NUM_TIPOS_DE_MONSTRO; t++) {
        const FichaDoBestiario& f = fichas[t];
        if (!f.visto && f.derrotados == 0) continue;
        arquivo << TIPOS_DE_MONSTRO[t].nome << "|visto=" << (f.visto ? 1 : 0)
                << "|derrotados=" << f.derrotados << "|campeoes=" << f.campeoesDerrotados << "|golpes=";
        bool primeiro = true;
        for (int g = 0; g < NUM_GOLPES; g++) {
            if (!(f.golpesVistos & (1ULL << g))) continue;
            arquivo << (primeiro ? "" : ",") << g;
            primeiro = false;
        }
        arquivo << "|elementos=";
        primeiro = true;
        for (int e = 0; e < NUM_ELEMENTOS; e++) {
            if (!(f.elementosVistos & (1 << e))) continue;
            arquivo << (primeiro ? "" : ",") << e;
            primeiro = false;
        }
        arquivo << "\n";
    }
}

// Número de 0 a "maximo"; -1 se não for número.
static int numero(const std::string& texto, int maximo) {
    if (texto.empty() || texto.size() > 9) return -1;
    int valor = 0;
    for (char c : texto) {
        if (c < '0' || c > '9') return -1;
        valor = valor * 10 + (c - '0');
    }
    return valor > maximo ? maximo : valor;
}

void carregarBestiario() {
    for (int t = 0; t < MAX_ESPECIES; t++) fichas[t] = { false, 0, 0, 0, 0 };
    std::ifstream arquivo(ARQUIVO_DO_BESTIARIO);
    std::string linha;
    while (std::getline(arquivo, linha)) {
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();   // arquivo do Windows
        if (linha.empty() || linha[0] == '#') continue;
        std::vector<std::string> partes;
        std::stringstream ss(linha);
        std::string parte;
        while (std::getline(ss, parte, '|')) partes.push_back(parte);
        int tipo = -1;
        for (int t = 0; t < NUM_TIPOS_DE_MONSTRO; t++) if (partes[0] == TIPOS_DE_MONSTRO[t].nome) tipo = t;
        if (tipo < 0) continue;
        FichaDoBestiario& f = fichas[tipo];
        for (size_t i = 1; i < partes.size(); i++) {
            size_t igual = partes[i].find('=');
            if (igual == std::string::npos) continue;
            std::string chave = partes[i].substr(0, igual), valor = partes[i].substr(igual + 1);
            if (chave == "visto") f.visto = (valor == "1");
            if (chave == "derrotados") f.derrotados = numero(valor, 999999) < 0 ? 0 : numero(valor, 999999);
            if (chave == "campeoes") f.campeoesDerrotados = numero(valor, 999999) < 0 ? 0 : numero(valor, 999999);
            if (chave == "golpes" || chave == "elementos") {
                std::stringstream lista(valor);
                std::string item;
                while (std::getline(lista, item, ',')) {
                    int n = numero(item, 1000);
                    if (chave == "golpes" && n >= 0 && n < NUM_GOLPES) f.golpesVistos |= 1ULL << n;
                    if (chave == "elementos" && n >= 0 && n < NUM_ELEMENTOS) f.elementosVistos |= 1 << n;
                }
            }
        }
        if (f.derrotados > 0) f.visto = true;
    }
    podeGravar = true;
}

void zerarBestiario() {
    for (int t = 0; t < MAX_ESPECIES; t++) fichas[t] = { false, 0, 0, 0, 0 };
    gravar();
}

int especiesVistas() {
    int n = 0;
    for (int t = 0; t < NUM_TIPOS_DE_MONSTRO; t++) if (fichas[t].visto) n++;
    return n;
}
int especiesDoBestiario() { return NUM_TIPOS_DE_MONSTRO; }

void registrarAvistado(Jogo& jogo, const Monstro& m) {
    if (m.tipo < 0 || m.tipo >= NUM_TIPOS_DE_MONSTRO) return;
    jogo.especiesVistasNaPartida |= 1 << m.tipo;
    FichaDoBestiario& f = fichas[m.tipo];
    int bit = 1 << (int)m.elemento;
    if (f.visto && (f.elementosVistos & bit)) return;   // nada de novo
    f.visto = true;
    f.elementosVistos |= bit;
    gravar();
}

void registrarGolpe(int tipo, int golpe) {
    if (tipo < 0 || tipo >= NUM_TIPOS_DE_MONSTRO || golpe < 0 || golpe >= NUM_GOLPES) return;
    FichaDoBestiario& f = fichas[tipo];
    if (f.golpesVistos & (1ULL << golpe)) return;
    f.golpesVistos |= 1ULL << golpe;
    gravar();
}

void registrarDerrota(const Monstro& m) {
    if (m.tipo < 0 || m.tipo >= NUM_TIPOS_DE_MONSTRO) return;
    FichaDoBestiario& f = fichas[m.tipo];
    f.visto = true;
    f.derrotados++;
    if (m.campeao) f.campeoesDerrotados++;
    gravar();
}

bool vistaNaPartida(const Jogo& jogo, int tipo) {
    return tipo >= 0 && tipo < NUM_TIPOS_DE_MONSTRO && (jogo.especiesVistasNaPartida & (1 << tipo));
}

std::string descricaoDaEspecie(int tipo) {
    if (tipo < 0 || tipo >= NUM_TIPOS_DE_MONSTRO) return "";
    switch (TIPOS_DE_MONSTRO[tipo].simbolo) {
        case 'r': return "Pequeno e fraco, mas aparece em todo canto dos primeiros andares.";
        case 'k': return "Criatura das minas. Ataca de frente, sem pensar duas vezes.";
        case 'j': return "Rápido e caça em bando: onde aparece um, costuma vir outro.";
        case 'm': return "Medroso: quando a vida fica baixa, ele foge (no mapa e na batalha).";
        case 't': return "Pula das poças. A língua grudenta deixa você mais lento.";
        case 'g': return "Esperto e venenoso. Aparece em quase todos os andares.";
        case 'v': return "Voa rápido e sem rumo. Suga a sua vida para se curar.";
        case 'O': return "Enorme e lento, mas bate muito forte. Quebra as caixas no caminho.";
        case 'N': return "Guardiã das águas profundas dos últimos andares. Hipnotiza e envenena.";
        case 'G': return "Ao ver você, sopra a corneta e chama os monstros das salas vizinhas. "
                         "Na primeira vez foge; na segunda, fica e luta.";
        case 'W': return "Só aparece de perto (2 casas) ou na luz das tochas. Difícil de acertar.";
        case 'M': return "Parece um baú dourado... até você tentar abrir.";
        case 'S': return "Espera no fim do andar 10. Fica mais forte junto com você e, com "
                         "metade da vida, fica furioso.";
    }
    return "";
}

std::string andaresDaEspecie(int tipo) {
    if (tipo < 0 || tipo >= NUM_TIPOS_DE_MONSTRO) return "";
    const TipoMonstro& t = TIPOS_DE_MONSTRO[tipo];
    if (t.especial & CHEFE) return "andar 10 (chefe final)";
    if (t.frequencia == 0) return "dentro de baús, em qualquer andar";
    return "andares " + std::to_string(t.andarMinimo) + " a " + std::to_string(t.andarMaximo);
}
