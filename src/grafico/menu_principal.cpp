// Menu 2D do Masmorra. As mecanicas estao em jogo.cpp, batalha.cpp, etc.
#include "menu_principal.h"
#include "visual.h"
#include "sprites.h"
#include "extra_sprites.h"
#include "raylib.h"
#include "pontuacao.h"
#include "som.h"
#include <array>
#include <string>

namespace {
using namespace Ui;
enum Pagina { INICIO, PERSONAGENS };
constexpr int NUM_BOTOES = 6;
constexpr Rectangle BOTAO[NUM_BOTOES] = {
    {155,352,390,56}, {155,418,390,56}, {155,484,390,56},
    {155,550,390,56}, {155,616,390,56}, {155,682,390,56}
};
enum Opcao { JOGAR, COMO_JOGAR, ITENS, PONTUACAO, PERSONAGENS_OPCAO, SAIR };

bool clicou(Rectangle r) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r);
}
bool teclaSubir() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
bool teclaDescer() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
bool confirmar() { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
void botao(Rectangle r, const std::string& legenda, bool ativo) {
    painel(r,ativo?OURO:PAINEL2,ativo?OURO:Color{69,89,110,255});
    textoCentro(legenda,r.x+r.width/2,r.y+(r.height-27)/2,27,ativo?FUNDO:TEXTO);
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
    const char* opcoes[NUM_BOTOES]={"JOGAR","COMO JOGAR","ITENS","PONTUAÇÃO","PERSONAGENS","SAIR"};
    for(int i=0;i<NUM_BOTOES;i++) {
        bool hover=CheckCollisionPointRec(mouseVirtual(),BOTAO[i]);
        botao(BOTAO[i],opcoes[i],i==cursor||hover);
    }
    painel({690,291,550,452},{22,32,51,255});
    DrawEllipse(964,684,204,30,{12,15,28,255});
    sprite(HEROI.data(),794,375,15.6f);
    sprite(GOBLIN.data(),1099,521,8.0f);
    texto("SEU JOGO, SUAS REGRAS",749,705,22,CIANO);
    texto("Setas / mouse para escolher  |  Enter para confirmar  |  O: som "+
          std::string(somLigado()?"ligado":"desligado"),151,770,20,APAGADO);
}
}

// Executa a opção escolhida. Devolve 1 = jogar, 0 = sair, -1 = continua no menu.
static int executar(int opcao, Pagina& pagina) {
    if(opcao==JOGAR) return 1;
    if(opcao==SAIR) return 0;
    if(opcao==COMO_JOGAR) mostrarPagina("COMO JOGAR", textoComoJogar());
    if(opcao==ITENS) mostrarPagina("ITENS", textoDosItens());
    if(opcao==PONTUACAO) mostrarPagina("PONTUAÇÃO E RANKING", textoDaPontuacao());
    if(opcao==PERSONAGENS_OPCAO) pagina=PERSONAGENS;
    return -1;
}

bool mostrarMenuPrincipal() {
    Pagina pagina=INICIO;
    int cursor=0;
    tocarMusica(MUSICA_MENU);
    while(!WindowShouldClose()) {
        // Desenha ANTES de ler o teclado: terminarQuadro() atualiza as teclas.
        // Assim a tecla que abriu este menu (Esc, Enter...) não é lida de novo.
        iniciarQuadro();
        if(pagina==INICIO)inicio(cursor);
        else personagens();
        terminarQuadro();

        if(pagina==INICIO) {
            if(teclaSubir()) { cursor=(cursor+NUM_BOTOES-1)%NUM_BOTOES; tocarSom(SOM_MENU); }
            if(teclaDescer()) { cursor=(cursor+1)%NUM_BOTOES; tocarSom(SOM_MENU); }
            if(IsKeyPressed(KEY_O)) alternarSom();
            int escolhida=-1;
            for(int i=0;i<NUM_BOTOES;i++) if(clicou(BOTAO[i])) escolhida=i;
            if(confirmar()) escolhida=cursor;
            if(escolhida>=0) {
                tocarSom(SOM_MENU);
                cursor=escolhida;
                int r=executar(escolhida,pagina);
                if(r>=0) return r==1;
            }
            if(IsKeyPressed(KEY_ESCAPE))return false;
        } else if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || confirmar() || clicou({960,812,330,46})) {
            pagina=INICIO;
        }
    }
    return false;
}

int mostrarMenuDePausa() {
    int cursor=0;
    while(!WindowShouldClose()) {
        const char* nomes[]={"CONTINUAR","MENU PRINCIPAL","SAIR",somLigado()?"SOM: LIGADO":"SOM: DESLIGADO"};
        // Desenha antes de ler o teclado (mesmo motivo do menu principal):
        // sem isso, o Esc que abriu a pausa fechava a pausa na mesma hora.
        iniciarQuadro();
        titulo();
        painel({397,265,646,520});
        textoCentro("JOGO PAUSADO",720,291,39,OURO);
        for(int i=0;i<4;i++)botao({460.0f,367.0f+i*92,520,68},nomes[i],cursor==i);
        terminarQuadro();

        if(IsKeyPressed(KEY_ESCAPE))return 0;
        if(teclaSubir()) { cursor=(cursor+3)%4; tocarSom(SOM_MENU); }
        if(teclaDescer()) { cursor=(cursor+1)%4; tocarSom(SOM_MENU); }
        int escolhida=-1;
        if(confirmar()) escolhida=cursor;
        for(int i=0;i<4;i++) if(clicou({460.0f,367.0f+i*92,520,68})) { escolhida=i; cursor=i; }
        if(escolhida==3) { alternarSom(); tocarSom(SOM_MENU); continue; }
        if(escolhida>=0) { tocarSom(SOM_MENU); return escolhida; }
    }
    return 2;
}
