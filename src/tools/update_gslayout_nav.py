import re

path = r'C:\Codex2\gpt\HazzureInternal\src\GSLayout.inl'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update gs_mainTabs
old_tabs = '''static const char* const gs_mainTabs[14] = {
    "Aim Assist",     // 0
    "Rage",           // 1
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
};'''

new_tabs = '''static const char* const gs_mainTabs[14] = {
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
};'''

text = text.replace(old_tabs.replace('\r\n', '\n'), new_tabs)

# 2. Update gs_hiddenTabs
old_hidden = '''static const bool gs_hiddenTabs[14] = {
    false, false, true, false, true, true,
    false, false, false, false, false, false, false, false
};'''

new_hidden = '''static const bool gs_hiddenTabs[14] = {
    false, false, false, false, true, false,
    false, false, false, false, false, false, true, false
};'''

text = text.replace(old_hidden.replace('\r\n', '\n'), new_hidden)

# 3. Update DrawGSHeader
old_header_pat = r'static void DrawGSHeader\(const GSLayoutData& layout\) \{[\s\S]*?\n\}'
new_header = '''static void DrawGSHeader(const GSLayoutData& layout) {
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

        // White rounded badge for Haz (matching AN0N's reference badge)
        ImVec2 badgeMin = ImVec2(textX - badgePadX, textY - badgePadY);
        ImVec2 badgeMax = ImVec2(textX + hazSz.x + badgePadX, textY + hazSz.y + badgePadY);
        d->AddRectFilled(badgeMin, badgeMax, IM_COL32(255, 255, 255, 255), 4.0f);

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

    // Botão Save à direita em todas as telas
    ImGui::SetCursorPos(ImVec2(layout.width - 88, 14));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(242, 242, 246, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(215, 215, 222, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(12, 12, 16, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    if (ImGui::Button("Save", ImVec2(72, 28))) SaveConfigFile();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Salvar configuracao em disco");
}'''

text = re.sub(old_header_pat, new_header, text)

# 4. Remove CONTROLS / APPEARANCE / WORKSPACE from DrawGSSidebar
text = re.sub(r'if \(layout\.sidebarW > 80 && \(i == 0 \|\| i == 6 \|\| i == 11\)\) \{[\s\S]*?\}', '', text)

# 5. Update DrawGSTopbar skip check
text = re.sub(
    r'if \(gs_currentTab == 0 \|\| gs_currentTab == 10 \|\| gs_currentTab == 11\) return;',
    'if (gs_currentTab == 0 || gs_currentTab == 2 || gs_currentTab == 3 || gs_currentTab == 6 || gs_currentTab == 7 || gs_currentTab == 10 || gs_currentTab == 11) return;',
    text
)

# 6. Update usesFullTop in DrawGSPanel
text = re.sub(
    r'const bool usesFullTop = \(gs_currentTab == 0 \|\| gs_currentTab == 10 \|\| gs_currentTab == 11\);',
    'const bool usesFullTop = (gs_currentTab == 0 || gs_currentTab == 2 || gs_currentTab == 3 || gs_currentTab == 6 || gs_currentTab == 7 || gs_currentTab == 10 || gs_currentTab == 11);',
    text
)

# 7. Update ValidateNavigationIndices (don't remap tab 2 to 6)
text = text.replace('else if (GSLayout::gs_currentTab == 2) remapped = 6; // Players -> Visuals\n', '')
text = text.replace('else if (GSLayout::gs_currentTab == 2) remapped = 6; // Players -> Visuals\r\n', '')

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)
print("Updated navigation and headers in GSLayout.inl")
