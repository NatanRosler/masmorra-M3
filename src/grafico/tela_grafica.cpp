// CAMADA VISUAL 2D. Usa o Jogo original como fonte dos dados. Nenhuma regra
// de movimento, batalha, visão, inventário ou IA foi refeita aqui.
#include "tela.h"
#include "terminal.h"
#include "visual.h"
#include "jogo.h"
#include "batalha.h"
#include "itens.h"
#include "monstros.h"
#include "pontuacao.h"
#include "som.h"
#include "dificuldade.h"
#include "sprites.h"
#include "extra_sprites.h"
#include "itens_sprites.h"
#include "salas_sprites.h"
#include "monstros_sprites.h"
#include "chefes_sprites.h"
#include "fundos_batalha.h"
#include "sala_do_chefe.h"
#include "diario.h"
#include "conquistas.h"
// v1.24: quanto o portão da sala do chefe já desceu (a cena anima de 0 a 1;
// fora dela, fechado é fechado).
float portaoDescendo = 1.0f;
#include "tema.h"
#include "icones_elementos.h"
#include "golpes_visuais.h"
#include "efeitos.h"
#include "opcoes.h"
#include "bestiario.h"
#include <functional>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// v1.27: o menu de golpes da batalha usa o texto do efeito da mochila.
namespace Mochila { std::string efeitoDoGolpe(const Golpe& g); }

namespace {
// Câmera do último mapa desenhado (para pôr o balão de fala perto da pessoa)
// e se os cartões do canto ficam escondidos (durante as falas).
float cameraX=0, cameraY=0, cameraZoom=1;
bool esconderCartoes=false;
bool esqueletosEnterrados=false;   // v1.24: a cena do Necromante ainda não levantou os esqueletos
// Nas cenas, a câmera não segue o herói: vai para (cenaX, cenaY) com o zoom
// cenaZoom (em casas; o centro da tela fica nesse ponto).
bool cameraDaCena=false;
float cenaX=0, cenaY=0, cenaZoom=1;
double zoomMudouEm=-10;
   // quando o zoom do mapa mudou (aviso "Zoom: PERTO")
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
// Cores do jogo (CorRGB, 0-255) para a raylib.
Color paraCor(CorRGB c) { return {(unsigned char)c.r,(unsigned char)c.g,(unsigned char)c.b,255}; }
Color corDoElemento(int e) { return paraCor(CORES_DOS_ELEMENTOS[e]); }
Color corDaMensagem(int tipo) {
    if (tipo==MSG_BOA) return OURO;
    if (tipo==MSG_PERIGO) return PERIGO;
    return TEXTO;
}
Color corDaDificuldade(int d) {
    if (d==FACIL) return {110,210,120,255};
    if (d==DIFICIL) return PERIGO;
    if (d==IMPOSSIVEL) return {200,90,255,255};
    return OURO;
}

// "o chacal A" -> "Chacal A" (nome do lutador sem o artigo, para os painéis).
std::string semArtigo(std::string nome) {
    if (nome.size() > 2 && (nome.rfind("o ",0)==0 || nome.rfind("a ",0)==0)) nome = nome.substr(2);
    if (!nome.empty() && nome[0]>='a' && nome[0]<='z') nome[0]=(char)(nome[0]-'a'+'A');
    return nome;
}
std::string emMaiusculas(std::string s) {
    for (size_t i=0;i<s.size();i++) {
        unsigned char c=(unsigned char)s[i];
        if (c>='a' && c<='z') s[i]=(char)(c-'a'+'A');
        // Letras acentuadas em UTF-8 (á, ã, é, í, ó, ú, ç...): 0xC3 + um byte
        // de 0xA0 a 0xBE; a maiúscula é o mesmo byte menos 0x20.
        else if (c==0xC3 && i+1<s.size()) {
            unsigned char d=(unsigned char)s[i+1];
            if (d>=0xA0 && d<=0xBE && d!=0xB7) s[i+1]=(char)(d-0x20);
            i++;
        }
    }
    return s;
}
// Lista dos elementos contra os quais "alvo" é fraco (eficácia 4) ou resiste (1).
std::string elementosCom(Elemento alvo, int eficaciaProcurada) {
    std::string lista;
    for (int e=0;e<NUM_ELEMENTOS;e++) {
        if (eficacia((Elemento)e,alvo)!=eficaciaProcurada) continue;
        if (!lista.empty()) lista += ", ";
        lista += NOMES_DOS_ELEMENTOS[e];
    }
    return lista.empty() ? "nenhum" : lista;
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
        case 'S':return SENHOR;
        case 'M':return MIMICO;
        case 'G':return GNOMO;
        case 'W':return SOMBRA;
        case 'a':return ARANHA;
        case 'f':return FUNGO;
        case 'e':return ESQUELETO;
        case 's':return SALAMANDRA;
        case 'R':return REI_GOBLIN;     // v1.24: os mini-chefes
        case 'A':return RAINHA_ARANHA;
        case 'n':return NECROMANTE;
        case 'L':return GOLEM_PEDRA;
    }
    return GOBLIN;
}
const Sprite& imagemNpc(int tipo) {
    if (tipo==NPC_MINEIRO) return MINEIRO;
    if (tipo==NPC_ESPIRITO) return ESPIRITO;
    if (tipo==NPC_COMERCIANTE) return COMERCIANTE;
    if (tipo==NPC_VIAJANTE) return VIAJANTE;
    return EXPLORADORA;
}
void desenharSprite(const Sprite& sp,float x,float y,float pixel,float iluminacao=1.0f,float alfa=1.0f) {
    // A arte e a paleta de pixels ficam em sprites.h, extra_sprites.h e visual.cpp.
    sprite(sp.data(),x,y,pixel,iluminacao,alfa);
}
// Identificação de cada tela, para o escurecimento ao trocar de uma para outra.
enum IdDaTela { TELA_MAPA = 1, TELA_BATALHA, TELA_CRIACAO, TELA_MORTE, TELA_AVISO, TELA_PAGINA = 100 };
// Etiqueta colorida com o nome do elemento (ex.: [ FOGO ]).
// v1.23: com o símbolo do elemento antes do nome.
float etiquetaDeElemento(int elemento,float x,float y,float tam=16) {
    std::string nome = emMaiusculas(NOMES_DOS_ELEMENTOS[elemento]);
    float icone=(tam+8)/9.0f*0.82f;          // tamanho do pixel do símbolo
    float w = std::max(60.0f, larguraDoTexto(nome,tam)+icone*9+30);
    DrawRectangleRounded({x,y,w,tam+8},0.5f,6,multiplicar(corDoElemento(elemento),0.45f));
    DrawRectangleRoundedLinesEx({x,y,w,tam+8},0.5f,6,1.2f,corDoElemento(elemento));
    iconeDoElemento(elemento,x+8,y+(tam+8-icone*9)/2,icone);
    textoCentro(nome,x+8+icone*9+(w-8-icone*9)/2,y+3,tam,TEXTO);
    return w;
}
void etiquetaDeStatus(int status,float x,float y) {
    if (status==SEM_STATUS) return;
    const char* nomes[]={"","ENVENENADO","DORMINDO","PARALISADO","QUEIMADO","CONGELADO","OFUSCADO","ATORDOADO","PRESO"};
    Color cores[]={TEXTO,{185,95,215,255},{120,150,230,255},{235,200,70,255},{255,130,60,255},{150,220,255,255},
                   {255,240,190,255},{255,210,90,255},{215,220,230,255}};
    DrawRectangleRounded({x,y,128,24},0.5f,6,multiplicar(cores[status],0.4f));
    textoCentro(nomes[status],x+64,y+3,15,cores[status]);
}

// Cor da célula sob a luz: a claridade escurece e a COR da luz tinge
// (vermelho perto da lava, azul nas tochas do trono...).
Color iluminarComCor(Color c,Luz l,float claridade) {
    float maior=std::max({l.r,l.g,l.b,0.001f});
    auto canal=[&](unsigned char v,float comp){ return (unsigned char)std::clamp(v*claridade*(0.55f+0.45f*comp/maior),0.0f,255.0f); };
    return {canal(c.r,l.r),canal(c.g,l.g),canal(c.b,l.b),c.a};
}
// Tapete da sala do trono: o caminho da chegada até o chefe (calculado uma vez).
bool tapete[MAPA_ALTURA][MAPA_LARGURA];
uint32_t tapeteDaSemente=0; bool tapetePronto=false;
// (v1.24: no salão do trono feito à mão, o tapete já faz parte da sala.)
bool noTapete(const Andar& a,int x,int y) { return !a.salaDoChefe.existe && tapetePronto && tapete[y][x]; }
void prepararTapete(const Jogo& jogo) {
    if(temaDoAndar(jogo.andar)!=TEMA_TRONO) return;
    if(tapetePronto && tapeteDaSemente==jogo.semente) return;
    const Andar& a=andarAtual(jogo);
    int chefe=-1;
    for(int i=0;i<a.numMonstros;i++) if(a.monstros[i].vivo && (tipoDe(a.monstros[i]).especial & CHEFE)) chefe=i;
    for(int y=0;y<MAPA_ALTURA;y++) for(int x=0;x<MAPA_LARGURA;x++) tapete[y][x]=false;
    tapeteDaSemente=jogo.semente; tapetePronto=true;
    if(chefe<0) return;
    // Volta do chefe até a chegada seguindo o mapa de distâncias.
    static int d[MAPA_ALTURA][MAPA_LARGURA];
    calcularDistancias(a.mapa,a.escadas.chegadaSobeX,a.escadas.chegadaSobeY,d);
    int x=a.monstros[chefe].x, y=a.monstros[chefe].y;
    for(int passos=0;passos<400 && d[y][x]>0;passos++) {
        tapete[y][x]=true;
        // Prefere andar em linha reta; na diagonal, o tapete também cobre a quina.
        int melhor=-1;
        for(int k=0;k<8 && melhor<0;k++) {
            int nx=x+DIRECOES_DX[k], ny=y+DIRECOES_DY[k];
            if(dentroDoMapa(nx,ny) && d[ny][nx]!=INALCANCAVEL && d[ny][nx]<d[y][x]) melhor=k;
        }
        if(melhor<0) break;
        if(melhor>=4) {
            if(a.mapa.celulas[y][x+DIRECOES_DX[melhor]]==CHAO) tapete[y][x+DIRECOES_DX[melhor]]=true;
            else if(a.mapa.celulas[y+DIRECOES_DY[melhor]][x]==CHAO) tapete[y+DIRECOES_DY[melhor]][x]=true;
        }
        x+=DIRECOES_DX[melhor]; y+=DIRECOES_DY[melhor];
    }
    tapete[y][x]=true;
}
// Parede encostada na lojinha (para desenhar as prateleiras).
bool encostaNaLoja(const Andar& a,int x,int y) {
    if(!a.loja.existe) return false;
    for(int d=0;d<8;d++) {
        int nx=x+DIRECOES_DX[d], ny=y+DIRECOES_DY[d];
        if(dentroDoMapa(nx,ny) && a.zonaSegura[ny][nx] && a.mapa.celulas[ny][nx]==CHAO) return true;
    }
    return false;
}
void tile(const Jogo& jogo,const Andar& a,int x,int y,int px,int py) {
    if(!dentroDoMapa(x,y) || !a.descoberto[y][x]) {
        DrawRectangle(px,py,TAM,TAM,{8,12,24,255});
        return;
    }
    bool agora=jogo.visivel[y][x];
    float claridade=agora?std::clamp(brilho(jogo.luz[y][x]),0.27f,1.0f):0.29f;
    auto tipo=a.mapa.celulas[y][x];
    int tema=temaDoAndar(jogo.andar);
    // Cores do chão e da parede de cada tema: minas, caverna, vulcão, trono.
    const Color CHAO_DO_TEMA[NUM_TEMAS]={{72,64,60,255},{52,70,74,255},{56,63,64,255},{66,50,46,255},{70,74,92,255}};
    const Color PAREDE_DO_TEMA[NUM_TEMAS]={{121,102,90,255},{78,108,116,255},{90,96,104,255},{92,58,52,255},{118,124,150,255}};
    Color chao=agora?CHAO_DO_TEMA[tema]:Color{24,33,55,255};
    Color parede=agora?PAREDE_DO_TEMA[tema]:Color{40,49,70,255};
    Color cor= (tipo==PAREDE || tipo==TOCHA) ? parede : chao;
    bool cripta=(tema==TEMA_CRIPTA);
    if(agora)cor=iluminarComCor(cor,jogo.luz[y][x],claridade);
    DrawRectangle(px,py,TAM,TAM,cor);
    if(tipo==PAREDE || tipo==TOCHA) {
        // Ladrilhos retangulares e linhas de rejunte.
        DrawLine(px,py+TAM/2,px+TAM,py+TAM/2,multiplicar({31,37,48,255},claridade));
        DrawLine(px+TAM/2,py,px+TAM/2,py+TAM/2,multiplicar({31,37,48,255},claridade));
        DrawLine(px+TAM/4,py+TAM/2,px+TAM/4,py+TAM,multiplicar({31,37,48,255},claridade));
        // Vulcão: algumas paredes têm veios de lava brilhando.
        if(tema==TEMA_VULCAO && agora && (x*13+y*7)%5==0) {
            float pulso=0.7f+0.3f*std::sin((float)GetTime()*2+x+y);
            DrawLine(px+3,py+4,px+9,py+10,multiplicar({255,120,40,255},pulso));
            DrawLine(px+9,py+10,px+8,py+18,multiplicar({255,120,40,255},pulso));
        }
        // Cripta: nichos do ossuário com um crânio dentro.
        if(cripta && tipo==PAREDE && (x*7+y*11)%6==0 && ehPassavel(a.mapa,x,y+1)) {
            DrawRectangle(px+5,py+7,14,13,multiplicar({22,24,30,255},claridade));
            DrawRectangle(px+5,py+6,14,2,multiplicar({120,126,134,255},claridade));
            Color osso=multiplicar({214,206,182,255},agora?claridade:0.4f);
            DrawCircle(px+12,py+13,4.5f,osso);
            DrawRectangle(px+10,py+16,5,3,osso);
            DrawRectangle(px+9,py+12,2,2,{20,20,26,255}); DrawRectangle(px+13,py+12,2,2,{20,20,26,255});
        }
        // Caverna: musgo verde-azulado em algumas paredes.
        if(tema==TEMA_CAVERNA && agora && (x*11+y*5)%4==0)
            DrawRectangle(px+2,py+TAM-5,TAM-6,3,multiplicar({70,140,120,255},claridade));
    } else if(tipo==POCA || tipo==LAVA) {
        // Poça (de água ou de lava): só uma parte da casa, com borda irregular.
        // Se a vizinha é do mesmo tipo, as duas se emendam numa poça só.
        bool lava=(tipo==LAVA);
        float t=(float)GetTime();
        // A lava brilha sozinha (mesmo no escuro) e pulsa devagar.
        float pulso=0.8f+0.2f*std::sin(t*2.5f+x*0.9f+y*1.4f);
        float f = lava ? (agora ? pulso : 0.45f) : (agora ? claridade : 0.4f);
        DrawRectangleLines(px,py,TAM,TAM,multiplicar({39,43,56,255},agora?claridade:0.4f));
        Color liquido=multiplicar(lava?Color{205,62,22,255}:Color{34,92,140,255},f);
        int jx=(x*7+y*3)%3-1, jy=(x*3+y*5)%3-1;   // cada poça um pouco torta
        float cxp=px+12.0f+jx, cyp=py+12.0f+jy;
        if(lava && agora) {
            Color quente={255,120,40,110}; Color fora=quente; fora.a=0;
            DrawCircleGradient((int)cxp,(int)cyp,17,quente,fora);   // calor em volta
        }
        auto igual=[&](int dx,int dy){ return dentroDoMapa(x+dx,y+dy) && a.mapa.celulas[y+dy][x+dx]==tipo; };
        bool l=igual(1,0), o=igual(-1,0), s=igual(0,1), n=igual(0,-1);
        // Raio diferente em cada casa: a borda da poça fica irregular.
        float r=7.5f+(float)((x*13+y*7)%4);
        int ri=(int)r;
        DrawCircleV({cxp,cyp},r,liquido);
        if(l) DrawRectangle((int)cxp,(int)cyp-ri,px+TAM-(int)cxp,2*ri+1,liquido);
        if(o) DrawRectangle(px,(int)cyp-ri,(int)cxp-px,2*ri+1,liquido);
        if(s) DrawRectangle((int)cxp-ri,(int)cyp,2*ri+1,py+TAM-(int)cyp,liquido);
        if(n) DrawRectangle((int)cxp-ri,py,2*ri+1,(int)cyp-py,liquido);
        // Cantos: com poça dos dois lados e na diagonal, ela fica cheia ali.
        if(l&&s&&igual(1,1)) DrawRectangle((int)cxp,(int)cyp,px+TAM-(int)cxp,py+TAM-(int)cyp,liquido);
        if(o&&s&&igual(-1,1)) DrawRectangle(px,(int)cyp,(int)cxp-px,py+TAM-(int)cyp,liquido);
        if(l&&n&&igual(1,-1)) DrawRectangle((int)cxp,py,px+TAM-(int)cxp,(int)cyp-py,liquido);
        if(o&&n&&igual(-1,-1)) DrawRectangle(px,py,(int)cxp-px,(int)cyp-py,liquido);
        if(lava) {
            // Bolhas amarelas que aparecem e somem.
            for(int k=0;k<2;k++) {
                float fase=std::fmod(t*0.7f+(x*0.37f+y*0.53f)+k*0.5f,1.0f);
                float raio=1.5f+2.5f*std::sin(fase*3.14159f);
                DrawCircleV({cxp-4+k*7.0f,cyp-2+k*4.0f},raio,multiplicar({255,200,80,255},f));
            }
        } else {
            // Ondinhas que se mexem.
            for(int k=0;k<2;k++) {
                float onda=std::sin(t*2.2f+x*1.3f+y*0.7f+k*2.1f);
                int oy=(int)cyp-3+k*6+(int)(onda*1.2f);
                DrawLine((int)cxp-5+k*2,oy,(int)cxp+3+k*2,oy,multiplicar({120,190,240,255},f*(0.7f+0.3f*onda)));
            }
        }
    } else if(cripta && (tipo==CHAO || tipo==TUMULO) && !a.zonaSegura[y][x]) {
        // Cripta: lajes grandes (2x2 casas) com rejunte, rachaduras e ossos.
        Color rejunte=multiplicar({34,37,42,255},claridade);
        if(x%2==0) DrawLine(px,py,px,py+TAM,rejunte);
        if(y%2==0) DrawLine(px,py,px+TAM,py,rejunte);
        if((x*31+y*17)%9==0) {   // rachadura
            Color r=multiplicar({30,32,36,255},claridade);
            DrawLine(px+4,py+6,px+10,py+11,r); DrawLine(px+10,py+11,px+9,py+18,r); DrawLine(px+10,py+11,px+16,py+13,r);
        }
        if((x*13+y*29)%23==0 && tipo==CHAO) {   // osso largado
            Color o=multiplicar({200,192,170,255},agora?claridade:0.4f);
            DrawLine(px+7,py+15,px+16,py+10,o); DrawLine(px+7,py+16,px+16,py+11,o);
            DrawCircle(px+7,py+14,1.6f,o); DrawCircle(px+7,py+17,1.6f,o);
            DrawCircle(px+16,py+9,1.6f,o); DrawCircle(px+17,py+12,1.6f,o);
        } else if((x*7+y*41)%37==0 && tipo==CHAO) {   // crânio pequeno
            Color o=multiplicar({205,198,176,255},agora?claridade:0.4f);
            DrawCircle(px+12,py+12,4,o); DrawRectangle(px+10,py+15,5,2,o);
            DrawRectangle(px+10,py+11,2,2,{25,25,30,255}); DrawRectangle(px+13,py+11,2,2,{25,25,30,255});
        }
        if(tipo==TUMULO && !a.enfeites[y][x]) {
            // Lápide: pedra com o topo redondo (ou em cruz), musgo e sombra.
            float f=agora?claridade:0.42f;
            Color pedra=multiplicar({128,134,140,255},f), escura=multiplicar({78,82,90,255},f);
            DrawEllipse(px+12,py+21,9,2.5f,{10,12,16,120});
            if((x+y*3)%3==0) {
                DrawRectangle(px+10,py+3,4,18,pedra); DrawRectangle(px+5,py+7,14,4,pedra);
                DrawRectangleLines(px+10,py+3,4,18,escura);
                DrawRectangle(px+8,py+19,8,3,escura);
            } else {
                DrawCircle(px+12,py+9,6,pedra); DrawRectangle(px+6,py+9,12,12,pedra);
                DrawRectangle(px+5,py+19,14,3,escura);
                DrawLine(px+9,py+11,px+15,py+11,escura); DrawLine(px+9,py+14,px+15,py+14,escura); DrawLine(px+9,py+17,px+13,py+17,escura);
                if((x*5+y)%2==0) DrawRectangle(px+6,py+16,4,4,multiplicar({70,110,80,255},f));   // musgo
                if((x*3+y*7)%4==0) DrawLine(px+15,py+5,px+13,py+10,escura);                       // trinca
            }
        }
        // Teia no canto da sala (parede em cima e à esquerda).
        if(tipo==CHAO && a.mapa.celulas[y-1][x]==PAREDE && a.mapa.celulas[y][x-1]==PAREDE && (x+y)%2==0) {
            Color w=multiplicar({190,196,204,255},agora?claridade:0.4f); w.a=150;
            DrawLine(px,py,px+14,py+14,w); DrawLine(px,py,px+16,py+5,w); DrawLine(px,py,px+5,py+16,w);
            DrawLine(px+5,py+2,px+2,py+5,w); DrawLine(px+10,py+3,px+3,py+10,w); DrawLine(px+14,py+5,px+5,py+14,w);
        }
        // Névoa rasteira: manchas claras e macias que andam devagar pelo chão.
        float t=(float)GetTime();
        for(int sy=0;sy<2;sy++) for(int sx=0;sx<2;sx++) {
            float gx=(x*2+sx)*0.21f, gy=(y*2+sy)*0.29f;
            float n=std::sin(gx+t*0.35f)*std::cos(gy-t*0.22f)+0.6f*std::sin(gx*0.7f-gy*0.9f+t*0.5f);
            float alfa=std::clamp((n-0.2f)*60.0f,0.0f,62.0f)*(agora?1.0f:0.45f);
            if(alfa>1) { Color c={185,205,210,(unsigned char)alfa}, f=c; f.a=0; DrawCircleGradient(px+sx*12+6,py+sy*12+6,11,c,f); }
        }
    } else if(tema==TEMA_TRONO && tipo==CHAO && agora) {
        // Sala do trono: chão polido em xadrez, com o tapete até o chefe.
        if((x+y)%2==0) DrawRectangle(px,py,TAM,TAM,multiplicar({86,90,112,255},claridade));
        DrawRectangleLines(px,py,TAM,TAM,multiplicar({50,54,72,255},claridade));
        if(noTapete(a,x,y)) {
            DrawRectangle(px+3,py,TAM-6,TAM,multiplicar({140,30,45,255},claridade));
            DrawLine(px+4,py,px+4,py+TAM,multiplicar({214,170,80,255},claridade));
            DrawLine(px+TAM-5,py,px+TAM-5,py+TAM,multiplicar({214,170,80,255},claridade));
        }
    } else if(a.zonaSegura[y][x] && tipo==CHAO) {
        // Lojinha do comerciante: tapete vinho com barra dourada.
        float f = agora ? claridade : 0.45f;
        DrawRectangle(px,py,TAM,TAM,multiplicar({118,34,46,255},f));
        DrawRectangleLines(px+2,py+2,TAM-4,TAM-4,multiplicar({214,170,80,255},f));
        if((x+y)%2==0) DrawPoly({(float)px+12,(float)py+12},4,3,45,multiplicar({214,170,80,255},f));
    } else {
        DrawRectangleLines(px,py,TAM,TAM,multiplicar({39,43,56,255},claridade));
        if((x*31+y*17)%7==0) DrawLine(px+7,py+13,px+11,py+17,multiplicar({108,101,94,255},claridade));
        // Caverna (v1.23): cogumelos de enfeite. O fungo disfarçado é igualzinho.
        if(tema==TEMA_CAVERNA && tipo==CHAO && (x*17+y*31)%19==0)
            desenharSprite(FUNGO_QUIETO,px+2,py+1,1.25f,agora?claridade:0.35f);
    }
    if(tipo==PORTAO) {
        // v1.24: portão de grade. Aberto, só as pontas aparecem lá no alto;
        // fechado (cena e luta), as barras descem até o chão.
        float f=agora?1.0f:0.5f;
        float desce=jogo.portaoFechado?portaoDescendo:0.0f;
        int alt=(int)(4+20*desce);
        Color ferro=multiplicar({110,112,124,255},f*std::max(claridade,0.6f)), escuro=multiplicar({40,42,50,255},f);
        DrawRectangle(px,py,TAM,3,escuro);
        for(int k=0;k<4;k++) DrawRectangle(px+2+k*6,py,3,alt,ferro);
        DrawRectangle(px,py+alt/2,TAM,2,ferro);
        for(int k=0;k<4;k++) DrawTriangle({(float)px+2+k*6,(float)py+alt},{(float)px+3.5f+k*6,(float)py+alt+3},{(float)px+5+k*6,(float)py+alt},ferro);
    }
    if(tipo==TEIA) {
        // Teia de aranha no chão (v1.23): raios e anéis brancos.
        Color w=multiplicar({225,230,240,255},agora?claridade:0.4f); w.a=200;
        float cx=px+12.0f, cy=py+12.0f;
        for(int k=0;k<8;k++) { float an=k*0.785f; DrawLineEx({cx,cy},{cx+std::cos(an)*12,cy+std::sin(an)*12},1.2f,w); }
        DrawCircleLines((int)cx,(int)cy,4,w); DrawCircleLines((int)cx,(int)cy,8,w); DrawCircleLines((int)cx,(int)cy,11.5f,w);
    }
    if(tipo==PAREDE && encostaNaLoja(a,x,y)) {
        // Prateleira de mercadorias na parede da lojinha: frascos coloridos.
        float f = agora ? claridade : 0.45f;
        DrawRectangle(px+2,py+8,TAM-4,3,multiplicar({140,95,50,255},f));
        DrawRectangle(px+2,py+18,TAM-4,3,multiplicar({140,95,50,255},f));
        Color frascos[]={{230,80,95,255},{90,160,255,255},{120,220,120,255},{255,205,70,255}};
        for(int k=0;k<3;k++) {
            Color c=multiplicar(frascos[(x*3+y+k)%4],f);
            DrawRectangle(px+4+k*6,py+3,4,5,c);
            DrawRectangle(px+4+k*6,py+13,4,5,multiplicar(frascos[(x+y*5+k)%4],f));
        }
    }
    if (tipo==PORTA || tipo==PORTA_TRANCADA) {
        DrawRectangle(px+5,py+3,14,18,agora ? Color{122,79,39,255}:Color{60,52,49,255});
        DrawRectangleLines(px+5,py+3,14,18,{206,161,87,255});
        DrawCircle(px+16,py+12,2,tipo==PORTA_TRANCADA?OURO:CIANO);
    } else if(tipo==ESCADA_DESCE) {
        // v1.25: enquanto o mini-chefe do andar está vivo, a escada fica
        // lacrada (grades vermelhas e um cadeado). Quando ele cai, as grades
        // sobem e somem em menos de um segundo.
        static int andarDoLacre=-1; static bool lacreAntes=false; static double lacreSumiuEm=-10;
        bool lacrada=miniChefeQueLacraAEscada(andarAtual(jogo))>=0;
        if(jogo.andar!=andarDoLacre) { andarDoLacre=jogo.andar; lacreAntes=lacrada; lacreSumiuEm=-10; }
        else if(lacreAntes && !lacrada) { lacreSumiuEm=GetTime(); lacreAntes=false; }
        else lacreAntes=lacrada;
        float sumindo=(float)((GetTime()-lacreSumiuEm)/0.9);   // 0 a 1 enquanto as grades sobem
        // v1.23.1: a escada para DESCER (o caminho) é um buraco escuro no chão,
        // com degraus sumindo para baixo, borda dourada e uma luz quente que pulsa.
        float f=agora?1.0f:0.5f, t=(float)GetTime();
        float pulso=agora?0.65f+0.35f*std::sin(t*3.2f):0.5f;
        if(lacrada) { f*=0.55f; pulso=0.45f; }   // lacrada: apagada, sem a luz quente
        if(agora && !lacrada) DrawCircleGradient(px+12,py+12,17,{255,190,80,(unsigned char)(70*pulso)},{255,190,80,0});
        DrawRectangle(px+2,py+3,20,19,{10,8,14,255});
        for(int i=0;i<4;i++) {
            float fundo=1.0f-i*0.22f;   // quanto mais fundo, mais escuro e mais estreito
            DrawRectangle(px+3+i*2,py+4+i*4,18-i*4,3,multiplicar({176,140,82,255},f*fundo));
            DrawRectangle(px+3+i*2,py+7+i*4,18-i*4,1,multiplicar({60,44,28,255},f*fundo));
        }
        DrawRectangleLinesEx({(float)px+2,(float)py+3,20,19},1.5f,multiplicar(OURO,f*pulso));
        // Seta para baixo, dentro do buraco, que "respira" (desce e volta).
        float b=agora?std::sin(t*3.2f)*1.2f:0;
        Color seta=multiplicar(OURO,agora?0.8f+0.2f*pulso:0.5f);
        DrawTriangle({(float)px+8,py+11+b},{(float)px+12,py+17+b},{(float)px+16,py+11+b},seta);
        if(lacrada || (sumindo>=0 && sumindo<1)) {
            float sobe=lacrada?0:sumindo*14, alfa=lacrada?1.0f:1.0f-sumindo;
            float brilho=agora?0.75f+0.25f*std::sin(t*2.4f):0.6f;
            if(agora && lacrada) DrawCircleGradient(px+12,py+12,15,{200,40,40,(unsigned char)(55*brilho)},{200,40,40,0});
            Color ferro=Fade(multiplicar({120,52,48,255},agora?brilho:0.6f),alfa);
            for(int k=0;k<4;k++) DrawRectangle(px+4+k*5,(int)(py+3-sobe),2,19,ferro);
            DrawRectangle(px+2,(int)(py+8-sobe),20,2,ferro); DrawRectangle(px+2,(int)(py+16-sobe),20,2,ferro);
            DrawRectangle(px+9,(int)(py+10-sobe),7,6,Fade(multiplicar({170,140,80,255},agora?1.0f:0.6f),alfa));   // cadeado
            DrawRectangle(px+11,(int)(py+12-sobe),3,2,Fade({40,30,20,255},alfa));
        }
    } else if(tipo==ESCADA_SOBE) {
        // A escada para SUBIR (por onde se chegou): degraus de pedra que sobem
        // para a luz, em tons frios e apagados. No andar 1, grades: está selada.
        float f=agora?1.0f:0.5f;
        DrawRectangle(px+2,py+2,20,20,multiplicar({34,40,52,255},f));
        if(agora) DrawRectangleGradientV(px+3,py+2,18,10,{200,220,255,90},{200,220,255,0});
        for(int i=0;i<4;i++) {
            float luz=0.55f+i*0.15f;   // quanto mais alto, mais claro
            DrawRectangle(px+3,py+18-i*4,18,3,multiplicar({150,162,180,255},f*luz));
            DrawRectangle(px+3,py+21-i*4,18,1,multiplicar({40,46,58,255},f));
        }
        DrawRectangleLinesEx({(float)px+2,(float)py+2,20,20},1.0f,multiplicar({120,135,160,255},f));
        if(jogo.andar==1) {
            Color ferro=multiplicar({70,72,80,255},agora?1.0f:0.6f);
            for(int k=0;k<4;k++) DrawRectangle(px+4+k*5,py+2,2,20,ferro);
            DrawRectangle(px+2,py+7,20,2,ferro); DrawRectangle(px+2,py+16,20,2,ferro);
            DrawRectangle(px+10,py+10,5,5,multiplicar({150,130,90,255},f));   // cadeado
        } else {
            // Seta para cima, pequena e apagada (não é por ali que se avança).
            Color seta=multiplicar({70,86,110,255},agora?1.0f:0.6f);
            DrawTriangle({(float)px+12,py+5.0f},{(float)px+8,py+10.0f},{(float)px+16,py+10.0f},seta);
        }
    } else if(tipo==TOCHA && agora) {
        // A chama tremula: o tamanho muda com duas ondas de ritmos diferentes.
        float t=(float)GetTime();
        float tremula=std::sin(t*11.0f+x*1.7f)*1.5f+std::sin(t*17.0f+y*2.3f)*1.0f;
        // A chama tem a cor do tema (azul na sala do trono, vermelha no vulcão).
        const Color BRILHO[NUM_TEMAS]={{255,173,48,0},{150,220,255,0},{140,255,190,0},{255,90,40,0},{110,150,255,0}};
        const Color CHAMA[NUM_TEMAS]={{255,211,96,255},{200,240,255,255},{190,255,215,255},{255,150,80,255},{170,200,255,255}};
        Color b=BRILHO[tema]; Color fora=b; b.a=(unsigned char)(140+tremula*10);
        if(tema==TEMA_CRIPTA) {
            // Velas derretidas numa prateleira, com chama verde-clara.
            DrawRectangle(px+3,py+17,18,3,{70,66,62,255});
            DrawCircleGradient(px+12,py+10,12+tremula,b,fora);
            int alt[3]={7,10,5};
            for(int k=0;k<3;k++) {
                int vx=px+5+k*6, h=alt[(x+y+k)%3];
                DrawRectangle(vx,py+17-h,4,h,{226,220,196,255});
                DrawRectangle(vx-1,py+16,6,2,{210,204,180,255});
                float tk=std::sin((float)GetTime()*13.0f+k*2.1f+x)*0.8f;
                DrawCircle(vx+2,(int)(py+14-h+tk*0.3f),2.0f+tk*0.2f,CHAMA[tema]);
            }
        } else {
            DrawRectangle(px+10,py+13,4,9,{105,75,50,255});
            DrawCircleGradient(px+12,py+8,11+tremula,b,fora);
            DrawCircle(px+12,(int)(py+8-tremula*0.4f),4+tremula*0.3f,CHAMA[tema]);
        }
    } else if(tipo==CAIXA) {
        // Caixa de madeira (dá para empurrar).
        float f = agora ? claridade : 0.45f;
        DrawRectangle(px+3,py+3,18,18,multiplicar({150,100,52,255},f));
        DrawRectangleLines(px+3,py+3,18,18,multiplicar({222,170,95,255},f));
        DrawLine(px+4,py+4,px+20,py+20,multiplicar({96,62,30,255},f));
        DrawLine(px+20,py+4,px+4,py+20,multiplicar({96,62,30,255},f));
    }
}
// Segunda camada da casa: armadilhas, salas especiais, NPCs e itens. Fica
// separada do chão para o "ambiente" das salas especiais caber no meio.
void tileObjetos(const Jogo& jogo,const Andar& a,int x,int y,int px,int py) {
    if(!dentroDoMapa(x,y) || !a.descoberto[y][x]) return;
    bool agora=jogo.visivel[y][x];
    float claridade=agora?std::clamp(brilho(jogo.luz[y][x]),0.27f,1.0f):0.29f;
    // Armadilha já descoberta: espinhos vermelhos.
    int armadilha = armadilhaEm(a,x,y);
    if (armadilha>=0 && a.armadilhas[armadilha].descoberta) {
        Color c = agora ? Color{235,95,95,255} : Color{110,60,70,255};
        for (int i=0;i<3;i++) {
            float bx=(float)px+4+i*6;
            DrawTriangle({bx,(float)py+19},{bx+6,(float)py+19},{bx+3,(float)py+7},c);
        }
    }
    // Fonte, altar, baú dourado e gongo da arena (salas.cpp).
    int esp = especialEm(a,x,y);
    if(esp>=0 && (agora || a.descoberto[y][x])) {
        const Especial& e=a.especiais[esp];
        float t=(float)GetTime();
        if(agora && !e.usado) {
            // Um brilho chama a atenção: azul na fonte, vermelho no altar,
            // dourado no baú; no gongo, vermelho pulsando durante o desafio.
            Color b = e.tipo==ESP_FONTE ? Color{120,200,255,0} : e.tipo==ESP_ALTAR ? Color{255,60,70,0}
                    : e.tipo==ESP_BAU ? Color{255,215,90,0} : Color{255,170,80,0};
            if(e.tipo==ESP_GONGO && e.onda>0) b={255,50,50,0};
            Color dentro=b; dentro.a=(unsigned char)(60+40*std::sin(t*3+x));
            DrawCircleGradient(px+12,py+12,15,dentro,b);
        }
        const Sprite* arte = e.tipo==ESP_FONTE ? (e.usado?&FONTE_SECA:&FONTE)
                           : e.tipo==ESP_ALTAR ? (e.usado?&ALTAR_USADO:&ALTAR)
                           : e.tipo==ESP_BAU   ? (e.usado?&BAU_ABERTO:&BAU_DOURADO)
                           : (e.usado?&GONGO_RACHADO:&GONGO);
        if(agora) desenharSprite(*arte,px+2,py+1,1.25f,claridade);
        else icone(arte->data(),px+2,py+1,1.25f,0.35f,0.8f);
    }
    if (agora) {
        int npc = npcEm(a,x,y);
        if(npc>=0) {
            desenharSprite(imagemNpc(a.npcs[npc].tipo),px+2,py+1,1.25f,1.0f);
            float pulo=std::sin((float)GetTime()*5)*2;
            if(!a.npcs[npc].conversou) textoCentro("!",px+20,py-6+pulo,16,OURO);
            else if(a.npcs[npc].tipo==NPC_COMERCIANTE) textoCentro("$",px+20,py-6+pulo,14,OURO);   // a loja continua aberta
        } else {
            int item=itemEm(a,x,y);
            // Itens no chão flutuam de leve.
            py += (int)std::round(std::sin((float)GetTime()*3.0f+x*0.9f+y*1.3f)*1.5f);
            if(item>=0) {
                // Desenho próprio de cada item (itens_sprites.h), com um brilho embaixo.
                int tipoDoItem=a.itens[item].tipo;
                bool raro=TIPOS_DE_ITEM[tipoDoItem].categoria==CRISTAL || tipoDoItem==I_CHAVE;
                if(raro) DrawCircleGradient(px+12,py+12,12,{255,230,140,(unsigned char)(70+40*std::sin((float)GetTime()*4))},{255,230,140,0});
                desenharSprite(*ARTE_DOS_ITENS[tipoDoItem],px+2,py+1,1.25f,claridade);
            }
        }
    } else {
        // Item já visto, fora da visão: aparece apagado (memória do mapa).
        int item=itemEm(a,x,y);
        if(item>=0 && a.itens[item].visto) icone(ARTE_DOS_ITENS[a.itens[item].tipo]->data(),px+2,py+2,1.25f,0.35f,0.8f);
    }
}
// Monstro no mapa (desenhado fora do tile para poder deslizar entre células).
// Aura dourada pulsando e uma coroinha: o monstro é um campeão (elite).
void auraDeCampeao(float cx,float cy,float raio,float t) {
    Color ouro={255,205,70,(unsigned char)(80+50*std::sin(t*4))}; Color fora=ouro; fora.a=0;
    DrawCircleGradient((int)cx,(int)cy,raio,ouro,fora);
}
// Triângulo em qualquer ordem (a raylib só desenha os de uma das voltas).
void trianguloCheio(Vector2 a,Vector2 b,Vector2 c,Color cor) {
    float giro=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
    if(giro>0) DrawTriangle(a,c,b,cor); else DrawTriangle(a,b,c,cor);
}
// Coroa: base em (x, y), com três pontas para cima.
void coroinha(float x,float y,float escala) {
    Color ouro={255,210,70,255}, escuro={120,80,10,255};
    float e=escala;
    trianguloCheio({x-5*e,y},{x-5*e,y-5*e},{x-2*e,y},ouro);
    trianguloCheio({x-2.5f*e,y},{x,y-6*e},{x+2.5f*e,y},ouro);
    trianguloCheio({x+2*e,y},{x+5*e,y-5*e},{x+5*e,y},ouro);
    DrawRectangleRec({x-5*e,y-e,10*e,2.5f*e},ouro);
    DrawRectangleLinesEx({x-5*e,y-e,10*e,2.5f*e},std::max(1.0f,e*0.4f),escuro);
    DrawCircleV({x,y-6*e},0.9f*e,{255,90,90,255});   // pedrinha na ponta do meio
}
void desenharMonstroNoMapa(const Jogo& jogo,const Monstro& m,float px,float py) {
    float claridade=std::clamp(brilho(jogo.luz[m.y][m.x]),0.27f,1.0f);
    float t=(float)GetTime();
    if(m.campeao) auraDeCampeao(px+12,py+12,17,t);
    // A sombra é meio transparente e tremula (some de vez se não estiver à vista).
    float alfa=(tipoDe(m).especial & SOMBRIO) ? 0.55f+0.2f*std::sin(t*3+m.x) : 1.0f;
    // O fungo parado parece só mais um cogumelo do cenário.
    bool disfarcado=(tipoDe(m).especial & DISFARCADO) && m.estado==DORMINDO;
    desenharSprite(disfarcado?FUNGO_QUIETO:imagemMonstro(m.tipo),px+2,py+1,1.25f,claridade,alfa);
    if(m.campeao) coroinha(px+12,py+1,1.3f);
    // Gnomo que parou de fugir: "!" vermelho em cima (vai lutar).
    if((tipoDe(m).especial & VIGIA) && m.cornetas>=2) textoCentro("!",px+20,py-8+std::sin(t*6)*1.5f,15,PERIGO);
    if(m.atordoadoMs>0) {
        // Atordoado: três estrelinhas girando sobre a cabeça.
        float giro=(float)GetTime()*5.5f;
        for(int k=0;k<3;k++) {
            float a=giro+k*2.094f;
            DrawPoly({px+12+std::cos(a)*8,py+2+std::sin(a)*3},4,3.2f,45,{255,226,90,255});
        }
    }
}
// Tab segurado: o painel completo de antes (todas as informações).
void painelCompleto(const Jogo& jogo);   // v1.25: mais abaixo, depois dos cartões
// ---- Cartões da tela de exploração (v1.18) ---------------------------------
// O mapa ocupa a tela toda; por cima ficam só cartões pequenos nos cantos.
const Color FUNDO_DO_CARTAO={10,15,28,215};
const Color BORDA_DO_CARTAO={70,95,125,210};
void cartao(Rectangle r) {
    DrawRectangleRounded(r,0.22f,8,FUNDO_DO_CARTAO);
    DrawRectangleRoundedLinesEx(r,0.22f,8,1.6f,BORDA_DO_CARTAO);
}
void coracao(float x,float y,float t,Color cor) {
    DrawCircleV({x-t*0.5f,y},t*0.55f,cor);
    DrawCircleV({x+t*0.5f,y},t*0.55f,cor);
    trianguloCheio({x-t*1.05f,y+t*0.15f},{x+t*1.05f,y+t*0.15f},{x,y+t*1.25f},cor);
}
// Monstro à vista mais perto do jogador (ou nullptr).
const Monstro* monstroMaisPerto(const Jogo& jogo) {
    const Andar& a=andarAtual(jogo);
    const Monstro* melhor=nullptr; int menor=1<<30;
    for(int i=0;i<a.numMonstros;i++) {
        const Monstro& m=a.monstros[i];
        if(!monstroAVista(jogo,m)) continue;
        if((tipoDe(m).especial & DISFARCADO) && m.estado==DORMINDO) continue;   // o fungo parado não se entrega
        int dx=m.x-jogo.jogador.x, dy=m.y-jogo.jogador.y, d=dx*dx+dy*dy;
        if(d<menor) { menor=d; melhor=&m; }
    }
    return melhor;
}
// Mensagens no canto: cada uma aparece por uns segundos e some devagar.
void mensagensQueSomem(const Jogo& jogo) {
    static std::vector<double> chegou;
    double agora=GetTime();
    if(jogo.historico.size()<chegou.size()) chegou.clear();          // partida nova
    while(chegou.size()<jogo.historico.size()) chegou.push_back(agora);
    const double DURA=7.0, SOME=1.5;
    int mostradas=0;
    for(int i=(int)jogo.historico.size()-1;i>=0 && mostradas<3;i--) {
        double idade=agora-chegou[i];
        if(idade>DURA) break;
        float alfa=idade>DURA-SOME ? (float)((DURA-idade)/SOME) : 1.0f;
        float y=Ui::ALTURA-48.0f-mostradas*30;
        Color cor=corDaMensagem(jogo.tiposDoHistorico[i]); cor.a=(unsigned char)(255*alfa);
        textoQueCabe(jogo.historico[i],22,y+2,22,780,{0,0,0,(unsigned char)(200*alfa)});
        textoQueCabe(jogo.historico[i],20,y,22,780,cor);
        mostradas++;
    }
}
// ---------------------------------------------------------------------------
// ÍCONES SIMPLES E O PAINEL DO TAB (v1.25)
// O painel do Tab e a ficha (C) mostram os números com desenhos e barras, em
// vez de linhas de texto.
// ---------------------------------------------------------------------------
namespace Icones {
void relogio(float x,float y,float t,Color c) {
    DrawCircleLines((int)x,(int)y,t,c); DrawCircleLines((int)x,(int)y,t-1,c);
    DrawLineEx({x,y},{x,y-t*0.65f},2,c); DrawLineEx({x,y},{x+t*0.5f,y},2,c);
}
void pegadas(float x,float y,float t,Color c) {
    DrawEllipse((int)(x-t*0.45f),(int)(y+t*0.25f),t*0.32f,t*0.5f,c);
    DrawEllipse((int)(x+t*0.45f),(int)(y-t*0.35f),t*0.32f,t*0.5f,c);
}
void caveira(float x,float y,float t,Color c,Color fundo) {
    DrawCircleV({x,y-t*0.15f},t*0.75f,c);
    DrawRectangleRec({x-t*0.45f,y+t*0.2f,t*0.9f,t*0.55f},c);
    DrawCircleV({x-t*0.3f,y-t*0.15f},t*0.2f,fundo); DrawCircleV({x+t*0.3f,y-t*0.15f},t*0.2f,fundo);
}
void joia(float x,float y,float t,Color c) {
    DrawPoly({x,y},4,t*0.9f,0,c);
    DrawPolyLines({x,y},4,t*0.9f,0,multiplicar(c,1.4f));
}
void bolsa(float x,float y,float t,Color c) {
    DrawRectangleRounded({x-t*0.8f,y-t*0.35f,t*1.6f,t*1.2f},0.35f,6,c);
    DrawRing({x,y-t*0.35f},t*0.3f,t*0.45f,180,360,12,c);
}
void escudo(float x,float y,float t,Color c) {
    DrawRectangleRec({x-t*0.75f,y-t*0.85f,t*1.5f,t*0.9f},c);
    DrawTriangle({x-t*0.75f,y+t*0.05f},{x,y+t*0.95f},{x+t*0.75f,y+t*0.05f},c);
}
void vento(float x,float y,float t,Color c) {
    for(int k=0;k<3;k++) DrawLineEx({x-t*0.9f+k*t*0.25f,y-t*0.5f+k*t*0.5f},{x+t*0.8f-k*t*0.15f,y-t*0.5f+k*t*0.5f},2.2f,c);
}
}

