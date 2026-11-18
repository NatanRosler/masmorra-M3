// jogo.cpp
#include "jogo.h"
#include "salvar.h"
#include "aleatorio.h"
#include "terminal.h"
#include "tela.h"
#include "pontuacao.h"
#include "som.h"

Mapa& mapaAtual(Jogo& jogo) {
    return jogo.andares[jogo.andar - 1].mapa;
}

const Mapa& mapaAtual(const Jogo& jogo) {
    return jogo.andares[jogo.andar - 1].mapa;
}

Andar& andarAtual(Jogo& jogo) {
    return jogo.andares[jogo.andar - 1];
}

const Andar& andarAtual(const Jogo& jogo) {
    return jogo.andares[jogo.andar - 1];
}

// Gera o andar se for a primeira visita. Cada andar tem sua própria semente,
// derivada da semente da partida — então a mesma partida sempre tem os
// mesmos andares (e os mesmos monstros), não importa a ordem das visitas.
static Andar& prepararAndar(Jogo& jogo, int numero) {
    Andar& a = jogo.andares[numero - 1];
    if (!a.gerado) {
        uint32_t sementeDoAndar = misturar(jogo.semente, (uint32_t)numero);
        gerarAndar(a.mapa, a.escadas, sementeDoAndar, numero == NUM_ANDARES, &a.tesouro);
        for (int y = 0; y < MAPA_ALTURA; y++) {
            for (int x = 0; x < MAPA_LARGURA; x++) {
                a.descoberto[y][x] = false;
                a.cheiro[y][x] = 0;
            }
        }
        a.numMonstros = 0;
        a.numArmadilhas = 0;
        a.numNpcs = 0;
        a.chavesGuardadas = 0;
        prepararSalaDaLoja(a, numero);   // antes dos monstros: lá dentro ninguém nasce
        int chegadaX = a.escadas.chegadaSobeX, chegadaY = a.escadas.chegadaSobeY;
        if (jogo.comMonstros) {
            // Semente separada: os monstros não mudam o desenho do mapa.
            povoarAndar(a, numero, misturar(jogo.semente, (uint32_t)numero + 1000), chegadaX, chegadaY);
            // No último andar, o chefe final espera no ponto mais distante.
            if (numero == NUM_ANDARES) {
                colocarChefe(a, chegadaX, chegadaY, jogo.elementoDaEspecie[tipoDoChefe()]);
            }
            // Cada monstro recebe o elemento sorteado para a sua espécie.
            for (int i = 0; i < a.numMonstros; i++) {
                a.monstros[i].elemento = jogo.elementoDaEspecie[a.monstros[i].tipo];
            }
        }
        // Itens depois dos monstros: a chave pode ficar com um deles.
        espalharItens(a, numero, misturar(jogo.semente, (uint32_t)numero + 2000));
        // Caixas, armadilhas e NPCs por último, nos lugares que sobraram.
        colocarCenario(a, numero, misturar(jogo.semente, (uint32_t)numero + 3000), chegadaX, chegadaY);
        colocarNpcs(a, numero, chegadaX, chegadaY);
        a.gerado = true;
    }
    return a;
}

// -----------------------------------------------------------------------------
// MENSAGENS
// -----------------------------------------------------------------------------
const size_t TAMANHO_DO_HISTORICO = 100;

void adicionarMensagem(Jogo& jogo, const std::string& texto, TipoMensagem tipo) {
    jogo.mensagensDoTurno.push_back(texto);
    if (tipo > jogo.tipoDoTurno) jogo.tipoDoTurno = tipo;
    jogo.historico.push_back(texto);
    jogo.tiposDoHistorico.push_back(tipo);
    if (jogo.historico.size() > TAMANHO_DO_HISTORICO) {
        jogo.historico.erase(jogo.historico.begin());
        jogo.tiposDoHistorico.erase(jogo.tiposDoHistorico.begin());
    }
    // Mensagem importante: também aparece em destaque no topo do mapa.
    if (tipo != MSG_NORMAL) {
        jogo.faixa = texto;
        jogo.tipoDaFaixa = tipo;
        jogo.faixaAte = relogioMs() + TEMPO_DA_FAIXA_MS;
    }
}

