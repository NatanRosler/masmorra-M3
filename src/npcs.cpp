// npcs.cpp
#include "npcs.h"
#include "tela.h"
#include "jogo.h"
#include "pontuacao.h"
#include "som.h"
#include "loja.h"
#include <cstdlib>

const FichaNpc FICHAS_DOS_NPCS[NUM_TIPOS_DE_NPC] = {
    { "Eldra, a exploradora",   '&', { 120, 210, 230 }, 1, 0 },
    { "Borin, o velho mineiro", '&', { 230, 170,  90 }, 4, 0 },
    { "Aldric, o espírito guardião", '&', { 200, 200, 255 }, 7, 0 },
    { "Garro, o comerciante",   '$', { 255, 205,  70 }, 3, 3 },   // andares 3, 6 e 9
    { "Tobias, o viajante perdido", '&', { 200, 160, 255 }, 0, 0 }, // sorteado em salas.cpp
};

// O NPC deste tipo aparece neste andar?
static bool apareceNoAndar(const FichaNpc& ficha, int numeroDoAndar) {
    if (numeroDoAndar == ficha.andar) return true;
    if (ficha.repeteACada <= 0 || numeroDoAndar < ficha.andar || numeroDoAndar >= NUM_ANDARES) return false;
    return (numeroDoAndar - ficha.andar) % ficha.repeteACada == 0;
}

bool temComercianteNoAndar(int numeroDoAndar) {
    return apareceNoAndar(FICHAS_DOS_NPCS[NPC_COMERCIANTE], numeroDoAndar);
}

// =============================================================================
// A LOJINHA DO COMERCIANTE
// =============================================================================
// Mesma ideia da sala do tesouro (gerador.cpp): testa cada porta como se fosse
// parede. Se isso isola uma salinha (de 9 a 80 casas, sem escada e sem outra
// porta), ela é candidata. Fica a mais perto da chegada.
void prepararSalaDaLoja(Andar& andar, int numeroDoAndar) {
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++) andar.zonaSegura[y][x] = false;
    andar.loja.existe = false;
    if (!temComercianteNoAndar(numeroDoAndar)) return;

    Mapa& mapa = andar.mapa;
    const Escadas& e = andar.escadas;
    static int antes[MAPA_ALTURA][MAPA_LARGURA], depois[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, antes);

    int melhorX = -1, melhorY = -1, melhorDistancia = 1000000;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (mapa.celulas[y][x] != PORTA || antes[y][x] == INALCANCAVEL) continue;
            if (antes[y][x] >= melhorDistancia) continue;

            mapa.celulas[y][x] = PAREDE;                     // e se fosse parede?
            calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, depois);
            mapa.celulas[y][x] = PORTA;                      // desfaz o teste
            if (e.temDescida && depois[e.chegadaDesceY][e.chegadaDesceX] == INALCANCAVEL) continue;

            int isoladas = 0;
            bool problema = false;
            for (int cy = 1; cy < MAPA_ALTURA - 1 && !problema; cy++) {
                for (int cx = 1; cx < MAPA_LARGURA - 1; cx++) {
                    if (antes[cy][cx] == INALCANCAVEL || depois[cy][cx] != INALCANCAVEL) continue;
                    if (cx == x && cy == y) continue;
                    isoladas++;
                    TipoCelula c = mapa.celulas[cy][cx];
                    if (c != CHAO) { problema = true; break; }     // escada ou outra porta lá dentro
                    for (int d = 0; d < 4; d++) {
                        if (mapa.celulas[cy + DIRECOES_DY[d]][cx + DIRECOES_DX[d]] == PORTA_TRANCADA) problema = true;
                    }
                }
            }
            if (problema || isoladas < 9 || isoladas > 80) continue;
            melhorX = x;
            melhorY = y;
            melhorDistancia = antes[y][x];
        }
    }
    if (melhorX < 0) return;   // sem salinha: o comerciante ganha só um tapete (colocarNpcs)

    // Marca a sala (e a porta) como zona segura.
    mapa.celulas[melhorY][melhorX] = PAREDE;
    calcularDistancias(mapa, e.chegadaSobeX, e.chegadaSobeY, depois);
    mapa.celulas[melhorY][melhorX] = PORTA;
    long somaX = 0, somaY = 0;
    int qtd = 0;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (antes[y][x] != INALCANCAVEL && depois[y][x] == INALCANCAVEL) {
                andar.zonaSegura[y][x] = true;
                somaX += x; somaY += y; qtd++;
            }
        }
    }
    andar.zonaSegura[melhorY][melhorX] = true;
    andar.loja.existe = true;
    andar.loja.portaX = melhorX;
    andar.loja.portaY = melhorY;

    // Balcão: a casa da sala mais perto do meio dela, longe da porta.
    float meioX = (float)somaX / qtd, meioY = (float)somaY / qtd;
    float melhor = 1e9f;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (!andar.zonaSegura[y][x] || mapa.celulas[y][x] != CHAO) continue;
            if (std::abs(x - melhorX) <= 1 && std::abs(y - melhorY) <= 1) continue;
            float d = (x - meioX) * (x - meioX) + (y - meioY) * (y - meioY);
            if (d < melhor) { melhor = d; andar.loja.balcaoX = x; andar.loja.balcaoY = y; }
        }
    }

    // Lampião: a parede da sala mais longe da porta vira uma tocha.
    int lampiaoX = -1, lampiaoY = -1, maisLonge = -1;
    for (int y = 1; y < MAPA_ALTURA - 1; y++) {
        for (int x = 1; x < MAPA_LARGURA - 1; x++) {
            if (mapa.celulas[y][x] != PAREDE) continue;
            bool encosta = false;
            for (int d = 0; d < 4; d++) {
                int nx = x + DIRECOES_DX[d], ny = y + DIRECOES_DY[d];
                if (andar.zonaSegura[ny][nx] && mapa.celulas[ny][nx] == CHAO) encosta = true;
            }
            int distancia = std::abs(x - melhorX) + std::abs(y - melhorY);
            if (encosta && distancia > maisLonge) { maisLonge = distancia; lampiaoX = x; lampiaoY = y; }
        }
    }
    if (lampiaoX >= 0) mapa.celulas[lampiaoY][lampiaoX] = TOCHA;
}

