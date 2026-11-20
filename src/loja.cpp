// loja.cpp
// -----------------------------------------------------------------------------
// Moedas e a loja do comerciante (ver loja.h).
// -----------------------------------------------------------------------------
#include "loja.h"
#include "jogo.h"
#include "npcs.h"
#include "aleatorio.h"
#include "som.h"
#include "tela.h"
#include "dificuldade.h"

// =============================================================================
// PREÇOS
// =============================================================================
// Um valor para cada item, na mesma ordem do enum IdItem (itens.h).
static const int PRECOS[NUM_TIPOS_DE_ITEM] = {
    20,  // Poção de Vida
    45,  // Poção Grande
    12,  // Antídoto
    20,  // Éter
    2,   // Adaga (ninguém compra, mas dá para vender)
    25,  // Espada Curta
    40,  // Machado
    80,  // Espada Longa
    45,  // Espada de Fogo
    45,  // Tridente
    45,  // Chicote de Espinhos
    45,  // Adaga Venenosa
    45,  // Martelo de Pedra
    20,  // Escudo de Madeira
    45,  // Escudo de Ferro
    60,  // Escudo de Torre
    70,  // Cristal de Vida
    70,  // Cristal de Força
    70,  // Cristal de Ferro
    70,  // Pena do Vento
    35,  // Pergaminho de Fogo
    35,  // Pergaminho de Água
    35,  // Pergaminho de Planta
    35,  // Pergaminho de Veneno
    35,  // Pergaminho de Pedra
    0,   // Chave (não está à venda)
    90,  // Mochila de Couro
    70,  // Lanterna
    30,  // Bomba de Fumaça
    25,  // Pergaminho do Mapa
};

int precoDeCompra(int tipoDeItem) {
    if (tipoDeItem < 0 || tipoDeItem >= NUM_TIPOS_DE_ITEM) return 0;
    // Fácil: 25% mais barato; Difícil: 25% mais caro (arredondado).
    return (PRECOS[tipoDeItem] * dificuldadeAtual().precosDaLoja + 50) / 100;
}

int precoDeVenda(int tipoDeItem) {
    if (tipoDeItem < 0 || tipoDeItem >= NUM_TIPOS_DE_ITEM || PRECOS[tipoDeItem] <= 0) return 0;
    int preco = PRECOS[tipoDeItem] / 2;   // metade do preço normal, em qualquer dificuldade
    return preco > 1 ? preco : 1;
}

// =============================================================================
// GANHAR MOEDAS
// =============================================================================
int moedasDoMonstro(Jogo& jogo, int nivelDoMonstro, bool ehChefe) {
    if (ehChefe) return 100;
    // Monstros mais fortes carregam um pouco mais (e sempre tem um pouco de sorte).
    return 2 + nivelDoMonstro / 3 + sortear(jogo.sorte, 0, 2);
}

int moedasDaCaixa(int numeroDoAndar, int sorteio) {
    return 5 + 2 * numeroDoAndar + sorteio % 6;
}

// =============================================================================
// O ESTOQUE
// =============================================================================
// Poções: o Garro traz uma quantidade limitada de cada uma por andar.
static const int POCOES_DA_LOJA[] = { I_POCAO_DE_VIDA, I_POCAO_GRANDE, I_ANTIDOTO, I_ETER };
const int NUM_POCOES_DA_LOJA = 4;
//                                          Vida Grande Antídoto Éter
static const int POCOES_NO_ANDAR_3[NUM_POCOES_DA_LOJA] = { 1, 0, 1, 1 };
static const int POCOES_MAIS_FUNDO[NUM_POCOES_DA_LOJA] = { 1, 0, 1, 1 };
static const int POCOES_NO_ANDAR_9[NUM_POCOES_DA_LOJA] = { 1, 1, 1, 1 };

// Itens de suporte gastáveis: também em quantidade limitada por andar.
static const int EXTRAS_DA_LOJA[2] = { I_BOMBA_DE_FUMACA, I_PERGAMINHO_DO_MAPA };

