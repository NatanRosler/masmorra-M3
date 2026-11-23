// Menu 2D do Masmorra. As mecanicas estao em jogo.cpp, batalha.cpp, etc.
#include "menu_principal.h"
#include "visual.h"
#include "sprites.h"
#include "extra_sprites.h"
#include "monstros_sprites.h"
#include "raylib.h"
#include "pontuacao.h"
#include "som.h"
#include "efeitos.h"
#include "salvar.h"
#include "versao.h"
#include "opcoes.h"
#include "dicas.h"
#include "tela.h"
#include "bestiario.h"
#include <array>
#include <cmath>
#include <string>

namespace {
using namespace Ui;
// Com partida salva aparece mais um botão ("CONTINUAR") no topo.
enum Opcao { CONTINUAR, JOGAR, COMO_JOGAR, ITENS, PONTUACAO, BESTIARIO_OPCAO, CONQUISTAS_OPCAO, OPCOES_OPCAO, SAIR, NUM_OPCOES };
const char* NOMES_DAS_OPCOES[NUM_OPCOES] = {"CONTINUAR","NOVO JOGO","COMO JOGAR","ITENS","PONTUAÇÃO","BESTIÁRIO","CONQUISTAS","OPÇÕES","SAIR"};
bool temSave = false;          // atualizado ao abrir o menu
std::string resumoDoSave;
int numBotoes() { return temSave ? NUM_OPCOES : NUM_OPCOES - 1; }
// A opção do botão i (sem save, o primeiro botão já é "NOVO JOGO").
int opcaoDoBotao(int i) { return temSave ? i : i + 1; }
Rectangle botaoDoMenu(int i) {
    // 8 botões com espaço de 50; com o CONTINUAR (9 botões) ficam mais juntos.
    return temSave ? Rectangle{155,346.0f+i*47,390,41} : Rectangle{155,350.0f+i*50,390,44};
}

bool clicou(Rectangle r) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r);
}
bool teclaSubir() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
bool teclaDescer() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
bool confirmar() { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
void botao(Rectangle r, const std::string& legenda, bool ativo) {
    bool sobre=mouseSobre(r);
    bool chegou=true;   // o destaque já está (quase todo) sobre este botão?
    if(ativo) {
        // O destaque dourado desliza entre os botões.
        painel(r,PAINEL2,Color{69,89,110,255});
        Rectangle d=Efeitos::destaqueDeslizante(r);
        painel(d,OURO,OURO);
        chegou=std::fabs(d.y-r.y)<r.height*0.5f && std::fabs(d.x-r.x)<r.width*0.5f;
    } else {
        // Mouse em cima: o botão cresce um pouquinho e ganha borda ciano.
        Rectangle g=sobre?Rectangle{r.x-4,r.y-2,r.width+8,r.height+4}:r;
        painel(g,sobre?Color{43,58,88,255}:PAINEL2,sobre?CIANO:Color{69,89,110,255});
    }
    textoCentro(legenda,r.x+r.width/2,r.y+(r.height-27)/2,27,(ativo&&chegou)?FUNDO:TEXTO);
}
// Mostra o botão "afundando" por um instante antes de executar a escolha.
void apertar(Rectangle r, void (*desenhar)(int), int cursor) {
    Efeitos::marcarClique(r);
    double inicio=GetTime();
    while(GetTime()-inicio<0.12 && !WindowShouldClose()) {
        iniciarQuadro(); desenhar(cursor); terminarQuadro();
    }
}
void titulo() {
    fundoEstrelado();
    texto("MASMORRA",145,91,76,OURO);
    texto("UMA AVENTURA PELA ESCURIDAO",152,188,23,CIANO);
    std::string versao="v" VERSAO_DO_JOGO;   // v1.25: para saber qual .exe está aberto
    texto(versao,1275-larguraDoTexto(versao,20),193,20,{120,135,160,255});
    DrawLineEx({150,242},{1275,242},2,{66,107,126,255});
}
void inicio(int cursor) {
    titulo();
    texto("Explore treze andares, descubra itens e",151,279,25,TEXTO);
    texto("enfrente criaturas em combates por turnos.",151,315,25,TEXTO);
    for(int i=0;i<numBotoes();i++) {
        Rectangle r=botaoDoMenu(i);
        bool hover=CheckCollisionPointRec(mouseVirtual(),r);
        botao(r,NOMES_DAS_OPCOES[opcaoDoBotao(i)],i==cursor||hover);
    }
    painel({690,291,550,452},{22,32,51,255});
    DrawEllipse(964,684,204,30,{12,15,28,255});
    sprite(HEROI.data(),794,375,15.6f);
    sprite(GOBLIN.data(),1099,521,8.0f);
    if(temSave) {
        // Resumo da partida salva, embaixo do herói.
        textoCentro("PARTIDA SALVA",965,700,18,OURO);
        textoCentro(resumoDoSave,965,722,19,TEXTO);
    } else texto("SEU JOGO, SUAS REGRAS",749,705,22,CIANO);
    texto("Setas / mouse para escolher  |  Enter para confirmar  |  O: som "+
          std::string(somLigado()?"ligado":"desligado"),151,785,20,APAGADO);
}
}

