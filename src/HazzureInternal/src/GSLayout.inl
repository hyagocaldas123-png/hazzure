// GSLayout.inl — Layout Principal Estilo GameSense
// DrawGSPanel() + DrawGSPanelSection() + roteamento dos 21 cards legados

#pragma once
// UIState.inl and GSWidgets.inl are included via HazzureUI.h
// UISettingsCards.inl, InventoryPanel.inl, LineupPanel.inl, SkinSelections.h, UITacticalHud.inl are already included via UIPanel.inl

namespace GSLayout {

using namespace GSWidgets;

// ============================================================================
// Estado de navegação (static: linkage interna; APENAS Internal.cpp em src inclui HazzureUI.h;
// testes são binário separado com sua própria cópia. Checkpoint original usava static.)
// ============================================================================
static int gs_currentTab = 0;
static int gs_currentSection[14] = {};
static int& s_legitSubTab = gs_currentSection[0]; // Uma section por aba principal
// gs_inventorySubTab movido para InventoryPanel.inl (g_graphicArtSubTab)

// Busca global do painel GS
static char gs_searchQuery[128] = {};
static bool gs_searchActive = false;
static float gs_searchAnim = 0.0f;

// Fixed capacity, owned by this UI context; no per-item storage growth.
struct NavigationFade {
    ImGuiContext* context{};
    int frame = -1;
    float selected[14]{}, hover[14]{};
    void BeginFrame() {
        auto* current = ImGui::GetCurrentContext();
        const int next = ImGui::GetFrameCount();
        if (context != current || next < frame) {
            *this = NavigationFade{}; context = current;
        }
        frame = next;
    }
};
static NavigationFade gs_navigationFade;

// Mapeamento de aliases de busca para (tab, section)
struct SearchAlias { const char* query; int tab; int section; };
static const SearchAlias gs_searchAliases[] = {
    // Clima / Weather -> Visuals/World (tab 6, section 1) ou World (tab 7, section 0)
    { "clima", 6, 1 }, { "tempo", 6, 1 }, { "clima 3d", 6, 1 }, { "weather", 6, 1 }, { "sky", 7, 0 }, { "fog", 7, 3 },
    // Mira / Crosshair -> Aim Assist (tab 1, section 0) ou View/Camera (tab 8, section 1)
    { "mira", 0, 0 }, { "mira personalizada", 0, 0 }, { "crosshair", 0, 0 }, { "ponteiro", 0, 0 },
    // Inventário / Inventory -> Inventory tab (11)
    { "inventario", 11, 0 }, { "inventário", 11, 0 }, { "inventory", 11, 0 }, { "equipment", 11, 1 }, { "editor", 11, 1 }, { "skins", 11, 0 }, { "itens", 11, 0 },
    // Visuals ESP
    { "esp", 6, 0 }, { "players", 6, 0 }, { "jogadores", 6, 0 }, { "glow", 6, 3 }, { "offscreen", 6, 3 },
    // Chams
    { "chams", 3, 0 }, { "material", 3, 3 },
    // Legitbot
    { "legit", 0, 0 }, { "aimbot", 0, 0 }, { "trigger", 0, 1 }, { "recoil", 0, 2 },
    // Rage
    { "rage", 1, 0 }, { "anti-aim", 1, 1 }, { "anti aim", 1, 1 }, { "antiaim", 1, 1 }, { "rage aim", 1, 0 },
    // World
    { "world", 7, 0 }, { "skybox", 7, 0 }, { "lighting", 7, 1 }, { "iluminacao", 7, 1 }, { "texturas", 7, 2 }, { "removals", 7, 3 },
    { "nightmode", 7, 1 }, { "dark world", 7, 1 }, { "tsukuyomi", 7, 1 }, { "vignette", 7, 1 }, { "vinheta", 7, 1 }, { "embers", 7, 1 },
    // View
    { "view", 8, 0 }, { "viewmodel", 8, 0 }, { "camera", 8, 1 }, { "aspect", 8, 2 },
    // Indicators
    { "indicators", 9, 0 }, { "overlay", 9, 0 }, { "tipografia", 9, 1 }, { "font", 9, 1 },
    // Misc
    { "misc", 10, 0 }, { "radar", 10, 1 }, { "som", 10, 2 }, { "hitsound", 10, 4 }, { "killsound", 10, 5 },
    // Grenade Helper
    { "granada", 12, 0 }, { "grenade", 12, 0 }, { "smoke", 12, 1 }, { "incendiary", 12, 2 }, { "fogo", 12, 2 }, { "tracer", 12, 3 },
    // Configs
    { "config", 13, 0 }, { "perfil", 13, 1 }, { "profile", 13, 1 }, { "diagnostico", 13, 2 }, { "diagnostics", 13, 2 },
    // Agents/Skins integrados ao Inventory (tab 11)
    { "agents", 11, 0 }, { "agentes", 11, 0 }, { "collection", 11, 0 }, { "colecao", 11, 0 },
};

// 14 Abas principais (Sidebar)
static const char* const gs_mainTabs[14] = {
    "Legitbot",       // 0
    "Aim Assist",     // 1
    "Players",        // 2
    "Chams",          // 3
    "Agents",         // 4
    "Items",          // 5
    "Visuals",        // 6
    "World",          // 7
    "View",           // 8
    "Indicators",     // 9
    "Miscellaneous",  // 10
    "Inventory",      // 11
    "Grenade Helper", // 12
    "Configs"         // 13
};

// Contador de sub-abas por aba principal (const: linkage interna por padrão C++; acessada via GSLayout:: qualificado)
const int gs_mainTabSectionCounts[14] = {
    4, // Aim Assist: Aim, Accuracy, RCS, Misc
    2, // Rage: Aim, Anti-Aim
    3, // Players: ESP, Glow, Offscreen
    4, // Chams: Enemy team, Local team, Flags, Customizations
    1, // Agents
    1, // Items
    4, // Visuals: ESP, World, Effects, Glow & Offscreen
    4, // World: Skybox, Lighting, Textures, Removals
    2, // View: Viewmodel, Camera & Third Person
    2, // Indicators: existing overlays and supported typography
    1, // Miscellaneous: supported movement, interface and audio
    5, // Inventory: Profile, Equipment, Containers, Tools, Graphic Art
    5, // Grenade Helper: Lineups, Prediction, Smoke, Incendiary, Tracers
    3  // Configs: General, Profiles, Diagnostics
};

// Nomes das sub-abas (topbar)
static const char* const gs_tabSections[14][6] = {
    // Legitbot
    { "Aim", "Accuracy", "RCS", "Misc", nullptr, nullptr },
    // Rage
    { "Aim", "Anti-Aim", nullptr, nullptr, nullptr, nullptr },
    // Players
    { "ESP", "Glow", "Offscreen", nullptr, nullptr, nullptr },
    // Chams
    { "Chams", "Gradient", "Flags", "Materials", nullptr, nullptr },
    // Skins
    { "Agents", nullptr, nullptr, nullptr, nullptr, nullptr },
    // Items
    { "Collection", nullptr, nullptr, nullptr, nullptr, nullptr },
    // Visuals
    { "ESP", "World", "Effects", "Glow & Offscreen", nullptr, nullptr },
    // World
    { "Skybox", "Lighting", "Textures", "Removals", nullptr, nullptr },
    // View
    { "Viewmodel", "Camera & Third Person", nullptr, nullptr, nullptr, nullptr },
    // Indicators
    { "Overlays", "Typography", nullptr, nullptr, nullptr, nullptr },
    // Miscellaneous
    { "General", nullptr, nullptr, nullptr, nullptr, nullptr },
    // Inventory
    { "Profile", "Editor", nullptr, nullptr, "Graphic Art", nullptr },
    // Grenade Helper
    { "Lineups & Helper", "Prediction", "Smoke Timer", "Incendiary", "Tracers", nullptr },
    // Configs
    { "General", "Profiles", "Diagnostics", nullptr, nullptr, nullptr }
};

// Processa a query de busca e navega para a aba/seção correspondente
// Retorna true se navegou, false caso contrário. NUNCA altera opções de configuração.
static bool GS_ProcessSearchQuery(const char* query) {
    if (!query || !*query) return false;
    using namespace HazzureUI::Workflow;
    std::string foldedQuery = Fold(query);
    if (foldedQuery.empty()) return false;

    // 1. Verificar aliases exatos (mapeamento direto)
    for (const auto& alias : gs_searchAliases) {
        if (Fold(alias.query) == foldedQuery) {
            gs_currentTab = alias.tab == 1 ? 0 : alias.tab;
            gs_currentSection[gs_currentTab] = std::clamp(alias.section, 0, gs_mainTabSectionCounts[gs_currentTab] - 1);
            if (gs_currentTab == 11 && alias.section == 1) g_inventorySubTab = 1;
            g_bindingTarget = nullptr;
            return true;
        }
    }

    // 2. Busca por substring nos nomes das abas principais
    for (int tab = 0; tab < 14; ++tab) {
        if (Matches(gs_mainTabs[tab], query)) {
            int targetTab = (tab == 4 || tab == 5) ? 11 : (tab == 2 ? 6 : tab);
            gs_currentTab = targetTab;
            gs_currentSection[targetTab] = std::clamp(gs_currentSection[targetTab], 0, gs_mainTabSectionCounts[targetTab] - 1);
            g_bindingTarget = nullptr;
            return true;
        }
    }

    // 3. Busca por substring nas sub-abas da aba atual
    int currentTab = (gs_currentTab >= 0 && gs_currentTab < 14) ? gs_currentTab : 0;
    int sectionCount = gs_mainTabSectionCounts[currentTab];
    for (int sec = 0; sec < sectionCount; ++sec) {
        const char* secName = gs_tabSections[currentTab][sec];
        if (secName && Matches(secName, query)) {
            gs_currentSection[currentTab] = sec;
            if (currentTab == 11 && (sec == 0 || sec == 1 || sec == 4)) g_inventorySubTab = sec;
            g_bindingTarget = nullptr;
            return true;
        }
    }

    // 4. Busca global em todas as sub-abas de todas as abas
    for (int tab = 0; tab < 14; ++tab) {
        int count = gs_mainTabSectionCounts[tab];
        for (int sec = 0; sec < count; ++sec) {
            const char* secName = gs_tabSections[tab][sec];
            if (secName && Matches(secName, query)) {
                int targetTab = (tab == 4 || tab == 5) ? 11 : (tab == 2 ? 6 : tab);
                gs_currentTab = targetTab;
                gs_currentSection[targetTab] = (targetTab == 11) ? 0 : sec;
                if (targetTab == 11 && (sec == 0 || sec == 1 || sec == 4)) g_inventorySubTab = sec;
                g_bindingTarget = nullptr;
                return true;
            }
        }
    }

    // 5. Busca nas sub-abas do Inventory (Profile, Equipment, Containers, Tools, Graphic Art)
    // g_inventoryTabs é definido em InventoryPanel.inl e visível via include order (UIPanel.inl -> InventoryPanel.inl)
    for (int i = 0; i < 5; ++i) {
        if (i != 2 && i != 3 && g_inventoryTabs[i] && Matches(g_inventoryTabs[i], query)) {
            gs_currentTab = 11;
            g_inventorySubTab = i;
            gs_currentSection[11] = 0;
            g_bindingTarget = nullptr;
            return true;
        }
    }

    return false;
}

// Widget de busca para o header (pode ser chamado de GSWidgets se preferir)
static void GS_DrawSearchBar(float x, float y, float width) {
    ImGui::SetCursorPos({ x, y });
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL(GS_DROPDOWN));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL(GS_CARD));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL(GS_CARD));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL(GS_BORDER));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL(GS_TEXT));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    ImGui::SetNextItemWidth(width);
    bool changed = ImGui::InputTextWithHint("##gs_search", "Buscar... (clima, mira, inventário)", gs_searchQuery, sizeof(gs_searchQuery));

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);

    if (changed && gs_searchQuery[0]) {
        GS_ProcessSearchQuery(gs_searchQuery);
        // Não limpamos a query automaticamente; usuário decide quando limpar
    }

    // Animação de foco
    gs_searchAnim = GS_Fade(gs_searchAnim, ImGui::IsItemActive() ? 1.f : 0.f, ImGui::GetIO().DeltaTime);
    gs_searchActive = ImGui::IsItemActive() || gs_searchAnim > .001f;

    // Indicador visual de busca ativa
    if (gs_searchAnim > 0.001f) {
        ImDrawList* d = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetItemRectMin();
        ImVec2 sz = ImGui::GetItemRectSize();
        ImU32 accentCol = IM_COL_A(GS_ACCENT, gs_searchAnim * 0.5f);
        d->AddRect(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + sz.x + 1, p.y + sz.y + 1), accentCol, 4.0f, 0, 1.5f + gs_searchAnim * 1.5f);
    }
    GS_Hint("Navega por nome sem alterar opcoes. Exemplos: clima, inventario, diagnostico.");
}

// ============================================================================
// Helpers de layout
// ============================================================================
struct GSLayoutData {
    float width = 820.0f;
    float height = 560.0f;
    float sidebarW = 200.0f;
    float headerH = 44.0f;
    float topbarH = 40.0f;
    float bodyX = 0.0f;
    float bodyY = 0.0f;
    float bodyW = 0.0f;
    float bodyH = 0.0f;
    bool twoColumns = true;
    float columnW = 0.0f;
};

static GSLayoutData ResolveGSLayout(int screenW, int screenH) {
    GSLayoutData layout;
    layout.width = std::max(1.0f, std::min(900.0f, static_cast<float>(screenW) - 16.0f));
    layout.height = std::max(1.0f, std::min(600.0f, static_cast<float>(screenH) - 16.0f));
    layout.headerH = 56.0f;
    layout.topbarH = 52.0f;
    layout.sidebarW = layout.width >= 860.0f ? 184.0f : (layout.width >= 580.0f ? 160.0f : 56.0f);
    layout.bodyX = layout.sidebarW + 12.0f;
    layout.bodyY = layout.headerH + layout.topbarH;
    layout.bodyW = std::max(1.0f, layout.width - layout.bodyX - 12.0f);
    layout.bodyH = std::max(1.0f, layout.height - layout.bodyY - 14.0f);
    layout.twoColumns = layout.bodyW >= 640.0f;
    layout.columnW = layout.twoColumns ? (layout.bodyW - 16.0f) * 0.5f : layout.bodyW;
    return layout;
}

// ============================================================================
// Particulas de Neve Animadas no Fundo do Menu (Estilo blocos caindo)
// ============================================================================
struct MenuSnowParticle {
    float x = 0.f;
    float y = 0.f;
    float size = 2.0f;
    float speedY = 40.0f;
    float swayAmp = 10.0f;
    float swayFreq = 1.0f;
    float swayPhase = 0.0f;
    float alpha = 0.7f;
};

static std::array<MenuSnowParticle, 96> s_menuSnowParticles{};
static bool s_menuSnowInitialized = false;

