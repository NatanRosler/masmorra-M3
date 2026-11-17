// terminal.h
// -----------------------------------------------------------------------------
// Tudo o que depende do sistema operacional fica AQUI (e em terminal.cpp).
// O resto do jogo só chama estas funções e não sabe se está no Windows,
// no Linux ou no Mac.
//
// Cores e posição do cursor usam "códigos ANSI" (sequências que começam com
// o caractere ESC, "\x1b"). Eles funcionam nos três sistemas — no Windows 10/11
// basta ativá-los uma vez, o que iniciarTerminal() já faz.
// -----------------------------------------------------------------------------
#pragma once

#include <string>

// Teclas especiais recebem códigos acima de 255 para não se misturar
// com as letras normais (que usam o próprio código ASCII, ex: 'a' = 97).
enum Tecla {
    TECLA_NENHUMA = -1,
    TECLA_ENTER = 13,
    TECLA_ESC = 27,

    TECLA_CIMA = 1000,
    TECLA_BAIXO,
    TECLA_ESQUERDA,
    TECLA_DIREITA,
    TECLA_HOME,      // no teclado numérico (Num Lock desligado) = 7
    TECLA_PAGE_UP,   // = 9
    TECLA_END,       // = 1
    TECLA_PAGE_DOWN, // = 3

    // Não é uma tecla: lerTecla() devolve isto quando a janela muda de
    // tamanho enquanto espera, para o jogo redesenhar a tela.
    TECLA_REDIMENSIONOU = 2000,
    TECLA_CLIQUE_MENU = 3000  // devolve 3000 + indice ao clicar numa opcao
};

// Modo texto (digitar o nome do personagem): as letras chegam como letras,
// sem virar comandos (na versão gráfica, W A S D deixam de andar).
void definirModoTexto(bool ligado);

// Prepara o terminal para o jogo: tela alternativa, cursor escondido,
// leitura de tecla sem precisar apertar Enter.
void iniciarTerminal();

// Devolve o terminal ao estado normal. É chamada automaticamente ao sair,
// mesmo se o jogador fechar com Ctrl+C.
void finalizarTerminal();

// Espera o jogador apertar uma tecla e devolve o código dela.
// Se a janela mudar de tamanho durante a espera, devolve TECLA_REDIMENSIONOU.
int lerTecla();

// Igual a lerTecla(), mas desiste depois de "esperaMs" milissegundos e
// devolve TECLA_NENHUMA. É o que deixa o mapa andar em tempo real.
int lerTeclaComEspera(int esperaMs);

// Relógio em milissegundos (só serve para medir intervalos).
long long relogioMs();

// Escreve um texto (com códigos ANSI) de uma vez só na tela.
// Montar a tela inteira numa string e escrever de uma vez evita que ela pisque.
void escreverNaTela(const std::string& texto);

// Pausa o programa por alguns milissegundos (usado nas animações).
void esperarMs(int milissegundos);

// Tamanho visível do terminal, em colunas e linhas.
// Devolve false se não for possível descobrir.
bool tamanhoDoTerminal(int& colunas, int& linhas);

// ---- Ajudantes para montar códigos ANSI ------------------------------------

// Move o cursor (linha e coluna começam em 1, como no padrão ANSI).
std::string ansiPosicao(int linha, int coluna);

// Muda a cor do texto e do fundo usando a paleta de 256 cores.
// Tabela das cores: https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
std::string ansiCor(int frente, int fundo);

// Volta às cores padrão do terminal.
std::string ansiResetar();
