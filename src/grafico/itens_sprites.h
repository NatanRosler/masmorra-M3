#pragma once
#include "sprites.h"
#include "itens.h"
// Arte dos itens no chão e dos ícones do painel (16x16, mesma legenda de
// cores de visual.cpp). Para mudar um desenho, edite as letras abaixo.

// Poção de Vida
inline const Sprite ARTE_POCAO_DE_VIDA = {{
    "................",
    "................",
    ".......KK.......",
    "......KIIK......",
    "......KIIK......",
    "......KWWK......",
    "......KWWK......",
    ".....KrrrrK.....",
    "....KrWrrrrK....",
    "...KrWrrrrrrK...",
    "...KrWrrrrrrK...",
    "...KrrrrrrrrK...",
    "...KrrrrrrrrK...",
    "....KrrrrrrK....",
    ".....KrrrrK.....",
    "......KKKK......"
}};

// Poção Grande
inline const Sprite ARTE_POCAO_GRANDE = {{
    "................",
    ".......KK.......",
    "......KIIK......",
    "......KIIK......",
    ".....KKWWKK.....",
    "....KQQQQQQK....",
    "...KQQQQQQQQK...",
    "..KQQWQQQQQQQK..",
    "..KQWQQQQQQQQK..",
    ".KQQWQQQQQQQQQK.",
    ".KQQQQQQQQQQQQK.",
    ".KQQQQQQQQQQQQK.",
    "..KMMMMMMMMMMK..",
    "..KQQQQQQQQQQK..",
    "...KQQQQQQQQK...",
    "....KQQQQQQK...."
}};

// Antídoto
inline const Sprite ARTE_ANTIDOTO = {{
    "................",
    "................",
    ".......KK.......",
    "......KIIK......",
    "......KIIK......",
    "......KWWK......",
    "......KWWK......",
    ".....KGGGGK.....",
    "....KGWGGGGK....",
    "...KGWGGGGGGK...",
    "...KGWGGGGGGK...",
    "...KGGGGGGGGK...",
    "...KGGGGGGGGK...",
    "....KGGGGGGK....",
    ".....KGGGGK.....",
    "......KKKK......"
}};

// Éter
inline const Sprite ARTE_ETER = {{
    "................",
    "................",
    ".......KK.......",
    "......KIIK......",
    "......KIIK......",
    "......KWWK......",
    "......KWWK......",
    ".....KUUUUK.....",
    "....KUWUUUUK....",
    "...KUWUUUUUUK...",
    "...KUWUUUUUUK...",
    "...KUUUUUUUUK...",
    "...KUUUUUUUUK...",
    "....KUUUUUUK....",
    ".....KUUUUK.....",
    "......KKKK......"
}};

// Adaga
inline const Sprite ARTE_ADAGA = {{
    "................",
    ".............K..",
    "............KLK.",
    ".......K...KLNK.",
    "......KMK.KLNK..",
    ".......KMKLNK...",
    "........KMKK....",
    ".......KIKMK....",
    "......KIK.KMK...",
    ".....KMK...K....",
    "......K.........",
    "................",
    "................",
    "................",
    "................",
    "................"
}};

// Espada Curta
inline const Sprite ARTE_ESPADA_CURTA = {{
    "................",
    ".............K..",
    "............KLK.",
    "...........KLNK.",
    "..........KLNK..",
    ".....K...KLNK...",
    "....KMK.KLNK....",
    ".....KMKLNK.....",
    "......KMKK......",
    ".....KIKMK......",
    "....KIK.KMK.....",
    "...KMK...K......",
    "....K...........",
    "................",
    "................",
    "................"
}};

// Machado
inline const Sprite ARTE_MACHADO = {{
    "................",
    ".......K.KK.....",
    "......KIKLLKK...",
    ".....KKINLLLLK..",
    "....KNNINLLLLWK.",
    "....KNNINLLLLWK.",
    ".....KKINLLLLWK.",
    "......KINLLLLK..",
    "......KIKLLKK...",
    "......KIKKK.....",
    "......KIK.......",
    "......KIK.......",
    "......KIK.......",
    "......KIK.......",
    "......KIK.......",
    ".......K........"
}};