static void UpdateAndRenderMenuSnow(ImDrawList* draw, ImVec2 winMin, ImVec2 winMax, float dt, const float* colSnow) {
    if (!draw || dt <= 0.0f) return;
    float width = winMax.x - winMin.x;
    float height = winMax.y - winMin.y;
    if (width <= 20.0f || height <= 20.0f) return;

    if (!s_menuSnowInitialized) {
        s_menuSnowInitialized = true;
        for (size_t i = 0; i < s_menuSnowParticles.size(); ++i) {
            auto& p = s_menuSnowParticles[i];
            p.x = winMin.x + (float)(rand() % (int)width);
            p.y = winMin.y + (float)(rand() % (int)height);
            p.size = 1.6f + (float)(rand() % 26) * 0.1f; // 1.6px a 4.2px blocos
            p.speedY = 28.0f + (float)(rand() % 46);     // 28 a 74 px/s
            p.swayAmp = 5.0f + (float)(rand() % 14);
            p.swayFreq = 0.7f + (float)(rand() % 16) * 0.1f;
            p.swayPhase = (float)(rand() % 628) * 0.01f;
            p.alpha = 0.35f + (float)(rand() % 50) * 0.01f;
        }
    }

    ImU32 baseCol = colSnow ? Float4ToImU32(colSnow) : IM_COL32(255, 255, 255, 200);
    int r = (baseCol >> IM_COL32_R_SHIFT) & 0xFF;
    int g = (baseCol >> IM_COL32_G_SHIFT) & 0xFF;
    int b = (baseCol >> IM_COL32_B_SHIFT) & 0xFF;
    int aBase = (baseCol >> IM_COL32_A_SHIFT) & 0xFF;

    for (auto& p : s_menuSnowParticles) {
        p.y += p.speedY * dt;
        p.swayPhase += p.swayFreq * dt;
        float sway = std::sin(p.swayPhase) * p.swayAmp;
        float drawX = p.x + sway;

        if (p.y > winMax.y) {
            p.y = winMin.y - p.size;
            p.x = winMin.x + (float)(rand() % (int)width);
        } else if (p.y < winMin.y - 10.0f) {
            p.y = winMax.y;
        }

        if (drawX > winMax.x) p.x -= width;
        else if (drawX < winMin.x) p.x += width;

        int finalA = std::clamp((int)(aBase * p.alpha), 0, 255);
        ImU32 col = IM_COL32(r, g, b, finalA);

        draw->AddRectFilled(ImVec2(drawX, p.y), ImVec2(drawX + p.size, p.y + p.size), col, 0.8f);
    }
}

// ============================================================================
// Desenha header superior: "[Haz]zure", aba atual, busca e Save
// ============================================================================
static void DrawGSHeader(const GSLayoutData& layout) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos();
    d->AddRectFilled(p, ImVec2(p.x + layout.width, p.y + layout.headerH), IM_COL(GS_SIDEBAR), 3.0f, ImDrawFlags_RoundCornersTop);
    d->AddLine(ImVec2(p.x, p.y + layout.headerH), ImVec2(p.x + layout.width, p.y + layout.headerH), IM_COL(GS_BORDER));

    float brandEndX = 110.0f;
    {
        HazzureTypography::Scope brandFont(HazzureTypography::brand ? HazzureTypography::brand : HazzureTypography::heading);
        ImVec2 hazSz = ImGui::CalcTextSize("Haz");
        ImVec2 zureSz = ImGui::CalcTextSize("zure");
        float badgePadX = 5.0f;
        float badgePadY = 3.0f;
        float textY = p.y + (layout.headerH - hazSz.y) * 0.5f;
        float textX = p.x + 18.0f;

        // Accent rounded badge for Haz
        ImVec2 badgeMin = ImVec2(textX - badgePadX, textY - badgePadY);
        ImVec2 badgeMax = ImVec2(textX + hazSz.x + badgePadX, textY + hazSz.y + badgePadY);
        d->AddRectFilled(badgeMin, badgeMax, Float4ToImU32(g_cfg.colMenuAccent), 4.0f);

        // "Haz" in black text inside white badge
        d->AddText(ImVec2(textX, textY), IM_COL32(10, 10, 14, 255), "Haz");

        // "zure" in pure white
        float zureX = badgeMax.x + 5.0f;
        d->AddText(ImVec2(zureX, textY), IM_COL32(255, 255, 255, 255), "zure");

        brandEndX = (zureX - p.x) + zureSz.x + 20.0f;
    }

    // Topbar contextual adaptativa conforme aba ativa:
    if (gs_currentTab == 0) { // Legitbot (Print 2): [x] Master switch + ≡ + Scope combo + Weapon combo
        ImGui::SetCursorPos(ImVec2(brandEndX, 14.0f));
        ImGui::Checkbox("Master switch", &g_cfg.aimActive);
        ImGui::SameLine(0, 10.0f);
        if (ImGui::Button("Options", ImVec2(62, 28))) {}
        ImGui::SameLine(0, 10.0f);
        static int s_legitScope = 0;
        const char* scopes[] = { "Global", "Per weapons", "Per group" };
        ImGui::SetNextItemWidth(104.0f);
        ImGui::Combo("##legitScope", &s_legitScope, scopes, 3);
        ImGui::SameLine(0, 8.0f);
        static int s_legitWeapon = 1;
        const char* legitWeapons[] = { "Desert Eagle", "AK-47", "AWP", "M4A4", "M4A1-S", "USP-S", "Glock-18" };
        ImGui::SetNextItemWidth(110.0f);
        ImGui::Combo("##legitWeapon", &s_legitWeapon, legitWeapons, 7);
    } else if (gs_currentTab == 3) { // Chams (Print 4): [x] Master switch + Target combo + Options
        ImGui::SetCursorPos(ImVec2(brandEndX, 14.0f));
        ImGui::Checkbox("Master switch", &g_cfg.chamsVisible);
        ImGui::SameLine(0, 10.0f);
        static int s_chamsTarget = 0;
        const char* targets[] = { "Players", "Viewmodel / Arms", "Weapons" };
        ImGui::SetNextItemWidth(130.0f);
        ImGui::Combo("##chamsTarget", &s_chamsTarget, targets, 3);
        ImGui::SameLine(0, 8.0f);
        if (ImGui::Button("Options", ImVec2(68, 28))) {}
    } else if (gs_currentTab == 11) { // Inventory (Print 1): [Settings] + [Create from link]
        ImGui::SetCursorPos(ImVec2(brandEndX, 14.0f));
        if (ImGui::Button("Settings", ImVec2(76, 28))) {}
        ImGui::SameLine(0, 8.0f);
        if (ImGui::Button("Create from link", ImVec2(120, 28))) {}
    } else if (layout.width >= 580) {
        ImGui::SetCursorPos(ImVec2(brandEndX, (layout.headerH - ImGui::GetFontSize()) * 0.5f));
        ImGui::TextColored(ImVec4(0.82f, 0.83f, 0.87f, 1.0f), "%s", gs_mainTabs[gs_currentTab]);
    }

    // Botão Theme & Snow no Header (acessível de qualquer tela)
    ImGui::SetCursorPos(ImVec2(layout.width - 168, 14));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    if (ImGui::Button("Theme", ImVec2(72, 28))) {
        ImGui::OpenPopup("##header_theme_popup");
    }
    ImGui::PopStyleVar();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Customizar cores do tema, painel e neve");

    if (ImGui::BeginPopup("##header_theme_popup")) {
        ImGui::Text("Menu Theme & Snow");
        ImGui::Separator();
        ImGui::Checkbox("Neve Caindo (Snow Particles)", &g_cfg.menuSnow);
        ImGui::Spacing();

        static int s_hdrThemePreset = 1;
        const char* hdrThemePresets[] = { "Personalizado", "Violet Haze", "Glacial Cyan", "Blood Moon", "Mono B&W", "Emerald Mint" };
        ImGui::Text("Preset do Tema:");
        ImGui::SetNextItemWidth(170.0f);
        if (ImGui::Combo("##hdrThemePreset", &s_hdrThemePreset, hdrThemePresets, 6)) {
            if (s_hdrThemePreset == 1) { // Violet Haze
                g_cfg.colMenuAccent[0] = 0.65f; g_cfg.colMenuAccent[1] = 0.33f; g_cfg.colMenuAccent[2] = 0.97f; g_cfg.colMenuAccent[3] = 1.0f;
                g_cfg.colSnow[0] = 0.95f; g_cfg.colSnow[1] = 0.91f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.85f;
                g_cfg.colMenuBg[0] = 0.043f; g_cfg.colMenuBg[1] = 0.043f; g_cfg.colMenuBg[2] = 0.063f; g_cfg.colMenuBg[3] = 1.0f;
            } else if (s_hdrThemePreset == 2) { // Glacial Cyan
                g_cfg.colMenuAccent[0] = 0.0f; g_cfg.colMenuAccent[1] = 0.90f; g_cfg.colMenuAccent[2] = 1.0f; g_cfg.colMenuAccent[3] = 1.0f;
                g_cfg.colSnow[0] = 0.88f; g_cfg.colSnow[1] = 0.97f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.80f;
                g_cfg.colMenuBg[0] = 0.039f; g_cfg.colMenuBg[1] = 0.055f; g_cfg.colMenuBg[2] = 0.078f; g_cfg.colMenuBg[3] = 1.0f;
            } else if (s_hdrThemePreset == 3) { // Blood Moon
                g_cfg.colMenuAccent[0] = 1.0f; g_cfg.colMenuAccent[1] = 0.16f; g_cfg.colMenuAccent[2] = 0.33f; g_cfg.colMenuAccent[3] = 1.0f;
                g_cfg.colSnow[0] = 1.0f; g_cfg.colSnow[1] = 1.0f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.85f;
                g_cfg.colMenuBg[0] = 0.047f; g_cfg.colMenuBg[1] = 0.047f; g_cfg.colMenuBg[2] = 0.055f; g_cfg.colMenuBg[3] = 1.0f;
            } else if (s_hdrThemePreset == 4) { // Mono B&W
                g_cfg.colMenuAccent[0] = 1.0f; g_cfg.colMenuAccent[1] = 1.0f; g_cfg.colMenuAccent[2] = 1.0f; g_cfg.colMenuAccent[3] = 1.0f;
                g_cfg.colSnow[0] = 1.0f; g_cfg.colSnow[1] = 1.0f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.75f;
                g_cfg.colMenuBg[0] = 0.055f; g_cfg.colMenuBg[1] = 0.058f; g_cfg.colMenuBg[2] = 0.068f; g_cfg.colMenuBg[3] = 1.0f;
            } else if (s_hdrThemePreset == 5) { // Emerald Mint
                g_cfg.colMenuAccent[0] = 0.0f; g_cfg.colMenuAccent[1] = 0.90f; g_cfg.colMenuAccent[2] = 0.46f; g_cfg.colMenuAccent[3] = 1.0f;
                g_cfg.colSnow[0] = 0.91f; g_cfg.colSnow[1] = 0.97f; g_cfg.colSnow[2] = 0.94f; g_cfg.colSnow[3] = 0.80f;
                g_cfg.colMenuBg[0] = 0.031f; g_cfg.colMenuBg[1] = 0.051f; g_cfg.colMenuBg[2] = 0.039f; g_cfg.colMenuBg[3] = 1.0f;
            }
        }

        static int s_hdrSnowPreset = 1;
        const char* hdrSnowPresets[] = { "Personalizado", "Lavanda Suave", "Branco Puro", "Ciano Glacial", "Carmesim" };
        ImGui::Text("Preset da Neve:");
        ImGui::SetNextItemWidth(170.0f);
        if (ImGui::Combo("##hdrSnowPreset", &s_hdrSnowPreset, hdrSnowPresets, 5)) {
            if (s_hdrSnowPreset == 1) { g_cfg.colSnow[0]=0.95f; g_cfg.colSnow[1]=0.91f; g_cfg.colSnow[2]=1.0f; g_cfg.colSnow[3]=0.85f; }
            else if (s_hdrSnowPreset == 2) { g_cfg.colSnow[0]=1.0f; g_cfg.colSnow[1]=1.0f; g_cfg.colSnow[2]=1.0f; g_cfg.colSnow[3]=0.85f; }
            else if (s_hdrSnowPreset == 3) { g_cfg.colSnow[0]=0.88f; g_cfg.colSnow[1]=0.97f; g_cfg.colSnow[2]=1.0f; g_cfg.colSnow[3]=0.80f; }
            else if (s_hdrSnowPreset == 4) { g_cfg.colSnow[0]=1.0f; g_cfg.colSnow[1]=0.70f; g_cfg.colSnow[2]=0.75f; g_cfg.colSnow[3]=0.85f; }
        }

        ImGui::Spacing();
        ImGui::Text("Ajuste Fino de Cores:");
        ImGui::ColorEdit4("Acento##pkr", g_cfg.colMenuAccent, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine(); ImGui::Text("Acento");
        ImGui::ColorEdit4("Fundo##pkr", g_cfg.colMenuBg, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine(); ImGui::Text("Fundo");
        ImGui::ColorEdit4("Neve##pkr", g_cfg.colSnow, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine(); ImGui::Text("Neve");

        ImGui::EndPopup();
    }

    // Botão Save à direita em todas as telas (colorido com accent MemeSense)
    ImGui::SetCursorPos(ImVec2(layout.width - 88, 14));
    ImVec4 saveBtnCol(g_cfg.colMenuAccent[0], g_cfg.colMenuAccent[1], g_cfg.colMenuAccent[2], g_cfg.colMenuAccent[3]);
    ImVec4 saveBtnHov(std::min(1.0f, saveBtnCol.x * 1.15f), std::min(1.0f, saveBtnCol.y * 1.15f), std::min(1.0f, saveBtnCol.z * 1.15f), 1.0f);
    ImVec4 saveBtnAct(saveBtnCol.x * 0.85f, saveBtnCol.y * 0.85f, saveBtnCol.z * 0.85f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, saveBtnCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saveBtnHov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, saveBtnAct);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    if (ImGui::Button("Save", ImVec2(72, 28))) SaveConfigFile();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Salvar configuracao em disco");
}

// Índices de tabs ocultos apenas no render (preservados nos arrays/persistência)
// 4 = Agents e 5 = Items unificados dentro de Inventory (11); 2 = Players unificado em Visuals (6)
static const bool gs_hiddenTabs[14] = {
    false, false, false, false, true, false,
    false, false, false, false, false, false, true, false
};

static void DrawGSSidebar(const GSLayoutData& layout, ImVec2 winPos) {
    ImDrawList* background = ImGui::GetWindowDrawList();
    background->AddRectFilled(ImVec2(winPos.x, winPos.y + layout.headerH), ImVec2(winPos.x + layout.sidebarW, winPos.y + layout.height), IM_COL(GS_SIDEBAR), 10.0f, ImDrawFlags_RoundCornersBottomLeft);
    background->AddLine(ImVec2(winPos.x + layout.sidebarW, winPos.y + layout.headerH), ImVec2(winPos.x + layout.sidebarW, winPos.y + layout.height), IM_COL(GS_BORDER));
    ImGui::SetCursorPos(ImVec2(8, layout.headerH + 12));
    ImGui::BeginChild("##gs_sidebar", ImVec2(layout.sidebarW - 16, layout.height - layout.headerH - 44), false);
    gs_navigationFade.BeginFrame();
    const char* hints[] = {
        "Mira, recoil e controles de disparo.", "Aimbot agressivo e anti-aim.",
        "Informacoes visuais dos jogadores.", "Materiais e cores dos modelos.",
        "Selecao visual de agentes.", "Itens da colecao local.",
        "ESP, clima e efeitos visuais.", "Ceu, iluminacao e ambiente.",
        "FOV, camera e proporcao da imagem.", "Overlays e tipografia.",
        "Eventos, radar e sons.", "Criar e organizar variantes locais; sincronizacao separada.",
        "Previsao de trajetoria e temporizadores.", "Preferencias, perfis e status observado dos recursos."
    };
    for (int i = 0; i < 14; ++i) {
        if (gs_hiddenTabs[i]) continue; // Pular render, mas preservar índice
        
        ImGui::PushID(i);
        bool selected = gs_currentTab == i;
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0,0,0,0));
        const float w = std::max(1.0f, ImGui::GetContentRegionAvail().x);
        if (ImGui::Button("##tab", ImVec2(w, 32))) {gs_currentTab = i; g_bindingTarget = nullptr;}
        ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        ImDrawList* d = ImGui::GetWindowDrawList(); // Child clip applies to all navigation visuals.
        auto& fade = gs_navigationFade;
        fade.selected[i] = GS_Fade(fade.selected[i], selected ? 1.f : 0.f, ImGui::GetIO().DeltaTime);
        fade.hover[i] = GS_Fade(fade.hover[i], ImGui::IsItemHovered() ? 1.f : 0.f, ImGui::GetIO().DeltaTime);
        d->AddRectFilled(a, b, IM_COL_A(GS_ACCENT, .14f * fade.selected[i] + .07f * fade.hover[i]), 3);
        if (selected) d->AddRectFilled(ImVec2(a.x, a.y + 7), ImVec2(a.x + 3, b.y - 7), Float4ToImU32(g_cfg.colMenuAccent), 2);
        d->PushClipRect(a, b, true);
        ImU32 iconCol = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(236, 238, 246, 255);
        ImU32 labelCol = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(240, 242, 248, 255);
        GS_NavIcon(d, i, ImVec2(a.x + 8, a.y + 8), iconCol);
        if (layout.sidebarW > 80) {
            if (selected && HazzureTypography::heading) {
                d->AddText(HazzureTypography::heading, HazzureTypography::heading->FontSize, ImVec2(a.x + 32, a.y + 8), labelCol, gs_mainTabs[i]);
            } else {
                d->AddText(ImVec2(a.x + 32, a.y + 8), labelCol, gs_mainTabs[i]);
            }
        }
        d->PopClipRect();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("%s\n%s", gs_mainTabs[i], hints[i]);
        ImGui::PopStyleColor(2);ImGui::PopID();
    }
    ImGui::EndChild();
}

