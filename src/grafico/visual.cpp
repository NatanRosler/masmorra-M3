#include "visual.h"
#include "efeitos.h"
#include "opcoes.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace {
Font fonte{};
bool fonteExterna = false;
Color corDoHeroi = {95, 170, 202, 255};   // 'J': a cor escolhida na criação
Color pixelCor(char p) {
    switch (p) {
        case 'J': return corDoHeroi;
        case 'E': return {200, 215, 255, 255};   // brilho do espírito
        case 'O': return {150, 40, 55, 255};     // vermelho-escuro do chefe
        case 'K': return {15, 22, 36, 255};
        case 'S': return {241,190,146,255};
        case 'H': return {87,58,54,255};
        case 'W': return {234,240,250,255};
        case 'A': return {95,170,202,255};
        case 'M': return {225,199,92,255};
        case 'B': return {75,83,113,255};
        case 'G': return {90,170,104,255};
        case 'D': return {61,117,73,255};
        case 'Y': return {253,229,112,255};
        case 'R': return {208,98,98,255};
        case 'F': return {238,235,200,255};
        case 'T': return {89,181,161,255};
        case 'V': return {132,101,180,255};
        case 'P': return {100,82,146,255};
        case 'C': return {210,136,78,255};
        // Cores dos itens (itens_sprites.h).
        case 'r': return {232,64,84,255};     // vermelho da poção
        case 'Q': return {255,125,175,255};   // rosa
        case 'U': return {110,175,255,255};   // azul-claro
        case 'X': return {255,135,45,255};    // laranja do fogo
        case 'L': return {214,220,232,255};   // aço claro
        case 'N': return {128,138,160,255};   // aço escuro
        case 'I': return {150,100,52,255};    // madeira
        case 'Z': return {96,62,30,255};      // madeira escura
        case 'l': return {160,235,110,255};   // verde-limão
        case 'v': return {190,140,240,255};   // lilás
        default: return {0,0,0,0};
    }
}
float escala() { return std::min((float)GetScreenWidth()/Ui::LARGURA, (float)GetScreenHeight()/Ui::ALTURA); }
Vector2 deslocamento() { float e=escala(); return {(GetScreenWidth()-Ui::LARGURA*e)/2.0f,(GetScreenHeight()-Ui::ALTURA*e)/2.0f}; }
}

