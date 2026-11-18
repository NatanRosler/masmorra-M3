// pontuacao.h
// -----------------------------------------------------------------------------
// Etapa 9: pontuação, ranking salvo em arquivo e os textos do menu
// (história, como jogar, itens e pontuação).
// -----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

struct Jogo;

// Pontuação:
//   + 250 por andar alcançado (o mais fundo)
//   +  30 por monstro derrotado
//   +  20 por item coletado
//   -   1 a cada 5 passos (movimentos)
//   - 300 por vida perdida
//   +3000 por derrotar o chefe final
//   +   1 por moeda guardada
// e o total é multiplicado pela dificuldade (fácil 50%, médio 100%, difícil 150%).
int calcularPontuacao(const Jogo& jogo);

struct RegistroRanking {
    std::string nome;
    int pontos;
    int andar;
    bool venceu;
    std::string dificuldade;   // "Fácil", "Médio" ou "Difícil"
};

const int TAMANHO_DO_RANKING = 10;
extern const char* ARQUIVO_DO_RANKING;

// Lê o ranking do arquivo (os 10 melhores, do maior para o menor).
std::vector<RegistroRanking> lerRanking();

// Fim da partida: calcula a pontuação final e grava no ranking (só uma vez).
void registrarNoRanking(Jogo& jogo);

// Textos das telas.
std::vector<std::string> textoDaHistoria(const std::string& nomeDoJogador);
std::vector<std::string> textoComoJogar();
std::vector<std::string> textoDosItens();
std::vector<std::string> textoDaPontuacao();

// Mostra uma página de texto e espera uma tecla.
void mostrarPagina(const std::string& titulo, const std::vector<std::string>& linhas);
