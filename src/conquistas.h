// conquistas.h
// -----------------------------------------------------------------------------
// Conquistas (v1.26): medalhas que valem para todas as partidas. Ficam no
// arquivo conquistas.txt (como o bestiário) e não dão prêmio no jogo.
//
// Cada partida também guarda quais conquistas ganhou (Jogo::conquistasDaPartida,
// vai no save), para o resumo do fim mostrar as "novas".
// -----------------------------------------------------------------------------
#pragma once

struct Jogo;

enum IdConquista {
    // Jornada
    C_PRIMEIROS_PASSOS, C_REI_DEPOSTO, C_FIM_DO_NINHO, C_OSSOS_NO_CHAO, C_CORACAO_APAGADO, C_LIBERTADOR,
    // Desafio
    C_CORAGEM, C_LENDA, C_SEM_TROPECOS, C_RELAMPAGO,
    // Exploração
    C_TESOURO, C_GLADIADOR, C_DESCONFIADO, C_BOM_CORACAO, C_VELHOS_AMIGOS, C_BOLSO_CHEIO,
    C_MUITOS_ELEMENTOS, C_NA_SURDINA, C_NATURALISTA,
    NUM_CONQUISTAS
};

enum GrupoDeConquista { GRUPO_JORNADA, GRUPO_DESAFIO, GRUPO_EXPLORACAO, NUM_GRUPOS_DE_CONQUISTA };

struct FichaDaConquista {
    const char* chave;      // nome no arquivo (não muda, mesmo que a lista mude de ordem)
    const char* nome;
    const char* comoGanhar;
    int grupo;              // GrupoDeConquista
};

extern const FichaDaConquista CONQUISTAS[NUM_CONQUISTAS];
extern const char* NOMES_DOS_GRUPOS[NUM_GRUPOS_DE_CONQUISTA];

void carregarConquistas();
bool conquistaGanha(int id);              // em alguma partida
int conquistasGanhas();
bool ganhouNaPartida(const Jogo& jogo, int id);
bool novaNaPartida(const Jogo& jogo, int id);   // ganhou pela primeira vez nesta partida

// Ganha a conquista (se ainda não tinha): grava o arquivo e avisa no topo do
// mapa. Nos testes automáticos não faz nada.
void ganharConquista(Jogo& jogo, int id);

// As conquistas que dependem só do estado do jogo (moedas, conversas, arena,
// tesouro, golpes, bestiário). Chamado depois de cada passo e cada batalha.
void conferirConquistas(Jogo& jogo);