// Um número com ícone e legenda, num quadradinho (painel do Tab e ficha).
void quadroDeNumero(Rectangle r,const std::string& numero,const char* legenda,Color corDoNumero) {
    DrawRectangleRounded(r,0.25f,6,{20,29,48,255});
    texto(numero,r.x+40,r.y+3,21,corDoNumero);
    texto(legenda,r.x+40,r.y+26,12,APAGADO);
}

void painelCompleto(const Jogo& jogo) {
    const Jogador& j=jogo.jogador;
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[j.cor]));
    Rectangle r={1001,122,400,505};
    painel(r);
    float x=r.x+24;
    // Quem é: nome, elemento, dificuldade e o desenho.
    texto(j.nome,x,140,28,OURO);
    float w=etiquetaDeElemento(j.elemento,x+2,178);
    texto(dificuldadeAtual().nome,x+2+w+12,180,18,corDaDificuldade(nivelDaDificuldade()));
    DrawEllipse(1300,226,44,9,{0,0,0,90});
    desenharSprite(HEROI,1258,138,5.2f);
    // Nível: um selo redondo e a barra de experiência.
    float cy=252;
    DrawCircleV({x+26,cy},26,{20,29,48,255});
    DrawCircleLines((int)(x+26),(int)cy,26,CIANO); DrawCircleLines((int)(x+26),(int)cy,25,CIANO);
    textoCentro(std::to_string(j.nivel),x+26,cy-14,26,TEXTO);
    texto("NÍVEL",x+64,cy-24,13,APAGADO);
    int xpNivel=experienciaParaNivel(j.nivel), xpProximo=experienciaParaNivel(j.nivel+1);
    bool noMaximo=j.nivel>=NIVEL_MAXIMO || xpProximo<=xpNivel;
    float fracao=noMaximo?1.0f:std::clamp((float)(j.experiencia-xpNivel)/(xpProximo-xpNivel),0.0f,1.0f);
    Rectangle bx={x+64,cy-4,288,12};
    DrawRectangleRounded(bx,0.5f,4,{28,38,60,255});
    if(fracao>0) DrawRectangleRounded({bx.x,bx.y,std::max(12.0f,bx.width*fracao),bx.height},0.5f,4,{110,175,255,255});
    icone(ARTE_ICONE_XP.data(),x+64,cy+12,0.9f);
    texto(noMaximo?"nível máximo":"faltam "+std::to_string(xpProximo-j.experiencia)+" de experiência",x+82,cy+12,13,APAGADO);
    // Vida: coração e uma barra grossa com o número dentro.
    float vy=300;
    coracao(x+12,vy+8,9,{232,70,90,255});
    barra(x+34,vy,318,22,j.vida,j.vidaMaxima);
    textoCentro(std::to_string(j.vida)+" / "+std::to_string(j.vidaMaxima),x+34+159,vy+1,18,TEXTO);
    if(j.vida>0 && j.vida*4<=j.vidaMaxima) {
        float pulso=0.5f+0.5f*std::sin((float)relogioMs()/150.0f);
        DrawRectangleRoundedLinesEx({x+32,vy-2,322,26},0.35f,8,3.0f,multiplicar(PERIGO,0.4f+0.6f*pulso));
    }
    // Vidas (corações) e pontos.
    float ly=346;
    for(int v=0;v<3 || v<j.vidas;v++) {
        if(v<j.vidas) coracao(x+12+v*28,ly+8,8,{232,70,90,255});
        else coracao(x+12+v*28,ly+8,8,{60,45,60,255});
    }
    texto("vidas",x+12+std::max(3,j.vidas)*28,ly+2,14,APAGADO);
    icone(ARTE_ICONE_PONTOS.data(),x+196,ly,1.3f);
    texto(std::to_string(calcularPontuacao(jogo)),x+224,ly-3,24,OURO);
    texto("pts",x+224+larguraDoTexto(std::to_string(calcularPontuacao(jogo)),24)+6,ly+5,14,APAGADO);
    separador(x,386,352);
    // A partida em quadrinhos: tempo, passos, moedas, mochila, monstros, itens.
    int segundos=(int)(jogo.tempoMs/1000);
    std::string tempo=std::to_string(segundos/60)+":"+(segundos%60<10?"0":"")+std::to_string(segundos%60);
    int naMochila=itensNaMochila(j), limite=limiteDaMochila(j);
    const float QW=170, QH=44, GAP=12;
    Rectangle q[6];
    for(int k=0;k<6;k++) q[k]={x+(k%2)*(QW+GAP),398.0f+(k/2)*(QH+8),QW,QH};
    Color icor={150,165,195,255}, fundo={20,29,48,255};
    quadroDeNumero(q[0],tempo,"tempo",TEXTO);            Icones::relogio(q[0].x+20,q[0].y+22,10,icor);
    quadroDeNumero(q[1],std::to_string(jogo.passos),"passos",TEXTO); Icones::pegadas(q[1].x+20,q[1].y+22,10,icor);
    quadroDeNumero(q[2],std::to_string(j.moedas),"moedas",OURO);   icone(ARTE_ICONE_MOEDA.data(),q[2].x+9,q[2].y+11,1.4f);
    quadroDeNumero(q[3],std::to_string(naMochila)+"/"+std::to_string(limite),"mochila",naMochila>=limite?PERIGO:TEXTO);
    Icones::bolsa(q[3].x+20,q[3].y+24,10,{170,130,80,255});
    DrawRectangleRounded({q[3].x+100,q[3].y+30,60,6},0.5f,4,{28,38,60,255});
    DrawRectangleRounded({q[3].x+100,q[3].y+30,60.0f*std::min(1.0f,(float)naMochila/std::max(1,limite)),6},0.5f,4,naMochila>=limite?PERIGO:Color{170,130,80,255});
    quadroDeNumero(q[4],std::to_string(jogo.monstrosMortos),"monstros vencidos",TEXTO); Icones::caveira(q[4].x+20,q[4].y+22,11,{220,215,200,255},fundo);
    quadroDeNumero(q[5],std::to_string(jogo.itensColetados),"itens pegos",TEXTO);  Icones::joia(q[5].x+20,q[5].y+22,10,{120,200,255,255});
    // Atributos: desenho e valor.
    const char* siglas[NUM_ATRIBUTOS]={"FOR","CON","DES","AGI"};
    for(int a=0;a<NUM_ATRIBUTOS;a++) {
        float ax=x+a*88;
        icone(ARTE_DOS_ATRIBUTOS[a]->data(),ax,556,1.4f);
        texto(std::to_string(j.atributos[a]),ax+27,556,21,TEXTO);
        texto(siglas[a],ax+27+larguraDoTexto(std::to_string(j.atributos[a]),21)+4,562,13,APAGADO);
    }
    if(j.pontosLivres>0) {
        float pulso=0.75f+0.25f*std::sin((float)GetTime()*5);
        icone(ARTE_ICONE_PONTO_LIVRE.data(),x,592,1.3f,pulso);
        std::string pl=std::to_string(j.pontosLivres)+(j.pontosLivres==1?" ponto livre":" pontos livres")+": aperte C";
        texto(pl,x+27,595,18,OURO);
    }
}

// ---------------------------------------------------------------------------
// MAPA DO ANDAR (v1.25)
// O mini-mapa fica sempre no canto, embaixo do cartão do andar. Com o Tab, o
// mapa inteiro aparece grande, com legenda, ao lado do painel de detalhes.
// Só aparece o que o jogador já descobriu; monstros, só os que estão à vista.
// ---------------------------------------------------------------------------
// Ligado enquanto uma tela fica por cima do mapa (diário, mochila, ficha):
// aí o Tab é dessa tela, e não abre o mapa grande no fundo.
bool telaPorCima=false;

namespace MapaDoAndar {
const Color CHAO_={44,54,78,255}, PAREDE_={96,110,140,255}, AGUA={60,120,190,255}, FOGO_={230,110,40,255},
            PORTA_={200,140,60,255}, LOJA={120,40,60,255}, SALA={112,84,40,255}, PORTAO_={190,190,200,255},
            DESCE={255,205,80,255}, LACRADA={225,70,60,255}, SOBE={150,170,205,255}, PESSOA={110,225,215,255},
            ITEM={240,225,140,255}, ESPECIAL={190,120,255,255}, MONSTRO={255,80,80,255}, VOCE={255,255,255,255};

// A cor de uma casa no mapa (false = não desenha: não descoberta ou rocha).
bool corDaCasa(const Jogo& jogo, const Andar& a, int x, int y, Color& c) {
    if(!a.descoberto[y][x]) return false;
    TipoCelula t=a.mapa.celulas[y][x];
    switch(t) {
        case PAREDE: if(!ehVisivelComoParede(a.mapa,x,y)) return false; c=PAREDE_; break;
        case TOCHA:  c={150,130,90,255}; break;
        case POCA:   c=AGUA; break;
        case LAVA:   c=FOGO_; break;
        case PORTA:  c=PORTA_; break;
        case PORTA_TRANCADA: c=DESCE; break;
        case PORTAO: c=jogo.portaoFechado?PORTAO_:multiplicar(PORTAO_,0.6f); break;
        case ESCADA_DESCE: case ESCADA_SOBE: c=CHAO_; break;   // marcadas por cima, maiores
        default:
            c=CHAO_;
            if(a.zonaSegura[y][x]) c=LOJA;
            else if(a.salaDoChefe.existe && dentroDaSalaDoChefe(a,x,y)) c=SALA;
            if(a.enfeites[y][x] && !ehPassavel(a.mapa,x,y)) c=multiplicar(PAREDE_,0.8f);
    }
    return true;
}

// Desenha o andar atual com o canto em (x0,y0) e "cel" pixels por casa.
void desenhar(const Jogo& jogo, float x0, float y0, float cel) {
    const Andar& a=andarAtual(jogo);
    for(int y=0;y<MAPA_ALTURA;y++) for(int x=0;x<MAPA_LARGURA;x++) {
        Color c;
        if(corDaCasa(jogo,a,x,y,c)) DrawRectangleRec({x0+x*cel,y0+y*cel,cel+0.5f,cel+0.5f},c);
    }
    // Marcadores (um pouco maiores que a casa, para se verem no mini-mapa).
    float m=std::max(cel*1.6f,4.0f);
    auto marca=[&](int x,int y,Color c,bool redondo) {
        float cx=x0+(x+0.5f)*cel, cy=y0+(y+0.5f)*cel;
        if(redondo) DrawCircle((int)cx,(int)cy,m*0.5f,c);
        else DrawRectangleRec({cx-m/2,cy-m/2,m,m},c);
    };
    for(int i=0;i<a.numItens;i++) {
        const ItemNoChao& it=a.itens[i];
        if(it.ativo && it.visto && a.descoberto[it.y][it.x]) marca(it.x,it.y,ITEM,true);
    }
    for(int i=0;i<a.numEspeciais;i++) {
        const Especial& e=a.especiais[i];
        if(!e.usado && a.descoberto[e.y][e.x]) marca(e.x,e.y,ESPECIAL,false);
    }
    for(int i=0;i<a.numNpcs;i++) if(a.descoberto[a.npcs[i].y][a.npcs[i].x]) marca(a.npcs[i].x,a.npcs[i].y,PESSOA,true);
    for(int y=0;y<MAPA_ALTURA;y++) for(int x=0;x<MAPA_LARGURA;x++) {
        if(!a.descoberto[y][x]) continue;
        if(a.mapa.celulas[y][x]==ESCADA_DESCE) marca(x,y,miniChefeQueLacraAEscada(a)>=0?LACRADA:DESCE,false);
        if(a.mapa.celulas[y][x]==ESCADA_SOBE) marca(x,y,SOBE,false);
    }
    for(int i=0;i<a.numMonstros;i++) {
        const Monstro& mo=a.monstros[i];
        if(mo.vivo && jogo.visivel[mo.y][mo.x]) marca(mo.x,mo.y,MONSTRO,true);
    }
    // Você: pisca devagar, com um anel em volta.
    float pulso=0.6f+0.4f*std::sin((float)GetTime()*5);
    float cx=x0+(jogo.jogador.x+0.5f)*cel, cy=y0+(jogo.jogador.y+0.5f)*cel;
    DrawCircleLines((int)cx,(int)cy,m*0.95f,multiplicar(CIANO,pulso));
    DrawCircle((int)cx,(int)cy,m*0.55f,VOCE);
}

// O mini-mapa, no meio do cartão do andar (entre o nome do andar e as moedas).
const float CEL_PEQUENO=3.5f;
float alturaDoPequeno() { return MAPA_ALTURA*CEL_PEQUENO; }
void pequeno(const Jogo& jogo, Rectangle cartaoDoAndar, float y) {
    float w=MAPA_LARGURA*CEL_PEQUENO;
    desenhar(jogo,cartaoDoAndar.x+(cartaoDoAndar.width-w)/2,y,CEL_PEQUENO);
}

// O mapa inteiro, grande, com a legenda embaixo.
void grande(const Jogo& jogo, bool fixo) {
    Rectangle r={24,122,962,505};
    painel(r);
    texto("MAPA DO ANDAR "+std::to_string(jogo.andar),r.x+24,r.y+16,26,OURO);
    texto(FICHAS_DOS_TEMAS[temaDoAndar(jogo.andar)].nome,r.x+24,r.y+50,16,APAGADO);
    std::string dica=fixo?"Tab: fechar":"solte o Tab para fechar  |  um toque deixa aberto";
    texto(dica,r.x+r.width-24-larguraDoTexto(dica,15),r.y+22,15,{120,135,160,230});
    const float cel=11.5f, w=MAPA_LARGURA*cel;
    float x0=r.x+(r.width-w)/2, y0=r.y+84;
    DrawRectangleRec({x0-6,y0-6,w+12,MAPA_ALTURA*cel+12},{12,17,30,255});
    desenhar(jogo,x0,y0,cel);
    // Legenda.
    struct Item { Color c; bool redondo; const char* nome; };
    const Item legenda[]={
        {VOCE,true,"você"},{DESCE,false,"escada (descer)"},{LACRADA,false,"escada lacrada"},{SOBE,false,"escada (subir)"},
        {PORTA_,false,"porta"},{LOJA,false,"loja"},{SALA,false,"sala do chefe"},{PESSOA,true,"pessoa"},
        {ITEM,true,"item"},{ESPECIAL,false,"fonte, altar, baú..."},{MONSTRO,true,"monstro à vista"},{AGUA,false,"água"},
    };
    float lx=r.x+40, ly=y0+MAPA_ALTURA*cel+26;
    for(int i=0;i<12;i++) {
        float x=lx+(i%4)*228, y=ly+(i/4)*30;
        if(legenda[i].redondo) DrawCircle((int)x+7,(int)y+9,6,legenda[i].c);
        else DrawRectangle((int)x+1,(int)y+3,12,12,legenda[i].c);
        texto(legenda[i].nome,x+22,y,17,TEXTO);
    }
}
}

void hud(const Jogo& jogo) {
    const Jogador& j=jogo.jogador;
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[j.cor]));
    float t=(float)GetTime();
    // Sombra suave em cima e embaixo, para os cartões e as mensagens se lerem bem.
    DrawRectangleGradientV(0,0,Ui::LARGURA,120,{6,9,18,150},{6,9,18,0});
    DrawRectangleGradientV(0,Ui::ALTURA-150,Ui::LARGURA,150,{6,9,18,0},{6,9,18,170});

    // ---- Herói (canto superior esquerdo) ----
    Rectangle c={16,14,380,92};
    cartao(c);
    bool vidaBaixa=j.vida>0 && j.vida*4<=j.vidaMaxima;
    if(vidaBaixa) {
        float pulso=0.5f+0.5f*std::sin((float)relogioMs()/150.0f);
        DrawRectangleRoundedLinesEx(c,0.22f,8,3.0f,multiplicar(PERIGO,0.4f+0.6f*pulso));
    }
    desenharSprite(HEROI,24,26,3.6f,jogo.furtivo?0.55f:1.0f);
    texto(j.nome,92,22,22,OURO);
    float xn=92+larguraDoTexto(j.nome,22)+10;
    texto("Nv "+std::to_string(j.nivel),xn,25,18,CIANO);
    etiquetaDeElemento(j.elemento,xn+larguraDoTexto("Nv "+std::to_string(j.nivel),18)+10,24,12);
    barra(92,54,210,16,j.vida,j.vidaMaxima);
    texto(std::to_string(j.vida)+"/"+std::to_string(j.vidaMaxima),310,52,17,vidaBaixa?PERIGO:TEXTO);
    // Experiência: barrinha fina embaixo da vida.
    int xpNivel=experienciaParaNivel(j.nivel), xpProximo=experienciaParaNivel(j.nivel+1);
    float fracao=(j.nivel>=NIVEL_MAXIMO || xpProximo<=xpNivel) ? 1.0f
                 : std::clamp((float)(j.experiencia-xpNivel)/(xpProximo-xpNivel),0.0f,1.0f);
    DrawRectangleRounded({92,78,210,6},1,4,{28,38,60,255});
    if(fracao>0) DrawRectangleRounded({92,78,std::max(6.0f,210*fracao),6},1,4,{110,175,255,255});
    // Vidas: corações.
    for(int v=0;v<j.vidas && v<5;v++) coracao(318+v*14,83,5.5f,{232,70,90,255});
    float yChip=112;
    if(j.pontosLivres>0) {
        float pulso=0.7f+0.3f*std::sin(t*5);
        Rectangle chip={16,yChip,250,28};
        cartao(chip);
        icone(ARTE_ICONE_PONTO_LIVRE.data(),24,yChip+6,1.0f,pulso);
        texto("+"+std::to_string(j.pontosLivres)+(j.pontosLivres==1?" ponto livre: aperte C":" pontos livres: aperte C"),
              46,yChip+5,16,multiplicar(OURO,pulso));
        yChip+=34;
    }

    // ---- Monstro mais perto (só quando há um à vista) ----
    const Monstro* m=monstroMaisPerto(jogo);
    if(m) {
        Rectangle cm={16,yChip,380,58};
        cartao(cm);
        if(m->campeao) DrawRectangleRoundedLinesEx(cm,0.22f,8,2.0f,{255,205,70,200});
        desenharSprite(imagemMonstro(m->tipo),24,yChip+6,2.9f);
        std::string nome=nomeDaEspecie(*m); nome[0]=(char)std::toupper((unsigned char)nome[0]);
        texto(nome+"  Nv "+std::to_string(m->nivel),78,yChip+7,18,m->campeao?OURO:TEXTO);
        // v1.23: o elemento só aparece na batalha.
        texto(m->campeao?"campeão: mais forte que o normal":"o elemento só se descobre na batalha",78,yChip+33,14,APAGADO);
    }

    // ---- Andar e moedas (canto superior direito) ----
    // v1.25: o mini-mapa fica no mesmo cartão, embaixo.
    // v1.25: andar e tema em cima, o mini-mapa no meio, moedas e pontos embaixo.
    float hMapa=MapaDoAndar::alturaDoPequeno();
    Rectangle ca={Ui::LARGURA-16.0f-290,14,290,92+hMapa+12};
    cartao(ca);
    texto("ANDAR "+std::to_string(jogo.andar)+" / "+std::to_string(NUM_ANDARES),ca.x+18,ca.y+12,22,CIANO);
    texto("Tab: mapa",ca.x+ca.width-18-larguraDoTexto("Tab: mapa",13),ca.y+17,13,{120,135,160,220});
    texto(FICHAS_DOS_TEMAS[temaDoAndar(jogo.andar)].nome,ca.x+18,ca.y+40,15,APAGADO);
    MapaDoAndar::pequeno(jogo,ca,ca.y+66);
    float yMoedas=ca.y+66+hMapa+10;
    icone(ARTE_ICONE_MOEDA.data(),ca.x+18,yMoedas+2,1.0f);
    texto(std::to_string(j.moedas),ca.x+40,yMoedas,18,OURO);
    icone(ARTE_ICONE_PONTOS.data(),ca.x+120,yMoedas+2,1.0f);
    texto(std::to_string(calcularPontuacao(jogo))+" pts",ca.x+142,yMoedas,18,TEXTO);

    // ---- Mensagens (canto inferior esquerdo) ----
    mensagensQueSomem(jogo);

    // ---- Atalhos (canto inferior direito) ----
    const char* letras[]={"I","C","J","Z","E","?","M"};
    const char* nomes[]={"mochila","ficha","diário","descansar","bestiário","ajuda","mensagens"};
    const int teclas[]={'i','c','j','z','e','?','M'};
    const int NUM_ATALHOS=7;   // v1.26: + diário
    const float PASSO=76;
    float bx=Ui::LARGURA-16.0f-NUM_ATALHOS*PASSO+4, by=Ui::ALTURA-16.0f-56;
    for(int i=0;i<NUM_ATALHOS;i++) {
        Rectangle r={bx+i*PASSO,by,72,56};
        bool sobre=mouseSobre(r);
        DrawRectangleRounded(r,0.25f,6,sobre?Color{40,56,84,235}:FUNDO_DO_CARTAO);
        DrawRectangleRoundedLinesEx(r,0.25f,6,1.4f,sobre?CIANO:BORDA_DO_CARTAO);
        textoCentro(letras[i],r.x+r.width/2,r.y+6,24,sobre?TEXTO:CIANO);
        textoCentro(nomes[i],r.x+r.width/2,r.y+36,12,APAGADO);
        registrarAtalho(r,teclas[i]);
    }
    // Estado do jogador, logo acima dos atalhos.
    std::string estado = jogo.descansando ? "zZz descansando" : jogo.furtivo ? "FURTIVO" : "";
    if(!estado.empty()) textoCentro(estado,bx+NUM_ATALHOS*PASSO/2-2,by-30,18,CIANO);
    texto("Tab: mapa e detalhes   Esc: pausa",bx,by-52+(estado.empty()?24:0),13,{120,135,160,200});

    // ---- Aviso do zoom (some sozinho) ----
    double desdeOZoom=GetTime()-zoomMudouEm;
    if(desdeOZoom<1.4) {
        static const char* NOMES[3]={"LONGE","MÉDIO","PERTO"};
        float alfa=desdeOZoom>1.0 ? (float)((1.4-desdeOZoom)/0.4) : 1.0f;
        std::string s=std::string("Zoom: ")+NOMES[opcoes().zoomDoMapa];
        float w=larguraDoTexto(s,22)+40;
        Rectangle r={(Ui::LARGURA-w)/2,Ui::ALTURA-130.0f,w,40};
        DrawRectangleRounded(r,0.4f,8,{10,15,28,(unsigned char)(220*alfa)});
        textoCentro(s,Ui::LARGURA/2.0f,r.y+9,22,{109,222,209,(unsigned char)(255*alfa)});
    }

    // ---- Tab: o mapa inteiro e o painel completo (v1.25) ----
    // Segurado, aparece enquanto o Tab estiver apertado; um toque rápido deixa
    // aberto até o próximo toque (ou até o jogador andar).
    static bool mapaFixo=false; static double tabDesde=0; static int fixoEmX=-1, fixoEmY=-1;
    if(telaPorCima) mapaFixo=false;
    else if(IsKeyPressed(KEY_TAB)) tabDesde=GetTime();
    if(!telaPorCima && IsKeyReleased(KEY_TAB) && GetTime()-tabDesde<0.3) {
        mapaFixo=!mapaFixo; fixoEmX=jogo.jogador.x; fixoEmY=jogo.jogador.y;
    }
    if(mapaFixo && (jogo.jogador.x!=fixoEmX || jogo.jogador.y!=fixoEmY)) mapaFixo=false;
    if((IsKeyDown(KEY_TAB) && !telaPorCima) || mapaFixo) {
        DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,140});
        MapaDoAndar::grande(jogo,mapaFixo && !IsKeyDown(KEY_TAB));
        painelCompleto(jogo);
    }
}
// Mensagem importante em destaque no topo do mapa por alguns segundos.
void faixaDeDestaque(const Jogo& jogo) {
    long long agora=relogioMs();
    if(jogo.faixa.empty() || agora>=jogo.faixaAte) return;
    float resta=(float)(jogo.faixaAte-agora);
    float alfa=std::min(1.0f,resta/500.0f);              // some devagar no fim
    Color cor=corDaMensagem(jogo.tipoDaFaixa);
    float largura=std::min(900.0f,larguraDoTexto(jogo.faixa,24)+70);
    Rectangle r={(Ui::LARGURA-largura)/2,112,largura,50};   // logo abaixo dos cartões do topo
    Color fundo=multiplicar(cor,0.25f); fundo.a=(unsigned char)(235*alfa);
    Color borda=cor; borda.a=(unsigned char)(255*alfa);
    DrawRectangleRounded(r,0.4f,8,fundo);
    DrawRectangleRoundedLinesEx(r,0.4f,8,2.5f,borda);
    Color letra=jogo.tipoDaFaixa==MSG_BOA?OURO:Color{255,170,160,255}; letra.a=(unsigned char)(255*alfa);
    float tam=24; while(tam>16 && larguraDoTexto(jogo.faixa,tam)>largura-40) tam-=1;
    textoCentro(jogo.faixa,r.x+r.width/2,r.y+(50-tam)/2,tam,letra);
}
// ---------------------------------------------------------------------------
// MOVIMENTO SUAVE NO MAPA
// A lógica anda de célula em célula; aqui o desenho "persegue" a posição real,
// então o jogador e os monstros deslizam em vez de pular. A câmera segue a
// posição desenhada do jogador.
// ---------------------------------------------------------------------------
struct PosicaoSuave { float x, y; };
PosicaoSuave jogadorSuave;
PosicaoSuave monstrosSuaves[MAX_MONSTROS];
int andarDesenhado = -1;
double ultimoQuadroDoMapa = 0;

