// loja.h
// -----------------------------------------------------------------------------
// Moedas e a loja do comerciante.
//
// O jogador ganha moedas ao derrotar monstros, ao subir de nível e quebrando
// caixas. Com elas, compra itens do comerciante (andares 3, 6 e 9) e também
// pode vender o que não usa mais pela metade do preço. As moedas que sobrarem
// no fim da partida valem pontos.
// -----------------------------------------------------------------------------
#pragma once

struct Jogo;
struct Npc;

// Preço de cada item na loja (0 = a loja não trabalha com esse item).
// Muda com a dificuldade: Fácil 75%, Médio 100%, Difícil 125%.
int precoDeCompra(int tipoDeItem);
// Quanto o comerciante paga por um item seu (metade do preço, no mínimo 1).
int precoDeVenda(int tipoDeItem);

// Moedas ganhas ao derrotar um monstro desse nível (com um pouco de sorte).
int moedasDoMonstro(Jogo& jogo, int nivelDoMonstro, bool ehChefe);
// Bônus de moedas a cada nível novo.
const int MOEDAS_POR_NIVEL = 5;
// Moedas guardadas numa caixa deste andar.
int moedasDaCaixa(int numeroDoAndar, int sorteio);

// Quantos desse item o comerciante ainda tem (poções: poucas por andar;
// itens especiais: só 1).
int estoqueDaLoja(const Npc& comerciante, int tipoDeItem);

// Compra e venda (sem tela; devolvem false se não deu: sem moedas, sem
// estoque ou sem espaço na mochila).
bool comprarItem(Jogo& jogo, Npc& comerciante, int tipoDeItem);
bool venderItem(Jogo& jogo, int tipoDeItem);

// Os 1 ou 2 itens especiais que o comerciante traz neste andar
// (uma arma do elemento do jogador, um escudo melhor, um cristal...).
void prepararEstoque(const Jogo& jogo, Npc& comerciante);

// Abre a tela da loja (comprar, vender, sair).
void abrirLoja(Jogo& jogo, Npc& comerciante);