static void DrawGSTopbar(const GSLayoutData& layout, ImVec2) {
    int count = gs_mainTabSectionCounts[gs_currentTab];
    int& section = gs_currentSection[gs_currentTab];section = std::clamp(section, 0, count - 1);
    ImGui::SetCursorPos(ImVec2(layout.bodyX, layout.headerH + 8));
    // Inventory, Legitbot e Miscellaneous possuem sua própria divisão interna; evita topbar duplicada desconectada.
    if (gs_currentTab == 0 || gs_currentTab == 2 || gs_currentTab == 3 || gs_currentTab == 6 || gs_currentTab == 7 || gs_currentTab == 10 || gs_currentTab == 11) return;
    if (count == 1) {
        ImGui::TextColored(GS_TEXT, "%s", gs_mainTabs[gs_currentTab]);return;
    }
    ImGui::BeginChild("##gs_topbar", ImVec2(layout.bodyW, layout.topbarH - 12), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (int i = 0; i < count; ++i) {
        if (i) ImGui::SameLine(0, 8);
        ImGui::PushID(i);
        const bool active = (section == i);
        ImGui::PushStyleColor(ImGuiCol_Button, active ? IM_COL32(244, 244, 248, 255) : IM_COL(GS_DROPDOWN));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? IM_COL32(255, 255, 255, 255) : IM_COL(GS_BORDER_HOVER));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, active ? IM_COL32(225, 225, 232, 255) : IM_COL(GS_BORDER_HOVER));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(0.05f, 0.05f, 0.07f, 1.0f) : ImVec4(0.92f, 0.93f, 0.96f, 1.0f));
        if (ImGui::Button(gs_tabSections[gs_currentTab][i], ImVec2(0, 28))) {section = i;g_bindingTarget=nullptr;}
        ImGui::PopStyleColor(4);ImGui::PopID();
    }
    ImGui::EndChild();
}

// ============================================================================
// Static arrays for GS_Combo (replacing C99 compound literals)
// ============================================================================
static const char* const gs_combo_aimPriority[4] = { "Distance", "FOV", "Damage", "Health" };
static const char* const gs_combo_aimBone[4] = { "Head", "Neck", "Chest", "Pelvis" };
static const char* const gs_combo_materialStyle[5] = { "Flat", "Metallic/Chrome", "Wireframe", "Fresnel Rim-Glow", "Crystal" };
static const char* const gs_combo_gloveModel[8] = { "Default", "Bloodhound", "Sporty", "Slick", "Leather", "Motorcycle", "Specialist", "Hydra" };
static const char* const gs_combo_agentCT[4] = { "Default", "FBI", "ST6", "SWAT" };
static const char* const gs_combo_agentT[4] = { "Default", "Phoenix", "Elite Crew", "Professionals" };
static const char* const gs_combo_hitSoundPreset[4] = { "Default", "Bell", "Bubble", "Custom" };

// ============================================================================
// Tela Legitbot / Aim Assist - Estilo MemeSense fiel ao print do usuario
// ============================================================================
// ============================================================================
// Tela Legitbot / Aim Assist - Estilo MemeSense fiel ao print 2 do usuario
// ============================================================================
static void DrawLegitbotMemeSense(int column, Shared* shared) {
    (void)shared;
    if (column == 0) {
        // Coluna Esquerda: General
        GS_SectionHeader("General");
        GS_Toggle("Enable", &g_cfg.aimActive);
        GS_Keybind("Aim key", &g_cfg.aimKey);
        GS_Toggle("Draw FOV", &g_cfg.espFovCircle);
        int aimTime = g_cfg.aimReactionMinMs;
        if (GS_SliderInt("Aim time", &aimTime, 0, 1000, aimTime == 0 ? "disabled" : "%d ms")) g_cfg.aimReactionMinMs = aimTime;

        ImGui::Dummy(ImVec2(0, 4.0f));

        // Sub-tabs internas: Aim | Accuracy | RCS | Misc com sublinhado fino
        const char* subTabs[] = { "Aim", "Accuracy", "RCS", "Misc" };
        for (int i = 0; i < 4; ++i) {
            if (i > 0) ImGui::SameLine(0, 16.0f);
            bool sel = (s_legitSubTab == i);
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 10));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 20));
            ImGui::PushStyleColor(ImGuiCol_Text, sel ? IM_COL32(255, 255, 255, 255) : IM_COL32(110, 114, 126, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            if (ImGui::Button(subTabs[i])) s_legitSubTab = i;
            if (sel) {
                ImVec2 bMin = ImGui::GetItemRectMin(), bMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine(ImVec2(bMin.x, bMax.y + 1.f), ImVec2(bMax.x, bMax.y + 1.f), IM_COL32(255, 255, 255, 255), 1.5f);
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::PopID();
        }

        ImGui::Dummy(ImVec2(0, 4.0f));

        if (s_legitSubTab == 0) { // Aim
            const char* radTypes[] = { "Static", "Dynamic / Distance" };
            int radType = g_cfg.aimDistanceFov ? 1 : 0;
            if (GS_Combo("Radius type", &radType, radTypes, 2)) g_cfg.aimDistanceFov = (radType == 1);
            GS_SliderFloat("Radius", &g_cfg.aimFov, 0.100f, 25.000f, "%.3f deg");
            GS_SliderFloat("Multiplier", &g_cfg.aimEaseBias, 1.00f, 10.00f, "%.2f");
            const char* smoothTypes[] = { "Static", "Dynamic" };
            int smType = g_cfg.aimEase ? 1 : 0;
            if (GS_Combo("Smooth type", &smType, smoothTypes, 2)) g_cfg.aimEase = (smType == 1);
            GS_SliderFloat("Smooth pitch", &g_cfg.aimSmooth, 1.000f, 25.000f, "%.3f");
            GS_SliderFloat("Smooth yaw", &g_cfg.aimSmooth, 1.000f, 25.000f, "%.3f");
        } else if (s_legitSubTab == 1) { // Accuracy
            DrawCombatReactionCard();
            DrawAccuracyControls();
        } else if (s_legitSubTab == 2) { // RCS
            DrawSettingsCard8(shared);
        } else if (s_legitSubTab == 3) { // Misc
            GS_Toggle("Auto-Scope AWP / Scout", &g_cfg.autoScope);
            GS_Toggle("Visible Only", &g_cfg.aimVisibleOnly);
            GS_SliderInt("After kill delay", &g_cfg.aimKillDelayMs, 0, 1000, "%d ms");
            GS_SliderInt("Target switch delay", &g_cfg.aimSwitchDelayMs, 0, 1000, "%d ms");
        }
    } else {
        // Coluna Direita: Hit groups + Auto fire / Auto wall
        GS_SectionHeader("Hit groups");
        GS_Combo("Selection", &g_cfg.aimBone, gs_combo_aimBone, 4);
        GS_ToggleWithKeybind("Only body aim", &g_cfg.forceBodyAim, &g_cfg.forceBodyAimKey);
        bool prefHead = !g_cfg.aimBodyFallback;
        if (GS_Toggle("Prefer head aim", &prefHead)) g_cfg.aimBodyFallback = !prefHead;
        GS_Toggle("Prefer body aim", &g_cfg.aimBodyFallback);
        GS_Toggle("Body aim if lethal", &g_cfg.baimOnPeek);
        const char* targetTypes[] = { "Players" };
        int tgtIdx = 0;
        GS_Combo("Targets", &tgtIdx, targetTypes, 1);

        ImGui::Spacing();
        GS_SectionHeader("Auto fire | Auto wall");
        GS_Toggle("Enable", &g_cfg.autoFire);
        GS_Toggle("Auto stop", &g_cfg.shotStop);
        const char* hcTypes[] = { "Strict", "Balanced" };
        int hcIdx = g_cfg.hitchanceEnabled ? 0 : 1;
        if (GS_Combo("Hit chance type", &hcIdx, hcTypes, 2)) g_cfg.hitchanceEnabled = (hcIdx == 0);
        int reactMs = static_cast<int>(g_cfg.triggerDelayMs);
        if (GS_SliderInt("Reaction time", &reactMs, 0, 500, reactMs == 0 ? "disabled" : "%d ms")) g_cfg.triggerDelayMs = static_cast<float>(reactMs);
        GS_SliderInt("Delay between shots", &g_cfg.triggerPauseMs, 0, 1000, g_cfg.triggerPauseMs == 0 ? "disabled" : "%d ms");
        GS_SliderFloat("Minimum hit chance", &g_cfg.hitchancePercent, 0.0f, 100.0f, "%.0f%%");
        GS_SliderInt("Ghost time", &g_cfg.poseHistoryMs, 0, 200, g_cfg.poseHistoryMs == 0 ? "disabled" : "%d ms");
    }
}

// ============================================================================
// Tela Players - Estilo MemeSense fiel ao print 3 do usuario
// ============================================================================
static void DrawPlayersMemeSense(int column, Shared* shared) {
    (void)shared;
    if (column == 0) {
        GS_SectionHeader("General");
        GS_Toggle("Enable", &g_cfg.espEnabled);
        GS_Toggle("Through wall", &g_cfg.espThroughWall);
        GS_Toggle("Nickname", &g_cfg.espName);
        GS_Toggle("Box", &g_cfg.espBoxes);
        if (g_cfg.espBoxes) {
            const char* boxStyles[] = { "Full box", "Corners" };
            GS_Combo("Box style", &g_cfg.espBoxType, boxStyles, 2);
        }
        GS_Toggle("Health bar", &g_cfg.espHealth);
        const char* barPositions[] = { "Left", "Right", "Top", "Bottom" };
        GS_Toggle("Armor bar", &g_cfg.espShowArmor);
        if (g_cfg.espShowArmor) {
            GS_ColorRow("Armor Color", g_cfg.colArmorBar);
            GS_Combo("Armor Position", &g_cfg.armorBarPos, barPositions, 4);
        }
        GS_Toggle("Ammo bar", &g_cfg.espAmmoBar);
        if (g_cfg.espAmmoBar) {
            GS_ColorRow("Ammo Color", g_cfg.colAmmoBar);
            const char* ammoPosNames[] = { "Bottom", "Top", "Left", "Right" };
            GS_Combo("Ammo Position", &g_cfg.ammoBarPos, ammoPosNames, 4);
        }
        GS_Toggle("Weapon", &g_cfg.espWeapon);
        if (g_cfg.espWeapon) GS_Toggle("Weapon icons", &g_cfg.espWeaponIcons);
        GS_Toggle("Skeleton", &g_cfg.espSkeleton);
        GS_Toggle("Sound arrow", &g_cfg.soundEsp);
        GS_Toggle("Out of FOV arrow", &g_cfg.offScreenArrows);
    } else {
        GS_SectionHeader("Flags");
        GS_Toggle("Bomb", &g_cfg.espFlagBomb);
        GS_Toggle("Health", &g_cfg.espFlagHealth);
        GS_Toggle("Ammo", &g_cfg.espFlagAmmo);
        GS_Toggle("Reload", &g_cfg.espFlagReload);
        GS_Toggle("Scoped", &g_cfg.espFlagScoped);
        GS_Toggle("Flashed", &g_cfg.espFlagFlashed);
        GS_Toggle("Distance", &g_cfg.espFlagDistance);
        GS_Toggle("Defuse kit", &g_cfg.espFlagDefuseKit);
        GS_Toggle("Helmet & kevlar", &g_cfg.espFlagHelmet);
        GS_Toggle("Defusing / rescuing", &g_cfg.espFlagDefusing);
        GS_Toggle("Pin pulled", &g_cfg.espFlagPinPulled);
        GS_Toggle("Planting", &g_cfg.espFlagPlanting);
    }
}

// ============================================================================
// Tela Chams - Estilo MemeSense fiel ao print 4 do usuario
// ============================================================================
static void DrawChamsMemeSense(int column, Shared* shared) {
    (void)shared;
    const char* matStyles[] = { "Textured", "Flat", "Metallic", "Wireframe", "Crystal" };
    if (column == 0) {
        GS_SectionHeader("Enemy team");
        GS_Toggle("Gun game immunity", &g_cfg.espFlagsExtras);
        ImGui::Dummy(ImVec2(0, 2.0f));
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL(GS_TEXT_MUTED)), "Customizations");
        GS_Toggle("Glow", &g_cfg.glowOutline);
        GS_ColorRow("Glow Color", g_cfg.colEnemy);
        GS_Toggle("Visible", &g_cfg.chamsVisible);
        GS_ColorRow("Visible Color", g_cfg.colVisible);
        GS_Combo("Visible Style", &g_cfg.chamsVisibleStyle, matStyles, 5);
        GS_Toggle("Invisible", &g_cfg.chamsHidden);
        GS_ColorRow("Invisible Color", g_cfg.colHidden);
        GS_Combo("Invisible Style", &g_cfg.chamsHiddenStyle, matStyles, 5);
        GS_Toggle("History (Rastro)", &g_cfg.chamsHistory);
        GS_ColorRow("History Color", g_cfg.colHistory);
        GS_Toggle("On-Shot Flash", &g_cfg.chamsOnShot);
        GS_ColorRow("On-Shot Color", g_cfg.colOnShot);
        GS_Toggle("Glow (Contorno)", &g_cfg.chamsGlow);
    } else {
        GS_SectionHeader("Local team");
        GS_Toggle("Gun game immunity##local", &g_cfg.espFlagsExtras);
        ImGui::Dummy(ImVec2(0, 2.0f));
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL(GS_TEXT_MUTED)), "Customizations");
        static bool s_teamGlow = false;
        GS_Toggle("Glow##team", &s_teamGlow);
        GS_ColorRow("Team Glow Color", g_cfg.colTeam);
        GS_Toggle("Visible##team", &g_cfg.chamsArms);
        GS_ColorRow("Team Visible Color", g_cfg.colTeam);
        static int s_teamMat = 0;
        const char* teamMatStyles[] = { "Textured", "Flat" };
        GS_Combo("Team Style", &s_teamMat, teamMatStyles, 2);
        GS_Toggle("Invisible##team", &g_cfg.chamsWeapon);
        GS_ColorRow("Team Invisible Color", g_cfg.colTeam);
        GS_Combo("Invisible Style##team", &s_teamMat, teamMatStyles, 2);
        GS_Toggle("Local Player##local", &g_cfg.chamsLocal);
        GS_ColorRow("Local Color", g_cfg.colLocal);
        GS_Toggle("Aliados##team", &g_cfg.chamsTeam);
        GS_ColorRow("Team Color", g_cfg.colTeam);
    }
}