void seguir(PosicaoSuave& s,int x,int y,bool pular,float dt) {
    if (pular || std::fabs(s.x-x)>2.5f || std::fabs(s.y-y)>2.5f) { s.x=(float)x; s.y=(float)y; return; }
    s.x=Efeitos::aproximar(s.x,(float)x,22,dt);
    s.y=Efeitos::aproximar(s.y,(float)y,22,dt);
}
// Nome do tema aparecendo grande no meio do mapa ao chegar num trecho novo.
double tituloDoTemaDesde=-100;
int temaDoTitulo=0;
uint32_t sementeDesenhada=0;
void atualizarPosicoesSuaves(const Jogo& jogo) {
    double agora=GetTime();
    float dt=(float)std::min(0.1,agora-ultimoQuadroDoMapa);
    ultimoQuadroDoMapa=agora;
    bool outraPartida=(sementeDesenhada!=jogo.semente);
    bool novoAndar=(andarDesenhado!=jogo.andar) || outraPartida;
    if(novoAndar) {
        int temaNovo=temaDoAndar(jogo.andar);
        if(andarDesenhado<0 || outraPartida || temaDoAndar(andarDesenhado)!=temaNovo) {
            tituloDoTemaDesde=agora; temaDoTitulo=temaNovo;
        }
        if(andarDesenhado>=0) Efeitos::escurecer();   // desceu ou subiu a escada
        andarDesenhado=jogo.andar;
        sementeDesenhada=jogo.semente;
        Efeitos::limparParticulas(Efeitos::CAMADA_MAPA);
        prepararTapete(jogo);
    }
    seguir(jogadorSuave,jogo.jogador.x,jogo.jogador.y,novoAndar,dt);
    const Andar& a=andarAtual(jogo);
    for(int i=0;i<a.numMonstros;i++) seguir(monstrosSuaves[i],a.monstros[i].x,a.monstros[i].y,novoAndar,dt);
}
// Gotas pingando na caverna e brasas subindo da lava (só onde o jogador vê).
double ultimaParticulaDoAmbiente=0;
void particulasDoAmbiente(const Jogo& jogo,const Andar& a,int cx,int cy,int nx,int ny) {
    int tema=temaDoAndar(jogo.andar);
    if(tema!=TEMA_CAVERNA && tema!=TEMA_VULCAO) return;
    double agora=GetTime();
    if(agora-ultimaParticulaDoAmbiente<0.06) return;
    ultimaParticulaDoAmbiente=agora;
    // Sorteia uma célula visível na tela.
    for(int tentativa=0;tentativa<12;tentativa++) {
        int x=cx+GetRandomValue(0,nx), y=cy+GetRandomValue(0,ny);
        if(!dentroDoMapa(x,y) || !jogo.visivel[y][x]) continue;
        TipoCelula c=a.mapa.celulas[y][x];
        Efeitos::Particula p{};
        p.inicio=agora; p.camada=Efeitos::CAMADA_MAPA; p.giro=0;
        if(tema==TEMA_CAVERNA && (c==CHAO || c==POCA)) {
            // Gota caindo do teto.
            p.x=x*TAM+GetRandomValue(4,20); p.y=y*TAM-10; p.vx=0; p.vy=60; p.gravidade=260;
            p.cor={150,205,255,220}; p.tamanho=1.6f; p.vida=0.35; p.forma=Efeitos::BOLINHA;
            Efeitos::criarParticula(p);
            if(c==POCA) {   // ao cair na poça, uma ondinha
                Efeitos::Particula anel=p; anel.y=y*TAM+14; anel.vy=0; anel.gravidade=0; anel.inicio=agora+0.33;
                anel.forma=Efeitos::ANEL; anel.tamanho=5; anel.vida=0.5; anel.cor={150,205,255,160};
                Efeitos::criarParticula(anel);
            }
            return;
        }
        if(tema==TEMA_VULCAO && c==LAVA) {
            // Brasa subindo e se apagando.
            p.x=x*TAM+GetRandomValue(3,21); p.y=y*TAM+GetRandomValue(6,18);
            p.vx=(float)GetRandomValue(-12,12); p.vy=-(float)GetRandomValue(25,55); p.gravidade=0;
            p.cor=GetRandomValue(0,1)?Color{255,170,60,255}:Color{255,90,40,255};
            p.tamanho=1.8f; p.vida=0.9+GetRandomValue(0,6)/10.0; p.forma=Efeitos::QUADRADO;
            Efeitos::criarParticula(p);
            return;
        }
    }
}
// Nome do tema grande no meio do mapa (aparece e some em ~3 segundos).
void tituloDoTema() {
    float t=(float)(GetTime()-tituloDoTemaDesde);
    if(t<0 || t>3.2f) return;
    float alfa = t<0.4f ? t/0.4f : (t>2.4f ? (3.2f-t)/0.8f : 1.0f);
    const FichaDoTema& f=FICHAS_DOS_TEMAS[temaDoTitulo];
    float meioX=Ui::LARGURA/2.0f, y=Ui::ALTURA/2.0f-110;
    DrawRectangle(0,(int)y-14,Ui::LARGURA,112,{8,12,24,(unsigned char)(170*alfa)});
    Color ouro=OURO; ouro.a=(unsigned char)(255*alfa);
    Color texto=TEXTO; texto.a=(unsigned char)(230*alfa);
    textoCentro(f.nome,meioX,y,40,ouro);
    textoCentro(f.frase,meioX,y+54,20,texto);
}
// ---- Ambiente das salas especiais -------------------------------------------
// Um pedaço de chão decorado em volta de cada surpresa (só o círculo de 3x3
// casas em volta dela, não a sala toda): pátio de pedra com musgo na fonte,
// círculo de runas com velas no altar, moedas espalhadas em volta do baú,
// arena de areia no gongo e o acampamento do viajante.
static float acaso(int x,int y,int k) {           // 0..1, sempre igual para a mesma casa
    unsigned h=(unsigned)x*73856093u^(unsigned)y*19349663u^(unsigned)k*83492791u;
    h^=h>>13; h*=0x5bd1e995u; h^=h>>15;
    return (h%1000)/1000.0f;
}
// Raio do ambiente: o círculo de 3x3 casas se tudo em volta for chão;
// se encostar numa parede (arena em sala estreita), fica só embaixo do objeto.
static float raioDoAmbiente(const Andar& a,int x,int y) {
    for(int d=0;d<8;d++) {
        int nx=x+DIRECOES_DX[d], ny=y+DIRECOES_DY[d];
        if(!dentroDoMapa(nx,ny) || a.mapa.celulas[ny][nx]!=CHAO) return 15;
    }
    return 34;
}
static void ambienteDaFonte(Vector2 c,float R,bool seca,float f,bool vivo,float t,int x,int y) {
    Color pedra=multiplicar(seca?Color{92,92,96,255}:Color{86,106,122,255},f);
    Color borda=multiplicar(seca?Color{120,118,116,255}:Color{146,166,182,255},f);
    Color rejunte=multiplicar({58,70,82,255},f);
    DrawCircleV(c,R,pedra);
    // Mosaico: um anel no meio e linhas saindo da fonte.
    DrawRing(c,R*0.55f,R*0.55f+2,0,360,32,borda);
    for(int k=0;k<8;k++) {
        float ang=k*0.785f+0.39f;
        DrawLineEx({c.x+std::cos(ang)*R*0.58f,c.y+std::sin(ang)*R*0.58f},
                   {c.x+std::cos(ang)*(R-3),c.y+std::sin(ang)*(R-3)},1.2f,rejunte);
    }
    DrawRing(c,R-3,R,0,360,40,borda);
    if(!seca) {
        // A água transborda: uma poça em volta da fonte e ondinhas saindo.
        DrawRing(c,11,15,0,360,32,multiplicar({60,140,205,255},f));
        if(vivo) {
            float fase=std::fmod(t*0.6f,1.0f);
            Color onda=multiplicar({150,210,255,255},f); onda.a=(unsigned char)(150*(1-fase));
            DrawRing(c,14+fase*(R*0.5f-14),15+fase*(R*0.5f-14),0,360,32,onda);
            for(int k=0;k<3;k++) {         // brilhos na água
                float b=std::sin(t*3+k*2.1f+x);
                if(b>0.6f) DrawCircleV({c.x+std::cos(k*2.1f+y)*13,c.y+std::sin(k*2.1f+y)*13},1.5f,{230,250,255,255});
            }
        }
    } else {
        // Seca: só rachaduras.
        DrawLineEx({c.x-R*0.7f,c.y+4},{c.x-R*0.4f,c.y+9},1.2f,rejunte);
        DrawLineEx({c.x-R*0.4f,c.y+9},{c.x-R*0.2f,c.y+R*0.5f},1.2f,rejunte);
    }
    // Musgo e florzinhas na borda do pátio.
    for(int k=0;k<7;k++) {
        float ang=acaso(x,y,k)*6.283f, r=R-1-acaso(x,y,k+20)*4;
        Vector2 p={c.x+std::cos(ang)*r,c.y+std::sin(ang)*r};
        Color musgo=multiplicar(seca?Color{110,100,70,255}:Color{70,150,90,255},f);
        DrawLineEx({p.x,p.y},{p.x-2,p.y-4},1.4f,musgo);
        DrawLineEx({p.x,p.y},{p.x+1,p.y-5},1.4f,musgo);
        DrawLineEx({p.x,p.y},{p.x+3,p.y-3},1.4f,musgo);
        if(!seca && k%3==0) DrawCircleV({p.x+1,p.y-5},1.6f,multiplicar({235,225,250,255},f));
    }
}
static void ambienteDoAltar(Vector2 c,float R,bool usado,float f,bool vivo,float t,int x,int y) {
    float pulso = (vivo && !usado) ? 0.75f+0.25f*std::sin(t*2.2f) : 1.0f;
    Color fundo=multiplicar({44,24,28,255},f);
    Color runa=multiplicar(usado?Color{90,58,60,255}:Color{190,40,52,255},f*pulso);
    DrawCircleV(c,R,fundo);
    DrawRing(c,R-3,R,0,360,40,runa);
    DrawRing(c,R-12,R-10.5f,0,360,40,runa);
    // Runas entre os dois anéis.
    for(int k=0;k<8;k++) {
        float ang=k*0.785f+0.39f, r=R-6.5f;
        Vector2 p={c.x+std::cos(ang)*r,c.y+std::sin(ang)*r};
        switch(k%4) {
            case 0: DrawLineEx({p.x-2,p.y-2},{p.x+2,p.y+2},1.2f,runa); DrawLineEx({p.x+2,p.y-2},{p.x-2,p.y+2},1.2f,runa); break;
            case 1: DrawLineEx({p.x-2,p.y-2},{p.x,p.y+2},1.2f,runa); DrawLineEx({p.x,p.y+2},{p.x+2,p.y-2},1.2f,runa); break;
            case 2: DrawLineEx({p.x,p.y-3},{p.x,p.y+3},1.2f,runa); DrawLineEx({p.x-2,p.y},{p.x+2,p.y},1.2f,runa); break;
            default: DrawRing(p,1.2f,2.4f,0,360,10,runa);
        }
    }
    // Manchas de sangue no chão.
    for(int k=0;k<3;k++) {
        float ang=acaso(x,y,k)*6.283f, r=R*0.35f+acaso(x,y,k+9)*R*0.25f;
        DrawEllipse((int)(c.x+std::cos(ang)*r),(int)(c.y+std::sin(ang)*r),3+k,2,multiplicar({110,18,26,255},f));
    }
    // Quatro velas nos cantos (apagadas depois do pacto).
    for(int k=0;k<4;k++) {
        float ang=k*1.571f+0.785f;
        float vx=c.x+std::cos(ang)*(R+5), vy=c.y+std::sin(ang)*(R+5);
        DrawRectangle((int)vx-2,(int)vy-3,4,7,multiplicar({228,214,180,255},f));
        if(!usado && vivo) {
            float tremula=std::sin(t*13+k*1.7f)*0.6f;
            Color luz={255,170,60,90}; Color fora=luz; fora.a=0;
            DrawCircleGradient((int)vx,(int)vy-6,7,luz,fora);
            DrawCircleV({vx,vy-6+tremula*0.5f},2.0f+tremula*0.3f,{255,205,90,255});
        } else if(usado && vivo) {
            float sobe=std::fmod(t*0.8f+k*0.3f,1.0f);
            DrawCircleV({vx+std::sin(t*2+k)*1.5f,vy-5-sobe*8},1.2f,{150,150,160,(unsigned char)(140*(1-sobe))});
        }
    }
}
static void ambienteDoBau(Vector2 c,float R,bool aberto,float f,int x,int y) {
    Color brilho=multiplicar({150,118,48,255},f); brilho.a=90; Color fora=brilho; fora.a=0;
    DrawCircleGradient((int)c.x,(int)c.y,R,brilho,fora);
    // Moedas espalhadas (e umas pedrinhas preciosas). Aberto, sobram poucas.
    int quantas = aberto ? 4 : 11;
    for(int k=0;k<quantas;k++) {
        float ang=acaso(x,y,k)*6.283f, r=14+acaso(x,y,k+30)*(R-16);
        Vector2 p={c.x+std::cos(ang)*r,c.y+std::sin(ang)*r};
        if(k%5==4) {
            const Color GEMAS[3]={{230,70,90,255},{90,170,255,255},{110,220,130,255}};
            DrawPoly(p,4,2.6f,45,multiplicar(GEMAS[k%3],f));
        } else {
            DrawEllipse((int)p.x,(int)p.y+1,3.2f,2.0f,multiplicar({150,110,30,255},f));
            DrawEllipse((int)p.x,(int)p.y,3.2f,2.0f,multiplicar({240,198,72,255},f));
            if(k%3==0) DrawEllipse((int)p.x+1,(int)p.y-2,3.2f,2.0f,multiplicar({255,220,110,255},f));   // pilha
        }
    }
}
static void ambienteDaArena(Vector2 c,float R,bool emLuta,float f,float t,int x,int y) {
    DrawCircleV(c,R,multiplicar({132,110,78,255},f));                 // areia
    DrawRing(c,R*0.6f,R*0.6f+1,0,360,32,multiplicar({112,92,64,255},f));
    // Riscos de batalha e ossinhos na areia.
    for(int k=0;k<2;k++) {
        float ang=acaso(x,y,k)*6.283f, r=std::max(R*0.5f,18.0f)+acaso(x,y,k+5)*std::max(R-26,0.0f);
        Vector2 p={c.x+std::cos(ang)*r,c.y+std::sin(ang)*r};
        DrawLineEx({p.x-4,p.y-1},{p.x+4,p.y+1},1.6f,multiplicar({225,215,192,255},f));
        DrawCircleV({p.x-4,p.y-1},1.6f,multiplicar({225,215,192,255},f));
        DrawCircleV({p.x+4,p.y+1},1.6f,multiplicar({225,215,192,255},f));
    }
    DrawLineEx({c.x-R*0.7f,c.y-R*0.3f},{c.x-R*0.35f,c.y-R*0.5f},1.0f,multiplicar({96,78,52,255},f));
    DrawLineEx({c.x+R*0.3f,c.y+R*0.55f},{c.x+R*0.7f,c.y+R*0.25f},1.0f,multiplicar({96,78,52,255},f));
    // Borda de pedras. Durante o desafio, ela brilha vermelha.
    if(emLuta) {
        Color v={255,50,50,(unsigned char)(110+70*std::sin(t*5))};
        DrawRing(c,R-3,R+3,0,360,48,v);
    }
    int pedras=(int)(R/2.4f);
    for(int k=0;k<pedras;k++) {
        float ang=k*6.283f/pedras;
        DrawCircleV({c.x+std::cos(ang)*(R-1),c.y+std::sin(ang)*(R-1)},2.6f,
                    multiplicar(k%2?Color{120,112,104,255}:Color{98,92,86,255},f));
    }
}
static void ambienteDoViajante(Vector2 c,float R,float f,bool vivo,float t,int x,int y) {
    Color terra=multiplicar({98,80,60,255},f); terra.a=110; Color fora=terra; fora.a=0;
    DrawCircleGradient((int)c.x,(int)c.y,R,terra,fora);
    // Saco de dormir de um lado do viajante e a fogueira do outro.
    if(R<30) return;
    int d=(int)(acaso(x,y,1)*4);                    // cima, direita, baixo ou esquerda
    Vector2 cama={c.x+DIRECOES_DX[d]*TAM,c.y+DIRECOES_DY[d]*TAM};
    Vector2 fogo={c.x-DIRECOES_DX[d]*TAM,c.y-DIRECOES_DY[d]*TAM};
    // Saco de dormir roxo com um travesseiro.
    DrawRectangleRounded({cama.x-10,cama.y-6,20,12},0.5f,6,multiplicar({104,66,138,255},f));
    DrawRectangleRounded({cama.x-10,cama.y-6,7,12},0.5f,6,multiplicar({206,196,176,255},f));
    DrawLineEx({cama.x-2,cama.y-5},{cama.x-2,cama.y+5},1.0f,multiplicar({70,44,96,255},f));
    // Fogueira: pedras em roda, lenha cruzada e brasas.
    for(int k=0;k<7;k++) {
        float ang=k*0.898f;
        DrawCircleV({fogo.x+std::cos(ang)*8,fogo.y+std::sin(ang)*6},2.2f,multiplicar({118,112,106,255},f));
    }
    DrawLineEx({fogo.x-5,fogo.y+3},{fogo.x+5,fogo.y-3},2.4f,multiplicar({110,72,40,255},f));
    DrawLineEx({fogo.x-5,fogo.y-3},{fogo.x+5,fogo.y+3},2.4f,multiplicar({96,62,34,255},f));
    if(vivo) {
        float tremula=std::sin(t*11+x)*1.2f+std::sin(t*17+y)*0.8f;
        Color luz={255,150,50,110}; Color semLuz=luz; semLuz.a=0;
        DrawCircleGradient((int)fogo.x,(int)fogo.y,16+tremula,luz,semLuz);
        DrawCircleV({fogo.x,fogo.y-2-tremula*0.3f},3.5f+tremula*0.4f,{255,140,40,255});
        DrawCircleV({fogo.x,fogo.y-3-tremula*0.3f},1.8f,{255,225,120,255});
    } else {
        DrawCircleV(fogo,2.0f,multiplicar({150,70,40,255},f));
    }
}
void desenharAmbientes(const Jogo& jogo,const Andar& a,float origemX,float origemY) {
    float t=(float)GetTime();
    auto preparar=[&](int x,int y,Vector2& c,float& f,bool& vivo) {
        c={std::round(origemX+x*TAM)+TAM/2.0f,std::round(origemY+y*TAM)+TAM/2.0f};
        vivo=jogo.visivel[y][x];
        f = vivo ? std::clamp(brilho(jogo.luz[y][x]),0.27f,1.0f) : 0.32f;
    };
    for(int i=0;i<a.numEspeciais;i++) {
        const Especial& e=a.especiais[i];
        if(!a.descoberto[e.y][e.x]) continue;
        Vector2 c; float f; bool vivo; preparar(e.x,e.y,c,f,vivo);
        float R=raioDoAmbiente(a,e.x,e.y);
        switch(e.tipo) {
            case ESP_FONTE: ambienteDaFonte(c,R,e.usado,f,vivo,t,e.x,e.y); break;
            case ESP_ALTAR: ambienteDoAltar(c,R,e.usado,f,vivo,t,e.x,e.y); break;
            case ESP_BAU:   ambienteDoBau(c,R,e.usado,f,e.x,e.y); break;
            default:        ambienteDaArena(c,R,vivo && e.onda>0 && !e.usado,f,t,e.x,e.y); break;
        }
    }
    for(int i=0;i<a.numNpcs;i++) {
        const Npc& n=a.npcs[i];
        if(n.tipo!=NPC_VIAJANTE || !a.descoberto[n.y][n.x]) continue;
        Vector2 c; float f; bool vivo; preparar(n.x,n.y,c,f,vivo);
        ambienteDoViajante(c,raioDoAmbiente(a,n.x,n.y),f,vivo,t,n.x,n.y);
    }
}
// Zoom do mapa: casas de 24, 36 ou 48 pixels (Opções e +/-; padrão 36).
// Zoom do mapa (Opções, roda do mouse, + e -). Muda devagar, sem pular.
float zoomEscolhido() {
    switch(opcoes().zoomDoMapa) { case 0: return 1.0f; case 2: return 2.0f; }
    return 1.5f;
}
float zoomDoMapa() {
    static float atual=-1;
    static double ultimo=0;
    float alvo=zoomEscolhido();
    double agora=GetTime();
    if(atual<0 || agora-ultimo>0.5) atual=alvo;
    else atual+=(alvo-atual)*std::min(1.0f,(float)(agora-ultimo)*10.0f);
    if(std::fabs(alvo-atual)<0.002f) atual=alvo;
    ultimo=agora;
    return atual;
}
// (Definidas mais abaixo, junto com as falas.)
size_t letrasReveladas(const std::string& s,double t,float porSegundo,bool& completo);
void vozDasLetras(const std::string& s,size_t antes,size_t agora,Voz voz,int& contador);
Vector2 casaNaTela(float x,float y);

// ---- Gritos dos monstros -----------------------------------------------------
// Quando um monstro à vista percebe o jogador (passa a caçar), aparece um
// balãozinho em cima dele com um grito, na voz da espécie. (Só desenho: a
// lógica do jogo não sabe disso, e os bots não veem.)
std::vector<std::string> gritosDaEspecie(char simbolo) {
    switch(simbolo) {
        case 'r': return {"Squiii!","Squi squi!"};
        case 'k': return {"Yip yip!","Brasa! Brasa!"};
        case 'j': return {"Grrr...","Au! Au!"};
        case 'm': return {"Uuh uuh!","Aah aah!"};
        case 't': return {"Croac!","Croooac..."};
        case 'g': return {"Hehehe!","Ouro! Ouro!","Pega ele!"};
        case 'v': return {"Iii iii!","Skriii!"};
        case 'O': return {"GRUAAAR!","ESMAGAR!"};
        case 'N': return {"Sssss...","Ssssiim..."};
        case 'G': return {"Hihihi! TUUU!"};
        case 'W': return {"...shhh...","...aqui..."};
        case 'M': return {"NHAC!","Nhac nhac!"};
        case 'S': return {"Venha... eu espero."};
        case 'a': return {"Tsc tsc tsc!","Sssk!"};
        case 'f': return {"Blup!","Esporos!"};
        case 'e': return {"Clac clac!","Ossos... novos..."};
        case 's': return {"Fssss!","Queima!"};
    }
    return {"!"};
}
struct Grito { int monstro; std::string texto; double inicio; Voz voz; size_t tocados; int contador; };
std::vector<Grito> gritos;
int estadoAntes[MAX_MONSTROS];
int andarDosGritos=-1, monstrosDosGritos=-1;
double ultimoGrito=0;
const double DURACAO_DO_GRITO=1.8;

void atualizarGritos(const Jogo& jogo) {
    const Andar& a=andarAtual(jogo);
    double agora=GetTime();
    // Andar novo (ou partida nova): começa do estado atual, sem gritaria.
    if(jogo.andar!=andarDosGritos || a.numMonstros!=monstrosDosGritos) {
        andarDosGritos=jogo.andar; monstrosDosGritos=a.numMonstros;
        for(int i=0;i<a.numMonstros;i++) estadoAntes[i]=a.monstros[i].estado;
        gritos.clear();
        return;
    }
    for(int i=0;i<a.numMonstros;i++) {
        const Monstro& m=a.monstros[i];
        bool comecouACacar = m.estado==CACANDO && estadoAntes[i]!=CACANDO;
        // Muitos de uma vez: os outros gritam logo depois (não perdem a vez).
        if(comecouACacar && (gritos.size()>=3 || agora-ultimoGrito<0.25) && m.vivo && monstroAVista(jogo,m)) continue;
        estadoAntes[i]=m.estado;
        if(!comecouACacar || !m.vivo || !monstroAVista(jogo,m) || cameraDaCena) continue;   // na cena, só o "!"
        char simbolo=tipoDe(m).simbolo;
        if(simbolo=='G' || simbolo=='S') continue;          // esses dois têm cena própria
        std::vector<std::string> opcoes=gritosDaEspecie(simbolo);
        gritos.push_back({i,opcoes[(size_t)(i+jogo.passos)%opcoes.size()],agora,vozDoMonstro(simbolo),0,0});
        ultimoGrito=agora;
    }
    gritos.erase(std::remove_if(gritos.begin(),gritos.end(),[&](const Grito& g){
        return agora-g.inicio>DURACAO_DO_GRITO || g.monstro>=a.numMonstros || !a.monstros[g.monstro].vivo; }),gritos.end());
}
// Balãozinho de grito (também usado pelo gnomo nas cenas). "x" e "y": ponto
// da tela onde a ponta encosta (em cima da cabeça de quem grita).
void balaozinho(const std::string& s,float x,float y,float alfa,Color tinta={46,36,30,255}) {
    float w=larguraDoTexto(s,17)+24, h=30;
    Rectangle r={x-w/2,y-h-12,w,h};
    unsigned char A=(unsigned char)(255*alfa);
    trianguloCheio({x-7,r.y+h-2},{x+7,r.y+h-2},{x,y-2},{58,44,34,A});
    DrawRectangleRounded(r,0.45f,8,{242,234,212,A});
    DrawRectangleRoundedLinesEx(r,0.45f,8,2.0f,{58,44,34,A});
    trianguloCheio({x-4,r.y+h-4},{x+4,r.y+h-4},{x,y-5},{242,234,212,A});
    tinta.a=A;
    texto(s,r.x+12,r.y+5,17,tinta);
}
// A câmera em um eixo: o herói fica sempre no meio da tela (assim ele nunca
// fica embaixo dos cartões dos cantos). Perto da borda do mapa, aparece o
// escuro de fora, mas no máximo meia tela.
static float eixoDaCamera(float jogador,float vista,int tamanho) {
    float centro=jogador+0.5f-vista/2.0f;
    return std::clamp(centro,-vista/2.0f,(float)tamanho-vista/2.0f);
}
// v1.24: enfeites das salas dos chefes (Andar::enfeites), por cima do chão:
// ouro, tronos, casulos, ovos, o círculo do ritual, velas, colunas, forja,
// bigorna, tapete e braseiros. Só nas casas que aparecem na tela.
void desenharEnfeites(const Jogo& jogo,const Andar& a,int cx,int cy,int nx,int ny) {
    float t=(float)GetTime();
    struct Enfeite { int x, y; char tipo; };
    for(int iy=-1;iy<=ny+1;iy++) for(int ix=-1;ix<=nx+1;ix++) {
        Enfeite e={cx+ix,cy+iy,0};
        if(!dentroDoMapa(e.x,e.y) || !a.enfeites[e.y][e.x] || !a.descoberto[e.y][e.x]) continue;
        e.tipo=a.enfeites[e.y][e.x];
        bool agora=jogo.visivel[e.y][e.x];
        float f=agora?std::clamp(brilho(jogo.luz[e.y][e.x]),0.35f,1.0f):0.35f;
        float px=(float)e.x*TAM, py=(float)e.y*TAM;
        auto c=[&](Color k){ return multiplicar(k,f); };
        switch(e.tipo) {
        case '$':   // moedas de ouro
            for(int k=0;k<6;k++) { float cx=px+5+(k%3)*7, cy=py+16-(k/3)*6; DrawCircleV({cx,cy},4,c({226,176,62,255})); DrawCircleLinesV({cx,cy},4,c({150,104,30,255})); }
            break;
        case 'R':   // trono de madeira do rei goblin
            DrawRectangle(px+3,py-6,18,22,c({92,58,32,255}));
            for(int k=0;k<3;k++) DrawTriangle({px+4+k*6,py-6},{px+10+k*6,py-6},{px+7+k*6,py-12},c({226,176,62,255}));
            DrawRectangle(px+1,py+12,22,8,c({120,80,44,255}));
            DrawRectangle(px+6,py+2,12,10,c({150,40,44,255}));
            break;
        case 'C': { // casulo
            float bal=std::sin(t+e.x)*1.2f;
            DrawLine((int)(px+12),(int)py,(int)(px+12+bal),(int)(py+5),c({210,220,225,255}));
            DrawEllipse(px+12+bal,py+13,6,9,c({214,216,205,255}));
            DrawLine((int)(px+7+bal),(int)(py+10),(int)(px+17+bal),(int)(py+12),c({150,150,145,255}));
            DrawLine((int)(px+7+bal),(int)(py+15),(int)(px+17+bal),(int)(py+17),c({150,150,145,255}));
            break; }
        case 'o':   // ovos
            for(int k=0;k<3;k++) DrawEllipse(px+6+k*6,py+15-(k%2)*4,4,5,c({214,222,200,255}));
            if(agora) DrawCircleGradient((int)px+12,(int)py+13,14,{170,255,160,(unsigned char)(50+30*std::sin(t*3+e.x))},{170,255,160,0});
            break;
        case 'O': { // círculo do ritual (3 casas para cada lado)
            float p2=0.6f+0.4f*std::sin(t*2);
            Color v={120,255,150,(unsigned char)(200*p2*(agora?1:0.4f))};
            DrawEllipseLines((int)px+12,(int)py+12,62,40,v);
            DrawEllipseLines((int)px+12,(int)py+12,54,34,v);
            for(int k=0;k<8;k++) { float an=t*0.4f+k*0.785f; DrawRectangle((int)(px+12+std::cos(an)*58)-2,(int)(py+12+std::sin(an)*37)-3,4,6,v); }
            break; }
        case 'v':   // velas
            for(int k=0;k<2;k++) {
                float vx=px+7+k*10, vy=py+18-k*3;
                DrawRectangle(vx-2,vy-8,4,8,c({220,214,190,255}));
                DrawCircleV({vx,vy-10},2.5f+0.6f*std::sin(t*9+k),{170,255,180,(unsigned char)(agora?230:90)});
            }
            if(agora) DrawCircleGradient((int)px+12,(int)py+10,26,{150,255,170,40},{150,255,170,0});
            break;
        case 'c':   // coluna de pedra
            DrawRectangle(px+4,py-8,16,30,c({120,124,132,255}));
            DrawRectangle(px+14,py-8,6,30,c({86,90,98,255}));
            DrawRectangle(px+2,py-10,20,4,c({140,144,152,255}));
            DrawRectangle(px+2,py+20,20,4,c({100,104,112,255}));
            break;
        case 'F':   // boca da forja
            DrawRectangle(px+1,py+2,22,20,c({50,36,36,255}));
            DrawRectangle(px+5,py+8,14,12,{255,(unsigned char)(130+40*std::sin(t*5)),50,(unsigned char)(agora?255:120)});
            if(agora) DrawCircleGradient((int)px+12,(int)py+14,34,{255,130,50,70},{255,130,50,0});
            break;
        case 'B':   // bigorna
            DrawRectangle(px+3,py+8,16,5,c({70,66,72,255}));
            DrawTriangle({px+19,py+8},{px+19,py+13},{px+23,py+8},c({70,66,72,255}));
            DrawRectangle(px+8,py+13,8,7,c({56,52,58,255}));
            break;
        case '=':   // tapete vermelho
            DrawRectangle(px+3,py,18,TAM,c({120,28,40,255}));
            DrawRectangle(px+3,py,2,TAM,c({220,170,70,255}));
            DrawRectangle(px+19,py,2,TAM,c({220,170,70,255}));
            break;
        case 'b':   // braseiro
            if(agora) DrawCircleGradient((int)px+12,(int)py+8,40,{255,130,50,60},{255,130,50,0});
            DrawRectangle(px+5,py+10,14,5,c({80,64,58,255}));
            DrawRectangle(px+10,py+15,4,8,c({70,56,50,255}));
            DrawCircleV({px+12,py+7},4.5f+std::sin(t*8+e.x),{255,170,60,(unsigned char)(agora?230:90)});
            DrawCircleV({px+12,py+8},2.5f,{255,240,170,(unsigned char)(agora?240:90)});
            break;
        case 'X':   // o trono do chefe final (passa um pouco da casa)
            DrawRectangle(px-6,py-14,36,36,c({44,32,44,255}));
            DrawTriangle({px-6,py-14},{px+12,py-26},{px+30,py-14},c({44,32,44,255}));
            DrawRectangle(px,py-6,24,22,c({120,26,38,255}));
            DrawCircleV({px+12,py-16},3,c({220,170,70,255}));
            break;
        }
    }
}
void mapa(const Jogo& jogo) {
    atualizarPosicoesSuaves(jogo);
    const Andar& a=andarAtual(jogo);
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    // O mapa ocupa a tela toda: tudo é desenhado em "pixels do mundo" (casa
    // x fica em x*TAM) e o zoom da câmera aumenta na tela.
    float z = cameraDaCena ? cenaZoom : zoomDoMapa(), tam=TAM*z;
    float vistaW=Ui::LARGURA/tam, vistaH=Ui::ALTURA/tam;   // em casas
    float camX=eixoDaCamera(cameraDaCena?cenaX:jogadorSuave.x,vistaW,MAPA_LARGURA);
    float camY=eixoDaCamera(cameraDaCena?cenaY:jogadorSuave.y,vistaH,MAPA_ALTURA);
    int cx=(int)std::floor(camX), cy=(int)std::floor(camY);
    int nx=(int)std::ceil(vistaW)+1, ny=(int)std::ceil(vistaH)+1;
    cameraX=camX; cameraY=camY; cameraZoom=z;
    Ui::comecarZoom(-camX*tam,-camY*tam,z);
    for(int iy=0;iy<=ny;iy++)for(int ix=0;ix<=nx;ix++)
        tile(jogo,a,cx+ix,cy+iy,(cx+ix)*TAM,(cy+iy)*TAM);
    // O chão em volta da fonte, do altar, do baú, do gongo e do viajante.
    desenharAmbientes(jogo,a,0,0);
    for(int iy=0;iy<=ny;iy++)for(int ix=0;ix<=nx;ix++)
        tileObjetos(jogo,a,cx+ix,cy+iy,(cx+ix)*TAM,(cy+iy)*TAM);
    desenharEnfeites(jogo,a,cx,cy,nx,ny);   // v1.24: salas dos chefes
    // Placa "LOJA" em cima da porta da lojinha.
    if(a.loja.existe && a.descoberto[a.loja.portaY][a.loja.portaX]) {
        float px=(float)a.loja.portaX*TAM, py=(float)a.loja.portaY*TAM;
        Rectangle placa={px-7,py-17,TAM+14.0f,16};
        DrawRectangleRec(placa,{120,80,40,255});
        DrawRectangleLinesEx(placa,1,OURO);
        textoCentro("LOJA",px+TAM/2.0f,py-16,13,OURO);
    }
    // Monstros visíveis e o jogador, nas posições suaves.
    auto naTela=[&](float x,float y,float& px,float& py){
        px=std::round(x*TAM); py=std::round(y*TAM);
    };
    for(int i=0;i<a.numMonstros;i++) {
        const Monstro& m=a.monstros[i];
        float px,py; naTela(monstrosSuaves[i].x,monstrosSuaves[i].y,px,py);
        if(!monstroAVista(jogo,m)) {
            // Sombra numa casa visível, mas longe: só um tremular no escuro.
            if(m.vivo && jogo.visivel[m.y][m.x] && (tipoDe(m).especial & SOMBRIO) &&
               std::sin((float)GetTime()*2.3f+m.x*1.7f)>0.75f) {
                DrawCircleGradient((int)px+12,(int)py+12,9,{150,120,210,40},{150,120,210,0});
            }
            continue;
        }
        // v1.24: os esqueletos do Necromante ficam enterrados (um montinho de
        // ossos) até ele levantar todos na cena da sala.
        if(m.guarda && tipoDe(m).simbolo=='e' && (esqueletosEnterrados || m.estado==DORMINDO)) {
            if(!esqueletosEnterrados) {
                Color osso={214,206,182,255};
                DrawEllipse((int)px+12,(int)py+19,9,3,{20,18,16,120});
                DrawRectangle((int)px+5,(int)py+15,14,3,osso); DrawRectangle((int)px+8,(int)py+12,9,3,osso);
                DrawCircle((int)px+16,(int)py+11,3.5f,osso);
            }
            continue;
        }
        desenharMonstroNoMapa(jogo,m,px,py);
    }
    float hx,hy; naTela(jogadorSuave.x,jogadorSuave.y,hx,hy);
    if(jogo.furtivo) {
        // Furtivo: o herói fica escuro e meio transparente, sem o brilho da tocha.
        desenharSprite(HEROI,hx+2,hy+1,1.25f,0.55f,0.7f);
    } else {
        DrawCircle((int)hx+12,(int)hy+12,12,{220,205,141,55});
        desenharSprite(HEROI,hx+2,hy+1,1.25f,1.0f);
    }
    particulasDoAmbiente(jogo,a,cx,cy,nx,ny);
    // Efeitos do mapa (lascas, espinhos, números) acompanham a câmera.
    Efeitos::desenharParticulas(Efeitos::CAMADA_MAPA,0,0);
    Efeitos::desenharTextosFlutuantes(Efeitos::CAMADA_MAPA,0,0);
    Ui::terminarZoom();
    // Gritos dos monstros que acabaram de te perceber.
    atualizarGritos(jogo);
    for(Grito& g:gritos) {
        double idade=GetTime()-g.inicio;
        bool completo;
        size_t bytes=letrasReveladas(g.texto,idade,26*ritmoDaVoz(g.voz),completo);
        if(bytes>g.tocados) { vozDasLetras(g.texto,g.tocados,bytes,g.voz,g.contador); g.tocados=bytes; }
        float alfa = idade>DURACAO_DO_GRITO-0.4 ? (float)((DURACAO_DO_GRITO-idade)/0.4) : 1.0f;
        float subida=(float)std::min(idade,0.25)*24;
        Vector2 p=casaNaTela(monstrosSuaves[g.monstro].x+0.5f,monstrosSuaves[g.monstro].y);
        balaozinho(g.texto.substr(0,bytes),p.x,p.y+4-subida,alfa);
    }
    if(esconderCartoes) return;
    tituloDoTema();
    faixaDeDestaque(jogo);
    hud(jogo);
}
void overlayBasico(const std::string& tituloModal) {
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,177});
    painel({215,145,1010,610},{26,35,56,255},OURO);
    textoCentro(tituloModal,720,174,31,OURO);
    separador(255,222,930);
}
// Se o texto de uma opção fala de um item ("Poção de Vida  4/6", "Largar Éter"...)
// ou de um atributo (ficha), devolve o desenho dele (o nome mais comprido que
// aparecer no começo do texto), ou nullptr.
const Sprite* iconeNoTexto(const std::string& s) {
    const Sprite* melhor=nullptr; size_t maior=0;
    auto testar=[&](const std::string& nome,const Sprite* arte){
        size_t pos=s.find(nome);
        if(pos!=std::string::npos && pos<=8 && nome.size()>maior) { maior=nome.size(); melhor=arte; }
    };
    for(int i=0;i<NUM_TIPOS_DE_ITEM;i++) testar(TIPOS_DE_ITEM[i].nome,ARTE_DOS_ITENS[i]);
    for(int a=0;a<NUM_ATRIBUTOS;a++) testar(NOMES_DOS_ATRIBUTOS[a],ARTE_DOS_ATRIBUTOS[a]);
    return melhor;
}
bool faixaBotao(const std::string& txt,Rectangle r,bool sel,int indice=-1,int cursor=0,float tam=23,const Sprite* arte=nullptr) {
    bool sobre=mouseSobre(r);
    bool chegou=true;   // o destaque já está (quase todo) sobre este botão?
    if(sel) {
        // O destaque dourado desliza da opção anterior até esta.
        painel(r,PAINEL2,Color{70,89,110,255});
        Rectangle d=Efeitos::destaqueDeslizante(r);
        painel(d,OURO,OURO);
        chegou=std::fabs(d.y-r.y)<r.height*0.5f && std::fabs(d.x-r.x)<r.width*0.5f;
    } else {
        // Mouse em cima: o botão clareia e ganha borda ciano.
        painel(r,sobre?Color{43,58,88,255}:PAINEL2,sobre?CIANO:Color{70,89,110,255});
    }
    float recuo=0;
    if(arte) {
        // Desenho do item (ou atributo) à esquerda do nome.
        float pixel=(r.height-6)/16.0f;
        icone(arte->data(),r.x+12,r.y+3,pixel);
        recuo=pixel*16+4;
    }
    texto(txt,r.x+20+recuo+(sobre&&!sel?3:0),r.y+(r.height-tam)/2-1,tam,(sel&&chegou)?FUNDO:TEXTO);
    if(indice>=0) registrarOpcao(r,indice,cursor);
    return sel&&chegou;   // v1.27: o texto por cima pode ficar escuro (sobre o ouro)
}
// ---- Golpes na batalha (v1.27) ----
// Cada golpe mostra o elemento, a força (em barrinhas), o PP e se é forte ou
// fraco contra o monstro; o marcado aparece em detalhes à esquerda.