// Tira da lojinha o que não combina com ela: caixas, armadilhas e monstros.
static void protegerSalaDaLoja(Andar& andar) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (!andar.zonaSegura[y][x]) continue;
            if (andar.mapa.celulas[y][x] == CAIXA) {
                andar.mapa.celulas[y][x] = CHAO;
                andar.conteudoDasCaixas[y][x] = -1;
            }
        }
    }
    for (int i = 0; i < andar.numArmadilhas; i++) {
        Armadilha& a = andar.armadilhas[i];
        if (andar.zonaSegura[a.y][a.x]) {
            andar.armadilhas[i] = andar.armadilhas[--andar.numArmadilhas];
            i--;
        }
    }
    for (int i = 0; i < andar.numMonstros; i++) {
        Monstro& m = andar.monstros[i];
        if (!m.vivo || !andar.zonaSegura[m.y][m.x]) continue;
        m.vivo = false;
        // Se ele carregava a chave do andar, ela fica no chão.
        if (m.carregaChave && andar.numItens < MAX_ITENS_NO_CHAO) {
            ItemNoChao& chave = andar.itens[andar.numItens++];
            chave.ativo = true;
            chave.tipo = I_CHAVE;
            chave.x = m.x;
            chave.y = m.y;
            chave.visto = false;
        }
    }
}