// Espada Longa
inline const Sprite ARTE_ESPADA_LONGA = {{
    "................",
    ".............K..",
    "............KLK.",
    "...........KLNK.",
    "..........KLNK..",
    ".........KLNK...",
    "........KLNK....",
    ".......KLNK.....",
    "..K...KLNK......",
    ".KMK.KLNK.......",
    "..KMKLNK........",
    "...KMKK.........",
    "..KIKMK.........",
    ".KIK.KMK........",
    "KMK...K.........",
    ".K.............."
}};

// Espada de Fogo
inline const Sprite ARTE_ESPADA_DE_FOGO = {{
    "................",
    ".............K..",
    "............KXK.",
    "...........KXYK.",
    "..........KXYK..",
    ".........KXYK...",
    "........KXYK....",
    "...K...KXYK.....",
    "..KMK.KXYK......",
    "...KMKXYK.......",
    "....KMKK........",
    "...KIKMK........",
    "..KIK.KMK.......",
    ".KMK...K........",
    "..K.............",
    "................"
}};

// Tridente
inline const Sprite ARTE_TRIDENTE = {{
    "...K...KK...K...",
    "..KUK.KWWK.KUK..",
    "..KUUKKUUKKUUK..",
    "..KUUKKUUKKUUK..",
    "..KUUKKUUKKUUK..",
    "..KAAAAAAAAAAK..",
    "...KKKKIIKKKK...",
    "......KIIK......",
    "......KIIK......",
    "......KIIK......",
    "......KIIK......",
    "......KIIK......",
    "......KIIK......",
    "......KIIK......",
    "......KIIK......",
    ".......KK......."
}};

// Chicote de Espinhos
inline const Sprite ARTE_CHICOTE_DE_ESPINHOS = {{
    "................",
    "................",
    "............K...",
    "...........KlK..",
    ".........KKGGK..",
    "........KlGKKGK.",
    "........KGK.KGlK",
    "......KKGK..KGK.",
    ".....KlGK..KGK..",
    ".....KGK..KGlK..",
    "....KGK..KlGK...",
    "...KIK....KKGK..",
    "..KIK.......K...",
    ".KIK............",
    "..K.............",
    "................"
}};

// Adaga Venenosa
inline const Sprite ARTE_ADAGA_VENENOSA = {{
    "................",
    ".............K..",
    "............KvK.",
    "...........KvVK.",
    "......K...KvVK..",
    ".....KMK.KvVK...",
    "......KMKvVK....",
    ".......KMKK.....",
    "......KIKMK.....",
    ".....KIK.KMK....",
    "....KMK...K.....",
    ".....K..........",
    "................",
    "................",
    "................",
    "................"
}};

// Martelo de Pedra
inline const Sprite ARTE_MARTELO_DE_PEDRA = {{
    ".........KKKK...",
    "........KHHHHK..",
    ".......KHCCCCHK.",
    ".......KHCHHHHK.",
    ".......KHHHZHHK.",
    ".......KHHHHHHK.",
    "........KHHHHK..",
    ".......KIKKKK...",
    "......KIK.......",
    ".....KIK........",
    "....KIK.........",
    "...KIK..........",
    "..KIK...........",
    ".KIK............",
    "..K.............",
    "................"
}};

// Escudo de Madeira
inline const Sprite ARTE_ESCUDO_DE_MADEIRA = {{
    "................",
    ".....KKKKKK.....",
    "....KNNNNNNK....",
    "...KNZIIIIZNK...",
    "..KNIZIIIIZINK..",
    ".KNIIZIIIIZIINK.",
    ".KNIIZIIIIZIINK.",
    ".KNIIZIMMIZIINK.",
    ".KNIIZIMMIZIINK.",
    ".KNIIZIIIIZIINK.",
    ".KNIIZIIIIZIINK.",
    "..KNIZIIIIZINK..",
    "...KNZIIIIZNK...",
    "....KNNNNNNK....",
    ".....KKKKKK.....",
    "................"
}};

