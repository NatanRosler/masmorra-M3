cmake_minimum_required(VERSION 3.20)
project(masmorra_original_2d VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local folder raylib-5.5 if it exists, otherwise fetch raylib 5.5.
# Windows: put the EXTRACTED source folder raylib-5.5 here if SSL fails.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/raylib-5.5/CMakeLists.txt")
    add_subdirectory(raylib-5.5 raylib_build EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(raylib)
endif()

set(FONTES_ORIGINAIS
    src/aleatorio.cpp
    src/batalha.cpp
    src/gerador.cpp
    src/itens.cpp
    src/jogo.cpp
    src/mapa.cpp
    src/monstros.cpp
    src/visao.cpp
)
set(FONTES_GRAFICAS
    src/main.cpp
    src/tela_grafica.cpp
    src/terminal_raylib.cpp
    src/visual.cpp
    src/menu_principal.cpp
)
# No Windows, criar aplicativo grafico sem janela de console preta.
# Nos demais sistemas, o mesmo projeto continua com ponto de entrada main().
add_executable(masmorra_2d ${FONTES_ORIGINAIS} ${FONTES_GRAFICAS})
if(WIN32)
    set_target_properties(masmorra_2d PROPERTIES WIN32_EXECUTABLE TRUE)
    # O projeto original utiliza int main(argc, argv). No MSVC, preservamos
    # esse ponto de entrada com o subsistema grafico do Windows.
    if(MSVC)
        target_link_options(masmorra_2d PRIVATE "/ENTRY:mainCRTStartup")
    endif()
endif()
target_link_libraries(masmorra_2d PRIVATE raylib)
if(WIN32)
    target_link_libraries(masmorra_2d PRIVATE winmm)
endif()
if(APPLE)
    # Linkage for raylib is handled by its own CMake target.
endif()
