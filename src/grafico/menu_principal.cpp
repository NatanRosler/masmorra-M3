// Menu 2D do Masmorra. As mecanicas estao em jogo.cpp, batalha.cpp, etc.
#include "menu_principal.h"
#include "visual.h"
#include "sprites.h"
#include "extra_sprites.h"
#include "raylib.h"
#include "pontuacao.h"
#include "som.h"
#include "efeitos.h"
#include "salvar.h"
#include <array>
#include <cmath>
#include <string>

namespace {
using namespace Ui;
enum Pagina { INICIO, PERSONAGENS };
// Com partida salva aparece mais um botão ("CONTINUAR") no topo.
enum Opcao { CONTINUAR, JOGAR, COMO_JOGAR, ITENS, PONTUACAO, PERSONAGENS_OPCAO, SAIR, NUM_OPCOES };
const char* NOMES_DAS_OPCOES[NUM_OPCOES] = {"CONTINUAR","NOVO JOGO","COMO JOGAR","ITENS","PONTUAÇÃO","PERSONAGENS","SAIR"};
bool temSave = false;          // atualizado ao abrir o menu
std::string resumoDoSave;
int numBotoes() { return temSave ? NUM_OPCOES : NUM_OPCOES - 1; }
// A opção do botão i (sem save, o primeiro botão já é "NOVO JOGO").
int opcaoDoBotao(int i) { return temSave ? i : i + 1; }
Rectangle botaoDoMenu(int i) {
    // 6 botões com espaço de 66; 7 botões ficam um pouco mais juntos.
    return temSave ? Rectangle{155,350.0f+i*58,390,50} : Rectangle{155,352.0f+i*66,390,56};
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
    DrawLineEx({150,242},{1275,242},2,{66,107,126,255});
}
void personagens() {
    titulo();
    texto("PERSONAGENS",149,255,32,OURO);
    struct Carta { const Sprite* arte; const char* nome; const char* tipo; };
    const std::array<Carta, 14> cartas = {{
        {&HEROI,"AVENTUREIRO","você"}, {&RATO,"RATO","andares 1-4"}, {&KOBOLD,"KOBOLD","andares 1-4"},
        {&CHACAL,"CHACAL","andares 2-6, em bando"}, {&MACACO,"MACACO","andares 2-6, medroso"},
        {&SAPO,"SAPO","andares 2-6"}, {&GOBLIN,"GOBLIN","andares 3-10"},
        {&MORCEGO,"MORCEGO VAMPIRO","andares 4-9, rápido"}, {&OGRO,"OGRO","andares 6-10"},
        {&NAGA,"NAGA","andares 8-10"}, {&SENHOR,"O SENHOR","chefe final, andar 10"},
        {&EXPLORADORA,"ELDRA","exploradora, andar 1"}, {&MINEIRO,"BORIN","velho mineiro, andar 4"},
        {&ESPIRITO,"ESPÍRITO","o guardião, andar 7"},
    }};
    for(int i=0;i<(int)cartas.size();i++) {
        int coluna=i%4, linha=i/4;
        float x=150+(float)coluna*285, y=300+(float)linha*130;
        painel({x,y,270,118},PAINEL);
        sprite(cartas[i].arte->data(),x+12,y+14,5.4f);
        texto(cartas[i].nome,x+104,y+34,cartas[i].nome[0]=='M'?16:18,TEXTO);
        texto(cartas[i].tipo,x+104,y+62,14,APAGADO);
    }
    texto("A cor dos monstros não diz o elemento: ele é sorteado a cada partida!",150,830,18,CIANO);
    botao({960,812,330,46},"VOLTAR (ESC)",false);
}
void inicio(int cursor) {
    titulo();
    texto("Explore dez andares, descubra itens e",151,279,25,TEXTO);
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
          std::string(somLigado()?"ligado":"desligado"),151,770,20,APAGADO);
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
static int executar(int opcao, Pagina& pagina) {
    if(opcao==CONTINUAR) return MENU_CONTINUAR;
    if(opcao==JOGAR) {
        if(temSave && !confirmarNovoJogo()) return -1;
        return MENU_NOVO_JOGO;
    }
    if(opcao==SAIR) return MENU_SAIR;
    if(opcao==COMO_JOGAR) mostrarPagina("COMO JOGAR", textoComoJogar());
    if(opcao==ITENS) mostrarPagina("ITENS", textoDosItens());
    if(opcao==PONTUACAO) mostrarPagina("PONTUAÇÃO E RANKING", textoDaPontuacao());
    if(opcao==PERSONAGENS_OPCAO) pagina=PERSONAGENS;
    return -1;
}

int mostrarMenuPrincipal() {
    Pagina pagina=INICIO;
    int cursor=0;
    temSave=existePartidaSalva(&resumoDoSave);
    tocarMusica(MUSICA_MENU);
    while(!WindowShouldClose()) {
        // Desenha ANTES de ler o teclado: terminarQuadro() atualiza as teclas.
        // Assim a tecla que abriu este menu (Esc, Enter...) não é lida de novo.
        Efeitos::trocarDeTela(pagina==INICIO?50:51);   // escurece ao trocar de página
        iniciarQuadro();
        if(pagina==INICIO)inicio(cursor);
        else personagens();
        terminarQuadro();

        if(pagina==INICIO) {
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
                int r=executar(opcaoDoBotao(escolhida),pagina);
                if(r>=0) return r;
            }
            if(IsKeyPressed(KEY_ESCAPE))return MENU_SAIR;
        } else if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || confirmar() || clicou({960,812,330,46})) {
            pagina=INICIO;
        }
    }
    return MENU_SAIR;
}

static void desenharPausa(int cursor) {
    const char* nomes[]={"CONTINUAR","SALVAR E IR AO MENU","SALVAR E SAIR",somLigado()?"SOM: LIGADO":"SOM: DESLIGADO"};
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
        if(escolhida==3) { alternarSom(); continue; }
        if(escolhida>=0) return escolhida;
    }
    return 2;
}
