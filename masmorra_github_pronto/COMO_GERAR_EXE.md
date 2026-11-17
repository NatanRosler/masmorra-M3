# Como gerar e distribuir o jogo no Windows

### O que mudou na janela

O arquivo `CMakeLists.txt` agora marca o executável do Windows como **aplicativo gráfico**. Assim, ao abrir `masmorra_2d.exe` com dois cliques, a janela preta do console não deverá aparecer. A mudança afeta apenas a forma de abertura do executável, sem alterar os cálculos do jogo. Para compiladores MSVC, também há configuração de ponto de entrada.

**Importante:** é preciso **recompilar** o jogo. O `.exe` antigo ainda abrirá a janela preta.

### Gerar o jogo

Com CMake, g++ e MinGW Make funcionando, abra esta pasta no VS Code. Se você já tem raylib 5.5 no computador, pode copiar a pasta `raylib-5.5` para cá. Confira que existe `raylib-5.5/src/raylib.h`.

No terminal PowerShell, execute:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
.\build\masmorra_2d.exe
```

Se aparecer erro por cache antigo de compilação, apague **somente a pasta `build` deste projeto** e refaça o processo. Se o download da raylib falhar por certificado SSL, coloque o código-fonte extraído da raylib 5.5 em `raylib-5.5`, como você fez anteriormente.

### Criar o ZIP que os amigos podem jogar

Depois de compilar, dê dois cliques em `preparar_masmorra_portatil.bat`. Ele copiará o `.exe` e algumas DLLs necessárias e criará:

`dist/Masmorra_2D_Windows.zip`

Extraia e teste esse ZIP em outro computador Windows. Algumas máquinas podem exigir dependências adicionais. O arquivo `compilar_e_preparar_windows.bat` faz a compilação e o empacotamento de uma vez.

### Diferença entre os arquivos ZIP

- `masmorra_github_sem_console.zip`: código completo para guardar no GitHub e modificar.
- `dist/Masmorra_2D_Windows.zip`: executável do Windows para quem quer apenas jogar, gerado **no seu computador**.

Este pacote de código não inclui um `.exe` pronto, pois ele precisa ser compilado para a plataforma de destino. Você já tem todas as ferramentas no seu PC para fazê-lo.