// ============================================================================
// Tela Visuals - Estilo MemeSense fiel ao print 5 do usuario (Grenades | Misc)
// ============================================================================
static void DrawVisualsMemeSense(int column, Shared* shared) {
    (void)shared;
    if (column == 0) {
        GS_SectionHeader("Grenades");
        GS_Toggle("Grenade prediction", &g_cfg.showGrenadeTrajectory);
        GS_ColorRow("Trajectory Color", g_cfg.colTrajectory);
        GS_ColorRow("Collision Color", g_cfg.colTrajectoryCollision);
        GS_Toggle("Smoke grenade timer", &g_cfg.grenadeZones);
        GS_ColorRow("Smoke Color", g_cfg.colSmoke);
        GS_Toggle("Incendiary grenade timer", &g_cfg.grenadeZones);
        GS_ColorRow("Incendiary Color", g_cfg.colMolotov);
        GS_Toggle("Grenade tracers", &g_cfg.grenadeProjectiles);
    } else {
        GS_SectionHeader("Miscellaneous");
        GS_Toggle("Bomb timer", &g_cfg.showBombTimer);
        GS_Toggle("Spectator list", &g_cfg.showSpectators);
        GS_Toggle("Hit marker", &g_cfg.showHitmarker);
        GS_Toggle("Textured radar", &g_cfg.nativeRadar);
    }
}

// ============================================================================
// Tela World - Estilo MemeSense fiel ao print 6 do usuario
// ============================================================================
static void DrawWorldMemeSense(int column, Shared* shared) {
    (void)shared;
    if (column == 0) {
        GS_SectionHeader("Textures");
        GS_Toggle("Skybox", &g_cfg.skyActive);
        GS_ColorRow("Skybox Color", g_cfg.colSky);
        GS_Toggle("Lightning", &g_cfg.darkWorld);
        GS_Toggle("World textures", &g_cfg.cleanMap);
        GS_ColorRow("World Fog Color", g_cfg.colFog);
        GS_SliderFloat("Exposure", &g_cfg.worldBrightness, 0.05f, 2.00f, "%.3f");
        if (ImGui::Button("Sincronizar Skybox & Fog (1-clique)")) {
            memcpy(g_cfg.colFog, g_cfg.colSky, sizeof(g_cfg.colFog));
            g_cfg.fogDensity = 0.65f;
            if (shared) {
                shared->fogDensity = g_cfg.fogDensity;
                shared->fogColour = Float4ToIpcCol(g_cfg.colFog);
            }
        }
        ImGui::Spacing();
        GS_SectionHeader("Extra");
        GS_Toggle("Force sniper crosshair", &g_cfg.forceSniperCrosshair);
        GS_Toggle("Penetration crosshair", &g_cfg.penetrationCrosshair);
        if (GS_SliderFloat("Flash scale", &g_cfg.flashAlphaLimit, 0.00f, 1.00f, "%.2f")) {
            g_cfg.limitFlash = (g_cfg.flashAlphaLimit < 1.0f);
        }
    } else {
        GS_SectionHeader("Removals");
        GS_Toggle("Shadows", &g_cfg.cleanMap);
        GS_Toggle("Zoom hud", &g_cfg.noScopeOverlay);
        GS_Toggle("Zoom spread", &g_cfg.showSpreadCircle);
        GS_Toggle("Visual recoil", &g_cfg.noVisualRecoil);
        GS_Toggle("Weather original", &g_cfg.removeOriginalWeather);
    }
}

// ============================================================================
// Tela Miscellaneous - Estilo MemeSense fiel ao print do usuario
// ============================================================================
static void DrawMiscellaneousMemeSense(int column, Shared* /*shared*/) {
    if (column == 0) {
        DrawMovementBasicCard();
        DrawMovementAutomationCard();
    } else {
        GS_SectionHeader("Interface & Radar");
        GS_Toggle("Menu Snow Particles", &g_cfg.menuSnow);
        GS_ColorRow("Snow Color", g_cfg.colSnow);
        GS_ColorRow("Panel Accent Color", g_cfg.colMenuAccent);
        GS_ColorRow("Panel Background Color", g_cfg.colMenuBg);
        ImGui::Spacing();
        ImGui::TextDisabled("Theme Presets:");
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        if (ImGui::Button("Violet Haze##m", ImVec2(84.0f, 22.0f))) {
            g_cfg.colMenuAccent[0] = 0.65f; g_cfg.colMenuAccent[1] = 0.33f; g_cfg.colMenuAccent[2] = 0.97f; g_cfg.colMenuAccent[3] = 1.0f;
            g_cfg.colSnow[0] = 0.95f; g_cfg.colSnow[1] = 0.91f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.85f;
            g_cfg.colMenuBg[0] = 0.043f; g_cfg.colMenuBg[1] = 0.043f; g_cfg.colMenuBg[2] = 0.063f; g_cfg.colMenuBg[3] = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Glacial Cyan##m", ImVec2(84.0f, 22.0f))) {
            g_cfg.colMenuAccent[0] = 0.0f; g_cfg.colMenuAccent[1] = 0.90f; g_cfg.colMenuAccent[2] = 1.0f; g_cfg.colMenuAccent[3] = 1.0f;
            g_cfg.colSnow[0] = 0.88f; g_cfg.colSnow[1] = 0.97f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.80f;
            g_cfg.colMenuBg[0] = 0.039f; g_cfg.colMenuBg[1] = 0.055f; g_cfg.colMenuBg[2] = 0.078f; g_cfg.colMenuBg[3] = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Blood Moon##m", ImVec2(84.0f, 22.0f))) {
            g_cfg.colMenuAccent[0] = 1.0f; g_cfg.colMenuAccent[1] = 0.16f; g_cfg.colMenuAccent[2] = 0.33f; g_cfg.colMenuAccent[3] = 1.0f;
            g_cfg.colSnow[0] = 1.0f; g_cfg.colSnow[1] = 1.0f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.85f;
            g_cfg.colMenuBg[0] = 0.047f; g_cfg.colMenuBg[1] = 0.047f; g_cfg.colMenuBg[2] = 0.055f; g_cfg.colMenuBg[3] = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Mono B&W##m", ImVec2(72.0f, 22.0f))) {
            g_cfg.colMenuAccent[0] = 1.0f; g_cfg.colMenuAccent[1] = 1.0f; g_cfg.colMenuAccent[2] = 1.0f; g_cfg.colMenuAccent[3] = 1.0f;
            g_cfg.colSnow[0] = 1.0f; g_cfg.colSnow[1] = 1.0f; g_cfg.colSnow[2] = 1.0f; g_cfg.colSnow[3] = 0.75f;
            g_cfg.colMenuBg[0] = 0.055f; g_cfg.colMenuBg[1] = 0.058f; g_cfg.colMenuBg[2] = 0.068f; g_cfg.colMenuBg[3] = 1.0f;
        }
        ImGui::PopStyleVar();
        ImGui::Spacing();
        GS_Toggle("Auto-accept matchmaking", &g_cfg.autoAcceptMatch);
        GS_Toggle("In-game radar", &g_cfg.nativeRadar);
        GS_Toggle("Preserve kill-feed", &g_cfg.killfeedEnabled);
        ImGui::Spacing();
        DrawEnvironmentAudioCard();
        ImGui::Spacing();
        GS_SectionHeader("Descarregar");
        if (ImGui::Button("Unload cheat", ImVec2(ImGui::GetContentRegionAvail().x, 28.0f)))
            HazzureState::g_unloadRequested.store(true);
        HAZZURE_OBSERVE_CONTROL("Unload cheat", &HazzureState::g_unloadRequested);
    }
}

