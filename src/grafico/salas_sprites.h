#pragma once
#include "sprites.h"
// Desenhos das salas especiais (salas.cpp), na legenda de cores de visual.cpp.

// Fonte cristalina
inline const Sprite FONTE = {{
    "................",
    "......K..K......",
    ".....KUKKUK.....",
    "....KUKKKKUK....",
    "....KKKUWKKK....",
    "...KUKKUWKKUK...",
    "....K.KUUK.K....",
    "......KUUK......",
    "..KKKKKUUKKKKK..",
    ".KLLLLLLLLLLLLK.",
    ".KNNUUUUUUUUNNK.",
    ".KNNNNNNNNNNNNK.",
    "..KNNNNNNNNNNK..",
    "..KNNNNNNNNNNK..",
    "...KNNNNNNNNK...",
    "....KKKKKKKK...."
}};

// Fonte seca
inline const Sprite FONTE_SECA = {{
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "..KKKKKKKKKKKK..",
    ".KLLLLLLLLLLLLK.",
    ".KNNHHHHHHHHNNK.",
    ".KNNNNNNNNNNNNK.",
    "..KNNNNNNNNNNK..",
    "..KNNNNNNNNNNK..",
    "...KNNNNNNNNK...",
    "....KKKKKKKK...."
}};

// Altar de sangue
inline const Sprite ALTAR = {{
    "................",
    "..K..........K..",
    ".KXK........KXK.",
    ".KYK........KYK.",
    ".KFK........KFK.",
    ".KFK........KFK.",
    ".KFK........KFK.",
    ".KFKKKKKKKKKKFK.",
    ".KLLLrrrrrrLLLK.",
    ".KLLLLrLLrLLLLK.",
    "..KKNNNNNrNNKK..",
    "...KNNNNNNNNK...",
    "...KNBBBBBBNK...",
    "...KNNNNNNNNK...",
    "...KNNNNNNNNK...",
    "....KKKKKKKK...."
}};

// Altar já usado
inline const Sprite ALTAR_USADO = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KFK........KFK.",
    ".KFK........KFK.",
    ".KFK........KFK.",
    ".KFKKKKKKKKKKFK.",
    ".KLLLOOOOOOLLLK.",
    ".KLLLLOLLOLLLLK.",
    "..KKNNNNNONNKK..",
    "...KNNNNNNNNK...",
    "...KNBBBBBBNK...",
    "...KNNNNNNNNK...",
    "...KNNNNNNNNK...",
    "....KKKKKKKK...."
}};

// Baú dourado
inline const Sprite BAU_DOURADO = {{
    "................",
    "................",
    "................",
    "...KKKKKKKKKK...",
    "..KMMMMMMMMMMK..",
    ".KCYMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KMMMMMYYMMMMMK.",
    ".KCCMCCYYCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    "..KKKKKKKKKKKK..",
    "................"
}};

// Baú aberto
inline const Sprite BAU_ABERTO = {{
    "................",
    "..KKKKKKKKKKKK..",
    ".KMMMMMMMMMMMMK.",
    ".KCCCCCCCCCCCCK.",
    ".KCCCCCCCCCCCCK.",
    ".KCCCCCCCCCCCCK.",
    "..KKKKKKKKKKKK..",
    "..KKKKYKKMKKKK..",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    ".KCCMCCCCCCMCCK.",
    "..KKKKKKKKKKKK..",
    "................"
}};

// Gongo da arena
inline const Sprite GONGO = {{
    "................",
    ".KKKKKKKKKKKKKK.",
    "KZZZZZZZZZZZZZZK",
    "KIKKKKKKKKKKKKIK",
    "KIK..KKCCKK..KIK",
    "KIK.KCCCCCCK.KIK",
    "KIKKCCCCCCCCKKIK",
    "KIKKCCYMMMCCKKIK",
    "KIKCCYMMMMCCCKIK",
    "KIKCCCMMMMCCCKIK",
    "KIKKCCMMMMCCKKIK",
    "KIKKCCCCCCCCKKIK",
    "KIK.KCCCCCCK.KIK",
    "KIK..KKCCKK..KIK",
    "KIK....KK....KIK",
    ".K............K."
}};

// Gongo depois da vitória
inline const Sprite GONGO_RACHADO = {{
    "................",
    ".KKKKKKKKKKKKKK.",
    "KZZZZZZZZZZZZZZK",
    "KIKKKKKKKKKKKKIK",
    "KIK...KHHKK..KIK",
    "KIK.KKHHHHHK.KIK",
    "KIKKHHKHHHHHKKIK",
    "KIKKHHKHHHHHKKIK",
    "KIKHHHHKHHHHHKIK",
    "KIKHHHHKHHHHHKIK",
    "KIKKHHHHKHHHKKIK",
    "KIKKHHHHKHHHKKIK",
    "KIK.KHHHHKHK.KIK",
    "KIK..KKHHKK..KIK",
    "KIK....KK....KIK",
    ".K............K."
}};

// Tobias, o viajante perdido
inline const Sprite VIAJANTE = {{
    "......KKKK......",
    ".....KVVVVK.....",
    "....KVVVVVVK....",
    "....KVSSSSVK.K..",
    "....KVSKSKVKKIK.",
    "....KVSSSSVKKIK.",
    "...KVVVHHVVVKIK.",
    "..KVVVVVVVVVVIK.",
    ".KCCVVVVVVVVSIK.",
    ".KCCVVVVVVVVKIK.",
    ".KCCVVVPPVVVKIK.",
    "..KCVVVPPVVVKIK.",
    "...KVVVVVVVVKIK.",
    "...KBBKKKKBBKK..",
    "...KKK....KKK...",
    "................"
}};

// Mímico (batalha)
inline const Sprite MIMICO = {{
    "..KKKKKKKKKKKK..",
    ".KCCCCCCCCCCCCK.",
    "KCMCCCCCCCCCCMCK",
    "KCMCYKKCCKKYCMCK",
    "KCMCCCCCCCCCCMCK",
    ".KWKWKWKWKWKWKWK",
    "..K.KKKKKKK.K.K.",
    ".KKKKrrrrrK.K.K.",
    "KWKWKWrrrKWKWKWK",
    "KCMCCCCrCCCCCMCK",
    "KCMCCCCCCCCCCMCK",
    "KCMMMMMMMMMMMMCK",
    "KCMCCCCCCCCCCMCK",
    "KCCCCCCCCCCCCCCK",
    ".KKKKKKKKKKKKKK.",
    "................"
}};