// -----------------------------------------------------------------------------
// ATRIBUTOS DO JOGADOR
// -----------------------------------------------------------------------------
// Atributo final = valor do nível + pontos distribuídos + bônus dos cristais
// + bônus do equipamento.
void recalcularAtributos(Jogador& j) {
    const int* pontos = j.atributos;
    j.vidaMaxima = vidaPeloNivel(BASE_VIDA_JOGADOR, j.nivel) + j.bonusVida + 3 * pontos[CONSTITUICAO];
    j.ataque     = atributoPeloNivel(BASE_ATAQUE_JOGADOR, j.nivel) + j.bonusAtaque + pontos[FORCA];
    j.defesa     = atributoPeloNivel(BASE_DEFESA_JOGADOR, j.nivel) + j.bonusDefesa + pontos[CONSTITUICAO] / 2;
    j.velocidade = atributoPeloNivel(BASE_VELOCIDADE_JOGADOR, j.nivel) + j.bonusVelocidade + pontos[AGILIDADE];
    j.precisaoExtra = 3 * pontos[DESTREZA];
    j.criticoExtra  = pontos[DESTREZA];
    j.esquiva       = 2 * pontos[AGILIDADE];
    if (j.esquiva > 40) j.esquiva = 40;

    if (j.arma != nullptr) {
        j.ataque += j.arma->bonusAtaque;
    }
    if (j.escudo != nullptr) {
        j.defesa += j.escudo->bonusDefesa;
        j.velocidade += j.escudo->bonusVelocidade;   // negativo no escudo pesado
    }
    if (j.velocidade < 1) j.velocidade = 1;
}

static void restaurarPP(Jogador& j) {
    for (int s = 0; s < j.numGolpes; s++) j.pp[s] = GOLPES[j.golpes[s]].ppMaximo;
}

// -----------------------------------------------------------------------------
// VISÃO, LUZ E CHEIRO
// -----------------------------------------------------------------------------
// Alcance da visão "em linha reta". É grande de propósito: o que limita o que
// o jogador vê é a LUZ. Uma sala iluminada por tocha é vista de longe; um
// corredor escuro, só até onde a luz do jogador alcança.
const int ALCANCE_DA_VISAO = 80;

const Luz COR_LUZ_JOGADOR = { 1.00f, 0.96f, 0.88f }; // branco amarelado
const Luz COR_LUZ_TOCHA   = { 1.00f, 0.68f, 0.38f }; // laranja
const int RAIO_LUZ_TOCHA  = 7;

// Como no Brogue, a luz do jogador enfraquece conforme ele desce:
// raio 9 nos andares 1-2, 8 nos andares 3-4, ..., 5 nos andares 9-10.
int raioDaLuzDoJogador(int andar) {
    return 9 - (andar - 1) / 2;
}

void atualizarVisao(Jogo& jogo) {
    const Mapa& mapa = mapaAtual(jogo);
    Andar& andar = andarAtual(jogo);
    int jx = jogo.jogador.x;
    int jy = jogo.jogador.y;

    // 1. Luz: a do jogador + a de todas as tochas do andar.
    apagarLuzes(jogo.luz);
    adicionarLuz(mapa, jx, jy, raioDaLuzDoJogador(jogo.andar), COR_LUZ_JOGADOR, jogo.luz);
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (mapa.celulas[y][x] == TOCHA) {
                adicionarLuz(mapa, x, y, RAIO_LUZ_TOCHA, COR_LUZ_TOCHA, jogo.luz);
            }
        }
    }

    // 2. Linha de visão (também usada pelos monstros para "ver" o jogador).
    calcularCampoDeVisao(mapa, jx, jy, ALCANCE_DA_VISAO, jogo.linhaDeVisao);

    // 3. Visível = em linha reta E iluminado. Tudo o que é visto vai para a memória.
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            bool iluminado = brilho(jogo.luz[y][x]) >= LUZ_MINIMA_PARA_VER;
            jogo.visivel[y][x] = jogo.linhaDeVisao[y][x] && iluminado;
            if (jogo.visivel[y][x]) {
                andar.descoberto[y][x] = true;
            }
        }
    }
    jogo.visivel[jy][jx] = true;
    andar.descoberto[jy][jx] = true;

    // Itens vistos ficam na memória do mapa.
    for (int i = 0; i < andar.numItens; i++) {
        ItemNoChao& item = andar.itens[i];
        if (item.ativo && jogo.visivel[item.y][item.x]) item.visto = true;
    }

    // 4. Distância (BFS) de cada célula até o jogador: os monstros que estão
    //    caçando seguem esse "mapa" na direção dos números menores.
    calcularDistancias(mapa, jx, jy, jogo.distanciaAteJogador);
}

