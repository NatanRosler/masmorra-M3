# Como o Masmorra funciona por dentro

Explicação das partes principais da lógica do jogo (arquivos em `src/`).
Serve para estudar para a defesa: cada seção diz o que a parte faz e como.

## Como a masmorra é gerada

Cada andar é montado por **acreção**, a mesma ideia do Brogue:

1. **Primeira sala** — uma caverna grande ou uma sala em cruz, em posição sorteada.
2. **Acreção** — 80 tentativas de encaixar uma sala nova. A sala é desenhada num
   *molde* (retangular, em cruz, circular ou caverna; às vezes com um corredor
   saindo dela). Depois o gerador procura, em ordem aleatória (embaralhamento de
   Fisher-Yates), uma parede do mapa com chão atrás e rocha na frente onde a
   porta da sala possa ficar, sem que a sala encoste em nada. Se couber, a sala é
   carimbada e a parede vira porta.
3. **Cavernas** — feitas com **autômato celular**: cada célula começa com 55% de
   chance de ser chão; em 5 rodadas, parede com 6+ vizinhos de chão vira chão e
   chão com menos de 4 vizinhos vira parede. No fim fica só a maior região
   conectada (*flood fill*).
4. **Atalhos** — paredes finas entre dois chãos são abertas se, por **BFS**, o
   caminho de um lado ao outro tiver mais de 20 passos. Assim o mapa tem voltas.
5. **Escadas** — ficam em nichos na parede; a de descida é sorteada entre as
   mais distantes (por BFS) da chegada.
6. **Sala do tesouro** — para cada porta, o gerador testa: "se esta porta fosse
   parede, alguma parte do mapa ficaria isolada?" (BFS de novo). Se a parte
   isolada for uma sala de bom tamanho e sem escadas, a porta é candidata; uma
   delas é sorteada e vira **porta trancada**. 99% dos andares têm uma.

Testado com 2.000 andares gerados: todo o chão é sempre alcançável a partir da
escada (cerca de 6 ms por andar).

## Como funciona a visão

O jogador só vê o que está **em linha reta** e **iluminado**.

1. **Linha de visão — shadowcasting recursivo.** A área em volta do jogador é
   dividida em 8 fatias (octantes). Cada fatia é percorrida linha por linha,
   como um cone de luz que se afasta do jogador. Quando o cone encontra uma
   parede, a função **chama a si mesma** para continuar só com a parte do cone
   que passou ao lado dela; o que fica atrás da parede vira sombra.
   Paredes, escadas e **portas fechadas** bloqueiam a visão — só dá para ver o
   outro lado de uma porta quando se está parado nela.
2. **Iluminação.** Cada fonte de luz (o próprio jogador e as tochas presas nas
   paredes) espalha uma cor RGB usando o mesmo cálculo de linha de visão, mais
   forte perto e mais fraca longe. As luzes se **somam**. A luz do jogador
   diminui conforme ele desce (raio 9 no andar 1, raio 5 no andar 10).
3. **O que aparece na tela.** Uma célula é visível se está na linha de visão e
   recebe luz suficiente. A cor desenhada é a cor do objeto multiplicada pela
   cor da luz, convertida para a paleta de 256 cores do terminal.
4. **Memória.** Tudo o que já foi visto fica marcado no andar e continua
   desenhado, escuro e azulado, quando sai da visão.

## Como funcionam os monstros

Cada andar recebe de 4 a 11 monstros, sorteados entre as espécies daquele
andar (sorteio com pesos: espécies mais comuns têm mais chance). Eles nascem a
pelo menos 12 passos da escada de chegada, com nível de cerca de 2 x o andar.

