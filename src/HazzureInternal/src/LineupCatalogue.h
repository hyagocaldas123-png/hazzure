// Coordinate references from the publisher's Mirage guide, retrieved 26/10/2026.
// Throw mode/force/timing were not supplied by the coordinate records; automation stays unvalidated.
#pragma once
#include "LineupLibrary.h"
namespace HazzureLineupCatalogue {
inline constexpr const char* source="https://cs2nades.gg/en/guides/best-mirage-nades-in-cs2";
inline int Merge(HazzureLineups::Library& library){
    struct Reference {const char* name;HazzureGame::Vec3 position,angles;};
    const Reference references[]={
        {"Referencia: janela / T",{1422.94f,-367.95f,-103.97f},{-32.27f,-174.24f,0}},
        {"Referencia: CT / rampa",{1087.041382f,-1017.001282f,-196.232117f},{-29.216162f,-143.676407f,0}},
        {"Referencia: escada / rampa",{908.409485f,-1036.031372f,-194.624115f},{-65.011642f,-159.897186f,0}},
        {"Referencia: janela cozinha / apps",{-160.031250f,887.968750f,-71.647980f},{-50.280293f,-146.488480f,0}}
    };
    int added=0;
    if(library.count<0||library.count>64)return 0;
    for(const auto& reference:references){bool exists=false;
        for(int i=0;i<library.count;++i)if(!strcmp(library.entries[i].map,"de_mirage")&&!strcmp(library.entries[i].name,reference.name))exists=true;
        if(exists||library.count==64)continue;HazzureLineups::Entry entry{};strcpy_s(entry.map,"de_mirage");strcpy_s(entry.name,reference.name);
        entry.position=reference.position;entry.angles=reference.angles;entry.definition=45;entry.validated=false;
        library.entries[library.count++]=entry;++added;
    }
    return added;
}
}