// ---- OPÇÕES (menu principal e pausa) -------------------------------------------
namespace {
enum LinhaDasOpcoes { OP_MUSICA, OP_EFEITOS, OP_AMBIENTE, OP_TELA, OP_ZOOM, OP_ANIMACAO, OP_TEXTO, OP_BATALHA, OP_DICAS,
                      OP_REVER, OP_ZERAR, OP_VOLTAR, NUM_LINHAS };
const char* NOMES_DAS_LINHAS[NUM_LINHAS] = {"Música","Efeitos sonoros","Ambiente","Tela cheia","Zoom do mapa","Animações",
                                           "Velocidade do texto","Batalha","Dicas","Rever as dicas","Zerar bestiário","VOLTAR"};
const char* EXPLICACOES[NUM_LINHAS] = {
    "Volume da música. Setas para os lados ajustam (de 10 em 10%).",
    "Volume dos golpes, itens e menus. A tecla O liga e desliga todo o som.",
    "Volume dos sons de cada andar (vento, gotas, lava), das salas especiais e dos passos.",
    "Ocupa a tela toda. A tecla F11 também troca, em qualquer tela do jogo.",
    "Tamanho do mapa na tela. No jogo, a roda do mouse e as teclas + e - também mudam.",
    "Rápidas: as animações da batalha levam metade do tempo.",
    "Falas e história: a velocidade com que as letras aparecem (Enter mostra tudo).",
    "Automática: as frases da luta passam sozinhas (Enter adianta). Manual: esperam o Enter.",
    "Uma caixa explicando cada novidade, na primeira vez que ela aparece.",
    "Todas as dicas voltam a aparecer (cada uma na próxima vez que a situação acontecer).",
    "Apaga os monstros já encontrados: o bestiário volta a ficar cheio de \"???\".",
    "Voltar (Esc).",
};
const char* NOMES_DO_ZOOM[3] = {"LONGE","MÉDIO","PERTO"};
const char* NOMES_DA_VELOCIDADE[4] = {"LENTA","NORMAL","RÁPIDA","NA HORA"};
std::string avisoDasOpcoes;      // "Pronto! ..." depois de rever as dicas
bool confirmandoZerar = false;   // o primeiro Enter em "Zerar bestiário" só pergunta
Rectangle linhaDasOpcoes(int i) { return {300,310.0f+i*40,840,36}; }
Rectangle setaEsquerda(int i) { Rectangle r=linhaDasOpcoes(i); return {r.x+470,r.y,50,r.height}; }
Rectangle setaDireita(int i)  { Rectangle r=linhaDasOpcoes(i); return {r.x+r.width-60,r.y,50,r.height}; }

void desenharSeta(Rectangle r,bool paraDireita,Color cor) {
    float cx=r.x+r.width/2, cy=r.y+r.height/2;
    if(paraDireita) DrawTriangle({cx-6,cy-9},{cx-6,cy+9},{cx+8,cy},cor);
    else DrawTriangle({cx+6,cy-9},{cx-8,cy},{cx+6,cy+9},cor);
}
void desenharOpcoes(int cursor) {
    const Opcoes& o=opcoes();
    titulo();
    painel({260,250,920,630});
    textoCentro("OPÇÕES",720,264,32,OURO);
    for(int i=0;i<NUM_LINHAS;i++) {
        Rectangle r=linhaDasOpcoes(i);
        bool ativa=(i==cursor);
        if(i==OP_VOLTAR) { botao({r.x+270,r.y+5,300,40},"VOLTAR",ativa); continue; }
        painel(r,ativa?Color{52,66,96,255}:PAINEL2,ativa?OURO:Color{69,89,110,255});
        texto(NOMES_DAS_LINHAS[i],r.x+26,r.y+7,22,ativa?OURO:TEXTO);
        Color cor=ativa?TEXTO:APAGADO;
        if(i==OP_MUSICA || i==OP_EFEITOS || i==OP_AMBIENTE) {
            int nivel=(i==OP_MUSICA)?o.volumeMusica:(i==OP_EFEITOS)?o.volumeEfeitos:o.volumeAmbiente;
            desenharSeta(setaEsquerda(i),false,cor);
            desenharSeta(setaDireita(i),true,cor);
            for(int k=0;k<10;k++) {
                Rectangle q={r.x+530+k*19.0f,r.y+10,14,17};
                if(k<nivel) DrawRectangleRec(q,ativa?OURO:CIANO); else DrawRectangleLinesEx(q,1.5f,{90,110,135,255});
            }
            texto(std::to_string(nivel*10)+"%",r.x+730,r.y+7,22,cor);
        } else if(i==OP_REVER) {
            std::string vistas=std::to_string(dicasVistas())+" de "+std::to_string((int)NUM_DICAS)+" já vistas";
            textoCentro(vistas,r.x+660,r.y+8,20,cor);
        } else if(i==OP_ZERAR) {
            std::string vistas=std::to_string(especiesVistas())+" de "+std::to_string(especiesDoBestiario())+" espécies vistas";
            textoCentro(vistas,r.x+660,r.y+8,20,cor);
        } else {
            std::string valor = i==OP_TELA ? (o.telaCheia?"SIM":"NÃO")
                              : i==OP_ZOOM ? NOMES_DO_ZOOM[o.zoomDoMapa]
                              : i==OP_TEXTO ? NOMES_DA_VELOCIDADE[o.velocidadeDoTexto]
                              : i==OP_ANIMACAO ? (o.animacaoRapida?"RÁPIDAS":"NORMAIS")
                              : i==OP_BATALHA ? (o.batalhaAutomatica?"AUTOMÁTICA":"MANUAL")
                              : (o.mostrarDicas?"LIGADAS":"DESLIGADAS");
            desenharSeta(setaEsquerda(i),false,cor);
            desenharSeta(setaDireita(i),true,cor);
            textoCentro(valor,r.x+655,r.y+7,22,ativa?OURO:TEXTO);
        }
    }
    std::string rodape = avisoDasOpcoes.empty() ? EXPLICACOES[cursor] : avisoDasOpcoes;
    textoCentro(rodape,720,800,20,avisoDasOpcoes.empty()?APAGADO:confirmandoZerar?PERIGO:VIDA);
    textoCentro("Setas: escolher e ajustar   |   Enter: trocar   |   Esc: voltar",720,836,18,CIANO);
}
// Muda a opção da linha (passo = -1 ou +1) e já grava e aplica. "daVolta":
// Enter ou clique no volume 100% volta para 0 (as setas param no limite).
void mudarOpcao(int linha,int passo,bool daVolta) {
    Opcoes& o=opcoes();
    avisoDasOpcoes.clear();
    if(linha!=OP_ZERAR) confirmandoZerar=false;
    if(linha==OP_MUSICA || linha==OP_EFEITOS || linha==OP_AMBIENTE) {
        int& nivel=(linha==OP_MUSICA)?o.volumeMusica:(linha==OP_EFEITOS)?o.volumeEfeitos:o.volumeAmbiente;
        nivel+=passo;
        if(nivel>10) nivel=daVolta?0:10;
        if(nivel<0) nivel=0;
        salvarOpcoes();
        aplicarVolumes();
        if(linha==OP_EFEITOS) tocarSom(SOM_ITEM);   // para ouvir o volume novo
        if(linha==OP_AMBIENTE) tocarPasso(PASSO_PEDRA,1.0f);
        return;
    }
    if(linha==OP_TELA) { alternarTelaCheia(); return; }
    if(linha==OP_TEXTO) {
        o.velocidadeDoTexto+=passo;
        if(o.velocidadeDoTexto>3) o.velocidadeDoTexto=daVolta?0:3;
        if(o.velocidadeDoTexto<0) o.velocidadeDoTexto=0;
    }
    if(linha==OP_ZOOM) {
        o.zoomDoMapa+=passo;
        if(o.zoomDoMapa>2) o.zoomDoMapa=daVolta?0:2;
        if(o.zoomDoMapa<0) o.zoomDoMapa=0;
    }
    if(linha==OP_ZERAR) {
        if(passo<0) return;
        if(!confirmandoZerar) {
            confirmandoZerar=true;
            avisoDasOpcoes="Tem certeza? Aperte Enter de novo para apagar o bestiário (não dá para desfazer).";
            return;
        }
        confirmandoZerar=false;
        zerarBestiario();
        avisoDasOpcoes="Pronto! O bestiário foi zerado.";
        return;
    }
    if(linha==OP_ANIMACAO) o.animacaoRapida=!o.animacaoRapida;
    if(linha==OP_BATALHA) o.batalhaAutomatica=!o.batalhaAutomatica;
    if(linha==OP_DICAS) o.mostrarDicas=!o.mostrarDicas;
    if(linha==OP_REVER) {
        if(passo<0) return;
        zerarDicas();
        avisoDasOpcoes="Pronto! Todas as dicas vão aparecer de novo.";
    }
    salvarOpcoes();
}
}