// A eficácia do golpe contra os inimigos que ainda lutam: 4 (super), 2
// (normal), 1 (pouco), 0 (não afeta) ou -1 se muda de um inimigo para outro.
int eficaciaContraTodos(const Batalha& b,const Golpe& g) {
    int e=-2;
    for(int i=0;i<b.numInimigos;i++) {
        if(!b.naLuta[i]) continue;
        int x=eficacia((Elemento)g.elemento,(Elemento)b.inimigos[i].elemento);
        if(e==-2) e=x; else if(e!=x) return -1;
    }
    return e<0?2:e;
}
// O selo de eficácia (nada quando é normal).
void seloDeEficacia(int efic,float x,float y,float tam,bool sobreOuro) {
    const char* t=nullptr; Color c=APAGADO;
    if(efic==4) { t="SUPER EFICAZ x2"; c={90,220,120,255}; }
    else if(efic==1) { t="pouco eficaz x½"; c={255,150,70,255}; }
    else if(efic==0) { t="não afeta x0"; c={150,155,170,255}; }
    else if(efic==-1) { t="depende do alvo"; c={150,175,210,255}; }
    if(!t) return;
    float w=larguraDoTexto(t,tam)+18, h=tam+8;
    Color fundo=sobreOuro?multiplicar(c,0.45f):multiplicar(c,0.22f);
    Color letra=sobreOuro?Color{20,20,24,255}:c;
    if(sobreOuro) { fundo=c; fundo.a=200; }
    DrawRectangleRounded({x,y,w,h},0.5f,6,fundo);
    texto(t,x+9,y+3,tam,letra);
}
// Força do golpe em 5 barrinhas (como sinal de celular): 1 a cada 20 de poder.
void barrinhasDePoder(int poder,float x,float base,Color cheia,Color vazia) {
    int cheias=(poder+19)/20;
    for(int k=0;k<5;k++) {
        float h=6+k*3.2f;
        DrawRectangleRec({x+k*7.0f,base-h,5,h},k<cheias?cheia:vazia);
    }
}
// Uma linha do menu de golpes.
void linhaDoGolpe(const Batalha* b,const Golpe& g,int pp,Rectangle r,bool sel,int indice,int cursor) {
    sel=faixaBotao("",r,sel,indice,cursor,20);
    Color letra=sel?FUNDO:TEXTO, fraca=sel?Color{70,58,30,255}:APAGADO;
    iconeDoElemento(g.elemento,r.x+12,r.y+(r.height-27)/2,3.0f);
    textoQueCabe(g.nome,r.x+48,r.y+(r.height-19)/2-1,19,205,letra);
    float px=r.x+262, meio=r.y+r.height/2;
    if(g.poder>0) {
        barrinhasDePoder(g.poder,px,meio+9,sel?Color{60,45,15,255}:Color{255,200,90,255},sel?Color{200,160,70,255}:Color{55,66,90,255});
        texto(std::to_string(g.poder),px+40,meio-10,18,letra);
    } else texto("efeito",px,meio-9,15,fraca);
    float qx=r.x+352;
    if(g.ppMaximo>0) {
        texto("PP",qx,meio-8,12,fraca);
        std::string t=std::to_string(pp)+"/"+std::to_string(g.ppMaximo);
        texto(t,qx+22,meio-11,17,pp==0?PERIGO:letra);
        float f=std::clamp((float)pp/g.ppMaximo,0.0f,1.0f);
        DrawRectangleRounded({qx+22,meio+9,56,5},0.5f,4,sel?Color{200,160,70,255}:Color{40,52,78,255});
        if(f>0) DrawRectangleRounded({qx+22,meio+9,std::max(5.0f,56*f),5},0.5f,4,f<0.25f?PERIGO:(sel?Color{40,90,110,255}:CIANO));
    } else texto("PP sem limite",qx,meio-9,14,fraca);
    if(b && g.poder>0) seloDeEficacia(eficaciaContraTodos(*b,g),r.x+r.width-150,meio-11,13,sel);
    if(!b) {   // sem inimigos para comparar: o efeito do golpe no lugar do selo
        std::string ef=Mochila::efeitoDoGolpe(g);
        float cabe=r.width-466;
        size_t ponto=ef.find("·");   // não cabe: fica só a primeira parte ("sobe a sua defesa")
        if(larguraDoTexto(ef,15)>cabe && ponto!=std::string::npos) ef=ef.substr(0,ef.find_last_not_of(' ',ponto-1)+1);
        if(!ef.empty() && ef!="só dano") textoQueCabe(ef,r.x+452,meio-9,15,cabe,sel?Color{70,58,30,255}:CIANO);
    }
}
// Os detalhes de um golpe (à esquerda, na caixa de comandos).
void detalhesDoGolpe(const Jogo& jogo,const Batalha* b,const Golpe& g,int pp,float x,float y,bool compacto) {
    const Jogador& j=jogo.jogador;
    float px=compacto?3.0f:4.0f;
    iconeDoElemento(g.elemento,x,y,px);
    float nx=x+9*px+12;
    texto(g.nome,nx,y-2,compacto?22:27,TEXTO);
    float w=larguraDoTexto(g.nome,compacto?22:27);
    etiquetaDeElemento(g.elemento,nx+w+12,y+(compacto?1:4),13);
    // Números em "fichas".
    float cy=y+(compacto?32:44);
    auto ficha=[&](const std::string& t,float& fx,Color c){
        float fw=larguraDoTexto(t,16)+20;
        DrawRectangleRounded({fx,cy,fw,26},0.5f,6,{34,46,72,255});
        texto(t,fx+10,cy+4,16,c);
        fx+=fw+8;
    };
    float fx=x;
    ficha(g.poder>0?"Poder "+std::to_string(g.poder):"Sem dano",fx,g.poder>0?Color{255,205,100,255}:APAGADO);
    ficha("Precisão "+std::to_string(g.precisao)+"%",fx,TEXTO);
    ficha(g.ppMaximo>0?"PP "+std::to_string(pp)+"/"+std::to_string(g.ppMaximo):"PP sem limite",fx,pp==0&&g.ppMaximo>0?PERIGO:TEXTO);
    // Efeito e bônus.
    float ly=cy+36;
    std::string ef=Mochila::efeitoDoGolpe(g);
    if(!ef.empty() && ef!="só dano") { texto("Efeito: "+ef,x,ly,17,CIANO); ly+=26; }
    std::string bonus;
    if(g.poder>0 && g.elemento==j.elemento) bonus="+30%: é do seu elemento";
    if(g.poder>0 && j.arma && j.arma->elemento==g.elemento)
        bonus+=std::string(bonus.empty()?"":"   ")+"+25%: sua arma é de "+NOMES_DOS_ELEMENTOS[g.elemento];
    if(!bonus.empty() && !compacto) { texto(bonus,x,ly,16,{120,230,140,255}); ly+=24; }
    // Contra quem (só no menu de golpes).
    if(b && !compacto && g.poder>0) {
        // Contra cada inimigo: o nome e o resultado na cor certa.
        float cx=x;
        for(int i=0;i<b->numInimigos && cx<x+600;i++) {
            if(!b->naLuta[i]) continue;
            int e=eficacia((Elemento)g.elemento,(Elemento)b->inimigos[i].elemento);
            const char* t=e==4?"super eficaz":e==1?"pouco eficaz":e==0?"não afeta":"normal";
            Color c=e==4?Color{90,220,120,255}:e==1?Color{255,150,70,255}:e==0?Color{150,155,170,255}:TEXTO;
            std::string nome="contra "+semArtigo(b->inimigos[i].nome)+": ";
            texto(nome,cx,ly,15,APAGADO); cx+=larguraDoTexto(nome,15);
            texto(t,cx,ly,15,c); cx+=larguraDoTexto(t,15)+22;
        }
    }
}

// Painel de um lutador na batalha: nome, nível, elemento, status e barra.
void painelDoLutador(const Lutador& l,int vidaMostrada,Rectangle r,Color borda,Color corDoNome,bool compacto) {
    painel(r,{18,28,45,255},borda);
    float x=r.x+18;
    std::string nome = l.ehJogador ? emMaiusculas(l.nome) : semArtigo(l.nome);
    if(compacto) {
        texto(nome,x,r.y+8,18,corDoNome);
        texto("Nv "+std::to_string(l.nivel),x,r.y+34,16,APAGADO);
        etiquetaDeElemento(l.elemento,x+60,r.y+32,13);
        barra(x,r.y+64,r.width-36,11,vidaMostrada,l.vidaMaxima,PERIGO);
        texto(std::to_string(std::max(0,vidaMostrada))+"/"+std::to_string(l.vidaMaxima),x,r.y+80,15,TEXTO);
        etiquetaDeStatus(l.status,x+70,r.y+79);
        return;
    }
    texto(nome+"  |  Nv "+std::to_string(l.nivel),x,r.y+12,22,corDoNome);
    texto("Vida "+std::to_string(std::max(0,vidaMostrada))+" / "+std::to_string(l.vidaMaxima),x,r.y+44,20,TEXTO);
    float w = etiquetaDeElemento(l.elemento,r.x+r.width-150,r.y+42,15);
    (void)w;
    etiquetaDeStatus(l.status,r.x+r.width-290,r.y+43);
    barra(x,r.y+76,r.width-40,13,vidaMostrada,l.vidaMaxima,l.ehJogador?VIDA:PERIGO);
}
// ---------------------------------------------------------------------------
// ANIMAÇÃO DA BATALHA
// Cada lutador (0 = jogador, 1 a 3 = inimigos) tem um deslocamento e uma
// transparência que as animações mudam quadro a quadro.
// ---------------------------------------------------------------------------
struct AnimacaoDoLutador { float dx, dy, alfa; };
AnimacaoDoLutador animacao[1+MAX_INIMIGOS] = {{0,0,1},{0,0,1},{0,0,1},{0,0,1}};
bool desmaiado[1+MAX_INIMIGOS] = {false,false,false,false};

// v1.24: numa luta de chefe (ou mini-chefe) com ajudantes, ele fica no meio
// e maior; os ajudantes, menores, dos lados.
int slotDoChefe(const Batalha& b) {
    if(b.numInimigos<2) return -1;
    for(int i=0;i<b.numInimigos;i++) if(b.inimigos[i].ehChefe || b.inimigos[i].ehMiniChefe) return i;
    return -1;
}
// Em que lugar da tela (0 = esquerda, 1 = meio, 2 = direita) fica o inimigo i.
int lugarNaTela(const Batalha& b,int i) {
    int c=slotDoChefe(b);
    if(c<0) return i;
    if(i==c) return 1;
    int k=0;
    for(int j=0;j<b.numInimigos;j++) { if(j==c) continue; if(j==i) return k==0?0:2; k++; }
    return i;
}
// Onde o sprite de cada lutador é desenhado (sem a animação).
void posicaoDoSprite(const Batalha& b,int quem,float& x,float& y,float& pixel) {
    if(quem==0) { x=200; y=267; pixel=12.2f; return; }
    if(b.numInimigos==1) { x=892; y=257; pixel=12.3f; return; }
    int lugar=lugarNaTela(b,quem-1);
    if(slotDoChefe(b)>=0) {
        if(lugar==1) { pixel=11.5f; x=1010-8*pixel; y=462-16*pixel; return; }
        pixel=7.4f; x=(lugar==0?790.0f:1230.0f)-8*pixel; y=470-16*pixel; return;
    }
    x=735.0f+lugar*211; y=280; pixel=8.5f;
}
// O desenho de cada inimigo na luta (o Golem muda na segunda fase).
const Sprite& spriteNaBatalha(const Batalha& b,int i) {
    const Monstro& m=*b.monstros[i];
    if(tipoDe(m).simbolo=='L' && b.golemQuebrou) return GOLEM_MAGMA;
    return imagemMonstro(m.tipo);
}
Vector2 centroDoSprite(const Batalha& b,int quem) {
    float x,y,p; posicaoDoSprite(b,quem,x,y,p);
    return {x+8*p,y+8*p};
}
// Ícone animado do status sobre a cabeça do lutador.
void iconeDeStatus(int status,float cx,float topo) {
    float t=(float)GetTime();
    if(status==ENVENENADO) {
        for(int k=0;k<3;k++) {
            float fase=std::fmod(t*0.8f+k/3.0f,1.0f);
            Color c={190,100,230,(unsigned char)(255*(1-fase))};
            DrawCircleLines((int)(cx-14+k*14+std::sin(fase*6+k)*5),(int)(topo-fase*34),5+fase*3,c);
        }
    } else if(status==ADORMECIDO) {
        for(int k=0;k<2;k++) {
            float fase=std::fmod(t*0.6f+k*0.5f,1.0f);
            Color c={150,175,255,(unsigned char)(255*(1-fase))};
            textoCentro("Z",cx+10+fase*22,topo-fase*36,16+fase*14,c);
        }
    } else if(status==PARALISADO) {
        if(((int)(t*8))%2==0) {
            Color c={255,226,90,255};
            DrawLineEx({cx-24,topo-2},{cx-14,topo-14},3,c); DrawLineEx({cx-14,topo-14},{cx-18,topo-4},3,c);
            DrawLineEx({cx+16,topo-6},{cx+26,topo-18},3,c); DrawLineEx({cx+26,topo-18},{cx+22,topo-8},3,c);
        }
    }
}
// Desenha o sprite de um lutador com a animação dele.
void spriteDoLutador(const Batalha& b,int quem,const Sprite& arte) {
    if(desmaiado[quem]) return;
    float x,y,p; posicaoDoSprite(b,quem,x,y,p);
    const AnimacaoDoLutador& an=animacao[quem];
    desenharSprite(arte,x+an.dx,y+an.dy,p,1.0f,an.alfa);
    const Lutador& l=(quem==0)?b.jogador:b.inimigos[quem-1];
    if(l.ehCampeao) coroinha(x+8*p+an.dx,y+an.dy+2.2f*p,p*0.6f);   // coroa na cabeça
    iconeDeStatus(l.status,x+8*p+an.dx,y+an.dy-6);
    GolpesVisuais::desenharEstado(l.status,l.semente,{x+8*p+an.dx,y+8*p+an.dy},8*p);   // estados da v1.23
}
// Escudo erguido (golpe Escudo): um escudo azulado brilhando na frente do herói.
void escudoErguidoNaBatalha(const Batalha& b) {
    Vector2 c=centroDoSprite(b,0);
    float t=(float)GetTime();
    float x=c.x+62+animacao[0].dx, y=c.y-6+animacao[0].dy;
    Color luz={150,200,255,(unsigned char)(70+30*std::sin(t*4))}; Color fora=luz; fora.a=0;
    DrawCircleGradient((int)x,(int)y,58,luz,fora);
    // Contorno do escudo: retângulo em cima e ponta embaixo.
    Color borda={215,235,255,235}, dentro={110,150,210,150};
    DrawRectangleRounded({x-24,y-34,48,40},0.25f,6,dentro);
    DrawTriangle({x-24,y+5},{x,y+38},{x+24,y+5},dentro);
    DrawRectangleRoundedLinesEx({x-24,y-34,48,40},0.25f,6,3,borda);
    DrawLineEx({x-24,y+5},{x,y+38},3,borda);
    DrawLineEx({x,y+38},{x+24,y+5},3,borda);
    DrawLineEx({x,y-28},{x,y+30},2,{215,235,255,140});
    DrawLineEx({x-18,y-10},{x+18,y-10},2,{215,235,255,140});
}
void telaBatalha(const Jogo& jogo,const Batalha& b) {
    titulo("BATALHA   |   Golpes, elementos e turnos");
    painel({39,121,1362,468});
    if(b.salaDaBatalha>=0) {
        // v1.24: a luta na sala do chefe tem o fundo da sala (fundos_batalha.h).
        recortar({42,125,1356,462});
        FundosDeBatalha::desenhar(b.salaDaBatalha,(float)GetTime(),b.chefeFurioso);
        pararDeRecortar();
        DrawEllipse(350,493,275,50,{0,0,0,70});
        DrawEllipse(1010,466,300,56,{0,0,0,70});
    } else {
        DrawRectangleGradientV(42,125,1356,462,{25,40,60,255},{30,29,45,255});
        // Arena estilizada, chão ao redor dos personagens.
        DrawEllipse(350,493,275,50,{44,63,76,255});
        DrawEllipse(1010,466,280,54,{44,63,76,255});
    }
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    spriteDoLutador(b,0,HEROI);
    if(b.escudoErguido) escudoErguidoNaBatalha(b);
    painelDoLutador(b.jogador,b.vidaMostrada[0],{55,140,430,107},CIANO,CIANO,false);
    // Campeões: aura dourada atrás e uma coroa em cima.
    for(int i=0;i<b.numInimigos;i++) {
        if(!b.naLuta[i] || !b.inimigos[i].ehCampeao || desmaiado[i+1]) continue;
        float sx,sy,sp; posicaoDoSprite(b,i+1,sx,sy,sp);
        float cx=sx+8*sp+animacao[i+1].dx, cy=sy+8*sp+animacao[i+1].dy;
        auraDeCampeao(cx,cy,110,(float)GetTime());
    }
    // Chefe furioso (segunda fase): aura vermelha pulsando atrás dele.
    // v1.24: o Golem sem a casca também (o magma brilhando).
    if(b.chefeFurioso || b.golemQuebrou) {
        for(int i=0;i<b.numInimigos;i++) {
            bool golem=b.golemQuebrou && tipoDe(*b.monstros[i]).simbolo=='L';
            if(!b.naLuta[i] || !((b.inimigos[i].ehChefe && b.chefeFurioso) || golem) || desmaiado[i+1]) continue;
            Vector2 c=centroDoSprite(b,i+1);
            float t=(float)GetTime();
            Color aura={255,40,40,(unsigned char)(130+50*std::sin(t*5))}; Color fora=aura; fora.a=0;
            DrawCircleGradient((int)(c.x+animacao[i+1].dx),(int)(c.y+animacao[i+1].dy),130+8*std::sin(t*5),aura,fora);
            for(int k=0;k<6;k++) {   // brasas subindo
                float fase=std::fmod(t*0.6f+k/6.0f,1.0f);
                float bx=c.x-70+k*28+std::sin(t*2+k)*8, by=c.y+60-fase*170;
                DrawCircleV({bx,by},3*(1-fase)+1,{255,(unsigned char)(120+100*(1-fase)),60,(unsigned char)(220*(1-fase))});
            }
        }
    }
    if(b.numInimigos==1) {
        const Monstro& m=*b.monstros[0];
        (void)m;
        spriteDoLutador(b,1,spriteNaBatalha(b,0));
        painelDoLutador(b.inimigos[0],b.vidaMostrada[1],{909,140,474,106},OURO,OURO,false);
    } else {
        for (int i=0;i<b.numInimigos;i++) {
            if(!b.naLuta[i])continue;
            int px=735+lugarNaTela(b,i)*211;
            spriteDoLutador(b,i+1,spriteNaBatalha(b,i));
            bool alvo=(b.menu==MENU_ALVO && b.alvos[b.cursor]==i);
            if(alvo) {
                // Seta pulando sobre o alvo escolhido.
                float pulo=std::fabs(std::sin((float)GetTime()*6))*8;
                Vector2 c=centroDoSprite(b,i+1);
                DrawTriangle({c.x-12,c.y-88-pulo},{c.x,c.y-72-pulo},{c.x+12,c.y-88-pulo},OURO);
            }
            painelDoLutador(b.inimigos[i],b.vidaMostrada[i+1],{(float)px-20,138,204,118},alvo?OURO:CIANO,OURO,true);
        }
    }
    painel({39,608,1362,243},{26,36,57,255});
    texto("COMANDOS",62,625,19,OURO);
    if(b.tipoDoTexto!=MSG_NORMAL && b.menu==MENU_NENHUM) {
        // Mensagem importante: faixa colorida com ícone; a borda pisca no começo.
        Color cor = b.tipoDoTexto==MSG_BOA ? OURO : PERIGO;
        float t = (float)(relogioMs()-b.textoDesde)/1000.0f;
        float brilho = t<0.6f ? 0.5f+0.5f*std::fabs(std::sin(t*16.0f)) : 1.0f;
        // Texto comprido (as dicas): letra um pouco menor, para caber.
        float tam=30, passo=38;
        std::vector<std::string> linhas=quebrarLinhas(b.texto,tam,1250);
        if(linhas.size()>2) { tam=25; passo=31; linhas=quebrarLinhas(b.texto,tam,1250); }
        Rectangle caixa={52,646,1336,62.0f+passo*(linhas.size()-1)};
        DrawRectangleRounded(caixa,0.3f,8,multiplicar(cor,0.22f));
        DrawRectangleRoundedLinesEx(caixa,0.3f,8,3.0f,multiplicar(cor,brilho));
        if(b.tipoDoTexto==MSG_BOA) DrawPoly({86,677},4,13,45,cor);
        else { DrawCircle(86,677,14,cor); textoCentro("!",86,663,24,FUNDO); }
        for(size_t i=0;i<linhas.size();i++) texto(linhas[i],112,661+passo*i,tam,cor);
        if(b.travado) {
            // Barrinha que esvazia enquanto as teclas estão travadas.
            float resta = 1.0f - (float)(relogioMs()-b.textoDesde)/tempoDeLeituraMs();
            DrawRectangle(1070,782,(int)(240*std::max(0.0f,resta)),6,cor);
        }
    } else {
        // Com menu aberto, o texto divide a caixa com os botões da direita.
        float largura = b.menu==MENU_NENHUM ? 1300 : (b.menu==MENU_STATUS ? 480 : 660);
        // v1.27: no menu de golpes, a esquerda mostra os detalhes do golpe marcado.
        if(b.menu==MENU_GOLPES && b.cursor>=0 && b.cursor<jogo.jogador.numGolpes)
            detalhesDoGolpe(jogo,&b,GOLPES[jogo.jogador.golpes[b.cursor]],jogo.jogador.pp[b.cursor],64,660,false);
        else if(b.menu==MENU_ESQUECER) {
            // v1.27: o golpe novo em detalhes e o pedido de escolha embaixo.
            texto("NOVO GOLPE",64,656,13,CIANO);
            const Golpe& novo=GOLPES[b.golpeNovo];
            detalhesDoGolpe(jogo,&b,novo,novo.ppMaximo,64,680,true);
            texto("Já sabe 4 golpes: escolha um para esquecer  »",64,780,17,{255,205,100,255});
        }
        else textoQuebrado(b.texto,62,660,28,largura,36,TEXTO);
    }
    if(b.menu==MENU_PRINCIPAL) {
        const char* nomes[]={"LUTAR", "ITENS", "STATUS", "FUGIR"};
        for(int i=0;i<4;i++) {
            int col=i%2,row=i/2;
            faixaBotao(nomes[i],{757.0f+col*302,646.0f+row*76,275,61},b.cursor==i,i,b.cursor);
        }
        int total=0;for(int i=0;i<NUM_TIPOS_DE_ITEM;i++)if(TIPOS_DE_ITEM[i].categoria==POCAO)total+=jogo.jogador.mochila[i];
        texto("Poções: "+std::to_string(total),63,752,22,APAGADO);
    } else if(b.menu==MENU_GOLPES || b.menu==MENU_ESQUECER) {
        int n=(b.menu==MENU_ESQUECER)?MAX_GOLPES+1:b.jogador.numGolpes;
        for(int i=0;i<n;i++) {
            Rectangle r={740.0f,625.0f+i*41,631,37};
            if(i<jogo.jogador.numGolpes)
                linhaDoGolpe(b.menu==MENU_GOLPES?&b:nullptr,GOLPES[jogo.jogador.golpes[i]],jogo.jogador.pp[i],r,b.cursor==i,i,b.cursor);
            else faixaBotao("Não aprender",r,b.cursor==i,i,b.cursor,20);
        }

    } else if(b.menu==MENU_ITENS) {
        for(int i=0;i<b.numItensUsaveis;i++) {
            int id=b.itensUsaveis[i];
            faixaBotao(std::string(TIPOS_DE_ITEM[id].nome)+"  x"+std::to_string(jogo.jogador.mochila[id]),
                {740.0f,630.0f+i*40,631,36},b.cursor==i,i,b.cursor,20,ARTE_DOS_ITENS[id]);
        }
    } else if(b.menu==MENU_ALVO) {
        for(int i=0;i<b.numAlvos;i++) {
            int id=b.alvos[i];
            Rectangle r={740.0f,630.0f+i*49,631,44};
            bool escuro=faixaBotao(semArtigo(b.inimigos[id].nome),r,b.cursor==i,i,b.cursor);
            // v1.27: o golpe escolhido contra este alvo.
            if(b.golpeEscolhido>=0 && b.golpeEscolhido<NUM_GOLPES) {
                const Golpe& g=GOLPES[b.golpeEscolhido];
                etiquetaDeElemento(b.inimigos[id].elemento,r.x+260,r.y+12,13);
                int e=eficacia((Elemento)g.elemento,(Elemento)b.inimigos[id].elemento);
                if(e==2) texto("normal",r.x+r.width-150,r.y+12,15,escuro?FUNDO:APAGADO);
                else seloDeEficacia(e,r.x+r.width-150,r.y+10,14,escuro);
            }
        }
    } else if(b.menu==MENU_STATUS) {
        const Lutador& eu=b.jogador;
        texto("ATQ "+std::to_string(eu.ataque)+"   DEF "+std::to_string(eu.defesa)+"   VEL "+std::to_string(eu.velocidade)+
              "   Precisão +"+std::to_string(eu.precisaoExtra)+"%   Esquiva "+std::to_string(eu.esquiva)+"%",560,640,19,CIANO);
        int linha=0;
        for(int i=0;i<b.numInimigos;i++) {
            if(!b.naLuta[i]) continue;
            const Lutador& l=b.inimigos[i];
            texto(semArtigo(l.nome)+" ("+NOMES_DOS_ELEMENTOS[l.elemento]+")  fraco contra: "+elementosCom(l.elemento,4)+
                  "   resiste a: "+elementosCom(l.elemento,1),560,676+linha*30,18,TEXTO);
            linha++;
        }
        texto("Aperte uma tecla para voltar",560,778,18,APAGADO);
    } else if(b.menu==MENU_NENHUM) {
        if(b.passaSozinhoEm>0) {
            // Batalha automática: a barrinha enche até a frase passar sozinha.
            float total=(float)(b.passaSozinhoEm-b.textoDesde), foi=(float)(relogioMs()-b.textoDesde);
            float f=total>0 ? std::clamp(foi/total,0.0f,1.0f) : 1.0f;
            DrawRectangle(1070,796,240,4,{45,60,85,255});
            DrawRectangle(1070,796,(int)(240*f),4,CIANO);
            texto("ENTER adianta",1070,770,19,APAGADO);
        } else if(b.passaSozinhoEm<0 && !b.travado) texto("ENTER para continuar",1070,770,19,CIANO);
    }
    texto("Setas: escolher     Enter: confirmar     Esc: voltar",62,811,19,APAGADO);
    // Partículas e números da batalha por cima de tudo.
    Efeitos::desenharParticulas(Efeitos::CAMADA_TELA);
    Efeitos::desenharTextosFlutuantes(Efeitos::CAMADA_TELA);
}
float suavizar(float t) { t=std::clamp(t,0.0f,1.0f); return 1-(1-t)*(1-t)*(1-t); }   // começa rápido, termina devagar
}

void posicaoDaCamera(const Jogo& jogo,int& cameraX,int& cameraY) {
    cameraX=std::clamp(jogo.jogador.x-CAMERA_LARGURA/2,0,MAPA_LARGURA-CAMERA_LARGURA);
    cameraY=std::clamp(jogo.jogador.y-CAMERA_ALTURA/2,0,MAPA_ALTURA-CAMERA_ALTURA);
}
void forcarLimpezaDaTela() {}
void mudarZoomDoMapa(int passo) {
    int antes=opcoes().zoomDoMapa;
    opcoes().zoomDoMapa=std::clamp(antes+passo,0,2);
    zoomMudouEm=GetTime();
    if(opcoes().zoomDoMapa!=antes) { salvarOpcoes(); tocarSom(SOM_MENU); }
}
// ---- Sons do mapa (v1.21): ambiente, salas especiais, passos e furtivo ----
// Tudo aqui só olha o jogo e toca sons (a lógica não muda; os bots não ouvem).
void sonsDoMapa(const Jogo& jogo) {
    static int andarAntes=-1, monstrosAntes=-1, jxAntes=-1, jyAntes=-1;
    static bool furtivoAntes=false;
    static int mxAntes[MAX_MONSTROS], myAntes[MAX_MONSTROS];
    const Andar& a=andarAtual(jogo);
    const Jogador& j=jogo.jogador;
    // O som do andar, conforme o tema.
    static const Ambiente DO_TEMA[NUM_TEMAS]={AMBIENTE_MINAS,AMBIENTE_CAVERNA,AMBIENTE_CRIPTA,AMBIENTE_VULCAO,AMBIENTE_TRONO};
    tocarAmbiente(DO_TEMA[temaDoAndar(jogo.andar)]);
    // Fonte e altar: mais altos quanto mais perto (até umas 9 casas).
    float fonte=0, altar=0;
    for(int i=0;i<a.numEspeciais;i++) {
        const Especial& e=a.especiais[i];
        if(e.usado || (e.tipo!=ESP_FONTE && e.tipo!=ESP_ALTAR)) continue;
        float d=std::hypot((float)(e.x-j.x),(float)(e.y-j.y));
        float v=std::clamp(1.0f-d/9.0f,0.0f,1.0f); v*=v;
        if(e.tipo==ESP_FONTE) fonte=std::max(fonte,v); else altar=std::max(altar,v);
    }
    volumeDasSalas(fonte,altar);
    // Andar novo (ou partida nova): só guarda onde cada um está.
    if(jogo.andar!=andarAntes || a.numMonstros!=monstrosAntes) {
        andarAntes=jogo.andar; monstrosAntes=a.numMonstros; jxAntes=j.x; jyAntes=j.y; furtivoAntes=jogo.furtivo;
        for(int i=0;i<a.numMonstros;i++) { mxAntes[i]=a.monstros[i].x; myAntes[i]=a.monstros[i].y; }
        return;
    }
    // Passos do herói: o som muda com o chão.
    if(j.x!=jxAntes || j.y!=jyAntes) {
        TipoCelula chao=a.mapa.celulas[j.y][j.x];
        Passo p = jogo.furtivo ? PASSO_FURTIVO : chao==POCA ? PASSO_AGUA
                : (noTapete(a,j.x,j.y) || a.zonaSegura[j.y][j.x]) ? PASSO_TAPETE : PASSO_PEDRA;
        tocarPasso(p,0.8f);
        bool estavaNaAgua = jxAntes>=0 && a.mapa.celulas[jyAntes][jxAntes]==POCA;
        if(chao==POCA && !estavaNaAgua) tocarSom(SOM_AGUA);
        jxAntes=j.x; jyAntes=j.y;
    }
    if(jogo.furtivo!=furtivoAntes) { tocarSom(SOM_FURTIVO); furtivoAntes=jogo.furtivo; }
    // Passos dos monstros: ouvidos até 8 casas, até através das paredes (mais
    // baixos quando não estão à vista). A sombra não faz barulho.
    for(int i=0;i<a.numMonstros;i++) {
        const Monstro& m=a.monstros[i];
        bool andou = m.x!=mxAntes[i] || m.y!=myAntes[i];
        mxAntes[i]=m.x; myAntes[i]=m.y;
        if(!andou || !m.vivo || m.estado==DORMINDO) continue;
        Passo p=passoDoMonstro(tipoDe(m).simbolo);
        if(p==SEM_PASSO) continue;
        int d=std::max(std::abs(m.x-j.x),std::abs(m.y-j.y));
        if(d>8) continue;
        bool aVista=monstroAVista(jogo,m);
        float v=std::pow(1.0f-d/9.0f,1.5f)*(aVista?1.0f:0.6f);
        tocarPasso(p,v);
        // O ogro e o chefe fazem o chão tremer quando estão perto.
        if(p==PASSO_OGRO && aVista && d<=4) Efeitos::tremer(1.6f,0.12);
        if(p==PASSO_CHEFE && d<=6) Efeitos::tremer(3.0f,0.2);
    }
}

void desenharJogo(const Jogo& jogo) {
    ultimoJogo=&jogo;
    sonsDoMapa(jogo);
    // Roda do mouse: zoom do mapa (as teclas + e - chegam pelo jogo.cpp).
    float roda=GetMouseWheelMove();
    if(roda>0.1f) mudarZoomDoMapa(+1);
    if(roda<-0.1f) mudarZoomDoMapa(-1);
    Efeitos::trocarDeTela(TELA_MAPA);
    definirRedesenho([&jogo]{ desenharJogo(jogo); });
    Ui::iniciarQuadro();mapa(jogo);Ui::terminarQuadro();
}
void desenharBatalha(const Jogo& jogo,const Batalha& b) {
    Efeitos::trocarDeTela(TELA_BATALHA);
    tocarAmbiente(SEM_AMBIENTE);   // na batalha, só a música
    volumeDasSalas(0,0);
    definirRedesenho([&jogo,b]{ desenharBatalha(jogo,b); });   // cópia da batalha
    Ui::iniciarQuadro();telaBatalha(jogo,b);Ui::terminarQuadro();
}

// ---------------------------------------------------------------------------
// ANIMAÇÕES DA BATALHA (chamadas pela lógica em batalha.cpp)
// Cada uma roda por uma fração de segundo, desenhando a batalha quadro a
// quadro com os lutadores deslocados, e depois devolve o controle.
// ---------------------------------------------------------------------------
void animarBatalha(const Jogo& jogo,const Batalha& b,int tipo,int quem,int alvo,int elemento,int valor,bool critico,int golpe) {
    if(jogo.modoAutomatico) return;
    double duracao=0.5;
    // v1.23: cada tipo de golpe tem o seu desenho e o seu tempo.
    int forma = golpe>=0 ? GolpesVisuais::formaDoGolpe(golpe) : GolpesVisuais::F_IMPACTO;
    bool encosta = GolpesVisuais::encosta(forma);
    float acerto = golpe>=0 ? GolpesVisuais::momentoDoAcerto(forma) : 0.25f;
    switch(tipo) {
        case ANIM_ENTRADA: duracao=0.65; break;
        case ANIM_GOLPE:   duracao=golpe>=0 ? GolpesVisuais::duracaoDaForma(forma) : 0.55; break;
        case ANIM_ERRO:    duracao=golpe>=0 ? GolpesVisuais::duracaoDaForma(forma) : 0.5; break;
        case ANIM_BLOQUEIO: duracao=0.6; break;
        case ANIM_FURIA:   duracao=1.1;  break;
        case ANIM_FUMACA:  duracao=1.0;  break;
        case ANIM_DESMAIO: duracao=0.6;  break;
        case ANIM_NIVEL:   duracao=0.8;  break;
        case ANIM_CHEGADA: duracao=0.7;  break;
    }
    if(opcoes().animacaoRapida) duracao*=0.5;   // Opções: animações rápidas
    if(tipo==ANIM_ENTRADA) {
        Efeitos::limparParticulas(Efeitos::CAMADA_TELA);
        for(int i=0;i<=MAX_INIMIGOS;i++) desmaiado[i]=false;
        Efeitos::piscar(WHITE,0.35);
    }
    Vector2 centroAlvo=centroDoSprite(b,alvo), centroQuem=centroDoSprite(b,quem);
    float direcao=(quem==0)?1.0f:-1.0f;   // o jogador ataca para a direita
    bool disparou=false;
    double inicio=GetTime();
    while(true) {
        float t=(float)((GetTime()-inicio)/duracao);
        if(t>=1) break;
        for(int i=0;i<=MAX_INIMIGOS;i++) animacao[i]={0,0,1};
        switch(tipo) {
        case ANIM_ENTRADA:
            // O herói entra pela esquerda e os monstros pela direita.
            animacao[0].dx=-420*(1-suavizar(t));
            for(int i=1;i<=MAX_INIMIGOS;i++) animacao[i].dx=520*(1-suavizar(t));
            break;
        case ANIM_GOLPE: case ANIM_ERRO: {
            // Bote: quem ataca avança e volta (só nos golpes de contato).
            if(t<0.4f && encosta) animacao[quem].dx=direcao*70*std::sin(3.14159f*t/0.4f);
            if(t<0.3f && !encosta) animacao[quem].dy=-8*std::sin(3.14159f*t/0.3f);   // pulinho ao lançar
            if(t>=acerto && !disparou) {
                disparou=true;
                if(tipo==ANIM_ERRO) {
                    Efeitos::textoFlutuante("ERROU",centroAlvo.x,centroAlvo.y-70,{190,195,205,255},30,Efeitos::CAMADA_TELA);
                } else {
                    Efeitos::explosaoDoElemento(elemento,centroAlvo.x,centroAlvo.y,Efeitos::CAMADA_TELA,critico?1.6f:1.0f);
                    if(valor>0) {
                        Color cor=critico?OURO:(alvo==0?Color{255,120,120,255}:WHITE);   // dano no jogador fica avermelhado
                        Efeitos::textoFlutuante("-"+std::to_string(valor)+(critico?"!":""),centroAlvo.x,centroAlvo.y-70,
                                                cor,critico?50:36,Efeitos::CAMADA_TELA,1.0);
                    }
                    if(critico) { Efeitos::tremer(10,0.35); Efeitos::piscar(WHITE,0.12); }
                    else if(valor>0) Efeitos::tremer(golpe==G_TERREMOTO?9:3,golpe==G_TERREMOTO?0.4:0.15);
                }
            }
            if(t>=acerto) {
                float u=(t-acerto)/(1-acerto);
                if(tipo==ANIM_ERRO) {
                    // O alvo dá um pulinho para o lado.
                    animacao[alvo].dx=-direcao*30*std::sin(3.14159f*std::min(1.0f,u*1.5f));
                    animacao[alvo].dy=-24*std::sin(3.14159f*std::min(1.0f,u*1.5f));
                } else if(valor>0) {
                    // O alvo treme e pisca.
                    animacao[alvo].dx=std::sin(u*60)*6*(1-u);
                    animacao[alvo].alfa=(((int)(u*duracao*1000/70))%2==0)?0.3f:1.0f;
                }
            }
            break;
        }
        case ANIM_BLOQUEIO: {
            // O monstro dá o bote, bate no escudo e é jogado para trás.
            if(t<0.25f) animacao[quem].dx=direcao*70*std::sin(3.14159f*t/0.5f);
            if(t>=0.25f && !disparou) {
                disparou=true;
                float frente=centroAlvo.x-direcao*58;   // o escudo fica na frente do herói
                Efeitos::brilhoSubindo(frente,centroAlvo.y,{200,225,255,255},22,Efeitos::CAMADA_TELA);
                Efeitos::textoFlutuante("BLOQUEOU!",centroAlvo.x,centroAlvo.y-80,{170,215,255,255},36,Efeitos::CAMADA_TELA,1.0);
                Efeitos::tremer(4,0.15);
                Efeitos::piscar({200,225,255,255},0.08);
            }
            if(t>=0.25f) {
                float u=(t-0.25f)/0.75f;
                animacao[quem].dx=direcao*(70*(1-suavizar(u*1.4f))-30*std::sin(3.14159f*u));   // recua além do lugar e volta
                animacao[alvo].dx=-direcao*6*std::sin(3.14159f*std::min(1.0f,u*3));   // o herói segura o tranco
            }
            break;
        }
        case ANIM_FUMACA:
            // Bomba de fumaça: nuvens cinzentas cobrem a arena e o herói some.
            if(!disparou) {
                disparou=true;
                // Nuvem: muitas bolas grandes e bem juntas, saindo do herói
                // e se espalhando pela arena (no fim parecem uma névoa só).
                Vector2 h=centroDoSprite(b,0);
                for(int k=0;k<140;k++) {
                    float ang=k*2.39996f, raio=(k%7)*14.0f;
                    float x=h.x+std::cos(ang)*raio, y=h.y+std::sin(ang)*raio*0.6f;
                    float vx=std::cos(ang)*(140+(k%5)*70), vy=std::sin(ang)*(60+(k%4)*25)-25;
                    unsigned char cinza=(unsigned char)(135+(k*13)%55);
                    Efeitos::criarParticula({x,y,vx,vy,0,{cinza,cinza,(unsigned char)(cinza+12),150},
                                             44+(k%6)*9.0f,GetTime()+(k%14)*0.02,1.25,Efeitos::BOLINHA,0,Efeitos::CAMADA_TELA});
                }
                Efeitos::piscar({200,205,215,255},0.15);
            }
            animacao[0].alfa=1.0f-suavizar(std::min(1.0f,t*1.4f));   // o herói some na fumaça
            break;
        case ANIM_CHEGADA:
            // v1.24: quem entra no meio da luta desce do alto e aparece aos poucos.
            if(!disparou) {
                disparou=true;
                desmaiado[quem]=false;
                Efeitos::brilhoSubindo(centroQuem.x,centroQuem.y+30,{220,220,235,255},18,Efeitos::CAMADA_TELA);
            }
            animacao[quem].dy=-70*(1-suavizar(t));
            animacao[quem].alfa=suavizar(std::min(1.0f,t*1.6f));
            break;
        case ANIM_FURIA:
            // O chefe treme, fica vermelho e solta faíscas; a tela pisca.
            if(!disparou) {
                disparou=true;
                bool golem=b.golemQuebrou && quem>0 && tipoDe(*b.monstros[quem-1]).simbolo=='L';
                Efeitos::piscar(golem?Color{255,120,30,255}:Color{200,20,30,255},0.15);
                Efeitos::tremer(12,0.7);
                Efeitos::brilhoSubindo(centroQuem.x,centroQuem.y,golem?Color{255,140,40,255}:Color{255,60,50,255},40,Efeitos::CAMADA_TELA);
                Efeitos::textoFlutuante(golem?"MAGMA!":"FÚRIA!",centroQuem.x,centroQuem.y-110,golem?Color{255,160,60,255}:Color{255,90,80,255},48,Efeitos::CAMADA_TELA,1.3);
            }
            animacao[quem].dx=std::sin(t*70)*7*(1-t);
            animacao[quem].dy=-10*std::sin(3.14159f*t);
            break;
        case ANIM_EFEITO_PROPRIO:
            if(!disparou) {
                disparou=true;
                Efeitos::brilhoSubindo(centroQuem.x,centroQuem.y,elemento==0?CIANO:corDoElemento(elemento),18,Efeitos::CAMADA_TELA);
            }
            animacao[quem].dy=-12*std::sin(3.14159f*t);
            break;
        case ANIM_CURA:
            if(!disparou) {
                disparou=true;
                Efeitos::brilhoSubindo(centroQuem.x,centroQuem.y,{120,230,140,255},20,Efeitos::CAMADA_TELA);
                if(valor>0) Efeitos::textoFlutuante("+"+std::to_string(valor),centroQuem.x,centroQuem.y-70,
                                                    {120,230,140,255},36,Efeitos::CAMADA_TELA,1.0);
            }
            break;
        case ANIM_DESMAIO:
            // Afunda e some aos poucos.
            animacao[quem].dy=70*suavizar(t);
            animacao[quem].alfa=1-t;
            break;
        case ANIM_NIVEL:
            if(!disparou) {
                disparou=true;
                Efeitos::brilhoSubindo(centroQuem.x,centroQuem.y,OURO,34,Efeitos::CAMADA_TELA);
                Efeitos::textoFlutuante("NÍVEL "+std::to_string(valor)+"!",centroQuem.x,centroQuem.y-90,OURO,42,
                                        Efeitos::CAMADA_TELA,1.3);
            }
            animacao[quem].dy=-14*std::fabs(std::sin(3.14159f*t*2));   // dois pulinhos de alegria
            break;
        }
        Ui::iniciarQuadro();telaBatalha(jogo,b);
        // O desenho do golpe por cima da cena (sai um pouco à frente de quem ataca).
        if((tipo==ANIM_GOLPE || tipo==ANIM_ERRO) && golpe>=0) {
            float x,y,p; posicaoDoSprite(b,alvo,x,y,p);
            Vector2 origem={centroQuem.x+direcao*60+animacao[quem].dx,centroQuem.y-20+animacao[quem].dy};
            GolpesVisuais::desenharGolpe(golpe,origem,centroAlvo,t,8*p);
        }
        Ui::terminarQuadro();
    }
    for(int i=0;i<=MAX_INIMIGOS;i++) animacao[i]={0,0,1};
    if(tipo==ANIM_FUMACA) animacao[0].alfa=0;   // o herói já sumiu (a batalha vai acabar)
    if(tipo==ANIM_DESMAIO) desmaiado[quem]=true;
}

