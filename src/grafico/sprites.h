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
    "....KKJBBJKK....",
    "...KBJJJJJBK....",
    "...KBJMMJBJK....",
    "...KBJMMJBJK....",
    "....KJJJJJK.....",
    ".....KJBBJK.....",
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

inline const Sprite EXPLORADORA = {{
    "................",
    ".....KKKKKK.....",
    "....KHHHHHHK....",
    "...KHHSSSSHK....",
    "...KHSWSSWSK....",
    "....KKSSSSK.....",
    ".....KKSSKK.....",
    "....KTTTTTTK....",
    "...KTTFTTFTTK...",
    "...KSKTTTTKSK...",
    "....KKTTTTKK....",
    ".....KBBKBBK....",
    ".....KBBKBBK....",
    ".....KCCKCCK....",
    ".....KKK.KKK....",
    "................"
}};

inline const Sprite MINEIRO = {{
    "................",
    ".....KKKKKK.....",
    "....KYYYYYYK....",
    "...KYYYWWYYYK...",
    "...KKKKKKKKKK...",
    "....KSWSSWSK....",
    "....KSSSSSSK....",
    "....KFFSSFFK....",
    "...KCFFFFFFCK...",
    "..KSKCCFFCCKSK..",
    "..KSKCCCCCCKSK..",
    "...KKBBBBBBKK...",
    "....KBBKKBBK....",
    "....KBBK.KBBK...",
    "....KKK...KKK...",
    "................"
}};

inline const Sprite ESPIRITO = {{
    "................",
    "......KKKK......",
    ".....KEEEEK.....",
    "....KEEEEEEK....",
    "....KEKEEKEK....",
    "....KEEEEEEK....",
    ".....KETTEK.....",
    "....KEEEEEEK....",
    "...KEETEETEEK...",
    "...KEEEEEEEEK...",
    "....KEEEEEEK....",
    "....KETEETEK....",
    ".....KEEEEK.....",
    "......KETK......",
    ".......KK.......",
    "................"
}};

inline const Sprite SENHOR = {{
    "..K..........K..",
    ".KOK........KOK.",
    ".KOOK......KOOK.",
    "..KOOKKKKKKOOK..",
    "...KOOOOOOOOK...",
    "..KOOYOOOOYOOK..",
    "..KOOKOOOOKOOK..",
    "..KOOOOWWOOOOK..",
    "...KOKWKKWKOK...",
    "..KKOOOOOOOOKK..",
    ".KOOKOOOOOOKOOK.",
    "KOOKKOOOOOOKKOOK",
    "KOK.KOOKKOOK.KOK",
    "KK..KOOK.KOOK.KK",
    "....KKK...KKK...",
    "................"
}};
