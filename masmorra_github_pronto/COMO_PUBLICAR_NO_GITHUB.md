# Como publicar o Masmorra 2D no GitHub

Você pode enviar o código sem instalar nenhum programa adicional.

## 1. Antes de enviar

1. Extraia o ZIP `masmorra_github_sem_console.zip`.
2. Abra a pasta **masmorra_github_pronto**. Ela contém `src`, `original_console`, `docs`, `CMakeLists.txt`, `README.md`, `LICENSE` e os guias.
3. **Não envie a pasta `build` nem `raylib-5.5`**. Elas ficam fora do ZIP de código, e a raylib é baixada automaticamente pelo CMake em computadores com conexão funcionando.

## 2. Pelo navegador

1. Entre no repositório no GitHub e abra a aba **Code**.
2. Clique **Add file > Upload files**.
3. No Explorador do Windows, abra a pasta **masmorra_github_pronto**.
4. Selecione os arquivos e pastas de dentro dela e arraste para o local de upload (não o ZIP fechado). Para preservar a estrutura, arraste as pastas inteiras.
5. Confira se `src` e `original_console` aparecem no upload, sem uma pasta extra envolvendo tudo.
6. Em **Commit changes**, escreva `Versao 2D do Masmorra, com interface grafica` e confirme.

**Se o repositório já tiver arquivos com o mesmo nome**, o GitHub poderá pedir confirmação ou recusar a substituição na interface. Evite misturar os diretórios do projeto anterior com os novos. Nesse caso, crie outro repositório, por exemplo `masmorra-2d`, ou faça a atualização via GitHub Desktop.

## 3. Como disponibilizar o jogo para outras pessoas

O repositório guarda o **código-fonte**. Para oferecer um arquivo que outras pessoas possam abrir no Windows:

1. No seu próprio Windows, gere o jogo com `compilar_e_preparar_windows.bat`.
2. O resultado ficará em `dist/Masmorra_2D_Windows.zip`.
3. No GitHub, abra **Releases > Draft a new release**.
4. Crie a tag `v1.0.0`, escreva o título `Masmorra 2D para Windows`.
5. Anexe o arquivo `Masmorra_2D_Windows.zip` e publique a release.

Assim, quem quiser jogar baixa a release; quem quiser estudar ou modificar vê o código nas pastas do repositório. O arquivo `.exe` do Windows não roda nativamente no Linux nem no macOS. Os usuários desses sistemas precisam compilar o projeto.

## 4. Se quiser atualizar depois

No VS Code, salve suas alterações, compile o projeto e teste. Envie os arquivos de código alterados para o GitHub com uma nova mensagem de commit. Se quiser distribuir um jogo atualizado, gere um novo pacote e publique outra release.

Página oficial de ajuda do GitHub: https://docs.github.com/en/repositories/working-with-files/managing-files/adding-a-file-to-a-repository