// ---------------------------------------------------------------------------
// EFEITOS NO MAPA (chamados pela lógica; não travam o jogo)
// ---------------------------------------------------------------------------
void efeitoNoMapa(const Jogo& jogo,int efeito,int x,int y,int valor) {
    if(jogo.modoAutomatico) return;
    float px=x*TAM+TAM/2.0f, py=y*TAM+TAM/2.0f;   // em pixels do mapa (a câmera é somada ao desenhar)
    const int MAPA=Efeitos::CAMADA_MAPA;
    switch(efeito) {
        case EFEITO_ARMADILHA:
            Efeitos::espinhos(px,py,MAPA);
            Efeitos::piscar({230,40,50,255},0.25);
            Efeitos::tremer(6,0.25);
            Efeitos::textoFlutuante("-"+std::to_string(valor),px,py-14,PERIGO,24,MAPA);
            break;
        case EFEITO_ESPINHOS:
            Efeitos::espinhos(px,py,MAPA);
            break;
        case EFEITO_ITEM:
            Efeitos::brilhoSubindo(px,py-10,OURO,8,MAPA);
            if(valor>=0 && valor<NUM_TIPOS_DE_ITEM)
                Efeitos::textoFlutuante(std::string("+ ")+TIPOS_DE_ITEM[valor].nome,px,py-16,CIANO,17,MAPA,1.1);
            break;
        case EFEITO_CAIXA:
            Efeitos::lascas(px,py,MAPA);
            break;
        case EFEITO_ATORDOAR:
            Efeitos::explosaoDoElemento(0,px,py,MAPA,0.6f);
            break;
        case EFEITO_LAVA:
            Efeitos::explosaoDoElemento(1,px,py,MAPA,0.7f);   // chamas
            Efeitos::piscar({255,80,30,255},0.2);
            Efeitos::tremer(4,0.2);
            Efeitos::textoFlutuante("-"+std::to_string(valor),px,py-14,PERIGO,24,MAPA);
            break;
        case EFEITO_MOEDAS:
            Efeitos::brilhoSubindo(px,py-6,OURO,14,MAPA);
            Efeitos::textoFlutuante("+"+std::to_string(valor)+" moedas",px,py-16,OURO,18,MAPA,1.2);
            break;
        case EFEITO_CORNETA:
            // Ondas de som saindo da corneta.
            for(int k=0;k<18;k++) {
                float ang=k*0.349f;
                Efeitos::criarParticula({px,py,std::cos(ang)*150,std::sin(ang)*150,0,{255,210,90,255},7,
                                         GetTime()+(k%3)*0.12,0.8,Efeitos::ANEL,0,MAPA});
            }
            Efeitos::textoFlutuante("TUUUU!",px,py-18,OURO,22,MAPA,1.4);
            Efeitos::tremer(5,0.4);
            break;
        case EFEITO_CURA:
            Efeitos::textoFlutuante("+"+std::to_string(valor),px+6,py-14,{120,230,140,255},17,MAPA,0.8);
            break;
    }
}
void desenharMenuEmCaixa(const std::string& nome,const std::vector<std::string>& opcoes,
                         const std::string& descricao,int cursor) {
    definirRedesenho([nome,opcoes,descricao,cursor]{ desenharMenuEmCaixa(nome,opcoes,descricao,cursor); });
    Ui::iniciarQuadro();if(ultimoJogo)mapa(*ultimoJogo);
    overlayBasico(nome);
    int primeira=std::max(0,cursor-6);
    int n=std::min(12,(int)opcoes.size());
    if(primeira+n>(int)opcoes.size()) primeira=std::max(0,(int)opcoes.size()-n);
    for(int i=primeira;i<primeira+n;i++)
        faixaBotao(opcoes[i],{274.0f,239.0f+(i-primeira)*36,890,32},cursor==i,i,cursor,21,iconeNoTexto(opcoes[i]));
    // Descrição: quebra em duas linhas se for comprida (separadas por " | ").
    size_t corte=descricao.find("  |  ");
    if(corte==std::string::npos) textoQueCabe(descricao,275,656,22,890,APAGADO);
    else {
        textoQueCabe(descricao.substr(0,corte),275,644,22,890,APAGADO);
        textoQueCabe(descricao.substr(corte+5),275,673,20,890,CIANO);
    }
    texto("Setas e Enter: selecionar   |   Esc: voltar",275,712,19,CIANO);
    Ui::terminarQuadro();
}
// Dica (dicas.cpp): caixa no meio do mapa com uma lâmpada, que espera uma tecla.
void desenharDica(const Jogo& jogo,const std::string& tituloDaDica,const std::string& textoDaDica,bool travada) {
    definirRedesenho([&jogo,tituloDaDica,textoDaDica,travada]{ desenharDica(jogo,tituloDaDica,textoDaDica,travada); });
    Ui::iniciarQuadro();mapa(jogo);
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,150});
    std::vector<std::string> linhas=quebrarLinhas(textoDaDica,26,730);
    float altura=190+38.0f*linhas.size();
    Rectangle caixa={330,(Ui::ALTURA-altura)/2-20,780,altura};
    painel(caixa,{26,35,56,255},OURO);
    // Lâmpada acesa.
    float t=(float)GetTime();
    Vector2 l={caixa.x+62,caixa.y+66};
    Color brilho={255,220,120,(unsigned char)(90+40*std::sin(t*3))}; Color semBrilho=brilho; semBrilho.a=0;
    DrawCircleGradient((int)l.x,(int)l.y,40,brilho,semBrilho);
    DrawCircleV(l,17,{255,214,90,255});
    DrawCircleV({l.x-5,l.y-5},5,{255,246,200,255});
    DrawRectangle((int)l.x-8,(int)l.y+14,16,12,{170,176,190,255});
    DrawLine((int)l.x-8,(int)l.y+19,(int)l.x+8,(int)l.y+19,{110,116,130,255});
    texto("DICA",caixa.x+112,caixa.y+30,20,CIANO);
    texto(tituloDaDica,caixa.x+112,caixa.y+54,32,OURO);
    separador(caixa.x+30,caixa.y+108,caixa.width-60);
    for(size_t i=0;i<linhas.size();i++) texto(linhas[i],caixa.x+30,caixa.y+128+38*i,26,TEXTO);
    float rodape=caixa.y+caixa.height-46;
    if(travada) {
        // Barrinha enchendo enquanto as teclas ainda não valem.
        DrawRectangleRounded({caixa.x+30,rodape+10,caixa.width-60,6},1,4,{45,58,84,255});
    } else {
        texto("Aperte qualquer tecla para continuar",caixa.x+30,rodape,21,OURO);
        textoCentro("Dicas: em OPÇÕES",caixa.x+caixa.width-130,rodape+2,18,APAGADO);
    }
    Ui::terminarQuadro();
}
void desenharAjuda(const Jogo& jogo) {
    definirRedesenho([&jogo]{ desenharAjuda(jogo); });
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("COMANDOS DO MASMORRA");
    const std::vector<std::string> linhas={
        "Setas ou WASD: andar     Diagonais: Home, PgUp, End, PgDn (ou o teclado numérico)",
        "Encoste nos monstros para começar a batalha",
        "I: mochila      C: ficha (distribuir pontos)      J: diário      E: bestiário",
        "Z: descansar e recuperar vida      M: mensagens",
        "Tab: mapa inteiro e detalhes (segurado, ou um toque para fixar)   Roda do mouse, + e -: zoom",
        "Shift (segurado) ou F: modo furtivo (lento, silencioso, ataque surpresa)",
        "Caixas: empurre contra um monstro para atordoá-lo; podem ter itens",
        "Espinhos vermelhos: armadilha! Empurre uma caixa em cima",
        "Pessoas com '!': ande até elas para conversar",
        "O chefe final espera no "+std::to_string(NUM_ANDARES)+"o andar. Dele não dá para fugir",
        "Andares 3, 6, 9 e 12: um mini-chefe lacra a escada até cair (dele também não dá para fugir)",
        "O elemento de cada monstro só aparece quando a batalha começa",
        "Semente desta partida: "+std::to_string(jogo.semente)};
    for(size_t i=0;i<linhas.size();i++)texto(linhas[i],263,232+i*37,21,i%2?TEXTO:CIANO);
    texto("Aperte qualquer tecla para retornar",263,700,20,OURO);Ui::terminarQuadro();
}
void desenharHistorico(const Jogo& jogo) {
    definirRedesenho([&jogo]{ desenharHistorico(jogo); });
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("HISTÓRICO");
    size_t n=jogo.historico.size();size_t ini=n>12?n-12:0;
    for(size_t i=ini;i<n;i++)textoQueCabe(jogo.historico[i],263,240+36*(i-ini),23,920,corDaMensagem(jogo.tiposDoHistorico[i]));
    texto("Aperte qualquer tecla para retornar",263,700,20,OURO);Ui::terminarQuadro();
}
void desenharResumoDoFim(const Jogo& jogo);   // v1.26: mais abaixo
void desenharMorte(const Jogo& jogo) {
    Efeitos::trocarDeTela(TELA_MORTE);
    definirRedesenho([&jogo]{ desenharMorte(jogo); });
    Ui::iniciarQuadro(); mapa(jogo); desenharResumoDoFim(jogo); Ui::terminarQuadro();
}
void desenharAviso(const std::string& a,const std::string& b) {
    Efeitos::trocarDeTela(TELA_AVISO);
    definirRedesenho([a,b]{ desenharAviso(a,b); });
    Ui::iniciarQuadro();titulo("AVISO");texto(a,140,270,25,PERIGO);texto(b,140,330,24,TEXTO);Ui::terminarQuadro();
}

// ---------------------------------------------------------------------------
// PÁGINAS DE TEXTO (história, conversas, como jogar, itens, pontuação)
// ---------------------------------------------------------------------------
void desenharPagina(const std::string& tituloDaPagina,const std::vector<std::string>& linhas,
                    const std::string& rodape) {
    // Cada página (pelo título) conta como uma tela diferente.
    int id=TELA_PAGINA; for(char ch:tituloDaPagina) id=id*31+(unsigned char)ch;
    Efeitos::trocarDeTela(id&0x7fffffff);
    definirRedesenho([tituloDaPagina,linhas,rodape]{ desenharPagina(tituloDaPagina,linhas,rodape); });
    Ui::iniciarQuadro();
    fundoEstrelado();
    texto("MASMORRA",145,48,46,OURO);
    separador(145,112,1150);
    painel({140,135,1160,690},{22,32,51,255});
    textoCentro(tituloDaPagina,720,158,32,OURO);
    separador(190,206,1060);
    // Escolhe a maior letra (até 28) em que todas as linhas cabem na página,
    // quebrando as linhas compridas. Linhas vazias ocupam meia altura.
    const float LARGURA_UTIL=1060, ALTURA_UTIL=530;
    struct Linha { std::string texto; bool subtitulo; bool vazia; };
    std::vector<Linha> prontas;
    float tam=28, altura=0;
    for(;;tam-=1) {
        prontas.clear(); altura=0;
        float passo=tam*1.38f;
        for(const std::string& l:linhas) {
            // Linhas em MAIÚSCULAS são subtítulos.
            bool subtitulo = !l.empty() && l==emMaiusculas(l) &&
                             l.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ")!=std::string::npos;
            if(l.empty()) { prontas.push_back({"",false,true}); altura+=passo*0.5f; continue; }
            for(const std::string& parte:quebrarLinhas(l,tam,LARGURA_UTIL)) { prontas.push_back({parte,subtitulo,false}); altura+=passo; }
        }
        if(altura<=ALTURA_UTIL || tam<=16) break;
    }
    float passo=tam*1.38f;
    float y=226+std::max(0.0f,(ALTURA_UTIL-altura)/2.5f);   // texto curto fica mais perto do meio
    for(const Linha& l:prontas) {
        if(l.vazia) { y+=passo*0.5f; continue; }
        texto(l.texto,190,y,tam,l.subtitulo?CIANO:TEXTO);
        y+=passo;
    }
    textoCentro(rodape,720,782,20,OURO);
    Ui::terminarQuadro();
}

// ---------------------------------------------------------------------------
// CRIAÇÃO DO PERSONAGEM
// ---------------------------------------------------------------------------
void desenharCriacao(const Criacao& c) {
    Efeitos::trocarDeTela(TELA_CRIACAO);
    definirRedesenho([c]{ desenharCriacao(c); });
    Ui::iniciarQuadro();
    fundoEstrelado();
    texto("MASMORRA",145,40,54,OURO);
    texto("CRIE SEU PERSONAGEM",152,110,23,CIANO);
    separador(150,150,1140);

    const char* rotulos[NUM_LINHAS_DA_CRIACAO]={"NOME","ELEMENTO","COR","DIFICULDADE","FORÇA","CONSTITUIÇÃO","DESTREZA","AGILIDADE","",""};
    float y=166;
    for(int i=0;i<NUM_LINHAS_DA_CRIACAO;i++) {
        bool sel=(c.linha==i);
        if(i==LINHA_FORCA) {
            icone(ARTE_ICONE_PONTO_LIVRE.data(),156,y+1,1.3f,c.pontosLivres>0?1.0f:0.45f);
            texto("ATRIBUTOS   (pontos livres: "+std::to_string(c.pontosLivres)+")",186,y+2,20,c.pontosLivres>0?OURO:APAGADO);
            y+=32;
        }
        Rectangle r={150,y,560,48};
        // Os dois botões do fim ficam lado a lado.
        if(i==LINHA_ALEATORIO || i==LINHA_COMECAR) {
            r = (i==LINHA_ALEATORIO) ? Rectangle{150,y+8,230,56} : Rectangle{394,y+8,316,56};
            bool sobre=mouseSobre(r);
            painel(r,sel?OURO:(sobre?Color{43,58,88,255}:PAINEL2),sel?OURO:(sobre?CIANO:Color{69,89,110,255}));
            textoCentro(i==LINHA_ALEATORIO?"ALEATÓRIO":"COMEÇAR AVENTURA",r.x+r.width/2,r.y+16,24,sel?FUNDO:TEXTO);
            registrarOpcao(r,i,c.linha);
            continue;
        }
        bool sobre=mouseSobre(r);
        painel(r,sel?Color{58,52,38,255}:(sobre?Color{40,54,82,255}:PAINEL2),sel?OURO:(sobre?CIANO:Color{69,89,110,255}));
        if(i>=LINHA_FORCA && i<=LINHA_AGILIDADE) {
            // Desenho do atributo antes do nome.
            icone(ARTE_DOS_ATRIBUTOS[i-LINHA_FORCA]->data(),r.x+14,r.y+10,1.75f);
            texto(rotulos[i],r.x+50,r.y+12,20,sel?OURO:APAGADO);
        } else texto(rotulos[i],r.x+20,r.y+12,20,sel?OURO:APAGADO);
        float vx=r.x+270;
        std::string valor;
        Color corValor=TEXTO;
        if(i==LINHA_NOME) {
            valor=c.nome;
            if(sel && ((int)(GetTime()*2))%2==0) valor+="_";
            if(c.nome.empty() && !sel) { valor="(digite um nome)"; corValor=APAGADO; }
        } else if(i==LINHA_ELEMENTO) {
            valor=NOMES_DOS_ELEMENTOS[c.elemento]; corValor=corDoElemento(c.elemento);
        } else if(i==LINHA_COR) {
            valor=NOMES_DAS_CORES_DO_JOGADOR[c.cor];
            DrawRectangleRounded({vx+150,r.y+12,40,24},0.4f,6,paraCor(CORES_DO_JOGADOR[c.cor]));
        } else if(i==LINHA_DIFICULDADE) {
            valor=DIFICULDADES[c.dificuldade].nome; corValor=corDaDificuldade(c.dificuldade);
            if(!dificuldadeLiberada(c.dificuldade)) {   // trancado: nome apagado e um cadeado
                corValor=APAGADO;
                float lx=vx+larguraDoTexto(valor,23)+14, ly=r.y+14;
                DrawRing({lx+8,ly+8},4.5f,7.5f,180,360,12,APAGADO);         // a alça
                DrawRectangleRounded({lx,ly+8,16,13},0.25f,4,APAGADO);      // o corpo
                DrawRectangle((int)lx+7,(int)ly+12,2,5,{22,32,51,255});     // o buraco da chave
            }
        } else {
            valor=std::to_string(c.atributos[i-LINHA_FORCA]);
        }
        if(i==LINHA_NOME) texto(valor,vx-30,r.y+11,23,corValor);
        else {
            texto("<",vx-30,r.y+10,24,sel?OURO:APAGADO);
            texto(valor,vx,r.y+11,23,corValor);
            texto(">",vx+205,r.y+10,24,sel?OURO:APAGADO);
            if(sel) {
                registrarAtalho({vx-40,r.y,36,48},TECLA_ESQUERDA);
                registrarAtalho({vx+195,r.y,40,48},TECLA_DIREITA);
            }
        }
        registrarOpcao(r,i,c.linha);
        y+=56;
    }

    // Painel da direita: o herói com a cor escolhida e o que o elemento faz.
    painel({760,172,540,606},{22,32,51,255});
    DrawEllipse(1030,470,150,24,{12,15,28,255});
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[c.cor]));
    desenharSprite(HEROI,910,215,15.0f);
    std::string nome=c.nome.empty()?"Aventureiro":c.nome;
    textoCentro(nome,1030,500,30,OURO);
    Elemento e=(Elemento)c.elemento;
    const Golpe& g=GOLPES[golpeInicialDoElemento(e)];
    texto("Elemento: ",800,550,22,APAGADO);
    texto(NOMES_DOS_ELEMENTOS[e],800+larguraDoTexto("Elemento: ",22),550,22,corDoElemento(e));
    texto("Golpe inicial: "+std::string(g.nome)+" (poder "+std::to_string(g.poder)+")",800,580,22,TEXTO);
    texto("Fraco contra: "+elementosCom(e,4),800,610,22,TEXTO);
    texto("Resiste a: "+elementosCom(e,1),800,640,22,TEXTO);
    std::string dica;
    if(c.linha==LINHA_NOME) dica="Digite o nome (até 12 letras). Backspace apaga.";
    else if(c.linha==LINHA_ELEMENTO) dica=resumoDoElemento((Elemento)c.elemento,true);
    else if(c.linha==LINHA_COR) dica="A cor da roupa do seu personagem.";
    else if(c.linha==LINHA_DIFICULDADE) dica = dificuldadeLiberada(c.dificuldade) ? DIFICULDADES[c.dificuldade].descricao
                                                : "Trancado: vença o Difícil para liberar o modo Impossível.";
    else if(c.linha>=LINHA_FORCA && c.linha<=LINHA_AGILIDADE) dica=EFEITOS_DOS_ATRIBUTOS[c.linha-LINHA_FORCA];
    else if(c.linha==LINHA_ALEATORIO) dica="Sorteia nome, elemento, cor e atributos. Aperte de novo para outro.";
    else dica="Tudo pronto? Enter para descer à masmorra!";
    separador(800,684,460);
    textoQuebrado(dica,800,696,22,480,28,CIANO);

    texto("Setas: escolher e mudar valores   |   Enter: próximo   |   Esc: voltar ao menu",150,830,19,APAGADO);
    Ui::terminarQuadro();
}

// ---------------------------------------------------------------------------
// BESTIÁRIO (tecla E no jogo e botão do menu principal)
// À esquerda, as cartas (silhueta "???" até a espécie aparecer); à direita,
// a ficha da carta escolhida. Duas abas: MONSTROS e PERSONAGENS.
// ---------------------------------------------------------------------------
namespace {
struct Personagem { const Sprite* arte; const char* nome; const char* onde; const char* texto; };
const Personagem PERSONAGENS[] = {
    {&HEROI,"Aventureiro","você",
     "Você escolhe o elemento, a cor e os atributos na criação. Vencendo monstros, sobe de nível e "
     "ganha pontos para distribuir (tecla C)."},
    {&EXPLORADORA,"Eldra, a exploradora","andar 1",
     "Chegou até o quarto andar e voltou com a perna ferida. Ensina como a masmorra funciona, dá uma "
     "poção e manda você procurar o Borin."},
    {&MINEIRO,"Borin, o velho mineiro","andar 4",
     "Cavou estes túneis por quarenta anos. Resmunga muito, mas destranca a sala do tesouro do andar dele."},
    {&ESPIRITO,"Aldric, o espírito guardião","andar 10",
     "O último guardião da mina: segurou o Senhor das Profundezas por cem anos. Cura as suas feridas "
     "antes da descida final."},
    {&COMERCIANTE,"Garro, o comerciante","andares 3, 6, 9 e 12",
     "Vende poções, armas, escudos e itens de suporte, e compra o que você não quer mais."},
    {&VIAJANTE,"Tobias, o viajante perdido","aparece por sorte",
     "Está ferido. Troca um cristal mágico por uma Poção de Vida ou por umas moedas."},
};
const int NUM_PERSONAGENS = (int)(sizeof(PERSONAGENS)/sizeof(PERSONAGENS[0]));

// v1.23: 5 colunas (são 16 espécies na aba dos monstros; os chefes, v1.24, têm aba própria).
const int COLUNAS_DO_BESTIARIO = 5;
Rectangle cartaDoBestiario(int i) {
    int coluna=i%COLUNAS_DO_BESTIARIO, linha=i/COLUNAS_DO_BESTIARIO;
    return {50.0f+coluna*148,122.0f+linha*168,138,158};
}
// v1.24: três abas. Os chefes (o final e os 4 mini-chefes) têm a deles.
const int ABA_MONSTROS=0, ABA_CHEFES=1, ABA_PERSONAGENS=2, NUM_ABAS_DO_BESTIARIO=3;
Rectangle abaDoBestiario(int aba) {
    static const Rectangle ABAS[NUM_ABAS_DO_BESTIARIO]={{380,32,250,48},{642,32,200,48},{854,32,230,48}};
    return ABAS[aba];
}
bool especieDeChefe(int tipo) { return (TIPOS_DE_MONSTRO[tipo].especial & (CHEFE|MINI_CHEFE))!=0; }
// A espécie que vai na carta i da aba (na ordem da tabela de monstros).
int especieDaCarta(int aba,int i) {
    for(int t=0;t<NUM_TIPOS_DE_MONSTRO;t++) {
        if(especieDeChefe(t)!=(aba==ABA_CHEFES)) continue;
        if(i--==0) return t;
    }
    return 0;
}
int cartasDaAba(int aba) {
    if(aba==ABA_PERSONAGENS) return NUM_PERSONAGENS;
    int n=0;
    for(int t=0;t<NUM_TIPOS_DE_MONSTRO;t++) if(especieDeChefe(t)==(aba==ABA_CHEFES)) n++;
    return n;
}
const Rectangle VOLTAR_DO_BESTIARIO={1150,828,240,50};

void cartaoDoBestiario(Rectangle r,bool escolhido) {
    bool sobre=mouseSobre(r);
    DrawRectangleRounded(r,0.12f,8,escolhido?Color{38,52,80,255}:sobre?Color{31,42,65,255}:PAINEL);
    DrawRectangleRoundedLinesEx(r,0.12f,8,escolhido?3.0f:1.5f,escolhido?OURO:sobre?CIANO:Color{65,88,113,255});
}
std::string comMaiusculaInicial(std::string s) {
    if(!s.empty() && s[0]>='a' && s[0]<='z') s[0]=(char)(s[0]-'a'+'A');
    return s;
}
// Barra de um atributo-base (0 a 100) na ficha.
void barraDoAtributo(const char* nome,int valor,float x,float y) {
    texto(nome,x,y,17,APAGADO);
    DrawRectangleRounded({x+110,y+4,250,12},1,4,{28,38,60,255});
    float f=std::clamp(valor/100.0f,0.0f,1.0f);
    DrawRectangleRounded({x+110,y+4,std::max(8.0f,250*f),12},1,4,{110,175,255,255});
    texto(std::to_string(valor),x+372,y,17,TEXTO);
}

// A espécie escolhida "fala" o grito dela (com a voz) ao aparecer na ficha.
double especieEscolhidaEm=0;
size_t letrasDoGritoTocadas=0;
int contadorDoGrito=0;
void fichaDoMonstro(const Jogo* jogo,int tipo) {
    Rectangle p={800,110,590,700};
    painel(p,PAINEL);
    const TipoMonstro& t=TIPOS_DE_MONSTRO[tipo];
    const FichaDoBestiario& f=fichaDoBestiario(tipo);
    float x=p.x+28, y=p.y+24;
    DrawRectangleRounded({x,y,170,170},0.12f,8,{16,22,38,255});
    if(!f.visto) {
        silhueta(imagemMonstro(tipo).data(),x+13,y+12,9.0f,{52,64,88,255});
        texto("???",x+200,y+20,38,APAGADO);
        texto("Ainda não encontrado.",x+200,y+72,20,TEXTO);
        textoQuebrado("Aparece: "+andaresDaEspecie(tipo),x+200,y+102,17,330,24,APAGADO);
        textoQuebrado("Quando ele aparecer na sua frente, a ficha completa fica aqui: o que ele faz, "
                      "os golpes que você já viu e quantos você venceu.",x,y+210,19,530,28,APAGADO);
        return;
    }
    sprite(imagemMonstro(tipo).data(),x+13,y+8,9.0f);
    {
        std::string grito=gritosDaEspecie(t.simbolo)[0];
        Voz voz=vozDoMonstro(t.simbolo);
        bool completo;
        size_t bytes=letrasReveladas(grito,GetTime()-especieEscolhidaEm-0.25,26*ritmoDaVoz(voz),completo);
        if(bytes>letrasDoGritoTocadas) { vozDasLetras(grito,letrasDoGritoTocadas,bytes,voz,contadorDoGrito); letrasDoGritoTocadas=bytes; }
        if(bytes>0) balaozinho(grito.substr(0,bytes),x+120,y+34,1.0f);
    }
    texto(comMaiusculaInicial(t.nome),x+200,y+8,30,OURO);
    texto(andaresDaEspecie(tipo),x+200,y+48,17,CIANO);
    textoQuebrado(descricaoDaEspecie(tipo),x+200,y+76,16,330,22,TEXTO);
    y+=196;

    // Elemento: muda a cada partida.
    if(jogo && enfrentadaNaPartida(*jogo,tipo)) {
        Elemento e=jogo->elementoDaEspecie[tipo];
        texto("Nesta partida:",x,y,18,APAGADO);
        float w=etiquetaDeElemento(e,x+140,y,14);
        texto("fraco contra: "+elementosCom(e,4),x+150+w,y+1,17,TEXTO);
    } else if(jogo) {
        texto("Nesta partida: lute com ele para descobrir o elemento.",x,y,17,APAGADO);
    } else {
        texto("O elemento é sorteado a cada partida.",x,y,17,APAGADO);
    }
    y+=32;
    texto("Já apareceu como:",x,y,18,APAGADO);
    float ex=x+176;
    for(int e=0;e<NUM_ELEMENTOS;e++) if(f.elementosVistos&(1<<e)) ex+=etiquetaDeElemento(e,ex,y,13)+8;
    y+=44;

    // Atributos: só depois de vencer um.
    texto("ATRIBUTOS",x,y,19,OURO);
    y+=30;
    if(f.derrotados>0) {
        barraDoAtributo("Vida",t.baseVida,x,y);
        barraDoAtributo("Ataque",t.baseAtaque,x,y+26);
        barraDoAtributo("Defesa",t.baseDefesa,x,y+52);
        barraDoAtributo("Velocidade",t.baseVelocidade,x,y+78);
        y+=112;
    } else {
        texto("Vença um para descobrir.",x,y,17,APAGADO);
        y+=36;
    }

    // Golpes: cada um aparece depois que o monstro o usa.
    texto("GOLPES",x,y,19,OURO);
    y+=30;
    int vistos=0;
    for(int g=0;g<NUM_GOLPES;g++) {
        if(!f.golpesVistos.test(g)) continue;
        float w=etiquetaDeElemento(GOLPES[g].elemento,x,y,12);
        texto(GOLPES[g].nome,x+w+10,y,17,TEXTO);
        y+=26;
        vistos++;
    }
    if(vistos==0) texto("??? (cada golpe aparece aqui quando ele usar na batalha)",x,y,17,APAGADO);
    else for(int k=vistos;k<t.numGolpes;k++) { texto("???",x,y,17,APAGADO); y+=26; }

    // Contagem.
    float yb=p.y+p.height-50;
    DrawLineEx({p.x+20,yb-12},{p.x+p.width-20,yb-12},1.5f,{65,88,113,255});
    texto("Derrotados: "+std::to_string(f.derrotados),x,yb,20,TEXTO);
    if(!(t.especial&CHEFE) && t.frequencia>0) {
        coroinha(x+262,yb+17,1.6f);
        texto("Campeões: "+std::to_string(f.campeoesDerrotados),x+274,yb,20,f.campeoesDerrotados>0?OURO:TEXTO);
    }
}

void fichaDoPersonagem(int i) {
    Rectangle p={800,110,590,700};
    painel(p,PAINEL);
    const Personagem& c=PERSONAGENS[i];
    float x=p.x+28, y=p.y+24;
    DrawRectangleRounded({x,y,170,170},0.12f,8,{16,22,38,255});
    sprite(c.arte->data(),x+13,y+8,9.0f);
    textoQuebrado(c.nome,x+200,y+8,28,330,34,OURO);
    texto(c.onde,x+200,y+86,18,CIANO);
    textoQuebrado(c.texto,x,y+210,21,530,30,TEXTO);
}

void desenharBestiario(const Jogo* jogo,int aba,int escolhido) {
    fundoEstrelado();
    texto("BESTIÁRIO",54,34,40,OURO);
    for(int a=0;a<NUM_ABAS_DO_BESTIARIO;a++) {
        Rectangle r=abaDoBestiario(a);
        bool ativa=(a==aba), sobre=mouseSobre(r);
        DrawRectangleRounded(r,0.25f,8,ativa?Color{52,66,96,255}:sobre?Color{31,42,65,255}:PAINEL);
        DrawRectangleRoundedLinesEx(r,0.25f,8,ativa?2.5f:1.5f,ativa?OURO:sobre?CIANO:Color{65,88,113,255});
        std::string nome="PERSONAGENS";
        if(a!=ABA_PERSONAGENS) {
            int vistas=0;
            for(int k=0;k<cartasDaAba(a);k++) if(fichaDoBestiario(especieDaCarta(a,k)).visto) vistas++;
            nome=std::string(a==ABA_MONSTROS?"MONSTROS  ":"CHEFES  ")+std::to_string(vistas)+"/"+std::to_string(cartasDaAba(a));
        }
        textoCentro(nome,r.x+r.width/2,r.y+12,22,ativa?OURO:TEXTO);
    }
    int total = cartasDaAba(aba);
    for(int carta=0;carta<total;carta++) {
        Rectangle r=cartaDoBestiario(carta);
        cartaoDoBestiario(r,carta==escolhido);
        int i = aba==ABA_PERSONAGENS ? carta : especieDaCarta(aba,carta);
        if(aba!=ABA_PERSONAGENS) {
            const FichaDoBestiario& f=fichaDoBestiario(i);
            if(f.visto) {
                sprite(imagemMonstro(i).data(),r.x+(r.width-88)/2,r.y+12,5.5f);
                textoQueCabe(comMaiusculaInicial(TIPOS_DE_MONSTRO[i].nome),r.x+r.width/2-larguraDoTexto(comMaiusculaInicial(TIPOS_DE_MONSTRO[i].nome),17)/2,
                             r.y+108,17,r.width-12,TEXTO);
                if(f.derrotados>0) textoCentro(std::to_string(f.derrotados)+(f.derrotados==1?" derrotado":" derrotados"),
                                               r.x+r.width/2,r.y+134,13,APAGADO);
                if(f.campeoesDerrotados>0) coroinha(r.x+r.width-24,r.y+20,1.6f);
            } else {
                silhueta(imagemMonstro(i).data(),r.x+(r.width-88)/2,r.y+12,5.5f,{44,56,80,255});
                textoCentro("???",r.x+r.width/2,r.y+110,20,APAGADO);
            }
        } else {
            sprite(PERSONAGENS[i].arte->data(),r.x+(r.width-88)/2,r.y+12,5.5f);
            textoQueCabe(PERSONAGENS[i].nome,r.x+8,r.y+108,16,r.width-16,TEXTO);
            textoCentro(PERSONAGENS[i].onde,r.x+r.width/2,r.y+134,13,APAGADO);
        }
    }
    if(aba!=ABA_PERSONAGENS) fichaDoMonstro(jogo,especieDaCarta(aba,escolhido)); else fichaDoPersonagem(escolhido);
    texto("Setas / mouse: escolher    Tab: trocar de aba    Esc ou E: voltar",54,842,18,APAGADO);
    bool sobre=mouseSobre(VOLTAR_DO_BESTIARIO);
    DrawRectangleRounded(VOLTAR_DO_BESTIARIO,0.25f,8,sobre?Color{43,58,88,255}:PAINEL2);
    DrawRectangleRoundedLinesEx(VOLTAR_DO_BESTIARIO,0.25f,8,1.5f,sobre?CIANO:Color{69,89,110,255});
    textoCentro("VOLTAR",VOLTAR_DO_BESTIARIO.x+VOLTAR_DO_BESTIARIO.width/2,VOLTAR_DO_BESTIARIO.y+13,24,TEXTO);
}
}

