#include "engine/character.h"

namespace engine {
namespace {

// Starter decks definition
constexpr StarterDeckCard DECK_IRONCLAD[] = {
    { "STRIKE_IRONCLAD", 5 },
    { "DEFEND_IRONCLAD", 4 },
    { "BASH", 1 },
};

constexpr StarterDeckCard DECK_SILENT[] = {
    { "STRIKE_SILENT", 5 },
    { "DEFEND_SILENT", 5 },
    { "NEUTRALIZE", 1 },
    { "SURVIVOR", 1 },
};

constexpr StarterDeckCard DECK_DEFECT[] = {
    { "STRIKE_DEFECT", 4 },
    { "DEFEND_DEFECT", 4 },
    { "ZAP", 1 },
    { "DUALCAST", 1 },
};

constexpr StarterDeckCard DECK_REGENT[] = {
    { "STRIKE_REGENT", 4 },
    { "DEFEND_REGENT", 4 },
    { "FALLING_STAR", 1 },
    { "VENERATE", 1 },
};

constexpr StarterDeckCard DECK_NECROBINDER[] = {
    { "STRIKE_NECROBINDER", 4 },
    { "DEFEND_NECROBINDER", 4 },
    { "UNLEASH", 1 },
    { "BODYGUARD", 1 },
};

constexpr CharacterDef ALL_CHARACTERS[] = {
    {
        CharacterId::IRONCLAD,
        "Soldat de Fer",
        "Ironclad",
        80, // HP
        99, // Gold
        3,  // Energy
        3,  // Potions
        "BURNING_BLOOD",
        "Force brute, Armure lourde et Sacrifice de PV",
        0x001F, // Crimson Red in BGR555
        std::span<const StarterDeckCard>(DECK_IRONCLAD)
    },
    {
        CharacterId::SILENT,
        "La Silencieuse",
        "Silent",
        70, // HP
        99, // Gold
        3,  // Energy
        3,  // Potions
        "RING_OF_THE_SNAKE",
        "Poison létal, Surins (Shivs) et Esquive",
        0x03E0, // Emerald Green in BGR555
        std::span<const StarterDeckCard>(DECK_SILENT)
    },
    {
        CharacterId::DEFECT,
        "Le Défectueux",
        "Defect",
        75, // HP
        99, // Gold
        3,  // Energy
        3,  // Potions
        "CRACKED_CORE",
        "Canalisation d'Orbes élémentaires et Algorithmes",
        0x7C00, // Cobalt Blue in BGR555
        std::span<const StarterDeckCard>(DECK_DEFECT)
    },
    {
        CharacterId::REGENT,
        "Le Régent",
        "Regent",
        75, // HP
        99, // Gold
        3,  // Energy
        3,  // Potions
        "DIVINE_RIGHT",
        "Commandement royal, Étoiles divines et Pouvoir absolu",
        0x03FF, // Gold / Amber in BGR555
        std::span<const StarterDeckCard>(DECK_REGENT)
    },
    {
        CharacterId::NECROBINDER,
        "La Nécrolieuse",
        "Necrobinder",
        65, // HP
        99, // Gold
        3,  // Energy
        3,  // Potions
        "BOUND_PHYLACTERY",
        "Nécromancie, Invocations d'âmes et Osselets",
        0x4010, // Dark Violet in BGR555
        std::span<const StarterDeckCard>(DECK_NECROBINDER)
    },
};

} // namespace

const CharacterDef& get_character(CharacterId id) {
    return ALL_CHARACTERS[static_cast<size_t>(id)];
}

std::span<const CharacterDef> get_all_characters() {
    return std::span<const CharacterDef>(ALL_CHARACTERS);
}

static constexpr PaletteDef PALETTES_IRONCLAD[] = {
    { PaletteVariant::CLASSIC, "Rouge Carmin (Classique)", "L'armure rouge sang traditionnelle du Soldat de Fer.", 0x001F, 0x000F },
    { PaletteVariant::SHINY,   "Flamme Azur (Chromatique)", "Une teinte cyan étincelante née du feu mystique.", 0x7C00, 0x5000 },
    { PaletteVariant::SHADOW,  "Obsidienne (Ombre)", "Une armure noir charbon forgée dans les abysses.", 0x1084, 0x0842 },
    { PaletteVariant::GOLDEN,  "Soleil Radieux (Or)", "Une parure dorée impériale ornée d'éclats solaires.", 0x03FF, 0x02AA }
};

static constexpr PaletteDef PALETTES_SILENT[] = {
    { PaletteVariant::CLASSIC, "Vert Émeraude (Classique)", "La cape d'assassin verte de la tribu du serpent.", 0x03E0, 0x0280 },
    { PaletteVariant::SHINY,   "Améthyste (Chromatique)", "Une tenue pourpre venimeuse des ombres sylvestres.", 0x7810, 0x540C },
    { PaletteVariant::SHADOW,  "Spectre Nocturne (Ombre)", "Un costume d'assassin invisible dans l'obscurité.", 0x14A5, 0x0C63 },
    { PaletteVariant::GOLDEN,  "Venin Doré (Or)", "La parure d'apparat des maîtres de la guilde silencieuse.", 0x03E8, 0x02E4 }
};

static constexpr PaletteDef PALETTES_DEFECT[] = {
    { PaletteVariant::CLASSIC, "Bleu Électrique (Classique)", "Le châssis automatique en cobalt forgé.", 0x7FE0, 0x5740 },
    { PaletteVariant::SHINY,   "Néon Cyberpunk (Chromatique)", "Une finition orange vif surchargée d'énergie.", 0x021F, 0x011F },
    { PaletteVariant::SHADOW,  "Néant Sidéral (Ombre)", "Un alliage noir mat dénué de tout reflet lumineux.", 0x1842, 0x0C21 },
    { PaletteVariant::GOLDEN,  "Cuivre Antique (Or)", "Une structure en laiton doré restaurée d'un âge perdu.", 0x02DF, 0x0218 }
};

static constexpr PaletteDef PALETTES_REGENT[] = {
    { PaletteVariant::CLASSIC, "Pourpre Royal (Classique)", "La pourpre souveraine réservée à la lignée des rois.", 0x510D, 0x3888 },
    { PaletteVariant::SHINY,   "Rubis Écarlate (Chromatique)", "Une robe cramoisie éclatante symbole de bravoure.", 0x001B, 0x0013 },
    { PaletteVariant::SHADOW,  "Éclipse Astrale (Ombre)", "Une cape d'ombre tissée de poussière d'étoiles éteintes.", 0x0C43, 0x0821 },
    { PaletteVariant::GOLDEN,  "Couronne Céleste (Or)", "Une splendeur dorée rayonnant de dignité céleste.", 0x03FF, 0x02DE }
};

static constexpr PaletteDef PALETTES_NECROBINDER[] = {
    { PaletteVariant::CLASSIC, "Teinte Spectrale (Classique)", "La bure sombre bordée d'un halo spectral vert d'eau.", 0x4104, 0x2482 },
    { PaletteVariant::SHINY,   "Âme Pourpre (Chromatique)", "Une aura violacée incandescente de magie interdite.", 0x001E, 0x0014 },
    { PaletteVariant::SHADOW,  "Abysse Mortuaire (Ombre)", "Le noir absolu des tombeaux millénaires de la Tour.", 0x0821, 0x0410 },
    { PaletteVariant::GOLDEN,  "Pharaonique (Or)", "Les bandelettes tressées de fils d'or pur des défunts rois.", 0x03B9, 0x0292 }
};

std::span<const PaletteDef> get_character_palettes(CharacterId id) {
    switch (id) {
        case CharacterId::SILENT:      return std::span<const PaletteDef>(PALETTES_SILENT);
        case CharacterId::DEFECT:      return std::span<const PaletteDef>(PALETTES_DEFECT);
        case CharacterId::REGENT:      return std::span<const PaletteDef>(PALETTES_REGENT);
        case CharacterId::NECROBINDER: return std::span<const PaletteDef>(PALETTES_NECROBINDER);
        default:                       return std::span<const PaletteDef>(PALETTES_IRONCLAD);
    }
}

} // namespace engine
