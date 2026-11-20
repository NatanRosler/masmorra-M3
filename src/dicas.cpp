// dicas.cpp
// -----------------------------------------------------------------------------
// Os textos das dicas e quando cada uma aparece. As já vistas ficam em
// opcoes.txt (opcoes.cpp).
// -----------------------------------------------------------------------------
#include "dicas.h"
#include "opcoes.h"
#include "jogo.h"
#include "itens.h"
#include "tela.h"
#include "terminal.h"
#include "som.h"

#include <string>

struct TextoDaDica {
    const char* titulo;
    const char* texto;
};

// Na mesma ordem do enum Dica.
static const TextoDaDica DICAS[NUM_DICAS] = {
    { "Bem-vindo à masmorra!",
      "Ande com as setas, WASD ou HJKL. O mapa começa escuro e se revela conforme você explora. "
      "Desça pelas escadas até o 10º andar. A tecla ? mostra todos os comandos." },
    { "Vida baixa",
      "Aperte Z para descansar: a vida e os PP voltam aos poucos (mais rápido a cada 5 níveis). "
      "Um monstro aparecendo interrompe o descanso." },
    { "Mochila",
      "Aperte I para abrir a mochila: lá você usa poções e equipa armas e escudos. "
      "Itens raros (cristais e chaves) brilham no chão." },
    { "Mochila cheia",
      "A mochila leva até 12 itens, e cada tipo tem o seu limite. Para pegar algo novo, "
      "use o que tem, troque no chão ou venda na loja do Garro." },
    { "Pontos de atributo",
      "Você subiu de nível! Aperte C para distribuir os pontos: Força (dano), Constituição "
      "(vida e armadura), Destreza (precisão e crítico) e Agilidade (esquiva e velocidade)." },
    { "Caixas",
      "Ande contra uma caixa para empurrá-la. Empurrada contra um monstro, ela o deixa tonto "
      "(ele perde a 1ª rodada da batalha). Caixas quebradas podem soltar itens e moedas." },
    { "Chave do andar",
      "Esta chave abre a porta trancada da sala do tesouro DESTE andar. "
      "Ela não serve nos outros andares: use antes de descer!" },
    { "Armadilha",
      "Os espinhos vermelhos ferem quem pisa neles. Desvie, ou empurre uma caixa em cima "
      "para desarmar. Algumas armadilhas só aparecem quando você chega perto." },
    { "Poças d'água",
      "Andar na água deixa os seus passos mais lentos. Nas cavernas, os monstros de Água "
      "ficam mais fortes." },
    { "Lava",
      "Pisar na lava queima! Sempre existe outro caminho em volta dela. No vulcão, os "
      "monstros de Fogo ficam mais fortes." },
    { "Fonte cristalina",
      "Encoste na fonte para encher a vida e os PP. Ela só funciona uma vez: se estiver "
      "bem, deixe para depois." },
    { "Altar de sangue",
      "O altar oferece um pacto: -5 de vida máxima, para sempre, em troca de +2 de ataque. "
      "Você pode recusar." },
    { "Baú dourado",
      "Baús dourados guardam um item bom e moedas... mas alguns são MÍMICOS e mordem! "
      "Abra com a vida cheia." },
    { "Arena",
      "Encoste no gongo para aceitar o desafio: a porta tranca e vêm 3 ondas de monstros. "
      "Vencendo, você ganha um cristal e moedas." },
    { "Viajante perdido",
      "O Tobias troca um cristal por 1 Poção de Vida ou 30 moedas. Cristais dão bônus "
      "para sempre (vida, força, defesa ou velocidade)." },
    { "Loja do Garro",
      "Na lojinha nenhum monstro entra. Fale com o Garro para comprar poções e equipamentos "
      "ou vender o que não usa mais. O estoque é limitado." },
    { "O último andar",
      "O Senhor das Profundezas espera aqui. Ele fica do seu nível (+1) e, com metade da vida, "
      "fica furioso. Dele não dá para fugir: leve poções e use o golpe Escudo." },
    { "Batalha",
      "Escolha LUTAR e um golpe. Golpes gastam PP (descansar recupera). O elemento importa: "
      "Fogo > Planta > Água > Fogo. A bolinha colorida no mapa mostra o elemento do monstro." },
    { "Escudo",
      "O golpe Escudo age antes de todos e pode bloquear os ataques da rodada (com um escudo "
      "melhor, a chance é maior). Ótimo contra grupos e com a vida baixa." },
    { "Status",
      "Veneno tira vida a cada rodada; sono faz perder a vez, e a paralisia, às vezes. "
      "O Antídoto cura os três." },
    { "Modo furtivo",
      "Segure Shift enquanto anda (ou aperte F para ligar e desligar): os passos ficam lentos, "
      "mas silenciosos. Chegando assim num monstro distraído, ele perde a 1ª rodada!" },
    { "Campeão!",
      "Esse monstro com aura dourada e coroa é um CAMPEÃO: mais forte e com mais vida, mas vale "
      "o dobro de experiência, o triplo de moedas e pode soltar um item bom." },
    { "Gnomo vigia",
      "Se ele te vir, sopra a corneta e chama os monstros da sala dele e das salas vizinhas! "
      "Chegue furtivo e pegue ele antes. Se ele fugir, na próxima vez ele fica para lutar." },
    { "A sombra",
      "Sombras só aparecem de perto (2 casas) ou na luz das tochas. Na batalha são difíceis de "
      "acertar, mas têm pouca vida. Uma lanterna ajuda a vê-las antes." },
};