// ---------------------------------------------------------------------------
// DIÁRIO (tecla J, v1.26)
// Aba MISSÕES: a missão principal (os 13 andares e os chefes) e as missões do
// andar atual. Aba ANOTAÇÕES: o que as pessoas contaram e os chefes vencidos.
// ---------------------------------------------------------------------------
// Um ✓ verde num círculo (o que já foi feito / vencido).
void seloVerde(float cx,float cy) {
    DrawCircleV({cx,cy},14,{60,160,90,255});
    DrawLineEx({cx-6,cy},{cx-1,cy+5},3,{235,255,240,255});
    DrawLineEx({cx-1,cy+5},{cx+7,cy-5},3,{235,255,240,255});
}
// A trilha dos 13 andares (diário e resumo do fim): o chefe de cada andar de
// chefe em cima (silhueta preta até ser visto; ✓ quando cai) e o herói no andar
// em que está.
void trilhaDosAndares(const Jogo& jogo,float x0,float x1,float ly) {
    float t=(float)GetTime();
    std::vector<EtapaPrincipal> etapas=etapasDaMissaoPrincipal(jogo);
    auto etapaDoAndar=[&](int n)->const EtapaPrincipal* { for(const auto& e:etapas) if(e.andar==n) return &e; return nullptr; };
    float passo=(x1-x0)/(NUM_ANDARES-1);
    DrawLineEx({x0,ly},{x1,ly},4,{50,64,90,255});
    DrawLineEx({x0,ly},{x0+passo*(jogo.andar-1),ly},4,multiplicar(CIANO,0.7f));
    for(int n=1;n<=NUM_ANDARES;n++) {
        float nx=x0+passo*(n-1);
        const EtapaPrincipal* e=etapaDoAndar(n);
        bool passou=n<jogo.andar, aqui=n==jogo.andar;
        if(e) {
            // O chefe do andar em cima da trilha: silhueta preta até ser visto.
            float P=2.6f, L=16*P;
            if(e->especie>=0) desenharSprite(imagemMonstro(e->especie),nx-L/2,ly-L-16,P,e->vencido?0.55f:1.0f);
            else desenharSprite(SENHOR,nx-L/2,ly-L-16,P,0.0f);
            if(e->vencido) seloVerde(nx+16,ly-L-12);
        }
        float raio=aqui?13:9;
        Color fundo=passou?Color{40,90,110,255}:aqui?Color{70,56,20,255}:Color{24,32,50,255};
        DrawCircleV({nx,ly},raio,fundo);
        Color borda=aqui?multiplicar(OURO,0.75f+0.25f*std::sin(t*4)):passou?CIANO:Color{70,85,110,255};
        DrawCircleLines((int)nx,(int)ly,raio,borda); DrawCircleLines((int)nx,(int)ly,raio-1,borda);
        if(aqui) { definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor])); desenharSprite(HEROI,nx-12,ly-12,1.5f); }
        textoCentro(std::to_string(n),nx,ly+16,14,aqui?OURO:APAGADO);
    }
}

namespace Diario {
const Rectangle PAINEL={110,50,1220,800};
Rectangle aba(int i) { return {PAINEL.x+PAINEL.width-30-(2-i)*190.0f,PAINEL.y+20,180,40}; }

// Selo no canto de um cartão: ✓ (feita) ou "!" (obrigatória que falta).
void seloDoDiario(float cx,float cy,bool feita,bool obrigatoria) {
    if(feita) {
        DrawCircleV({cx,cy},14,{60,160,90,255});
        DrawLineEx({cx-6,cy},{cx-1,cy+5},3,{235,255,240,255});
        DrawLineEx({cx-1,cy+5},{cx+7,cy-5},3,{235,255,240,255});
    } else if(obrigatoria) {
        float p=0.75f+0.25f*std::sin((float)GetTime()*4);
        DrawCircleV({cx,cy},14,multiplicar(OURO,p));
        DrawRectangle((int)cx-2,(int)cy-8,4,10,{40,30,10,255});
        DrawRectangle((int)cx-2,(int)cy+5,4,4,{40,30,10,255});
    }
}
// Escadinha (cartão da escada).
void escadinha(float x,float y,float t,float brilho) {
    DrawRectangleRec({x,y,t,t},{10,8,14,255});
    for(int i=0;i<4;i++) {
        float f=(1.0f-i*0.2f)*brilho;
        DrawRectangleRec({x+t*0.06f+i*t*0.09f,y+t*0.1f+i*t*0.2f,t*0.88f-i*t*0.18f,t*0.12f},multiplicar({176,140,82,255},f));
    }
    DrawRectangleLinesEx({x,y,t,t},2,multiplicar(OURO,brilho));
}
// O desenho de uma missão, centrado em (cx, cy), com uns 64 pixels.
void desenhoDaMissao(const Missao& m,float cx,float cy) {
    const float P=4.0f, L=16*P;
    float x=cx-L/2, y=cy-L/2, luz=m.feita?0.55f:1.0f;
    switch(m.tipo) {
        case MISSAO_CHEFE:
            if(m.especie>=0) desenharSprite(imagemMonstro(m.especie),x,y,P,luz);
            else { desenharSprite(SENHOR,x,y,P,0.0f); textoCentro("?",cx,cy-16,30,{150,160,190,255}); }
            break;
        case MISSAO_ESCADA: escadinha(cx-26,cy-26,52,luz); break;
        case MISSAO_TESOURO: desenharSprite(*ARTE_DOS_ITENS[I_CHAVE],x,y,P,luz); break;
        case MISSAO_FONTE: desenharSprite(m.feita?FONTE_SECA:FONTE,x,y,P); break;
        case MISSAO_ALTAR: desenharSprite(m.feita?ALTAR_USADO:ALTAR,x,y,P); break;
        case MISSAO_BAU: desenharSprite(m.feita?BAU_ABERTO:BAU_DOURADO,x,y,P); break;
        case MISSAO_ARENA: desenharSprite(m.feita?GONGO_RACHADO:GONGO,x,y,P); break;
        default:
            if(m.npc==NPC_EXPLORADORA||m.npc==NPC_MINEIRO||m.npc==NPC_ESPIRITO||m.npc==NPC_COMERCIANTE||m.npc==NPC_VIAJANTE)
                desenharSprite(imagemNpc(m.npc),x,y,P,luz);
    }
}

void missoes(const Jogo& jogo) {
    float x=PAINEL.x+30, w=PAINEL.width-60;
    // ---- Missão principal: os 13 andares e os chefes ----
    Rectangle c={x,PAINEL.y+90,w,196};
    DrawRectangleRounded(c,0.1f,8,{20,29,48,255});
    texto("MISSÃO PRINCIPAL",c.x+20,c.y+14,14,APAGADO);
    texto("Vencer o Senhor das Profundezas",c.x+20,c.y+32,22,OURO);
    trilhaDosAndares(jogo,c.x+50,c.x+c.width-50,c.y+150);
    // ---- Este andar: um cartão por missão ----
    float my=PAINEL.y+306;
    texto("ANDAR "+std::to_string(jogo.andar)+"  ·  "+FICHAS_DOS_TEMAS[temaDoAndar(jogo.andar)].nome,x,my,14,APAGADO);
    std::vector<Missao> lista=missoesDoAndar(jogo);
    std::stable_sort(lista.begin(),lista.end(),[](const Missao& a,const Missao& b){
        if(a.feita!=b.feita) return !a.feita;
        return a.obrigatoria && !b.obrigatoria; });
    if(lista.empty()) {
        Rectangle r={x,my+24,w,80};
        DrawRectangleRounded(r,0.2f,8,{20,29,48,255});
        textoCentro("Explore o andar para descobrir o que fazer aqui.",x+w/2,r.y+29,18,APAGADO);
    }
    const int COLUNAS=5;
    const float CW=(w-(COLUNAS-1)*14)/COLUNAS, CH=150;
    for(size_t i=0;i<lista.size() && i<10;i++) {
        const Missao& m=lista[i];
        Rectangle r={x+(i%COLUNAS)*(CW+14),my+24+(i/COLUNAS)*(CH+14),CW,CH};
        bool destaque=!m.feita && m.obrigatoria;
        DrawRectangleRounded(r,0.14f,8,m.feita?Color{17,23,37,255}:Color{24,34,56,255});
        DrawRectangleRoundedLinesEx(r,0.14f,8,destaque?2.2f:1.0f,destaque?OURO:m.feita?Color{40,52,72,255}:Color{60,76,104,255});
        desenhoDaMissao(m,r.x+r.width/2,r.y+50);
        float tam=19; while(tam>13 && larguraDoTexto(m.titulo,tam)>r.width-16) tam-=1;
        textoCentro(m.titulo,r.x+r.width/2,r.y+92,tam,m.feita?Color{120,135,160,255}:TEXTO);
        float td=14; while(td>11 && larguraDoTexto(m.detalhe,td)>r.width-14) td-=1;
        textoCentro(m.detalhe,r.x+r.width/2,r.y+120,td,m.feita?Color{120,230,140,255}:APAGADO);
        seloDoDiario(r.x+r.width-18,r.y+18,m.feita,m.obrigatoria);
    }
}

void anotacoes(const Jogo& jogo,int& rolagem) {
    std::vector<Anotacao> notas=anotacoesDoDiario(jogo);
    float x=PAINEL.x+30, w=PAINEL.width-60;
    if(notas.empty()) {
        Rectangle r={x,PAINEL.y+96,w,80};
        DrawRectangleRounded(r,0.2f,8,{20,29,48,255});
        textoCentro("Ainda não há anotações. Converse com as pessoas que encontrar.",x+w/2,r.y+29,18,APAGADO);
        return;
    }
    const int COLUNAS=3, LINHAS_NA_TELA=3;
    int linhas=((int)notas.size()+COLUNAS-1)/COLUNAS;
    rolagem=std::clamp(rolagem,0,std::max(0,linhas-LINHAS_NA_TELA));
    float cw=(w-2*16)/COLUNAS, ch=196;
    for(int i=0;i<(int)notas.size();i++) {
        int linha=i/COLUNAS-rolagem;
        if(linha<0 || linha>=LINHAS_NA_TELA) continue;
        const Anotacao& a=notas[i];
        Rectangle r={x+(i%COLUNAS)*(cw+16),PAINEL.y+96+linha*(ch+16),cw,ch};
        Color cor=a.chefe?Color{230,90,80,255}:CIANO;
        DrawRectangleRounded(r,0.12f,8,{20,29,48,255});
        DrawRectangleRoundedLinesEx(r,0.12f,8,1.2f,multiplicar(cor,0.55f));
        // Retrato.
        Rectangle f={r.x+16,r.y+16,80,80};
        DrawRectangleRounded(f,0.2f,8,a.chefe?Color{50,24,30,255}:Color{24,40,58,255});
        if(a.chefe) desenharSprite(imagemMonstro(a.especie),f.x+4,f.y+4,4.5f,0.8f);
        else desenharSprite(imagemNpc(a.npc),f.x+4,f.y+4,4.5f);
        // Andar, nome (só o primeiro nome) e, no chefe, o carimbo VENCIDO.
        std::string andar="Andar "+std::to_string(a.andar);
        float wa=larguraDoTexto(andar,13)+18;
        DrawRectangleRounded({r.x+110,r.y+18,wa,22},0.5f,6,{34,46,72,255});
        textoCentro(andar,r.x+110+wa/2,r.y+21,13,APAGADO);
        std::string nome=a.quem; size_t v=nome.find(',');
        std::string papel = v!=std::string::npos ? nome.substr(v+2) : "";
        if(v!=std::string::npos) nome=nome.substr(0,v);
        float tn=22; while(tn>15 && larguraDoTexto(nome,tn)>r.width-126) tn-=1;
        texto(nome,r.x+110,r.y+46,tn,a.chefe?Color{240,120,100,255}:OURO);
        if(!papel.empty()) texto(papel,r.x+110,r.y+74,14,APAGADO);
        if(a.chefe) {
            Rectangle carimbo={r.x+110,r.y+72,110,26};
            DrawRectangleRoundedLinesEx(carimbo,0.3f,6,2.0f,{230,90,80,255});
            textoCentro("VENCIDO",carimbo.x+55,carimbo.y+4,16,{240,120,100,255});
        }
        // As dicas, uma por linha, com um marcador.
        for(size_t k=0;k<a.pontos.size() && k<3;k++) {
            float py=r.y+112+k*26;
            DrawCircleV({r.x+24,py+9},4,multiplicar(cor,0.9f));
            textoQueCabe(a.pontos[k],r.x+36,py,15,r.width-50,TEXTO);
        }
    }
    if(linhas>LINHAS_NA_TELA)
        texto("setas para cima e para baixo: mais anotações ("+std::to_string(rolagem+1)+"/"+
              std::to_string(linhas-LINHAS_NA_TELA+1)+")",x,PAINEL.y+PAINEL.height-70,14,APAGADO);
}

void desenhar(const Jogo& jogo,int abaAtual,int& rolagem) {
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,177});
    painel(PAINEL,{26,35,56,255},OURO);
    texto("DIÁRIO",PAINEL.x+30,PAINEL.y+22,30,OURO);
    const char* nomes[2]={"MISSÕES","ANOTAÇÕES"};
    for(int i=0;i<2;i++) {
        Rectangle r=aba(i);
        bool ativa=i==abaAtual;
        DrawRectangleRounded(r,0.5f,8,ativa?Color{70,56,20,255}:Color{24,32,50,255});
        DrawRectangleRoundedLinesEx(r,0.5f,8,ativa?2.0f:1.0f,ativa?OURO:Color{60,74,100,255});
        textoCentro(nomes[i],r.x+r.width/2,r.y+10,18,ativa?OURO:APAGADO);
    }
    separador(PAINEL.x+30,PAINEL.y+72,PAINEL.width-60);
    if(abaAtual==0) missoes(jogo); else anotacoes(jogo,rolagem);
    textoCentro("Tab ou setas para os lados: trocar de aba    Esc ou J: voltar",
                PAINEL.x+PAINEL.width/2,PAINEL.y+PAINEL.height-38,16,{120,135,160,255});
}
}

void mostrarDiario(const Jogo& jogo) {
    telaPorCima=true;
    static int abaAtual=0;
    int rolagem=0;
    tocarSom(SOM_MENU);
    auto clicou=[](Rectangle r){ return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r); };
    while(!WindowShouldClose()) {
        Ui::iniciarQuadro(); mapa(jogo); Diario::desenhar(jogo,abaAtual,rolagem); Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
        int antes=abaAtual;
        if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_BACKSPACE)) break;
        if(IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_A))
            abaAtual=1-abaAtual;
        for(int i=0;i<2;i++) if(clicou(Diario::aba(i))) abaAtual=i;
        if(IsKeyPressed(KEY_DOWN)||IsKeyPressedRepeat(KEY_DOWN)||IsKeyPressed(KEY_S)) rolagem++;
        if(IsKeyPressed(KEY_UP)||IsKeyPressedRepeat(KEY_UP)||IsKeyPressed(KEY_W)) rolagem=std::max(0,rolagem-1);
        float roda=GetMouseWheelMove();
        if(roda<0) rolagem++; else if(roda>0) rolagem=std::max(0,rolagem-1);
        if(abaAtual!=antes) { tocarSom(SOM_MENU); rolagem=0; }
    }
    Ui::iniciarQuadro(); mapa(jogo); Diario::desenhar(jogo,abaAtual,rolagem); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
    tocarSom(SOM_MENU);
    telaPorCima=false;
}

// ---------------------------------------------------------------------------
// MOCHILA (tecla i, v1.25)
// À esquerda, o que o jogador está usando (arma e escudo) e os golpes que
// sabe; à direita, os itens em quadrinhos e a descrição do marcado.
// ---------------------------------------------------------------------------
void alternarMapaInteiro() {}   // v1.25: na versão gráfica o Tab é lido direto (hud)
bool mochilaMostraEquipamento() { return true; }
namespace Mochila {
const Rectangle PAINEL={110,60,1220,780};
const float LX=140, LW=470, RX=640, RW=660, TW=156, TH=104, TGAP=12;
const int COLUNAS=4;
Rectangle quadro(int i) { return {RX+(i%COLUNAS)*(TW+TGAP),150.0f+(i/COLUNAS)*(TH+TGAP),TW,TH}; }

std::string efeitoDoGolpe(const Golpe& g) {
    static const char* NOMES[]={"","envenena","faz dormir","paralisa","sobe o seu ataque","sobe a sua defesa",
        "baixa o ataque do alvo","baixa a defesa do alvo","suga vida","acerta 2 vezes","queima","congela","ofusca",
        "atordoa","prende","cura","planta uma semente que suga vida","o próximo golpe é crítico","acerta 3 vezes"};
    std::string s;
    if(g.efeito!=SEM_EFEITO) {
        s=NOMES[g.efeito];
        if(g.efeito==CURAR) s+=" "+std::to_string(g.chanceDoEfeito)+"% da vida";
        else if(g.poder>0 && g.chanceDoEfeito>0 && g.chanceDoEfeito<100 && g.efeito!=DRENAR && g.efeito!=DOIS_GOLPES && g.efeito!=TRES_GOLPES)
            s+=" ("+std::to_string(g.chanceDoEfeito)+"%)";
    }
    if(g.area) s+=std::string(s.empty()?"":"  ·  ")+"acerta todos";
    if(g.prioridade>0) s+=std::string(s.empty()?"":"  ·  ")+"age primeiro";
    if(s.empty()) s=g.poder>0?"só dano":"";
    return s;
}
const char* verbo(int tipo) {
    switch(TIPOS_DE_ITEM[tipo].categoria) {
        case POCAO: return "beber";
        case ARMA: case ESCUDO: return "equipar";
        case PERGAMINHO: return "ler";
        case SUPORTE: return "usar";
        default: return nullptr;
    }
}

void cartaoDoEquipamento(Rectangle r,const char* rotulo,const TipoItem* item) {
    DrawRectangleRounded(r,0.18f,8,{20,29,48,255});
    DrawRectangleRoundedLinesEx(r,0.18f,8,1.2f,item?Color{90,110,140,255}:Color{50,62,85,255});
    texto(rotulo,r.x+r.width-14-larguraDoTexto(rotulo,13),r.y+10,13,APAGADO);
    if(!item) {
        DrawRectangleRoundedLinesEx({r.x+14,r.y+16,54,54},0.2f,6,1.2f,{60,74,100,255});
        texto(std::string("Sem ")+rotulo,r.x+84,r.y+18,21,APAGADO);
        texto("pegue um no chão ou compre na loja",r.x+84,r.y+48,14,APAGADO);
        return;
    }
    int tipo=(int)(item-TIPOS_DE_ITEM);
    DrawRectangleRounded({r.x+14,r.y+16,54,54},0.2f,6,{30,42,66,255});
    desenharSprite(*ARTE_DOS_ITENS[tipo],r.x+17,r.y+19,3.0f);
    texto(item->nome,r.x+84,r.y+12,22,OURO);
    if(item->categoria==ARMA) {
        texto("+"+std::to_string(item->bonusAtaque)+" de ataque",r.x+84,r.y+44,17,TEXTO);
        if(item->elemento>=0) {
            float w=etiquetaDeElemento(item->elemento,r.x+84+larguraDoTexto("+"+std::to_string(item->bonusAtaque)+" de ataque",17)+14,r.y+42,12);
            (void)w;
            texto("+25%",r.x+84+larguraDoTexto("+"+std::to_string(item->bonusAtaque)+" de ataque",17)+14+w+8,r.y+44,16,{120,230,140,255});
        }
    } else {
        std::string d="+"+std::to_string(item->bonusDefesa)+" de defesa";
        if(item->bonusVelocidade<0) d+="   "+std::to_string(item->bonusVelocidade)+" de velocidade";
        texto(d,r.x+84,r.y+40,17,TEXTO);
        texto("bloqueia "+std::to_string(chanceDeBloqueio(item,false))+"% sozinho, "+
              std::to_string(chanceDeBloqueio(item,true))+"% com o golpe Escudo",r.x+84,r.y+62,14,APAGADO);
    }
}

void cartaoDoGolpe(Rectangle r,const Jogador& j,int slot) {
    DrawRectangleRounded(r,0.18f,8,{20,29,48,255});
    if(slot>=j.numGolpes) {
        DrawRectangleRoundedLinesEx(r,0.18f,8,1.0f,{45,56,78,255});
        textoCentro("espaço livre para um golpe",r.x+r.width/2,r.y+r.height/2-9,15,{90,105,130,255});
        return;
    }
    const Golpe& g=GOLPES[j.golpes[slot]];
    Color ce=corDoElemento(g.elemento);
    DrawRectangleRoundedLinesEx(r,0.18f,8,1.4f,multiplicar(ce,0.7f));
    DrawRectangleRounded({r.x,r.y,8,r.height},0.5f,4,ce);
    iconeDoElemento(g.elemento,r.x+20,r.y+12,1.9f);
    texto(g.nome,r.x+62,r.y+8,21,TEXTO);
    texto(NOMES_DOS_ELEMENTOS[g.elemento],r.x+62+larguraDoTexto(g.nome,21)+10,r.y+13,14,multiplicar(ce,1.2f));
    std::string ef=efeitoDoGolpe(g);
    textoQueCabe(ef,r.x+62,r.y+38,14,r.width-62-150,APAGADO);
    // Poder e PP à direita.
    float dx=r.x+r.width-140;
    std::string poder=g.poder>0?std::to_string(g.poder):"-";
    texto(poder,dx,r.y+6,24,g.poder>0?TEXTO:APAGADO);
    texto("poder",dx,r.y+34,12,APAGADO);
    float px=dx+58;
    if(g.ppMaximo>0) {
        texto(std::to_string(j.pp[slot])+"/"+std::to_string(g.ppMaximo),px,r.y+9,17,TEXTO);
        texto("PP",px+larguraDoTexto(std::to_string(j.pp[slot])+"/"+std::to_string(g.ppMaximo),17)+5,r.y+13,12,APAGADO);
        Rectangle b={px,r.y+36,66,6};
        DrawRectangleRounded(b,0.5f,4,{28,38,60,255});
        float f=std::clamp((float)j.pp[slot]/g.ppMaximo,0.0f,1.0f);
        if(f>0) DrawRectangleRounded({b.x,b.y,std::max(6.0f,b.width*f),b.height},0.5f,4,f<0.25f?PERIGO:CIANO);
    } else texto("sem limite",px,r.y+12,14,APAGADO);
    texto("precisão "+std::to_string(g.precisao)+"%",r.x+62,r.y+58,13,{110,125,150,255});
}

void desenhar(const Jogo& jogo,const std::vector<int>& tipos,int cursor) {
    const Jogador& j=jogo.jogador;
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,177});
    painel(PAINEL,{26,35,56,255},OURO);
    texto("MOCHILA",PAINEL.x+30,PAINEL.y+18,30,OURO);
    // Espaço ocupado.
    int usados=itensNaMochila(j), limite=limiteDaMochila(j);
    std::string cap=std::to_string(usados)+" / "+std::to_string(limite)+" itens";
    float bw=200, bx=PAINEL.x+PAINEL.width-30-bw;
    texto(cap,bx-14-larguraDoTexto(cap,18),PAINEL.y+26,18,usados>=limite?PERIGO:TEXTO);
    DrawRectangleRounded({bx,PAINEL.y+32,bw,10},0.5f,4,{28,38,60,255});
    float f=std::clamp((float)usados/std::max(1,limite),0.0f,1.0f);
    if(f>0) DrawRectangleRounded({bx,PAINEL.y+32,std::max(10.0f,bw*f),10},0.5f,4,usados>=limite?PERIGO:Color{170,130,80,255});
    separador(PAINEL.x+30,PAINEL.y+64,PAINEL.width-60);

    // ---- Equipado e golpes ----
    texto("EQUIPADO",LX,132,14,APAGADO);
    cartaoDoEquipamento({LX,152,LW,86},"arma",j.arma);
    cartaoDoEquipamento({LX,248,LW,86},"escudo",j.escudo);
    texto("GOLPES  "+std::to_string(j.numGolpes)+"/"+std::to_string(MAX_GOLPES),LX,354,14,APAGADO);
    for(int s=0;s<MAX_GOLPES;s++) cartaoDoGolpe({LX,374.0f+s*90,LW,80},j,s);

    // ---- Itens ----
    texto("ITENS",RX,132,14,APAGADO);
    if(tipos.empty()) {
        Rectangle r={RX,150,RW,TH};
        DrawRectangleRounded(r,0.18f,8,{20,29,48,255});
        textoCentro("A mochila está vazia.",RX+RW/2,r.y+40,19,APAGADO);
    }
    for(int i=0;i<(int)tipos.size();i++) {
        int t=tipos[i];
        Rectangle r=quadro(i);
        bool marcado=i==cursor;
        DrawRectangleRounded(r,0.18f,8,marcado?Color{38,52,82,255}:Color{20,29,48,255});
        DrawRectangleRoundedLinesEx(r,0.18f,8,marcado?2.4f:1.0f,marcado?OURO:Color{50,64,90,255});
        desenharSprite(*ARTE_DOS_ITENS[t],r.x+r.width/2-24,r.y+12,3.0f);
        std::string nome=t==I_CHAVE?"Chave do andar "+std::to_string(jogo.andar):TIPOS_DE_ITEM[t].nome;
        float tam=15; while(tam>11 && larguraDoTexto(nome,tam)>r.width-12) tam-=1;
        textoCentro(nome,r.x+r.width/2,r.y+70,tam,marcado?OURO:TEXTO);
        if(t!=I_CHAVE) {
            std::string n=std::to_string(j.mochila[t])+"/"+std::to_string(limiteDoItem(j,t));
            texto(n,r.x+r.width-10-larguraDoTexto(n,14),r.y+6,14,j.mochila[t]>=limiteDoItem(j,t)?Color{200,170,90,255}:APAGADO);
        }
    }
    // Descrição do marcado.
    Rectangle d={RX,PAINEL.y+PAINEL.height-206,RW,150};
    DrawRectangleRounded(d,0.12f,8,{20,29,48,255});
    if(!tipos.empty()) {
        int t=tipos[std::clamp(cursor,0,(int)tipos.size()-1)];
        std::string nome=t==I_CHAVE?"Chave do andar "+std::to_string(jogo.andar):TIPOS_DE_ITEM[t].nome;
        texto(nome,d.x+20,d.y+14,22,OURO);
        std::string desc=t==I_CHAVE?"Abre a porta trancada da sala do tesouro DESTE andar.  |  Não ocupa espaço na mochila."
                                   :descricaoDoItem(j,t);
        size_t corte=desc.find("  |  ");
        textoQuebrado(desc.substr(0,corte),d.x+20,d.y+50,17,d.width-40,24,TEXTO);
        if(corte!=std::string::npos) textoQueCabe(desc.substr(corte+5),d.x+20,d.y+102,16,d.width-40,CIANO);
        const char* v=verbo(t);
        if(v) {
            std::string acao=std::string("Enter: ")+v;
            texto(acao,d.x+d.width-20-larguraDoTexto(acao,16),d.y+18,16,{120,230,140,255});
        }
    }
    textoCentro("Setas: escolher    Enter ou clique duplo: usar    Esc ou I: voltar",
                PAINEL.x+PAINEL.width/2,PAINEL.y+PAINEL.height-40,16,{120,135,160,255});
}
}

int escolherNaMochila(const Jogo& jogo, const std::vector<int>& tipos, int& cursor) {
    telaPorCima=true;
    tocarSom(SOM_MENU);
    int total=(int)tipos.size();
    if(cursor>=total) cursor=std::max(0,total-1);
    auto clicou=[](Rectangle r){ return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r); };
    int escolha=-1;
    while(!WindowShouldClose()) {
        Ui::iniciarQuadro(); mapa(jogo); Mochila::desenhar(jogo,tipos,cursor); Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
        int antes=cursor;
        if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_I) || IsKeyPressed(KEY_BACKSPACE)) break;
        if(total>0) {
            int C=Mochila::COLUNAS;
            if(IsKeyPressed(KEY_RIGHT)||IsKeyPressedRepeat(KEY_RIGHT)||IsKeyPressed(KEY_D)) cursor=std::min(total-1,cursor+1);
            if(IsKeyPressed(KEY_LEFT)||IsKeyPressedRepeat(KEY_LEFT)||IsKeyPressed(KEY_A)) cursor=std::max(0,cursor-1);
            if(IsKeyPressed(KEY_DOWN)||IsKeyPressedRepeat(KEY_DOWN)||IsKeyPressed(KEY_S)) cursor=std::min(total-1,cursor+C);
            if(IsKeyPressed(KEY_UP)||IsKeyPressedRepeat(KEY_UP)||IsKeyPressed(KEY_W)) cursor=std::max(0,cursor-C);
            bool usar=IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)||IsKeyPressed(KEY_SPACE);
            for(int i=0;i<total;i++) if(clicou(Mochila::quadro(i))) { if(i==cursor) usar=true; cursor=i; }
            if(cursor!=antes) tocarSom(SOM_MENU);
            if(usar && Mochila::verbo(tipos[cursor])) { escolha=cursor; break; }
        }
    }
    Ui::iniciarQuadro(); mapa(jogo); Mochila::desenhar(jogo,tipos,cursor); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
    tocarSom(SOM_MENU);
    telaPorCima=false;
    return escolha;
}

// ---------------------------------------------------------------------------
// PERGAMINHO COM 4 GOLPES (v1.27): o golpe novo em cima e os 4 que o jogador
// sabe embaixo, com elemento, poder e PP, para escolher qual esquecer.
// ---------------------------------------------------------------------------
namespace Esquecer {
const Rectangle PAINEL={290,110,860,660};
Rectangle linha(int i) { return {PAINEL.x+40,PAINEL.y+322+i*48.0f,PAINEL.width-80,42}; }
void desenhar(const Jogo& jogo,int golpeNovo,int cursor) {
    const Jogador& j=jogo.jogador;
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,150});
    painel(PAINEL,{26,35,56,255},OURO);
    float x=PAINEL.x+40;
    texto("APRENDER UM GOLPE",x,PAINEL.y+28,30,OURO);
    texto("pergaminho",x+larguraDoTexto("APRENDER UM GOLPE",30)+14,PAINEL.y+41,15,APAGADO);
    // O golpe novo, num cartão.
    Rectangle c={x,PAINEL.y+80,PAINEL.width-80,160};
    DrawRectangleRounded(c,0.08f,8,{20,28,46,255});
    DrawRectangleRoundedLinesEx(c,0.08f,8,1.5f,multiplicar(corDoElemento(GOLPES[golpeNovo].elemento),0.8f));
    texto("NOVO GOLPE",c.x+22,c.y+14,13,CIANO);
    detalhesDoGolpe(jogo,nullptr,GOLPES[golpeNovo],GOLPES[golpeNovo].ppMaximo,c.x+22,c.y+38,false);
    texto("Você já sabe 4 golpes. Qual vai esquecer?",x,PAINEL.y+278,21,TEXTO);
    for(int i=0;i<=j.numGolpes;i++) {
        if(i<j.numGolpes) linhaDoGolpe(nullptr,GOLPES[j.golpes[i]],j.pp[i],linha(i),cursor==i,-1,cursor);
        else faixaBotao("Não aprender (guardar o pergaminho)",linha(i),cursor==i,-1,cursor,19);
    }
    texto("Setas e Enter: escolher   |   Esc: não aprender",x,PAINEL.y+PAINEL.height-44,17,CIANO);
}
}
int escolherGolpeParaEsquecer(const Jogo& jogo,int golpeNovo) {
    telaPorCima=true;
    tocarSom(SOM_MENU);
    const Jogador& j=jogo.jogador;
    int total=j.numGolpes+1, cursor=0, escolha=-1;
    auto clicou=[](Rectangle r){ return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r); };
    while(!WindowShouldClose()) {
        Ui::iniciarQuadro(); mapa(jogo); Esquecer::desenhar(jogo,golpeNovo,cursor); Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
        int antes=cursor;
        if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) break;
        if(IsKeyPressed(KEY_DOWN)||IsKeyPressedRepeat(KEY_DOWN)||IsKeyPressed(KEY_S)) cursor=(cursor+1)%total;
        if(IsKeyPressed(KEY_UP)||IsKeyPressedRepeat(KEY_UP)||IsKeyPressed(KEY_W)) cursor=(cursor+total-1)%total;
        bool usar=IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)||IsKeyPressed(KEY_SPACE);
        for(int i=0;i<total;i++) if(clicou(Esquecer::linha(i))) { if(i==cursor) usar=true; cursor=i; }
        if(cursor!=antes) tocarSom(SOM_MENU);
        if(usar) { escolha=cursor; break; }
    }
    while(GetCharPressed()!=0) {}
    tocarSom(SOM_MENU);
    telaPorCima=false;
    return (escolha>=0 && escolha<j.numGolpes) ? escolha : -1;
}

// ---------------------------------------------------------------------------
// FICHA (tecla C, v1.25)
// À esquerda, quem é o personagem (desenho, nível, vida); à direita, os
// atributos em cartões e, embaixo, o que eles dão. O atributo marcado acende
// os números que ele aumenta e mostra quanto sobe com mais um ponto.
// ---------------------------------------------------------------------------
namespace Ficha {
const Rectangle PAINEL={200,92,1040,640};
const float CX=590, CW=620, CY=208, CH=70, CGAP=10;
Rectangle cartaoDoAtributo(int a) { return {CX,CY+a*(CH+CGAP),CW,CH}; }
Rectangle botaoMais(int a) { Rectangle r=cartaoDoAtributo(a); return {r.x+r.width-56,r.y+16,40,40}; }
const char* RESUMOS[NUM_ATRIBUTOS]={"+1 de ataque por ponto","+3 de vida e +1 de defesa a cada 2","+3% de precisão e +1% de crítico","+1 de velocidade e +1% de esquiva"};
// Os números que cada atributo aumenta (índices de RESULTADOS).
const char* RESULTADOS[7]={"ataque","defesa","vida","velocidade","precisão","esquiva","crítico"};
bool aumenta(int atributo,int resultado) {
    switch(atributo) {
        case FORCA: return resultado==0;
        case CONSTITUICAO: return resultado==1 || resultado==2;
        case DESTREZA: return resultado==4 || resultado==6;
        default: return resultado==3 || resultado==5;
    }
}
void valores(const Jogador& j,int v[7]) {
    v[0]=j.ataque; v[1]=j.defesa; v[2]=j.vidaMaxima; v[3]=j.velocidade;
    v[4]=j.precisaoExtra; v[5]=j.esquiva; v[6]=6+j.criticoExtra;
}
std::string comoTexto(int resultado,int v) {
    if(resultado==4) return "+"+std::to_string(v)+"%";
    if(resultado>=5) return std::to_string(v)+"%";
    return std::to_string(v);
}
void iconeDoResultado(int k,float cx,float cy) {
    Color c={170,185,215,255};
    switch(k) {
        case 0: icone(ARTE_ICONE_FORCA.data(),cx-14,cy-14,1.75f); break;
        case 1: Icones::escudo(cx,cy,13,{120,150,200,255}); break;
        case 2: coracao(cx,cy-4,11,{232,70,90,255}); break;
        case 3: icone(ARTE_ICONE_AGILIDADE.data(),cx-14,cy-14,1.75f); break;
        case 4: icone(ARTE_ICONE_DESTREZA.data(),cx-14,cy-14,1.75f); break;
        case 5: Icones::vento(cx,cy,14,c); break;
        default: icone(ARTE_ICONE_XP.data(),cx-14,cy-14,1.75f); break;
    }
}

void desenhar(const Jogo& jogo,int cursor) {
    const Jogador& j=jogo.jogador;
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[j.cor]));
    float t=(float)GetTime();
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,177});
    painel(PAINEL,{26,35,56,255},OURO);
    textoCentro("FICHA DO PERSONAGEM",PAINEL.x+PAINEL.width/2,PAINEL.y+18,28,OURO);
    separador(PAINEL.x+30,PAINEL.y+62,PAINEL.width-60);

    // ---- Quem é ----
    float lx=232, lw=320, lc=lx+lw/2;
    DrawEllipse((int)lc,330,70,13,{0,0,0,110});
    desenharSprite(HEROI,lc-72,182,9.0f);
    textoCentro(j.nome,lc,350,32,OURO);
    // A mesma conta de largura da etiquetaDeElemento (tam 16), para centralizar.
    float wEl=std::max(60.0f,larguraDoTexto(emMaiusculas(NOMES_DOS_ELEMENTOS[j.elemento]),16)+(24/9.0f*0.82f)*9+30);
    std::string nv="Nível "+std::to_string(j.nivel);
    float total=wEl+14+larguraDoTexto(nv,22);
    etiquetaDeElemento(j.elemento,lc-total/2,396);
    texto(nv,lc-total/2+wEl+14,394,22,CIANO);
    // Experiência.
    int xpNivel=experienciaParaNivel(j.nivel), xpProximo=experienciaParaNivel(j.nivel+1);
    bool noMaximo=j.nivel>=NIVEL_MAXIMO || xpProximo<=xpNivel;
    float fracao=noMaximo?1.0f:std::clamp((float)(j.experiencia-xpNivel)/(xpProximo-xpNivel),0.0f,1.0f);
    texto("EXPERIÊNCIA",lx,440,13,APAGADO);
    Rectangle bx={lx,460,lw,14};
    DrawRectangleRounded(bx,0.5f,4,{28,38,60,255});
    if(fracao>0) DrawRectangleRounded({bx.x,bx.y,std::max(14.0f,bx.width*fracao),bx.height},0.5f,4,{110,175,255,255});
    texto(noMaximo?"nível máximo":"faltam "+std::to_string(xpProximo-j.experiencia)+" para o nível "+std::to_string(j.nivel+1),lx,480,14,APAGADO);
    // Vida.
    texto("VIDA",lx,516,13,APAGADO);
    coracao(lx+12,546,10,{232,70,90,255});
    barra(lx+34,536,lw-34,24,j.vida,j.vidaMaxima);
    textoCentro(std::to_string(j.vida)+" / "+std::to_string(j.vidaMaxima),lx+34+(lw-34)/2,538,19,TEXTO);
    // Vidas.
    texto("VIDAS",lx,584,13,APAGADO);
    for(int v=0;v<3 || v<j.vidas;v++) coracao(lx+12+v*30,614,9,v<j.vidas?Color{232,70,90,255}:Color{60,45,60,255});

    // ---- Atributos ----
    texto("ATRIBUTOS",CX,PAINEL.y+86,15,APAGADO);
    std::string pl=j.pontosLivres>0 ? std::to_string(j.pontosLivres)+(j.pontosLivres==1?" ponto livre":" pontos livres")
                                    : "sem pontos livres: suba de nível";
    float pulso=0.75f+0.25f*std::sin(t*5);
    float wp=larguraDoTexto(pl,16)+(j.pontosLivres>0?44:24);
    Rectangle chip={CX+CW-wp,PAINEL.y+78,wp,28};
    DrawRectangleRounded(chip,0.5f,6,j.pontosLivres>0?Color{60,48,20,255}:Color{24,32,50,255});
    if(j.pontosLivres>0) {
        DrawRectangleRoundedLinesEx(chip,0.5f,6,1.6f,multiplicar(OURO,pulso));
        icone(ARTE_ICONE_PONTO_LIVRE.data(),chip.x+10,chip.y+6,1.0f,pulso);
        texto(pl,chip.x+34,chip.y+5,16,OURO);
    } else texto(pl,chip.x+12,chip.y+5,16,APAGADO);
    for(int a=0;a<NUM_ATRIBUTOS;a++) {
        Rectangle r=cartaoDoAtributo(a);
        bool marcado=a==cursor;
        DrawRectangleRounded(r,0.2f,8,marcado?Color{38,52,82,255}:Color{20,29,48,255});
        DrawRectangleRoundedLinesEx(r,0.2f,8,marcado?2.4f:1.2f,marcado?OURO:Color{55,72,100,255});
        icone(ARTE_DOS_ATRIBUTOS[a]->data(),r.x+16,r.y+16,2.5f);
        texto(NOMES_DOS_ATRIBUTOS[a],r.x+70,r.y+10,24,marcado?OURO:TEXTO);
        texto(RESUMOS[a],r.x+70,r.y+42,15,APAGADO);
        std::string v=std::to_string(j.atributos[a]);
        texto(v,r.x+r.width-(j.pontosLivres>0?84:24)-larguraDoTexto(v,38),r.y+14,38,TEXTO);
        if(j.pontosLivres>0) {
            Rectangle b=botaoMais(a);
            bool sobre=CheckCollisionPointRec(mouseVirtual(),b);
            Color cb=marcado||sobre?multiplicar(OURO,pulso):Color{90,80,50,255};
            DrawCircleV({b.x+20,b.y+20},19,marcado||sobre?Color{70,56,20,255}:Color{30,34,44,255});
            DrawCircleLines((int)b.x+20,(int)b.y+20,19,cb);
            DrawRectangle((int)b.x+11,(int)b.y+18,18,4,cb); DrawRectangle((int)b.x+18,(int)b.y+11,4,18,cb);
        }
    }

    // ---- O que dão ----
    float ry=CY+4*(CH+CGAP)+8;
    texto("O QUE ELES DÃO",CX,ry,15,APAGADO);
    int agora[7]; valores(j,agora);
    int depois[7]; Jogador copia=j; copia.atributos[cursor]++; recalcularAtributos(copia); valores(copia,depois);
    const float TW=(CW-6*8)/7.0f, TH=88;
    for(int k=0;k<7;k++) {
        Rectangle r={CX+k*(TW+8),ry+24,TW,TH};
        bool acende=aumenta(cursor,k);
        DrawRectangleRounded(r,0.2f,6,acende?Color{30,44,66,255}:Color{20,29,48,255});
        if(acende) DrawRectangleRoundedLinesEx(r,0.2f,6,1.8f,CIANO);
        iconeDoResultado(k,r.x+TW/2,r.y+22);
        textoCentro(comoTexto(k,agora[k]),r.x+TW/2,r.y+40,22,acende?TEXTO:Color{200,208,225,255});
        textoCentro(RESULTADOS[k],r.x+TW/2,r.y+66,13,APAGADO);
        int ganho=depois[k]-agora[k];
        if(acende && j.pontosLivres>0 && ganho>0) {
            std::string g="+"+std::to_string(ganho)+(k>=4?"%":"");
            texto(g,r.x+r.width-6-larguraDoTexto(g,14),r.y+4,14,{120,230,140,255});
        }
    }
    textoCentro(j.pontosLivres>0 ? "Setas: escolher    Enter ou +: gastar um ponto    Esc ou C: voltar"
                                 : "Setas: ver o que cada atributo dá    Esc ou C: voltar",
                PAINEL.x+PAINEL.width/2,PAINEL.y+PAINEL.height-36,16,{120,135,160,255});
}
}