// Escudo de Ferro
inline const Sprite ARTE_ESCUDO_DE_FERRO = {{
    "................",
    "..KKKKKKKKKKKK..",
    ".KLLLLLLNNNNNNK.",
    ".KLLLLLAANNNNNK.",
    ".KLLLLLAANNNNNK.",
    ".KLLAAAAAAAANNK.",
    ".KLLLLLAANNNNNK.",
    "..KLLLLAANNNNK..",
    "..KLLLLAANNNNK..",
    "...KLLLAANNNK...",
    "....KLLAANNK....",
    ".....KLAANK.....",
    "......KLNK......",
    ".......KK.......",
    "................",
    "................"
}};

// Escudo de Torre
inline const Sprite ARTE_ESCUDO_DE_TORRE = {{
    "....KKKKKKKK....",
    "...KNLNNNNNNK...",
    "...KNLWNNNWNK...",
    "...KNLNNNNNNK...",
    "...KBBBBBBBBK...",
    "...KNLNNNNNNK...",
    "...KNLNNNNNNK...",
    "...KNLNNNNNNK...",
    "...KNLNNNNNNK...",
    "...KNLNNNNNNK...",
    "...KNLNNNNNNK...",
    "...KBBBBBBBBK...",
    "...KNLNNNNNNK...",
    "...KNLWNNNWNK...",
    "...KNLNNNNNNK...",
    "....KKKKKKKK...."
}};

// Cristal de Vida
inline const Sprite ARTE_CRISTAL_DE_VIDA = {{
    "................",
    "......KKKK......",
    ".....KrQrrK.....",
    "....KrQrrrrK....",
    "...KrQQrrrrrK...",
    "..KrQQrrrrWrrK..",
    "..KrrQrrrrrrrK..",
    "...KrrrrrrrrK...",
    "...KrrrrrQrrK...",
    "....KrrrrrrK....",
    "....KrrrrrrK....",
    ".....KrrrrK.....",
    "......KrrK......",
    "......KrrK......",
    ".......KK.......",
    "................"
}};

// Cristal de Força
inline const Sprite ARTE_CRISTAL_DE_FORCA = {{
    "................",
    "......KKKK......",
    ".....KXYXXK.....",
    "....KXYXXXXK....",
    "...KXYYXXXXXK...",
    "..KXYYXXXXWXXK..",
    "..KXXYXXXXXXXK..",
    "...KXXXXXXXXK...",
    "...KXXXXXYXXK...",
    "....KXXXXXXK....",
    "....KXXXXXXK....",
    ".....KXXXXK.....",
    "......KXXK......",
    "......KXXK......",
    ".......KK.......",
    "................"
}};

// Cristal de Ferro
inline const Sprite ARTE_CRISTAL_DE_FERRO = {{
    "................",
    "......KKKK......",
    ".....KNLNNK.....",
    "....KNLNNNNK....",
    "...KNLLNNNNNK...",
    "..KNLLNNNNWNNK..",
    "..KNNLNNNNNNNK..",
    "...KNNNNNNNNK...",
    "...KNNNNNLNNK...",
    "....KNNNNNNK....",
    "....KNNNNNNK....",
    ".....KNNNNK.....",
    "......KNNK......",
    "......KNNK......",
    ".......KK.......",
    "................"
}};

// Pena do Vento
inline const Sprite ARTE_PENA_DO_VENTO = {{
    "................",
    "............K...",
    "...........KWK..",
    ".........KKWK...",
    "........KTTK....",
    ".......KKTWKK...",
    "......KTTWKKTK..",
    ".....KKTWKKTK...",
    "....KTTWKKTTK...",
    "...KKTWKKTKK....",
    "..KTTWKKTTK.....",
    "..KTKWKTKK......",
    "...KWKTTK.......",
    "..KWKTKK........",
    ".KIK.K..........",
    "..K............."
}};

// Pergaminho de Fogo
inline const Sprite ARTE_PERGAMINHO_DE_FOGO = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFXFFHFCK.",
    ".KCFFFFXXXFFFCK.",
    ".KCKKKKXXXKKKCK.",
    "..K....KXK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Água