// ============================================================================
// Roteador de seções - mapeia cards legados para novas seções GameSense
// ============================================================================
static void DrawGSPanelSection(int tab, int section, int column, Shared* shared) {
    // Validar tab
    if (tab < 0 || tab >= 14) {
        GS_Log("[GUI] DrawGSPanelSection: tab inválido=%d", tab);
        return;
    }

    // Validar section
    int sectionCount = gs_mainTabSectionCounts[tab];
    if (section < 0 || section >= sectionCount) {
        GS_Log("[GUI] DrawGSPanelSection: section inválido=%d para tab=%d (max=%d)", section, tab, sectionCount - 1);
        return;
    }


    using namespace HazzureUI;

    switch (tab) {
        case 0: { // Legitbot
            DrawLegitbotMemeSense(column, shared);
            break;
        }
        case 2: { // Players
            DrawPlayersMemeSense(column, shared);
            break;
        }
        
        case 1: { // Rage
            if (section == 0) { // Aim
                if (column == 0) {
                    GS_SectionHeader("Rage Aim");
                    GS_Toggle("Enable", &g_cfg.rageActive);
                    GS_SliderFloat("FOV", &g_cfg.rageFov, 0.5f, 360.0f, "%.1f deg");
                    GS_SliderFloat("Smooth", &g_cfg.rageSmooth, 1.0f, 25.0f, "%.2f");
                    GS_Toggle("Auto-stop", &g_cfg.rageAutoStop);
                    GS_SliderFloat("Auto-stop velocidade", &g_cfg.rageAutoStopSpeed, 0.0f, 300.0f, "%.0f u/s");
                    GS_Toggle("Ignore Smoke", &g_cfg.rageIgnoreSmoke);
                    GS_Toggle("Silent Aim", &g_cfg.rageSilentAim);
                    GS_Toggle("Perfect Silent (Zero Snap)", &g_cfg.ragePerfectSilent);
                    GS_Toggle("Firing Crosshair (Swapchain)", &g_cfg.rageFiringCrosshair);
                    ImGui::TextWrapped("Perfect Silent envia o angulo vencedor exclusivamente no sub-tick (CSGOUserCmdPB), mantendo a camera local 100% estavel sem snap ou jitter. O Firing Crosshair desenha a reticula no impacto real com compensacao de punch.");
                    ImGui::Spacing();
                    GS_SectionHeader("Autowall (chega / nao chega)");
                    GS_Toggle("Enable##autowall", &g_cfg.autowallEnabled);
                    GS_SliderInt("Camadas maximas", &g_cfg.autowallMaxLayers, 1, 4, "%d");
                    GS_SliderFloat("Penalidade por camada", &g_cfg.autowallChancePenalty, 0.0f, 100.0f, "%.0f%%");
                    ImGui::TextWrapped("Autowall decide ALCANCE (a bala chega ou nao chega). O dano que sobra depois da parede nao e calculado.");
                    ImGui::Spacing();
                    GS_SectionHeader("Selecao de alvo");
                    GS_Combo("Osso Alvo", &g_cfg.aimBone, gs_combo_aimBone, 4);
                    const char* modes[] = { "Fixed bone", "Closest bone", "Head / visible fallback" };
                    GS_Combo("Selecao de Alvo", &g_cfg.aimTargetMode, modes, 3);
                    const char* safeModes[] = { "Desativado", "Preferir Safe Point", "Forcar Safe Point" };
                    GS_Combo("Safe Point", &g_cfg.safePointMode, safeModes, 3);
                    GS_ToggleWithKeybind("Force Body Aim (Baim)", &g_cfg.forceBodyAim, &g_cfg.forceBodyAimKey);
                    GS_Toggle("Baim on Peek", &g_cfg.baimOnPeek);
                    GS_Toggle("Extrapolacao por Ping", &g_cfg.aimExtrapolation);
                    if (g_cfg.aimExtrapolation)
                        GS_SliderFloat("Escala Extrapolacao", &g_cfg.aimExtrapolationScale, 0.1f, 3.0f, "%.2fx");
                } else {
                    ImGui::TextWrapped("Rage aim usa o FOV acima para escolher o alvo e o Smooth para closar (1 = instantaneo, o comportamento antigo). FOV 360 cobre tudo ao redor. Auto-stop exige velocidade abaixo do limite antes de travar o alvo. Disparar sozinho e o Auto-fire da aba Legitbot > Misc, ligado por ele e nao pelo rage. Com Autowall desligado mira atraves de parede nunca e permitida; ligado, o ponto so e aceito quando a sonda de camadas termina o ray no alvo, e isso decide apenas se a bala CHEGA, nunca quanto dano sobra depois da superficie.");
                }
            } else if (section == 1) { // Anti-Aim
                if (column == 0) {
                    GS_SectionHeader("Anti-Aim");
                    GS_Toggle("Enable", &g_cfg.rageAntiAim);
                    const char* aaModes[] = { "Estatico", "Jitter", "Spin" };
                    GS_Combo("Modo", &g_cfg.rageAntiAimMode, aaModes, 3);
                    GS_SliderFloat("Pitch (+89 = Chao/Guardado)", &g_cfg.rageAntiAimPitch, -89.0f, 89.0f, "%.0f deg");
                    if (g_cfg.rageAntiAimMode == 0)
                        GS_SliderFloat("Yaw Offset", &g_cfg.rageAntiAimYaw, -180.0f, 180.0f, "%.0f deg");
                    else if (g_cfg.rageAntiAimMode == 1) {
                        GS_SliderFloat("Jitter Min", &g_cfg.rageAntiAimJitterMin, -180.0f, 180.0f, "%.0f deg");
                        GS_SliderFloat("Jitter Max", &g_cfg.rageAntiAimJitterMax, -180.0f, 180.0f, "%.0f deg");
                    } else {
                        GS_SliderFloat("Spin Passo", &g_cfg.rageAntiAimSpinStep, 1.0f, 180.0f, "%.0f deg/cmd");
                    }
                    GS_Toggle("Only on ground", &g_cfg.rageAntiAimOnGroundOnly);
                    GS_Toggle("At-Targets (Olhar Oposto ao Alvo)", &g_cfg.rageAntiAimAtTargets);
                    GS_Toggle("Mouse Override Direcional", &g_cfg.rageAntiAimMouseOverride);
                    GS_Toggle("Anti-Aim Defensivo (Ar/Velocidade)", &g_cfg.rageAntiAimDefensive);
                    GS_Toggle("Pitch Invert", &g_cfg.rageAntiAimPitchInvert);
                    GS_Toggle("Desync (AA no sub-tick, sem snap)", &g_cfg.rageDesync);
                    if (g_cfg.rageDesync)
                        GS_SliderFloat("Fake Limit (desync)", &g_cfg.rageDesyncFakeLimit, 0.0f, 60.0f, "%.0f deg");
                    if (g_cfg.rageDesync)
                        GS_Toggle("Body Yaw (alterna lados)", &g_cfg.rageBodyYaw);
                    GS_Toggle("Resolver", &g_cfg.rageResolver);
                    GS_Toggle("Backtrack", &g_cfg.rageBacktrack);
                    if (g_cfg.rageBacktrack)
                        GS_SliderInt("BT Ticks", &g_cfg.rageBacktrackTicks, 1, 12);
                    GS_Toggle("Fake Lag", &g_cfg.rageFakeLag);
                    if (g_cfg.rageFakeLag)
                        GS_SliderInt("Choke Ticks", &g_cfg.rageFakeLagTicks, 1, 14);
                    GS_ToggleWithKeybind("Fake Duck", &g_cfg.fakeDuck, &g_cfg.fakeDuckKey);
                    ImGui::TextWrapped("Precedencia no mesmo comando: no tick em que ha disparo o angulo do tiro (Silent Aim) vence; nos ticks sem disparo o Anti-Aim escreve o comando. Com rage sem Silent Aim ligado e mira travada o Anti-Aim nao entra, porque ai a propria camera e a mira.");
                    const LONG aaCmdOffset=InterlockedCompareExchange(&g_antiAimCmdOffset,0,0);
                    ImGui::Text("Ultimo CreateMove: aplicado=%d tickTiro=%d via=%s cmdOff=0x%llX",
                        static_cast<int>(InterlockedCompareExchange(&g_antiAimAppliedLast,0,0)),
                        static_cast<int>(InterlockedCompareExchange(&g_shotTickLast,0,0)),
                        aaCmdOffset>=0?"CUserCmd":"estado vivo (busca campo)",
                        static_cast<unsigned long long>(aaCmdOffset<0?0:aaCmdOffset));
                    ImGui::Text("Angulo lido de volta no comando: %.2f/%.2f  ok=%d falha=%d fakeNoCmd=%d probe=%d/%d",
                        InterlockedCompareExchange(&g_antiAimCmdPitch,0,0)/100.0,
                        InterlockedCompareExchange(&g_antiAimCmdYaw,0,0)/100.0,
                        static_cast<int>(InterlockedCompareExchange(&g_antiAimCmdOk,0,0)),
                        static_cast<int>(InterlockedCompareExchange(&g_antiAimCmdFail,0,0)),
                        static_cast<int>(InterlockedCompareExchange(&g_antiAimCmdFakeHits,0,0)),
                        static_cast<int>(InterlockedCompareExchange(&g_antiAimCmdHits,0,0)),
                        static_cast<int>(InterlockedCompareExchange(&g_antiAimCmdRuns,0,0)));
                    ImGui::Text("Desync sub-tick: aplicado=%d real=%.2f enviado=%.2f ok=%d falha=%d",
                        static_cast<int>(InterlockedCompareExchange(&g_desyncAppliedLast,0,0)),
                        InterlockedCompareExchange(&g_desyncYawReal,0,0)/100.0,
                        InterlockedCompareExchange(&g_desyncYawSent,0,0)/100.0,
                        static_cast<int>(InterlockedCompareExchange(&g_desyncStampOk,0,0)),
                        static_cast<int>(InterlockedCompareExchange(&g_desyncStampFail,0,0)));
                    ImGui::Spacing();
                    GS_SectionHeader("Thirdperson (ver anti-aim)");
                    GS_ToggleWithKeybind("Thirdperson", &g_cfg.thirdpersonEnabled, &g_cfg.thirdpersonKey);
                    GS_SliderFloat("Distancia", &g_cfg.thirdpersonDistance, 30.0f, 300.0f, "%.0f u");
                } else {
                    ImGui::TextWrapped("Anti-aim escreve so no comando que vai ao servidor. Desync grava o angulo falso direto no sub-tick. Body Yaw alterna o lado do corpo a cada tick. Resolver estima o yaw real do inimigo por historico de angulos. Backtrack avalia posicoes anteriores do alvo (ring buffer). Fake Lag suprime comandos para desinc de posicao. Fake Duck oscila o crouch em 8 ticks.");
                }
            }
            break;
        }
        
        case 4: { // Independent agent customization
            if(column==0)DrawSettingsCard16(shared);
            break;
        }
        case 5: { // Collection entry, no direct cosmetic application
            if(column==0)ImGui::TextWrapped("Crie e edite armas, facas e luvas na aba Inventory.");
            break;
        }
        case 6: { // Visuals
            DrawVisualsMemeSense(column, shared);
            break;
        }
        case 7: { // World
            DrawWorldMemeSense(column, shared);
            break;
        }
        case 8: { // View
            if (section == 0) { // Viewmodel
                if (column == 0) {
                    GS_SectionHeader("Viewmodel Offset & FOV");
                    GS_Toggle("Custom Viewmodel", &g_cfg.customViewmodel);
                    GS_SliderFloat("X (Eixo Horizontal)", &g_cfg.viewmodelX, -10.0f, 10.0f, "%.2f");
                    GS_SliderFloat("Y (Profundidade)", &g_cfg.viewmodelY, -10.0f, 10.0f, "%.2f");
                    GS_SliderFloat("Z (Altura)", &g_cfg.viewmodelZ, -10.0f, 10.0f, "%.2f");
                    GS_SliderFloat("FOV da Arma", &g_cfg.viewmodelFov, 40.0f, 120.0f, "%.1f deg");
                } else if (column == 1) {
                    GS_SectionHeader("Proporcao de Tela");
                    GS_Toggle("Forcar Proporcao (Aspect)", &g_cfg.aspectRatioOverride);
                    GS_SliderFloat("Proporcao", &g_cfg.aspectRatio, 0.5f, 2.5f, "%.3f");
                    if (ImGui::Button("4:3")) g_cfg.aspectRatio = 4.f / 3.f; ImGui::SameLine();
                    if (ImGui::Button("16:9")) g_cfg.aspectRatio = 16.f / 9.f; ImGui::SameLine();
                    if (ImGui::Button("16:10")) g_cfg.aspectRatio = 16.f / 10.f; ImGui::SameLine();
                    if (ImGui::Button("5:4")) g_cfg.aspectRatio = 5.f / 4.f;
                }
            } else if (section == 1) { // Camera (World FOV)
                if (column == 0) {
                    GS_SectionHeader("Camera (World FOV)");
                    GS_Toggle("Alterar FOV da Camera", &g_cfg.cameraFovOverride);
                    GS_SliderFloat("FOV Geral (Camera)", &g_cfg.cameraFov, 60.0f, 140.0f, "%.0f deg");
                    ImGui::TextWrapped("Campo de visao do mundo. O zoom das snipers continua nativo.");
                    if (g_cfg.cameraFovOverride) {
                        ImGui::TextWrapped("%s", HazzureCamera::Label(GetTickCount64()));
                    }
                }
            }
            break;
        }
        case 9: { // Indicators
            if (section == 0) { // Existing overlay visibility
                if (column == 0) {
                    GS_SectionHeader("Overlays");
                    GS_Toggle("Telemetry HUD", &g_cfg.showTelemetryHUD);
                    GS_Toggle("Keybinds Overlay", &g_cfg.showKeybindsOverlay);
                }
            } else if (section == 1) { // Font Size
                if (column == 0) {
                    GS_SectionHeader("Font Size");
                    GS_SliderFloat("UI Font", &g_cfg.uiFontSize, 12.0f, 18.0f, "%.1f");
                    GS_SliderFloat("ESP Font", &g_cfg.espFontSize, 11.0f, 18.0f, "%.1f");
                }
            }
            break;
        }
        case 10: { // Miscellaneous (Estilo MemeSense fiel ao print do usuario)
            DrawMiscellaneousMemeSense(column, shared);
            break;
        }
        case 11: { // Inventory
            // DrawLocalInventory agora gerencia todas as sub-tabs internamente (Profile, Equipment, Containers, Tools, Graphic Art)
            if (column == 0) DrawLocalInventory(shared);
            break;
        }
        case 12: { // Grenade Helper
            if (section == 0) { // Lineups & Helper
                if (column == 0) { uintptr_t clientModule = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll")); DrawLineupControls(clientModule); }
            } else if (section == 1) { // Prediction
                if (column == 0) {
                    GS_SectionHeader("Grenade Prediction");
                    GS_Toggle("Enabled", &g_cfg.showGrenadeTrajectory);
                    GS_Toggle("Trajectory Glow", &g_cfg.trajectoryGlow);
                    GS_SliderFloat("Width", &g_cfg.trajectoryWidth, 0.5f, 5.0f, "%.1f");
                }
            } else if (section == 2) { // Smoke Timer
                if (column == 0) {
                    GS_SectionHeader("Smoke Timer");
                    GS_Toggle("Enabled", &g_cfg.strictSmoke);
                    GS_Toggle("Smoke Glow", &g_cfg.smokeGlowEnabled);
                    GS_ColorRow("Smoke Color", g_cfg.smokeGlowColor);
                }
            } else if (section == 3) { // Incendiary
                if (column == 0) {
                    GS_SectionHeader("Incendiary Timer");
                    GS_Toggle("Fire Glow", &g_cfg.fireGlowEnabled);
                    GS_ColorRow("Fire Color", g_cfg.fireGlowColor);
                }
            } else if (section == 4) { // Tracers
                if (column == 0) {
                    GS_SectionHeader("Grenade Tracers");
                    GS_Toggle("Enabled", &g_cfg.tracerGlow);
                    GS_SliderFloat("Width", &g_cfg.tracerWidth, 0.5f, 5.0f, "%.1f");
                    GS_ColorRow("Color", g_cfg.colTracer);
                }
            }
            break;
        }
        case 13: { // Configs
            if (section == 0) { // General
                if (column == 0) DrawSettingsCard19(shared);
            } else if (section == 1) { // Profiles
                if (column == 0) DrawSettingsCard18(shared);
            } else if (section == 2) { // Diagnostics
                if (column == 0) DrawProfilesDiagnosticsCard();
                else DrawProfilesTelemetryCard(shared);
            }
            break;
        }
    }
} // DrawGSPanelSection

// ============================================================================
// Panel Principal - DrawGSPanel()
// ============================================================================

// stage: 3 | reason: painéis EN para abas não-Aim — Ragebot, Accuracy, Anti-aim, Legitbot, Visuals, Misc
// Skins mantém DrawLocalInventory (complexo demais p/ reescrever sem ganho).

static void EnigmaColorRow(const char* label, const char* id, float col[4], float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 tsz = ImGui::CalcTextSize(label);
    d->AddText(ImVec2(p.x, p.y + (38.f - tsz.y) * .5f), GSWidgets::IM_COL(EN_TEXT), label);
    ImGui::Dummy(ImVec2(panelW, 38.f));
    ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 80.f, p.y + 7.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
    ImGui::SetNextItemWidth(80.f);
    ImGui::ColorEdit4(id, col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf);
    ImGui::PopStyleVar();
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

static void EnigmaSliderIntRow(const char* label, int* v, int low, int high, const char* fmt, float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float fv = static_cast<float>(*v);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 6.f));
    GSWidgets::GS_EnSlider(label, &fv, static_cast<float>(low), static_cast<float>(high), fmt);
    *v = static_cast<int>(fv);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

static void EnigmaKeybindRow(const char* label, int* key, float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 tsz = ImGui::CalcTextSize(label);
    d->AddText(ImVec2(p.x, p.y + (38.f - tsz.y) * .5f), GSWidgets::IM_COL(EN_TEXT), label);
    ImGui::Dummy(ImVec2(panelW, 38.f));
    ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 60.f, p.y + 10.f));
    GSWidgets::GS_EnChip(label, key);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

static void EnigmaToggleKeybindRow(const char* label, const char* id, bool* v, int* key, float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 tsz = ImGui::CalcTextSize(label);
    d->AddText(ImVec2(p.x, p.y + (38.f - tsz.y) * .5f), GSWidgets::IM_COL(EN_TEXT), label);
    ImGui::Dummy(ImVec2(panelW, 38.f));
    ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 34.f, p.y + 10.f));
    GSWidgets::GS_EnToggle(id, v);
    ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 34.f - 8.f - 50.f, p.y + 10.f));
    GSWidgets::GS_EnChip(label, key);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

// --- Ragebot (s_enigmaTab == 1) ---
inline void DrawEnigmaRagebotPanel(float panelW) {
    if (GSWidgets::GS_EnSectionBar("Rage Aim", panelW, "##enRageReset")) {
        g_cfg.rageActive = false; g_cfg.rageFov = 5.f; g_cfg.rageSmooth = 1.f;
    }
    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 6.f));

    EnigmaToggleRow("Enable", "##enRageEn", &g_cfg.rageActive, panelW);
    const bool dim = !g_cfg.rageActive;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dim ? .45f : 1.f);
    ImGui::BeginDisabled(dim);

    EnigmaSliderRow("FOV", &g_cfg.rageFov, 0.5f, 360.f, "%.1f\xC2\xB0", panelW);
    EnigmaSliderRow("Smooth", &g_cfg.rageSmooth, 1.f, 25.f, "%.2f", panelW);
    EnigmaToggleRow("Auto-stop", "##enAutoStop", &g_cfg.rageAutoStop, panelW);
    if (g_cfg.rageAutoStop)
        EnigmaSliderRow("Auto-stop speed", &g_cfg.rageAutoStopSpeed, 0.f, 300.f, "%.0f u/s", panelW);
    EnigmaToggleRow("Ignore Smoke", "##enIgnSmk", &g_cfg.rageIgnoreSmoke, panelW);
    EnigmaToggleRow("Silent Aim", "##enSilent", &g_cfg.rageSilentAim, panelW);
    EnigmaToggleRow("Perfect Silent", "##enPSilent", &g_cfg.ragePerfectSilent, panelW);
    EnigmaToggleRow("Firing Crosshair", "##enFCross", &g_cfg.rageFiringCrosshair, panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Autowall", panelW, "##enAWReset")) {
        g_cfg.autowallEnabled = false; g_cfg.autowallMaxLayers = 2; g_cfg.autowallChancePenalty = 30.f;
    }
    EnigmaToggleRow("Enable##aw", "##enAWEn", &g_cfg.autowallEnabled, panelW);
    if (g_cfg.autowallEnabled) {
        EnigmaSliderIntRow("Max layers", &g_cfg.autowallMaxLayers, 1, 4, "%d", panelW);
        EnigmaSliderRow("Penalty per layer", &g_cfg.autowallChancePenalty, 0.f, 100.f, "%.0f%%", panelW);
    }

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Target Selection", panelW, "##enTgtReset")) {
        g_cfg.aimBone = 0; g_cfg.aimTargetMode = 0; g_cfg.safePointMode = 0;
    }
    static const char* kBones[] = { "Head", "Neck", "Chest", "Pelvis" };
    EnigmaComboRow("Aim bone", &g_cfg.aimBone, kBones, 4, panelW);
    static const char* kTargetModes[] = { "Fixed bone", "Closest bone", "Head / visible fallback" };
    EnigmaComboRow("Target selection", &g_cfg.aimTargetMode, kTargetModes, 3, panelW);
    static const char* kSafeModes[] = { "Off", "Prefer", "Force" };
    EnigmaComboRow("Safe Point", &g_cfg.safePointMode, kSafeModes, 3, panelW);
    EnigmaToggleKeybindRow("Force Body Aim", "##enBaim", &g_cfg.forceBodyAim, &g_cfg.forceBodyAimKey, panelW);
    EnigmaToggleRow("Baim on Peek", "##enBaimPeek", &g_cfg.baimOnPeek, panelW);
    EnigmaToggleRow("Ping Extrapolation", "##enExtrap", &g_cfg.aimExtrapolation, panelW);
    if (g_cfg.aimExtrapolation)
        EnigmaSliderRow("Extrapolation scale", &g_cfg.aimExtrapolationScale, 0.1f, 3.f, "%.2fx", panelW);

    ImGui::EndDisabled();
    ImGui::PopStyleVar();
}