int estoqueDaLoja(const Npc& comerciante, int tipoDeItem) {
    for (int i = 0; i < NUM_POCOES_DA_LOJA; i++) {
        if (POCOES_DA_LOJA[i] == tipoDeItem) return comerciante.pocoesAVenda[i];
    }
    for (int i = 0; i < 2; i++) {
        if (EXTRAS_DA_LOJA[i] == tipoDeItem) return comerciante.extrasAVenda[i];
    }
    if (comerciante.estoqueExtra == tipoDeItem) return 1;
    for (int k = 0; k < 2; k++) {
        if (comerciante.estoque[k] == tipoDeItem) return 1;
    }
    return 0;
}

// =============================================================================
// COMPRAR E VENDER
// =============================================================================
bool comprarItem(Jogo& jogo, Npc& comerciante, int tipoDeItem) {
    Jogador& j = jogo.jogador;
    int preco = precoDeCompra(tipoDeItem);
    if (preco <= 0 || j.moedas < preco || !cabeNaMochila(j, tipoDeItem)) return false;
    if (estoqueDaLoja(comerciante, tipoDeItem) <= 0) return false;
    // Tira do estoque do comerciante.
    bool ehPocao = false;
    for (int i = 0; i < NUM_POCOES_DA_LOJA; i++) {
        if (POCOES_DA_LOJA[i] == tipoDeItem) { comerciante.pocoesAVenda[i]--; ehPocao = true; }
    }
    for (int i = 0; i < 2 && !ehPocao; i++) {
        if (EXTRAS_DA_LOJA[i] == tipoDeItem) { comerciante.extrasAVenda[i]--; ehPocao = true; }
    }
    if (!ehPocao && comerciante.estoqueExtra == tipoDeItem) { comerciante.estoqueExtra = -1; ehPocao = true; }
    for (int k = 0; k < 2 && !ehPocao; k++) {
        if (comerciante.estoque[k] == tipoDeItem) { comerciante.estoque[k] = -1; break; }
    }
    j.moedas -= preco;
    // Mochila de Couro e Lanterna valem na hora (não vão para a mochila).
    if (usaNaHora(tipoDeItem) && TIPOS_DE_ITEM[tipoDeItem].categoria != CRISTAL) {
        ganharItemPermanente(jogo, tipoDeItem);
        return true;
    }
    j.mochila[tipoDeItem]++;

    const TipoItem& t = TIPOS_DE_ITEM[tipoDeItem];
    // Arma ou escudo melhor que o da mão: já sai equipado.
    if ((t.categoria == ARMA && (j.arma == nullptr || t.bonusAtaque > j.arma->bonusAtaque)) ||
        (t.categoria == ESCUDO && (j.escudo == nullptr || t.bonusDefesa > j.escudo->bonusDefesa))) {
        equipar(jogo, tipoDeItem);
    }
    // Cristais fazem efeito na hora (como quando são pegos no chão).
    if (t.categoria == CRISTAL) {
        j.mochila[tipoDeItem]--;
        absorverCristal(jogo, tipoDeItem);
    }
    return true;
}

bool venderItem(Jogo& jogo, int tipoDeItem) {
    Jogador& j = jogo.jogador;
    int preco = precoDeVenda(tipoDeItem);
    if (preco <= 0 || j.mochila[tipoDeItem] <= 0) return false;
    j.mochila[tipoDeItem]--;
    j.moedas += preco;
    return true;
}

