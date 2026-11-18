// salvar.h
// -----------------------------------------------------------------------------
// Salvar e continuar a partida (arquivo binário "save.dat").
//
// O jogo salva sozinho ao sair (pausa, Q, fechar a janela) e a cada troca de
// andar. No menu principal aparece "Continuar". Como num roguelike, o save é
// apagado quando a partida acaba (vitória ou derrota): não dá para voltar no
// tempo depois de morrer.
//
// O arquivo é gravado campo por campo (nada de ponteiros): a arma e o escudo
// viram o número do item na tabela, e cada texto é gravado como tamanho +
// letras. Um cabeçalho com uma "assinatura", a versão e o tamanho das
// estruturas impede de carregar um arquivo de outra versão do jogo.
// -----------------------------------------------------------------------------
#pragma once

#include <string>

struct Jogo;

extern const char* ARQUIVO_DO_SAVE;

// Grava a partida. Devolve false se não conseguiu (o jogo continua normalmente).
bool salvarPartida(const Jogo& jogo);

// Lê a partida salva para dentro de "jogo". Devolve false se não existe save
// ou se ele é de outra versão / está corrompido (nesse caso "jogo" fica inválido).
bool carregarPartida(Jogo& jogo);

// Existe uma partida salva? "resumo" recebe algo como
// "Nata  ·  Andar 4  ·  Nível 9  ·  Médio".
bool existePartidaSalva(std::string* resumo = nullptr);

void apagarPartidaSalva();

// Carrega e deixa tudo pronto para jogar (música, mensagem de boas-vindas).
bool continuarPartida(Jogo& jogo);

// A partida que está na tela: se a janela for fechada no meio de qualquer
// menu ou batalha, ela é salva antes de o programa terminar.
void definirPartidaEmAndamento(Jogo* jogo);
void salvarPartidaEmAndamento();
