#pragma once
#include "sprites.h"
// v1.24: os mini-chefes (legenda de cores em visual.cpp).

// Rei Goblin (andar 3): coroa, capa vermelha e cinto de ouro.
inline const Sprite REI_GOBLIN = {{
    ".....Y..Y..Y....",
    "....KYKKYKKYK...",
    "....KYYrYYrYK...",
    ".K..KGGGGGGGK..K",
    ".KGKGGGGGGGGGKGK",
    "..KGGYKGGGYKGGK.",
    "...KGGGGGGGGGK..",
    "....KGWKKKWGK...",
    "..KRRKGGGGGKRRK.",
    ".KRRKGGDDDGGKRRK",
    ".KRKGGDMMMDGGKRK",
    ".KRKGGDDDDDGGKRK",
    "..KKKBBBBBBBKKK.",
    "....KBBK.KBBK...",
    "....KHHK.KHHK...",
    ".....KK...KK...."
}};

// Rainha Aranha (andar 6): coroa, muitos olhos e a ampulheta vermelha.
inline const Sprite RAINHA_ARANHA = {{
    ".....Y.YY.Y.....",
    "....KYKYYKYK....",
    "....KPPPPPPK....",
    "...KPRPRRPRPK...",
    ".VVKPPPPPPPPKVV.",
    "V..KPPWPPWPPK..V",
    "V...KPPPPPPK...V",
    ".VVKKKPPPPKKKVV.",
    "V.KPPPPPPPPPPK.V",
    ".VKPPPRRRRPPPKV.",
    "V.KPPPPRRPPPPK.V",
    ".VKPPPRRRRPPPKV.",
    "V.KPPPPPPPPPPK.V",
    "...KPPPPPPPPK...",
    "....KKPPPPKK....",
    "......KKKK......"
}};

// Necromante (andar 9): capuz, rosto de caveira e cajado com a joia verde.
inline const Sprite NECROMANTE = {{
    "..l.....KKK.....",
    ".lTl...KPPPK....",
    "..l...KPPPPPK...",
    "..N..KPPKKKPPK..",
    "..N..KPKWWWKPK..",
    "..N..KPKlWlKPK..",
    "..N..KPKWKWKPK..",
    "..NK.KPPKKKPPK..",
    "..NWKKPPPPPPPKK.",
    "..NKPPPVPPPVPPK.",
    "..N.KPPVPPPVPPK.",
    "..N.KPPVPPPVPPPK",
    "..N.KPPPPPPPPPPK",
    "..N.KPPPPPPPPPPK",
    "..NKPPPPPPPPPPPK",
    "..KKKKKKKKKKKKKK"
}};

// Golem de Magma, fase 1: a casca de pedra, com o fogo aparecendo nas rachaduras.
inline const Sprite GOLEM_PEDRA = {{
    "................",
    ".....KKKKKK.....",
    "....KNNNNNNK....",
    "....KNXNNXNK....",
    "....KNNNNNNK....",
    "..KKKKNNNNKKKK..",
    ".KNNNKNNNNKNNNK.",
    ".KNLNKNXNNKNLNK.",
    ".KNNNKNNXNKNNNK.",
    ".KNNNKNNNNKNNNK.",
    ".KKKKKNNNNKKKKK.",
    "..KNNNNKKNNNNK..",
    "..KNNNK..KNNNK..",
    "..KNXNK..KNXNK..",
    ".KNNNNK..KNNNNK.",
    ".KKKKKK..KKKKKK."
}};

// Golem de Magma, fase 2: a casca quebrou e sobrou o fogo.
inline const Sprite GOLEM_MAGMA = {{
    "...X.......X....",
    ".....KKKKKK..X..",
    "....KOXOOXOK....",
    "....KOYOOYOK....",
    "....KOXXXXOK....",
    "..KKKKOXXOKKKK..",
    ".KOXOKXYYXKOXOK.",
    ".KXYXKOXYXKXYXK.",
    ".KOXOKXYXOKOXOK.",
    ".KOOXKOXXOKXOOK.",
    ".KKKKKOXXOKKKKK.",
    "..KOXOXKKXOXOK..",
    "..KOXOK..KOXOK..",
    "..KXYXK..KXYXK..",
    ".KOXXOK..KOXXOK.",
    ".KKKKKK..KKKKKK."
}};