inline const Sprite ARTE_PERGAMINHO_DE_AGUA = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFUFFHFCK.",
    ".KCFFFFUUUFFFCK.",
    ".KCKKKKUUUKKKCK.",
    "..K....KUK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Planta
inline const Sprite ARTE_PERGAMINHO_DE_PLANTA = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFGFFHFCK.",
    ".KCFFFFGGGFFFCK.",
    ".KCKKKKGGGKKKCK.",
    "..K....KGK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Veneno
inline const Sprite ARTE_PERGAMINHO_DE_VENENO = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFvFFHFCK.",
    ".KCFFFFvvvFFFCK.",
    ".KCKKKKvvvKKKCK.",
    "..K....KvK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Pedra
inline const Sprite ARTE_PERGAMINHO_DE_PEDRA = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFHFFHFCK.",
    ".KCFFFFHHHFFFCK.",
    ".KCKKKKHHHKKKCK.",
    "..K....KHK...K..",
    "........K.......",
    "................",
    "................"
}};

// Chave
inline const Sprite ARTE_CHAVE = {{
    "................",
    "...KKKK.........",
    "..KMMMMK........",
    ".KMYKKKMK.......",
    ".KMK..KMK.......",
    ".KMK..KMK.......",
    ".KMKKKMMK.......",
    "..KMMMMYK.......",
    "...KKKKKMK......",
    "........KMK.....",
    ".........KMK....",
    ".........KKMK...",
    "........KMMKMK..",
    ".........KK.KMK.",
    "...........KMMK.",
    "............KK.."
}};

// Mochila de Couro
inline const Sprite ARTE_MOCHILA_DE_COURO = {{
    "......KKKK......",
    ".....KIIIIK.....",
    "....KIKKKKIK....",
    "...KZCCCCCCZK...",
    "..KZCCCCCCCCZK..",
    "..KZCIIIIIICZK..",
    "..KZCIMMMMICZK..",
    "..KZCIIMMIICZK..",
    ".KZZCCCCCCCCZZK.",
    ".KZCCCCCCCCCCZK.",
    ".KZCCZCCCCZCCZK.",
    ".KZCCZCCCCZCCZK.",
    ".KZCCCCCCCCCCZK.",
    "..KZZZZZZZZZZK..",
    "...KKKKKKKKKK...",
    "................"
}};

// Lanterna
inline const Sprite ARTE_LANTERNA = {{
    "......KKKK......",
    ".....KNNNNK.....",
    "....KNKKKKNK....",
    ".....KNNNNK.....",
    "....KLLLLLLK....",
    "...KNYYYYYYNK...",
    "...KNYWWWWYNK...",
    "...KNYWXXWYNK...",
    "...KNYWXXWYNK...",
    "...KNYYXXYYNK...",
    "...KNYYYYYYNK...",
    "....KLLLLLLK....",
    "...KNNNNNNNNK...",
    "....KKKKKKKK....",
    "................",
    "................"
}};

// Bomba de Fumaça
inline const Sprite ARTE_BOMBA_DE_FUMACA = {{
    ".........KLK..K.",
    "........KLLLKKLK",
    ".........KXK..K.",
    "........KXYK....",
    ".......KIKK.....",
    "......KIK.......",
    "....KKNNKK......",
    "...KBBBBBBK.....",
    "..KBBBBBBBBK....",
    ".KBBLBBBBBBBK...",
    ".KBLLBBBBBBBK...",
    ".KBBBBBBBBBBK...",
    ".KBBBBBBBBBBK...",
    "..KBBBBBBBBK....",
    "...KBBBBBBK.....",
    "....KKKKKK......"
}};

// Pergaminho do Mapa
inline const Sprite ARTE_PERGAMINHO_DO_MAPA = {{
    "................",
    "..KKKKKKKKKKKK..",
    ".KIFFFFFFFFFFIK.",
    ".KIFFFFFFFFFFIK.",
    "..KFHFFFFFFFFK..",
    "..KFHHHFFFHFFK..",
    "..KFFFHFFFHFFK..",
    "..KFFFHHHHHFFK..",
    "..KFFFFFFFHFFK..",
    "..KFFrFFFFHFFK..",
    "..KFrrrFFFHHFK..",
    "..KFFrFFFFFFFK..",
    ".KIFFFFFFFFFFIK.",
    ".KIFFFFFFFFFFIK.",
    "..KKKKKKKKKKKK..",
    "................"
}};