int escolherNaFicha(const Jogo& jogo, int& cursor) {
    telaPorCima=true;
    tocarSom(SOM_MENU);
    auto clicou=[](Rectangle r){ return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r); };
    int escolha=-1;
    while(!WindowShouldClose()) {
        Ui::iniciarQuadro(); mapa(jogo); Ficha::desenhar(jogo,cursor); Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}   // as letras não sobram para o jogo depois
        int antes=cursor;
        if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_C) || IsKeyPressed(KEY_BACKSPACE)) break;
        if(IsKeyPressed(KEY_DOWN)||IsKeyPressedRepeat(KEY_DOWN)||IsKeyPressed(KEY_S)) cursor=(cursor+1)%NUM_ATRIBUTOS;
        if(IsKeyPressed(KEY_UP)||IsKeyPressedRepeat(KEY_UP)||IsKeyPressed(KEY_W)) cursor=(cursor+NUM_ATRIBUTOS-1)%NUM_ATRIBUTOS;
        for(int a=0;a<NUM_ATRIBUTOS;a++) if(clicou(Ficha::cartaoDoAtributo(a))) cursor=a;
        if(cursor!=antes) tocarSom(SOM_MENU);
        bool gastar=IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)||IsKeyPressed(KEY_SPACE)||
                    IsKeyPressed(KEY_EQUAL)||IsKeyPressed(KEY_KP_ADD);
        for(int a=0;a<NUM_ATRIBUTOS;a++) if(jogo.jogador.pontosLivres>0 && clicou(Ficha::botaoMais(a))) { cursor=a; gastar=true; }
        if(gastar && jogo.jogador.pontosLivres>0) { escolha=cursor; break; }
    }
    // Mais um quadro: a tecla que fechou (Esc, C) não chega ao jogo.
    Ui::iniciarQuadro(); mapa(jogo); Ficha::desenhar(jogo,cursor); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
    if(escolha<0) tocarSom(SOM_MENU);
    telaPorCima=false;
    return escolha;
}

// ---------------------------------------------------------------------------
// CONQUISTAS E RESUMO DO FIM (v1.26)
// ---------------------------------------------------------------------------
namespace Medalhas {
const Color COR_DO_GRUPO[NUM_GRUPOS_DE_CONQUISTA]={{240,195,80,255},{230,90,80,255},{100,210,220,255}};
int especiePeloSimbolo(char c) {
    for(int i=0;i<NUM_TIPOS_DE_MONSTRO;i++) if(TIPOS_DE_MONSTRO[i].simbolo==c) return i;
    return 0;
}
// O desenho de dentro da medalha (16x16 com "p" de pixel, centrado em cx, cy).
void desenhoDe(int id,float cx,float cy,float p,float luz) {
    float x=cx-8*p, y=cy-8*p;
    switch(id) {
        case C_PRIMEIROS_PASSOS: desenharSprite(HEROI,x,y,p,luz); break;
        case C_REI_DEPOSTO:     desenharSprite(imagemMonstro(especiePeloSimbolo('R')),x,y,p,luz); break;
        case C_FIM_DO_NINHO:    desenharSprite(imagemMonstro(especiePeloSimbolo('A')),x,y,p,luz); break;
        case C_OSSOS_NO_CHAO:   desenharSprite(imagemMonstro(especiePeloSimbolo('n')),x,y,p,luz); break;
        case C_CORACAO_APAGADO: desenharSprite(imagemMonstro(especiePeloSimbolo('L')),x,y,p,luz); break;
        case C_LIBERTADOR:      desenharSprite(SENHOR,x,y,p,luz); break;
        case C_CORAGEM:         Icones::caveira(cx,cy,p*5,multiplicar({235,230,215,255},luz),{20,24,36,255}); break;
        case C_LENDA:           icone(ARTE_ICONE_XP.data(),x,y,p,luz); break;
        case C_SEM_TROPECOS:    coracao(cx,cy-p*2,p*5.5f,multiplicar({232,70,90,255},luz)); break;
        case C_RELAMPAGO:       iconeDoElemento(RAIO,cx-4.5f*p*1.2f,cy-4.5f*p*1.2f,p*1.2f,luz); break;
        case C_TESOURO:         desenharSprite(*ARTE_DOS_ITENS[I_CHAVE],x,y,p,luz); break;
        case C_GLADIADOR:       desenharSprite(GONGO,x,y,p,luz); break;
        case C_DESCONFIADO:     desenharSprite(imagemMonstro(tipoDoMimico()),x,y,p,luz); break;
        case C_BOM_CORACAO:     desenharSprite(imagemNpc(NPC_VIAJANTE),x,y,p,luz); break;
        case C_VELHOS_AMIGOS:   desenharSprite(imagemNpc(NPC_EXPLORADORA),x,y,p,luz); break;
        case C_BOLSO_CHEIO:     icone(ARTE_ICONE_MOEDA.data(),x,y,p,luz); break;
        case C_MUITOS_ELEMENTOS:
            iconeDoElemento(FOGO,cx-7*p,cy-6*p,p*0.75f,luz); iconeDoElemento(AGUA,cx+0.2f*p,cy-6*p,p*0.75f,luz);
            iconeDoElemento(PLANTA,cx-3.4f*p,cy+0.5f*p,p*0.75f,luz); break;
        case C_NA_SURDINA:
            textoCentro("zZz",cx,cy-p*4,p*7,multiplicar({150,190,255,255},luz)); break;
        default:                desenharSprite(imagemMonstro(especiePeloSimbolo('f')),x,y,p,luz); break;
    }
}
// A medalha que está embaixo do mouse neste quadro (-1 = nenhuma): o balão
// com a explicação é desenhado por último, por cima de tudo (desenharBalao).
int sobOMouse=-1;
bool sobOMouseGanha=false, sobOMouseNova=false;
void comecarQuadro() { sobOMouse=-1; }

// Uma medalha: fita em cima, círculo com a cor do grupo e o desenho no meio.
// Ainda não ganha: cinza e apagada.
void medalha(int id,float cx,float cy,float r,bool ganha,bool nova) {
    if(CheckCollisionPointCircle(mouseVirtual(),{cx,cy},r+4)) { sobOMouse=id; sobOMouseGanha=ganha; sobOMouseNova=nova; }
    Color cor=ganha?COR_DO_GRUPO[CONQUISTAS[id].grupo]:Color{70,78,96,255};
    Color fita=ganha?multiplicar(cor,0.65f):Color{45,50,62,255};
    DrawTriangle({cx-r*0.75f,cy-r*1.35f},{cx-r*0.05f,cy-r*0.4f},{cx-r*0.45f,cy-r*1.35f},fita);
    DrawTriangle({cx+r*0.45f,cy-r*1.35f},{cx+r*0.05f,cy-r*0.4f},{cx+r*0.75f,cy-r*1.35f},fita);
    DrawRectangleRec({cx-r*0.75f,cy-r*1.35f,r*0.3f,r*0.5f},fita); DrawRectangleRec({cx+r*0.45f,cy-r*1.35f,r*0.3f,r*0.5f},fita);
    if(nova) {
        float p=0.6f+0.4f*std::sin((float)GetTime()*4);
        DrawCircleGradient((int)cx,(int)cy,r*1.6f,Fade(cor,0.45f*p),Fade(cor,0.0f));
    }
    DrawCircleV({cx,cy},r,ganha?multiplicar(cor,0.35f):Color{26,30,40,255});
    DrawRing({cx,cy},r-3,r,0,360,40,cor);
    DrawRing({cx,cy},r-7,r-5,0,360,40,Fade(cor,0.5f));
    desenhoDe(id,cx,cy,r/12.0f,ganha?1.0f:0.0f);
    if(!ganha) DrawCircleV({cx,cy},r-4,{18,22,32,140});
    if(sobOMouse==id) DrawRing({cx,cy},r+2,r+5,0,360,40,Fade(TEXTO,0.8f));
}
// O balão da medalha embaixo do mouse: nome, como ganhar e se já tem.
void desenharBalao() {
    if(sobOMouse<0) return;
    const FichaDaConquista& c=CONQUISTAS[sobOMouse];
    Color cor=COR_DO_GRUPO[c.grupo];
    std::string estado=sobOMouseNova?"Ganha nesta partida (nova!)":sobOMouseGanha?"Você já tem esta medalha":"Ainda não ganha";
    float w=std::max({larguraDoTexto(c.nome,20)+larguraDoTexto(NOMES_DOS_GRUPOS[c.grupo],12)+24,
                      larguraDoTexto(c.comoGanhar,16),larguraDoTexto(estado,14)})+36;
    float h=96;
    Vector2 m=mouseVirtual();
    float x=m.x+18, y=m.y+18;
    if(x+w>Ui::LARGURA-10) x=m.x-18-w;
    if(y+h>Ui::ALTURA-10) y=m.y-18-h;
    Rectangle r={x,y,w,h};
    DrawRectangleRounded({r.x+4,r.y+5,r.width,r.height},0.18f,8,{0,0,0,110});
    DrawRectangleRounded(r,0.18f,8,{14,20,34,250});
    DrawRectangleRoundedLinesEx(r,0.18f,8,2.0f,cor);
    texto(c.nome,x+18,y+12,20,cor);
    texto(NOMES_DOS_GRUPOS[c.grupo],x+w-18-larguraDoTexto(NOMES_DOS_GRUPOS[c.grupo],12),y+17,12,Fade(cor,0.8f));
    texto(c.comoGanhar,x+18,y+42,16,TEXTO);
    texto(estado,x+18,y+68,14,sobOMouseGanha?Color{120,230,140,255}:APAGADO);
}
}

// Tela de todas as conquistas (menu principal).
namespace TelaDeConquistas {
// Quebra o texto em linhas de até "largura" pixels (no máximo 3).
std::vector<std::string> quebrar(const std::string& texto,float tam,float largura) {
    std::vector<std::string> linhas;
    std::string linha, palavra;
    auto fechar=[&](){
        if(palavra.empty()) return;
        std::string tentativa=linha.empty()?palavra:linha+" "+palavra;
        if(!linha.empty() && larguraDoTexto(tentativa,tam)>largura) { linhas.push_back(linha); linha=palavra; }
        else linha=tentativa;
        palavra.clear();
    };
    for(char c:texto) { if(c==' ') fechar(); else palavra+=c; }
    fechar();
    if(!linha.empty()) linhas.push_back(linha);
    if(linhas.size()>3) linhas.resize(3);
    return linhas;
}
void desenhar() {
    Medalhas::comecarQuadro();
    titulo("CONQUISTAS");
    int ganhas=conquistasGanhas();
    std::string n=std::to_string(ganhas)+" / "+std::to_string(NUM_CONQUISTAS);
    DrawRectangleRounded({1200,58,200,12},0.5f,4,{28,38,60,255});
    DrawRectangleRounded({1200,58,std::max(12.0f,200.0f*ganhas/NUM_CONQUISTAS),12},0.5f,4,OURO);
    texto(n,1200-14-larguraDoTexto(n,24),48,24,OURO);
    float y=118;
    const float CW=136, X0=108;
    for(int g=0;g<NUM_GRUPOS_DE_CONQUISTA;g++) {
        texto(NOMES_DOS_GRUPOS[g],X0+8,y,16,Medalhas::COR_DO_GRUPO[g]);
        DrawLineEx({X0+8+larguraDoTexto(NOMES_DOS_GRUPOS[g],16)+14,y+10},{X0+9*CW,y+10},1,Fade(Medalhas::COR_DO_GRUPO[g],0.35f));
        y+=24;
        int col=0;
        for(int id=0;id<NUM_CONQUISTAS;id++) {
            if(CONQUISTAS[id].grupo!=g) continue;
            float cx=X0+col*CW+CW/2;
            bool ganha=conquistaGanha(id);
            Medalhas::medalha(id,cx,y+56,30,ganha,false);
            float tam=15; while(tam>11 && larguraDoTexto(CONQUISTAS[id].nome,tam)>CW-6) tam-=1;
            textoCentro(CONQUISTAS[id].nome,cx,y+94,tam,ganha?TEXTO:APAGADO);
            std::vector<std::string> linhas=quebrar(CONQUISTAS[id].comoGanhar,12,CW-12);
            for(size_t k=0;k<linhas.size();k++)
                textoCentro(linhas[k],cx,y+114+k*15,12,ganha?Color{150,165,190,255}:Color{100,112,135,255});
            col++;
        }
        y+=178;
    }
    textoCentro("Esc ou Enter: voltar    (passe o mouse numa medalha para ver os detalhes)",720,858,16,{120,135,160,255});
    Medalhas::desenharBalao();
}
}

void mostrarConquistas(const Jogo* jogo) {
    (void)jogo;
    tocarSom(SOM_MENU);
    while(!WindowShouldClose()) {
        Efeitos::trocarDeTela(56);
        Ui::iniciarQuadro(); TelaDeConquistas::desenhar(); Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
        if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_BACKSPACE)||IsKeyPressed(KEY_SPACE)||
           (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Medalhas::sobOMouse<0)) break;
    }
    Ui::iniciarQuadro(); TelaDeConquistas::desenhar(); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
    tocarSom(SOM_MENU);
}

// O resumo do fim da partida (vitória ou derrota).
void desenharResumoDoFim(const Jogo& jogo) {
    Medalhas::comecarQuadro();
    const Jogador& j=jogo.jogador;
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[j.cor]));
    float t=(float)GetTime();
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{4,6,12,215});
    Rectangle p={60,30,1320,840};
    painel(p,{26,35,56,255},jogo.venceu?OURO:PERIGO);
    // ---- Faixa do resultado ----
    Color cor=jogo.venceu?OURO:PERIGO;
    textoCentro(jogo.venceu?"VITÓRIA":"FIM DA JORNADA",720,52,52,cor);
    std::string frase=jogo.venceu ? j.nome+" derrotou o Senhor das Profundezas. Valdoria está livre!"
                                  : j.nome+" caiu no andar "+std::to_string(jogo.andar)+": "+jogo.causaDaMorte+".";
    textoCentro(frase,720,116,21,TEXTO);
    desenharSprite(HEROI,190-44,78,5.5f,jogo.venceu?1.0f:0.55f);
    if(jogo.venceu) { desenharSprite(SENHOR,1250-44,78,5.5f,0.35f); seloVerde(1286,88); }
    else if(jogo.algoz>=0) {
        DrawCircleGradient(1250,122,70,Fade(PERIGO,0.25f+0.1f*std::sin(t*3)),Fade(PERIGO,0.0f));
        desenharSprite(imagemMonstro(jogo.algoz),1250-44,78,5.5f);
    }
    textoCentro(dificuldadeAtual().nome,720,148,17,corDaDificuldade(nivelDaDificuldade()));
    // ---- A trilha da partida ----
    Rectangle tr={100,190,1240,146};
    DrawRectangleRounded(tr,0.12f,8,{20,29,48,255});
    trilhaDosAndares(jogo,tr.x+60,tr.x+tr.width-60,tr.y+102);
    // ---- Os números ----
    int segundos=(int)(jogo.tempoMs/1000);
    std::string tempo=std::to_string(segundos/60)+":"+(segundos%60<10?"0":"")+std::to_string(segundos%60);
    struct Numero { std::string valor; const char* nome; int icone; };
    Numero nums[7]={
        {std::to_string(jogo.pontuacaoFinal),"pontos",0},{std::to_string(jogo.andarMaisFundo)+" / "+std::to_string(NUM_ANDARES),"andar",1},
        {std::to_string(j.nivel),"nível",2},{tempo,"tempo",3},{std::to_string(jogo.passos),"passos",4},
        {std::to_string(jogo.monstrosMortos),"monstros",5},{std::to_string(jogo.itensColetados),"itens",6}};
    float qw=(1240-6*12)/7.0f;
    for(int k=0;k<7;k++) {
        Rectangle q={100+k*(qw+12),352,qw,96};
        DrawRectangleRounded(q,0.18f,8,k==0?Color{48,40,20,255}:Color{20,29,48,255});
        if(k==0) DrawRectangleRoundedLinesEx(q,0.18f,8,1.6f,OURO);
        float cx=q.x+q.width/2;
        Color ic={160,175,205,255};
        switch(nums[k].icone) {
            case 0: icone(ARTE_ICONE_PONTOS.data(),cx-12,q.y+10,1.5f); break;
            case 1: DrawRectangle((int)cx-11,(int)q.y+12,22,22,{10,8,14,255}); for(int s=0;s<3;s++) DrawRectangle((int)cx-9+s*2,(int)q.y+15+s*6,18-s*4,3,{176,140,82,255}); break;
            case 2: icone(ARTE_ICONE_XP.data(),cx-12,q.y+10,1.5f); break;
            case 3: Icones::relogio(cx,q.y+22,11,ic); break;
            case 4: Icones::pegadas(cx,q.y+22,11,ic); break;
            case 5: Icones::caveira(cx,q.y+22,11,{220,215,200,255},k==0?Color{48,40,20,255}:Color{20,29,48,255}); break;
            default: Icones::joia(cx,q.y+22,10,{120,200,255,255}); break;
        }
        textoCentro(nums[k].valor,cx,q.y+40,24,k==0?OURO:TEXTO);
        textoCentro(nums[k].nome,cx,q.y+70,13,APAGADO);
    }
    if(jogo.posicaoNoRanking>=0)
        textoCentro(std::to_string(jogo.posicaoNoRanking+1)+"º no ranking!",100+qw/2,452,15,CIANO);
    // ---- Conquistas desta partida ----
    texto("CONQUISTAS DESTA PARTIDA",100,482,15,APAGADO);
    std::string total=std::to_string(conquistasGanhas())+" de "+std::to_string(NUM_CONQUISTAS)+" no total";
    texto(total,1340-larguraDoTexto(total,15),482,15,APAGADO);
    Rectangle ca={100,504,1240,150};
    DrawRectangleRounded(ca,0.12f,8,{20,29,48,255});
    std::vector<int> desta;
    for(int id=0;id<NUM_CONQUISTAS;id++) if(ganhouNaPartida(jogo,id)) desta.push_back(id);
    if(desta.empty()) textoCentro("Nenhuma conquista nesta partida. Fica para a próxima!",720,ca.y+62,18,APAGADO);
    float passo=std::min(150.0f,(ca.width-40)/std::max(1,(int)desta.size()));
    float x0=720-passo*(desta.size()-1)/2.0f;
    for(size_t k=0;k<desta.size();k++) {
        int id=desta[k];
        float cx=x0+k*passo;
        bool nova=novaNaPartida(jogo,id);
        Medalhas::medalha(id,cx,ca.y+64,28,true,nova);
        float tam=14; while(tam>10 && larguraDoTexto(CONQUISTAS[id].nome,tam)>passo-6) tam-=1;
        textoCentro(CONQUISTAS[id].nome,cx,ca.y+102,tam,TEXTO);
        if(nova) {
            Rectangle chip={cx-28,ca.y+124,56,18};
            DrawRectangleRounded(chip,0.5f,6,Fade(OURO,0.25f+0.15f*std::sin(t*5)));
            textoCentro("NOVA!",cx,chip.y+1,13,OURO);
        }
    }
    // ---- Ranking e botões ----
    texto("MELHORES PARTIDAS",100,672,15,APAGADO);
    std::vector<RegistroRanking> ranking=lerRanking();
    for(int i=0;i<(int)ranking.size() && i<5;i++) {
        const RegistroRanking& r=ranking[i];
        Color c=(i==jogo.posicaoNoRanking)?OURO:TEXTO;
        float ry=696+i*28;
        texto(std::to_string(i+1)+".",100,ry,18,c);
        texto(r.nome,130,ry,18,c);
        texto(std::to_string(r.pontos)+" pts",370,ry,18,c);
        texto("andar "+std::to_string(r.andar)+"   "+r.dificuldade+(r.venceu?"   VENCEU":""),510,ry,18,
              i==jogo.posicaoNoRanking?OURO:Color{170,182,205,255});
    }
    Rectangle b1={960,700,380,56}, b2={960,770,380,56};
    painel(b1,{70,56,20,255},OURO); textoCentro("N: nova partida",b1.x+190,b1.y+14,24,OURO);
    painel(b2,{24,32,50,255},{70,85,110,255}); textoCentro("Q: menu principal",b2.x+190,b2.y+14,24,TEXTO);
    registrarAtalho(b1,'n'); registrarAtalho(b2,'q');
    Medalhas::desenharBalao();
}

void mostrarBestiario(const Jogo* jogo) {
    int aba=0, escolhido=0;
    tocarSom(SOM_MENU);
    especieEscolhidaEm=GetTime(); letrasDoGritoTocadas=0; contadorDoGrito=0;
    auto clicou=[](Rectangle r){ return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseVirtual(),r); };
    while(!WindowShouldClose()) {
        Efeitos::trocarDeTela(55);
        Ui::iniciarQuadro(); desenharBestiario(jogo,aba,escolhido); Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}   // as letras não sobram para o jogo depois
        int total = cartasDaAba(aba);
        int antes=escolhido, abaAntes=aba;
        if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E) || IsKeyPressed(KEY_BACKSPACE) || clicou(VOLTAR_DO_BESTIARIO)) break;
        if(IsKeyPressed(KEY_TAB)) aba=(aba+1)%NUM_ABAS_DO_BESTIARIO;
        for(int a=0;a<NUM_ABAS_DO_BESTIARIO;a++) if(clicou(abaDoBestiario(a))) aba=a;
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressedRepeat(KEY_RIGHT)||IsKeyPressed(KEY_D)) escolhido=std::min(total-1,escolhido+1);
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressedRepeat(KEY_LEFT)||IsKeyPressed(KEY_A)) escolhido=std::max(0,escolhido-1);
        if(IsKeyPressed(KEY_DOWN)||IsKeyPressedRepeat(KEY_DOWN)||IsKeyPressed(KEY_S))
            escolhido=std::min(total-1,escolhido+COLUNAS_DO_BESTIARIO);
        if(IsKeyPressed(KEY_UP)||IsKeyPressedRepeat(KEY_UP)||IsKeyPressed(KEY_W))
            escolhido=std::max(0,escolhido-COLUNAS_DO_BESTIARIO);
        for(int i=0;i<total;i++) if(clicou(cartaDoBestiario(i))) escolhido=i;
        if(aba!=abaAntes) escolhido=0;
        if(escolhido!=antes || aba!=abaAntes) {
            tocarSom(SOM_MENU);
            especieEscolhidaEm=GetTime(); letrasDoGritoTocadas=0; contadorDoGrito=0;
        }
    }
    // Mais um quadro: a tecla que fechou (Esc, E) não chega ao jogo.
    Ui::iniciarQuadro(); desenharBestiario(jogo,aba,escolhido); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
    tocarSom(SOM_MENU);
}

// ---------------------------------------------------------------------------
// FALAS E HISTÓRIA: o texto aparece letra por letra, como alguém falando.
// Enter (ou espaço, ou clique) mostra o resto de uma vez; com tudo à mostra,
// passa para o próximo balão. Esc pula a conversa (ou a história) inteira.
// Cada letra toca um "bip" na voz de quem fala (som.h: tocarVoz).
// ---------------------------------------------------------------------------
namespace {
// Letras por segundo em cada velocidade das Opções (0 = aparece tudo na hora).
float letrasPorSegundo(float multiplicador=1.0f) {
    static const float VELOCIDADES[4]={22,38,75,0};
    return VELOCIDADES[std::clamp(opcoes().velocidadeDoTexto,0,3)]*multiplicador;
}
// Quantos bytes do texto já apareceram depois de "t" segundos (sem cortar
// uma letra acentuada no meio). Depois de . ! ? a "voz" respira um pouco, e
// depois de vírgula, menos.
size_t letrasReveladas(const std::string& s,double t,float porSegundo,bool& completo) {
    completo=true;
    if(porSegundo<=0) return s.size();
    double tempo=0;
    size_t i=0;
    while(i<s.size()) {
        unsigned char c=(unsigned char)s[i];
        size_t tam = c<0x80 ? 1 : (c>>5)==6 ? 2 : (c>>4)==14 ? 3 : 4;
        tempo+=1.0/porSegundo;
        if(tempo>t) { completo=false; return i; }
        i+=tam;
        if(c=='.'||c=='!'||c=='?') tempo+=0.30;
        else if(c==','||c==';'||c==':') tempo+=0.12;
    }
    return s.size();
}
// Junta as linhas em parágrafos (as linhas vazias separam um do outro).
std::vector<std::string> emParagrafos(const std::vector<std::string>& linhas) {
    std::vector<std::string> pars;
    std::string atual;
    for(const std::string& l:linhas) {
        if(l.empty() || l==QUEBRA_DE_PAGINA) { if(!atual.empty()) pars.push_back(atual); atual.clear(); continue; }
        atual += atual.empty() ? l : " "+l;
    }
    if(!atual.empty()) pars.push_back(atual);
    return pars;
}
// Desenha só os primeiros "bytes" do texto, já quebrado em linhas.
// "tremor" > 0: cada linha balança um pouco (a voz do chefe).
void textoRevelado(const std::vector<std::string>& linhas,size_t bytes,float x,float y,float tam,float entrelinha,
                   Color cor,bool centralizado=false,float tremor=0) {
    size_t usados=0;
    for(size_t i=0;i<linhas.size();i++) {
        if(usados>=bytes) break;
        size_t aqui=std::min(linhas[i].size(),bytes-usados);
        std::string parte=linhas[i].substr(0,aqui);
        float dx=0, dy=0;
        if(tremor>0) { dx=GetRandomValue(-100,100)/100.0f*tremor; dy=GetRandomValue(-100,100)/100.0f*tremor; }
        if(centralizado) {
            // Centraliza pela linha inteira: as letras não "andam" enquanto aparecem.
            float w=larguraDoTexto(linhas[i],tam);
            texto(parte,x-w/2+dx,y+i*entrelinha+dy,tam,cor);
        } else texto(parte,x+dx,y+i*entrelinha+dy,tam,cor);
        usados+=linhas[i].size()+1;   // +1: o espaço que virou quebra de linha
    }
}
bool teclaDeAvancar() {
    return IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)||IsKeyPressed(KEY_SPACE)||IsKeyPressed(KEY_Z)||
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
Voz vozDoNpc(int tipo) {
    switch(tipo) {
        case NPC_EXPLORADORA: return VOZ_ELDRA;
        case NPC_MINEIRO:     return VOZ_BORIN;
        case NPC_ESPIRITO:    return VOZ_ALDRIC;
        case NPC_COMERCIANTE: return VOZ_GARRO;
        case NPC_VIAJANTE:    return VOZ_TOBIAS;
    }
    return VOZ_NARRADOR;
}
// Toca a voz para as letras novas (espaços e pontuação não fazem som).
void vozDasLetras(const std::string& s,size_t antes,size_t agora,Voz voz,int& contador) {
    for(size_t i=antes;i<agora && i<s.size();i++) {
        unsigned char c=(unsigned char)s[i];
        if(c==' '||c=='.'||c==','||c=='!'||c=='?'||c=='"'||c==';'||c==':'||(c>=0x80&&c<0xC0)) continue;
        tocarVoz(voz,contador++);
    }
}

// Quem fala numa conversa: onde está no mapa, o desenho, o nome e a voz.
enum EstiloDoBalao { BALAO_PAPEL, BALAO_HEROI, BALAO_CHEFE };
struct Falante {
    float x, y;                 // casa no mapa
    const Sprite* arte;
    std::string nome;
    Color corDoNome;
    Voz voz;
    EstiloDoBalao estilo;
    // Fora do mapa (na batalha): onde fica, na tela, o meio, o alto da
    // cabeça e o pé de quem fala.
    bool naTela=false;
    float telaX=0, telaTopo=0, telaPe=0;
};
// Um balão: quem fala (-1 = narração), o texto e o prêmio (linha verde).
struct Trecho { int falante; std::string texto, premio; };

// As linhas das falas (npcs.cpp) viram trechos: entre aspas = a pessoa
// (falante 0); "> " na frente = o herói (falante 1); sem aspas = narração.
std::vector<Trecho> emTrechos(const std::vector<std::string>& linhas) {
    std::vector<Trecho> trechos;
    for(std::string par:emParagrafos(linhas)) {
        Trecho t={0,"",""};
        if(par.rfind("> ",0)==0) { t.falante=1; par=par.substr(2); }
        else if(par[0]!='"') t.falante=-1;
        size_t abre=par.rfind("  (");
        if(abre!=std::string::npos && par.back()==')') { t.premio=par.substr(abre+2); par=par.substr(0,abre); }
        for(char c:par) if(c!='"') t.texto+=c;   // sem aspas: o balão já mostra que é fala
        trechos.push_back(t);
    }
    return trechos;
}

// Canto de cima à esquerda de uma casa, na tela (a mesma conta da câmera do mapa).
Vector2 casaNaTela(float x,float y) {
    return {(x*TAM-cameraX*TAM)*cameraZoom,(y*TAM-cameraY*TAM)*cameraZoom};
}

// O balão, com a "ponta" apontando para quem fala.
void balaoDeFala(const Falante& f,const Trecho& t,size_t bytes,bool completo,int pagina,int paginas) {
    const float W=640, TAM_TEXTO=22, LINHA=31;
    Color papel={242,234,212,255}, contorno={58,44,34,255}, tinta={46,36,30,255}, fundoDaFoto={226,214,184,255};
    if(f.estilo==BALAO_HEROI) { papel={232,237,246,255}; contorno={44,52,78,255}; tinta={30,36,56,255}; fundoDaFoto={208,216,232,255}; }
    if(f.estilo==BALAO_CHEFE) { papel={34,8,12,245}; contorno={190,40,50,255}; tinta={255,204,196,255}; fundoDaFoto={70,14,20,255}; }
    std::vector<std::string> linhas=quebrarLinhas(t.texto,TAM_TEXTO,W-150);
    float h=std::max(118.0f,46+linhas.size()*LINHA+(t.premio.empty()?0:LINHA)+30);
    Vector2 topo=casaNaTela(f.x,f.y), pe=casaNaTela(f.x,f.y+1);
    float sx=casaNaTela(f.x+0.5f,f.y).x;
    if(f.naTela) { topo.y=f.telaTopo; pe.y=f.telaPe; sx=f.telaX; }
    bool emCima = topo.y-22-h >= 24;
    float by = emCima ? topo.y-22-h : pe.y+22;
    float bx = std::clamp(sx-W*0.32f,24.0f,Ui::LARGURA-24.0f-W);
    Rectangle r={bx,by,W,h};
    if(f.estilo==BALAO_CHEFE) DrawCircleGradient((int)(r.x+r.width/2),(int)(r.y+r.height/2),r.width*0.62f,{150,20,30,70},{0,0,0,0});
    DrawRectangleRounded({r.x+5,r.y+7,r.width,r.height},0.18f,10,{0,0,0,110});
    float px=std::clamp(sx,r.x+40,r.x+r.width-40);
    Vector2 a={px-16,emCima?r.y+r.height-2:r.y+2}, b={px+16,a.y}, c={sx,emCima?topo.y-4:pe.y+4};
    trianguloCheio(a,b,c,contorno);
    DrawRectangleRounded(r,0.18f,10,papel);
    DrawRectangleRoundedLinesEx(r,0.18f,10,3.0f,contorno);
    Vector2 a2={a.x+4,emCima?a.y-3:a.y+3}, b2={b.x-4,b.y+(emCima?-3:3)}, c2={c.x,c.y+(emCima?-6:6)};
    trianguloCheio(a2,b2,c2,papel);
    // Retrato e nome.
    Rectangle foto={r.x+16,r.y+18,96,96};
    DrawRectangleRounded(foto,0.15f,6,fundoDaFoto);
    sprite(f.arte->data(),foto.x+8,foto.y+6,5.0f);
    float wn=larguraDoTexto(f.nome,19)+28;
    Rectangle etiqueta={r.x+122,r.y-15,wn,30};
    DrawRectangleRounded(etiqueta,0.5f,8,f.estilo==BALAO_CHEFE?Color{20,4,6,255}:contorno);
    DrawRectangleRoundedLinesEx(etiqueta,0.5f,8,2.0f,f.corDoNome);
    texto(f.nome,etiqueta.x+14,etiqueta.y+5,19,f.corDoNome);
    // O texto, aparecendo aos poucos.
    float tx=r.x+128, ty=r.y+28;
    textoRevelado(linhas,bytes,tx,ty,TAM_TEXTO,LINHA,tinta,false,f.estilo==BALAO_CHEFE?1.2f:0.0f);
    if(completo && !t.premio.empty()) texto(t.premio,tx,ty+linhas.size()*LINHA+4,TAM_TEXTO-1,{36,120,62,255});
    if(paginas>1) texto(std::to_string(pagina+1)+"/"+std::to_string(paginas),r.x+r.width-62,r.y+r.height-28,15,
                        f.estilo==BALAO_CHEFE?Color{170,90,90,255}:Color{150,128,100,255});
    if(completo) {
        float pulo=std::fabs(std::sin((float)GetTime()*5.0f))*5;
        float cx=r.x+r.width-24, cy=r.y+r.height-22+pulo;
        trianguloCheio({cx-8,cy-6},{cx+8,cy-6},{cx,cy+4},f.estilo==BALAO_CHEFE?Color{230,60,60,255}:Color{170,110,40,255});
    }
}

// Narração: uma caixa escura no alto da tela, na voz do narrador.
void caixaDeNarracao(const Trecho& t,size_t bytes,bool completo) {
    const float W=780, TAM_TEXTO=21, LINHA=30;
    std::vector<std::string> linhas=quebrarLinhas(t.texto,TAM_TEXTO,W-60);
    float h=34+linhas.size()*LINHA+(t.premio.empty()?0:LINHA)+16;
    Rectangle r={(Ui::LARGURA-W)/2,56,W,h};
    DrawRectangleRounded({r.x+4,r.y+6,r.width,r.height},0.2f,10,{0,0,0,120});
    DrawRectangleRounded(r,0.2f,10,{14,18,30,238});
    DrawRectangleRoundedLinesEx(r,0.2f,10,2.0f,{120,108,86,255});
    textoRevelado(linhas,bytes,r.x+30,r.y+18,TAM_TEXTO,LINHA,{220,210,186,255});
    if(completo && !t.premio.empty()) texto(t.premio,r.x+30,r.y+18+linhas.size()*LINHA+2,TAM_TEXTO-1,{120,220,140,255});
    if(completo) {
        float pulo=std::fabs(std::sin((float)GetTime()*5.0f))*4;
        float cx=r.x+r.width-24, cy=r.y+r.height-18+pulo;
        trianguloCheio({cx-7,cy-5},{cx+7,cy-5},{cx,cy+4},{200,160,90,255});
    }
}

// Uma conversa inteira. "fundo" desenha o que fica atrás (normalmente o
// mapa); recebe os segundos desde o começo do balão.
void conversar(const std::vector<Falante>& falantes,const std::vector<Trecho>& trechos,
               const std::function<void(double)>& fundo,float velocidade=1.0f) {
    bool pular=false;
    for(size_t p=0;p<trechos.size() && !pular && !WindowShouldClose();p++) {
        const Trecho& t=trechos[p];
        bool narracao = t.falante<0 || t.falante>=(int)falantes.size();
        Voz voz = narracao ? VOZ_NARRADOR : falantes[t.falante].voz;
        double inicio=GetTime();
        bool tudo=false;
        size_t antes=0;
        int contador=0;
        while(!WindowShouldClose()) {
            bool completo;
            size_t bytes = tudo ? t.texto.size()
                         : letrasReveladas(t.texto,GetTime()-inicio,letrasPorSegundo(velocidade*ritmoDaVoz(voz)),completo);
            if(tudo) completo=true;
            if(bytes>antes) { vozDasLetras(t.texto,antes,bytes,voz,contador); antes=bytes; }
            Ui::iniciarQuadro();
            fundo(GetTime()-inicio);
            if(narracao) caixaDeNarracao(t,bytes,completo);
            else balaoDeFala(falantes[t.falante],t,bytes,completo,(int)p,(int)trechos.size());
            texto("Enter: continuar    Esc: pular",24,Ui::ALTURA-36.0f,15,{150,165,185,200});
            Ui::terminarQuadro();
            while(GetCharPressed()!=0) {}
            if(IsKeyPressed(KEY_ESCAPE)) { pular=true; break; }
            if(teclaDeAvancar()) {
                if(!completo) tudo=true;
                else { tocarSom(SOM_MENU); break; }
            }
        }
    }
}
// O herói como falante (balão azulado, nome em dourado).
Falante falanteHeroi(const Jogo& jogo) {
    return {jogadorSuave.x,jogadorSuave.y,&HEROI,jogo.jogador.nome,OURO,VOZ_HEROI,BALAO_HEROI};
}
// Depois de uma conversa: mais um quadro, para a tecla que fechou não chegar
// ao jogo (o Esc abriria a pausa).
void engolirTeclas(const Jogo& jogo) {
    Ui::iniciarQuadro(); mapa(jogo); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
}
}

