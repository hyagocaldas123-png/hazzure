static void DrawCleanNavIcon(ImDrawList* d, ImVec2 c, int idx, ImU32 col) {
    const float r = 6.0f;
    if (idx == 0) { // Legitbot (Reticula)
        d->AddCircle(c, r * 0.85f, col, 16, 1.3f);
        d->AddLine(ImVec2(c.x - r * 1.2f, c.y), ImVec2(c.x + r * 1.2f, c.y), col, 1.2f);
        d->AddLine(ImVec2(c.x, c.y - r * 1.2f), ImVec2(c.x, c.y + r * 1.2f), col, 1.2f);
    } else if (idx == 1) { // Visuals (Olho)
        d->AddQuad(ImVec2(c.x - r, c.y), ImVec2(c.x, c.y - r * 0.7f), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r * 0.7f), col, 1.3f);
        d->AddCircleFilled(c, 1.8f, col);
    } else if (idx == 2) { // Misc (Setas / Onda de Movimento)
        d->AddLine(ImVec2(c.x - r, c.y + 3.f), ImVec2(c.x, c.y - 4.f), col, 1.4f);
        d->AddLine(ImVec2(c.x, c.y - 4.f), ImVec2(c.x + r, c.y + 3.f), col, 1.4f);
        d->AddCircleFilled(ImVec2(c.x, c.y + 1.f), 1.5f, col);
    } else if (idx == 3) { // Skins (Estrela / Faca)
        d->AddQuad(ImVec2(c.x, c.y - r), ImVec2(c.x + r * 0.85f, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r * 0.85f, c.y), col, 1.3f);
        d->AddCircleFilled(c, 1.6f, col);
    } else { // Configs (Sliders de Ajuste)
        d->AddLine(ImVec2(c.x - r, c.y - 3.f), ImVec2(c.x + r, c.y - 3.f), col, 1.3f);
        d->AddLine(ImVec2(c.x - r, c.y + 3.f), ImVec2(c.x + r, c.y + 3.f), col, 1.3f);
        d->AddCircleFilled(ImVec2(c.x - 2.f, c.y - 3.f), 2.2f, col);
        d->AddCircleFilled(ImVec2(c.x + 2.f, c.y + 3.f), 2.2f, col);
    }
}

// ==========================================================
// PAINEL PRINCIPAL ENIGMA CS2 (5 ABAS, 2 COLUNAS, OBSIDIANA)
// ==========================================================
#include "SkinSelections.h"
#include "LineupPanel.inl"

#include "UISettingsCards.inl"
#include "InventoryPanel.inl"

static void DrawAccuracyControls() { DrawCombatAccuracyCard(); }
static void DrawAdvancedMovementControls() { DrawMovementTechCard(); }
static void DrawShotStopControls() { DrawCombatBrakeCard(); }
static void DrawTriggerControls() { DrawCombatTriggerCard(); }
static void DrawCameraControls() { DrawEnvironmentCameraCard(); }

// 5 Abas Canonicas Enigma CS2: Legitbot, Visuals, Misc, Skins, Configs
static int g_panelSections[5]{};
static constexpr int kPanelSectionCounts[]={3,3,4,3,3};
static const char* const kPanelSections[][4]={
    {"Aimbot","Triggerbot","Recoil & Parada",nullptr},
    {"ESP Jogadores","Chams & Materiais","Mundo & Efeitos",nullptr},
    {"Movimento","Camera & QoL","Granadas & Lineups","Audio & Clima"},
    {"Inventory","Agentes","Colecao local",nullptr},
    {"Geral & Menu","Perfis","Diagnostico",nullptr}
};

struct PanelLayout { float width{}, height{}, sidebar{}; bool twoColumns{}; };
static PanelLayout ResolvePanelLayout(int width,int height){
    PanelLayout result{};
    result.width = std::max(280.f, std::min(980.f, static_cast<float>(width) - 16.f));
    result.height = std::max(210.f, std::min(660.f, static_cast<float>(height) - 16.f));
    if (result.width >= 820.f && result.height >= 560.f) {
        result.width = 820.f;
        result.height = 560.f;
    }
    result.sidebar = 180.f;
    result.twoColumns = (result.width - result.sidebar >= 480.f);
    return result;
}