void mostrarOpcoes() {
    int cursor=0;
    avisoDasOpcoes.clear();
    confirmandoZerar=false;
    while(!WindowShouldClose()) {
        Efeitos::trocarDeTela(54);
        iniciarQuadro(); desenharOpcoes(cursor); terminarQuadro();
        if(IsKeyPressed(KEY_ESCAPE)) return;
        if(teclaSubir()) { cursor=(cursor+NUM_LINHAS-1)%NUM_LINHAS; avisoDasOpcoes.clear(); confirmandoZerar=false; tocarSom(SOM_MENU); }
        if(teclaDescer()) { cursor=(cursor+1)%NUM_LINHAS; avisoDasOpcoes.clear(); confirmandoZerar=false; tocarSom(SOM_MENU); }
        bool esquerda=IsKeyPressed(KEY_LEFT)||IsKeyPressedRepeat(KEY_LEFT)||IsKeyPressed(KEY_A);
        bool direita=IsKeyPressed(KEY_RIGHT)||IsKeyPressedRepeat(KEY_RIGHT)||IsKeyPressed(KEY_D);
        if((esquerda||direita) && cursor!=OP_VOLTAR && cursor!=OP_REVER && cursor!=OP_ZERAR) {
            mudarOpcao(cursor,direita?+1:-1,false);
            if(cursor!=OP_EFEITOS) tocarSom(SOM_MENU);
        }
        // Mouse: as setas ajustam; clicar na linha é igual ao Enter.
        int clicada=-1, passo=+1;
        bool naSeta=false;
        for(int i=0;i<NUM_LINHAS;i++) {
            bool temSetas=(i!=OP_VOLTAR && i!=OP_REVER && i!=OP_ZERAR);
            if(temSetas && clicou(setaEsquerda(i))) { clicada=i; passo=-1; naSeta=true; }
            else if(temSetas && clicou(setaDireita(i))) { clicada=i; naSeta=true; }
            else if(clicou(linhaDasOpcoes(i))) clicada=i;
        }
        if(confirmar()) clicada=cursor;
        if(clicada>=0) {
            cursor=clicada;
            tocarSom(SOM_MENU);
            if(clicada==OP_VOLTAR) return;
            mudarOpcao(clicada,passo,!naSeta);
        }
    }
}

