// dicas.h
// -----------------------------------------------------------------------------
// Dicas: na primeira vez que algo acontece (primeira batalha, primeira caixa,
// primeiro altar...), aparece uma caixa explicando, que espera uma tecla.
// Cada dica aparece uma vez só: as já vistas ficam guardadas em opcoes.txt
// (opcoes.h), então não se repetem nas próximas partidas. Em Opções dá para
// desligar as dicas ou ver todas de novo.
// -----------------------------------------------------------------------------
#pragma once

struct Jogo;

enum Dica {
    // No mapa (caixa no meio da tela)
    DICA_INICIO, DICA_DESCANSO, DICA_ITEM, DICA_MOCHILA_CHEIA, DICA_NIVEL,
    DICA_CAIXA, DICA_CHAVE, DICA_ARMADILHA, DICA_POCA, DICA_LAVA,
    DICA_FONTE, DICA_ALTAR, DICA_BAU, DICA_ARENA, DICA_VIAJANTE,
    DICA_LOJA, DICA_CHEFE,
    // Na batalha (na caixa de texto da batalha)
    DICA_BATALHA, DICA_ESCUDO, DICA_STATUS,
    // v1.17, no mapa (no fim: opcoes.txt guarda as vistas pela posição)
    DICA_FURTIVO, DICA_CAMPEAO, DICA_GNOMO, DICA_SOMBRA,
    NUM_DICAS
};

const char* tituloDaDica(Dica dica);
const char* textoDaDica(Dica dica);

// A dica ainda deve aparecer? (dicas ligadas e esta ainda não foi vista)
bool dicaPendente(Dica dica);
void marcarDicaVista(Dica dica);   // marca e já grava em opcoes.txt
void zerarDicas();                  // "Rever as dicas": todas aparecem de novo
int dicasVistas();

// Mostra a dica no mapa (se estiver pendente) e espera uma tecla. Nos testes
// automáticos (jogo.modoAutomatico) não faz nada.
void mostrarDica(Jogo& jogo, Dica dica);

// A dica do mapa que vale agora (a primeira pendente cuja situação está
// acontecendo), ou NUM_DICAS se nenhuma.
Dica proximaDica(const Jogo& jogo);

// Chamada a cada quadro do mapa: vê se apareceu algo novo que merece dica
// (uma caixa à vista, a primeira chave, vida baixa...). Mostra no máximo uma.
void verificarDicasDoMapa(Jogo& jogo);