static void DrawPanelSection(Shared* shared,int tab,int section,int column){
    auto card=[&](int id){switch(id){
        case 0:DrawSettingsCard0(shared);break;
        case 1:DrawSettingsCard1(shared);break;
        case 2:DrawSettingsCard2(shared);break;
        case 3:DrawSettingsCard3(shared);break;
        case 4:DrawSettingsCard4(shared);break;
        case 5:DrawSettingsCard5(shared);break;
        case 6:DrawSettingsCard6(shared);break;
        case 7:DrawSettingsCard7(shared);break;
        case 8:DrawSettingsCard8(shared);break;
        case 9:DrawSettingsCard9(shared);break;
        case 10:DrawSettingsCard10(shared);break;
        case 11:DrawSettingsCard11(shared);break;
        case 12:DrawSettingsCard12(shared);break;
        case 13:DrawSettingsCard13(shared);break;
        case 14:DrawSettingsCard14(shared);break;
        case 15:DrawSettingsCard15(shared);break;
        case 16:DrawSettingsCard16(shared);break;
        case 17:DrawSettingsCard17(shared);break;
        case 18:DrawSettingsCard18(shared);break;
        case 19:DrawSettingsCard19(shared);break;
        case 20:DrawSettingsCard20(shared);break;
    }};
    if(tab==0){ // Legitbot
        if(section==0){if(column==0){card(6);card(7);}else card(9);}
        else if(section==1){if(column==0)DrawTriggerControls();else DrawAccuracyControls();}
        else if(section==2){if(column==0)card(8);else DrawShotStopControls();}
    }else if(tab==1){ // Visuals
        if(section==0){if(column==0)card(0);else DrawESPSelectionPreview();}
        else if(section==1){if(column==0){card(3);card(5);}else card(4);}
        else if(section==2){if(column==0)card(1);else card(2);}
    }else if(tab==2){ // Misc
        if(section==0){if(column==0)card(10);else DrawAdvancedMovementControls();}
        else if(section==1){if(column==0){DrawCameraControls();
            ImGui::Checkbox("Ocultar mira personalizada ao usar scope",&g_cfg.hideCustomCrosshairScoped);
            ImGui::TextDisabled("A mascara nativa do scope permanece.");
        }else card(12);}
        else if(section==2){if(column==0)card(11);else DrawLineupControls(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll")));}
        else if(section==3){if(column==0)card(13);else {card(14);card(15);}}
    }else if(tab==3){ // Skins
        if(section==0){if(column==0)card(17);else DrawWeaponSelectionPreview(shared);}
        else if(section==1){if(column==0)card(16);else DrawAgentSelectionPreview(shared);}
        else if(section==2){if(column==0)DrawLocalInventory(shared);else DrawWeaponSelectionPreview(shared);}
    }else if(tab==4){ // Configs
        if(section==0){if(column==0)card(18);else card(20);}
        else if(section==1){if(column==0)card(19);}
        else if(section==2){if(column==0)card(20);}
    }
}

static void DrawPanel(Shared* shared,bool* pOpen,int screenW,int screenH){
    if(!pOpen||!*pOpen||screenW<280||screenH<210)return;
    g_currentTab=std::clamp(g_currentTab,0,4);
    auto layout=ResolvePanelLayout(screenW,screenH);

    ImGui::SetNextWindowSize({layout.width,layout.height},ImGuiCond_Always);
    ImGui::SetNextWindowPos({(screenW-layout.width)*.5f,(screenH-layout.height)*.5f},ImGuiCond_FirstUseEver);

    // Paleta de Estilo Enigma CS2
    ImGui::PushStyleColor(ImGuiCol_WindowBg, kWinBg);              // #101014
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border, kBorder);               // #2A2C38
    ImGui::PushStyleColor(ImGuiCol_Button, kWidgetBg);             // #13141B
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kBorderHover);    // #3F4357
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccent);         // #7A5AF8
    ImGui::PushStyleColor(ImGuiCol_Header, kCardBg);               // #1A1C24
    ImGui::PushStyleColor(ImGuiCol_FrameBg, kWidgetBg);            // #13141B
    ImGui::PushStyleColor(ImGuiCol_PopupBg, kWinBg);               // #101014

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,8.0f);        // 8px
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,6.0f);         // 6px
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,5.0f);         // 5px
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{10,8});

    auto flags=ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar;
    if(ImGui::Begin("##HazzureEnigmaStudio",pOpen,flags)){
        auto pos=ImGui::GetWindowPos();auto size=ImGui::GetWindowSize();
        ImGui::SetWindowPos({std::clamp(pos.x,0.f,std::max(0.f,screenW-size.x)),std::clamp(pos.y,0.f,std::max(0.f,screenH-size.y))});
        pos=ImGui::GetWindowPos();
        auto* draw=ImGui::GetWindowDrawList();
        RenderGlassBlurBackdrop(draw, pos, size);

        // 1. Barra Lateral Esquerda (Sidebar 180px) em #0C0D10 (kSideBg)
        draw->AddRectFilled(pos,{pos.x+layout.sidebar,pos.y+size.y},IM_COL32(12, 13, 16, 255),8.0f,ImDrawFlags_RoundCornersLeft);
        // Divisor vertical de 1px em #2A2C38 (kBorder)
        draw->AddLine({pos.x+layout.sidebar,pos.y},{pos.x+layout.sidebar,pos.y+size.y},IM_COL32(42, 44, 56, 255),1.0f);

        // Header da Sidebar
        ImGui::SetCursorPos({14,16});
        ImGui::BeginChild("##sidebarHeader",{layout.sidebar-28,48},false,ImGuiWindowFlags_NoScrollbar);
        ImGui::TextColored(kAccent,"HAZZURE");
        ImGui::SameLine();
        ImGui::TextColored(kTextMuted,"CS2");
        ImGui::TextColored(kTextMuted,"en1gma edition");
        ImGui::EndChild();

        // 2. Botoes de Navegacao (5 Abas Canonicas) com Indicador Lateral de 3.5px #7A5AF8
        ImGui::SetCursorPos({10,68});
        ImGui::BeginChild("##navigation",{layout.sidebar-20,size.y-96},false,ImGuiWindowFlags_NoScrollbar);
        const char* tabs[]={"Legitbot","Visuals","Misc","Skins","Configs"};
        auto* navigationDraw=ImGui::GetWindowDrawList();
        for(int i=0;i<5;++i){
            bool selected=(g_currentTab==i);
            ImGui::PushID(i);
            float btnW=layout.sidebar-24.f;
            float btnH=38.f;

            if(selected){
                ImGui::PushStyleColor(ImGuiCol_Button, kCardBg);
                ImGui::PushStyleColor(ImGuiCol_Text, kTextPrimary);
            }else{
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_Text, kTextMuted);
            }
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f,0.11f,0.14f,0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, kCardBg);

            if(ImGui::Button("##navTab",ImVec2(btnW,btnH)))g_currentTab=i;
            bool hovered=ImGui::IsItemHovered();

            ImVec2 bMin=ImGui::GetItemRectMin();
            ImVec2 bMax=ImGui::GetItemRectMax();

            // Indicador lateral ativo de 3.5px em roxo eletrico #7A5AF8
            if(selected){
                navigationDraw->AddRectFilled(
                    ImVec2(bMin.x, bMin.y + 4.f),
                    ImVec2(bMin.x + 3.5f, bMax.y - 4.f),
                    IM_COL32(122, 90, 248, 255), // #7A5AF8
                    2.0f
                );
            }

            // Icone da aba
            ImU32 iconCol = selected ? IM_COL32(122, 90, 248, 255) : (hovered ? IM_COL32(244, 244, 246, 255) : IM_COL32(113, 115, 132, 255));
            DrawCleanNavIcon(navigationDraw, ImVec2(bMin.x + 20.f, bMin.y + btnH * 0.5f), i, iconCol);

            // Texto da aba
            ImU32 textCol = selected ? IM_COL32(244, 244, 246, 255) : (hovered ? IM_COL32(230, 230, 238, 255) : IM_COL32(113, 115, 132, 255));
            navigationDraw->AddText(ImVec2(bMin.x + 38.f, bMin.y + (btnH - ImGui::GetFontSize()) * 0.5f), textCol, tabs[i]);

            ImGui::PopStyleColor(4);
            ImGui::PopID();
            ImGui::Dummy(ImVec2(0.f, 3.f));
        }
        ImGui::EndChild();

        // Footer da Sidebar
        ImGui::SetCursorPos({14,size.y-34});
        ImGui::TextColored(kAccent,"v2.4 | %s %s",__DATE__,__TIME__);
        ImGui::TextDisabled("CS2 Build %u",HazzureOffsets::kBuildNumber);

        // 3. Area Principal de Conteudo (Body)
        const float x=layout.sidebar+16.f;
        const float bodyWidth=size.x-x-16.f;

        // Topbar: Sub-tabs / Seletor de Secoes da Aba Atual
        ImGui::SetCursorPos({x,14.f});
        ImGui::BeginChild("##sectionPicker",{bodyWidth,44.f},false,ImGuiWindowFlags_NoScrollbar);
        int& section=g_panelSections[g_currentTab];
        section=std::clamp(section,0,kPanelSectionCounts[g_currentTab]-1);

        for(int i=0;i<kPanelSectionCounts[g_currentTab];++i){
            if(i)ImGui::SameLine(0,8.f);
            bool selected=(section==i);
            ImGui::PushID(i);
            if(selected){
                ImGui::PushStyleColor(ImGuiCol_Button, kCardBg);
                ImGui::PushStyleColor(ImGuiCol_Border, kAccent);
                ImGui::PushStyleColor(ImGuiCol_Text, kTextPrimary);
            }else{
                ImGui::PushStyleColor(ImGuiCol_Button, kWidgetBg);
                ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
                ImGui::PushStyleColor(ImGuiCol_Text, kTextMuted);
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,5.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.0f);
            if(ImGui::Button(kPanelSections[g_currentTab][i],{0,30.f}))section=i;
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
        ImGui::EndChild();

        // 4. Area de Colunas e Cards (Layout de 2 Colunas com espacamento 14px)
        ImGui::SetCursorPos({x,66.f});
        float bodyHeight=std::max(60.f,size.y-94.f);
        ImGui::PushID(g_currentTab);
        ImGui::PushID(section);
        if(layout.twoColumns){
            float columnWidth=(bodyWidth-14.f)*0.5f;
            for(int column=0;column<2;++column){
                if(column)ImGui::SameLine(0,14.f);
                ImGui::PushID(column);
                ImGui::BeginChild("##settingsColumn",{columnWidth,bodyHeight},false);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
                DrawPanelSection(shared,g_currentTab,section,column);
                ImGui::PopStyleVar();
                ImGui::EndChild();
                ImGui::PopID();
            }
        }else{
            ImGui::BeginChild("##settingsPage",{bodyWidth,bodyHeight},false);
            DrawPanelSection(shared,g_currentTab,section,0);
            DrawPanelSection(shared,g_currentTab,section,1);
            ImGui::EndChild();
        }
        ImGui::PopID();
        ImGui::PopID();

        // Footer da Pagina
        ImGui::SetCursorPos({x,size.y-22.f});
        ImGui::TextDisabled("%s  /  %s",tabs[g_currentTab],kPanelSections[g_currentTab][section]);
    }
    ImGui::End();
    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(9);
}
