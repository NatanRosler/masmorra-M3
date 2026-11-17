#pragma once

// ARTE DOS PERSONAGENS - EDITE AQUI.
// Cada personagem usa 16 linhas com 16 caracteres por linha.
// '.' = transparente. A legenda das cores fica em visual.cpp (funcao pixelCor).

#include <array>

using Sprite = std::array<const char*, 16>;

inline const Sprite HEROI = {{
    "................",
    "......KKKK......",
    ".....KHHHHK.....",
    ".....KSSSSK.....",
    ".....KSWWSK.....",
    "......KSSK......",
    "....KKABBAKK....",
    "...KBAAAAABK....",
    "...KBAMMABAK....",
    "...KBAMMABAK....",
    "....KAAAAAK.....",
    ".....KABBAK.....",
    "....KBB..BBK....",
    "....KBB..BBK....",
    "....KK....KK....",
    "................"
}};

inline const Sprite GOBLIN = {{
    "................",
    "................",
    "....K......K....",
    "...KGGK..KGGK...",
    "....KGGGGGGK....",
    "....KDDGGDDK....",
    "....KGYGGYGK....",
    ".....KGGGGK.....",
    "......KRRK......",
    "....KKGGGGKK....",
    "...KGGDDDDGGK...",
    "...KGGDDDDGGK...",
    "....KGBBBGK.....",
    "....KBB..BBK....",
    ".....KK..KK.....",
    "................"
}};

inline const Sprite OGRO = {{
    "................",
    ".....KKKKKK.....",
    "....KGGGGGGK....",
    "...KGGGGGGGGK...",
    "...KGGGGGGGGK...",
    "...KGYGGGGYGK...",
    "...KGGGKKGGGK...",
    "....KGRRRRGK....",
    "...KKGFFFFGKK...",
    "..KGGGKKKKGGGK..",
    "..KGGGDDDDGGGK..",
    "..KGGGDDDDGGGK..",
    "...KGGDDDDGGK...",
    "...KBBK..KBBK...",
    "...KBBK..KBBK...",
    "....KK....KK...."
}};

inline const Sprite NAGA = {{
    "................",
    "......KKKK......",
    ".....KGGGGK.....",
    "....KGGGGGGK....",
    "....KGYGGYGK....",
    ".....KGRRGK.....",
    "......KGGK......",
    ".....KTTTTK.....",
    "....KTTGGTTK....",
    ".....KTTTTK.....",
    "......KTTK......",
    ".....KTTTTK.....",
    "....KTTKKTTK....",
    "....KTTTTTTK....",
    ".....KKTTKK.....",
    ".......KK......."
}};

inline const Sprite MORCEGO = {{
    "................",
    "................",
    "..K..........K..",
    ".KVK........KVK.",
    "KVVVKK....KKVVVK",
    "KVVVVVK..KVVVVVK",
    "KVVVVVVKKVVVVVVK",
    ".KVVKKVVVVKKVVK.",
    "..KK..KPPK..KK..",
    ".....KPRRPK.....",
    ".....KPFFPK.....",
    "......KPPK......",
    ".......KK.......",
    "................",
    "................",
    "................"
}};