// O jogador deixa cheiro em tudo o que está na linha de visão dele. O valor
// cresce com o turno (cheiro novo vale mais que cheiro velho) e diminui com a
// distância (perto do jogador, cheiro mais forte). Um monstro que perdeu o
// jogador de vista anda sempre para o vizinho de cheiro mais forte.
static void deixarCheiro(Jogo& jogo) {
    Andar& andar = andarAtual(jogo);
    int jx = jogo.jogador.x, jy = jogo.jogador.y;
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            if (!jogo.linhaDeVisao[y][x]) continue;
            int dx = x > jx ? x - jx : jx - x;
            int dy = y > jy ? y - jy : jy - y;
            int maior = dx > dy ? dx : dy, menor = dx > dy ? dy : dx;
            int valor = 1000 + 3 * jogo.passos - (2 * maior + menor);
            if (valor > andar.cheiro[y][x]) andar.cheiro[y][x] = valor;
        }
    }
}

// -----------------------------------------------------------------------------
// ANDARES
// -----------------------------------------------------------------------------
// Leva o jogador para outro andar. Quem desce chega na frente da escada que
// sobe do novo andar; quem sobe chega na frente da escada que desce.
static void irParaAndar(Jogo& jogo, int novoAndar) {
    bool descendo = novoAndar > jogo.andar;
    tocarSom(SOM_ESCADA);
    // A chave só abre a porta do próprio andar: ela fica guardada aqui e
    // volta para a mochila se o jogador voltar.
    Jogador& j = jogo.jogador;
    int andarAnterior = jogo.andar;
    andarAtual(jogo).chavesGuardadas = j.mochila[I_CHAVE];
    Andar& destino = prepararAndar(jogo, novoAndar);
    j.mochila[I_CHAVE] = destino.chavesGuardadas;
    jogo.andar = novoAndar;
    if (novoAndar > jogo.andarMaisFundo) jogo.andarMaisFundo = novoAndar;

    if (descendo) {
        jogo.jogador.x = destino.escadas.chegadaSobeX;
        jogo.jogador.y = destino.escadas.chegadaSobeY;
        adicionarMensagem(jogo, "Você desce para o andar " + std::to_string(novoAndar) + ".");
        if (jogo.andares[andarAnterior - 1].chavesGuardadas > 0) {
            adicionarMensagem(jogo, "A chave do andar " + std::to_string(andarAnterior) +
                                    " ficou para trás: ela só abre a porta daquele andar.");
        }
        if (novoAndar == NUM_ANDARES) {
            adicionarMensagem(jogo, "Você chegou ao fundo da masmorra. O Senhor das Profundezas está aqui.", MSG_PERIGO);
        }
    } else {
        jogo.jogador.x = destino.escadas.chegadaDesceX;
        jogo.jogador.y = destino.escadas.chegadaDesceY;
        adicionarMensagem(jogo, "Você sobe para o andar " + std::to_string(novoAndar) + ".");
    }
    // Salva sozinho a cada andar novo (se o jogo fechar de repente, perde pouco).
    salvarPartida(jogo);
}

void iniciarJogo(Jogo& jogo, uint32_t semente, bool comMonstros) {
    jogo.semente = semente;
    jogo.comMonstros = comMonstros;
    iniciarGerador(jogo.sorte, misturar(semente, 0xC0FFEEu));
    sortearElementosDasEspecies(jogo, misturar(semente, 777u));
    for (int i = 0; i < NUM_ANDARES; i++) {
        jogo.andares[i].gerado = false;
    }

    jogo.andar = 1;
    Andar& primeiro = prepararAndar(jogo, 1);

    // O jogador começa no nível 5 com uma adaga. As vidas e as poções
    // dependem da dificuldade (ver aplicarCriacao).
    Jogador& j = jogo.jogador;
    j.x = primeiro.escadas.chegadaSobeX;
    j.y = primeiro.escadas.chegadaSobeY;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) j.mochila[i] = 0;
    j.arma = &TIPOS_DE_ITEM[I_ADAGA];
    j.escudo = nullptr;
    j.bonusVida = j.bonusAtaque = j.bonusDefesa = j.bonusVelocidade = 0;
    j.moedas = 0;
    j.nivel = NIVEL_INICIAL;
    j.experiencia = experienciaParaNivel(NIVEL_INICIAL);
    // Personagem padrão (os testes usam este; o jogo de verdade troca pelo
    // que o jogador criou, com aplicarCriacao).
    Criacao padrao;
    criacaoPadrao(padrao);
    padrao.atributos[FORCA] = 2;
    padrao.atributos[CONSTITUICAO] = 1;
    padrao.atributos[DESTREZA] = 1;
    padrao.atributos[AGILIDADE] = 1;
    padrao.pontosLivres = 0;
    aplicarCriacao(jogo, padrao);
    jogo.batalhaPendente = -1;

    jogo.passos = 0;
    jogo.tempoMs = 0;
    jogo.esperaDoJogadorMs = 0;
    jogo.descansando = false;
    jogo.msDeDescanso = 0;
    jogo.segundosDeDescanso = 0;
    jogo.rodadasDaUltimaBatalha = 0;
    jogo.monstrosMortos = 0;
    jogo.itensColetados = 0;
    jogo.andarMaisFundo = 1;
    jogo.morto = false;
    jogo.venceu = false;
    jogo.rankingRegistrado = false;
    jogo.pontuacaoFinal = 0;
    jogo.posicaoNoRanking = -1;
    jogo.causaDaMorte = "";
    jogo.rodando = true;
    jogo.mensagensDoTurno.clear();
    jogo.tipoDoTurno = MSG_NORMAL;
    jogo.historico.clear();
    jogo.tiposDoHistorico.clear();
    jogo.faixa = "";
    jogo.faixaAte = 0;
    adicionarMensagem(jogo, "Você desce a escadaria iluminada por tochas. As portas se fecham atrás de você.");
    atualizarVisao(jogo);
}