void mostrarFala(const Jogo& jogo,int indice,const std::vector<std::string>& linhas) {
    const Andar& andar=andarAtual(jogo);
    if(indice<0 || indice>=andar.numNpcs) return;
    const Npc& npc=andar.npcs[indice];
    std::string nome=FICHAS_DOS_NPCS[npc.tipo].nome;
    nome=nome.substr(0,nome.find(','));
    std::vector<Falante> falantes={
        {(float)npc.x,(float)npc.y,&imagemNpc(npc.tipo),nome,paraCor(FICHAS_DOS_NPCS[npc.tipo].cor),vozDoNpc(npc.tipo),BALAO_PAPEL},
        falanteHeroi(jogo),
    };
    esconderCartoes=true;
    // Cada pessoa tem a sua música; no fim da conversa, volta a da masmorra.
    static const Musica TEMA_DE[NUM_TIPOS_DE_NPC]={MUSICA_ELDRA,MUSICA_BORIN,MUSICA_ALDRIC,MUSICA_GARRO,MUSICA_TOBIAS};
    if(npc.tipo>=0 && npc.tipo<NUM_TIPOS_DE_NPC) tocarMusica(TEMA_DE[npc.tipo]);
    conversar(falantes,emTrechos(linhas),[&](double){
        mapa(jogo);
        DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{4,6,14,70});
    });
    tocarMusica(MUSICA_MASMORRA);
    engolirTeclas(jogo);
    esconderCartoes=false;
}

// ---- Cenas: o gnomo vigia e o Senhor das Profundezas -----------------------
namespace {
float suave(float t) { t=std::clamp(t,0.0f,1.0f); return t*t*(3-2*t); }
// Faixas pretas de cinema (0 = sem faixas, 1 = faixas inteiras).
void faixasDeCinema(float quanto) {
    float h=92*suave(quanto);
    if(h<1) return;
    DrawRectangle(-200,-200,Ui::LARGURA+400,(int)h+200,BLACK);
    DrawRectangle(-200,Ui::ALTURA-(int)h,Ui::LARGURA+400,(int)h+200,BLACK);
}
// "!" sobre a cabeça de um monstro (perceberam o jogador).
void exclamacao(float x,float y,float tempo,Color cor) {
    float pulo=std::max(0.0f,1.0f-tempo*4)*10;
    textoCentro("!",x,y-40-pulo,36,cor);
}
// Junta a câmera da cena com a câmera normal (t: 0 = no herói, 1 = no alvo).
void camera(float alvoX,float alvoY,float zoomNoAlvo,float t) {
    float s=suave(t);
    float z0=zoomDoMapa();
    cameraDaCena=true;
    cenaX=jogadorSuave.x+(alvoX-jogadorSuave.x)*s;
    cenaY=jogadorSuave.y+(alvoY-jogadorSuave.y)*s;
    cenaZoom=z0+(zoomNoAlvo-z0)*s;
}
bool pulouACena() {
    return IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)||IsKeyPressed(KEY_SPACE);
}
}

void cenaDaCorneta(const Jogo& jogo,int indice,bool primeira) {
    const Andar& a=andarAtual(jogo);
    if(indice<0 || indice>=a.numMonstros) return;
    const Monstro& g=a.monstros[indice];
    esconderCartoes=true;
    // Linha do tempo (segundos).
    const double CORNETA=1.0, FIM=primeira?5.0:5.4, VOLTA=FIM-0.8;
    double inicio=GetTime();
    bool tocou=false, riu=false, riu2=false;
    std::string fala = primeira ? "Peguem ele!" : "Agora você não escapa!";
    double tFala = primeira ? 2.9 : 1.9;
    size_t vozTocada=0; int contador=0;
    float zoomNoGnomo=std::min(3.0f,zoomEscolhido()*1.8f);
    while(!WindowShouldClose()) {
        double t=GetTime()-inicio;
        bool pular=pulouACena();
        if(t>=CORNETA && !tocou) {
            tocou=true;
            tocarSom(SOM_CORNETA);
            efeitoNoMapa(jogo,EFEITO_CORNETA,g.x,g.y,0);
            Efeitos::tremer(5.0f,0.4);
        }
        if(primeira && t>=2.0 && !riu) { riu=true; tocarSom(SOM_RISADA_GNOMO); }
        if(!primeira && t>=3.3 && !riu) { riu=true; tocarSom(SOM_RISADA_GNOMO); }
        if(!primeira && t>=3.8 && !riu2) { riu2=true; tocarSomComTom(SOM_RISADA_GNOMO,0.92f); }
        if(pular || t>=FIM) break;
        float ida=(float)(t/0.9), volta=(float)((t-VOLTA)/0.8);
        camera(monstrosSuaves[indice].x,monstrosSuaves[indice].y,zoomNoGnomo,t<VOLTA?ida:1-volta);
        Ui::iniciarQuadro();
        mapa(jogo);
        Vector2 cabeca=casaNaTela(monstrosSuaves[indice].x+0.5f,monstrosSuaves[indice].y);
        // O gnomo percebe você.
        if(t>=0.6 && t<CORNETA+0.3) exclamacao(cabeca.x,cabeca.y,(float)(t-0.6),OURO);
        // Os monstros que ouviram e estão à vista também ganham um "!".
        if(t>=CORNETA+0.25) {
            for(int i=0;i<a.numMonstros;i++) {
                const Monstro& m=a.monstros[i];
                if(i==indice || !m.vivo || m.estado!=CACANDO || m.turnosSemVerJogador>=0 || !monstroAVista(jogo,m)) continue;
                Vector2 c=casaNaTela(monstrosSuaves[i].x+0.5f,monstrosSuaves[i].y);
                exclamacao(c.x,c.y,(float)(t-CORNETA-0.25),PERIGO);
            }
        }
        // Risada e fala em balõezinhos.
        bool mostraRisada = primeira ? (t>=2.0 && t<tFala) : (t>=3.3 && t<VOLTA);
        if(mostraRisada) balaozinho(primeira?"Hihihi!":"Hihihihi!",cabeca.x,cabeca.y,1.0f);
        if(t>=tFala && t<(primeira?VOLTA:3.3)) {
            bool completo;
            size_t bytes=letrasReveladas(fala,t-tFala,30,completo);
            if(bytes>vozTocada) { vozDasLetras(fala,vozTocada,bytes,VOZ_GNOMO,contador); vozTocada=bytes; }
            if(bytes>0) balaozinho(fala.substr(0,bytes),cabeca.x,cabeca.y,1.0f);
        }
        faixasDeCinema(t<VOLTA ? (float)(t/0.4) : (float)((FIM-t)/0.5));
        if(t>0.3) texto("Enter: pular",Ui::LARGURA-150.0f,Ui::ALTURA-34.0f,15,{150,150,160,220});
        Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
    }
    if(!tocou) { tocarSom(SOM_CORNETA); efeitoNoMapa(jogo,EFEITO_CORNETA,g.x,g.y,0); }
    cameraDaCena=false;
    engolirTeclas(jogo);
    esconderCartoes=false;
}

void cenaDoChefe(const Jogo& jogo,int indice,const std::vector<std::string>& falas,bool primeira) {
    const Andar& a=andarAtual(jogo);
    if(indice<0 || indice>=a.numMonstros) return;
    esconderCartoes=true;
    tocarMusica(SEM_MUSICA);
    float bx=monstrosSuaves[indice].x, by=monstrosSuaves[indice].y;
    float zoomNoChefe=std::min(3.2f,zoomEscolhido()*2.0f);
    // Linha do tempo da primeira vez (segundos): a câmera vai devagar, as
    // luzes se apagam, silêncio, os olhos acendem e ele ruge.
    const double CHEGA = primeira ? 1.8 : 0.8;
    const double OLHOS = primeira ? 3.4 : 0.9, RUGIDO = primeira ? 3.6 : 99, FALA = primeira ? 4.8 : 1.2;
    double apagou[3]={1.8,2.25,2.7};
    bool tocouApagar[3]={false,false,false}, rugiu=false;
    double inicio=GetTime();
    auto escuro=[&](double t){
        if(!primeira) return 150.0f;
        float e=(float)std::clamp(t/1.6,0.0,1.0)*50;
        for(int k=0;k<3;k++) if(t>=apagou[k]) e+=35;
        return e;
    };
    // O que fica na tela (o mapa escuro e o chefe crescendo, com os olhos acesos).
    auto desenharCena=[&](double t){
        float ida=(float)(t/CHEGA);
        camera(bx,by,zoomNoChefe,ida);
        mapa(jogo);
        DrawRectangle(-200,-200,Ui::LARGURA+400,Ui::ALTURA+400,{0,0,0,(unsigned char)escuro(t)});
        Vector2 centro=casaNaTela(bx+0.5f,by+0.5f);
        float cresce = t<RUGIDO ? 1.0f : 1.0f+0.8f*suave((float)((t-RUGIDO)/0.6));
        if(!primeira) cresce=1.6f;
        float pulso=0.5f+0.5f*std::sin((float)GetTime()*4);
        if(t>=OLHOS) {
            DrawCircleGradient((int)centro.x,(int)centro.y,(int)(110*cameraZoom*cresce/1.5f),
                               {170,20,30,(unsigned char)(90+50*pulso)},{0,0,0,0});
            float p=1.25f*cameraZoom*cresce;
            float sx=centro.x-8*p, sy=centro.y-8*p-2*cameraZoom;
            sprite(SENHOR.data(),sx,sy,p);
            for(int olho=0;olho<2;olho++) {   // os olhos (pixels amarelos do desenho)
                Vector2 c={sx+(olho==0?5.5f:10.5f)*p,sy+5.5f*p};
                DrawCircleGradient((int)c.x,(int)c.y,(int)(p*3.2f),{255,60,40,(unsigned char)(200+55*pulso)},{255,0,0,0});
                DrawCircleV(c,p*0.75f,{255,240,170,255});
            }
        }
        faixasDeCinema((float)(t/0.5));
    };
    // 1) A câmera chega, as luzes se apagam, ele acorda e ruge.
    while(!WindowShouldClose()) {
        double t=GetTime()-inicio;
        for(int k=0;k<3;k++) if(primeira && t>=apagou[k] && !tocouApagar[k]) { tocouApagar[k]=true; tocarSom(SOM_TOCHA_APAGA); }
        if(t>=OLHOS) tocarMusica(MUSICA_CHEFE_CENA);   // o coração começa a bater
        if(t>=RUGIDO && !rugiu) { rugiu=true; tocarSom(SOM_RUGIDO); Efeitos::tremer(12.0f,1.1); }
        if(t>=FALA || pulouACena()) break;
        Ui::iniciarQuadro();
        desenharCena(t);
        if(t>0.3) texto("Enter: pular",Ui::LARGURA-150.0f,Ui::ALTURA-34.0f,15,{150,150,160,220});
        Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
    }
    // 2) As falas, com a voz dele.
    tocarMusica(MUSICA_CHEFE_CENA);
    double tFixo=std::max(GetTime()-inicio,FALA+0.7);
    std::vector<Falante> falantes={{bx,by,&SENHOR,"Senhor das Profundezas",{255,90,90,255},VOZ_CHEFE,BALAO_CHEFE}};
    conversar(falantes,emTrechos(falas),[&](double){ desenharCena(tFixo); });
    // 3) A tela racha em vermelho e a batalha começa.
    tocarSom(SOM_RACHADURA);
    Efeitos::tremer(8.0f,0.4);
    double racha=GetTime();
    while(!WindowShouldClose() && GetTime()-racha<0.6) {
        float f=(float)((GetTime()-racha)/0.6);
        Ui::iniciarQuadro();
        desenharCena(tFixo);
        Vector2 c={Ui::LARGURA/2.0f,Ui::ALTURA/2.0f};
        for(int k=0;k<9;k++) {   // rachaduras saindo do meio
            float ang=k*0.7f+0.3f, comp=900*suave(f*1.6f);
            Vector2 p1=c, p2={c.x+std::cos(ang)*comp*0.5f+std::sin(k*3.1f)*40,c.y+std::sin(ang)*comp*0.5f};
            Vector2 p3={c.x+std::cos(ang)*comp,c.y+std::sin(ang)*comp};
            DrawLineEx(p1,p2,4,{255,70,60,255}); DrawLineEx(p2,p3,3,{255,120,90,220});
        }
        DrawRectangle(-200,-200,Ui::LARGURA+400,Ui::ALTURA+400,{180,10,20,(unsigned char)(200*f)});
        Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
    }
    cameraDaCena=false;
    engolirTeclas(jogo);
    esconderCartoes=false;
}

// v1.24: entrou na sala de um mini-chefe. O portão desce atrás do herói, a
// câmera vai até o mini-chefe, ele acorda (maior, com um brilho), os 2
// ajudantes acordam com um "!" (os esqueletos saem da terra), aparece o nome
// dele num cartaz e ele fala. Na volta (depois de perder), a cena é curta.
void cenaDoMiniChefe(const Jogo& jogo,int indice,bool primeira) {
    const Andar& a=andarAtual(jogo);
    if(indice<0 || indice>=a.numMonstros) return;
    const Monstro& m=a.monstros[indice];
    int sala=salaDoMiniChefe(m.tipo);
    if(sala<0) return;
    static const Color CORES[4]={{250,200,90,255},{190,120,240,255},{120,255,150,255},{255,130,50,255}};
    static const Voz VOZES[4]={VOZ_GOBLIN,VOZ_ARANHA,VOZ_ESQUELETO,VOZ_OGRO};
    static const Som SONS[4]={SOM_CORNETA,SOM_FOLHAS,SOM_REMONTA,SOM_RUGIDO};
    Color cor=CORES[sala];
    std::string nome=TIPOS_DE_MONSTRO[m.tipo].nome, nomeGrande=nome;
    for(char& c:nomeGrande) if(c>='a' && c<='z') c=(char)(c-'a'+'A');
    const SalaDoChefe& s=a.salaDoChefe;
    float bx=(float)m.x, by=(float)m.y;
    float zoomNoChefe=std::min(2.6f,zoomEscolhido()*1.3f);
    // Linha do tempo (segundos). Na volta, tudo mais rápido e sem o cartaz.
    const double CAI=0.15, BATE=0.43, CAMERA=primeira?1.0:0.5, CHEGA=primeira?1.3:0.6;
    const double ACORDA=primeira?2.2:1.1, AJUDANTES=primeira?2.9:1.2, CARTAZ=primeira?2.6:99, FALA=primeira?4.4:1.6;
    bool bateu=false, rugiu=false, levantou=false;
    esconderCartoes=true;
    tocarMusica(MUSICA_CHEFE_CENA);
    double inicio=GetTime();
    auto desenharCena=[&](double t){
        portaoDescendo=suave((float)((t-CAI)/0.28));
        if(t>CAMERA) camera(bx,by,zoomNoChefe,(float)((t-CAMERA)/CHEGA)); else cameraDaCena=false;
        bool levantando = sala==ALTAR_DO_NECROMANTE && t<AJUDANTES+0.5;
        esqueletosEnterrados=levantando;
        mapa(jogo);
        // Poeira do portão batendo no chão.
        if(t>BATE && t<BATE+0.9 && s.existe) {
            float f=(float)((t-BATE)/0.9);
            Vector2 g=casaNaTela(s.portaoX+0.5f,s.portaoY+(s.invertida?0.1f:0.9f));
            for(int k=0;k<10;k++) {
                float ang=3.14159f*(k/9.0f), d=(20+40*f)*cameraZoom/1.5f;
                DrawCircleV({g.x+std::cos(ang)*d*1.4f,g.y-std::sin(ang)*d*0.5f},(6+6*f)*cameraZoom/1.5f,{170,160,150,(unsigned char)(150*(1-f))});
            }
        }
        DrawRectangle(-200,-200,Ui::LARGURA+400,Ui::ALTURA+400,{0,0,0,(unsigned char)(std::clamp((t-CAMERA)/1.0,0.0,1.0)*70)});
        // O mini-chefe acorda: maior e com um brilho na cor dele.
        if(t>ACORDA) {
            Vector2 centro=casaNaTela(bx+0.5f,by+0.5f);
            float cresce=1.0f+0.6f*suave((float)((t-ACORDA)/0.4));
            float pulso=0.5f+0.5f*std::sin((float)GetTime()*4);
            DrawCircleGradient((int)centro.x,(int)centro.y,(int)(95*cameraZoom*cresce/1.5f),{cor.r,cor.g,cor.b,(unsigned char)(70+40*pulso)},{0,0,0,0});
            float p=1.25f*cameraZoom*cresce;
            sprite(imagemMonstro(m.tipo).data(),centro.x-8*p,centro.y-8*p-2*cameraZoom,p);
        }
        // Os ajudantes acordam com um "!" (os esqueletos saem de dentro da terra).
        if(t>AJUDANTES) {
            for(int i=0;i<a.numMonstros;i++) {
                const Monstro& h=a.monstros[i];
                if(i==indice || !h.vivo || !h.guarda || !dentroDaSalaDoChefe(a,h.x,h.y)) continue;
                Vector2 c=casaNaTela(h.x+0.5f,h.y+0.5f);
                float f=suave((float)((t-AJUDANTES)/0.5));
                float p=1.25f*cameraZoom*(1.0f+0.35f*f);
                bool esqueleto=tipoDe(h).simbolo=='e';
                DrawCircleGradient((int)c.x,(int)c.y,(int)(70*cameraZoom/1.5f),{cor.r,cor.g,cor.b,(unsigned char)(110*f)},{0,0,0,0});
                if(esqueleto) {
                    float sobe=(1-f)*16*p;
                    Ui::recortar({0,0,(float)Ui::LARGURA,c.y+8*p});
                    sprite(imagemMonstro(h.tipo).data(),c.x-8*p,c.y-8*p-2*cameraZoom+sobe,p);
                    Ui::pararDeRecortar();
                    for(int k=0;k<6;k++) {
                        float ang=3.14159f*(k/5.0f);
                        DrawCircleV({c.x+std::cos(ang)*p*7*f,c.y+6*p-std::sin(ang)*p*4*f},p*0.9f*(1-f),{150,140,120,(unsigned char)(200*(1-f))});
                    }
                } else {
                    sprite(imagemMonstro(h.tipo).data(),c.x-8*p,c.y-8*p-2*cameraZoom,p);
                }
                if(t>AJUDANTES+0.4) exclamacao(c.x,c.y-8*p+10,(float)(t-AJUDANTES-0.4),{255,236,140,255});
            }
        }
        faixasDeCinema((float)((t-CAMERA+0.1)/0.5));
        // O nome dele, como num cartaz.
        if(t>CARTAZ && t<FALA) {
            float ap=(float)std::clamp(std::min((t-CARTAZ)/0.4,(FALA-t)/0.3),0.0,1.0);
            unsigned char A=(unsigned char)(255*ap);
            float y=Ui::ALTURA-230.0f;
            DrawRectangleGradientH(220,(int)y-14,500,118,{0,0,0,0},{0,0,0,(unsigned char)(170*ap)});
            DrawRectangleGradientH(720,(int)y-14,500,118,{0,0,0,(unsigned char)(170*ap)},{0,0,0,0});
            DrawLineEx({420,y-8},{1020,y-8},2,{cor.r,cor.g,cor.b,A});
            textoCentro(nomeGrande,720,y,64,{250,236,200,A});
            textoCentro(tituloDoMiniChefe(sala),720,y+70,26,{cor.r,cor.g,cor.b,A});
            DrawLineEx({420,y+106},{1020,y+106},2,{cor.r,cor.g,cor.b,A});
        }
    };
    // 1) O portão, a câmera, o despertar e o cartaz.
    while(!WindowShouldClose()) {
        double t=GetTime()-inicio;
        if(t>=BATE && !bateu) { bateu=true; tocarSom(SOM_PORTA_FECHANDO); Efeitos::tremer(10.0f,0.5); }
        if(t>=ACORDA && !rugiu) { rugiu=true; tocarSom(SONS[sala]); if(sala==FORJA_DO_GOLEM) Efeitos::tremer(8.0f,0.8); }
        if(t>=AJUDANTES && !levantou) { levantou=true; if(sala==ALTAR_DO_NECROMANTE) tocarSom(SOM_REMONTA); }
        if(t>=FALA || (t>0.6 && pulouACena())) break;
        Ui::iniciarQuadro();
        desenharCena(t);
        if(t>0.3) texto("Enter: pular",Ui::LARGURA-150.0f,Ui::ALTURA-34.0f,15,{150,150,160,220});
        Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
    }
    // 2) A fala, com a voz dele (o Golem só ruge: vira narração).
    double tFixo=std::max(GetTime()-inicio,FALA+0.7);
    std::vector<Falante> falantes={{bx,by,&imagemMonstro(m.tipo),nome,cor,VOZES[sala],BALAO_CHEFE}};
    conversar(falantes,emTrechos({falaDoMiniChefe(sala,primeira)}),[&](double){ desenharCena(tFixo); });
    // 3) Um clarão e a luta começa.
    tocarSom(SOM_RACHADURA);
    double flash=GetTime();
    while(!WindowShouldClose() && GetTime()-flash<0.35) {
        float f=(float)((GetTime()-flash)/0.35);
        Ui::iniciarQuadro();
        desenharCena(tFixo);
        DrawRectangle(-200,-200,Ui::LARGURA+400,Ui::ALTURA+400,{cor.r,cor.g,cor.b,(unsigned char)(180*f)});
        Ui::terminarQuadro();
        while(GetCharPressed()!=0) {}
    }
    esqueletosEnterrados=false;
    portaoDescendo=1.0f;
    cameraDaCena=false;
    engolirTeclas(jogo);
    esconderCartoes=false;
}

void falaDoChefeNaBatalha(const Jogo& jogo,const Batalha& b,int inimigo,const std::string& fala) {
    if(inimigo<0 || inimigo>=b.numInimigos) return;
    float x,y,p; posicaoDoSprite(b,inimigo+1,x,y,p);
    Falante chefe={0,0,&SENHOR,"Senhor das Profundezas",{255,90,90,255},VOZ_CHEFE,BALAO_CHEFE};
    chefe.naTela=true; chefe.telaX=x+8*p; chefe.telaTopo=y+2*p; chefe.telaPe=y+16*p;
    conversar({chefe},emTrechos({"\""+fala+"\""}),[&](double){ telaBatalha(jogo,b); });
    Ui::iniciarQuadro(); telaBatalha(jogo,b); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
}

// ---- A história do começo ---------------------------------------------------
namespace {
// Uma cena para cada parágrafo. "t" = segundos desde que a cena começou.
void cenaDaHistoria(int cena,double t,bool& bateuAPorta) {
    float cx=Ui::LARGURA/2.0f, cy=290;
    float tf=(float)t;
    switch(cena) {
    case 0: {   // O Senhor das Profundezas, em brasa.
        float pulso=0.5f+0.5f*std::sin(tf*2.2f);
        DrawCircleGradient((int)cx,(int)cy,250,{170,30,40,(unsigned char)(70+50*pulso)},{0,0,0,0});
        for(int k=0;k<46;k++) {
            float x=cx+std::sin(k*12.9898f)*300+std::sin(tf*0.8f+k)*12;
            float subida=std::fmod(tf*(35+k%5*9)+k*53.0f,420.0f);
            float y=cy+200-subida;
            unsigned char alfa=(unsigned char)(220*(1-subida/420.0f));
            DrawCircleV({x,y},1.5f+(k%3),{255,(unsigned char)(120+k%4*25),40,alfa});
        }
        sprite(SENHOR.data(),cx-8*14,cy-8*14-10,14.0f);
        break;
    }
    case 1: {   // Três aventureiros descem... e somem, um de cada vez.
        DrawCircleGradient((int)cx,(int)cy+40,260,{60,80,120,40},{0,0,0,0});
        for(int k=0;k<3;k++) {
            float some=std::clamp((tf-1.0f-k*1.1f)/0.9f,0.0f,1.0f);
            unsigned char alfa=(unsigned char)(255*(1-some));
            if(!alfa) continue;
            float x=cx-230+k*230-8*8, y=cy-8*8-20+some*30;
            silhueta(HEROI.data(),x,y,8.0f,{70,82,110,alfa});
        }
        break;
    }
    case 2: {   // Você, com a tocha.
        float balanco=std::sin(tf*2.0f)*4;
        DrawCircleGradient((int)cx,(int)cy+10,240,{250,190,91,(unsigned char)(55+15*std::sin(tf*7.0f))},{0,0,0,0});
        sprite(HEROI.data(),cx-8*13,cy-8*13+balanco,13.0f);
        break;
    }
    default: {  // As portas da entrada se fecham.
        Rectangle arco={cx-170,cy-180,340,360};
        Rectangle muro={arco.x-60,arco.y-46,arco.width+120,arco.height+46};
        DrawRectangleRec(muro,{46,48,58,255});   // muro de pedra
        for(float y=muro.y;y<muro.y+muro.height;y+=46) {
            DrawLineEx({muro.x,y},{muro.x+muro.width,y},2,{30,32,40,255});
            int linha=(int)((y-muro.y)/46);
            for(float x=muro.x+(linha%2)*35+35;x<muro.x+muro.width;x+=70)
                DrawLineEx({x,y},{x,std::min(y+46,muro.y+muro.height)},2,{30,32,40,255});
        }
        DrawRectangleRec(arco,{6,8,14,255});
        DrawCircleGradient((int)cx,(int)(arco.y+arco.height-60),150,{250,190,91,45},{0,0,0,0});
        sprite(HEROI.data(),cx-8*7,arco.y+arco.height-8*7*2-10,7.0f);
        // As duas folhas da porta, fechando de fora para dentro.
        float p=std::clamp((tf-0.8f)/1.8f,0.0f,1.0f);
        p=p*p*(3-2*p);
        float meia=arco.width/2;
        for(int lado=0;lado<2;lado++) {
            float w=meia*p;
            Rectangle f = lado==0 ? Rectangle{arco.x,arco.y,w,arco.height} : Rectangle{arco.x+arco.width-w,arco.y,w,arco.height};
            if(w<1) continue;
            DrawRectangleRec(f,{92,58,32,255});
            for(float tabua=f.x+18;tabua<f.x+f.width;tabua+=36) DrawLineEx({tabua,f.y},{tabua,f.y+f.height},2,{62,38,20,255});
            DrawRectangleRec({f.x,f.y+60,f.width,14},{52,54,64,255});
            DrawRectangleRec({f.x,f.y+f.height-80,f.width,14},{52,54,64,255});
        }
        DrawRectangleLinesEx(arco,5,{26,28,36,255});
        if(p>=1.0f && !bateuAPorta) {
            bateuAPorta=true;
            tocarSom(SOM_PORTA_FECHANDO);
            Efeitos::tremer(9.0f,0.45);
        }
        break;
    }
    }
}
}

namespace {
// Conta uma história em tela escura: cada parágrafo com a sua cena
// ("cena(indice, segundos, marca)"; a marca serve para um som tocar só uma vez).
void contarHistoria(const std::vector<std::string>& pars,const std::function<void(int,double,bool&)>& cena,
                    const char* dicaDoEsc) {
    const float TAM_TEXTO=30, LINHA=46, LARGURA_TEXTO=1000, Y_TEXTO=560;
    bool pular=false;
    for(size_t p=0;p<pars.size() && !pular && !WindowShouldClose();p++) {
        std::vector<std::string> quebradas=quebrarLinhas(pars[p],TAM_TEXTO,LARGURA_TEXTO);
        double inicio=GetTime(), saindo=-1;
        bool tudo=false, marca=false;
        size_t antes=0;
        int contador=0;
        while(!WindowShouldClose()) {
            double t=GetTime()-inicio;
            // O texto começa depois de um respiro (a cena aparece primeiro).
            bool completo;
            double tTexto=std::max(0.0,t-0.7);
            size_t bytes = tudo ? pars[p].size()
                         : letrasReveladas(pars[p],tTexto,letrasPorSegundo(ritmoDaVoz(VOZ_NARRADOR)),completo);
            if(tudo) completo=true;
            if(bytes>antes) { vozDasLetras(pars[p],antes,bytes,VOZ_NARRADOR,contador); antes=bytes; }
            float entrada=std::clamp((float)t/0.7f,0.0f,1.0f);
            float saida = saindo<0 ? 1.0f : std::clamp(1.0f-(float)(GetTime()-saindo)/0.5f,0.0f,1.0f);
            Ui::iniciarQuadro();
            DrawRectangle(-200,-200,Ui::LARGURA+400,Ui::ALTURA+400,{3,4,9,255});
            cena((int)p,t,marca);
            textoRevelado(quebradas,bytes,Ui::LARGURA/2.0f,Y_TEXTO,TAM_TEXTO,LINHA,{226,220,206,255},true);
            if(completo && saindo<0) {
                float pisca=0.5f+0.5f*std::sin((float)GetTime()*4);
                textoCentro("Enter",Ui::LARGURA/2.0f,Y_TEXTO+quebradas.size()*LINHA+26,18,
                            {250,190,91,(unsigned char)(120+120*pisca)});
            }
            texto(dicaDoEsc,Ui::LARGURA-larguraDoTexto(dicaDoEsc,16)-30,Ui::ALTURA-40.0f,16,{110,120,140,255});
            // Escurece na entrada e na saída de cada parágrafo.
            float claro=std::min(entrada,saida);
            if(claro<1) DrawRectangle(-200,-200,Ui::LARGURA+400,Ui::ALTURA+400,{3,4,9,(unsigned char)(255*(1-claro))});
            Ui::terminarQuadro();
            while(GetCharPressed()!=0) {}
            if(saindo>=0) { if(saida<=0) break; continue; }
            if(IsKeyPressed(KEY_ESCAPE)) { pular=true; saindo=GetTime(); continue; }
            if(teclaDeAvancar()) {
                if(!completo) tudo=true;
                else { tocarSom(SOM_MENU); saindo=GetTime(); }
            }
        }
    }
    Ui::iniciarQuadro(); DrawRectangle(-200,-200,Ui::LARGURA+400,Ui::ALTURA+400,{3,4,9,255}); Ui::terminarQuadro();
    while(GetCharPressed()!=0) {}
}

// As cenas do final: o chefe se desfaz, a luz de Aldric se apaga, as portas
// se abrem, quem esperava por você, e o nascer do sol.
void cenaDoFinal(const Jogo& jogo,int cena,double t,bool& marca) {
    float cx=Ui::LARGURA/2.0f, cy=290, tf=(float)t;
    switch(cena) {
    case 0: {   // O Senhor das Profundezas se desfaz em fumaça.
        float some=std::clamp((tf-0.6f)/2.4f,0.0f,1.0f);
        DrawCircleGradient((int)cx,(int)cy,250,{170,30,40,(unsigned char)(110*(1-some))},{0,0,0,0});
        sprite(SENHOR.data(),cx-8*14,cy-8*14-10+some*30,14.0f,1.0f-0.5f*some,1.0f-some);
        for(int k=0;k<40;k++) {
            float subida=std::fmod(tf*(40+k%5*10)+k*41.0f,380.0f);
            float x=cx+std::sin(k*12.9898f)*170+std::sin(tf+k)*14, y=cy+120-subida;
            unsigned char a=(unsigned char)(150*some*(1-subida/380.0f));
            DrawCircleV({x,y},4.0f+k%4,{60,50,70,a});
        }
        if(!marca && some>0.05f) { marca=true; tocarSom(SOM_RUGIDO); }
        break;
    }
    case 1: {   // A luz de Aldric se apaga, devagar.
        float some=std::clamp((tf-1.0f)/3.0f,0.0f,1.0f);
        DrawCircleGradient((int)cx,(int)cy,230,{120,150,255,(unsigned char)(90*(1-some))},{0,0,0,0});
        sprite(ESPIRITO.data(),cx-8*12,cy-8*12,12.0f,1.0f,1.0f-some);
        for(int k=0;k<24;k++) {
            float subida=std::fmod(tf*30+k*29.0f,300.0f);
            float x=cx+std::sin(k*7.7f)*120, y=cy+110-subida;
            DrawCircleV({x,y},2,{170,200,255,(unsigned char)(200*(1-some)*(1-subida/300.0f))});
        }
        break;
    }
    case 2: {   // As portas se abrem e o sol entra.
        Rectangle arco={cx-170,cy-180,340,360};
        float p=1.0f-suave((tf-0.6f)/2.0f);   // 1 = fechada, 0 = aberta
        Rectangle muro={arco.x-60,arco.y-46,arco.width+120,arco.height+46};
        DrawRectangleRec(muro,{46,48,58,255});
        for(float y=muro.y;y<muro.y+muro.height;y+=46) {
            DrawLineEx({muro.x,y},{muro.x+muro.width,y},2,{30,32,40,255});
            int linha=(int)((y-muro.y)/46);
            for(float x=muro.x+(linha%2)*35+35;x<muro.x+muro.width;x+=70)
                DrawLineEx({x,y},{x,std::min(y+46,muro.y+muro.height)},2,{30,32,40,255});
        }
        DrawRectangleGradientV((int)arco.x,(int)arco.y,(int)arco.width,(int)arco.height,{255,236,170,255},{255,190,110,255});
        // Raios de sol saindo da porta.
        float abre=1-p;
        for(int k=0;k<7;k++) {
            float ang=-0.9f+k*0.3f;
            Vector2 a={cx-60+k*20.0f,arco.y+arco.height}, b={cx+std::sin(ang)*700,arco.y+arco.height+500};
            DrawTriangle(a,{b.x+40,b.y},{b.x-40,b.y},{255,220,140,(unsigned char)(50*abre)});
        }
        float meia=arco.width/2;
        for(int lado=0;lado<2;lado++) {
            float w=meia*p;
            Rectangle f = lado==0 ? Rectangle{arco.x,arco.y,w,arco.height} : Rectangle{arco.x+arco.width-w,arco.y,w,arco.height};
            if(w<1) continue;
            DrawRectangleRec(f,{92,58,32,255});
            for(float tabua=f.x+18;tabua<f.x+f.width;tabua+=36) DrawLineEx({tabua,f.y},{tabua,f.y+f.height},2,{62,38,20,255});
            DrawRectangleRec({f.x,f.y+60,f.width,14},{52,54,64,255});
            DrawRectangleRec({f.x,f.y+f.height-80,f.width,14},{52,54,64,255});
        }
        DrawRectangleLinesEx(arco,5,{26,28,36,255});
        if(!marca && tf>0.6f) { marca=true; tocarSom(SOM_PORTA_FECHANDO); }
        break;
    }
    case 3: {   // Quem esperava por você na saída.
        DrawRectangleGradientV(0,0,Ui::LARGURA,480,{60,90,140,255},{230,170,110,255});
        DrawRectangle(0,470,Ui::LARGURA,40,{70,90,60,255});
        std::vector<const Sprite*> quem;
        auto encontrou=[&](int tipo){
            for(int a=0;a<NUM_ANDARES;a++) for(int i=0;i<jogo.andares[a].numNpcs;i++)
                if(jogo.andares[a].npcs[i].tipo==tipo && jogo.andares[a].npcs[i].conversou) return true;
            return false;
        };
        if(encontrou(NPC_EXPLORADORA)) quem.push_back(&EXPLORADORA);
        if(encontrou(NPC_MINEIRO)) quem.push_back(&MINEIRO);
        quem.insert(quem.begin()+(long)quem.size()/2,&HEROI);   // você no meio
        if(encontrou(NPC_VIAJANTE)) quem.push_back(&VIAJANTE);
        if(encontrou(NPC_COMERCIANTE)) quem.push_back(&COMERCIANTE);
        float passo=170, x0=cx-(quem.size()-1)*passo/2;
        for(size_t k=0;k<quem.size();k++) {
            float aparece=std::clamp((tf-0.3f-k*0.35f)/0.4f,0.0f,1.0f);
            float pulo=std::fabs(std::sin(tf*3+k))*6*aparece;
            sprite(quem[k]->data(),x0+k*passo-8*8,330-8*8*2+60-pulo,8.0f,1.0f,aparece);
        }
        break;
    }
    default: {  // O sol nascendo sobre Valdoria.
        float sobe=suave(tf/2.5f);
        DrawRectangleGradientV(0,0,Ui::LARGURA,480,{40,60,110,255},{250,170,100,255});
        DrawCircleGradient((int)cx,(int)(480-140*sobe),(int)(150+20*sobe),{255,220,130,255},{255,160,90,0});
        DrawCircle((int)cx,(int)(480-140*sobe),70,{255,236,170,255});
        DrawRectangle(0,470,Ui::LARGURA,40,{50,70,48,255});
        sprite(HEROI.data(),cx-8*8,470-8*8*2+4,8.0f);
        float a=std::clamp((tf-1.5f)/1.0f,0.0f,1.0f);
        textoCentro("VALDORIA ESTÁ LIVRE",cx,90,52,{255,214,120,(unsigned char)(255*a)});
        break;
    }
    }
}
}

void mostrarHistoria(const Jogo& jogo,const std::vector<std::string>& linhas) {
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    tocarMusica(MUSICA_HISTORIA);
    tocarAmbiente(SEM_AMBIENTE);
    std::vector<std::string> pars=emParagrafos(linhas);
    contarHistoria(pars,[&](int p,double t,bool& marca){
        int cena=std::min(p,3);
        if(p+1==(int)pars.size()) cena=3;   // a última é sempre a das portas
        cenaDaHistoria(cena,t,marca);
    },"Esc: pular a história");
}

void mostrarFinal(const Jogo& jogo,const std::vector<std::string>& linhas) {
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    tocarMusica(MUSICA_FINAL);
    tocarAmbiente(SEM_AMBIENTE);
    contarHistoria(emParagrafos(linhas),[&](int p,double t,bool& marca){ cenaDoFinal(jogo,p,t,marca); },
                   "Esc: pular o final");
}
