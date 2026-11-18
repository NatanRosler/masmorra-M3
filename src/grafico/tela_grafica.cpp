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
    }
    return GOBLIN;
}
const Sprite& imagemNpc(int tipo) {
    if (tipo==NPC_MINEIRO) return MINEIRO;
    if (tipo==NPC_ESPIRITO) return ESPIRITO;
    return EXPLORADORA;
}
void desenharSprite(const Sprite& sp,float x,float y,float pixel,float iluminacao=1.0f) {
    // A arte e a paleta de pixels ficam em sprites.h, extra_sprites.h e visual.cpp.
    sprite(sp.data(),x,y,pixel,iluminacao);
}
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
    } else if(tipo==CAIXA) {
        // Caixa de madeira (dá para empurrar).
        float f = agora ? claridade : 0.45f;
        DrawRectangle(px+3,py+3,18,18,multiplicar({150,100,52,255},f));
        DrawRectangleLines(px+3,py+3,18,18,multiplicar({222,170,95,255},f));
        DrawLine(px+4,py+4,px+20,py+20,multiplicar({96,62,30,255},f));
        DrawLine(px+20,py+4,px+4,py+20,multiplicar({96,62,30,255},f));
    }
    // Armadilha já descoberta: espinhos vermelhos.
    int armadilha = armadilhaEm(a,x,y);
    if (armadilha>=0 && a.armadilhas[armadilha].descoberta) {
        Color c = agora ? Color{235,95,95,255} : Color{110,60,70,255};
        for (int i=0;i<3;i++) {
            float bx=(float)px+4+i*6;
            DrawTriangle({bx,(float)py+19},{bx+6,(float)py+19},{bx+3,(float)py+7},c);
        }
    }
    if (agora) {
        int idx = monstroEm(a,x,y);
        int npc = npcEm(a,x,y);
        if(idx>=0) {
            const Monstro& m = a.monstros[idx];
            desenharSprite(imagemMonstro(m.tipo),px+2,py+1,1.25f,claridade);
            DrawCircle(px+21,py+4,3.5f,corDoElemento(m.elemento));   // elemento sorteado
            if(m.atordoadoMs>0) {
                // Atordoado: três estrelinhas girando sobre a cabeça.
                float giro=(float)relogioMs()/180.0f;
                for(int k=0;k<3;k++) {
                    float a=giro+k*2.094f;
                    DrawPoly({px+12+std::cos(a)*8,(float)py+2+std::sin(a)*3},4,3.2f,45,{255,226,90,255});
                }
            }
        } else if(npc>=0) {
            desenharSprite(imagemNpc(a.npcs[npc].tipo),px+2,py+1,1.25f,1.0f);
            if(!a.npcs[npc].conversou) textoCentro("!",px+20,py-6,16,OURO);
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
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[j.cor]));
    painel({1001,122,400,505});
    texto(j.nome,1025,140,26,OURO);
    float w=etiquetaDeElemento(j.elemento,1027,178);
    texto(dificuldadeAtual().nome,1027+w+12,180,18,corDaDificuldade(nivelDaDificuldade()));
    desenharSprite(HEROI,1238,150,4.6f);
    texto("Andar",1027,226,19,APAGADO);
    texto(std::to_string(jogo.andar)+" / "+std::to_string(NUM_ANDARES),1027,248,30,TEXTO);
    texto("Nível "+std::to_string(j.nivel),1210,252,24,CIANO);
    texto("Vida "+std::to_string(j.vida)+" / "+std::to_string(j.vidaMaxima),1027,300,21,TEXTO);
    barra(1027,330,341,19,j.vida,j.vidaMaxima);
    if(j.vida>0 && j.vida*4<=j.vidaMaxima) {
        // Vida baixa: a barra pisca em vermelho.
        float pulso=0.5f+0.5f*std::sin((float)relogioMs()/150.0f);
        DrawRectangleRoundedLinesEx({1025,328,345,23},0.35f,8,3.0f,multiplicar(PERIGO,0.4f+0.6f*pulso));
        texto("VIDA BAIXA",1270,300,18,multiplicar(PERIGO,0.6f+0.4f*pulso));
    }
    texto("Vidas: "+std::to_string(j.vidas),1027,364,21,TEXTO);
    texto("Pontos: "+std::to_string(calcularPontuacao(jogo)),1180,364,21,OURO);
    separador(1027,402,341);
    int segundos=(int)(jogo.tempoMs/1000);
    std::string tempo="Tempo "+std::to_string(segundos/60)+":"+(segundos%60<10?"0":"")+std::to_string(segundos%60);
    texto(tempo,1027,414,19,APAGADO);
    if(jogo.descansando) texto("zZz descansando",1180,414,19,CIANO);
    texto("Passos: "+std::to_string(jogo.passos),1027,444,19,TEXTO);
    texto("Monstros: "+std::to_string(jogo.monstrosMortos),1027,470,19,TEXTO);
    texto("Itens: "+std::to_string(jogo.itensColetados),1027,496,19,TEXTO);
    texto("FOR "+std::to_string(j.atributos[FORCA])+"  CON "+std::to_string(j.atributos[CONSTITUICAO])+
          "  DES "+std::to_string(j.atributos[DESTREZA])+"  AGI "+std::to_string(j.atributos[AGILIDADE]),1027,530,18,APAGADO);
    if(j.pontosLivres>0) {
        std::string p=std::to_string(j.pontosLivres)+(j.pontosLivres==1?" ponto livre":" pontos livres")+": aperte C";
        texto(p,1027,568,19,OURO);
    }
    painel({39,648,1362,120});
    texto("MENSAGENS",61,662,19,OURO);
    int n=(int)jogo.historico.size();
    int primeira=std::max(0,n-2);
    for(int i=primeira;i<n;i++) texto(jogo.historico[i],61,695+26*(i-primeira),20,corDaMensagem(jogo.tiposDoHistorico[i]));
    painel({39,782,1362,76},PAINEL2);
    texto("SETAS / WASD / HJKL: andar",61,796,19,APAGADO);
    const char* rotulos[]={"[I] MOCHILA","[C] FICHA","[Z] DESCANSAR","[?] AJUDA","[M] HISTÓRICO"};
    const int atalhos[]={'i','c','z','?','M'};
    for(int i=0;i<5;i++) {
        Rectangle r={392.0f+i*202,794,190,48};
        painel(r,PAINEL,Color{67,100,129,255});
        textoCentro(rotulos[i],r.x+r.width/2,r.y+14,18,CIANO);
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
    float largura=std::min(900.0f,(float)jogo.faixa.size()*11.5f+90);
    Rectangle r={MX+(CW*TAM-largura)/2,(float)MY+14,largura,48};
    Color fundo=multiplicar(cor,0.25f); fundo.a=(unsigned char)(235*alfa);
    Color borda=cor; borda.a=(unsigned char)(255*alfa);
    DrawRectangleRounded(r,0.4f,8,fundo);
    DrawRectangleRoundedLinesEx(r,0.4f,8,2.5f,borda);
    Color letra=jogo.tipoDaFaixa==MSG_BOA?OURO:Color{255,170,160,255}; letra.a=(unsigned char)(255*alfa);
    textoCentro(jogo.faixa,r.x+r.width/2,r.y+13,21,letra);
}
void mapa(const Jogo& jogo) {
    titulo("EXPLORAÇÃO   |   O mapa se revela conforme você caminha");
    painel({33,115,948,516}, {16,22,36,255});
    int cx,cy;posicaoDaCamera(jogo,cx,cy);
    const Andar& a=andarAtual(jogo);
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    for(int iy=0;iy<CH;iy++)for(int ix=0;ix<CW;ix++)
        tile(jogo,a,cx+ix,cy+iy,MX+ix*TAM,MY+iy*TAM);
    DrawRectangleLinesEx({(float)MX,(float)MY,(float)CW*TAM,(float)CH*TAM},2,{83,110,133,255});
    faixaDeDestaque(jogo);
    hud(jogo);
}
void overlayBasico(const std::string& tituloModal) {
    DrawRectangle(0,0,Ui::LARGURA,Ui::ALTURA,{0,0,0,177});
    painel({215,145,1010,610},{26,35,56,255},OURO);
    textoCentro(tituloModal,720,174,31,OURO);
    separador(255,222,930);
}
void faixaBotao(const std::string& txt,Rectangle r,bool sel,int indice=-1,int cursor=0,float tam=23) {
    Color fundo=sel?OURO:PAINEL2;
    painel(r,fundo,sel?OURO:Color{70,89,110,255});
    texto(txt,r.x+20,r.y+(r.height-tam)/2-1,tam,sel?FUNDO:TEXTO);
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
void telaBatalha(const Jogo& jogo,const Batalha& b) {
    titulo("BATALHA   |   Golpes, elementos e turnos");
    painel({39,121,1362,468});
    DrawRectangleGradientV(42,125,1356,462,{25,40,60,255},{30,29,45,255});
    // Arena estilizada, chão ao redor dos personagens.
    DrawEllipse(350,493,275,50,{44,63,76,255});
    DrawEllipse(1010,466,280,54,{44,63,76,255});
    definirCorDoHeroi(paraCor(CORES_DO_JOGADOR[jogo.jogador.cor]));
    desenharSprite(HEROI,200,267,12.2f);
    painelDoLutador(b.jogador,b.vidaMostrada[0],{55,140,430,107},CIANO,CIANO,false);
    if(b.numInimigos==1) {
        const Monstro& m=*b.monstros[0];
        desenharSprite(imagemMonstro(m.tipo),892,257,12.3f);
        painelDoLutador(b.inimigos[0],b.vidaMostrada[1],{909,140,474,106},OURO,OURO,false);
    } else {
        for (int i=0;i<b.numInimigos;i++) {
            if(!b.naLuta[i])continue;
            int px=735+i*211;
            desenharSprite(imagemMonstro(b.monstros[i]->tipo),(float)px,280,8.5f);
            bool alvo=(b.menu==MENU_ALVO && b.alvos[b.cursor]==i);
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
        Rectangle caixa={52,650,1336,58};
        DrawRectangleRounded(caixa,0.3f,8,multiplicar(cor,0.22f));
        DrawRectangleRoundedLinesEx(caixa,0.3f,8,3.0f,multiplicar(cor,brilho));
        if(b.tipoDoTexto==MSG_BOA) DrawPoly({86,679},4,13,45,cor);
        else { DrawCircle(86,679,14,cor); textoCentro("!",86,665,24,FUNDO); }
        texto(b.texto,112,664,26,cor);
        if(b.travado) {
            // Barrinha que esvazia enquanto as teclas estão travadas.
            float resta = 1.0f - (float)(relogioMs()-b.textoDesde)/TEMPO_DE_LEITURA_MS;
            DrawRectangle(1070,782,(int)(240*std::max(0.0f,resta)),6,cor);
        }
    } else {
        texto(b.texto,62,664,22,TEXTO);
    }
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
                s=std::string(g.nome)+"  |  "+NOMES_DOS_ELEMENTOS[g.elemento]+"  |  poder "+
                  (g.poder>0?std::to_string(g.poder):std::string("-"))+"  |  PP "+
                 (g.ppMaximo==0?std::string("--"):std::to_string(jogo.jogador.pp[i])+"/"+std::to_string(g.ppMaximo));
            } else s="Não aprender";
            faixaBotao(s,{740.0f,625.0f+i*41,631,37},b.cursor==i,i,b.cursor,20);
        }
        if(b.menu==MENU_ESQUECER) texto("Novo golpe: "+std::string(GOLPES[b.golpeNovo].nome),63,722,20,CIANO);
    } else if(b.menu==MENU_ITENS) {
        for(int i=0;i<b.numItensUsaveis;i++) {
            int id=b.itensUsaveis[i];
            faixaBotao(std::string(TIPOS_DE_ITEM[id].nome)+"  x"+std::to_string(jogo.jogador.mochila[id]),
                {740.0f,630.0f+i*40,631,36},b.cursor==i,i,b.cursor,20);
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
        faixaBotao(opcoes[i],{274.0f,239.0f+(i-primeira)*36,890,32},cursor==i,i,cursor,21);
    // Descrição: quebra em duas linhas se for comprida (separadas por " | ").
    size_t corte=descricao.find("  |  ");
    if(corte==std::string::npos) texto(descricao,275,660,19,APAGADO);
    else {
        texto(descricao.substr(0,corte),275,648,19,APAGADO);
        texto(descricao.substr(corte+5),275,674,17,CIANO);
    }
    texto("Setas e Enter: selecionar   |   Esc: voltar",275,712,19,CIANO);
    Ui::terminarQuadro();
}
void desenharAjuda(const Jogo& jogo) {
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
    Ui::iniciarQuadro();mapa(jogo);overlayBasico("HISTÓRICO");
    size_t n=jogo.historico.size();size_t ini=n>14?n-14:0;
    for(size_t i=ini;i<n;i++)texto(jogo.historico[i],263,245+31*(i-ini),19,corDaMensagem(jogo.tiposDoHistorico[i]));
    texto("Aperte qualquer tecla para retornar",263,700,20,OURO);Ui::terminarQuadro();
}
void desenharMorte(const Jogo& jogo) {
    limparOpcoes();
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
    Ui::iniciarQuadro();titulo("AVISO");texto(a,140,270,25,PERIGO);texto(b,140,330,24,TEXTO);Ui::terminarQuadro();
}

// ---------------------------------------------------------------------------
// PÁGINAS DE TEXTO (história, conversas, como jogar, itens, pontuação)
// ---------------------------------------------------------------------------
void desenharPagina(const std::string& tituloDaPagina,const std::vector<std::string>& linhas,
                    const std::string& rodape) {
    limparOpcoes();
    Ui::iniciarQuadro();
    fundoEstrelado();
    texto("MASMORRA",145,48,46,OURO);
    separador(145,112,1150);
    painel({140,135,1160,690},{22,32,51,255});
    textoCentro(tituloDaPagina,720,158,32,OURO);
    separador(190,206,1060);
    int n=(int)linhas.size();
    float passo=std::min(30.0f,520.0f/std::max(1,n));
    float tam=passo>=28?21.0f:18.0f;
    for(int i=0;i<n;i++) {
        // Linhas em MAIÚSCULAS são subtítulos.
        bool subtitulo = !linhas[i].empty() && linhas[i]==emMaiusculas(linhas[i]) &&
                         linhas[i].find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ")!=std::string::npos;
        texto(linhas[i],200,226+i*passo,tam,subtitulo?CIANO:TEXTO);
    }
    textoCentro(rodape,720,782,20,OURO);
    Ui::terminarQuadro();
}

// ---------------------------------------------------------------------------
// CRIAÇÃO DO PERSONAGEM
// ---------------------------------------------------------------------------
void desenharCriacao(const Criacao& c) {
    limparOpcoes();
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
            texto("ATRIBUTOS   (pontos livres: "+std::to_string(c.pontosLivres)+")",160,y+2,20,c.pontosLivres>0?OURO:APAGADO);
            y+=32;
        }
        Rectangle r={150,y,560,48};
        // Os dois botões do fim ficam lado a lado.
        if(i==LINHA_ALEATORIO || i==LINHA_COMECAR) {
            r = (i==LINHA_ALEATORIO) ? Rectangle{150,y+8,230,56} : Rectangle{394,y+8,316,56};
            painel(r,sel?OURO:PAINEL2,sel?OURO:Color{69,89,110,255});
            textoCentro(i==LINHA_ALEATORIO?"ALEATÓRIO":"COMEÇAR AVENTURA",r.x+r.width/2,r.y+16,24,sel?FUNDO:TEXTO);
            registrarOpcao(r,i,c.linha);
            continue;
        }
        painel(r,sel?Color{58,52,38,255}:PAINEL2,sel?OURO:Color{69,89,110,255});
        texto(rotulos[i],r.x+20,r.y+12,20,sel?OURO:APAGADO);
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
    texto("Elemento: ",800,550,20,APAGADO);
    texto(NOMES_DOS_ELEMENTOS[e],900,550,20,corDoElemento(e));
    texto("Golpe inicial: "+std::string(g.nome)+" (poder "+std::to_string(g.poder)+")",800,580,20,TEXTO);
    texto("Fraco contra: "+elementosCom(e,4),800,610,20,TEXTO);
    texto("Resiste a: "+elementosCom(e,1),800,640,20,TEXTO);
    std::string dica;
    if(c.linha==LINHA_NOME) dica="Digite o nome (até 12 letras). Backspace apaga.";
    else if(c.linha==LINHA_ELEMENTO) dica="Golpes do seu elemento causam 50% a mais de dano.";
    else if(c.linha==LINHA_COR) dica="A cor da roupa do seu personagem.";
    else if(c.linha==LINHA_DIFICULDADE) dica=DIFICULDADES[c.dificuldade].descricao;
    else if(c.linha>=LINHA_FORCA && c.linha<=LINHA_AGILIDADE) dica=EFEITOS_DOS_ATRIBUTOS[c.linha-LINHA_FORCA];
    else if(c.linha==LINHA_ALEATORIO) dica="Sorteia nome, elemento, cor e atributos. Aperte de novo para outro.";
    else dica="Tudo pronto? Enter para descer à masmorra!";
    separador(800,684,460);
    // Quebra a dica em até duas linhas de ~46 letras.
    if(dica.size()>46) {
        size_t corte=dica.rfind(' ',46);
        texto(dica.substr(0,corte),800,698,19,CIANO);
        texto(dica.substr(corte+1),800,724,19,CIANO);
    } else texto(dica,800,698,19,CIANO);

    texto("Setas: escolher e mudar valores   |   Enter: próximo   |   Esc: voltar ao menu",150,830,19,APAGADO);
    Ui::terminarQuadro();
}
