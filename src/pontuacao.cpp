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
        "e a transformou numa masmorra de treze andares. Desde então, criaturas",
        "sobem pelos túneis e atacam a vila durante a noite.",
        "",
        "Muitos desceram para enfrentá-lo. Nenhum voltou.",
        "",
        nome + ", agora é a sua vez. Desça até o último andar, derrote o",
        "Senhor das Profundezas e liberte Valdoria.",
        "",
        "As portas da entrada se fecham atrás de você.",
        "O único caminho é para baixo.",
    };
}

// Já falou com alguém deste tipo em algum andar?
static bool encontrou(const Jogo& jogo, int tipoDeNpc) {
    for (int a = 0; a < NUM_ANDARES; a++)
        for (int i = 0; i < jogo.andares[a].numNpcs; i++)
            if (jogo.andares[a].npcs[i].tipo == tipoDeNpc && jogo.andares[a].npcs[i].conversou) return true;
    return false;
}

std::vector<std::string> textoDoFinal(const Jogo& jogo) {
    const std::string nome = jogo.jogador.nome;
    std::vector<std::string> t = {
        "O Senhor das Profundezas cai de joelhos. A escuridão que enchia a mina se desfaz como fumaça.",
        "",
    };
    if (encontrou(jogo, NPC_ESPIRITO))
        t.push_back("Lá no décimo andar, uma luz azulada se apaga devagar. Aldric, enfim, pode descansar.");
    else
        t.push_back("Em algum lugar lá em cima, uma luz azulada se apaga devagar. O último guardião, enfim, pode "
                    "descansar.");
    t.push_back("");
    t.push_back("As portas da entrada se abrem sozinhas. Pela primeira vez em cem anos, a luz do sol entra na mina.");
    t.push_back("");
    // Quem esperava na saída: quem você encontrou (e o Tobias, só se você ajudou).
    std::vector<std::string> quem;
    if (encontrou(jogo, NPC_EXPLORADORA)) quem.push_back("Eldra");
    if (encontrou(jogo, NPC_MINEIRO)) quem.push_back("Borin");
    if (encontrou(jogo, NPC_VIAJANTE)) quem.push_back("Tobias");
    std::string saida;
    if (quem.empty()) {
        saida = "Na saída, só o vento esperava por você. Mas lá embaixo, na vila, as janelas já se acendiam.";
    } else {
        std::string lista = quem[0];
        for (size_t i = 1; i < quem.size(); i++) lista += (i + 1 == quem.size() ? " e " : ", ") + quem[i];
        saida = "Na saída, " + lista + (quem.size() == 1 ? " esperava" : " esperavam") + " por você.";
        if (encontrou(jogo, NPC_VIAJANTE)) saida += " Tobias até chorou um pouquinho.";
    }
    if (encontrou(jogo, NPC_COMERCIANTE)) saida += " E Garro, claro, já tinha montado uma banca ali do lado.";
    t.push_back(saida);
    t.push_back("");
    t.push_back("Valdoria está livre. E a história de " + nome + ", que desceu os treze andares e voltou, vai ser "
                "contada por mais cem anos.");
    return t;
}

std::vector<std::string> falasDoChefe(const Jogo& jogo, bool primeiraVez) {
    if (primeiraVez) return {
        "\"Mais um... que desce para morrer.\"",
        "",
        "\"Cem anos eu espero aqui embaixo. Venha, " + jogo.jogador.nome + ". Mostre o que a superfície tem de "
        "melhor.\"",
        "",
        "\"Ogros! De pé. Temos visita.\"",
    };
    return { "\"Voltou? Ótimo. Eu ainda estou com fome.\"", "", "\"Ogros, de pé!\"" };
}

