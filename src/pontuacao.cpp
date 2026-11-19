// pontuacao.cpp
#include "pontuacao.h"
#include "salvar.h"
#include "jogo.h"
#include "tela.h"
#include "terminal.h"
#include "dificuldade.h"

#include <fstream>
#include <sstream>

const char* ARQUIVO_DO_RANKING = "ranking.txt";

// -----------------------------------------------------------------------------
// PONTUAÇÃO
// -----------------------------------------------------------------------------
int calcularPontuacao(const Jogo& jogo) {
    int vidasPerdidas = dificuldadeAtual().vidas - jogo.jogador.vidas;
    int pontos = 250 * jogo.andarMaisFundo
               + 30 * jogo.monstrosMortos
               + 20 * jogo.itensColetados
               - jogo.passos / 5
               - 300 * vidasPerdidas
               + jogo.jogador.moedas;   // moedas guardadas: 1 ponto cada
    if (jogo.venceu) pontos += 3000;
    pontos = pontos * dificuldadeAtual().pontuacao / 100;   // difícil vale mais
    return pontos < 0 ? 0 : pontos;
}

// -----------------------------------------------------------------------------
// RANKING (arquivo de texto, uma partida por linha:
//          pontos;andar;venceu;nome;dificuldade)
// -----------------------------------------------------------------------------
std::vector<RegistroRanking> lerRanking() {
    std::vector<RegistroRanking> ranking;
    std::ifstream arquivo(ARQUIVO_DO_RANKING);
    std::string linha;
    while (std::getline(arquivo, linha)) {
        std::stringstream campos(linha);
        std::string pontos, andar, venceu, nome;
        if (!std::getline(campos, pontos, ';') || !std::getline(campos, andar, ';') ||
            !std::getline(campos, venceu, ';') || !std::getline(campos, nome)) continue;
        if (!nome.empty() && nome.back() == '\r') nome.pop_back();
        RegistroRanking r;
        // A dificuldade vem depois do nome (arquivos antigos não têm).
        size_t separador = nome.find(';');
        r.dificuldade = (separador == std::string::npos) ? "Difícil" : nome.substr(separador + 1);
        r.nome = nome.substr(0, separador);
        r.pontos = std::atoi(pontos.c_str());
        r.andar = std::atoi(andar.c_str());
        r.venceu = (venceu == "1");
        ranking.push_back(r);
    }
    return ranking;
}

static void salvarRanking(const std::vector<RegistroRanking>& ranking) {
    std::ofstream arquivo(ARQUIVO_DO_RANKING);
    for (const RegistroRanking& r : ranking) {
        arquivo << r.pontos << ';' << r.andar << ';' << (r.venceu ? 1 : 0) << ';' << r.nome << ';' << r.dificuldade << '\n';
    }
}

void registrarNoRanking(Jogo& jogo) {
    if (jogo.rankingRegistrado) return;
    jogo.rankingRegistrado = true;
    // Roguelike: a partida acabou (vitória ou derrota), então o save some.
    if (!jogo.modoAutomatico) apagarPartidaSalva();
    jogo.pontuacaoFinal = calcularPontuacao(jogo);

    RegistroRanking novo;
    novo.nome = jogo.jogador.nome;
    novo.pontos = jogo.pontuacaoFinal;
    novo.andar = jogo.andarMaisFundo;
    novo.venceu = jogo.venceu;
    novo.dificuldade = dificuldadeAtual().nome;

    // Insere na posição certa (maior pontuação primeiro) e corta no top 10.
    std::vector<RegistroRanking> ranking = lerRanking();
    int posicao = (int)ranking.size();
    for (int i = 0; i < (int)ranking.size(); i++) {
        if (novo.pontos > ranking[i].pontos) { posicao = i; break; }
    }
    ranking.insert(ranking.begin() + posicao, novo);
    if ((int)ranking.size() > TAMANHO_DO_RANKING) ranking.resize(TAMANHO_DO_RANKING);
    jogo.posicaoNoRanking = (posicao < TAMANHO_DO_RANKING) ? posicao : -1;
    salvarRanking(ranking);
}

// -----------------------------------------------------------------------------
// TEXTOS
// -----------------------------------------------------------------------------
std::vector<std::string> textoDaHistoria(const std::string& nome) {
    return {
        "Há cem anos, o Senhor das Profundezas tomou a velha mina de Valdoria",
        "e a transformou numa masmorra de dez andares. Desde então, criaturas",
        "sobem pelos túneis e atacam a vila durante a noite.",
        "",
        "Muitos desceram para enfrentá-lo. Nenhum voltou.",
        "",
        nome + ", agora é a sua vez. Desça até o décimo andar, derrote o",
        "Senhor das Profundezas e liberte Valdoria.",
        "",
        "As portas da entrada se fecham atrás de você.",
        "O único caminho é para baixo.",
    };
}

