// Catalogue helpers retained for item browsing. No legacy skin publication.
#pragma once
static int FirstPaintFor(int definition) {
    for(const auto& choice:kSkins)if(choice.def==definition)return choice.kit;
    return 0;
}

#include "ArmoryEditor.h"
static void DrawSkinSelections(Shared*) {
    ImGui::TextWrapped("Gerencie armas, facas e luvas na aba Inventory. Skinchanger removido.");
}