const char* tituloDaDica(Dica dica) { return DICAS[dica].titulo; }
const char* textoDaDica(Dica dica)  { return DICAS[dica].texto; }

bool dicaPendente(Dica dica) {
    return opcoes().mostrarDicas && !opcoes().dicaVista[dica];
}

void marcarDicaVista(Dica dica) {
    opcoes().dicaVista[dica] = true;
    salvarOpcoes();
}

void zerarDicas() {
    for (int i = 0; i < NUM_DICAS; i++) opcoes().dicaVista[i] = false;
    salvarOpcoes();
}

int dicasVistas() {
    int n = 0;
    for (int i = 0; i < NUM_DICAS; i++) {
        if (opcoes().dicaVista[i]) n++;
    }
    return n;
}

// Tempo em que as teclas são ignoradas quando a dica aparece: quem estava
// andando com a tecla apertada não fecha a caixa sem querer.
const int TRAVA_DA_DICA_MS = 700;

void mostrarDica(Jogo& jogo, Dica dica) {
    if (jogo.modoAutomatico || !dicaPendente(dica)) return;
    marcarDicaVista(dica);
    adicionarMensagem(jogo, std::string("Dica: ") + DICAS[dica].titulo);
    tocarSom(SOM_MENU);
    long long inicio = relogioMs();
    bool desenhada = false, estavaTravada = true;
    while (true) {
        bool travada = relogioMs() - inicio < TRAVA_DA_DICA_MS;
        // Só desenha de novo quando muda algo (no terminal, redesenhar sem
        // parar faria a tela piscar; na versão gráfica a espera já anima).
        if (!desenhada || travada != estavaTravada) {
            desenharDica(jogo, DICAS[dica].titulo, DICAS[dica].texto, travada);
            desenhada = true;
            estavaTravada = travada;
        }
        int tecla = lerTeclaComEspera(30);
        if (tecla == TECLA_REDIMENSIONOU) desenhada = false;
        if (travada || tecla == TECLA_NENHUMA || tecla == TECLA_REDIMENSIONOU) continue;
        break;
    }
}

// Algum monstro vivo à vista?
static bool monstroAVista(const Jogo& jogo, const Andar& a) {
    for (int i = 0; i < a.numMonstros; i++) {
        if (monstroAVista(jogo, a.monstros[i])) return true;
    }
    return false;
}

