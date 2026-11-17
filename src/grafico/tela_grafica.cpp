// CAMADA VISUAL 2D. Usa o Jogo original como fonte dos dados. Nenhuma regra
// de movimento, batalha, visão, inventário ou IA foi refeita aqui.
#include "tela.h"
#include "terminal.h"
#include "visual.h"
#include "jogo.h"
#include "batalha.h"
#include "itens.h"
#include "monstros.h"
#include "sprites.h"
#include "extra_sprites.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int TAM = 24;
constexpr int MX = 39;
constexpr int MY = 122;
constexpr int CW = CAMERA_LARGURA;
constexpr int CH = CAMERA_ALTURA;
const Jogo* ultimoJogo = nullptr;
using namespace Ui;

void separador(float x,float y,float w) { DrawLineEx({x,y},{x+w,y},1.6f,{74,93,113,180}); }
void titulo(const std::string& subtitulo) {
    fundoEstrelado();
    texto("MASMORRA",39,26,41,OURO);
    texto(subtitulo,300,42,23,APAGADO);
    separador(39,94,1362);
}
Color multiplicar(Color c,float f) {
    f=std::clamp(f,0.0f,1.0f);
    return {(unsigned char)(c.r*f),(unsigned char)(c.g*f),(unsigned char)(c.b*f),c.a};
}
const Sprite& imagemMonstro(int tipo) {
    if (tipo<0 || tipo>=NUM_TIPOS_DE_MONSTRO) return GOBLIN;
    switch(TIPOS_DE_MONSTRO[tipo].simbolo) {
        case 'r':return RATO;
        case 'k':return KOBOLD;
        case 'j':return CHACAL;
        case 'm':return MACACO;
        case 't':return SAPO;
        case 'g':return GOBLIN;
        case 'v':return MORCEGO;
        case 'O':return OGRO;
        case 'N':return NAGA;
    }
    return GOBLIN;
}
void desenharSprite(const Sprite& sp,float x,float y,float pixel,float iluminacao=1.0f) {
    // A arte e a paleta de pixels ficam em sprites.h, extra_sprites.h e visual.cpp.
    sprite(sp.data(),x,y,pixel,iluminacao);
}
void tile(const Jogo& jogo,const Andar& a,int x,int y,int px,int py) {
    if(!dentroDoMapa(x,y) || !a.descoberto[y][x]) {
        DrawRectangle(px,py,TAM,TAM,{8,12,24,255});
        return;
    }
    bool agora=jogo.visivel[y][x];
    float claridade=agora?std::clamp(brilho(jogo.luz[y][x]),0.27f,1.0f):0.29f;
    auto tipo=a.mapa.celulas[y][x];
    Color chao=agora?Color{72,64,60,255}:Color{24,33,55,255};
    Color parede=agora?Color{121,102,90,255}:Color{40,49,70,255};
    Color cor= (tipo==PAREDE || tipo==TOCHA) ? parede : chao;
    if(agora)cor=multiplicar(cor,claridade);
    DrawRectangle(px,py,TAM,TAM,cor);
    if(tipo==PAREDE || tipo==TOCHA) {
        // Ladrilhos retangulares e linhas de rejunte.
        DrawLine(px,py+TAM/2,px+TAM,py+TAM/2,multiplicar({31,37,48,255},claridade));
        DrawLine(px+TAM/2,py,px+TAM/2,py+TAM/2,multiplicar({31,37,48,255},claridade));
        DrawLine(px+TAM/4,py+TAM/2,px+TAM/4,py+TAM,multiplicar({31,37,48,255},claridade));
    } else {
        DrawRectangleLines(px,py,TAM,TAM,multiplicar({39,43,56,255},claridade));
        if((x*31+y*17)%7==0) DrawLine(px+7,py+13,px+11,py+17,multiplicar({108,101,94,255},claridade));
    }
    if (tipo==PORTA || tipo==PORTA_TRANCADA) {
        DrawRectangle(px+5,py+3,14,18,agora ? Color{122,79,39,255}:Color{60,52,49,255});
        DrawRectangleLines(px+5,py+3,14,18,{206,161,87,255});
        DrawCircle(px+16,py+12,2,tipo==PORTA_TRANCADA?OURO:CIANO);
    } else if(tipo==ESCADA_DESCE || tipo==ESCADA_SOBE) {
        DrawRectangle(px+3,py+5,18,15,{39,61,73,255});
        for(int i=0;i<3;i++)DrawRectangle(px+5,py+6+i*4,14,2,CIANO);
        textoCentro(tipo==ESCADA_DESCE?"v":"^",px+12,py-2,18,OURO);
    } else if(tipo==TOCHA && agora) {
        DrawRectangle(px+10,py+13,4,9,{105,75,50,255});
        DrawCircleGradient(px+12,py+8,11,{255,173,48,150},{255,173,48,0});
        DrawCircle(px+12,py+8,4,{255,211,96,255});
    }
    if (agora) {
        int idx = monstroEm(a,x,y);
        if(idx>=0) {
            desenharSprite(imagemMonstro(a.monstros[idx].tipo),px+2,py+1,1.25f,claridade);
        } else {
            int item=itemEm(a,x,y);
            if(item>=0) {
                auto cat=TIPOS_DE_ITEM[a.itens[item].tipo].categoria;
                Color c=(cat==POCAO)?PERIGO:(cat==CRISTAL)?CIANO:(cat==CHAVE)?OURO:Color{190,182,146,255};
                DrawCircle(px+12,py+12,7,{17,25,40,200});
                if(cat==POCAO) {
                    DrawRectangle(px+9,py+5,6,4,{225,217,185,255});
                    DrawRectangleRounded({(float)px+6,(float)py+9,12,12},0.26f,4,c);
                } else if(cat==CHAVE) {
                    DrawCircleLines(px+8,py+9,4,c);DrawLineEx({(float)px+10,(float)py+12},{(float)px+19,(float)py+20},3,c);
                } else {
                    DrawPoly({(float)px+12,(float)py+12},4,9,45,c);
                }
            }
        }
    } else {
        int item=itemEm(a,x,y);
        if(item>=0 && a.itens[item].visto) {
            DrawCircle(px+12,py+12,4,{87,103,130,255});
        }
    }
    if(x==jogo.jogador.x && y==jogo.jogador.y) {
        DrawCircle(px+12,py+12,12,{220,205,141,55});
        desenharSprite(HEROI,px+2,py+1,1.25f,1.0f);
    }
}
void hud(const Jogo& jogo) {
    const Jogador& j=jogo.jogador;
    painel({1001,122,400,505});
    texto("EXPLORADOR",1025,143,25,OURO);
    desenharSprite(HEROI,1198,172,5.0f);
    texto("Andar",1027,247,20,APAGADO);
    texto(std::to_string(jogo.andar)+" / "+std::to_string(NUM_ANDARES),1027,272,32,TEXTO);
    texto("Nível "+std::to_string(j.nivel),1027,320,24,CIANO);
    texto("Vida "+std::to_string(j.vida)+" / "+std::to_string(j.vidaMaxima),1027,369,21,TEXTO);
    barra(1027,401,341,19,j.vida,j.vidaMaxima);
    texto("Vidas restantes: "+std::to_string(j.vidas),1027,447,20,TEXTO);
    separador(1027,488,341);
    texto("Exploracao",1027,504,19,APAGADO);
    texto("Passos: "+std::to_string(jogo.passos),1027,536,19,TEXTO);
    texto("Monstros: "+std::to_string(jogo.monstrosMortos),1027,564,19,TEXTO);
    texto("Itens: "+std::to_string(jogo.itensColetados),1027,592,19,TEXTO);
    painel({39,648,1362,120});
    texto("MENSAGENS",61,662,19,OURO);
    int n=(int)jogo.historico.size();
    int min=std::max(0,n-2);
    for(int i=min;i<n;i++) texto(jogo.historico[i],61,695+26*(i-min),20,TEXTO);
    painel({39,782,1362,76},PAINEL2);
    texto("SETAS / WASD / HJKL: andar",61,796,19,APAGADO);
    const char* rotulos[]={"[I] MOCHILA","[Z] DESCANSAR","[?] AJUDA","[M] HISTORICO"};
    const int atalhos[]={'i','z','?','M'};
    for(int i=0;i<4;i++) {
        Rectangle r={440.0f+i*238,794,221,48};
        painel(r,PAINEL,Color{67,100,129,255});
        textoCentro(rotulos[i],r.x+r.width/2,r.y+13,19,CIANO);
        registrarAtalho(r,atalhos[i]);
    }
    texto("ESC: pausa",62,830,17,APAGADO);
}
void mapa(const Jogo& jogo) {
    titulo("EXPLORAÇÃO   |   O mapa se revela conforme você caminha");
    painel({33,115,948,516}, {16,22,36,255});
    int cx,cy;posicaoDaCamera(jogo,cx,cy);
    const Andar& a=andarAtual(jogo);
    for(int iy=0;iy<CH;iy++)for(int ix=0;ix<CW;ix++)
        tile(jogo,a,cx+ix,cy+iy,MX+ix*TAM,MY+iy*TAM);
    DrawRectangleLinesEx({(float)MX,(float)MY,(float)CW*TAM,(float)CH*TAM},2,{83,110,133,255});
    hud(jogo);
}
void overlayBasico(const std::string& tituloModal) {
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,177});
    painel({215,145,1010,610},{26,35,56,255},OURO);
    textoCentro(tituloModal,720,174,31,OURO);
    separador(255,222,930);
}
void faixaBotao(const std::string& txt,Rectangle r,bool sel,int indice=-1,int cursor=0) {
    Color fundo=sel?OURO:PAINEL2;
    painel(r,fundo,sel?OURO:Color{70,89,110,255});
    texto(txt,r.x+20,r.y+12,23,sel?FUNDO:TEXTO);
    if(indice>=0) registrarOpcao(r,indice,cursor);
}
void telaBatalha(const Jogo& jogo,const Batalha& b) {
    titulo("BATALHA   |   Regras e turnos do jogo original");
    painel({39,121,1362,468});
    DrawRectangleGradientV(42,125,1356,462,{25,40,60,255},{30,29,45,255});
    // Arena estilizada, chão ao redor dos personagens.
    DrawEllipse(350,493,275,50,{44,63,76,255});
    DrawEllipse(1010,466,280,54,{44,63,76,255});
    desenharSprite(HEROI,200,267,12.2f);
    painel({55,140,414,107},{18,28,45,255},CIANO);
    texto("VOCÊ  |  NÍVEL "+std::to_string(b.jogador.nivel),74,153,22,CIANO);
    texto("Vida "+std::to_string(b.vidaMostrada[0])+" / "+std::to_string(b.jogador.vidaMaxima),74,182,20,TEXTO);
    barra(74,216,370,13,b.vidaMostrada[0],b.jogador.vidaMaxima);
    if(b.numInimigos==1) {
        const Monstro& m=*b.monstros[0];
        desenharSprite(imagemMonstro(m.tipo),892,257,12.3f);
        painel({909,140,474,106},{18,28,45,255},OURO);
        texto(std::string(tipoDe(m).nome)+"  |  Nv "+std::to_string(m.nivel),931,153,22,OURO);
        texto("Vida "+std::to_string(b.vidaMostrada[1])+" / "+std::to_string(b.inimigos[0].vidaMaxima),931,181,20,TEXTO);
        barra(931,215,426,13,b.vidaMostrada[1],b.inimigos[0].vidaMaxima,PERIGO);
    } else {
        for (int i=0;i<b.numInimigos;i++) {
            if(!b.naLuta[i])continue;
            int px=735+i*211;
            desenharSprite(imagemMonstro(b.monstros[i]->tipo),(float)px,270,8.5f);
            painel({(float)px-20,142,204,110},{18,28,45,255},(b.menu==MENU_ALVO && b.alvos[b.cursor]==i)?OURO:CIANO);
            texto(tipoDe(*b.monstros[i]).nome,(float)px-6,152,18,OURO);
            texto("Nv "+std::to_string(b.monstros[i]->nivel),(float)px-6,178,16,APAGADO);
            barra((float)px-6,211,165,11,b.vidaMostrada[i+1],b.inimigos[i].vidaMaxima,PERIGO);
        }
    }
    painel({39,608,1362,243},{26,36,57,255});
    texto("COMANDOS",62,625,19,OURO);
    texto(b.texto,62,664,22,TEXTO);
    if(b.menu==MENU_PRINCIPAL) {
        const char* nomes[]={"LUTAR", "ITENS", "STATUS", "FUGIR"};
        for(int i=0;i<4;i++) {
            int col=i%2,row=i/2;
            faixaBotao(nomes[i],{757.0f+col*302,646.0f+row*76,275,61},b.cursor==i,i,b.cursor);
        }
        int total=0;for(int i=0;i<NUM_TIPOS_DE_ITEM;i++)if(TIPOS_DE_ITEM[i].categoria==POCAO)total+=jogo.jogador.mochila[i];
        texto("Poções: "+std::to_string(total),63,750,21,APAGADO);
    } else if(b.menu==MENU_GOLPES || b.menu==MENU_ESQUECER) {
        int n=(b.menu==MENU_ESQUECER)?MAX_GOLPES+1:b.jogador.numGolpes;
        for(int i=0;i<n;i++) {
            std::string s;
            if(i<jogo.jogador.numGolpes) {
                const Golpe& g=GOLPES[jogo.jogador.golpes[i]];
                s=std::string(g.nome)+"  |  "+NOMES_DOS_ELEMENTOS[g.elemento]+"  |  PP "+
                 (g.ppMaximo==0?std::string("--"):std::to_string(jogo.jogador.pp[i])+"/"+std::to_string(g.ppMaximo));
            } else s="Não aprender";
            faixaBotao(s,{740.0f,625.0f+i*41,631,37},b.cursor==i,i,b.cursor);
        }
        if(b.menu==MENU_ESQUECER) texto("Novo golpe: "+std::string(GOLPES[b.golpeNovo].nome),63,722,20,CIANO);
    } else if(b.menu==MENU_ITENS) {
        for(int i=0;i<b.numItensUsaveis;i++) {
            int id=b.itensUsaveis[i];
            faixaBotao(std::string(TIPOS_DE_ITEM[id].nome)+"  x"+std::to_string(jogo.jogador.mochila[id]),
                {740.0f,630.0f+i*40,631,36},b.cursor==i,i,b.cursor);
        }
    } else if(b.menu==MENU_ALVO) {
        for(int i=0;i<b.numAlvos;i++) {
            int id=b.alvos[i];
            faixaBotao(b.inimigos[id].nome,{740.0f,630.0f+i*49,631,44},b.cursor==i,i,b.cursor);
        }
    } else if(b.menu==MENU_STATUS) {
        texto("ATQ: "+std::to_string(b.jogador.ataque)+"   DEF: "+std::to_string(b.jogador.defesa)+
            "   VEL: "+std::to_string(b.jogador.velocidade),710,684,22,CIANO);
        texto("Aperte uma tecla para voltar",710,750,20,APAGADO);
    } else if(b.menu==MENU_NENHUM) {
        texto("ENTER para continuar",1070,770,19,CIANO);
    }
    texto("Setas: escolher     Enter: confirmar     Esc: voltar",62,811,19,APAGADO);
}
}