static std::string comMaiusculaInicial(std::string texto) {
    if (!texto.empty() && texto[0] >= 'a' && texto[0] <= 'z') texto[0] = texto[0] - 'a' + 'A';
    return texto;
}

// -----------------------------------------------------------------------------
// BATALHAS E VIDAS
// -----------------------------------------------------------------------------
static int batalhasTravadas = 0;

static std::string umOuUma(const TipoMonstro& t) {
    return (std::string(t.artigo) == "a") ? "uma " : "um ";
}

static std::string nomeComArtigoDoMonstro(const Monstro& m) {
    return std::string(tipoDe(m).artigo) + " " + tipoDe(m).nome;
}

// Gasta uma vida e volta para a entrada do andar. Sem vidas, é o fim do jogo.
void desmaiar(Jogo& jogo, const std::string& causa) {
    Jogador& j = jogo.jogador;
    j.vidas--;
    jogo.descansando = false;
    tocarSom(SOM_DERROTA);
    if (j.vidas <= 0) {
        jogo.morto = true;
        jogo.causaDaMorte = causa;
        tocarMusica(SEM_MUSICA);
        return;
    }
    // Volta para a entrada do andar (ou a célula livre mais perto dela, se
    // houver um monstro parado ali).
    Andar& andar = andarAtual(jogo);
    static int distancia[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(andar.mapa, andar.escadas.chegadaSobeX, andar.escadas.chegadaSobeY, distancia);
    int melhorX = andar.escadas.chegadaSobeX, melhorY = andar.escadas.chegadaSobeY, melhor = 1000000;
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            int d = distancia[y][x];
            if (d == INALCANCAVEL || d >= melhor || monstroEm(andar, x, y) >= 0) continue;
            melhor = d;
            melhorX = x;
            melhorY = y;
        }
    }
    j.x = melhorX;
    j.y = melhorY;
    j.vida = j.vidaMaxima;
    restaurarPP(j);

    std::string restam = (j.vidas == 1) ? "Resta 1 vida." : "Restam " + std::to_string(j.vidas) + " vidas.";
    adicionarMensagem(jogo, "Você desmaiou e acordou na entrada do andar. " + restam, MSG_PERIGO);
    atualizarVisao(jogo);
}

// O jogador perdeu uma batalha.
static void perderVida(Jogo& jogo, Monstro* participantes[], int quantos) {
    // Quem "venceu": o primeiro monstro da batalha que continua de pé.
    Monstro* vencedor = participantes[0];
    for (int i = 0; i < quantos; i++) {
        if (participantes[i]->vivo) { vencedor = participantes[i]; break; }
    }
    // Os monstros que venceram se recuperam e voltam a vagar.
    for (int i = 0; i < quantos; i++) {
        Monstro& m = *participantes[i];
        if (!m.vivo) continue;
        m.vida = vidaMaximaDoMonstro(m);
        m.estado = VAGANDO;
        m.turnosSemBatalha = 10;
    }
    desmaiar(jogo, "derrotado por " + umOuUma(tipoDe(*vencedor)) + tipoDe(*vencedor).nome);
}

