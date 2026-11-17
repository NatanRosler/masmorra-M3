# GUIA DE EDIÇÃO: MASMORRA 2D

Você não precisa aprender todo o C++ de uma vez. Comece mudando **uma coisa por vez**, compile e veja o resultado.

## 1. Como um personagem vira desenho

Abra `src/sprites.h`. O `HEROI` é uma lista de 16 linhas com 16 símbolos cada. Pense em uma folha quadriculada: cada letra é um quadradinho colorido. Cada `.` fica transparente.

Por exemplo:

```cpp
inline const Sprite HEROI = {{
    "................",
    "......KKKK......",
    // ... outras 14 linhas ...
}};
```

Trocar um `A` por um `R` muda **a cor daquele pixel**, mas não muda a vida, a força ou o comportamento do herói.

**Atenção:** não apague linhas nem mude seu comprimento. Cada personagem deve ter sempre **16 linhas e 16 caracteres por linha**.

## 2. Como mudar as cores

Abra `src/visual.cpp` e procure `pixelCor(char p)`. Lá você encontrará uma lista de letras ligadas a cores:

```cpp
case 'G': return {90, 170, 104, 255};
```

A cor tem quatro valores: `{vermelho, verde, azul, opacidade}`. Cada número vai de 0 a 255. O exemplo acima representa um verde. Você pode mudar apenas os três primeiros números para testar novas combinações.

As cores dos painéis e do menu estão em `src/visual.h`. Exemplo:

```cpp
inline constexpr Color OURO = {250, 190, 91, 255};
```

Evite usar nomes próprios `RED`, `GREEN` ou `GOLD` para novas variáveis. Esses nomes já pertencem à raylib.

## 3. Como mudar o texto dos botões

Abra `src/menu_principal.cpp` e encontre:

```cpp
const char* opcoes[]={"JOGAR","PERSONAGENS","CONTROLES","SAIR"};
```

Você pode trocar `JOGAR` por `INICIAR AVENTURA` sem mexer nas regras. **Não mude a quantidade nem a ordem** das opções sem ajustar também o código que trata cada seleção.

O menu de batalha está desenhado em `src/tela_grafica.cpp` dentro de `telaBatalha()`, mas as regras e opções de verdade continuam em `src/batalha.cpp`. Modifique o desenho e os rótulos visuais, não as regras.

## 4. Como os mapas continuam surgindo conforme você anda

O arquivo `src/visao.cpp` calcula o que o herói consegue enxergar. O arquivo `src/jogo.cpp` guarda em `descoberto[y][x]` tudo que ele já viu. Na tela 2D, a função `tile()` em `src/tela_grafica.cpp` lê isso:

```cpp
if (!dentroDoMapa(x, y) || !a.descoberto[y][x]) {
    DrawRectangle(px, py, TAM, TAM, {8,12,24,255});
    return;
}
```

Assim, uma parte ainda não descoberta aparece escura. Onde o herói já passou, mas não está enxergando agora, o mapa aparece com cores mais apagadas. A mesma regra do original.

## 5. Como os itens e os monstros aparecem

A função `tile()` consulta `itemEm()` e `monstroEm()` do jogo original. Ela **não inventa itens ou inimigos**. Apenas desenha o que já existe no local.

Para mudar a aparência de um item, procure dentro de `tile()` a parte que compara `cat==POCAO`, `cat==CHAVE` etc. Não altere `src/itens.cpp` se quiser manter os efeitos e a distribuição originais.

Para mudar um boneco de inimigo, edite `src/sprites.h` ou `src/extra_sprites.h`. A função `imagemMonstro()` escolhe o desenho pelo tipo original de monstro.

## 6. Onde está a batalha 2D

Em `src/tela_grafica.cpp`, a função `telaBatalha()` desenha o herói de um lado, os inimigos do outro, as barras de vida e os botões. Em `src/batalha.cpp`, o original continua calculando dano, crítico, efeitos, turnos e experiência.

Quando você escolhe LUTAR, o botão envia uma escolha ao sistema original. É por isso que o visual pode mudar sem reescrever os combates.

## 7. Como testar mudanças

Sempre salve com **Ctrl + S**. Depois, no terminal aberto na pasta principal do projeto:

```powershell
cmake --build build --parallel 4
.\build\masmorra_2d.exe
```

Não é necessário repetir a configuração inicial do CMake toda vez que mudar apenas um `.cpp` ou `.h`.

Se aparecer um erro de compilação, copie as **primeiras linhas vermelhas** do terminal, porque os erros seguintes muitas vezes são consequência do primeiro.

## 8. O que ainda precisa ser testado no computador

A estrutura C++ foi verificada com uma simulação da API gráfica. A compilação real com raylib, o ajuste de janela e a interação por mouse e teclado **precisam ser testados no Windows**. A versão original de console foi preservada separadamente.