// --- Accuracy (s_enigmaTab == 0, aimSub == 1) ---
inline void DrawEnigmaAccuracyPanel(float panelW) {
    if (GSWidgets::GS_EnSectionBar("Precision & Points", panelW, "##enAccReset")) {
        g_cfg.hitchanceEnabled = true; g_cfg.hitchancePercent = 60.f; g_cfg.hitchanceSamples = 128;
        g_cfg.aimMultipoint = false; g_cfg.multipointScale = 0.f;
    }
    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 6.f));

    EnigmaToggleRow("Hitchance filter", "##enHC", &g_cfg.hitchanceEnabled, panelW);
    EnigmaSliderRow("Min. hit chance", &g_cfg.hitchancePercent, 0.f, 100.f, "%.0f%%", panelW);
    static const char* kSamples[] = { "64 rays", "128 rays", "256 rays" };
    int quality = g_cfg.hitchanceSamples <= 64 ? 0 : g_cfg.hitchanceSamples <= 128 ? 1 : 2;
    EnigmaComboRow("Samples", &quality, kSamples, 3, panelW);
    g_cfg.hitchanceSamples = quality == 0 ? 64 : quality == 1 ? 128 : 256;

    EnigmaToggleRow("Multipoint", "##enMP", &g_cfg.aimMultipoint, panelW);
    if (g_cfg.aimMultipoint)
        EnigmaSliderRow("Point scale", &g_cfg.multipointScale, 0.f, .8f, "%.2f", panelW);

    EnigmaDivider(panelW);
    EnigmaToggleRow("Prefer body if head uncertain", "##enBodyFB", &g_cfg.aimBodyFallback, panelW);
    if (g_cfg.aimBodyFallback)
        EnigmaSliderRow("Head threshold", &g_cfg.bodyFallbackBelow, 0.f, 100.f, "%.0f%%", panelW);
    EnigmaToggleKeybindRow("Force Body Aim", "##enAccBaim", &g_cfg.forceBodyAim, &g_cfg.forceBodyAimKey, panelW);
    EnigmaToggleRow("Baim on Peek", "##enAccBaimPk", &g_cfg.baimOnPeek, panelW);
    static const char* kSafe[] = { "Off", "Prefer Safe Point", "Force Safe Point" };
    EnigmaComboRow("Safe Point", &g_cfg.safePointMode, kSafe, 3, panelW);
    EnigmaToggleRow("Ping Extrapolation", "##enAccExtrap", &g_cfg.aimExtrapolation, panelW);
    if (g_cfg.aimExtrapolation)
        EnigmaSliderRow("Extrapolation scale", &g_cfg.aimExtrapolationScale, 0.1f, 3.f, "%.2fx", panelW);

    if (g_frameDiagnostics.hitchance >= 0)
        ImGui::TextDisabled("Last estimate: %.1f%%", g_frameDiagnostics.hitchance);
}

// --- Anti-aim (s_enigmaTab == 0, aimSub == 2) ---
inline void DrawEnigmaAntiAimPanel(float panelW) {
    static const char* kAAModes[] = { "Static", "Jitter", "Spin" };
    if (GSWidgets::GS_EnSectionBar("Anti-Aim", panelW, "##enAAReset")) {
        g_cfg.rageAntiAim = false; g_cfg.rageAntiAimMode = 0; g_cfg.rageAntiAimPitch = 0.f;
    }
    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 6.f));

    EnigmaToggleRow("Enable", "##enAAEn", &g_cfg.rageAntiAim, panelW);
    const bool dim = !g_cfg.rageAntiAim;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dim ? .45f : 1.f);
    ImGui::BeginDisabled(dim);

    EnigmaComboRow("Mode", &g_cfg.rageAntiAimMode, kAAModes, 3, panelW);
    EnigmaSliderRow("Pitch", &g_cfg.rageAntiAimPitch, -89.f, 89.f, "%.0f\xC2\xB0", panelW);

    if (g_cfg.rageAntiAimMode == 0)
        EnigmaSliderRow("Yaw Offset", &g_cfg.rageAntiAimYaw, -180.f, 180.f, "%.0f\xC2\xB0", panelW);
    else if (g_cfg.rageAntiAimMode == 1) {
        EnigmaSliderRow("Jitter Min", &g_cfg.rageAntiAimJitterMin, -180.f, 180.f, "%.0f\xC2\xB0", panelW);
        EnigmaSliderRow("Jitter Max", &g_cfg.rageAntiAimJitterMax, -180.f, 180.f, "%.0f\xC2\xB0", panelW);
    } else
        EnigmaSliderRow("Spin Step", &g_cfg.rageAntiAimSpinStep, 1.f, 45.f, "%.0f\xC2\xB0", panelW);

    EnigmaToggleRow("Only on ground", "##enAAGnd", &g_cfg.rageAntiAimOnGroundOnly, panelW);
    EnigmaToggleRow("At-Targets", "##enAAAtTgt", &g_cfg.rageAntiAimAtTargets, panelW);
    EnigmaToggleRow("Mouse Override", "##enAAMouse", &g_cfg.rageAntiAimMouseOverride, panelW);
    EnigmaToggleRow("Defensive AA", "##enAADef", &g_cfg.rageAntiAimDefensive, panelW);
    EnigmaToggleRow("Pitch Invert", "##enAAPInv", &g_cfg.rageAntiAimPitchInvert, panelW);

    EnigmaDivider(panelW);
    EnigmaToggleRow("Desync", "##enDesync", &g_cfg.rageDesync, panelW);
    if (g_cfg.rageDesync) {
        EnigmaSliderRow("Fake Limit", &g_cfg.rageDesyncFakeLimit, 0.f, 60.f, "%.0f\xC2\xB0", panelW);
        EnigmaToggleRow("Body Yaw", "##enBodyYaw", &g_cfg.rageBodyYaw, panelW);
    }
    EnigmaToggleRow("Resolver", "##enResolver", &g_cfg.rageResolver, panelW);

    EnigmaDivider(panelW);
    EnigmaToggleRow("Backtrack", "##enAABT", &g_cfg.rageBacktrack, panelW);
    if (g_cfg.rageBacktrack)
        EnigmaSliderIntRow("BT Ticks", &g_cfg.rageBacktrackTicks, 1, 12, "%d", panelW);
    EnigmaToggleRow("Fake Lag", "##enFakeLag", &g_cfg.rageFakeLag, panelW);
    if (g_cfg.rageFakeLag)
        EnigmaSliderIntRow("Choke Ticks", &g_cfg.rageFakeLagTicks, 1, 14, "%d", panelW);
    EnigmaToggleKeybindRow("Fake Duck", "##enFDuck", &g_cfg.fakeDuck, &g_cfg.fakeDuckKey, panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Thirdperson", panelW, "##enTPReset")) {
        g_cfg.thirdpersonEnabled = false; g_cfg.thirdpersonDistance = 120.f;
    }
    EnigmaToggleKeybindRow("Thirdperson", "##enTP", &g_cfg.thirdpersonEnabled, &g_cfg.thirdpersonKey, panelW);
    EnigmaSliderRow("Distance", &g_cfg.thirdpersonDistance, 30.f, 300.f, "%.0f u", panelW);

    ImGui::EndDisabled();
    ImGui::PopStyleVar();
}

// --- Legitbot (s_enigmaTab == 2) ---
inline void DrawEnigmaLegitbotPanel(float panelW) {
    static int s_enLegitSub = 0;
    if (GSWidgets::GS_EnSectionBar("Legitbot", panelW, "##enLegitReset")) {
        g_cfg.aimActive = false; g_cfg.aimFov = 2.f; g_cfg.aimSmooth = 5.f;
    }
    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 6.f));

    EnigmaToggleRow("Enable", "##enLegitEn", &g_cfg.aimActive, panelW);
    EnigmaKeybindRow("Aim key", &g_cfg.aimKey, panelW);
    EnigmaToggleRow("Draw FOV", "##enLegitFov", &g_cfg.espFovCircle, panelW);
    EnigmaSliderIntRow("Aim time", &g_cfg.aimReactionMinMs, 0, 1000, "%d ms", panelW);

    // sub-tabs Aim | Accuracy | RCS | Misc
    ImGui::Dummy(ImVec2(panelW, 4.f));
    static const char* kSubs[] = { "Aim", "Accuracy", "RCS", "Misc" };
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ImGui::SameLine(0, 12.f);
        bool sel = (s_enLegitSub == i);
        ImGui::PushStyleColor(ImGuiCol_Text, sel ? GSWidgets::IM_COL(EN_TEXT) : GSWidgets::IM_COL(EN_TEXT_DIM));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255,255,255,10));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255,255,255,20));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushID(i);
        if (ImGui::Button(kSubs[i])) s_enLegitSub = i;
        if (sel) {
            ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(mn.x, mx.y+1.f), ImVec2(mx.x, mx.y+1.f), GSWidgets::IM_COL(EN_ACCENT), 1.5f);
        }
        ImGui::PopID();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }
    ImGui::Dummy(ImVec2(panelW, 6.f));
    EnigmaDivider(panelW);

    const bool dim = !g_cfg.aimActive;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dim ? .45f : 1.f);
    ImGui::BeginDisabled(dim);

    if (s_enLegitSub == 0) { // Aim
        static const char* kRadType[] = { "Static", "Dynamic / Distance" };
        int radType = g_cfg.aimDistanceFov ? 1 : 0;
        EnigmaComboRow("Radius type", &radType, kRadType, 2, panelW);
        g_cfg.aimDistanceFov = (radType == 1);
        EnigmaSliderRow("Radius", &g_cfg.aimFov, 0.1f, 25.f, "%.3f\xC2\xB0", panelW);
        EnigmaSliderRow("Multiplier", &g_cfg.aimEaseBias, 1.f, 10.f, "%.2f", panelW);
        static const char* kSmType[] = { "Static", "Dynamic" };
        int smType = g_cfg.aimEase ? 1 : 0;
        EnigmaComboRow("Smooth type", &smType, kSmType, 2, panelW);
        g_cfg.aimEase = (smType == 1);
        EnigmaSliderRow("Smooth pitch", &g_cfg.aimSmooth, 1.f, 25.f, "%.3f", panelW);
        EnigmaSliderRow("Smooth yaw", &g_cfg.aimSmooth, 1.f, 25.f, "%.3f", panelW);
    } else if (s_enLegitSub == 1) { // Accuracy
        EnigmaSliderRow("Hit chance", &g_cfg.hitchancePercent, 0.f, 100.f, "%.0f%%", panelW);
        EnigmaToggleRow("Multipoint", "##enLMP", &g_cfg.aimMultipoint, panelW);
        if (g_cfg.aimMultipoint)
            EnigmaSliderRow("Point scale", &g_cfg.multipointScale, 0.f, .8f, "%.2f", panelW);
        EnigmaToggleRow("Body fallback", "##enLBF", &g_cfg.aimBodyFallback, panelW);
    } else if (s_enLegitSub == 2) { // RCS
        DrawCombatRecoilCard();
    } else { // Misc
        EnigmaToggleRow("Auto-Scope AWP/Scout", "##enLAutoScope", &g_cfg.autoScope, panelW);
        EnigmaToggleRow("Visible Only", "##enLVisOnly", &g_cfg.aimVisibleOnly, panelW);
        EnigmaSliderIntRow("After kill delay", &g_cfg.aimKillDelayMs, 0, 1000, "%d ms", panelW);
        EnigmaSliderIntRow("Target switch delay", &g_cfg.aimSwitchDelayMs, 0, 1000, "%d ms", panelW);
    }

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Hit Groups", panelW, "##enLHGReset")) {
        g_cfg.aimBone = 0; g_cfg.forceBodyAim = false;
    }
    static const char* kBones[] = { "Head", "Neck", "Chest", "Pelvis" };
    EnigmaComboRow("Selection", &g_cfg.aimBone, kBones, 4, panelW);
    EnigmaToggleKeybindRow("Only body aim", "##enLOnlyBA", &g_cfg.forceBodyAim, &g_cfg.forceBodyAimKey, panelW);
    EnigmaToggleRow("Body aim if lethal", "##enLBLethal", &g_cfg.baimOnPeek, panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Auto Fire", panelW, "##enLAFReset")) {
        g_cfg.autoFire = false; g_cfg.shotStop = false;
    }
    EnigmaToggleRow("Enable", "##enLAFEn", &g_cfg.autoFire, panelW);
    EnigmaToggleRow("Auto stop", "##enLAFStop", &g_cfg.shotStop, panelW);
    static const char* kHCType[] = { "Strict", "Balanced" };
    int hcIdx = g_cfg.hitchanceEnabled ? 0 : 1;
    EnigmaComboRow("Hit chance type", &hcIdx, kHCType, 2, panelW);
    g_cfg.hitchanceEnabled = (hcIdx == 0);
    EnigmaSliderRow("Min hit chance", &g_cfg.hitchancePercent, 0.f, 100.f, "%.0f%%", panelW);
    { float v = g_cfg.triggerDelayMs; EnigmaSliderRow("Reaction time", &v, 0.f, 500.f, "%.0f ms", panelW); g_cfg.triggerDelayMs = v; }
    EnigmaSliderIntRow("Delay between shots", &g_cfg.triggerPauseMs, 0, 1000, "%d ms", panelW);
    EnigmaSliderIntRow("Ghost time", &g_cfg.poseHistoryMs, 0, 200, "%d ms", panelW);

    ImGui::EndDisabled();
    ImGui::PopStyleVar();
}

// --- Visuals (s_enigmaTab == 3) ---
inline void DrawEnigmaVisualsPanel(float panelW) {
    if (GSWidgets::GS_EnSectionBar("Grenades", panelW, "##enGrenReset")) {
        g_cfg.showGrenadeTrajectory = false; g_cfg.grenadeZones = false; g_cfg.grenadeProjectiles = false;
    }
    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 6.f));

    EnigmaToggleRow("Grenade prediction", "##enGPred", &g_cfg.showGrenadeTrajectory, panelW);
    EnigmaColorRow("Trajectory color", "##enColTraj", g_cfg.colTrajectory, panelW);
    EnigmaColorRow("Collision color", "##enColColl", g_cfg.colTrajectoryCollision, panelW);
    EnigmaToggleRow("Smoke timer", "##enSmkTimer", &g_cfg.grenadeZones, panelW);
    EnigmaColorRow("Smoke color", "##enColSmk", g_cfg.colSmoke, panelW);
    EnigmaColorRow("Incendiary color", "##enColMol", g_cfg.colMolotov, panelW);
    EnigmaToggleRow("Grenade tracers", "##enGTrace", &g_cfg.grenadeProjectiles, panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("HUD", panelW, "##enHUDReset")) {
        g_cfg.showBombTimer = false; g_cfg.showSpectators = false; g_cfg.showHitmarker = false;
    }
    EnigmaToggleRow("Bomb timer", "##enBomb", &g_cfg.showBombTimer, panelW);
    EnigmaToggleRow("Spectator list", "##enSpec", &g_cfg.showSpectators, panelW);
    EnigmaToggleRow("Hit marker", "##enHitMk", &g_cfg.showHitmarker, panelW);
    EnigmaToggleRow("Textured radar", "##enRadar", &g_cfg.nativeRadar, panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("World", panelW, "##enWorldReset")) {
        g_cfg.skyActive = false; g_cfg.darkWorld = false; g_cfg.cleanMap = false;
    }
    EnigmaToggleRow("Skybox", "##enSky", &g_cfg.skyActive, panelW);
    EnigmaColorRow("Skybox color", "##enColSky", g_cfg.colSky, panelW);
    EnigmaToggleRow("Lightning", "##enDark", &g_cfg.darkWorld, panelW);
    EnigmaToggleRow("World textures", "##enClean", &g_cfg.cleanMap, panelW);
    EnigmaColorRow("World fog color", "##enColFog", g_cfg.colFog, panelW);
    EnigmaSliderRow("Exposure", &g_cfg.worldBrightness, 0.05f, 2.f, "%.3f", panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Crosshair & Flash", panelW, "##enCHReset")) {
        g_cfg.forceSniperCrosshair = false; g_cfg.flashAlphaLimit = 1.f;
    }
    EnigmaToggleRow("Force sniper crosshair", "##enSnCH", &g_cfg.forceSniperCrosshair, panelW);
    EnigmaToggleRow("Penetration crosshair", "##enPenCH", &g_cfg.penetrationCrosshair, panelW);
    EnigmaSliderRow("Flash scale", &g_cfg.flashAlphaLimit, 0.f, 1.f, "%.2f", panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Removals", panelW, "##enRemReset")) {
        g_cfg.noScopeOverlay = false; g_cfg.noVisualRecoil = false;
    }
    EnigmaToggleRow("Shadows", "##enRemShad", &g_cfg.cleanMap, panelW);
    EnigmaToggleRow("Zoom HUD", "##enRemZoom", &g_cfg.noScopeOverlay, panelW);
    EnigmaToggleRow("Zoom spread", "##enRemSpread", &g_cfg.showSpreadCircle, panelW);
    EnigmaToggleRow("Visual recoil", "##enRemRecoil", &g_cfg.noVisualRecoil, panelW);
    EnigmaToggleRow("Remove weather", "##enRemWx", &g_cfg.removeOriginalWeather, panelW);
}