// Quem entra na batalha: o monstro que a começou e até mais 2 que estejam
// acordados, caçando e a no máximo 3 passos do jogador (os mais perto primeiro).
static int juntarParticipantes(Jogo& jogo, int principal, Monstro* participantes[]) {
    Andar& andar = andarAtual(jogo);
    int quantos = 0;
    participantes[quantos++] = &andar.monstros[principal];
    for (int passos = 1; passos <= 3; passos++) {
        for (int i = 0; i < andar.numMonstros && quantos < MAX_INIMIGOS; i++) {
            Monstro& m = andar.monstros[i];
            if (i == principal || !m.vivo || m.estado != CACANDO || m.turnosSemBatalha > 0) continue;
            if (jogo.distanciaAteJogador[m.y][m.x] == passos) participantes[quantos++] = &m;
        }
    }
    return quantos;
}

static void pararDeDescansar(Jogo& jogo, const std::string& motivo) {
    if (!jogo.descansando) return;
    jogo.descansando = false;
    if (!motivo.empty()) adicionarMensagem(jogo, motivo);
}

static void resolverBatalha(Jogo& jogo) {
    Monstro* participantes[MAX_INIMIGOS];
    int quantos = juntarParticipantes(jogo, jogo.batalhaPendente, participantes);
    jogo.batalhaPendente = -1;
    batalhasTravadas++;
    pararDeDescansar(jogo, "");

    int nivelAntes = jogo.jogador.nivel;
    ResultadoBatalha resultado = batalhar(jogo, participantes, quantos);
    forcarLimpezaDaTela();
    tocarMusica(MUSICA_MASMORRA);
    if (jogo.jogador.nivel > nivelAntes && jogo.jogador.pontosLivres > 0) {
        int p = jogo.jogador.pontosLivres;
        adicionarMensagem(jogo, "Nível " + std::to_string(jogo.jogador.nivel) + "! Você tem " + std::to_string(p) +
                                (p == 1 ? " ponto" : " pontos") + " para distribuir: aperte C.", MSG_BOA);
    }

    // O que aconteceu com cada monstro.
    for (int i = 0; i < quantos; i++) {
        Monstro& m = *participantes[i];
        std::string nome = nomeComArtigoDoMonstro(m);
        if (!m.vivo) {
            jogo.monstrosMortos++;
            adicionarMensagem(jogo, "Você derrotou " + nome + "!");
            if (tipoDe(m).especial & CHEFE) {
                jogo.venceu = true;   // fim de jogo: vitória!
                tocarMusica(SEM_MUSICA);
                tocarSom(SOM_VITORIA);
                adicionarMensagem(jogo, "O Senhor das Profundezas caiu! Valdoria está livre!", MSG_BOA);
            }
            if (m.carregaChave) {
                jogo.jogador.mochila[I_CHAVE]++;
                jogo.itensColetados++;
                adicionarMensagem(jogo, comMaiusculaInicial(nome) + " carregava uma chave! Agora ela é sua.", MSG_BOA);
                revelarPortaDoTesouro(jogo);
            }
        } else if (m.estado == FUGINDO) {
            adicionarMensagem(jogo, comMaiusculaInicial(nome) + " fugiu de você.");
        }
    }

    if (resultado == JOGADOR_FUGIU) {
        for (int i = 0; i < quantos; i++) {
            Monstro& m = *participantes[i];
            if (!m.vivo || m.estado == FUGINDO) continue;
            m.estado = VAGANDO;
            m.turnosSemBatalha = 6;   // demora um pouco para puxar briga de novo
        }
        adicionarMensagem(jogo, "Você escapou da batalha.");
    }
    if (resultado == DERROTA) {
        perderVida(jogo, participantes, quantos);
    }
}

// -----------------------------------------------------------------------------
// TEMPO REAL
// -----------------------------------------------------------------------------
// O mapa anda em tempo real: os monstros agem no ritmo deles (monstros.cpp) e
// o jogador pode dar um passo sempre que o intervalo dele tiver passado.
int intervaloDoJogador(const Jogador& j) {
    int ms = 260 - 3 * j.velocidade;   // mais veloz = passos mais rápidos
    if (ms < 100) ms = 100;
    return ms;
}

static bool algumMonstroVisivel(const Jogo& jogo) {
    const Andar& andar = andarAtual(jogo);
    for (int i = 0; i < andar.numMonstros; i++) {
        const Monstro& m = andar.monstros[i];
        if (m.vivo && jogo.visivel[m.y][m.x]) return true;
    }
    return false;
}