std::vector<std::string> textoComoJogar() {
    return {
        "OBJETIVO: chegar ao 10o andar e derrotar o Senhor das Profundezas.",
        "Vidas: 5 no Fácil, 3 no Médio e no Difícil. Ao perder uma, você volta",
        "para a entrada do andar.",
        "",
        "Andar: setas, WASD ou HJKL (diagonais: Y U B N). O mapa começa escuro",
        "e se revela conforme você explora. Escadas: ande para dentro delas.",
        "",
        "Batalha: encoste num monstro. Escolha LUTAR (golpe e alvo), ITENS,",
        "STATUS ou FUGIR. Golpes têm PP e elemento: Fogo > Planta > Água > Fogo.",
        "Até 3 monstros podem entrar na mesma batalha.",
        "",
        "Andares 4-6: poças d'água (passos lentos). Andares 7-9: lava (queima, desvie!).",
        "Surpresas: fonte (cura), altar (pacto), baú dourado (cuidado: pode morder!),",
        "gongo da arena (3 ondas de monstros por um prêmio) e o viajante perdido.",
        "Sair salva a partida: use CONTINUAR no menu. Ao perder ou vencer, o save some.",
        "",
        "Z: descansar (+1 de vida por segundo)    I: mochila    C: ficha",
        "?: ajuda    M: mensagens    Esc: pausa",
        "",
        "ATRIBUTOS (1 ponto a cada nível, tecla C):",
        "Força = dano   Constituição = vida e armadura",
        "Destreza = precisão e crítico   Agilidade = esquiva e velocidade",
        "",
        "Caixas: empurre contra um monstro para atordoá-lo (ele perde a 1a rodada).",
        "Quebradas, elas podem soltar poções, armas, cristais e moedas.",
        "",
        "Moedas: ganhe derrotando monstros, subindo de nível e quebrando caixas.",
        "Gaste com Garro, o comerciante ($), nos andares 3, 6 e 9. Na lojinha",
        "dele nenhum monstro entra.",
        "",
        "Mochila: até 12 itens (6 Poções de Vida, 3 de cada outra poção e 1 de",
        "cada arma, escudo ou pergaminho de reserva). Cheia? Troque no chão.",
    };
}

std::vector<std::string> textoDosItens() {
    return {
        "POÇÕES (uso único)",
        "  Poção de Vida: cura metade da vida   Poção Grande: cura toda a vida",
        "  Antídoto: tira veneno, sono e paralisia   Éter: recupera os PP",
        "",
        "ARMAS (equipar: + ataque)",
        "  Adaga, Espada Curta, Machado, Espada Longa",
        "  Armas elementais: golpes do mesmo elemento causam +25% de dano",
        "",
        "ESCUDOS (equipar: + defesa)",
        "  Madeira, Ferro e Torre (o de torre deixa você mais lento)",
        "",
        "CRISTAIS (bônus permanente ao pegar)",
        "  Vida, Força, Ferro (defesa) e Pena do Vento (velocidade)",
        "",
        "PERGAMINHOS: ensinam um golpe de um elemento",
        "CHAVE: abre a porta trancada da sala do tesouro DO MESMO ANDAR",
        "  (ao pegar, a porta aparece no mapa; o andar 4 não tem chave: o Borin abre)",
        "",
        "MOCHILA: no máximo 12 itens; cada poção e cada equipamento tem o seu limite",
        "",
        "LOJA DO GARRO (andares 3, 6 e 9): poções sempre à venda e 2 itens especiais.",
        "  Ele também compra o que você não usa mais, pela metade do preço.",
    };
}

std::vector<std::string> textoDaPontuacao() {
    std::vector<std::string> linhas = {
        "+250 por andar alcançado    +30 por monstro derrotado",
        "+20 por item coletado       -1 a cada 5 passos",
        "-300 por vida perdida       +3000 por derrotar o chefe final",
        "+1 por moeda guardada no fim da partida",
        "Dificuldade: Fácil vale 50% dos pontos, Médio 100% e Difícil 150%.",
        "",
        "MELHORES PARTIDAS",
    };
    std::vector<RegistroRanking> ranking = lerRanking();
    if (ranking.empty()) linhas.push_back("  (nenhuma partida ainda)");
    for (int i = 0; i < (int)ranking.size(); i++) {
        const RegistroRanking& r = ranking[i];
        std::string linha = (i < 9 ? " " : "") + std::to_string(i + 1) + ". " + r.nome;
        while ((int)linha.size() < 20) linha += ' ';
        linha += std::to_string(r.pontos) + " pontos   andar " + std::to_string(r.andar) + "   " + r.dificuldade;
        if (r.venceu) linha += "   VENCEU";
        linhas.push_back(linha);
    }
    return linhas;
}

void mostrarPagina(const std::string& titulo, const std::vector<std::string>& linhas) {
    int t;
    do {
        desenharPagina(titulo, linhas, "Aperte qualquer tecla para continuar");
        t = lerTecla();
    } while (t == TECLA_REDIMENSIONOU);
}
