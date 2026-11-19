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
#include "tema.h"
#include "efeitos.h"
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
    return OURO;
}

// "o chacal A" -> "Chacal A" (nome do lutador sem o artigo, para os painéis).
std::string semArtigo(std::string nome) {
    if (nome.size() > 2 && (nome.rfind("o ",0)==0 || nome.rfind("a ",0)==0)) nome = nome.substr(2);
    if (!nome.empty() && nome[0]>='a' && nome[0]<='z') nome[0]=(char)(nome[0]-'a'+'A');
    return nome;
}
std::string emMaiusculas(std::string s) {
    for (char& c : s) if (c>='a' && c<='z') c=(char)(c-'a'+'A');
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
float etiquetaDeElemento(int elemento,float x,float y,float tam=16) {
    std::string nome = emMaiusculas(NOMES_DOS_ELEMENTOS[elemento]);
    float w = std::max(60.0f, (float)nome.size()*tam*0.62f+24);
    DrawRectangleRounded({x,y,w,tam+8},0.5f,6,multiplicar(corDoElemento(elemento),0.45f));
    DrawRectangleRoundedLinesEx({x,y,w,tam+8},0.5f,6,1.2f,corDoElemento(elemento));
    textoCentro(nome,x+w/2,y+3,tam,TEXTO);
    return w;
}
void etiquetaDeStatus(int status,float x,float y) {
    if (status==SEM_STATUS) return;
    const char* nomes[]={"","ENVENENADO","DORMINDO","PARALISADO"};
    Color cores[]={TEXTO,{185,95,215,255},{120,150,230,255},{235,200,70,255}};
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
bool noTapete(const Andar& a,int x,int y) { (void)a; return tapetePronto && tapete[y][x]; }
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
    const Color CHAO_DO_TEMA[NUM_TEMAS]={{72,64,60,255},{52,70,74,255},{66,50,46,255},{70,74,92,255}};
    const Color PAREDE_DO_TEMA[NUM_TEMAS]={{121,102,90,255},{78,108,116,255},{92,58,52,255},{118,124,150,255}};
    Color chao=agora?CHAO_DO_TEMA[tema]:Color{24,33,55,255};
    Color parede=agora?PAREDE_DO_TEMA[tema]:Color{40,49,70,255};
    Color cor= (tipo==PAREDE || tipo==TOCHA) ? parede : chao;
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
    } else if(tipo==ESCADA_DESCE || tipo==ESCADA_SOBE) {
        DrawRectangle(px+3,py+5,18,15,{39,61,73,255});
        for(int i=0;i<3;i++)DrawRectangle(px+5,py+6+i*4,14,2,CIANO);
        textoCentro(tipo==ESCADA_DESCE?"v":"^",px+12,py-2,18,OURO);
    } else if(tipo==TOCHA && agora) {
        // A chama tremula: o tamanho muda com duas ondas de ritmos diferentes.
        float t=(float)GetTime();
        float tremula=std::sin(t*11.0f+x*1.7f)*1.5f+std::sin(t*17.0f+y*2.3f)*1.0f;
        // A chama tem a cor do tema (azul na sala do trono, vermelha no vulcão).
        const Color BRILHO[NUM_TEMAS]={{255,173,48,0},{150,220,255,0},{255,90,40,0},{110,150,255,0}};
        const Color CHAMA[NUM_TEMAS]={{255,211,96,255},{200,240,255,255},{255,150,80,255},{170,200,255,255}};
        Color b=BRILHO[tema]; Color fora=b; b.a=(unsigned char)(140+tremula*10);
        DrawRectangle(px+10,py+13,4,9,{105,75,50,255});
        DrawCircleGradient(px+12,py+8,11+tremula,b,fora);
        DrawCircle(px+12,(int)(py+8-tremula*0.4f),4+tremula*0.3f,CHAMA[tema]);
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
void desenharMonstroNoMapa(const Jogo& jogo,const Monstro& m,float px,float py) {
    float claridade=std::clamp(brilho(jogo.luz[m.y][m.x]),0.27f,1.0f);
    desenharSprite(imagemMonstro(m.tipo),px+2,py+1,1.25f,claridade);
    DrawCircle((int)px+21,(int)py+4,3.5f,corDoElemento(m.elemento));   // elemento sorteado
    if(m.atordoadoMs>0) {
        // Atordoado: três estrelinhas girando sobre a cabeça.
        float giro=(float)GetTime()*5.5f;
        for(int k=0;k<3;k++) {
            float a=giro+k*2.094f;
            DrawPoly({px+12+std::cos(a)*8,py+2+std::sin(a)*3},4,3.2f,45,{255,226,90,255});
        }
    }
}
void hud(const Jogo& jogo) {
    const Jogador& j=jogo.jogador;
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[j.cor]));
    painel({1001,122,400,505});
    texto(j.nome,1025,140,26,OURO);
    float w=etiquetaDeElemento(j.elemento,1027,178);
    texto(dificuldadeAtual().nome,1027+w+12,180,18,corDaDificuldade(nivelDaDificuldade()));
    desenharSprite(HEROI,1238,150,4.6f);
    texto("Andar",1027,226,19,APAGADO);
    texto(std::to_string(jogo.andar)+" / "+std::to_string(NUM_ANDARES),1027,248,30,TEXTO);
    texto("Nível "+std::to_string(j.nivel),1210,248,24,CIANO);
    // Experiência: estrela + barrinha até o próximo nível.
    icone(ARTE_ICONE_XP.data(),1208,276,1.0f);
    {
        int xpNivel=experienciaParaNivel(j.nivel), xpProximo=experienciaParaNivel(j.nivel+1);
        float fracao=(j.nivel>=NIVEL_MAXIMO || xpProximo<=xpNivel) ? 1.0f
                     : std::clamp((float)(j.experiencia-xpNivel)/(xpProximo-xpNivel),0.0f,1.0f);
        Rectangle fundo={1229,280,139,9};
        DrawRectangleRounded(fundo,0.5f,4,{28,38,60,255});
        if(fracao>0) DrawRectangleRounded({fundo.x,fundo.y,std::max(9.0f,fundo.width*fracao),fundo.height},0.5f,4,{110,175,255,255});
        DrawRectangleRoundedLinesEx(fundo,0.5f,4,1.0f,{70,100,150,255});
    }
    texto("Vida "+std::to_string(j.vida)+" / "+std::to_string(j.vidaMaxima),1027,300,21,TEXTO);
    barra(1027,330,341,19,j.vida,j.vidaMaxima);
    if(j.vida>0 && j.vida*4<=j.vidaMaxima) {
        // Vida baixa: a barra pisca em vermelho.
        float pulso=0.5f+0.5f*std::sin((float)relogioMs()/150.0f);
        DrawRectangleRoundedLinesEx({1025,328,345,23},0.35f,8,3.0f,multiplicar(PERIGO,0.4f+0.6f*pulso));
        texto("VIDA BAIXA",1270,300,18,multiplicar(PERIGO,0.6f+0.4f*pulso));
    }
    texto("Vidas: "+std::to_string(j.vidas),1027,364,21,TEXTO);
    icone(ARTE_ICONE_PONTOS.data(),1180,366,1.1f);
    texto("Pontos: "+std::to_string(calcularPontuacao(jogo)),1202,364,21,OURO);
    separador(1027,402,341);
    int segundos=(int)(jogo.tempoMs/1000);
    std::string tempo="Tempo "+std::to_string(segundos/60)+":"+(segundos%60<10?"0":"")+std::to_string(segundos%60);
    texto(tempo,1027,414,19,APAGADO);
    if(jogo.descansando) texto("zZz descansando",1180,414,19,CIANO);
    texto("Passos: "+std::to_string(jogo.passos),1027,444,19,TEXTO);
    // Moedas: uma moedinha dourada desenhada ao lado do valor.
    icone(ARTE_ICONE_MOEDA.data(),1180,446,1.05f);
    texto("Moedas: "+std::to_string(j.moedas),1202,444,19,OURO);
    int naMochila=itensNaMochila(j);
    texto("Mochila: "+std::to_string(naMochila)+"/"+std::to_string(LIMITE_DA_MOCHILA),1202,470,19,
          naMochila>=LIMITE_DA_MOCHILA?PERIGO:TEXTO);
    texto("Monstros: "+std::to_string(jogo.monstrosMortos),1027,470,19,TEXTO);
    texto("Itens: "+std::to_string(jogo.itensColetados),1027,496,19,TEXTO);
    // Atributos: desenho de cada um (força, constituição, destreza, agilidade) e o valor.
    const char* siglas[NUM_ATRIBUTOS]={"FOR","CON","DES","AGI"};
    for(int a=0;a<NUM_ATRIBUTOS;a++) {
        float ax=1027.0f+a*86;
        icone(ARTE_DOS_ATRIBUTOS[a]->data(),ax,524,1.4f);
        texto(std::to_string(j.atributos[a]),ax+27,526,21,TEXTO);
        texto(siglas[a],ax+27+larguraDoTexto(std::to_string(j.atributos[a]),21)+4,531,13,APAGADO);
    }
    if(j.pontosLivres>0) {
        // Ponto livre: o "+" verde pulsa até o jogador distribuir.
        float pulso=0.75f+0.25f*std::sin((float)GetTime()*5);
        icone(ARTE_ICONE_PONTO_LIVRE.data(),1027,563,1.3f,pulso);
        std::string p=std::to_string(j.pontosLivres)+(j.pontosLivres==1?" ponto livre":" pontos livres")+": aperte C";
        texto(p,1054,566,19,OURO);
    }
    painel({39,648,1362,120});
    texto("MENSAGENS",61,662,19,OURO);
    int n=(int)jogo.historico.size();
    int primeira=std::max(0,n-2);
    for(int i=primeira;i<n;i++) textoQueCabe(jogo.historico[i],61,690+32*(i-primeira),25,1320,corDaMensagem(jogo.tiposDoHistorico[i]));
    painel({39,782,1362,76},PAINEL2);
    texto("SETAS / WASD / HJKL: andar",61,796,19,APAGADO);
    const char* rotulos[]={"[I] MOCHILA","[C] FICHA","[Z] DESCANSAR","[?] AJUDA","[M] HISTÓRICO"};
    const int atalhos[]={'i','c','z','?','M'};
    for(int i=0;i<5;i++) {
        Rectangle r={392.0f+i*202,794,190,48};
        bool sobre=mouseSobre(r);
        painel(r,sobre?Color{40,56,84,255}:PAINEL,sobre?CIANO:Color{67,100,129,255});
        textoCentro(rotulos[i],r.x+r.width/2,r.y+14,18,sobre?TEXTO:CIANO);
        registrarAtalho(r,atalhos[i]);
    }
    texto(std::string("ESC: pausa   O: som ")+(somLigado()?"(ligado)":"(desligado)"),62,830,17,APAGADO);
}
// Mensagem importante em destaque no topo do mapa por alguns segundos.
void faixaDeDestaque(const Jogo& jogo) {
    long long agora=relogioMs();
    if(jogo.faixa.empty() || agora>=jogo.faixaAte) return;
    float resta=(float)(jogo.faixaAte-agora);
    float alfa=std::min(1.0f,resta/500.0f);              // some devagar no fim
    Color cor=corDaMensagem(jogo.tipoDaFaixa);
    float largura=std::min((float)CW*TAM-20,larguraDoTexto(jogo.faixa,24)+70);
    Rectangle r={MX+(CW*TAM-largura)/2,(float)MY+14,largura,50};
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
void particulasDoAmbiente(const Jogo& jogo,const Andar& a,int cx,int cy) {
    int tema=temaDoAndar(jogo.andar);
    if(tema!=TEMA_CAVERNA && tema!=TEMA_VULCAO) return;
    double agora=GetTime();
    if(agora-ultimaParticulaDoAmbiente<0.06) return;
    ultimaParticulaDoAmbiente=agora;
    // Sorteia uma célula visível na tela.
    for(int tentativa=0;tentativa<12;tentativa++) {
        int x=cx+GetRandomValue(0,CW), y=cy+GetRandomValue(0,CH);
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
    float meioX=MX+CW*TAM/2.0f, y=MY+CH*TAM/2.0f-60;
    DrawRectangle(MX,(int)y-14,CW*TAM,112,{8,12,24,(unsigned char)(170*alfa)});
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
void mapa(const Jogo& jogo) {
    titulo("EXPLORAÇÃO   |   O mapa se revela conforme você caminha");
    painel({33,115,948,516}, {16,22,36,255});
    atualizarPosicoesSuaves(jogo);
    const Andar& a=andarAtual(jogo);
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    // Câmera com casas "quebradas": desliza junto com o jogador.
    float camX=std::clamp(jogadorSuave.x-CW/2,0.0f,(float)(MAPA_LARGURA-CW));
    float camY=std::clamp(jogadorSuave.y-CH/2,0.0f,(float)(MAPA_ALTURA-CH));
    int cx=(int)std::floor(camX), cy=(int)std::floor(camY);
    int sobraX=(int)std::round((camX-cx)*TAM), sobraY=(int)std::round((camY-cy)*TAM);
    Rectangle janela={(float)MX,(float)MY,(float)CW*TAM,(float)CH*TAM};
    recortar(janela);
    for(int iy=0;iy<=CH;iy++)for(int ix=0;ix<=CW;ix++)
        tile(jogo,a,cx+ix,cy+iy,MX+ix*TAM-sobraX,MY+iy*TAM-sobraY);
    // O chão em volta da fonte, do altar, do baú, do gongo e do viajante.
    desenharAmbientes(jogo,a,MX-camX*TAM,MY-camY*TAM);
    for(int iy=0;iy<=CH;iy++)for(int ix=0;ix<=CW;ix++)
        tileObjetos(jogo,a,cx+ix,cy+iy,MX+ix*TAM-sobraX,MY+iy*TAM-sobraY);
    // Placa "LOJA" em cima da porta da lojinha.
    if(a.loja.existe && a.descoberto[a.loja.portaY][a.loja.portaX]) {
        float px=std::round(MX+(a.loja.portaX-camX)*TAM), py=std::round(MY+(a.loja.portaY-camY)*TAM);
        Rectangle placa={px-7,py-17,TAM+14.0f,16};
        DrawRectangleRec(placa,{120,80,40,255});
        DrawRectangleLinesEx(placa,1,OURO);
        textoCentro("LOJA",px+TAM/2.0f,py-16,13,OURO);
    }
    // Monstros visíveis e o jogador, nas posições suaves.
    auto naTela=[&](float x,float y,float& px,float& py){
        px=std::round(MX+(x-camX)*TAM); py=std::round(MY+(y-camY)*TAM);
    };
    for(int i=0;i<a.numMonstros;i++) {
        const Monstro& m=a.monstros[i];
        if(!m.vivo || !jogo.visivel[m.y][m.x]) continue;
        float px,py; naTela(monstrosSuaves[i].x,monstrosSuaves[i].y,px,py);
        desenharMonstroNoMapa(jogo,m,px,py);
    }
    float hx,hy; naTela(jogadorSuave.x,jogadorSuave.y,hx,hy);
    DrawCircle((int)hx+12,(int)hy+12,12,{220,205,141,55});
    desenharSprite(HEROI,hx+2,hy+1,1.25f,1.0f);
    particulasDoAmbiente(jogo,a,cx,cy);
    // Efeitos do mapa (lascas, espinhos, números) acompanham a câmera.
    Efeitos::desenharParticulas(Efeitos::CAMADA_MAPA,MX-camX*TAM,MY-camY*TAM);
    Efeitos::desenharTextosFlutuantes(Efeitos::CAMADA_MAPA,MX-camX*TAM,MY-camY*TAM);
    pararDeRecortar();
    DrawRectangleLinesEx(janela,2,{83,110,133,255});
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
void faixaBotao(const std::string& txt,Rectangle r,bool sel,int indice=-1,int cursor=0,float tam=23,const Sprite* arte=nullptr) {
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

// Onde o sprite de cada lutador é desenhado (sem a animação).
void posicaoDoSprite(const Batalha& b,int quem,float& x,float& y,float& pixel) {
    if(quem==0) { x=200; y=267; pixel=12.2f; return; }
    if(b.numInimigos==1) { x=892; y=257; pixel=12.3f; return; }
    x=735.0f+(quem-1)*211; y=280; pixel=8.5f;
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
    iconeDeStatus(l.status,x+8*p+an.dx,y+an.dy-6);
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
    DrawRectangleGradientV(42,125,1356,462,{25,40,60,255},{30,29,45,255});
    // Arena estilizada, chão ao redor dos personagens.
    DrawEllipse(350,493,275,50,{44,63,76,255});
    DrawEllipse(1010,466,280,54,{44,63,76,255});
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    spriteDoLutador(b,0,HEROI);
    if(b.escudoErguido) escudoErguidoNaBatalha(b);
    painelDoLutador(b.jogador,b.vidaMostrada[0],{55,140,430,107},CIANO,CIANO,false);
    // Chefe furioso (segunda fase): aura vermelha pulsando atrás dele.
    if(b.chefeFurioso) {
        for(int i=0;i<b.numInimigos;i++) {
            if(!b.naLuta[i] || !b.inimigos[i].ehChefe || desmaiado[i+1]) continue;
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
        spriteDoLutador(b,1,imagemMonstro(m.tipo));
        painelDoLutador(b.inimigos[0],b.vidaMostrada[1],{909,140,474,106},OURO,OURO,false);
    } else {
        for (int i=0;i<b.numInimigos;i++) {
            if(!b.naLuta[i])continue;
            int px=735+i*211;
            spriteDoLutador(b,i+1,imagemMonstro(b.monstros[i]->tipo));
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
        std::vector<std::string> linhas=quebrarLinhas(b.texto,30,1250);
        Rectangle caixa={52,646,1336,62.0f+38*(linhas.size()-1)};
        DrawRectangleRounded(caixa,0.3f,8,multiplicar(cor,0.22f));
        DrawRectangleRoundedLinesEx(caixa,0.3f,8,3.0f,multiplicar(cor,brilho));
        if(b.tipoDoTexto==MSG_BOA) DrawPoly({86,677},4,13,45,cor);
        else { DrawCircle(86,677,14,cor); textoCentro("!",86,663,24,FUNDO); }
        for(size_t i=0;i<linhas.size();i++) texto(linhas[i],112,661+38*i,30,cor);
        if(b.travado) {
            // Barrinha que esvazia enquanto as teclas estão travadas.
            float resta = 1.0f - (float)(relogioMs()-b.textoDesde)/TEMPO_DE_LEITURA_MS;
            DrawRectangle(1070,782,(int)(240*std::max(0.0f,resta)),6,cor);
        }
    } else {
        // Com menu aberto, o texto divide a caixa com os botões da direita.
        float largura = b.menu==MENU_NENHUM ? 1300 : (b.menu==MENU_STATUS ? 480 : 660);
        textoQuebrado(b.texto,62,660,28,largura,36,TEXTO);
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
            std::string s;
            if(i<jogo.jogador.numGolpes) {
                const Golpe& g=GOLPES[jogo.jogador.golpes[i]];
                s=std::string(g.nome)+"  |  "+NOMES_DOS_ELEMENTOS[g.elemento]+"  |  poder "+
                  (g.poder>0?std::to_string(g.poder):std::string("-"))+"  |  PP "+
                 (g.ppMaximo==0?std::string("--"):std::to_string(jogo.jogador.pp[i])+"/"+std::to_string(g.ppMaximo));
            } else s="Não aprender";
            faixaBotao(s,{740.0f,625.0f+i*41,631,37},b.cursor==i,i,b.cursor,20);
        }
        if(b.menu==MENU_ESQUECER) texto("Novo golpe: "+std::string(GOLPES[b.golpeNovo].nome),63,752,22,CIANO);
    } else if(b.menu==MENU_ITENS) {
        for(int i=0;i<b.numItensUsaveis;i++) {
            int id=b.itensUsaveis[i];
            faixaBotao(std::string(TIPOS_DE_ITEM[id].nome)+"  x"+std::to_string(jogo.jogador.mochila[id]),
                {740.0f,630.0f+i*40,631,36},b.cursor==i,i,b.cursor,20,ARTE_DOS_ITENS[id]);
        }
    } else if(b.menu==MENU_ALVO) {
        for(int i=0;i<b.numAlvos;i++) {
            int id=b.alvos[i];
            faixaBotao(semArtigo(b.inimigos[id].nome),{740.0f,630.0f+i*49,631,44},b.cursor==i,i,b.cursor);
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
        if(!b.travado) texto("ENTER para continuar",1070,770,19,CIANO);
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
void desenharJogo(const Jogo& jogo) {
    ultimoJogo=&jogo;
    Efeitos::trocarDeTela(TELA_MAPA);
    definirRedesenho([&jogo]{ desenharJogo(jogo); });
    Ui::iniciarQuadro();mapa(jogo);Ui::terminarQuadro();
}
void desenharBatalha(const Jogo& jogo,const Batalha& b) {
    Efeitos::trocarDeTela(TELA_BATALHA);
    definirRedesenho([&jogo,b]{ desenharBatalha(jogo,b); });   // cópia da batalha
    Ui::iniciarQuadro();telaBatalha(jogo,b);Ui::terminarQuadro();
}

// ---------------------------------------------------------------------------
// ANIMAÇÕES DA BATALHA (chamadas pela lógica em batalha.cpp)
// Cada uma roda por uma fração de segundo, desenhando a batalha quadro a
// quadro com os lutadores deslocados, e depois devolve o controle.
// ---------------------------------------------------------------------------
void animarBatalha(const Jogo& jogo,const Batalha& b,int tipo,int quem,int alvo,int elemento,int valor,bool critico) {
    if(jogo.modoAutomatico) return;
    double duracao=0.5;
    switch(tipo) {
        case ANIM_ENTRADA: duracao=0.65; break;
        case ANIM_GOLPE:   duracao=0.55; break;
        case ANIM_BLOQUEIO: duracao=0.6; break;
        case ANIM_FURIA:   duracao=1.1;  break;
        case ANIM_DESMAIO: duracao=0.6;  break;
        case ANIM_NIVEL:   duracao=0.8;  break;
    }
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
            // Bote: quem ataca avança e volta.
            if(t<0.4f) animacao[quem].dx=direcao*70*std::sin(3.14159f*t/0.4f);
            if(t>=0.25f && !disparou) {
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
                    else if(valor>0) Efeitos::tremer(3,0.15);
                }
            }
            if(t>=0.25f) {
                float u=(t-0.25f)/0.75f;
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
        case ANIM_FURIA:
            // O chefe treme, fica vermelho e solta faíscas; a tela pisca.
            if(!disparou) {
                disparou=true;
                Efeitos::piscar({200,20,30,255},0.15);
                Efeitos::tremer(12,0.7);
                Efeitos::brilhoSubindo(centroQuem.x,centroQuem.y,{255,60,50,255},40,Efeitos::CAMADA_TELA);
                Efeitos::textoFlutuante("FÚRIA!",centroQuem.x,centroQuem.y-110,{255,90,80,255},48,Efeitos::CAMADA_TELA,1.3);
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
        Ui::iniciarQuadro();telaBatalha(jogo,b);Ui::terminarQuadro();
    }
    for(int i=0;i<=MAX_INIMIGOS;i++) animacao[i]={0,0,1};
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
void desenharAjuda(const Jogo& jogo) {
    definirRedesenho([&jogo]{ desenharAjuda(jogo); });
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("COMANDOS DO MASMORRA");
    const std::vector<std::string> linhas={
        "Setas / WASD / H J K L: andar     Diagonais: Y U B N",
        "Encoste nos monstros para começar a batalha",
        "I: mochila      C: ficha (distribuir pontos de atributo)",
        "Z: descansar e recuperar vida      M: mensagens",
        "Caixas: empurre contra um monstro para atordoá-lo; podem ter itens",
        "Espinhos vermelhos: armadilha! Empurre uma caixa em cima",
        "Pessoas com '!': ande até elas para conversar",
        "O chefe final espera no 10o andar. Dele não dá para fugir",
        "Bolinha colorida no monstro = elemento dele nesta partida",
        "Semente desta partida: "+std::to_string(jogo.semente)};
    for(size_t i=0;i<linhas.size();i++)texto(linhas[i],263,240+i*41,21,i%2?TEXTO:CIANO);
    texto("Aperte qualquer tecla para retornar",263,696,20,OURO);Ui::terminarQuadro();
}
void desenharHistorico(const Jogo& jogo) {
    definirRedesenho([&jogo]{ desenharHistorico(jogo); });
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("HISTÓRICO");
    size_t n=jogo.historico.size();size_t ini=n>12?n-12:0;
    for(size_t i=ini;i<n;i++)textoQueCabe(jogo.historico[i],263,240+36*(i-ini),23,920,corDaMensagem(jogo.tiposDoHistorico[i]));
    texto("Aperte qualquer tecla para retornar",263,700,20,OURO);Ui::terminarQuadro();
}
void desenharMorte(const Jogo& jogo) {
    Efeitos::trocarDeTela(TELA_MORTE);
    definirRedesenho([&jogo]{ desenharMorte(jogo); });
    Ui::iniciarQuadro();mapa(jogo);
    overlayBasico(jogo.venceu ? "VITÓRIA!" : "FIM DE JOGO");
    if(jogo.venceu) {
        desenharSprite(HEROI,1010,250,7.0f);
        texto(jogo.jogador.nome+" derrotou o Senhor das Profundezas!",263,245,26,OURO);
        texto("Valdoria está livre. As portas da masmorra se abrem.",263,285,21,TEXTO);
    } else {
        desenharSprite(SENHOR,1010,250,7.0f,0.6f);
        texto("Suas vidas acabaram no andar "+std::to_string(jogo.andar),263,245,26,PERIGO);
        texto(jogo.causaDaMorte,263,285,21,TEXTO);
    }
    texto("PONTUAÇÃO: "+std::to_string(jogo.pontuacaoFinal),263,330,34,OURO);
    texto("Andar "+std::to_string(jogo.andarMaisFundo)+"   |   "+std::to_string(jogo.monstrosMortos)+" monstros   |   "+
          std::to_string(jogo.itensColetados)+" itens   |   "+std::to_string(jogo.passos)+" passos   |   "+
          std::to_string(dificuldadeAtual().vidas-jogo.jogador.vidas)+" vidas perdidas   |   "+
          dificuldadeAtual().nome,263,378,18,APAGADO);
    if(jogo.posicaoNoRanking>=0)
        texto("Você ficou em "+std::to_string(jogo.posicaoNoRanking+1)+"º lugar no ranking!",263,410,21,CIANO);
    separador(263,446,880);
    texto("MELHORES PARTIDAS",263,458,19,OURO);
    std::vector<RegistroRanking> ranking=lerRanking();
    for(int i=0;i<(int)ranking.size() && i<5;i++) {
        const RegistroRanking& r=ranking[i];
        Color c=(i==jogo.posicaoNoRanking)?OURO:TEXTO;
        texto(std::to_string(i+1)+". "+r.nome,280,490+i*30,20,c);
        texto(std::to_string(r.pontos)+" pts",620,490+i*30,20,c);
        texto("andar "+std::to_string(r.andar)+"   "+r.dificuldade+(r.venceu?"   VENCEU":""),800,490+i*30,20,c);
    }
    texto("N: nova partida       Q: menu principal",263,700,24,OURO);
    Ui::terminarQuadro();
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
    else if(c.linha==LINHA_ELEMENTO) dica="Golpes do seu elemento causam 50% a mais de dano.";
    else if(c.linha==LINHA_COR) dica="A cor da roupa do seu personagem.";
    else if(c.linha==LINHA_DIFICULDADE) dica=DIFICULDADES[c.dificuldade].descricao;
    else if(c.linha>=LINHA_FORCA && c.linha<=LINHA_AGILIDADE) dica=EFEITOS_DOS_ATRIBUTOS[c.linha-LINHA_FORCA];
    else if(c.linha==LINHA_ALEATORIO) dica="Sorteia nome, elemento, cor e atributos. Aperte de novo para outro.";
    else dica="Tudo pronto? Enter para descer à masmorra!";
    separador(800,684,460);
    textoQuebrado(dica,800,696,22,480,28,CIANO);

    texto("Setas: escolher e mudar valores   |   Enter: próximo   |   Esc: voltar ao menu",150,830,19,APAGADO);
    Ui::terminarQuadro();
}