void posicaoDaCamera(const Jogo& jogo,int& cameraX,int& cameraY) {
    cameraX=std::clamp(jogo.jogador.x-CAMERA_LARGURA/2,0,MAPA_LARGURA-CAMERA_LARGURA);
    cameraY=std::clamp(jogo.jogador.y-CAMERA_ALTURA/2,0,MAPA_ALTURA-CAMERA_ALTURA);
}
void forcarLimpezaDaTela() {}
void desenharJogo(const Jogo& jogo) {
    ultimoJogo=&jogo;
    limparOpcoes();Ui::iniciarQuadro();mapa(jogo);Ui::terminarQuadro();
}
void desenharBatalha(const Jogo& jogo,const Batalha& b) {
    limparOpcoes();Ui::iniciarQuadro();telaBatalha(jogo,b);Ui::terminarQuadro();
}
void desenharMenuEmCaixa(const std::string& nome,const std::vector<std::string>& opcoes,
                         const std::string& descricao,int cursor) {
    limparOpcoes();Ui::iniciarQuadro();if(ultimoJogo)mapa(*ultimoJogo);
    overlayBasico(nome);
    int primeira=std::max(0,cursor-6);
    int n=std::min(12,(int)opcoes.size());
    if(primeira+n>(int)opcoes.size()) primeira=std::max(0,(int)opcoes.size()-n);
    for(int i=primeira;i<primeira+n;i++)
        faixaBotao(opcoes[i],{274.0f,239.0f+(i-primeira)*34,890,30},cursor==i,i,cursor);
    texto(descricao,275,675,19,APAGADO);
    texto("Setas e Enter: selecionar   |   Esc: voltar",275,707,19,CIANO);
    Ui::terminarQuadro();
}
void desenharAjuda(const Jogo& jogo) {
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("COMANDOS DO MASMORRA");
    const std::vector<std::string> linhas={
        "Setas / H J K L: mover o aventureiro",
        "Diagonais: Y U B N ou teclado numerico",
        "Encoste nos monstros para comecar a batalha",
        "I: abrir mochila e usar ou equipar itens",
        "Z: descansar e recuperar vida",
        "M: mensagens anteriores       Q: sair da partida",
        "Suba ou desca andando sobre as escadas do mapa",
        "O mapa so mostra areas visiveis ou ja descobertas",
        "Na batalha: LUTAR, ITENS, STATUS e FUGIR",
        "Semente desta partida: "+std::to_string(jogo.semente)};
    for(size_t i=0;i<linhas.size();i++)texto(linhas[i],263,245+i*41,22,i%2?TEXTO:CIANO);
    texto("Aperte qualquer tecla para retornar",263,696,20,OURO);Ui::terminarQuadro();
}
void desenharHistorico(const Jogo& jogo) {
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("HISTORICO");
    size_t n=jogo.historico.size();size_t ini=n>14?n-14:0;
    for(size_t i=ini;i<n;i++)texto(jogo.historico[i],263,245+31*(i-ini),19,TEXTO);
    texto("Aperte qualquer tecla para retornar",263,700,20,OURO);Ui::terminarQuadro();
}
void desenharMorte(const Jogo& jogo) {
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("FIM DE JOGO");
    texto("Suas vidas acabaram no andar "+std::to_string(jogo.andar),300,290,28,PERIGO);
    texto(jogo.causaDaMorte,300,356,22,TEXTO);
    texto("Nivel "+std::to_string(jogo.jogador.nivel)+"  |  Monstros: "+std::to_string(jogo.monstrosMortos),300,408,22,APAGADO);
    texto("Semente: "+std::to_string(jogo.semente),300,455,22,CIANO);
    texto("N: nova partida       Q: menu principal",300,601,24,OURO);
    Ui::terminarQuadro();
}
void desenharAviso(const std::string& a,const std::string& b) {
    Ui::iniciarQuadro();titulo("AVISO");texto(a,140,270,25,PERIGO);texto(b,140,330,24,TEXTO);Ui::terminarQuadro();
}