static bool ppCheio(const Jogador& j) {
    for (int s = 0; s < j.numGolpes; s++) {
        if (j.pp[s] < GOLPES[j.golpes[s]].ppMaximo) return false;
    }
    return true;
}

// Tecla Z: começa a descansar. A vida volta só assim (1 por segundo).
static void comecarDescanso(Jogo& jogo) {
    const Jogador& j = jogo.jogador;
    if (algumMonstroVisivel(jogo)) {
        adicionarMensagem(jogo, "Não dá para descansar com inimigos por perto!");
        return;
    }
    if (j.vida >= j.vidaMaxima && ppCheio(j)) {
        adicionarMensagem(jogo, "Você já está descansado.");
        return;
    }
    jogo.descansando = true;
    jogo.msDeDescanso = 0;
    jogo.segundosDeDescanso = 0;
    tocarSom(SOM_CURA);
    adicionarMensagem(jogo, "Você descansa... (+1 de vida por segundo; qualquer tecla para parar)");
}

void avancarTempo(Jogo& jogo, int ms) {
    if (jogo.morto || jogo.venceu) return;
    if (ms > 250) ms = 250;   // depois de um menu ou batalha, o relógio não "pula"
    jogo.tempoMs += ms;
    if (jogo.esperaDoJogadorMs > 0) jogo.esperaDoJogadorMs -= ms;

    // Monstros (se um alcançar o jogador, a batalha começa).
    if (jogo.comMonstros && jogo.batalhaPendente < 0) agirMonstros(jogo, ms);
    if (jogo.batalhaPendente >= 0) {
        resolverBatalha(jogo);
        return;
    }

    // Descanso: +1 de vida por segundo e +1 PP em cada golpe a cada 5 segundos.
    if (jogo.descansando) {
        Jogador& j = jogo.jogador;
        if (algumMonstroVisivel(jogo)) {
            pararDeDescansar(jogo, "Você para de descansar: algo se aproxima!");
            return;
        }
        jogo.msDeDescanso += ms;
        while (jogo.msDeDescanso >= 1000) {
            jogo.msDeDescanso -= 1000;
            jogo.segundosDeDescanso++;
            if (j.vida < j.vidaMaxima) {
                j.vida++;
                efeitoNoMapa(jogo, EFEITO_CURA, j.x, j.y, 1);
            }
            if (jogo.segundosDeDescanso % 5 == 0) {
                for (int s = 0; s < j.numGolpes; s++) {
                    if (j.pp[s] < GOLPES[j.golpes[s]].ppMaximo) j.pp[s]++;
                }
            }
        }
        if (j.vida >= j.vidaMaxima && ppCheio(j)) pararDeDescansar(jogo, "Você se sente descansado.");
    }
}

// Depois de um passo do jogador: atualiza visão e cheiro, e ele precisa
// esperar o próprio intervalo para dar o próximo passo.
static void depoisDoPasso(Jogo& jogo) {
    jogo.passos++;
    armadilhaPegaJogador(jogo);   // pisou numa armadilha? (Etapa 7)
    if (jogo.morto) return;
    atualizarVisao(jogo);
    deixarCheiro(jogo);
    jogo.esperaDoJogadorMs = intervaloDoJogador(jogo.jogador);
    if (jogo.batalhaPendente >= 0) resolverBatalha(jogo);
}

// -----------------------------------------------------------------------------
// Converte a tecla numa direção (dx, dy). Devolve false se não for movimento.
//
// Aceita três esquemas, como o Brogue:
//   - setas (e Home/End/PgUp/PgDn para as diagonais)
//   - teclado numérico: 7 8 9 / 4 6 / 1 2 3
//   - letras do Vim: h j k l (retas) e y u b n (diagonais)
// -----------------------------------------------------------------------------
static bool teclaParaDirecao(int tecla, int& dx, int& dy) {
    switch (tecla) {
        case TECLA_CIMA:      case '8': case 'k': dx =  0; dy = -1; return true;
        case TECLA_BAIXO:     case '2': case 'j': dx =  0; dy =  1; return true;
        case TECLA_ESQUERDA:  case '4': case 'h': dx = -1; dy =  0; return true;
        case TECLA_DIREITA:   case '6': case 'l': dx =  1; dy =  0; return true;
        case TECLA_HOME:      case '7': case 'y': dx = -1; dy = -1; return true;
        case TECLA_PAGE_UP:   case '9': case 'u': dx =  1; dy = -1; return true;
        case TECLA_END:       case '1': case 'b': dx = -1; dy =  1; return true;
        case TECLA_PAGE_DOWN: case '3': case 'n': dx =  1; dy =  1; return true;
    }
    return false;
}