namespace Ui {
void definirCorDoHeroi(Color cor) { corDoHeroi = cor; }
void iniciarRecursos() {
    fonte = GetFontDefault();
    // A fonte é procurada NO COMPUTADOR do jogador; não é distribuída no ZIP.
    const char* possibilidades[] = {
        "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf"
    };
    int codigos[224]; for (int i=0;i<224;i++) codigos[i]=i+32;
    for (const char* caminho: possibilidades) {
        if (!std::filesystem::exists(caminho)) continue;
        Font carregada = LoadFontEx(caminho, 48, codigos, 224);
        if (carregada.texture.id != 0 && carregada.glyphCount > 0) {
            SetTextureFilter(carregada.texture, TEXTURE_FILTER_BILINEAR);
            fonte = carregada;
            fonteExterna = true;
            break;
        }
    }
}
void finalizarRecursos() { if (fonteExterna) UnloadFont(fonte); }
void aplicarTelaCheia() {
    // "Tela cheia sem bordas": ocupa o monitor sem trocar a resolução dele
    // (mais rápido e sem a tela piscando preta no Windows).
    bool esta=IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    if(opcoes().telaCheia!=esta) ToggleBorderlessWindowed();
}
void alternarTelaCheia() {
    opcoes().telaCheia=!opcoes().telaCheia;
    salvarOpcoes();
    aplicarTelaCheia();
}
void iniciarQuadro() {
    // F11 troca a tela cheia em qualquer tela do jogo.
    if(IsKeyPressed(KEY_F11)) alternarTelaCheia();
    // Cada quadro registra de novo os botões clicáveis que desenhar.
    limparOpcoes();
    BeginDrawing(); ClearBackground(FUNDO);
    float e=escala(); Vector2 d=deslocamento();
    Vector2 tremor=Efeitos::deslocamentoDoTremor();   // golpe crítico, armadilha...
    BeginMode2D({{d.x+tremor.x*e,d.y+tremor.y*e},{0,0},0.0f,e});
}
void comecarZoom(float x0, float y0, float z) {
    EndMode2D();
    float e=escala(); Vector2 d=deslocamento();
    Vector2 tremor=Efeitos::deslocamentoDoTremor();
    BeginMode2D({{d.x+(tremor.x+x0)*e,d.y+(tremor.y+y0)*e},{0,0},0.0f,e*z});
}
void terminarZoom() {
    EndMode2D();
    float e=escala(); Vector2 d=deslocamento();
    Vector2 tremor=Efeitos::deslocamentoDoTremor();
    BeginMode2D({{d.x+tremor.x*e,d.y+tremor.y*e},{0,0},0.0f,e});
}
void terminarQuadro() {
    Efeitos::desenharSobreposicoes();   // piscada, escurecimento, botão apertado
    EndMode2D(); EndDrawing();
}
void recortar(Rectangle r) {
    float e=escala(); Vector2 d=deslocamento();
    BeginScissorMode((int)(d.x+r.x*e),(int)(d.y+r.y*e),(int)(r.width*e),(int)(r.height*e));
}
void pararDeRecortar() { EndScissorMode(); }
bool mouseSobre(Rectangle r) { return CheckCollisionPointRec(mouseVirtual(),r); }
void texto(const std::string& s, float x, float y, float tam, Color cor) {
    DrawTextEx(fonte,s.c_str(),{x,y},tam,1.0f,cor);
}
float larguraDoTexto(const std::string& s, float tam) {
    return MeasureTextEx(fonte,s.c_str(),tam,1.0f).x;
}
std::vector<std::string> quebrarLinhas(const std::string& s, float tam, float largura) {
    std::vector<std::string> linhas;
    std::string atual;
    size_t i=0;
    while(i<=s.size()) {
        size_t fim=s.find(' ',i);
        if(fim==std::string::npos) fim=s.size();
        std::string palavra=s.substr(i,fim-i);
        std::string tentativa=atual.empty()?palavra:atual+" "+palavra;
        if(!atual.empty() && larguraDoTexto(tentativa,tam)>largura) { linhas.push_back(atual); atual=palavra; }
        else atual=tentativa;
        i=fim+1;
    }
    linhas.push_back(atual);
    return linhas;
}
int textoQuebrado(const std::string& s, float x, float y, float tam, float largura, float entrelinha, Color cor) {
    std::vector<std::string> linhas=quebrarLinhas(s,tam,largura);
    for(size_t i=0;i<linhas.size();i++) texto(linhas[i],x,y+i*entrelinha,tam,cor);
    return (int)linhas.size();
}
void textoQueCabe(const std::string& s, float x, float y, float tam, float largura, Color cor) {
    float t=tam;
    while(t>14 && larguraDoTexto(s,t)>largura) t-=1;
    texto(s,x,y+(tam-t)/2,t,cor);   // mantém o texto centrado na mesma altura
}
void textoCentro(const std::string& s, float x, float y, float tam, Color cor) {
    Vector2 m=MeasureTextEx(fonte,s.c_str(),tam,1.0f);
    texto(s,x-m.x/2.0f,y,tam,cor);
}
void painel(Rectangle r, Color cor, Color borda) {
    DrawRectangleRounded(r,0.13f,8,cor);
    DrawRectangleRoundedLinesEx(r,0.13f,8,1.7f,borda);
}
void barra(float x, float y, float w, float h, int atual, int maximo, Color cor) {
    DrawRectangleRounded({x,y,w,h},0.35f,8,PAINEL2);
    float p = maximo > 0 ? std::clamp((float)atual/(float)maximo,0.0f,1.0f) : 0.0f;
    if(p>0) DrawRectangleRounded({x+2,y+2,(w-4)*p,h-4},0.35f,8,cor);
    DrawRectangleRoundedLinesEx({x,y,w,h},0.35f,8,1.0f,{88,98,112,255});
}
Vector2 mouseVirtual() {
    Vector2 d=deslocamento(); float e=escala(); Vector2 p=GetMousePosition();
    return {(p.x-d.x)/e,(p.y-d.y)/e};
}
void fundoEstrelado() {
    for (int i=0; i<105;i++) {
        float x=(float)((i*257+43)%LARGURA),y=(float)((i*149+77)%ALTURA);
        DrawCircleV({x,y},i%7==0 ? 1.8f : 0.9f,{109,222,209,(unsigned char)(60+i%70)});
    }
    DrawCircleGradient(1200,190,250,{44,96,103,105},{12,17,32,0});
    DrawCircleGradient(150,770,310,{81,54,93,110},{12,17,32,0});
}
void sprite(const char* const linhas[16], float x, float y, float pixel, float brilho, float alfa) {
    if (alfa <= 0.01f) return;
    DrawEllipse((int)(x+pixel*8),(int)(y+pixel*16),pixel*7.2f,pixel*1.4f,{0,0,0,(unsigned char)(65*alfa)});
    icone(linhas,x,y,pixel,brilho,alfa);
}
void silhueta(const char* const linhas[16], float x, float y, float pixel, Color cor) {
    for (int l=0;l<16;l++) {
        if(!linhas[l]) continue;
        for (int c=0;c<16 && linhas[l][c];c++) {
            if (!pixelCor(linhas[l][c]).a) continue;
            DrawRectangle((int)(x+c*pixel),(int)(y+l*pixel),std::max(1,(int)std::ceil(pixel)),std::max(1,(int)std::ceil(pixel)),cor);
        }
    }
}
void icone(const char* const linhas[16], float x, float y, float pixel, float brilho, float alfa) {
    if (alfa <= 0.01f) return;
    for (int l=0;l<16;l++) {
        if(!linhas[l]) continue;
        for (int c=0;c<16 && linhas[l][c];c++) {
            Color p=pixelCor(linhas[l][c]); if (!p.a) continue;
            p.r=(unsigned char)(p.r*brilho); p.g=(unsigned char)(p.g*brilho); p.b=(unsigned char)(p.b*brilho);
            p.a=(unsigned char)(p.a*alfa);
            DrawRectangle((int)(x+c*pixel),(int)(y+l*pixel),std::max(1,(int)std::ceil(pixel)),std::max(1,(int)std::ceil(pixel)),p);
        }
    }
}
}