// Força
inline const Sprite ARTE_ICONE_FORCA = {{
    "................",
    "................",
    ".........KKKK...",
    "........KSSSSK..",
    ".......KSCSSSSK.",
    ".......KSSCCSSK.",
    "........KSSSSK..",
    "...KKKK.KSSSSK..",
    "..KSSSSKKSSSSK..",
    ".KSSWWSSKSSSSK..",
    "KSSWSSSSSSSSSK..",
    "KSSSSSSSSSSSSK..",
    "KSSSSSSSSSSSSK..",
    "KCCCCCCCCCCCCK..",
    ".KKKKKKKKKKKK...",
    "................"
}};

// Constituição
inline const Sprite ARTE_ICONE_CONSTITUICAO = {{
    "................",
    "..KKKKKKKKKKKK..",
    ".KGGGGGGDDDDDDK.",
    ".KGGGGGGDDDDDDK.",
    ".KGGGrrGDrrDDDK.",
    ".KGGrWrrrrrrDDK.",
    ".KGGrrrrrrrrDDK.",
    "..KGGrrrrrrDDK..",
    "..KGGGrrrrDDDK..",
    "...KGGGrrDDDK...",
    "....KGGGDDDK....",
    ".....KGGDDK.....",
    "......KGDK......",
    ".......KK.......",
    "................",
    "................"
}};

// Destreza
inline const Sprite ARTE_ICONE_DESTREZA = {{
    "................",
    ".....KKKKKK.....",
    "...KKrrrrrrKK...",
    "..KrrrrWWrrrrK..",
    "..KrrWWWWWWrrK..",
    ".KrrWWWrrWWWrrK.",
    ".KrrWWrrrrWWrrK.",
    ".KrWWrrYYrrWWrK.",
    ".KrWWrrYYrrWWrK.",
    ".KrrWWrrrrWWrrK.",
    ".KrrWWWrrWWWrrK.",
    "..KrrWWWWWWrrK..",
    "..KrrrrWWrrrrK..",
    "...KKrrrrrrKK...",
    ".....KKKKKK.....",
    "................"
}};

// Agilidade
inline const Sprite ARTE_ICONE_AGILIDADE = {{
    ".KKK............",
    "KWWWK...........",
    "WUWWWKKKKK......",
    "WWUWWKCCCCK.....",
    "WUWUWKIIIIK.....",
    "KWUWWKIIIIK.....",
    ".KWUWKIIIIK.....",
    "..KWWKIIIIK.....",
    "...KKKIIIIK.....",
    ".....KIIIIKKKK..",
    ".....KIIIIIIIIK.",
    ".....KIIIIIIIIIK",
    ".....KIIIIIIIIIK",
    ".....KZZZZZZZZZK",
    "......KKKKKKKKK.",
    "................"
}};

// Ponto de atributo livre
inline const Sprite ARTE_ICONE_PONTO_LIVRE = {{
    "................",
    "................",
    "......KKKK......",
    ".....KlGGGK.....",
    ".....KlGGGK.....",
    "...KKKlGGGKKK...",
    "..KllllGGGGGGK..",
    "..KGGGGGGGGGGK..",
    "..KGGGGGGGGGGK..",
    "..KGGGGGGGGGGK..",
    "...KKKGGGGKKK...",
    ".....KGGGGK.....",
    ".....KGGGGK.....",
    "......KKKK......",
    "................",
    "................"
}};

// Moeda
inline const Sprite ARTE_ICONE_MOEDA = {{
    "................",
    ".....KKKKKK.....",
    "...KKCCCCCCKK...",
    "..KCCMYMMMMCCK..",
    "..KCMYCCCCMMCK..",
    ".KCMYCCMMCCMMCK.",
    ".KCMYCMMMMCCMCK.",
    ".KCMCMMMMMMCMCK.",
    ".KCMCMMMMMMCMCK.",
    ".KCMCCMMMMCCMCK.",
    ".KCMMCCMMCCMMCK.",
    "..KCMMCCCCMMCK..",
    "..KCCMMMMMMCCK..",
    "...KKCCCCCCKK...",
    ".....KKKKKK.....",
    "................"
}};