// Tenta mover o jogador. Andar na direção de um monstro = atacar.
// Devolve true se ele gastou o turno.
static bool moverJogador(Jogo& jogo, int dx, int dy) {
    Jogador& j = jogo.jogador;
    const Mapa& mapa = mapaAtual(jogo);
    int novoX = j.x + dx;
    int novoY = j.y + dy;

    // Porta trancada: abre se o jogador tiver uma chave (só de frente).
    if (dentroDoMapa(novoX, novoY) && mapa.celulas[novoY][novoX] == PORTA_TRANCADA &&
        (dx == 0 || dy == 0)) {
        if (j.mochila[I_CHAVE] <= 0) {
            adicionarMensagem(jogo, "A porta está trancada. Você precisa de uma chave.");
            return false;
        }
        j.mochila[I_CHAVE]--;
        mapaAtual(jogo).celulas[novoY][novoX] = PORTA;
        tocarSom(SOM_PORTA);
        adicionarMensagem(jogo, "Você usa a chave e destranca a porta!", MSG_BOA);
        return true;
    }

    // Caixa: empurra (ou quebra, se ela estiver presa).
    if (dentroDoMapa(novoX, novoY) && mapa.celulas[novoY][novoX] == CAIXA) {
        return empurrarCaixa(jogo, dx, dy);
    }

    // NPC: andar na direção dele abre a conversa (não gasta tempo).
    int npc = npcEm(andarAtual(jogo), novoX, novoY);
    if (npc >= 0) {
        conversarComNpc(jogo, npc);
        return false;
    }

    // Bater na parede (ou tentar cortar uma quina) não gasta turno.
    if (!podeMover(mapa, j.x, j.y, dx, dy)) {
        return false;
    }

    // Andar na direção de um monstro começa uma batalha.
    int alvo = monstroEm(andarAtual(jogo), novoX, novoY);
    if (alvo >= 0) {
        jogo.batalhaPendente = alvo;
        return true;
    }

    // Escadas, como no Brogue: andar para dentro delas troca de andar.
    TipoCelula destino = mapa.celulas[novoY][novoX];
    if (destino == ESCADA_DESCE) {
        irParaAndar(jogo, jogo.andar + 1);
        return true;
    }
    if (destino == ESCADA_SOBE) {
        if (jogo.andar == 1) {
            adicionarMensagem(jogo, "As portas estão seladas. A única saída é derrotar o guardião do fundo da masmorra.");
            return false;
        }
        irParaAndar(jogo, jogo.andar - 1);
        return true;
    }

    j.x = novoX;
    j.y = novoY;
    pegarItensDoChao(jogo);
    return true;
}

static int esperarTecla(const Jogo& jogo, void (*desenharPorCima)(const Jogo&)) {
    int t;
    do {
        desenharJogo(jogo);  // redesenha o fundo (a janela pode ter mudado)
        if (desenharPorCima) desenharPorCima(jogo);
        t = lerTecla();
    } while (t == TECLA_REDIMENSIONOU);
    return t;
}

int escolherEmCaixa(const Jogo& jogo, const std::string& titulo,
                    const std::vector<std::string>& opcoes,
                    const std::vector<std::string>& descricoes, int cursorInicial) {
    int total = (int)opcoes.size();
    int cursor = (cursorInicial >= 0 && cursorInicial < total) ? cursorInicial : 0;
    while (true) {
        desenharJogo(jogo);
        std::string descricao = (cursor < (int)descricoes.size()) ? descricoes[cursor] : "";
        desenharMenuEmCaixa(titulo, opcoes, descricao, cursor);
        int t = lerTecla();
        if (t == TECLA_REDIMENSIONOU) continue;
        if (t == TECLA_ENTER || t == ' ' || t == 'z') { tocarSom(SOM_MENU); return cursor; }
        if (t >= TECLA_CLIQUE_MENU && t < TECLA_CLIQUE_MENU+total) { tocarSom(SOM_MENU); return t-TECLA_CLIQUE_MENU; }
        if (t == TECLA_ESC || t == 'x' || t == 'i') return -1;
        if (t == TECLA_CIMA || t == 'k')  { cursor = (cursor + total - 1) % total; tocarSom(SOM_MENU); }
        if (t == TECLA_BAIXO || t == 'j') { cursor = (cursor + 1) % total; tocarSom(SOM_MENU); }
    }
}