void colocarNpcs(Andar& andar, int numeroDoAndar, int chegadaX, int chegadaY) {
    andar.numNpcs = 0;
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, chegadaX, chegadaY, distancia);

    for (int tipo = 0; tipo < NUM_TIPOS_DE_NPC && andar.numNpcs < MAX_NPCS; tipo++) {
        if (!apareceNoAndar(FICHAS_DOS_NPCS[tipo], numeroDoAndar)) continue;
        // O comerciante fica no balcão da lojinha dele.
        if (tipo == NPC_COMERCIANTE && andar.loja.existe) {
            Npc& npc = andar.npcs[andar.numNpcs++];
            npc.tipo = tipo;
            npc.x = andar.loja.balcaoX;
            npc.y = andar.loja.balcaoY;
            npc.conversou = false;
            npc.estoque[0] = npc.estoque[1] = -1;
            npc.estoqueExtra = -1;
            npc.extrasAVenda[0] = npc.extrasAVenda[1] = 0;
            for (int k = 0; k < 4; k++) npc.pocoesAVenda[k] = 0;
            int item = itemEm(andar, npc.x, npc.y);
            if (item >= 0) andar.itens[item].ativo = false;   // nada embaixo do balcão
            protegerSalaDaLoja(andar);
            continue;
        }
        // O lugar mais perto da chegada que esteja a pelo menos 4 passos dela.
        int melhorX = -1, melhorY = -1, melhor = 1000000;
        for (int y = 1; y < MAPA_ALTURA - 1; y++) {
            for (int x = 1; x < MAPA_LARGURA - 1; x++) {
                int d = distancia[y][x];
                if (andar.mapa.celulas[y][x] != CHAO || d == INALCANCAVEL || d < 4 || d >= melhor) continue;
                // Só no meio de uma sala (8 vizinhas livres): o NPC nunca tranca um corredor.
                bool livreEmVolta = true;
                for (int k = 0; k < 8; k++) {
                    TipoCelula v = andar.mapa.celulas[y + DIRECOES_DY[k]][x + DIRECOES_DX[k]];
                    if (v != CHAO) livreEmVolta = false;
                }
                if (!livreEmVolta) continue;
                if (monstroEm(andar, x, y) >= 0 || itemEm(andar, x, y) >= 0 || armadilhaEm(andar, x, y) >= 0) continue;
                if (npcEm(andar, x, y) >= 0) continue;
                melhor = d;
                melhorX = x;
                melhorY = y;
            }
        }
        if (melhorX < 0) {
            // Sem lugar para o Borin: a porta já fica aberta (este andar não tem chave).
            if (tipo == NPC_MINEIRO && andar.tesouro.existe) andar.mapa.celulas[andar.tesouro.portaY][andar.tesouro.portaX] = PORTA;
            continue;
        }
        Npc& npc = andar.npcs[andar.numNpcs++];
        npc.tipo = tipo;
        npc.x = melhorX;
        npc.y = melhorY;
        npc.conversou = false;
        npc.estoque[0] = npc.estoque[1] = -1;
        npc.estoqueExtra = -1;
        npc.extrasAVenda[0] = npc.extrasAVenda[1] = 0;
        for (int k = 0; k < 4; k++) npc.pocoesAVenda[k] = 0;
        if (tipo == NPC_COMERCIANTE) {
            // Sem salinha: um tapete 5x5 em volta dele vira a área protegida.
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    int x = melhorX + dx, y = melhorY + dy;
                    if (dentroDoMapa(x, y) && andar.mapa.celulas[y][x] == CHAO && distancia[y][x] != INALCANCAVEL)
                        andar.zonaSegura[y][x] = true;
                }
            protegerSalaDaLoja(andar);
        }
    }
}

int npcEm(const Andar& andar, int x, int y) {
    for (int i = 0; i < andar.numNpcs; i++) {
        if (andar.npcs[i].x == x && andar.npcs[i].y == y) return i;
    }
    return -1;
}

// Abre a sala do tesouro deste andar, se ela ainda estiver trancada.
// Devolve true se a porta foi aberta.
static bool abrirSalaDoTesouro(Andar& andar) {
    if (!andar.tesouro.existe) return false;
    TipoCelula& porta = andar.mapa.celulas[andar.tesouro.portaY][andar.tesouro.portaX];
    if (porta != PORTA_TRANCADA) return false;
    porta = PORTA;
    return true;
}

// Já falou com alguém deste tipo em algum andar? (As pessoas se conhecem:
// o Borin pergunta da Eldra, o Aldric sabe de quem você ajudou...)
static bool conversouCom(const Jogo& jogo, int tipo) {
    for (int a = 0; a < NUM_ANDARES; a++) {
        const Andar& andar = jogo.andares[a];
        for (int i = 0; i < andar.numNpcs; i++)
            if (andar.npcs[i].tipo == tipo && andar.npcs[i].conversou) return true;
    }
    return false;
}

// Uma frase entre várias, sem usar o sorteio do jogo (que decide o mapa e as
// batalhas): muda com os passos dados.
static std::string umaDelas(const Jogo& jogo, const std::vector<std::string>& frases) {
    return frases[(size_t)jogo.passos % frases.size()];
}