| | Espécie | Elemento | Golpes | Andares | Especial |
|---|---|---|---|---|---|
| `r` | rato | Normal | Mordida, Rosnado, Ataque Rápido | 1–4 | |
| `k` | kobold | Fogo | Brasa, Arranhão, Rosnado | 1–4 | |
| `j` | chacal | Normal | Mordida, Ataque Rápido | 2–6 | rápido, anda em dupla |
| `m` | macaco | Planta | Arranhão, Folha Navalha, Rosnado | 2–6 | foge com pouca vida |
| `t` | sapo | Água | Jato d'Água, Língua Grudenta | 2–6 | |
| `g` | goblin | Veneno | Ferrão Venenoso, Golpe, Grito de Guerra | 3–10 | |
| `v` | morcego vampiro | Normal | Sugar Sangue, Ataque Rápido, Hipnose | 4–9 | rápido, voo errático |
| `O` | ogro | Pedra | Esmagar, Rugido, Golpe | 6–10 | |
| `N` | naga | Água | Chicote d'Água, Hipnose, Ferrão Venenoso | 8–10 | |

**Estados no mapa.** Cada monstro está em um de quatro estados:

- **Dormindo** — parado. Se o jogador passar perto (7 passos, ou 3 se estiver
  descansando), tem 1/3 de chance por turno de acordar.
- **Vagando** — anda até um destino sorteado (seguindo a BFS a partir do
  destino). Se vir o jogador a até 14 passos, começa a caçar.
- **Caçando** — se vê o jogador, segue a BFS a partir do jogador (o caminho
  mais curto). Se perde de vista, segue o **rastro de cheiro**: a cada turno o
  jogador "marca" as células que ele vê com um número que cresce com o tempo, e
  o monstro anda sempre para o vizinho de número maior. Depois de 20 turnos
  sem ver o jogador, desiste e volta a vagar.
- **Fugindo** — os medrosos, com 1/4 da vida, vão para onde a distância até o
  jogador aumenta, e se recuperam aos poucos.

Todos os movimentos usam a mesma função: olhar os 8 vizinhos num "mapa de
valores" e ir para o melhor.

## Como funciona a batalha

Quando o jogador anda na direção de um monstro (ou um monstro caçando alcança o
jogador), abre a **tela de batalha**, no estilo dos Pokémon clássicos. Outros
monstros **acordados e caçando a até 3 passos** entram junto — no máximo
**3 inimigos**, lado a lado na tela, cada um com sua barra de vida. Ao escolher
um golpe, o jogador escolhe também o **alvo**.

**Rodada.** O jogador escolhe **LUTAR** (um dos golpes), **ITENS** (Poção de
Vida, cura metade da vida), **STATUS** (ficha dos dois lutadores, com as
fraquezas do inimigo) ou **FUGIR**. Cada monstro sorteia um golpe, evitando os
que não fariam nada. Itens e fuga acontecem primeiro; depois todos agem em
ordem: golpes com prioridade primeiro e, empatando, o mais **rápido**. A chance
de fugir é calculada contra o inimigo mais rápido. No fim da rodada,
quem está envenenado perde vida.

**Dano** (fórmula no estilo Pokémon):

```
dano = ((2 x nível / 5 + 2) x poder x ataque / defesa) / 50 + 2
```

multiplicado por: x1,5 se o golpe é do mesmo elemento de quem usa; eficácia do
elemento (x0,5, x1 ou x2); x1,5 em golpe crítico (chance de 1/16); e um sorteio
entre 85% e 100%.

**Elementos.** Normal, Fogo, Água, Planta, Veneno e Pedra. Fogo vence Planta;
Água vence Fogo e Pedra; Planta vence Água e Pedra; Veneno vence Planta; Pedra
vence Fogo. A tabela completa está em `batalha.cpp` (`TABELA_DE_EFICACIA`).

**Status.** Envenenado (perde 1/8 da vida por rodada), dormindo (não age por 1
a 3 rodadas) e paralisado (velocidade 1/4 e às vezes não consegue agir).
Monstros encontrados **dormindo** no mapa começam a batalha dormindo.