Dica proximaDica(const Jogo& jogo) {
    const Andar& a = andarAtual(jogo);
    const Jogador& j = jogo.jogador;
    if (dicaPendente(DICA_INICIO)) return DICA_INICIO;
    if (dicaPendente(DICA_CHEFE) && jogo.andar == NUM_ANDARES) return DICA_CHEFE;
    if (dicaPendente(DICA_NIVEL) && j.pontosLivres > 0) return DICA_NIVEL;
    if (dicaPendente(DICA_CHAVE) && j.mochila[I_CHAVE] > 0) return DICA_CHAVE;
    if (dicaPendente(DICA_MOCHILA_CHEIA) && itensNaMochila(j) >= limiteDaMochila(j)) return DICA_MOCHILA_CHEIA;
    if (dicaPendente(DICA_ITEM) && jogo.itensColetados > 0) return DICA_ITEM;
    if (dicaPendente(DICA_LOJA) && a.loja.existe && a.zonaSegura[j.y][j.x]) return DICA_LOJA;
    if (dicaPendente(DICA_DESCANSO) && j.vida * 2 < j.vidaMaxima && !jogo.descansando && !monstroAVista(jogo, a))
        return DICA_DESCANSO;

    // Monstros especiais à vista (a sombra só conta quando aparece de verdade).
    bool querCampeao = dicaPendente(DICA_CAMPEAO), querGnomo = dicaPendente(DICA_GNOMO);
    bool querSombra = dicaPendente(DICA_SOMBRA), querFurtivo = dicaPendente(DICA_FURTIVO);
    if (querCampeao || querGnomo || querSombra || querFurtivo) {
        for (int i = 0; i < a.numMonstros; i++) {
            const Monstro& m = a.monstros[i];
            if (!monstroAVista(jogo, m)) continue;
            int especial = tipoDe(m).especial;
            if (querCampeao && m.campeao) return DICA_CAMPEAO;
            if (querGnomo && (especial & VIGIA)) return DICA_GNOMO;
            if (querSombra && (especial & SOMBRIO)) return DICA_SOMBRA;
            // Um monstro dormindo ou distraído, ainda longe: hora de ser furtivo.
            int distancia = jogo.distanciaAteJogador[m.y][m.x];
            if (querFurtivo && m.estado != CACANDO && distancia > 3 && jogo.andar >= 2) return DICA_FURTIVO;
        }
    }

    // Coisas à vista: caixas, poças, lava, armadilhas e as surpresas.
    for (int i = 0; i < a.numEspeciais; i++) {
        const Especial& e = a.especiais[i];
        if (!jogo.visivel[e.y][e.x] || e.usado) continue;
        const Dica DA_SURPRESA[NUM_TIPOS_ESPECIAIS] = { DICA_FONTE, DICA_ALTAR, DICA_BAU, DICA_ARENA };
        if (dicaPendente(DA_SURPRESA[e.tipo])) return DA_SURPRESA[e.tipo];
    }
    for (int i = 0; i < a.numNpcs; i++) {
        const Npc& n = a.npcs[i];
        if (n.tipo == NPC_VIAJANTE && jogo.visivel[n.y][n.x] && dicaPendente(DICA_VIAJANTE)) return DICA_VIAJANTE;
    }
    for (int i = 0; i < a.numArmadilhas; i++) {
        const Armadilha& t = a.armadilhas[i];
        if (t.ativa && t.descoberta && jogo.visivel[t.y][t.x] && dicaPendente(DICA_ARMADILHA)) return DICA_ARMADILHA;
    }
    bool querCaixa = dicaPendente(DICA_CAIXA), querPoca = dicaPendente(DICA_POCA), querLava = dicaPendente(DICA_LAVA);
    if (querCaixa || querPoca || querLava) {
        for (int y = 0; y < MAPA_ALTURA; y++) {
            for (int x = 0; x < MAPA_LARGURA; x++) {
                if (!jogo.visivel[y][x]) continue;
                int celula = a.mapa.celulas[y][x];
                if (querCaixa && celula == CAIXA) return DICA_CAIXA;
                if (querPoca && celula == POCA) return DICA_POCA;
                if (querLava && celula == LAVA) return DICA_LAVA;
            }
        }
    }
    return NUM_DICAS;
}

void verificarDicasDoMapa(Jogo& jogo) {
    if (jogo.modoAutomatico || !opcoes().mostrarDicas || jogo.morto || jogo.venceu) return;
    Dica dica = proximaDica(jogo);
    if (dica != NUM_DICAS) mostrarDica(jogo, dica);
}