// --- Misc (s_enigmaTab == 4) ---
inline void DrawEnigmaMiscPanel(float panelW) {
    // Movement
    if (GSWidgets::GS_EnSectionBar("Movement", panelW, "##enMovReset")) {
        g_cfg.bhopActive = false;
    }
    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 6.f));

    EnigmaToggleRow("BunnyHop", "##enBhop", &g_cfg.bhopActive, panelW);
    if (g_cfg.bhopActive) {
        static const char* kBhopMode[] = { "Hold", "Toggle", "Always" };
        EnigmaComboRow("Mode", &g_cfg.bhopMode, kBhopMode, 3, panelW);
        if (g_cfg.bhopMode != 2) EnigmaKeybindRow("Bhop key", &g_cfg.bhopKey, panelW);
        { float v=(float)g_cfg.bhopStrafeGain; EnigmaSliderRow("Strafe Gain %", &v, 0.f, 100.f, "%.0f", panelW); g_cfg.bhopStrafeGain=(int)v; }
        EnigmaSliderRow("Angle Step", &g_cfg.bhopAngleStep, 45.f, 90.f, "%.1f", panelW);
        EnigmaToggleRow("Auto Edge-Jump", "##enEdge", &g_cfg.bhopAutoEdgeJump, panelW);
        EnigmaToggleRow("Auto JumpBug", "##enJBug", &g_cfg.bhopAutoJumpBug, panelW);
        EnigmaToggleRow("Prestrafe Align", "##enPre", &g_cfg.bhopPrestrafeAlign, panelW);
        EnigmaToggleRow("Preserve on land", "##enPres", &g_cfg.bhopPreserveOnLand, panelW);
        EnigmaToggleRow("Only moving", "##enOnlyMov", &g_cfg.bhopOnlyMoving, panelW);
        EnigmaSliderRow("Min speed gate", &g_cfg.bhopMinSpeedGate, 0.f, 100.f, "%.0f", panelW);
    }

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Automation", panelW, "##enAutoReset")) {
        g_cfg.jumpthrowActive = false; g_cfg.autoPistol = false; g_cfg.antiAfk = false;
    }
    EnigmaToggleRow("Jumpthrow", "##enJT", &g_cfg.jumpthrowActive, panelW);
    if (g_cfg.jumpthrowActive) EnigmaKeybindRow("Jumpthrow key", &g_cfg.jumpthrowKey, panelW);
    EnigmaToggleRow("Auto grenade release", "##enAutoGren", &g_cfg.autoGrenadeRelease, panelW);
    EnigmaToggleRow("Auto-Pistol", "##enAutoPistol", &g_cfg.autoPistol, panelW);
    EnigmaToggleRow("Anti-AFK", "##enAntiAFK", &g_cfg.antiAfk, panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Interface", panelW, "##enIfcReset")) {
        g_cfg.menuSnow = false; g_cfg.autoAcceptMatch = false;
    }
    EnigmaToggleRow("Menu Snow", "##enSnow", &g_cfg.menuSnow, panelW);
    EnigmaColorRow("Snow color", "##enColSnow", g_cfg.colSnow, panelW);
    EnigmaColorRow("Panel accent", "##enColAccent", g_cfg.colMenuAccent, panelW);
    EnigmaColorRow("Panel background", "##enColBg", g_cfg.colMenuBg, panelW);
    EnigmaToggleRow("Auto-accept match", "##enAutoMatch", &g_cfg.autoAcceptMatch, panelW);
    EnigmaToggleRow("In-game radar", "##enIGRadar", &g_cfg.nativeRadar, panelW);
    EnigmaToggleRow("Preserve kill-feed", "##enKillFeed", &g_cfg.killfeedEnabled, panelW);

    EnigmaDivider(panelW);
    if (GSWidgets::GS_EnSectionBar("Audio", panelW, "##enAudioReset")) {
        g_cfg.proceduralAudio = false; g_cfg.hitSound = false; g_cfg.killSound = false;
    }
    EnigmaToggleRow("Procedural Audio", "##enProcAudio", &g_cfg.proceduralAudio, panelW);
    EnigmaToggleRow("Hit Sound", "##enHitSnd", &g_cfg.hitSound, panelW);
    EnigmaToggleRow("Kill Sound", "##enKillSnd", &g_cfg.killSound, panelW);
    EnigmaToggleRow("Hitmarker", "##enMiscHitMk", &g_cfg.showHitmarker, panelW);
    EnigmaToggleRow("C4 Beep", "##enC4Beep", &g_cfg.bombBeepAudio, panelW);
    EnigmaSliderRow("Volume", &g_cfg.soundVolume, 0.f, 1.f, "%.2f", panelW);

    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 8.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    ImGui::PushStyleColor(ImGuiCol_Button, GSWidgets::IM_COL(EN_ACCENT));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GSWidgets::IM_COL(EN_ACCENT));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GSWidgets::IM_COL(EN_ACCENT));
    if (ImGui::Button("Unload cheat", ImVec2(panelW, 32.f)))
        HazzureState::g_unloadRequested.store(true);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
}