std::vector<std::string> textoComoJogar() {
    return {
        "OBJETIVO: chegar ao 13o andar e derrotar o Senhor das Profundezas.",
        "Vidas: 5 no Fácil, 3 no Médio e no Difícil. Ao perder uma, você volta",
        "para a entrada do andar.",
        "",
        "Andar: setas, WASD ou HJKL (diagonais: Y U B N). O mapa começa escuro",
        "e se revela conforme você explora. Escadas: ande para dentro delas.",
        "",
        "Batalha: encoste num monstro. Escolha LUTAR (golpe e alvo), ITENS,",
        "STATUS ou FUGIR. Golpes têm PP e elemento: Fogo > Planta > Água > Fogo.",
        "Até 3 monstros podem entrar na mesma batalha. O golpe Escudo age primeiro",
        "e pode bloquear os ataques da rodada (mais chance com escudo melhor).",
        "",
        "Z: descansar (cura mais a cada 5 níveis)  I: mochila  C: ficha  E: bestiário",
        "Shift (segurado) ou F: furtivo   ?: ajuda   M: mensagens   Esc: pausa",
        "Tela gráfica: segure Tab para ver tudo; roda do mouse, + e -: zoom do mapa.",
        "Sair salva a partida: use CONTINUAR no menu. Ao perder ou vencer, o save some.",
        QUEBRA_DE_PAGINA,
        "Andares 4-6: poças d'água (passos lentos). 7-9: cripta (lápides). 10-12: lava (queima!).",
        "Surpresas: fonte (cura), altar (pacto), baú dourado (cuidado: pode morder!),",
        "gongo da arena (3 ondas de monstros por um prêmio) e o viajante perdido.",
        "",
        "Modo furtivo: passos lentos, mas os monstros te percebem de mais perto e",
        "quase não acordam. Chegue num monstro distraído: ele perde a 1a rodada!",
        "",
        "Campeões (aura dourada e coroa): mais fortes, mas valem o dobro de",
        "experiência, o triplo de moedas e podem soltar um item bom.",
        "Gnomo vigia: se te vir, sopra a corneta e chama as salas vizinhas.",
        "Sombra: só aparece de perto ou na luz das tochas.",
        "O Senhor das Profundezas nunca luta sozinho: 2 ogros guarda-costas entram junto.",
        "Andares 3, 6, 9 e 12: um mini-chefe guarda a escada, na sala dele. O portão fecha",
        "  quando você entra e não dá para fugir. Vencendo, ganha um prêmio.",
        QUEBRA_DE_PAGINA,
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
        "A Mochila de Couro aumenta para 16 itens (e +1 em cada limite).",
        "",
        "OPÇÕES (no menu ou no Esc): volumes, tela cheia (F11), animações e dicas.",
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
        "ESCUDOS (equipar: + defesa e chance de bloquear)",
        "  Madeira, Ferro e Torre (o de torre deixa você mais lento)",
        "  Golpe Escudo: 30% sem escudo, 45% madeira, 60% ferro, 75% torre",
        "  Sozinho (sem usar o golpe): 5% madeira, 8% ferro, 12% torre",
        "",
        "CRISTAIS (bônus permanente ao pegar)",
        "  Vida, Força, Ferro (defesa) e Pena do Vento (velocidade)",
        "",
        "PERGAMINHOS: ensinam o próximo golpe do elemento que você ainda não sabe",
        "CHAVE: abre a porta trancada da sala do tesouro DO MESMO ANDAR",
        "  (ao pegar, a porta aparece no mapa; o andar 4 não tem chave: o Borin abre)",
        "",
        QUEBRA_DE_PAGINA,
        "SUPORTE",
        "  Mochila de Couro: 16 itens na mochila e +1 em cada limite (para sempre)",
        "  Lanterna: +2 no raio de luz, para ver mais longe (para sempre)",
        "  Bomba de Fumaça: fuga garantida da batalha (menos do chefe e da arena)",
        "  Pergaminho do Mapa: mostra o andar inteiro no mapa",
        "  (a mochila e a lanterna vêm do tesouro, do baú dourado ou do Garro)",
        "",
        "MOCHILA: no máximo 12 itens; cada poção e cada equipamento tem o seu limite",
        "",
        "LOJA DO GARRO (andares 3, 6 e 9): poções, bombas e pergaminhos do mapa,",
        "  2 itens especiais e, se você ainda não tiver, a mochila ou a lanterna.",
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
    // Textos longos vêm divididos com QUEBRA_DE_PAGINA: uma página de cada vez.
    std::vector<std::vector<std::string>> paginas(1);
    for (const std::string& l : linhas) {
        if (l == QUEBRA_DE_PAGINA) paginas.push_back({});
        else paginas.back().push_back(l);
    }
    for (size_t p = 0; p < paginas.size(); p++) {
        std::string rodape = "Aperte qualquer tecla para continuar";
        if (paginas.size() > 1) {
            rodape = "Página " + std::to_string(p + 1) + " de " + std::to_string(paginas.size()) +
                     "  -  aperte qualquer tecla para " + (p + 1 < paginas.size() ? "a próxima" : "continuar");
        }
        int t;
        do {
            desenharPagina(titulo, paginas[p], rodape);
            t = lerTecla();
        } while (t == TECLA_REDIMENSIONOU);
    }
}