// Tecla i: mostra a mochila e usa/equipa o item escolhido.
// Devolve true se a ação gastou o turno.
static bool abrirMochila(Jogo& jogo) {
    const Jogador& j = jogo.jogador;
    std::vector<int> tipos;
    std::vector<std::string> opcoes, descricoes;
    for (int i = 0; i < NUM_TIPOS_DE_ITEM; i++) {
        if (j.mochila[i] <= 0) continue;
        tipos.push_back(i);
        if (i == I_CHAVE) {
            // A chave é deste andar e não ocupa espaço.
            opcoes.push_back("Chave do andar " + std::to_string(jogo.andar));
            descricoes.push_back("Abre a porta trancada da sala do tesouro DESTE andar.  |  Não ocupa espaço na mochila.");
            continue;
        }
        opcoes.push_back(std::string(TIPOS_DE_ITEM[i].nome) + "   " + std::to_string(j.mochila[i]) + "/" +
                         std::to_string(limiteDoItem(i)));
        descricoes.push_back(TIPOS_DE_ITEM[i].descricao);
    }
    std::string arma = (j.arma != nullptr) ? j.arma->nome : "nenhuma";
    std::string escudo = (j.escudo != nullptr) ? j.escudo->nome : "nenhum";
    std::string titulo = "MOCHILA " + std::to_string(itensNaMochila(j)) + "/" + std::to_string(LIMITE_DA_MOCHILA) +
                         "   (arma: " + arma + ", escudo: " + escudo + ")";
    if (tipos.empty()) {
        adicionarMensagem(jogo, "A mochila está vazia. (arma: " + arma + ", escudo: " + escudo + ")");
        return false;
    }
    int escolha = escolherEmCaixa(jogo, titulo, opcoes, descricoes);
    if (escolha < 0) return false;
    return usarItemForaDaBatalha(jogo, tipos[escolha]);
}

static void confirmarSaida(Jogo& jogo) {
    jogo.mensagensDoTurno.clear();
    jogo.tipoDoTurno = MSG_NORMAL;
    jogo.mensagensDoTurno.push_back("Deseja sair? A partida fica salva para continuar depois. (s/n)");
    int resposta = esperarTecla(jogo, nullptr);
    jogo.mensagensDoTurno.clear();
    jogo.tipoDoTurno = MSG_NORMAL;
    if (resposta == 's' || resposta == 'S') {
        jogo.rodando = false;
    }
}

void processarTecla(Jogo& jogo, int tecla) {
    if (jogo.morto || jogo.venceu) return;

    // Telas de consulta não gastam tempo nem apagam as mensagens.
    if (tecla == '?') { esperarTecla(jogo, desenharAjuda); return; }
    if (tecla == 'M') { esperarTecla(jogo, desenharHistorico); return; }
    if (tecla == 'c' || tecla == 'C') { jogo.descansando = false; abrirFicha(jogo); return; }

    // Teclas que não fazem nada são ignoradas (e não apagam as mensagens).
    int dx = 0, dy = 0;
    bool ehMovimento = teclaParaDirecao(tecla, dx, dy);
    bool ehDescanso = (tecla == 'z' || tecla == 'Z' || tecla == '5' || tecla == '.');
    bool ehComando = ehDescanso || tecla == 'Q' || tecla == 'i';
    if (!ehMovimento && !ehComando) return;

    // O jogador ainda está terminando o passo anterior.
    if (ehMovimento && jogo.esperaDoJogadorMs > 0 && !jogo.descansando) return;

    jogo.mensagensDoTurno.clear();
    jogo.tipoDoTurno = MSG_NORMAL;

    // Qualquer tecla interrompe o descanso (e a tecla de descanso só interrompe).
    if (jogo.descansando) {
        pararDeDescansar(jogo, "Você para de descansar.");
        if (ehDescanso) return;
    }
    if (ehMovimento) {
        if (moverJogador(jogo, dx, dy)) depoisDoPasso(jogo);
    } else if (ehDescanso) {
        comecarDescanso(jogo);
    } else if (tecla == 'i') {
        // Usar ou equipar um item leva o tempo de dois passos.
        if (abrirMochila(jogo)) jogo.esperaDoJogadorMs = 2 * intervaloDoJogador(jogo.jogador);
    } else if (tecla == 'Q') {
        confirmarSaida(jogo);
    }
}