// AS FALAS
// Cada linha vazia separa um balão. Linhas entre aspas são da pessoa; com
// "> " na frente, é o herói respondendo; sem aspas, é a narração da cena.
// "(+1 Poção de Vida)" no fim de um balão aparece em verde, como prêmio.
void conversarComNpc(Jogo& jogo, int indice) {
    Andar& andar = andarAtual(jogo);
    Npc& npc = andar.npcs[indice];
    Jogador& j = jogo.jogador;
    const std::string nome = j.nome;
    std::vector<std::string> falas;

    // O comerciante: na primeira vez em cada andar ele conversa; depois, abre direto a loja.
    if (npc.tipo == NPC_COMERCIANTE) {
        if (!npc.conversou) {
            prepararEstoque(jogo, npc);
            bool jaConhece = conversouCom(jogo, NPC_COMERCIANTE);
            if (!jaConhece) {
                falas = {
                    "Um homem baixinho arruma garrafas numa banca improvisada, assobiando como se estivesse "
                    "numa feira.",
                    "",
                    "\"Um cliente! E vivo! Faz semanas que só aparece goblin por aqui... e goblin não paga nada.\"",
                    "",
                    "> \"Uma loja... aqui dentro da masmorra?\"",
                    "",
                    "\"Garro, ao seu dispor! Onde tem aventureiro desesperado, tem bom negócio. Poções, armas, "
                    "escudos e umas bugigangas muito úteis.\"",
                    "",
                    "\"Dica de graça, porque gostei da sua cara: os monstros carregam moedas. Algumas caixas "
                    "também. Junte tudo!\"",
                    "",
                    "\"Vou descendo junto com você. Me procure no sexto e no nono andar... se eu ainda estiver "
                    "inteiro, claro!\"",
                };
            } else if (jogo.andar < NUM_ANDARES - 1) {
                falas = {
                    "\"" + nome + "! Eu disse que a gente se via de novo!\"",
                    "",
                    "\"Trouxe mercadoria nova... e preços novos. A descida também não é de graça para mim, "
                    "sabia?\"",
                    "",
                    "\"Daqui para baixo o ar fica pesado. Não economize nas poções.\"",
                };
            } else {
                falas = {
                    "Garro olha para os lados e fala baixinho.",
                    "",
                    "\"Psiu! " + nome + "! O Senhor das Profundezas está logo abaixo. Dá para sentir o chão "
                    "tremendo.\"",
                    "",
                    "\"Moeda não serve de nada na barriga de um monstro. Gaste tudo, gaste tudo!\"",
                };
            }
            npc.conversou = true;
            if (!jogo.modoAutomatico) mostrarFala(jogo, indice, falas);
        }
        abrirLoja(jogo, npc);
        return;
    }

    // O viajante perdido: troca 1 Poção de Vida (ou 30 moedas) por um cristal.
    // (npc.estoque[0] == 1 marca que ele já se apresentou.)
    if (npc.tipo == NPC_VIAJANTE) {
        if (npc.conversou) {
            if (!jogo.modoAutomatico)
                mostrarFala(jogo, indice, { umaDelas(jogo, {
                    "\"Obrigado de novo, " + nome + "! O braço já nem dói tanto.\"",
                    "\"Se achar a saída... volta para me buscar? Promete?\"",
                    "\"Eu fico aqui, quietinho. Os monstros nem me notam mais.\"" }) });
            return;
        }
        if (jogo.modoAutomatico) return;
        const int PRECO = 30;
        std::vector<std::string> opcoes = { "Dar 1 Poção de Vida", "Pagar " + std::to_string(PRECO) + " moedas", "Agora não" };
        std::vector<std::string> descricoes = {
            "Você tem " + std::to_string(j.mochila[I_POCAO_DE_VIDA]) + " Poção(ões) de Vida.  |  Em troca: um cristal mágico (efeito permanente).",
            "Você tem " + std::to_string(j.moedas) + " moedas.  |  Em troca: um cristal mágico (efeito permanente).",
            "Deixar para depois.",
        };
        if (npc.estoque[0] != 1) {
            npc.estoque[0] = 1;
            mostrarFala(jogo, indice, {
                "Encostado num canto, um rapaz de capa rasgada aperta o próprio braço ferido.",
                "",
                "\"Ei... ei! Você é de verdade? Não é mais uma sombra? Que alívio! Meu nome é Tobias. "
                "Estou perdido aqui faz dias.\"",
                "",
                "> \"Eu sou " + nome + ". O que aconteceu com você?\"",
                "",
                "\"Entrei atrás de um tesouro, acredita? E achei! Só que também achei um ogro. "
                "Adivinha quem ganhou.\"",
                "",
                "\"Se você me arrumar uma Poção de Vida, ou umas moedas para comprar uma do Garro, "
                "o tesouro é seu: um cristal mágico de verdade.\"",
            });
        } else {
            mostrarFala(jogo, indice, {
                "\"" + nome + "! Pensou na minha proposta? Uma Poção de Vida, ou umas moedas, pelo meu cristal.\"",
            });
        }
        int escolha = escolherEmCaixa(jogo, "TOBIAS, O VIAJANTE: o que você faz?", opcoes, descricoes, 0);
        if (escolha == 0 && j.mochila[I_POCAO_DE_VIDA] <= 0) {
            adicionarMensagem(jogo, "Você não tem nenhuma Poção de Vida.");
            return;
        }
        if (escolha == 1 && j.moedas < PRECO) {
            adicionarMensagem(jogo, "Você não tem moedas suficientes (" + std::to_string(j.moedas) + "/" + std::to_string(PRECO) + ").");
            return;
        }
        if (escolha != 0 && escolha != 1) {
            mostrarFala(jogo, indice, { "\"Tudo bem... eu espero aqui. Não tenho para onde ir mesmo.\"" });
            return;
        }
        if (escolha == 0) j.mochila[I_POCAO_DE_VIDA]--;
        else j.moedas -= PRECO;
        npc.conversou = true;
        tocarSom(SOM_ITEM);
        mostrarFala(jogo, indice, {
            "\"Muito obrigado, " + nome + "! Tome, é seu. Brilha bonito, não é?\"",
            "",
            "\"E se um dia você achar a saída... volta para me buscar?\"",
        });
        adicionarMensagem(jogo, "Tobias te deu um cristal mágico!", MSG_BOA);
        int cristais[4] = { I_CRISTAL_DE_VIDA, I_CRISTAL_DE_FORCA, I_CRISTAL_DE_FERRO, I_PENA_DO_VENTO };
        absorverCristal(jogo, cristais[sortear(jogo.sorte, 0, 3)]);
        return;
    }

    switch (npc.tipo) {
        case NPC_EXPLORADORA:
            if (npc.conversou) {
                falas = { umaDelas(jogo, {
                    "\"Ainda por aqui, " + nome + "? Desça! E olhe o elemento antes de atacar.\"",
                    "\"Minha perna está melhorando... devagar. Vai, que o Senhor das Profundezas não vai cair "
                    "sozinho.\"",
                    "\"Se encontrar o Borin, diga que eu mandei um abraço. Ele vai fingir que não gostou.\"" }) };
                break;
            }
            falas = {
                "Uma mulher está sentada, encostada na parede, com a perna enfaixada e um mapa amassado no colo.",
                "",
                "\"Passos... e não são de rato. Calma, eu não mordo! Meu nome é Eldra. Sou exploradora... "
                "ou era, até esta perna resolver me trair.\"",
                "",
                "> \"Eu sou " + nome + ". Vim acabar com o Senhor das Profundezas.\"",
                "",
                "\"Acabar com ele? Gostei de você, " + nome + "! Então as portas também se fecharam atrás de você. "
                "Elas fazem isso com todo mundo que entra.\"",
                "",
                "\"Cheguei até o quarto andar antes de voltar assim. Aprendi umas coisas: os monstros mudam de "
                "elemento a cada vez que alguém entra. Olhe sempre antes de atacar.\"",
                "",
                "\"E está vendo essas caixas? Empurre uma contra um monstro e ele fica tonto. Quebradas, às vezes "
                "escondem poções e armas.\"",
                "",
                "\"Tome. Eu guardei esta poção para a volta, mas não vou a lugar nenhum tão cedo. Nas suas mãos "
                "ela serve mais.\"  (+1 Poção de Vida)",
                "",
                "\"Se chegar no quarto andar, procure o Borin, um velho mineiro. Diga que fui eu que mandei. "
                "Ele vai resmungar, mas ajuda.\"",
            };
            guardarOuLargar(jogo, I_POCAO_DE_VIDA);
            break;

        case NPC_MINEIRO:
            if (npc.conversou) {
                falas = { umaDelas(jogo, {
                    "\"Lá embaixo os ogros quebram tudo o que veem pela frente. Até as minhas caixas!\"",
                    "\"Ainda aqui? Vai, vai! E se vir um gnomo com uma corneta, acabe com ele antes que ele "
                    "assopre.\"",
                    "\"Quarenta anos cavando estes túneis... e agora estão cheios de goblin. Que desaforo.\"" }) };
                break;
            }
            falas = {
                "Um velho de capacete amassado bate com uma picareta na parede, resmungando sozinho.",
                "",
                "\"Ei! Cuidado onde pisa, que essa pedra aí é mais velha que você! ...Hm. Você não é um monstro. "
                "Já é alguma coisa.\"",
                "",
            };
            if (conversouCom(jogo, NPC_EXPLORADORA)) {
                falas.push_back("> \"Sou " + nome + ". A Eldra disse que você podia ajudar.\"");
                falas.push_back("");
                falas.push_back("\"A Eldra! Aquela teimosa ainda está viva? Boa notícia. Eu sou Borin. Esta mina era "
                                "minha, sabia? Quarenta anos cavando estes túneis.\"");
            } else {
                falas.push_back("> \"Sou " + nome + ". Estou descendo até o fim da masmorra.\"");
                falas.push_back("");
                falas.push_back("\"Até o fim? Coragem não falta. Eu sou Borin. Esta mina era minha, sabia? Quarenta "
                                "anos cavando estes túneis.\"");
            }
            falas.push_back("");
            falas.push_back("\"Até o Senhor das Profundezas aparecer e encher tudo de goblin. Os meus túneis!\"");
            falas.push_back("");
            if (abrirSalaDoTesouro(andar)) {
                falas.push_back("\"Mas uma coisa ele não me tirou: a chave mestra. Abre qualquer porta destas minas, "
                                "inclusive a da sala do tesouro deste andar.\"");
                falas.push_back("");
                falas.push_back("Borin some no corredor com uma chave enorme. Pouco depois, você ouve uma fechadura "
                                "se abrir em algum lugar do andar.");
                falas.push_back("");
                falas.push_back("\"Pronto! O que tiver lá dentro é seu. Só não conte para os goblins.\"");
                adicionarMensagem(jogo, "Borin destrancou a sala do tesouro deste andar!", MSG_BOA);
                tocarSom(SOM_PORTA);
            } else {
                guardarOuLargar(jogo, I_ETER);
                falas.push_back("\"Não sobrou porta para abrir neste andar... mas leve este Éter. Eu usava para não "
                                "dormir no turno da noite.\"  (+1 Éter)");
            }
            break;

        case NPC_ESPIRITO:
            if (npc.conversou) {
                falas = { umaDelas(jogo, {
                    "\"Ele está acordado lá embaixo. Eu sinto.\"",
                    "\"Não olhe para trás, " + nome + ". Daqui, o único caminho é para baixo.\"",
                    "\"Quando ele ficar furioso, não se assuste. É sinal de que está doendo.\"" }) };
                break;
            }
            falas = {
                "O ar esfria. Uma luz azulada se junta no meio da sala e toma a forma de um velho guerreiro "
                "de armadura.",
                "",
                "\"Não tenha medo, " + nome + ". Eu sou... eu era... Aldric, o último guardião desta mina.\"",
                "",
            };
            if (conversouCom(jogo, NPC_EXPLORADORA) || conversouCom(jogo, NPC_MINEIRO))
                falas.push_back("\"Eu vi você pelo caminho. A exploradora, o velho mineiro... Faz tempo que alguém "
                                "não chegava tão fundo.\"");
            else
                falas.push_back("\"Faz tempo que alguém não chegava tão fundo. Muito tempo.\"");
            falas.push_back("");
            falas.push_back("\"Quando o Senhor das Profundezas subiu, eu fiquei para segurá-lo aqui embaixo. "
                            "Segurei por cem anos. Mas não consigo mais.\"");
            falas.push_back("");
            falas.push_back("\"Ele espera no décimo andar. Dele não se foge: só se vence. E, quando estiver "
                            "ferido, vai ficar furioso. Não se assuste.\"");
            falas.push_back("");
            falas.push_back("\"Ainda tenho força para uma última coisa. Fique parado.\"");
            falas.push_back("");
            falas.push_back("A luz te envolve, morna. Suas feridas se fecham.  (vida e PP cheios)");
            falas.push_back("");
            falas.push_back("\"Vá. Quando ele cair, a minha guarda também termina.\"");
            j.vida = j.vidaMaxima;
            for (int s = 0; s < j.numGolpes; s++) j.pp[s] = GOLPES[j.golpes[s]].ppMaximo;
            break;
    }
    npc.conversou = true;
    if (!jogo.modoAutomatico) mostrarFala(jogo, indice, falas);
}
