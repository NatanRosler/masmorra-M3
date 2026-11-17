// Menu 2D do Masmorra. As mecanicas estao em jogo.cpp, batalha.cpp, etc.
#include "menu_principal.h"
#include "visual.h"
#include "sprites.h"
#include "extra_sprites.h"
#include "raylib.h"
#include <array>
#include <string>

namespace {
using namespace Ui;
enum Pagina { INICIO, PERSONAGENS, CONTROLES };
constexpr Rectangle BOTAO[4] = {
    {155,360,390,67}, {155,442,390,67},
    {155,524,390,67}, {155,606,390,67}
};

bool clicou(Rectangle r) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r);
}
bool teclaSubir() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
bool teclaDescer() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
bool confirmar() { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
void botao(Rectangle r, const std::string& legenda, bool ativo) {
    painel(r,ativo?OURO:PAINEL2,ativo?OURO:Color{69,89,110,255});
    textoCentro(legenda,r.x+r.width/2,r.y+19,27,ativo?FUNDO:TEXTO);
}
void titulo() {
    fundoEstrelado();
    texto("MASMORRA",145,91,76,OURO);
    texto("UMA AVENTURA PELA ESCURIDAO",152,188,23,CIANO);
    DrawLineEx({150,242},{1275,242},2,{66,107,126,255});
}
void personagens() {
    titulo();
    texto("PERSONAGENS",149,265,36,OURO);
    const std::array<const Sprite*, 9> artes = {
        &HEROI,&RATO,&KOBOLD,&CHACAL,&MACACO,&SAPO,&GOBLIN,&OGRO,&NAGA
    };
    const char* nomes[] = {"AVENTUREIRO","RATO","KOBOLD","CHACAL","MACACO","SAPO","GOBLIN","OGRO","NAGA"};
    for(int i=0;i<9;i++) {
        int coluna=i%3, linha=i/3;
        float x=165+(float)coluna*410, y=333+(float)linha*169;
        painel({x,y,370,145},PAINEL);
        sprite(artes[i]->data(),x+28,y+23,5.6f);
        texto(nomes[i],x+140,y+56,22,TEXTO);
    }
    botao({525,848,390,42},"VOLTAR (ESC)",false);
}
void controles() {
    titulo();
    painel({142,280,1150,520});
    texto("COMANDOS DO JOGO ORIGINAL",182,313,33,OURO);
    const std::array<std::string,10> linhas = {{
        "Setas, H J K L e teclado numerico: andar pelo mapa",
        "Y U B N, Home, End, PgUp e PgDn: diagonais",
        "Encoste num inimigo: iniciar batalha por turnos",
        "I: abrir a mochila e usar itens",
        "Z: descansar e recuperar pontos de vida",
        "?: ajuda      M: historico de mensagens",
        "Q: confirmar saida para o menu principal",
        "Esc: pausar (a nova interface grafica)",
        "Batalha: LUTAR, ITENS, STATUS, FUGIR",
        "Os mapas se revelam enquanto voce explora"
    }};
    for(int i=0;i<(int)linhas.size();i++) texto(linhas[i],186,385+38*i,24,(i%2==0)?CIANO:TEXTO);
    botao({525,848,390,42},"VOLTAR (ESC)",false);
}
void inicio(int cursor) {
    titulo();
    texto("Explore dez andares, descubra itens e",151,279,25,TEXTO);
    texto("enfrente criaturas em combates por turnos.",151,315,25,TEXTO);
    const char* opcoes[]={"JOGAR","PERSONAGENS","CONTROLES","SAIR"};
    for(int i=0;i<4;i++) {
        bool hover=CheckCollisionPointRec(mouseVirtual(),BOTAO[i]);
        botao(BOTAO[i],opcoes[i],i==cursor||hover);
    }
    painel({690,291,550,452},{22,32,51,255});
    DrawEllipse(964,684,204,30,{12,15,28,255});
    sprite(HEROI.data(),794,375,15.6f);
    sprite(GOBLIN.data(),1099,521,8.0f);
    texto("SEU JOGO, SUAS REGRAS",749,705,22,CIANO);
    texto("Setas / mouse para escolher  |  Enter para confirmar",151,788,20,APAGADO);
}
}

bool mostrarMenuPrincipal() {
    Pagina pagina=INICIO;
    int cursor=0;
    while(!WindowShouldClose()) {
        if(pagina==INICIO) {
            if(teclaSubir()) cursor=(cursor+3)%4;
            if(teclaDescer()) cursor=(cursor+1)%4;
            for(int i=0;i<4;i++) if(clicou(BOTAO[i])) {
                cursor=i;
                if(i==0)return true;
                if(i==1)pagina=PERSONAGENS;
                if(i==2)pagina=CONTROLES;
                if(i==3)return false;
            }
            if(confirmar()) {
                if(cursor==0)return true;
                if(cursor==1)pagina=PERSONAGENS;
                if(cursor==2)pagina=CONTROLES;
                if(cursor==3)return false;
            }
            if(IsKeyPressed(KEY_ESCAPE))return false;
        } else if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || confirmar() || clicou({525,848,390,42})) {
            pagina=INICIO;
        }
        iniciarQuadro();
        if(pagina==INICIO)inicio(cursor);
        else if(pagina==PERSONAGENS)personagens();
        else controles();
        terminarQuadro();
    }
    return false;
}

int mostrarMenuDePausa() {
    int cursor=0;
    const char* nomes[]={"CONTINUAR","MENU PRINCIPAL","SAIR"};
    while(!WindowShouldClose()) {
        if(IsKeyPressed(KEY_ESCAPE))return 0;
        if(teclaSubir())cursor=(cursor+2)%3;
        if(teclaDescer())cursor=(cursor+1)%3;
        if(confirmar())return cursor;
        for(int i=0;i<3;i++) if(clicou({460.0f,387.0f+i*92,520,68}))return i;
        iniciarQuadro();
        titulo();
        painel({397,275,646,477});
        textoCentro("JOGO PAUSADO",720,301,39,OURO);
        for(int i=0;i<3;i++)botao({460.0f,387.0f+i*92,520,68},nomes[i],cursor==i);
        terminarQuadro();
    }
    return 2;
}