// =============================================================================
// O QUE ELE TRAZ EM CADA ANDAR
// =============================================================================
void prepararEstoque(const Jogo& jogo, Npc& comerciante) {
    for (int i = 0; i < NUM_POCOES_DA_LOJA; i++) {
        comerciante.pocoesAVenda[i] = (jogo.andar <= 3) ? POCOES_NO_ANDAR_3[i] : (jogo.andar >= 9 ? POCOES_NO_ANDAR_9[i] : POCOES_MAIS_FUNDO[i]);
    }
    comerciante.estoque[0] = comerciante.estoque[1] = -1;
    // Suporte: bombas e pergaminhos do mapa, e um item permanente que o
    // jogador ainda não tenha (a mochila primeiro, depois a lanterna).
    comerciante.extrasAVenda[0] = (jogo.andar <= 3) ? 1 : 2;
    comerciante.extrasAVenda[1] = 1;
    const Jogador& j = jogo.jogador;
    int primeiro = (jogo.andar >= 6) ? I_LANTERNA : I_MOCHILA_DE_COURO;
    int segundo = (primeiro == I_LANTERNA) ? I_MOCHILA_DE_COURO : I_LANTERNA;
    bool temPrimeiro = (primeiro == I_LANTERNA) ? j.lanterna : j.mochilaDeCouro;
    bool temSegundo = (segundo == I_LANTERNA) ? j.lanterna : j.mochilaDeCouro;
    comerciante.estoqueExtra = !temPrimeiro ? primeiro : (!temSegundo ? segundo : -1);
    if (jogo.andar <= 3) {
        // Andar 3: uma arma do elemento do jogador (+25% nos golpes dele).
        comerciante.estoque[0] = I_MACHADO;
        for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
            const TipoItem& t = TIPOS_DE_ITEM[i];
            if (t.categoria == ARMA && t.elemento >= 0 && t.elemento == (int)jogo.jogador.elemento) {
                comerciante.estoque[0] = i;
            }
        }
        comerciante.estoque[1] = I_ESCUDO_DE_MADEIRA;
    } else if (jogo.andar <= 6) {
        comerciante.estoque[0] = I_ESCUDO_DE_FERRO;
        comerciante.estoque[1] = I_CRISTAL_DE_VIDA;
    } else {
        comerciante.estoque[0] = I_ESPADA_LONGA;
        comerciante.estoque[1] = I_CRISTAL_DE_FORCA;
    }
}

// =============================================================================
// A TELA DA LOJA
// =============================================================================
static std::string emMoedas(int valor) {
    return std::to_string(valor) + (valor == 1 ? " moeda" : " moedas");
}

// Segunda linha da descrição: quanto o jogador já tem, ou quanto falta.
static std::string situacao(const Jogador& j, const Npc& comerciante, int tipo) {
    if (estoqueDaLoja(comerciante, tipo) <= 0) return "Esgotado: o Garro não tem mais neste andar.";
    int preco = precoDeCompra(tipo);
    if (j.moedas < preco) return "Faltam " + emMoedas(preco - j.moedas) + " para comprar.";
    if (!cabeNaMochila(j, tipo)) return motivoDeNaoCaber(j, tipo) + " Venda ou use algo antes.";
    const TipoItem& t = TIPOS_DE_ITEM[tipo];
    if (t.categoria == CRISTAL) return "Efeito na hora: o cristal é usado assim que você compra.";
    if (usaNaHora(tipo)) return "Vale na hora e para a partida toda (não ocupa a mochila).";
    if (j.arma == &t || j.escudo == &t) return "Você já está usando um desses.";
    return "Na sua mochila: " + std::to_string(j.mochila[tipo]) + " de " + std::to_string(limiteDoItem(j, tipo)) +
           "   (mochila: " + std::to_string(itensNaMochila(j)) + "/" + std::to_string(limiteDaMochila(j)) + ")";
}

// Vender: lista o que há na mochila (o equipamento em uso não aparece).
static void telaDeVenda(Jogo& jogo, const std::string& nomeDoComerciante) {
    Jogador& j = jogo.jogador;
    int cursor = 0;
    while (true) {
        std::vector<int> tipos;
        std::vector<std::string> opcoes, descricoes;
        for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
            if (j.mochila[i] <= 0 || precoDeVenda(i) <= 0) continue;
            tipos.push_back(i);
            opcoes.push_back(std::string(TIPOS_DE_ITEM[i].nome) + "  x" + std::to_string(j.mochila[i]) +
                             "   ·   +" + emMoedas(precoDeVenda(i)));
            descricoes.push_back(std::string(TIPOS_DE_ITEM[i].descricao) + "  |  " + nomeDoComerciante +
                                 " paga " + emMoedas(precoDeVenda(i)) + " por um.");
        }
        opcoes.push_back("Voltar");
        descricoes.push_back(tipos.empty() ? "Você não tem nada que eu queira comprar." : "Voltar para as compras.");
        std::string titulo = "VENDER   (suas moedas: " + std::to_string(j.moedas) + ")";
        int escolha = escolherEmCaixa(jogo, titulo, opcoes, descricoes, cursor);
        if (escolha < 0 || escolha >= (int)tipos.size()) return;
        cursor = escolha;
        int tipo = tipos[escolha];
        if (venderItem(jogo, tipo)) {
            tocarSom(SOM_ITEM);
            adicionarMensagem(jogo, "Você vendeu " + nomeComArtigo(tipo) + " por " + emMoedas(precoDeVenda(tipo)) + ".");
        }
    }
}