// Pontos (troféu)
inline const Sprite ARTE_ICONE_PONTOS = {{
    "................",
    "...KKKKKKKKKK...",
    ".KKMMMMMMMMMMKK.",
    "KMMMMYYMMMMMMMMK",
    "KMKKMYMMMMMMKKMK",
    "KMKKMMMMMMMMKKMK",
    ".KMKKMMMMMMKKMK.",
    "..K.KMMMMMMK.K..",
    ".....KKMMKK.....",
    "......KMMK......",
    "....KKKMMKKK....",
    "...KCCCCCCCCK...",
    "...KCCCCCCCCK...",
    "....KCCCCCCK....",
    ".....KKKKKK.....",
    "................"
}};

// Experiência (estrela)
inline const Sprite ARTE_ICONE_XP = {{
    ".......K........",
    "......KUK.......",
    "......KUK.......",
    "......KWK.......",
    ".....KUWUK......",
    "..KKKKWUUKKKK...",
    ".KUUUUUUUUUUUK..",
    "..KUUWWUUUUUK...",
    "...KUUUUUUUK....",
    "...KUUUUUUUK....",
    "...KUUUUUUUK....",
    "...KUUUKUUUK....",
    "...KUKK.KKUK....",
    "..KUK.....KUK...",
    "...K.......K....",
    "................"
}};

// Na mesma ordem do enum IdItem (itens.h).

// ---- v1.23: armas dos elementos novos ----
// Lança de Gelo
inline const Sprite ARTE_LANCA_DE_GELO = {{
    "............KKK.",
    "...........KWUK.",
    "..........KWUUK.",
    ".........KWUUAK.",
    "........KUUUAK..",
    ".......KUKAAK...",
    "......KUKKKK....",
    ".....KNK........",
    "....KNK.........",
    "...KIK..........",
    "..KIK...........",
    ".KIK............",
    "KIK.............",
    "KK..............",
    "................",
    "................"
}};

// Martelo do Trovão
inline const Sprite ARTE_MARTELO_DO_TROVAO = {{
    "........KKKKKK..",
    ".......KLLLLLLK.",
    "......KLLLYLLLNK",
    "......KLLYYLLLNK",
    "......KLYYYYLLNK",
    "......KLLLYYLLNK",
    "......KLLYYLLLNK",
    ".......KNNNNNNK.",
    "......KZKKKKKK..",
    ".....KZK........",
    "....KMK.........",
    "...KZK..........",
    "..KZK...........",
    ".KZK............",
    "..K.............",
    "................"
}};

// Foice das Trevas
inline const Sprite ARTE_FOICE_DAS_TREVAS = {{
    "...KKKKKK.......",
    ".KKVVVVPPKK.....",
    "KVVPPPPPPPPKK...",
    "KPPKKKKKKPPZK...",
    ".KK......KKZK...",
    "...........KZK..",
    "...........KZK..",
    "..........KZK...",
    "..........KZK...",
    ".........KZK....",
    ".........KZK....",
    "........KZK.....",
    "........KZK.....",
    ".......KMK......",
    ".......KZK......",
    "........K......."
}};

// Cetro de Luz
inline const Sprite ARTE_CETRO_DE_LUZ = {{
    "..........KKKK..",
    ".........KYWWYK.",
    "........KYWWWWYK",
    "........KYWWWWYK",
    ".........KYWWYK.",
    "........KMKKKK..",
    ".......KMMK.....",
    "......KMMK......",
    ".....KIIK.......",
    "....KIIK........",
    "...KMMK.........",
    "..KIIK..........",
    ".KIIK...........",
    ".KKK............",
    "................",
    "................"
}};

// Pergaminho de Normal
inline const Sprite ARTE_PERGAMINHO_DE_NORMAL = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFWFFHFCK.",
    ".KCFFFFWWWFFFCK.",
    ".KCKKKKWWWKKKCK.",
    "..K....KWK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Gelo
