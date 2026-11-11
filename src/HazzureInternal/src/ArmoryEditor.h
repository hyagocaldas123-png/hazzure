// Included inside HazzureUI after SkinSelections helpers.
#pragma once
static int g_armoryPage{};
static void DrawInventoryAttributes(HazzureArmory::Profile& draft){
    ImGui::Separator();
    ImGui::TextColored(kAccent, "The Armory - Modulos do Item");
    const char* pages[]={"Adesivos (5 Slots)","Chaveiro (Keychain)","StatTrak","Nametag"};
    CleanCombo("Modulo Armory",&g_armoryPage,pages,4);

    if(g_armoryPage==0){
        CleanToggle("Cinco Adesivos Ativos",&draft.stickers);
        if(draft.stickers){
            static int index{};const char* slots[]={"Adesivo 1 (Posicao 1)","Adesivo 2 (Posicao 2)","Adesivo 3 (Posicao 3)","Adesivo 4 (Posicao 4)","Adesivo 5 (Posicao 5)"};
            CleanCombo("Slot",&index,slots,5);
            auto& s=draft.slots[index];

            // Popular stickers preset combo
            struct StickerPreset { const char* label; int id; };
            static const StickerPreset kPopularStickers[] = {
                {"Manual / Personalizado", 0},
                {"Titan (Holo) | Katowice 2014 (#1)", 1},
                {"iBUYPOWER (Holo) | Katowice 2014 (#2)", 2},
                {"Crown (Foil) (#9)", 9},
                {"Howling Dawn (#10)", 10},
                {"Flammable (Foil) (#12)", 12},
                {"Natus Vincere (Holo) | Katowice 2014 (#14)", 14},
                {"Team Dignitas (Holo) | Katowice 2014 (#16)", 16},
                {"Reason Gaming (Holo) | Katowice 2014 (#19)", 19},
                {"LGB eSports (Holo) | Katowice 2014 (#21)", 21},
                {"HellRaisers (Holo) | Katowice 2014 (#23)", 23},
                {"Cloud9 (Holo) | DreamHack 2014 (#175)", 175},
                {"Team Liquid (Foil) | MLG 2016 (#583)", 583}
            };
            static int popularStickerIdx = 0;
            const char* stickerPresetNames[IM_ARRAYSIZE(kPopularStickers)];
            for (size_t p = 0; p < IM_ARRAYSIZE(kPopularStickers); ++p) stickerPresetNames[p] = kPopularStickers[p].label;
            if (CleanCombo("Adesivos Famosos", &popularStickerIdx, stickerPresetNames, IM_ARRAYSIZE(kPopularStickers))) {
                if (popularStickerIdx > 0) s.id = kPopularStickers[popularStickerIdx].id;
            }

            ImGui::SetNextItemWidth(g_cleanCardContentW);
            ImGui::InputInt("ID do Adesivo",&s.id);
            const char* name=HazzureArmory::Lookup(HazzureArmory::stickersCatalog,s.id);
            ImGui::TextWrapped("%s",s.id==0?"Sem adesivo":(name?name:"ID fora do catalogo local"));

            // Fast copy buttons
            if (ImGui::Button("Copiar p/ Todos os 5 Slots", ImVec2(g_cleanCardContentW * 0.48f, 20.f))) {
                for (int k = 0; k < 5; ++k) draft.slots[k] = s;
            }
            ImGui::SameLine();
            if (ImGui::Button("Limpar Todos Adesivos", ImVec2(g_cleanCardContentW * 0.48f, 20.f))) {
                for (int k = 0; k < 5; ++k) draft.slots[k] = {};
            }

            CleanSliderFloat("Posicao X",&s.x,-10,10,"%.3f");
            CleanSliderFloat("Posicao Y",&s.y,-10,10,"%.3f");
            CleanSliderFloat("Rotacao",&s.rotation,-360,360,"%.1f deg");

            CleanSliderFloat("Raspagem (Wear)",&s.wear,0,1,"%.3f");
            const float wearBtnW = (g_cleanCardContentW - 16.f) / 5.f;
            if (ImGui::Button("0%##stk", ImVec2(wearBtnW, 18.f))) s.wear = 0.0f; ImGui::SameLine();
            if (ImGui::Button("25%##stk", ImVec2(wearBtnW, 18.f))) s.wear = 0.25f; ImGui::SameLine();
            if (ImGui::Button("50%##stk", ImVec2(wearBtnW, 18.f))) s.wear = 0.50f; ImGui::SameLine();
            if (ImGui::Button("75%##stk", ImVec2(wearBtnW, 18.f))) s.wear = 0.75f; ImGui::SameLine();
            if (ImGui::Button("99%##stk", ImVec2(wearBtnW, 18.f))) s.wear = 0.99f;

            CleanSliderFloat("Escala",&s.scale,.1f,5,"%.2f");
        }
    }
    if(g_armoryPage==1){
        CleanToggle("Chaveiro (Keychain)",&draft.charm);
        if(draft.charm){
            struct CharmPreset { const char* label; int id; };
            static const CharmPreset kPopularCharms[] = {
                {"Manual / Personalizado", 0},
                {"Hot Howl (#1)", 1},
                {"Diamond Dog (#2)", 2},
                {"Lil' Monster (#3)", 3},
                {"Baby Karat T (#4)", 4},
                {"Semi-Precious (#5)", 5},
                {"Diner Dog (#6)", 6},
                {"T-Mini (#7)", 7},
                {"Hot Wurst (#8)", 8}
            };
            static int popularCharmIdx = 0;
            const char* charmPresetNames[IM_ARRAYSIZE(kPopularCharms)];
            for (size_t p = 0; p < IM_ARRAYSIZE(kPopularCharms); ++p) charmPresetNames[p] = kPopularCharms[p].label;
            if (CleanCombo("Chaveiros Famosos", &popularCharmIdx, charmPresetNames, IM_ARRAYSIZE(kPopularCharms))) {
                if (popularCharmIdx > 0) draft.charmId = kPopularCharms[popularCharmIdx].id;
            }

            ImGui::SetNextItemWidth(g_cleanCardContentW);
            ImGui::InputInt("ID do Chaveiro",&draft.charmId);
            auto name=HazzureArmory::Lookup(HazzureArmory::charmsCatalog,draft.charmId);
            ImGui::TextWrapped("%s",draft.charmId==0?"Sem chaveiro":(name?name:"ID fora do catalogo local"));

            ImGui::SetNextItemWidth(g_cleanCardContentW);
            ImGui::InputInt("Seed do Chaveiro",&draft.charmSeed);
            if (ImGui::Button("Gerar Seed Aleatoria", ImVec2(g_cleanCardContentW, 20.f))) {
                draft.charmSeed = (rand() % 99999) + 1;
            }

            CleanSliderFloat("Chaveiro X",&draft.charmX,-20,20,"%.3f");
            CleanSliderFloat("Chaveiro Y",&draft.charmY,-20,20,"%.3f");
            CleanSliderFloat("Chaveiro Z",&draft.charmZ,-20,20,"%.3f");
        }
    }
    if(g_armoryPage==2){
        CleanToggle("StatTrak Ativo",&draft.stat);
        if(draft.stat){
            ImGui::SetNextItemWidth(g_cleanCardContentW);
            ImGui::InputInt("Contador de Abates",&draft.counter);
            const float cntBtnW = (g_cleanCardContentW - 12.f) / 4.f;
            if (ImGui::Button("+100", ImVec2(cntBtnW, 20.f))) draft.counter = static_cast<int>(std::min<int64_t>(INT_MAX,static_cast<int64_t>(draft.counter)+100)); ImGui::SameLine();
            if (ImGui::Button("+500", ImVec2(cntBtnW, 20.f))) draft.counter = static_cast<int>(std::min<int64_t>(INT_MAX,static_cast<int64_t>(draft.counter)+500)); ImGui::SameLine();
            if (ImGui::Button("1337", ImVec2(cntBtnW, 20.f))) draft.counter = 1337; ImGui::SameLine();
            if (ImGui::Button("Zerar", ImVec2(cntBtnW, 20.f))) draft.counter = 0;

            ImGui::TextDisabled("Contador salvo no item; incremento automatico indisponivel.");

        }
    }
    if(g_armoryPage==3){
        CleanToggle("Nametag Personalizada",&draft.nametag);
        if(draft.nametag){
            ImGui::SetNextItemWidth(g_cleanCardContentW);
            ImGui::InputText("Nome da Arma",draft.name,sizeof(draft.name));
            static const char* kNamePresets[] = { "Magic Stick", "CYBR-MAHI", "Point Click", "Head Hunter", "Aimbot.exe" };
            for (const char* np : kNamePresets) {
                if (ImGui::SmallButton(np)) {
                    strncpy_s(draft.name, sizeof(draft.name), np, _TRUNCATE);
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();
        }
    }
    ImGui::TextDisabled("Modulos salvos com o item da colecao.");
}