**Golpes e PP.** Cada lutador sabe até 4 golpes. Os do jogador têm usos
limitados (PP), que voltam aos poucos fora da batalha; o *Golpe* básico é
infinito. Ao subir de nível o jogador aprende golpes novos (Tocha no nível 7,
Jato d'Água no 11, Pó do Sono no 15...) e, se já souber 4, escolhe qual esquecer.

**Níveis.** Atributos crescem com o nível: `atributo = 2 x base x nível / 100 + 5`.
Vencer dá `experiência-base x nível do monstro / 5` pontos; o nível n exige
`4/5 x n³` pontos no total.

**Vidas.** O jogador tem **3 vidas**. Perder uma batalha gasta uma vida e o
jogador acorda na entrada do andar, com vida e PP cheios. Sem vidas, fim de jogo.

**Ponteiros na batalha.** A ficha de batalha (`struct Lutador`) guarda
*ponteiros* para a vida e os PP de verdade do jogador e do monstro: o que
acontece na batalha já fica salvo no `Jogador` e no `Monstro` do mapa.



## Itens

Itens ficam no chão (2 a 4 por andar) e são pegos automaticamente ao passar por
cima. A mochila (`i`) mostra o que o jogador carrega; na batalha, o menu
**ITENS** mostra as poções.

| Símbolo | Tipo | Exemplos | Efeito |
|---|---|---|---|
| `!` | Poções | Poção de Vida, Poção Grande, Antídoto, Éter | Cura metade / toda a vida; tira veneno, sono e paralisia; recupera os PP. Somem ao usar. |
| `)` | Armas | Adaga, Espada Curta, Machado, Espada Longa | Ataque +1 a +6 |
| `)` | Armas elementais | Espada de Fogo, Tridente, Chicote de Espinhos, Adaga Venenosa, Martelo de Pedra | Ataque +3 e golpes do mesmo elemento causam +25% |
| `]` | Escudos | Madeira, Ferro, Torre | Defesa +2, +4, +6 (o de torre tira 3 de velocidade) |
| `*` | Cristais (power-ups) | Vida, Força, Ferro, Pena do Vento | Bônus **permanente** assim que são pegos (+4 de vida ou +2 no atributo) |
| `?` | Pergaminhos | de Fogo, Água, Planta, Veneno, Pedra | Ensinam um golpe daquele elemento |
| `-` | Chave | | Abre a porta trancada do andar |

**Sala do tesouro.** Quase todo andar tem uma sala trancada com um equipamento
bom, um cristal e às vezes uma Poção Grande. A chave fica no chão em algum
lugar do andar ou com um dos monstros, que a deixa ao ser derrotado.

**Equipamento com ponteiros.** A arma e o escudo equipados são *ponteiros*
para a ficha do item na tabela `TIPOS_DE_ITEM` (ou `nullptr` se a mão está
vazia). Na hora de trocar, `equipar()` usa um ponteiro para ponteiro
(`const TipoItem** lugar`) para tratar arma e escudo com o mesmo código.

**Atributos finais** = valor do nível + bônus dos cristais + bônus do equipamento
(ver `recalcularAtributos()` em `jogo.cpp`).

## Tempo real e descanso

O **mapa anda em tempo real**: o laço principal (`main.cpp`) espera uma tecla
por no máximo 50 ms e, com ou sem tecla, chama `avancarTempo()`, que faz o
relógio do jogo andar.

- **Monstros** acumulam tempo e agem quando completam o seu intervalo:
  `560 - 6 x velocidade` milissegundos (mínimo 220). Os **rápidos** (chacal,
  morcego) agem no dobro do ritmo; quem está vagando ou dormindo vai 1,5x mais
  devagar. Como a velocidade cresce com o nível, os monstros do fundo são mais
  ligeiros.
- **O jogador** anda na hora em que aperta a tecla, mas precisa esperar o seu
  intervalo entre um passo e outro: `260 - 3 x velocidade` ms (mínimo 100).
  Assim o atributo **velocidade** afeta a velocidade de movimento.
- **Batalhas e menus** param o relógio do mapa.
- **Descanso:** a vida **não volta andando**. Um toque em `Z` começa a
  descansar: **+1 de vida por segundo** e +1 PP em cada golpe a cada 5 segundos.
  Qualquer tecla, um monstro aparecendo ou uma batalha interrompem. Descansando,
  o jogador faz menos barulho (os monstros percebem a metade da distância).

## Equilíbrio

Medido com dois robôs que jogam sozinhos, em tempo simulado (300 partidas
cada). Cada batalha mede quantas rodadas durou e quanto de vida custou.

| | Rodadas por batalha | Vida perdida por batalha | Chega ao andar 10 |
|---|---|---|---|
| **Explorador** (limpa cada andar e abre as salas do tesouro) | 2 a 4 | 7% a 15% | 55% das partidas |
| **Mergulhador** (só desce, sem explorar) | 2 a 4 | 11% a 50% | quase nunca (morre nos andares 3 a 5) |

Os números saíram de ajustes medidos: ataque-base do jogador 50, bônus de
armas e cristais moderados, monstros com 15% a mais de vida que a fórmula do
jogador (`VIDA_EXTRA_DOS_MONSTROS` em `monstros.cpp`).



## Personagem e atributos (Etapa 6)

Na criação o jogador escolhe **nome**, **elemento** (muda o golpe inicial, o
bônus de 50% nos golpes do mesmo elemento e as fraquezas), **cor** e distribui
**5 pontos**. A cada nível ganha **1 ponto**, gasto na ficha (tecla `C`).

| Atributo | Efeito por ponto | Onde é usado |
|---|---|---|
| Força | +1 de ataque | fórmula de dano |
| Constituição | +3 de vida máxima, +1 de defesa a cada 2 pontos | vida e armadura |
| Destreza | +3% de precisão, +1% de crítico | `usarGolpe` e `calcularDano` |
| Agilidade | +2% de esquiva, +1 de velocidade | chance de o inimigo errar, ordem na batalha, passo no mapa |

Chance de acertar = precisão do golpe + precisão do atacante − esquiva do alvo.
Os monstros têm 5% de esquiva (15% os rápidos).

**Elementos dos monstros:** a cada partida, `sortearElementosDasEspecies`
embaralha os 6 elementos (Fisher-Yates) e distribui em ordem de andar, então as
espécies que aparecem juntas ficam com elementos diferentes. O primeiro golpe de
dano simples de cada espécie vira o golpe daquele elemento (`golpesDoMonstro`).

## Mapa vivo (Etapa 7)

- **Armadilhas** (`cenario.cpp`): escondidas no chão, longe da entrada. Quem pisa
  perde 15% (jogador) ou 20% (monstro) da vida máxima. Ao passar do lado, o
  jogador pode percebê-las (20% + 5% por ponto de Destreza).
- **Caixas:** ficam no meio das salas (as 8 vizinhas livres, então nunca fecham
  um caminho). Andar contra uma caixa a empurra; se não der, o jogador a quebra.
  Caixa empurrada sobre uma armadilha a desarma. Ogros e o chefe despedaçam as
  caixas encostadas neles.
- **Conteúdo (v1.4):** 70% das caixas escondem um item, guardado em
  `Andar::conteudoDasCaixas[y][x]` (anda junto quando a caixa é empurrada).
  Poção de Vida 35%, arma do andar 25%, Antídoto 12%, Éter 10%, Poção Grande 10%,
  cristal 8%. Quando a caixa quebra, por qualquer motivo, o item cai no chão.
- **Atordoar (v1.4):** empurrar uma caixa contra um monstro a despedaça e deixa o
  monstro parado por `TEMPO_ATORDOADO_MS` (5 s), com estrelinhas na cabeça. Se a
  batalha começar enquanto ele está tonto, ele perde a primeira rodada
  (começa `ADORMECIDO` por 1 rodada) — vale até contra o chefe.
- **NPCs** (`npcs.cpp`): Borin, o mineiro (andar 4), destranca a sala do tesouro.

## História, NPCs e chefe (Etapa 8)

A história aparece depois da criação (`textoDaHistoria`). Três NPCs: Eldra
(andar 1, dá uma poção e dicas), Borin (andar 4, abre a sala do tesouro) e o
espírito do guardião (andar 7, cura vida e PP). O **Senhor das Profundezas**
fica no ponto mais distante da entrada do andar 10; não dá para fugir dele.
Derrotá-lo vence o jogo.

## Pontuação e ranking (Etapa 9)

`calcularPontuacao`: +250 por andar alcançado, +30 por monstro, +20 por item,
−1 a cada 5 passos, −300 por vida perdida e +3000 pela vitória. No fim da
partida, `registrarNoRanking` grava os 10 melhores em `ranking.txt`
(uma linha por partida: `pontos;andar;venceu;nome`).

## Equilíbrio com as etapas 6 a 9

Medido com os robôs de teste: o explorador perde de 5% a 7% da vida por batalha
e chega ao andar 10 na maioria das partidas; quem só desce morre entre os andares
3 e 6. Contra o chefe (nível 24), um jogador de nível 22 vence cerca de 35% das
tentativas — com 3 vidas, a chance total fica perto de 70%.


## Som e música (v1.1)

Não há nenhum arquivo de áudio: tudo é **gerado no código** (`src/grafico/audio.cpp`),
como nos videogames de 8 bits.

- **Ondas:** quadrada (o "bip" clássico), triangular (som macio, usado no baixo)
  e ruído (números aleatórios, usado em pancadas e espinhos).
- **Efeitos:** cada um é uma lista de notas `{frequência, frequência final,
  duração, onda, volume}`. `gerarSom` calcula as amostras (22050 por segundo)
  com um envelope que entra rápido e vai sumindo, e entrega para a raylib.
- **Música:** um sequenciador de duas vozes (melodia + baixo) roda dentro do
  callback de áudio da raylib. Cada música é uma lista de notas MIDI
  (69 = Lá 440 Hz; frequência = 440 x 2^((nota-69)/12)), tocada em loop.
  As 4 melodias foram compostas para o jogo.
- A lógica só chama `tocarSom(SOM_GOLPE)` ou `tocarMusica(MUSICA_CHEFE)` (`som.h`).
  Na versão de terminal essas funções não fazem nada (`console/sem_som.cpp`).
- Tecla `O` (ou a opção na pausa) liga e desliga o som. Sem placa de som, o jogo
  segue mudo.


## Dificuldades (v1.2)

Escolhida na criação do personagem (`src/dificuldade.h/.cpp`). Cada nível é uma
linha de uma tabela; as regras leem `dificuldadeAtual()` em vez de números fixos.

| | Fácil | Médio | Difícil |
|---|---|---|---|
| Vida dos monstros | −15% | +10% | +30% |
| Ataque dos monstros (≈ dano) | 75% | 100% (era 95% até a v1.7) | 125% |
| Intervalo entre passos dos monstros | 130% (mais lentos) | 110% | 100% |
| Dano das armadilhas | 6% da vida | 10% | 15% |
| Experiência ganha | 150% | 125% | 100% |
| Vidas / poções no início | 5 / 6 | 3 / 3 (eram 4 até a v1.7) | 3 / 3 |
| Pontuação final | 50% | 100% | 150% |
| Preços na loja (v1.8) | 75% | 100% | 125% |

Medido com os robôs (150 partidas cada):

| | Fácil | Médio | Difícil |
|---|---|---|---|
| Explorador: vida perdida por batalha | ~1% | 2–4% | 5–7% |
| Quem só desce: chega ao andar 10 | 87% | 7% | 0% |
| Chefe vencido (por tentativa, jogador nível 22) | 95% | 74% | 35% |


## Mensagens importantes (v1.3)

Cada mensagem tem um tipo (`TipoMensagem` em `jogo.h`): **normal**, **boa**
(dourado: subir de nível, ponto de atributo, golpe novo, crítico/super eficaz
seu, chave, porta aberta) ou **perigo** (vermelho: status em você, crítico/super
eficaz do inimigo, armadilha, vida baixa, grupo de monstros, chefe, desmaio).

- **Batalha:** `narrar(jogo, b, texto, tipo)`. Nas mensagens boas e de perigo as
  teclas ficam travadas por `TEMPO_DE_LEITURA_MS` (0,9 s): o laço desenha a tela
  e lê as teclas só para jogá-las fora. Assim quem aperta Enter sem parar não
  pula o aviso. A caixa ganha cor, ícone e uma borda que pisca no começo.
- **Mapa:** `adicionarMensagem(jogo, texto, tipo)` guarda o tipo junto no
  histórico; as importantes aparecem também numa faixa no topo do mapa por
  `TEMPO_DA_FAIXA_MS` (3 s). Com menos de 25% de vida, a barra pisca.

## Botão Aleatório (v1.3)

Na criação do personagem, `sortearPersonagem` sorteia nome (de uma lista de 30
nomes), elemento, cor e os 5 pontos de atributo. A dificuldade não é sorteada, e
um nome digitado pelo jogador é mantido.

## Animações (v1.5)

Só a versão gráfica anima; no terminal as funções `animarBatalha` e
`efeitoNoMapa` existem mas não fazem nada. A lógica só avisa **o que aconteceu**
(ex.: "o jogador acertou o rato com Fogo, 9 de dano, crítico") e a parte gráfica
decide como mostrar.

- **`src/grafico/efeitos.cpp`:** partículas (estrelas, chamas, bolhas, folhas,
  pedras, lascas, espinhos), números flutuantes, tremor da tela, clarão, o
  escurecimento ao trocar de tela, o "afundar" do botão clicado e o destaque
  dourado que desliza entre as opções. A posição de cada partícula é calculada
  pelo tempo (`x = x0 + v·t + g·t²/2`), então não depende da velocidade do PC.
- **Batalha (`animarBatalha` em `tela_grafica.cpp`):** entrada dos lutadores,
  bote de quem ataca, partículas do elemento no alvo, alvo piscando com o dano
  subindo (crítico maior, dourado e com tremor), "ERROU" com pulinho, cura com
  brilho verde, desmaio afundando e sumindo, e brilho dourado ao subir de nível.
  Cada animação dura de 0,5 a 0,8 s e não roda no modo automático dos testes.
- **Mapa:** o desenho "persegue" a posição real (`PosicaoSuave`), então o
  jogador e os monstros deslizam entre as casas e a câmera acompanha. Itens
  flutuam, tochas tremulam, e armadilha, caixa quebrada, item pego, monstro
  atordoado e descanso têm efeito próprio (`efeitoNoMapa`).
- **Telas paradas continuam animando:** cada função `desenhar...` registra com
  `definirRedesenho` como se redesenhar; enquanto o jogo espera uma tecla, essa
  função é chamada a cada quadro.

## Textos maiores (v1.6)

As falas, a história, a narração da batalha e as mensagens ficaram maiores. Para
nada sair da caixa, `visual.cpp` ganhou `quebrarLinhas` (quebra a frase nos
espaços conforme a largura real das letras), `textoQuebrado` (desenha já
quebrado) e `textoQueCabe` (numa linha só, diminui a letra se não couber). As
páginas de texto usam a maior letra (até 28) em que todas as linhas cabem.

## Moedas e o comerciante (v1.7)

- **Ganhar moedas** (`loja.cpp`): cada monstro derrotado dá `2 + nível/3` moedas
  (mais 0 a 2 de sorte); cada nível novo dá 5 moedas; e metade das caixas que não
  têm item guarda `5 + 2 × andar` moedas (mais 0 a 5). Dentro da caixa isso é
  guardado como `MOEDAS_NA_CAIXA + quantidade` (`cenario.h`), e só o jogador
  recolhe: se um ogro quebrar a caixa, as moedas se perdem.
- **Garro, o comerciante** (`npcs.cpp`): aparece nos andares 3, 6 e 9
  (`repeteACada = 3` na ficha). Na primeira conversa ele se apresenta; depois,
  andar até ele abre direto a loja.
- **A loja** (`abrirLoja`): poções sempre à venda e 2 itens especiais, só 1 de
  cada (`prepararEstoque`): no andar 3, a arma do elemento do jogador e um escudo
  de madeira; no 6, escudo de ferro e cristal de vida; no 9, espada longa e
  cristal de força. Ele também compra itens da mochila pela metade do preço.
- **Pontos:** cada moeda guardada no fim vale 1 ponto, então existe a escolha
  entre gastar para sobreviver ou guardar para o ranking.
- **Balanceamento (bots, 150 partidas):** quem explora tudo chega ao comerciante
  com ~65, ~170 e ~235 moedas novas (andares 3, 6 e 9); quem só corre para a
  escada junta bem menos (~20 no andar 3). Na v1.7, com poções à vontade na
  loja, o bot que só corre chegava ao andar 10 em ~20% das partidas no Médio:
  fácil demais. A v1.8 corrigiu isso (ver abaixo).

## Lojinha, limite da mochila e chave por andar (v1.8)

- **Lojinha do Garro** (`prepararSalaDaLoja` em `npcs.cpp`): logo depois de
  gerar o mapa, testa cada porta como se fosse parede (a mesma ideia da sala do
  tesouro). A salinha isolada mais perto da chegada (9 a 80 casas, sem escada e
  sem outra porta) vira a loja: `Andar::zonaSegura` marca as casas dela, a parede
  mais longe da porta vira um lampião (tocha) e o comerciante fica no meio. Em
  ~92% dos andares 3, 6 e 9 existe uma salinha assim; nos outros, um tapete 5×5
  em volta dele vira a área protegida.
- **Monstros e a zona segura** (`monstros.cpp`): não nascem nela
  (`povoarAndar`), não pisam nela (`celulaLivre`), não a escolhem como destino e
  não atacam o jogador que está dentro dela (`podeAtacarJogador`).
- **Limite da mochila** (`itens.cpp`): `limiteDoItem` (6 Poções de Vida, 3 de
  cada outra poção, 1 de cada arma/escudo/pergaminho de reserva) e
  `LIMITE_DA_MOCHILA = 12` no total. A chave e os cristais não ocupam espaço.
  Com a mochila cheia, pisar num item abre a troca: o item escolhido fica no
  chão, no mesmo lugar. Presentes de NPC e o item que sai da mão ao equipar
  usam `guardarOuLargar` (se não couber, fica no chão).
- **Chave por andar** (`irParaAndar` em `jogo.cpp`): ao trocar de andar, as
  chaves ficam guardadas em `Andar::chavesGuardadas` e voltam se o jogador
  voltar. Ao pegar a chave, `revelarPortaDoTesouro` marca a porta no mapa e diz
  a direção. O andar 4 não tem chave (o Borin abre a porta).
- **Estoque limitado e novo equilíbrio do Médio:** o Garro traz poucas poções
  por andar (`prepararEstoque` em `loja.cpp`): andares 3 e 6, 1 Poção de Vida,
  1 Antídoto e 1 Éter; andar 9, mais 1 Poção Grande. Os testes mostraram que a
  Poção Grande era o que mais facilitava. Os preços seguem a dificuldade
  (`precosDaLoja`: 75%, 100%, 125%). No Médio, o ataque dos monstros foi de 95%
  para 100% e as poções iniciais de 4 para 3. Resultado (bots, 400 partidas de
  quem só desce e 100 de quem explora, comprando tudo o que pode na loja):

  | | Fácil | Médio | Difícil |
  |---|---|---|---|
  | Quem só desce: chega ao andar 10 | 83% | 9% | 0% |
  | Quem explora: chega ao andar 10 | 86% | 85% | 73% |
  | Chefe vencido (Médio, jogador nível 22) | | 69% | |