inline const Sprite ARTE_PERGAMINHO_DE_GELO = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFUFFHFCK.",
    ".KCFFFFUUUFFFCK.",
    ".KCKKKKUUUKKKCK.",
    "..K....KUK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Raio
inline const Sprite ARTE_PERGAMINHO_DE_RAIO = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFYFFHFCK.",
    ".KCFFFFYYYFFFCK.",
    ".KCKKKKYYYKKKCK.",
    "..K....KYK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Trevas
inline const Sprite ARTE_PERGAMINHO_DE_TREVAS = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFPFFHFCK.",
    ".KCFFFFPPPFFFCK.",
    ".KCKKKKPPPKKKCK.",
    "..K....KPK...K..",
    "........K.......",
    "................",
    "................"
}};

// Pergaminho de Luz
inline const Sprite ARTE_PERGAMINHO_DE_LUZ = {{
    "................",
    "................",
    "................",
    "..K..........K..",
    ".KCKKKKKKKKKKCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFHFHFHFHFFCK.",
    ".KCFFFFFFFFFFCK.",
    ".KCFFHFFMFFHFCK.",
    ".KCFFFFMMMFFFCK.",
    ".KCKKKKMMMKKKCK.",
    "..K....KMK...K..",
    "........K.......",
    "................",
    "................"
}};

inline const Sprite* const ARTE_DOS_ITENS[] = {
    &ARTE_POCAO_DE_VIDA,
    &ARTE_POCAO_GRANDE,
    &ARTE_ANTIDOTO,
    &ARTE_ETER,
    &ARTE_ADAGA,
    &ARTE_ESPADA_CURTA,
    &ARTE_MACHADO,
    &ARTE_ESPADA_LONGA,
    &ARTE_ESPADA_DE_FOGO,
    &ARTE_TRIDENTE,
    &ARTE_CHICOTE_DE_ESPINHOS,
    &ARTE_ADAGA_VENENOSA,
    &ARTE_MARTELO_DE_PEDRA,
    &ARTE_ESCUDO_DE_MADEIRA,
    &ARTE_ESCUDO_DE_FERRO,
    &ARTE_ESCUDO_DE_TORRE,
    &ARTE_CRISTAL_DE_VIDA,
    &ARTE_CRISTAL_DE_FORCA,
    &ARTE_CRISTAL_DE_FERRO,
    &ARTE_PENA_DO_VENTO,
    &ARTE_PERGAMINHO_DE_FOGO,
    &ARTE_PERGAMINHO_DE_AGUA,
    &ARTE_PERGAMINHO_DE_PLANTA,
    &ARTE_PERGAMINHO_DE_VENENO,
    &ARTE_PERGAMINHO_DE_PEDRA,
    &ARTE_CHAVE,
    &ARTE_MOCHILA_DE_COURO,
    &ARTE_LANTERNA,
    &ARTE_BOMBA_DE_FUMACA,
    &ARTE_PERGAMINHO_DO_MAPA,
    &ARTE_LANCA_DE_GELO,
    &ARTE_MARTELO_DO_TROVAO,
    &ARTE_FOICE_DAS_TREVAS,
    &ARTE_CETRO_DE_LUZ,
    &ARTE_PERGAMINHO_DE_NORMAL,
    &ARTE_PERGAMINHO_DE_GELO,
    &ARTE_PERGAMINHO_DE_RAIO,
    &ARTE_PERGAMINHO_DE_TREVAS,
    &ARTE_PERGAMINHO_DE_LUZ
};
static_assert(sizeof(ARTE_DOS_ITENS) / sizeof(ARTE_DOS_ITENS[0]) == NUM_TIPOS_DE_ITEM, "um desenho para cada item");
// Na mesma ordem do enum Atributo (jogo.h).
inline const Sprite* const ARTE_DOS_ATRIBUTOS[] = { &ARTE_ICONE_FORCA, &ARTE_ICONE_CONSTITUICAO, &ARTE_ICONE_DESTREZA, &ARTE_ICONE_AGILIDADE };