// Pergunta antes de começar outra partida por cima da salva.
static bool confirmarNovoJogo() {
    int cursor=1;   // começa em "VOLTAR", para ninguém apagar sem querer
    Rectangle botoes[2]={{430,520,280,62},{730,520,280,62}};
    auto desenhar=[&](int c){
        titulo();
        painel({380,330,680,300});
        textoCentro("COMEÇAR UM NOVO JOGO?",720,360,32,OURO);
        textoCentro("A partida salva será apagada:",720,420,22,TEXTO);
        textoCentro(resumoDoSave,720,452,22,CIANO);
        botao(botoes[0],"SIM, NOVO JOGO",c==0);
        botao(botoes[1],"VOLTAR",c==1);
    };
    while(!WindowShouldClose()) {
        Efeitos::trocarDeTela(53);
        iniciarQuadro(); desenhar(cursor); terminarQuadro();
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_A)||IsKeyPressed(KEY_D)) { cursor=1-cursor; tocarSom(SOM_MENU); }
        if(IsKeyPressed(KEY_ESCAPE)) return false;
        int escolhida=-1;
        if(confirmar()) escolhida=cursor;
        for(int i=0;i<2;i++) if(clicou(botoes[i])) escolhida=i;
        if(escolhida>=0) {
            tocarSom(SOM_MENU);
            Efeitos::marcarClique(botoes[escolhida]);
            double t0=GetTime();
            while(GetTime()-t0<0.12 && !WindowShouldClose()) { iniciarQuadro(); desenhar(escolhida); terminarQuadro(); }
            return escolhida==0;
        }
    }
    return false;
}