// ============================================================================
// T073 — Shell Enigma v3 (replica da GUI da foto): janela 1280x735, pagina
// #0B0B0C com padding 12, sidebar fixa 250px, card principal #1A1A1D r10 com
// topline 36 + weapon bar 36 + corpo (config <=510 | preview 440). As outras
// abas da sidebar roteiam para o conteudo legado existente.
// ============================================================================
static void DrawEnigmaPanel(Shared* shared, bool* pOpen, int screenW, int screenH) {
    if (!pOpen || !*pOpen || screenW < 240 || screenH < 200) { g_bindingTarget = nullptr; return; }
    HazzureUI::ValidateNavigationIndices();

    const float winW = std::min(1280.f, float(screenW));
    const float winH = std::min(735.f, float(screenH));
    ImGui::SetNextWindowSize({ winW, winH }, ImGuiCond_Always);
    ImGui::SetNextWindowPos({ (screenW - winW) * 0.5f, (screenH - winH) * 0.5f }, ImGuiCond_FirstUseEver);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL(EN_PAGE));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL(EN_BORDER));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL(EN_INSET));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL(EN_HOVER));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL(EN_ACCENT_DARK));
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL(EN_SECTION));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL(EN_INSET));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL(EN_CARD_INNER));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL(EN_TEXT));
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, IM_COL(EN_TEXT_DIM));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL(EN_ACCENT));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL(EN_ACCENT));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL(EN_ACCENT_SAVE_HOVER));
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, IM_COL(EN_ACCENT));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8, 6 });

    auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("##HazzureEnigma", pOpen, flags)) {
        ImDrawList* d = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        d->AddRectFilled(wp, ImVec2(wp.x + winW, wp.y + winH), IM_COL(EN_PAGE), 3.0f);

        static int s_enigmaTab = 0; // 0 Aimbot, 1 Ragebot, 2 Legitbot, 3 Visuals, 4 Misc, 5 Skins
        static int s_aimSub = 0;    // 0 Aim, 1 Accuracy, 2 Anti-aim (sub-abas da aba Aimbot)
        static bool s_bodyOpen = true;
        static bool s_cfgPopup = false;

        // Grid: pagina com padding 12.
        const float pad = 12.f;
        const float contentX = wp.x + pad, contentY = wp.y + pad;
        const float contentW = winW - pad * 2.f, contentH = winH - pad * 2.f;

        // ---------- Sidebar 250px ----------
        const float sideW = 250.f;
        const float sx = contentX, sy = contentY;
        // Barra superior 32px: hamburger + divisor vertical + "TABS / LEFT".
        {
            GSWidgets::GS_NavIcon(d, 17, ImVec2(sx + 8.f, sy + 8.f), IM_COL(EN_TEXT));
            d->AddLine(ImVec2(sx + 34.f, sy + 6.f), ImVec2(sx + 34.f, sy + 26.f), IM_COL(EN_DIVIDER), 1.f);
            d->AddText(ImVec2(sx + 44.f, sy + 8.f), IM_COL(EN_TEXT_MUTE), "TABS / LEFT");
            d->AddLine(ImVec2(sx, sy + 32.5f), ImVec2(sx + sideW, sy + 32.5f), IM_COL(EN_DIVIDER), 1.f);
        }
        // Marca: "Enigma" 20px bold + "build v3".
        {
            float by = sy + 32.f + 14.f;
            if (HazzureTypography::brand) {
                HazzureTypography::Scope brandScope(HazzureTypography::brand);
                d->AddText(ImVec2(sx + 4.f, by), IM_COL32(255, 255, 255, 255), "Hazzure");
            } else {
                d->AddText(ImVec2(sx + 4.f, by), IM_COL32(255, 255, 255, 255), "Hazzure");
            }
            ImVec2 vsz = ImGui::CalcTextSize("build v3");
            d->AddText(ImVec2(sx + sideW - vsz.x - 4.f, by + 6.f), IM_COL(EN_TEXT_DIM), "build v3");
        }
        // Label CHEATS.
        float ly = sy + 32.f + 14.f + 30.f;
        d->AddText(ImVec2(sx + 4.f, ly), IM_COL(EN_TEXT_MUTE), "CHEATS");
        // Abas 30px.
        struct EnTab { const char* name; int icon; int legacyTab; };
        static const EnTab kTabs[6] = {
            { "Aimbot", 1, -1 }, { "Ragebot", 9, 1 }, { "Legitbot", 0, 0 },
            { "Visuals", 6, 6 }, { "Misc", 10, 10 }, { "Skins", 5, 11 }
        };
        float ty = ly + 20.f;
        for (int i = 0; i < 6; ++i) {
            const bool active = s_enigmaTab == i;
            const bool hov = ImGui::IsMouseHoveringRect(ImVec2(sx, ty), ImVec2(sx + sideW, ty + 30.f));
            if (active) {
                d->AddRectFilled(ImVec2(sx + 2.f, ty), ImVec2(sx + sideW - 2.f, ty + 30.f), IM_COL(EN_HOVER), 6.f);
                d->AddRectFilled(ImVec2(sx + 2.f, ty), ImVec2(sx + 5.f, ty + 30.f), IM_COL(EN_ACCENT), 1.f);
            }
            GSWidgets::GS_NavIcon(d, kTabs[i].icon, ImVec2(sx + 14.f, ty + 7.f), IM_COL(active ? EN_TEXT : (hov ? EN_TEXT : EN_TEXT_DIM)));
            ImGui::PushID(kTabs[i].name);
            ImGui::SetCursorScreenPos(ImVec2(sx, ty));
            ImGui::InvisibleButton("##enTab", ImVec2(sideW, 30.f));
            if (ImGui::IsItemClicked()) { s_enigmaTab = i; g_bindingTarget = nullptr; }
            ImGui::PopID();
            d->AddText(ImVec2(sx + 40.f, ty + (30.f - ImGui::CalcTextSize(kTabs[i].name).y) * .5f),
                IM_COL(active ? EN_TEXT : EN_TEXT_DIM), kTabs[i].name);
            ty += 30.f;
        }
        // Card de usuario na base da sidebar.
        {
            const float uy = sy + contentH - 56.f;
            d->AddLine(ImVec2(sx, uy - 8.f), ImVec2(sx + sideW, uy - 8.f), IM_COL(EN_DIVIDER), 1.f);
            d->AddCircleFilled(ImVec2(sx + 21.f, uy + 21.f), 17.f, IM_COL(EN_SECTION), 24);
            d->AddCircle(ImVec2(sx + 21.f, uy + 21.f), 17.f, IM_COL(EN_BORDER), 24, 1.f);
            d->AddText(ImVec2(sx + 48.f, uy + 6.f), IM_COL(EN_TEXT), "Hazure");
            d->AddText(ImVec2(sx + 48.f, uy + 22.f), IM_COL(EN_TEXT_MUTE), "Admin");
            // chevron para baixo
            GSWidgets::GS_NavIcon(d, 16, ImVec2(sx + sideW - 20.f, uy + 15.f), IM_COL(EN_TEXT_DIM));
        }
        d->AddLine(ImVec2(sx + sideW, sy), ImVec2(sx + sideW, sy + contentH), IM_COL(EN_DIVIDER), 1.f);

        // ---------- Card principal ----------
        const float cardX = sx + sideW + 12.f;
        const float cardY = sy;
        const float cardW = contentW - sideW - 12.f;
        const float cardH = contentH;
        d->AddRectFilled(ImVec2(cardX, cardY), ImVec2(cardX + cardW, cardY + cardH), IM_COL(EN_CARD), 10.f);
        d->AddRect(ImVec2(cardX, cardY), ImVec2(cardX + cardW, cardY + cardH), IM_COL(EN_BORDER), 10.f, 0, 1.f);

        const float cpad = 16.f;
        const float ix = cardX + cpad, iw = cardW - cpad * 2.f;

        // ---------- Topline 36px: busca + dropdown de config + Save ----------
        const float topY = cardY + cpad;
        const float saveW = 90.f, cfgW = 150.f, gap = 12.f;
        const float searchW = iw - saveW - cfgW - gap * 2.f;
        ImGui::SetCursorScreenPos(ImVec2(ix, topY));
        if (GSWidgets::GS_EnSearch(gs_searchQuery, sizeof(gs_searchQuery), searchW) && gs_searchQuery[0])
            GS_ProcessSearchQuery(gs_searchQuery);
        // Dropdown de config 150x34.
        {
            const float cxp = ix + searchW + gap;
            ImGui::SetCursorScreenPos(ImVec2(cxp, topY));
            ImGui::PushID("##enCfg");
            ImGui::InvisibleButton("##cfgBox", ImVec2(cfgW, 34.f));
            const bool hov = ImGui::IsItemHovered();
            d->AddRectFilled(ImVec2(cxp, topY), ImVec2(cxp + cfgW, topY + 34.f), IM_COL(EN_CARD_INNER), 6.f);
            d->AddRect(ImVec2(cxp, topY), ImVec2(cxp + cfgW, topY + 34.f), IM_COL(hov ? EN_TEXT_DIM : EN_BORDER), 6.f, 0, 1.f);
            GSWidgets::GS_NavIcon(d, 15, ImVec2(cxp + 8.f, topY + 9.f), IM_COL(EN_TEXT_DIM));
            d->AddText(ImVec2(cxp + 30.f, topY + 9.f), IM_COL(EN_TEXT), "default.cfg");
            d->AddCircleFilled(ImVec2(cxp + cfgW - 26.f, topY + 17.f), 3.f, IM_COL(EN_ACCENT), 12);
            GSWidgets::GS_NavIcon(d, 16, ImVec2(cxp + cfgW - 18.f, topY + 11.f), IM_COL(EN_TEXT_DIM));
            if (ImGui::IsItemClicked()) s_cfgPopup = !s_cfgPopup;
            if (s_cfgPopup) {
                ImGui::SetNextWindowPos(ImVec2(cxp, topY + 36.f));
                ImGui::SetNextWindowSize(ImVec2(cfgW, 40.f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
                ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL(EN_CARD_INNER));
                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL(EN_BORDER));
                if (ImGui::Begin("##enCfgPopup", &s_cfgPopup, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove)) {
                    if (ImGui::Selectable("default.cfg")) s_cfgPopup = false;
                }
                ImGui::End();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar();
            }
            ImGui::PopID();
        }
        // Save 90x34 #E13B3B, hover #C93030.
        {
            const float svx = ix + iw - saveW;
            ImGui::SetCursorScreenPos(ImVec2(svx, topY));
            ImGui::PushID("##enSave");
            ImGui::InvisibleButton("##saveBtn", ImVec2(saveW, 34.f));
            const bool hov = ImGui::IsItemHovered();
            d->AddRectFilled(ImVec2(svx, topY), ImVec2(svx + saveW, topY + 34.f), IM_COL(hov ? EN_ACCENT_SAVE_HOVER : EN_ACCENT), 6.f);
            ImVec2 ssz = ImGui::CalcTextSize("Save");
            d->AddText(ImVec2(svx + (saveW - ssz.x) * .5f, topY + (34.f - ssz.y) * .5f), IM_COL32(255, 255, 255, 255), "Save");
            if (ImGui::IsItemClicked()) SaveConfigFile();
            ImGui::PopID();
        }

        // ---------- Weapon bar 36px: "Pistols · N own, the rest from General" ----------
        const float wbY = topY + 36.f + gap;
        {
            const int own = [&] {
                auto it = g_weaponClasses.find("pistols");
                return it == g_weaponClasses.end() ? 0 : static_cast<int>(it->second.size());
            }();
            char ownTxt[64];
            snprintf(ownTxt, sizeof(ownTxt), "\xC2\xB7 %d own, the rest from General", own);
            ImGui::SetCursorScreenPos(ImVec2(ix, wbY));
            ImGui::InvisibleButton("##enWeaponBar", ImVec2(iw, 36.f));
            if (ImGui::IsItemClicked()) s_bodyOpen = !s_bodyOpen;
            GSWidgets::GS_NavIcon(d, 18, ImVec2(ix + 2.f, wbY + 10.f), IM_COL(EN_TEXT));
            d->AddText(ImVec2(ix + 26.f, wbY + 10.f), IM_COL32(255, 255, 255, 255), "Pistols");
            const float px = ix + 26.f + ImGui::CalcTextSize("Pistols").x + 8.f;
            d->AddText(ImVec2(px, wbY + 10.f), IM_COL(EN_TEXT_DIM), ownTxt);
            // chevron para a direita
            {
                const float cxw = ix + iw - 16.f, cyw = wbY + 18.f;
                ImU32 col = IM_COL(EN_TEXT_DIM);
                d->AddLine(ImVec2(cxw - 3.f, cyw - 4.f), ImVec2(cxw + 1.f, cyw), col, 1.4f);
                d->AddLine(ImVec2(cxw + 1.f, cyw), ImVec2(cxw - 3.f, cyw + 4.f), col, 1.4f);
            }
            d->AddLine(ImVec2(ix, wbY + 36.5f), ImVec2(ix + iw, wbY + 36.5f), IM_COL(EN_DIVIDER), 1.f);
        }

        // ---------- Corpo ----------
        if (s_bodyOpen) {
            float bodyY = wbY + 36.f + gap;
            const float bodyH0 = cardY + cardH - cpad - bodyY;
            // Sub-abas 28px (Aim / Accuracy / Anti-aim) so na aba Aimbot.
            int aimSub = s_aimSub;
            if (s_enigmaTab == 0) {
                static const char* kSubTabs[3] = { "Aim", "Accuracy", "Anti-aim" };
                float stx = ix;
                for (int i = 0; i < 3; ++i) {
                    const bool active = aimSub == i;
                    const ImVec2 tsz = ImGui::CalcTextSize(kSubTabs[i]);
                    ImGui::PushID(kSubTabs[i]);
                    ImGui::SetCursorScreenPos(ImVec2(stx, bodyY));
                    ImGui::InvisibleButton("##enSub", ImVec2(tsz.x + 24.f, 28.f));
                    if (ImGui::IsItemClicked()) s_aimSub = i;
                    ImGui::PopID();
                    d->AddText(ImVec2(stx + 12.f, bodyY + (28.f - tsz.y) * .5f),
                        IM_COL(active ? EN_TEXT : EN_TEXT_DIM), kSubTabs[i]);
                    if (active)
                        d->AddRectFilled(ImVec2(stx + 12.f, bodyY + 26.f), ImVec2(stx + 12.f + tsz.x, bodyY + 28.f), IM_COL(EN_ACCENT), 0.f);
                    stx += tsz.x + 24.f;
                }
                d->AddLine(ImVec2(ix, bodyY + 28.5f), ImVec2(ix + iw, bodyY + 28.5f), IM_COL(EN_DIVIDER), 1.f);
                bodyY += 28.f + gap;
            }
            const float bodyH = cardY + cardH - cpad - bodyY;
            const float previewW = std::min(440.f, iw);
            const float settingsW = std::min(510.f, iw - previewW - 16.f);
            if (s_enigmaTab == 0 && aimSub == 0) {
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::PushID("##enAim");
                ImGui::BeginChild("##enAimSettings", ImVec2(settingsW, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaAimPanel(settingsW);
                ImGui::EndChild();
                ImGui::PopID();
                const float pvx = ix + settingsW + 16.f;
                ImGui::SetCursorScreenPos(ImVec2(pvx, bodyY));
                ImGui::PushID("##enPreview");
                ImGui::BeginChild("##enPreviewBody", ImVec2(std::min(previewW, ix + iw - pvx), bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaPreview(std::min(previewW, ix + iw - pvx), bodyH);
                ImGui::EndChild();
                ImGui::PopID();
            } else if (s_enigmaTab == 0 && aimSub == 1) {
                // stage: 4 | reason: Accuracy agora usa painel EN
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::BeginChild("##enAccuracy", ImVec2(settingsW, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaAccuracyPanel(settingsW);
                ImGui::EndChild();
            } else if (s_enigmaTab == 0 && aimSub == 2) {
                // stage: 4 | reason: Anti-aim agora usa painel EN
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::BeginChild("##enAntiAim", ImVec2(settingsW, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaAntiAimPanel(settingsW);
                ImGui::EndChild();
            } else if (s_enigmaTab == 1) {
                // stage: 4 | reason: Ragebot EN
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::BeginChild("##enRagebot", ImVec2(settingsW, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaRagebotPanel(settingsW);
                ImGui::EndChild();
            } else if (s_enigmaTab == 2) {
                // stage: 4 | reason: Legitbot EN
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::BeginChild("##enLegitbot", ImVec2(iw, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaLegitbotPanel(iw);
                ImGui::EndChild();
            } else if (s_enigmaTab == 3) {
                // stage: 4 | reason: Visuals EN
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::BeginChild("##enVisuals", ImVec2(iw, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaVisualsPanel(iw);
                ImGui::EndChild();
            } else if (s_enigmaTab == 4) {
                // stage: 4 | reason: Misc EN
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::BeginChild("##enMisc", ImVec2(iw, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawEnigmaMiscPanel(iw);
                ImGui::EndChild();
            } else if (s_enigmaTab == 5) {
                // stage: 4 | reason: Skins — wrap legado em coluna EN
                ImGui::SetCursorScreenPos(ImVec2(ix, bodyY));
                ImGui::BeginChild("##enSkins", ImVec2(iw, bodyH), false, ImGuiWindowFlags_NoScrollbar);
                DrawLocalInventory(shared);
                ImGui::EndChild();
            }
        }

        ImGui::Dummy(ImVec2(winW, winH));
    }
    ImGui::End();
    if (g_bindingTarget && g_bindingRenderedFrame != ImGui::GetFrameCount()) g_bindingTarget = nullptr;

    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(15);
}

static void DrawGSPanel(Shared* shared, bool* pOpen, int screenW, int screenH) {
    if (g_useEnigmaLayout) { DrawEnigmaPanel(shared, pOpen, screenW, screenH); return; }
    if (!pOpen || !*pOpen || screenW < 240 || screenH < 200) { g_bindingTarget = nullptr; return; }

    // Aplicar o mesmo contrato da persistência antes do header, topbar e roteamento.
    HazzureUI::ValidateNavigationIndices();

    GSLayoutData layout = ResolveGSLayout(screenW, screenH);

    ImGui::SetNextWindowSize({ layout.width, layout.height }, ImGuiCond_Always);
    ImGui::SetNextWindowPos({ (screenW - layout.width) * 0.5f, (screenH - layout.height) * 0.5f }, ImGuiCond_FirstUseEver);

    // Estilo GameSense
    ImVec4 menuBg(g_cfg.colMenuBg[0], g_cfg.colMenuBg[1], g_cfg.colMenuBg[2], g_cfg.colMenuBg[3]);
    ImVec4 accentCol(g_cfg.colMenuAccent[0], g_cfg.colMenuAccent[1], g_cfg.colMenuAccent[2], g_cfg.colMenuAccent[3]);
    ImVec4 accentAlt(accentCol.x * 0.85f, accentCol.y * 0.85f, accentCol.z * 0.85f, accentCol.w);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL(menuBg));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL(GS_BORDER));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL(GS_DROPDOWN));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL(GS_BORDER_HOVER));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, accentCol);
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL(GS_CARD));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL(GS_DROPDOWN));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL(GS_CARD));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL(GS_TEXT));
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, IM_COL(GS_TEXT_MUTED));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, accentCol);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, accentCol);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, accentAlt);
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, accentCol);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10, 8 });

    auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("##HazzureGameSense", pOpen, flags)) {
        ImVec2 winPos = ImGui::GetWindowPos();
        winPos.x = std::clamp(winPos.x, 0.f, std::max(0.f, float(screenW) - layout.width));
        winPos.y = std::clamp(winPos.y, 0.f, std::max(0.f, float(screenH) - layout.height));
        ImGui::SetWindowPos(winPos);

        ImDrawList* bgDraw = ImGui::GetWindowDrawList();
        ImVec2 winMax = ImVec2(winPos.x + layout.width, winPos.y + layout.height);
        bgDraw->AddRectFilled(winPos, winMax, IM_COL(menuBg), 3.0f);

        // Falling Snow Particle effect behind content
        if (g_cfg.menuSnow) {
            UpdateAndRenderMenuSnow(bgDraw, winPos, winMax, ImGui::GetIO().DeltaTime, g_cfg.colSnow);
        }

        // 1. Header superior
        DrawGSHeader(layout);

        // 2. Sidebar esquerda
        DrawGSSidebar(layout, winPos);

        // 3. Topbar de seções
        DrawGSTopbar(layout, winPos);

        // 4. Área de conteúdo (duas colunas; Legitbot, Miscellaneous e Inventory usam o topo completo sem topbar duplicada)
        const bool usesFullTop = (gs_currentTab == 0 || gs_currentTab == 2 || gs_currentTab == 3 || gs_currentTab == 6 || gs_currentTab == 7 || gs_currentTab == 10 || gs_currentTab == 11);
        const float contentY = usesFullTop ? (layout.headerH + 6.0f) : layout.bodyY;
        const float contentH = usesFullTop ? (layout.bodyH + layout.topbarH - 6.0f) : layout.bodyH;
        ImGui::SetCursorPos({ layout.bodyX, contentY });

        // Validar section index
        int sectionIdx = gs_currentSection[gs_currentTab];
        int sectionCount = gs_mainTabSectionCounts[gs_currentTab];
        if (sectionIdx < 0 || sectionIdx >= sectionCount) {
            sectionIdx = 0;
            gs_currentSection[gs_currentTab] = 0;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::PushID(gs_currentTab);
        ImGui::PushID(sectionIdx);

        if (layout.twoColumns && gs_currentTab != 11) {
            ImDrawList* d = ImGui::GetWindowDrawList();
            float divX = winPos.x + layout.bodyX + layout.columnW + 8.0f;
            d->AddLine(ImVec2(divX, winPos.y + contentY + 6.0f), ImVec2(divX, winPos.y + contentY + contentH - 6.0f), IM_COL(GS_BORDER), 1.0f);
            for (int column = 0; column < 2; ++column) {
                if (column) ImGui::SameLine(0, 16.0f);
                ImGui::PushID(column);
                ImGui::BeginChild("##gsColumn", { layout.columnW, contentH }, false, ImGuiWindowFlags_AlwaysUseWindowPadding);
                DrawGSPanelSection(gs_currentTab, sectionIdx, column, shared);
                ImGui::EndChild();
                ImGui::PopID();
            }
        } else {
            ImGui::BeginChild("##gsSingleColumn", { layout.bodyW, contentH }, false, ImGuiWindowFlags_AlwaysUseWindowPadding);
            DrawGSPanelSection(gs_currentTab, sectionIdx, 0, shared);
            DrawGSPanelSection(gs_currentTab, sectionIdx, 1, shared);
            ImGui::EndChild();
        }

        ImGui::PopID();
        ImGui::PopID();

        ImGui::PopStyleVar();
    }
    ImGui::End();
    if (g_bindingTarget && g_bindingRenderedFrame != ImGui::GetFrameCount()) g_bindingTarget = nullptr;

    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(15);
}

} // namespace GSLayout

// Valida e remapeia índices de navegação persistidos/carregados para valores válidos visíveis
// Chamado após LoadConfigFile (OverlayPersistence.h) para garantir que nenhum índice oculto/fora de faixa chegue ao render
// Definição em header (GSLayout.inl) incluído por HazzureUI.h; compilação single-TU por binário (DLL + Test.exe separados)
// Ligação dos símbolos de navegação: static (linkage interna) pois APENAS Internal.cpp (src) inclui HazzureUI.h;
// testes são binário separado (Hazzure.Internal.Test.exe) com sua própria cópia. O checkpoint original usava static.
// Sem inline/static: linkage externa, single-TU por binário garante ausência de ODR violation.
void ValidateNavigationIndices() {
    // gs_currentTab: validar faixa e remapear tabs ocultos (Agents=4, Items=5 -> Inventory=11)
    if (GSLayout::gs_currentTab < 0 || GSLayout::gs_currentTab >= 14 || GSLayout::gs_hiddenTabs[GSLayout::gs_currentTab]) {
        int remapped = 0;
        if (GSLayout::gs_currentTab == 4 || GSLayout::gs_currentTab == 5) remapped = 11; // Agents / Items -> Inventory
                else remapped = 0;
        GSWidgets::GS_Log("[GUI] ValidateNavigationIndices: gs_currentTab %d inválido/oculto, corrigido para %d", GSLayout::gs_currentTab, remapped);
        GSLayout::gs_currentTab = remapped;
    }

    // gs_currentSection por aba: clamp para faixa válida
    for (int t = 0; t < 14; ++t) {
        int maxSec = GSLayout::gs_mainTabSectionCounts[t] - 1;
        if (GSLayout::gs_currentSection[t] < 0 || GSLayout::gs_currentSection[t] > maxSec) {
            GSWidgets::GS_Log("[GUI] ValidateNavigationIndices: gs_currentSection[%d]=%d fora de faixa (0-%d), corrigido para 0", t, GSLayout::gs_currentSection[t], maxSec);
            GSLayout::gs_currentSection[t] = 0;
        }
    }

    g_graphicArtSubTab = std::clamp(g_graphicArtSubTab, 0, 6);

    // g_inventorySubTab: ocultar Containers(2) e Tools(3), remapear para 0 (Profile)
    // Valid tabs: 0=Profile, 1=Equipment, 4=Graphic Art
    if (g_inventorySubTab == 2 || g_inventorySubTab == 3 || g_inventorySubTab < 0 || g_inventorySubTab > 4) {
        int old = g_inventorySubTab;
        g_inventorySubTab = 0; // Default para Profile
        GSWidgets::GS_Log("[GUI] ValidateNavigationIndices: g_inventorySubTab %d oculto/inválido, corrigido para %d", old, g_inventorySubTab);
    }
}
