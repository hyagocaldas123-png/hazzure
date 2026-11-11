#pragma once
// IDs verified against the installed items_game paint_kits and glove model families.
namespace HazzureGloves {
struct GlovePaint {
    int definition;
    int paint;
    const char* name;
};

inline constexpr GlovePaint kCatalog[] = {
    // Bloodhound Gloves (5027)
    {5027, 10006, "Bloodhound Gloves | Charred"},
    {5027, 10007, "Bloodhound Gloves | Snakebite"},
    {5027, 10008, "Bloodhound Gloves | Bronzed"},
    {5027, 10039, "Bloodhound Gloves | Guerrilla"},
    // Sport Gloves (5030)
    {5030, 10018, "Sport Gloves | Superconductor"},
    {5030, 10019, "Sport Gloves | Arid"},
    {5030, 10037, "Sport Gloves | Pandora's Box"},
    {5030, 10038, "Sport Gloves | Hedge Maze"},
    {5030, 10045, "Sport Gloves | Amphibious"},
    {5030, 10046, "Sport Gloves | Bronze Morph"},
    {5030, 10047, "Sport Gloves | Omega"},
    {5030, 10048, "Sport Gloves | Vice"},
    {5030, 10073, "Sport Gloves | Slingshot"},
    {5030, 10074, "Sport Gloves | Big Game"},
    {5030, 10075, "Sport Gloves | Scarlet Shamagh"},
    {5030, 10076, "Sport Gloves | Nocts"},
    // Driver Gloves (5031)
    {5031, 10013, "Driver Gloves | Lunar Weave"},
    {5031, 10015, "Driver Gloves | Convoy"},
    {5031, 10016, "Driver Gloves | Crimson Weave"},
    {5031, 10040, "Driver Gloves | Diamondback"},
    {5031, 10041, "Driver Gloves | King Snake"},
    {5031, 10042, "Driver Gloves | Imperial Plaid"},
    {5031, 10043, "Driver Gloves | Overtake"},
    {5031, 10044, "Driver Gloves | Racing Green"},
    {5031, 10069, "Driver Gloves | Rezan the Red"},
    {5031, 10070, "Driver Gloves | Snow Leopard"},
    {5031, 10071, "Driver Gloves | Queen Jaguar"},
    {5031, 10072, "Driver Gloves | Black Tie"},
    // Hand Wraps (5032)
    {5032, 10009, "Hand Wraps | Leather"},
    {5032, 10010, "Hand Wraps | Spruce DDPAT"},
    {5032, 10021, "Hand Wraps | Slaughter"},
    {5032, 10036, "Hand Wraps | Badlands"},
    {5032, 10053, "Hand Wraps | Cobalt Skulls"},
    {5032, 10054, "Hand Wraps | Overprint"},
    {5032, 10055, "Hand Wraps | Duct Tape"},
    {5032, 10056, "Hand Wraps | Arboreal"},
    {5032, 10081, "Hand Wraps | Desert Shamagh"},
    {5032, 10082, "Hand Wraps | Giraffe"},
    {5032, 10083, "Hand Wraps | Constrictor"},
    {5032, 10084, "Hand Wraps | CAUTION!"},
    // Moto Gloves (5033)
    {5033, 10024, "Moto Gloves | Eclipse"},
    {5033, 10026, "Moto Gloves | Spearmint"},
    {5033, 10027, "Moto Gloves | Boom!"},
    {5033, 10028, "Moto Gloves | Cool Mint"},
    {5033, 10049, "Moto Gloves | POW!"},
    {5033, 10050, "Moto Gloves | Turtle"},
    {5033, 10051, "Moto Gloves | Transport"},
    {5033, 10052, "Moto Gloves | Polygon"},
    {5033, 10077, "Moto Gloves | Finish Line"},
    {5033, 10078, "Moto Gloves | Smoke Out"},
    {5033, 10079, "Moto Gloves | Blood Pressure"},
    {5033, 10080, "Moto Gloves | 3rd Commando Company"},
    // Specialist Gloves (5034)
    {5034, 10030, "Specialist Gloves | Forest DDPAT"},
    {5034, 10033, "Specialist Gloves | Crimson Kimono"},
    {5034, 10034, "Specialist Gloves | Emerald Web"},
    {5034, 10035, "Specialist Gloves | Foundation"},
    {5034, 10061, "Specialist Gloves | Crimson Web"},
    {5034, 10062, "Specialist Gloves | Buckshot"},
    {5034, 10063, "Specialist Gloves | Fade"},
    {5034, 10064, "Specialist Gloves | Mogul"},
    {5034, 10065, "Specialist Gloves | Marble Fade"},
    {5034, 10066, "Specialist Gloves | Lt. Commander"},
    {5034, 10067, "Specialist Gloves | Tiger Strike"},
    {5034, 10068, "Specialist Gloves | Field Agent"},
    // Hydra Gloves (5035)
    {5035, 10057, "Hydra Gloves | Emerald"},
    {5035, 10058, "Hydra Gloves | Mangrove"},
    {5035, 10059, "Hydra Gloves | Rattler"},
    {5035, 10060, "Hydra Gloves | Case Hardened"},
    // Broken Fang Gloves (4725)
    {4725, 10085, "Broken Fang Gloves | Jade"},
    {4725, 10086, "Broken Fang Gloves | Yellow-banded"},
    {4725, 10087, "Broken Fang Gloves | Needle Point"},
    {4725, 10088, "Broken Fang Gloves | Unhinged"},
};

inline int DefaultPaint(int definition){
    switch(definition){case 5027:return 10006;case 5030:return 10018;case 5031:return 10013;
        case 5032:return 10009;case 5033:return 10024;case 5034:return 10033;
        case 5035:return 10057;case 4725:return 10085;default:return 0;}
}

inline bool ValidPaint(int definition, int paint){
    if(paint == 0) return definition == 5028 || definition == 5029;
    for(const auto& entry : kCatalog){
        if(entry.definition == definition && entry.paint == paint) return true;
    }
    return false;
}

inline const char* PaintName(int definition, int paint){
    for(const auto& entry : kCatalog){
        if(entry.definition == definition && entry.paint == paint) return entry.name;
    }
    return nullptr;
}

inline const char* FamilyName(int definition){
    switch(definition){
        case 4725: return "Broken Fang Gloves";
        case 5027: return "Bloodhound Gloves";
        case 5028: return "Default T Gloves";
        case 5029: return "Default CT Gloves";
        case 5030: return "Sport Gloves";
        case 5031: return "Driver Gloves";
        case 5032: return "Hand Wraps";
        case 5033: return "Moto Gloves";
        case 5034: return "Specialist Gloves";
        case 5035: return "Hydra Gloves";
        default: return "Luvas";
    }
}
}