void abrirLoja(Jogo& jogo, Npc& comerciante) {
    if (jogo.modoAutomatico) return;   // os testes automáticos compram direto
    tocarMusica(MUSICA_GARRO);         // a musiquinha da loja
    Jogador& j = jogo.jogador;
    const std::string nome = "Garro";
    int cursor = 0;
    while (true) {
        // Monta a lista a cada volta: o estoque e as moedas mudam.
        std::vector<int> tipos;
        for (int i = 0; i < NUM_POCOES_DA_LOJA; i++) {
            // Poção que ele nem trouxe (ex.: Poção Grande no andar 3) não aparece.
            const int* trouxe = (jogo.andar <= 3) ? POCOES_NO_ANDAR_3 : POCOES_MAIS_FUNDO;
            if (trouxe[i] > 0) tipos.push_back(POCOES_DA_LOJA[i]);
        }
        for (int i = 0; i < 2; i++) tipos.push_back(EXTRAS_DA_LOJA[i]);   // bomba e pergaminho do mapa
        int primeiroEspecial = (int)tipos.size();
        for (int k = 0; k < 2; k++) if (comerciante.estoque[k] >= 0) tipos.push_back(comerciante.estoque[k]);
        if (comerciante.estoqueExtra >= 0) tipos.push_back(comerciante.estoqueExtra);

        std::vector<std::string> opcoes, descricoes;
        for (int i = 0; i < (int)tipos.size(); i++) {
            int t = tipos[i];
            int resta = estoqueDaLoja(comerciante, t);
            std::string marca = (i >= primeiroEspecial) ? "  (só 1!)"
                              : (resta > 0 ? "  (restam " + std::to_string(resta) + ")" : "  (esgotado)");
            opcoes.push_back(std::string(TIPOS_DE_ITEM[t].nome) + marca + "   ·   " + emMoedas(precoDeCompra(t)));
            descricoes.push_back(std::string(TIPOS_DE_ITEM[t].descricao) + "  |  " + situacao(j, comerciante, t));
        }
        int opcaoVender = (int)opcoes.size();
        opcoes.push_back("Vender itens");
        descricoes.push_back("Troque o que você não usa mais por moedas (metade do preço).");
        opcoes.push_back("Sair");
        descricoes.push_back("\"Volte sempre! Bom, se sobreviver.\"");

        std::string titulo = "LOJA DO GARRO   (moedas: " + std::to_string(j.moedas) + "   mochila: " +
                             std::to_string(itensNaMochila(j)) + "/" + std::to_string(limiteDaMochila(j)) + ")";
        int escolha = escolherEmCaixa(jogo, titulo, opcoes, descricoes, cursor);
        if (escolha < 0 || escolha > opcaoVender) { tocarMusica(MUSICA_MASMORRA); return; }   // Esc ou "Sair"
        cursor = escolha;
        if (escolha == opcaoVender) { telaDeVenda(jogo, nome); continue; }

        int tipo = tipos[escolha];
        int preco = precoDeCompra(tipo);
        if (!comprarItem(jogo, comerciante, tipo)) {
            tocarSom(SOM_ERROU);
            continue;   // a descrição já mostra o motivo (esgotado, moedas, mochila)
        }
        tocarSom(SOM_ITEM);
        adicionarMensagem(jogo, "Você comprou " + nomeComArtigo(tipo) + " por " + emMoedas(preco) + ".", MSG_BOA);
    }
}