// Executa a opção escolhida. Devolve a escolha do menu, ou -1 = continua no menu.
static int executar(int opcao) {
    if(opcao==CONTINUAR) return MENU_CONTINUAR;
    if(opcao==JOGAR) {
        if(temSave && !confirmarNovoJogo()) return -1;
        return MENU_NOVO_JOGO;
    }
    if(opcao==SAIR) return MENU_SAIR;
    if(opcao==COMO_JOGAR) mostrarPagina("COMO JOGAR", textoComoJogar());
    if(opcao==ITENS) mostrarPagina("ITENS", textoDosItens());
    if(opcao==PONTUACAO) mostrarPagina("PONTUAÇÃO E RANKING", textoDaPontuacao());
    if(opcao==BESTIARIO_OPCAO) mostrarBestiario(nullptr);
    if(opcao==CONQUISTAS_OPCAO) mostrarConquistas(nullptr);   // v1.26
    if(opcao==OPCOES_OPCAO) mostrarOpcoes();
    return -1;
}

int mostrarMenuPrincipal() {
    int cursor=0;
    temSave=existePartidaSalva(&resumoDoSave);
    tocarMusica(MUSICA_MENU);
    tocarAmbiente(SEM_AMBIENTE);
    volumeDasSalas(0,0);
    while(!WindowShouldClose()) {
        // Desenha ANTES de ler o teclado: terminarQuadro() atualiza as teclas.
        // Assim a tecla que abriu este menu (Esc, Enter...) não é lida de novo.
        Efeitos::trocarDeTela(50);   // escurece ao voltar de outra tela
        iniciarQuadro(); inicio(cursor); terminarQuadro();

        int n=numBotoes();
        if(teclaSubir()) { cursor=(cursor+n-1)%n; tocarSom(SOM_MENU); }
        if(teclaDescer()) { cursor=(cursor+1)%n; tocarSom(SOM_MENU); }
        if(IsKeyPressed(KEY_O)) alternarSom();
        int escolhida=-1;
        for(int i=0;i<n;i++) if(clicou(botaoDoMenu(i))) escolhida=i;
        if(confirmar()) escolhida=cursor;
        if(escolhida>=0) {
            tocarSom(SOM_MENU);
            cursor=escolhida;
            apertar(botaoDoMenu(escolhida),inicio,cursor);
            int r=executar(opcaoDoBotao(escolhida));
            if(r>=0) return r;
        }
        if(IsKeyPressed(KEY_ESCAPE))return MENU_SAIR;
    }
    return MENU_SAIR;
}

static void desenharPausa(int cursor) {
    const char* nomes[]={"CONTINUAR","SALVAR E IR AO MENU","SALVAR E SAIR","OPÇÕES"};
    titulo();
    painel({397,265,646,520});
    textoCentro("JOGO PAUSADO",720,291,39,OURO);
    for(int i=0;i<4;i++)botao({460.0f,367.0f+i*92,520,68},nomes[i],cursor==i);
}

int mostrarMenuDePausa() {
    int cursor=0;
    while(!WindowShouldClose()) {
        // Desenha antes de ler o teclado (mesmo motivo do menu principal):
        // sem isso, o Esc que abriu a pausa fechava a pausa na mesma hora.
        Efeitos::trocarDeTela(52);
        iniciarQuadro(); desenharPausa(cursor); terminarQuadro();

        if(IsKeyPressed(KEY_ESCAPE))return 0;
        if(teclaSubir()) { cursor=(cursor+3)%4; tocarSom(SOM_MENU); }
        if(teclaDescer()) { cursor=(cursor+1)%4; tocarSom(SOM_MENU); }
        int escolhida=-1;
        if(confirmar()) escolhida=cursor;
        for(int i=0;i<4;i++) if(clicou({460.0f,367.0f+i*92,520,68})) { escolhida=i; cursor=i; }
        if(escolhida>=0) { tocarSom(SOM_MENU); apertar({460.0f,367.0f+escolhida*92,520,68},desenharPausa,cursor); }
        if(escolhida==3) { mostrarOpcoes(); continue; }
        if(escolhida>=0) return escolhida;
    }
    return 2;
}
