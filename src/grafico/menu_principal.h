#pragma once

// Apenas apresentacao. Nao altera as regras do jogo original.
// O que o jogador escolheu no menu principal.
enum EscolhaDoMenu { MENU_SAIR, MENU_NOVO_JOGO, MENU_CONTINUAR };
// Mostra o menu (com "Continuar" se houver partida salva) e devolve a escolha.
// MENU_SAIR quando o jogador fecha a janela ou escolhe Sair.
int mostrarMenuPrincipal();
// Retorna 0 para continuar, 1 para voltar ao menu inicial, 2 para sair.
int mostrarMenuDePausa();
// Tela de Opções (volumes, tela cheia, animações e dicas), em opcoes.txt.
void mostrarOpcoes();
