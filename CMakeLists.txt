# -----------------------------------------------------------------------------
# MASMORRA - compilação com CMake
#
#   masmorra          versão gráfica 2D (raylib)     -> a versão principal
#   masmorra_console  versão de terminal (texto)     -> opcional
#
# As duas usam OS MESMOS arquivos de lógica em src/. Só muda a parte que
# desenha a tela e lê o teclado (src/grafico ou src/console).
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(masmorra VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Lógica do jogo (mapa, visão, monstros, itens, batalha...).
set(FONTES_DA_LOGICA
    src/aleatorio.cpp
    src/batalha.cpp
    src/gerador.cpp
    src/itens.cpp
    src/jogo.cpp
    src/mapa.cpp
    src/monstros.cpp
    src/visao.cpp
    src/personagem.cpp
    src/cenario.cpp
    src/npcs.cpp
    src/loja.cpp
    src/pontuacao.cpp
    src/dificuldade.cpp
)

# Junta tudo que é estático, para o jogo rodar em outro computador sem
# precisar instalar nada (no Windows: nada de libstdc++-6.dll etc.).
function(configurar_executavel alvo)
    target_include_directories(${alvo} PRIVATE src)
    if(MSVC)
        target_compile_options(${alvo} PRIVATE /W4)
    else()
        target_compile_options(${alvo} PRIVATE -Wall -Wextra)
    endif()
    if(MINGW)
        target_link_options(${alvo} PRIVATE -static -static-libgcc -static-libstdc++)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(${alvo} PRIVATE -static-libgcc -static-libstdc++)
    endif()
endfunction()

# ------------------------------- versão gráfica ------------------------------
option(MASMORRA_GRAFICO "Compilar a versao grafica (raylib)" ON)
if(MASMORRA_GRAFICO)
    # A raylib 5.5 já vem junto no projeto (externo/raylib-5.5), então não
    # precisa baixar nada. Só se a pasta faltar é que ela é baixada.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/externo/raylib-5.5/CMakeLists.txt")
        add_subdirectory(externo/raylib-5.5 raylib_build EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(raylib
            URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(raylib)
    endif()

    add_executable(masmorra ${FONTES_DA_LOGICA}
        src/grafico/main.cpp
        src/grafico/tela_grafica.cpp
        src/grafico/terminal_raylib.cpp
        src/grafico/visual.cpp
        src/grafico/menu_principal.cpp
        src/grafico/audio.cpp
        src/grafico/efeitos.cpp
    )
    configurar_executavel(masmorra)
    target_link_libraries(masmorra PRIVATE raylib)
    if(WIN32)
        # Aplicativo de janela: não abre o console preto junto.
        set_target_properties(masmorra PROPERTIES WIN32_EXECUTABLE TRUE)
        if(MSVC)
            target_link_options(masmorra PRIVATE "/ENTRY:mainCRTStartup")
        endif()
        target_link_libraries(masmorra PRIVATE winmm)
    endif()
endif()

# ------------------------------- versão console ------------------------------
add_executable(masmorra_console ${FONTES_DA_LOGICA}
    src/console/main.cpp
    src/console/tela.cpp
    src/console/terminal.cpp
    src/console/sem_som.cpp
)
configurar_executavel(masmorra_console)