namespace HazzureKnifeFinishes {
struct KnifePaint {
    int definition; // 0 = universal across all knives, or specific knife definition ID
    int paint;
    const char* name;
    float defaultWear;
};

// Universal and iconic CS2 knife finishes verified in items_game paint_kits
inline constexpr KnifePaint kCatalog[] = {
    {0,   0,   "Vanilla (Padrao)", 0.01f},
    {0,   38,  "Fade", 0.01f},
    {0,   413, "Marble Fade", 0.01f},
    {0,   415, "Doppler (Ruby)", 0.01f},
    {0,   416, "Doppler (Sapphire)", 0.01f},
    {0,   417, "Doppler (Black Pearl)", 0.01f},
    {0,   418, "Doppler (Phase 1)", 0.01f},
    {0,   419, "Doppler (Phase 2)", 0.01f},
    {0,   420, "Doppler (Phase 3)", 0.01f},
    {0,   421, "Doppler (Phase 4)", 0.01f},
    {0,   568, "Gamma Doppler (Emerald)", 0.01f},
    {0,   569, "Gamma Doppler (Phase 1)", 0.01f},
    {0,   570, "Gamma Doppler (Phase 2)", 0.01f},
    {0,   571, "Gamma Doppler (Phase 3)", 0.01f},
    {0,   572, "Gamma Doppler (Phase 4)", 0.01f},
    {0,   409, "Tiger Tooth", 0.01f},
    {0,   59,  "Slaughter", 0.01f},
    {0,   12,  "Crimson Web", 0.06f},
    {0,   44,  "Case Hardened", 0.15f},
    {0,   42,  "Blue Steel", 0.15f},
    {0,   410, "Damascus Steel", 0.05f},
    {0,   98,  "Ultraviolet", 0.07f},
    {0,   40,  "Night", 0.07f},
    {0,   43,  "Stained", 0.15f},
    {0,   578, "Bright Water", 0.05f},
    {0,   580, "Freehand", 0.05f},
    {0,   414, "Rust Coat", 0.65f},
    {0,   175, "Scorched", 0.15f},
    {0,   5,   "Forest DDPAT", 0.15f},
    {0,   77,  "Boreal Forest", 0.15f},
    {0,   72,  "Safari Mesh", 0.15f},
    {0,   143, "Urban Masked", 0.15f},
    {500, 558, "Lore (Bayonet)", 0.05f},
    {505, 559, "Lore (Flip)", 0.05f},
    {506, 560, "Lore (Gut)", 0.05f},
    {507, 561, "Lore (Karambit)", 0.05f},
    {508, 562, "Lore (M9 Bayonet)", 0.05f},
    {515, 1105,"Lore (Butterfly)", 0.05f},
    {500, 573, "Autotronic (Bayonet)", 0.05f},
    {505, 574, "Autotronic (Flip)", 0.05f},
    {506, 575, "Autotronic (Gut)", 0.05f},
    {507, 576, "Autotronic (Karambit)", 0.05f},
    {508, 577, "Autotronic (M9 Bayonet)", 0.05f},
    {515, 1115,"Autotronic (Butterfly)", 0.05f},
    {500, 563, "Black Laminate (Bayonet)", 0.10f},
    {507, 566, "Black Laminate (Karambit)", 0.10f},
    {508, 567, "Black Laminate (M9 Bayonet)", 0.10f},
};

inline bool ValidPaint(int definition, int paint){
    for(const auto& entry : kCatalog){
        if((entry.definition == 0 || entry.definition == definition) && entry.paint == paint) return true;
    }
    return false;
}

inline const char* PaintName(int definition, int paint){
    for(const auto& entry : kCatalog){
        if((entry.definition == 0 || entry.definition == definition) && entry.paint == paint) return entry.name;
    }
    return nullptr;
}
}
