// Own-process tests observe real widget bindings and hit boxes; absent in production.
#ifdef INTERNAL_SELF_TEST
static void (*g_uiControlObserver)(const char*, const void*, ImVec2, ImVec2) = nullptr;
#define HAZZURE_OBSERVE_CONTROL(label, value) do { if (g_uiControlObserver) g_uiControlObserver(label, value, ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); } while (0)
#else
#define HAZZURE_OBSERVE_CONTROL(label, value) ((void)0)
#endif

// ==========================================================
// SISTEMA DE DESIGN — WIDGETS MONOCROMATICOS PRETO & BRANCO
// ==========================================================
static const char* VisibleLabelEnd(const char* s) {
    if (!s) return "";
    const char* h = strstr(s, "##");
    return h ? h : s + strlen(s);
}

static const char* KeyLabel(int vk, char* buf, size_t cap) {
    switch (vk) {
        case 0: return "NENHUMA";
        case VK_LBUTTON: return "MOUSE 1";
        case VK_RBUTTON: return "MOUSE 2";
        case VK_MBUTTON: return "MOUSE 3";
        case VK_XBUTTON1: return "MOUSE 4";
        case VK_XBUTTON2: return "MOUSE 5";
        case VK_INSERT: return "INSERT";
        case VK_DELETE: return "DELETE";
        case VK_HOME: return "HOME";
        case VK_END: return "END";
        case VK_PRIOR: return "PAGE UP";
        case VK_NEXT: return "PAGE DN";
        case VK_MENU: case VK_LMENU: case VK_RMENU: return "ALT";
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: return "SHIFT";
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return "CTRL";
        case VK_CAPITAL: return "CAPS";
        case VK_TAB: return "TAB";
        case VK_SPACE: return "SPACE";
        default: break;
    }
    if (vk >= 'A' && vk <= 'Z') { snprintf(buf, cap, "%c", static_cast<char>(vk)); return buf; }
    if (vk >= '0' && vk <= '9') { snprintf(buf, cap, "%c", static_cast<char>(vk)); return buf; }
    if (vk >= VK_F1 && vk <= VK_F12) { snprintf(buf, cap, "F%d", vk - VK_F1 + 1); return buf; }
    snprintf(buf, cap, "VK %d", vk);
    return buf;
}

static bool AnyBindInputDown() {
    auto& io = ImGui::GetIO();
    for (int m = 0; m < 5; ++m) if (io.MouseDown[m] || ImGui::IsMouseClicked(m)) return true;
    for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k)
        if (ImGui::IsKeyDown(static_cast<ImGuiKey>(k)) || ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) return true;
    static const int vks[] = {
        VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2,
        VK_SHIFT, VK_LSHIFT, VK_RSHIFT, VK_CONTROL, VK_LCONTROL, VK_RCONTROL,
        VK_MENU, VK_LMENU, VK_RMENU, VK_INSERT, VK_DELETE, VK_HOME, VK_END,
        VK_PRIOR, VK_NEXT, VK_TAB, VK_SPACE, VK_CAPITAL, VK_ESCAPE, VK_BACK
    };
    for (int vk : vks) if (g_bindingKeyDown[vk & 0xFF]) return true;
    for (int vk = '0'; vk <= '9'; ++vk) if (g_bindingKeyDown[vk]) return true;
    for (int vk = 'A'; vk <= 'Z'; ++vk) if (g_bindingKeyDown[vk]) return true;
    for (int vk = VK_F1; vk <= VK_F12; ++vk) if (g_bindingKeyDown[vk]) return true;
    return false;
}

static void BeginCleanCard(const char* title, const char* rightTag = nullptr) {
    (void)rightTag;
    const float w = std::max(1.f, ImGui::GetContentRegionAvail().x);
    g_cleanCardContentW = w;
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0.f, 2.f));
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    {
        HazzureTypography::Scope headingFont(HazzureTypography::heading);
        ImVec2 ts = ImGui::CalcTextSize(title);
        float textX = p.x + std::max(0.0f, (w - ts.x) * 0.5f);
        float lineY = p.y + ts.y * 0.5f;
        if (textX - p.x > 10.0f) {
            d->AddLine(ImVec2(p.x, lineY), ImVec2(textX - 8.0f, lineY), IM_COL32(46, 46, 56, 255), 1.0f);
        }
        if (p.x + w - (textX + ts.x) > 10.0f) {
            d->AddLine(ImVec2(textX + ts.x + 8.0f, lineY), ImVec2(p.x + w, lineY), IM_COL32(46, 46, 56, 255), 1.0f);
        }
        d->PushClipRect(p, ImVec2(p.x + w, p.y + ts.y + 2.0f), true);
        d->AddText(ImVec2(textX, p.y), IM_COL32(242, 243, 248, 255), title);
        d->PopClipRect();
        ImGui::Dummy(ImVec2(w, ts.y + 6.0f));
    }
}

static void EndCleanCard() {
    ImGui::Dummy(ImVec2(0.f, 8.f));
    ImGui::EndGroup();
    ImGui::Dummy(ImVec2(0.f, 4.f));
}

static bool CleanToggle(const char* label, bool* v) {
    ImGui::PushID(label);
    const float w = std::max(1.f, std::min(g_cleanCardContentW, ImGui::GetContentRegionAvail().x));
    const float boxSz = 16.f;
    const float textW = std::max(1.f, w - boxSz - 10.f);
    const char* end = VisibleLabelEnd(label);
    ImVec2 textSz = ImGui::CalcTextSize(label, end, false, textW);
    const float h = std::max(22.f, textSz.y + 6.f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##tgl", ImVec2(w, h));
    if (pressed) *v = !*v;
    bool hov = ImGui::IsItemHovered();
    const bool disabled = ImGui::GetStyle().Alpha < 0.99f;

    ImDrawList* d = ImGui::GetWindowDrawList();

    // Checkbox quadrado no estilo MemeSense (sem parte vermelha: branco no ativo, grafite no inativo)
    ImVec2 boxMin(p.x, p.y + (h - boxSz) * 0.5f);
    ImVec2 boxMax(boxMin.x + boxSz, boxMin.y + boxSz);

    if (*v) {
        // Ativo: Quadrado preenchido com branco puro e checkmark escuro
        ImU32 fillCol = disabled ? IM_COL32(180, 180, 186, 160) : (hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(245, 246, 250, 255));
        d->AddRectFilled(boxMin, boxMax, fillCol, 3.0f);
        // Checkmark '✓' em preto/carvão
        ImU32 checkCol = IM_COL32(10, 10, 14, 255);
        ImVec2 p0(boxMin.x + 3.5f, boxMin.y + 8.5f);
        ImVec2 p1(boxMin.x + 6.5f, boxMin.y + 12.0f);
        ImVec2 p2(boxMin.x + 12.5f, boxMin.y + 4.5f);
        d->AddLine(p0, p1, checkCol, 2.0f);
        d->AddLine(p1, p2, checkCol, 2.0f);
    } else {
        // Inativo: Quadrado escuro com borda sutil
        ImU32 bgCol = hov ? IM_COL32(32, 32, 40, 255) : IM_COL32(20, 20, 26, 255);
        ImU32 borderCol = hov ? IM_COL32(80, 82, 96, 255) : IM_COL32(48, 50, 60, 255);
        d->AddRectFilled(boxMin, boxMax, bgCol, 3.0f);
        d->AddRect(boxMin, boxMax, borderCol, 3.0f, 0, 1.2f);
    }

    // Texto da opção logo ao lado do checkbox
    ImU32 txtCol = disabled ? IM_COL32(130, 132, 142, 165) : (*v || hov) ? IM_COL32(255, 255, 255, 255) : IM_COL32(236, 238, 244, 255);
    d->PushClipRect(ImVec2(boxMax.x + 8.f, p.y), ImVec2(p.x + w, p.y + h), true);
    d->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(boxMax.x + 8.f, p.y + (h - textSz.y) * 0.5f), txtCol, label, end, textW);
    d->PopClipRect();

    ImGui::PopID();
    HAZZURE_OBSERVE_CONTROL(label, v);
    return pressed;
}

static bool CleanSliderFloat(const char* label, float* v, float vMin, float vMax, const char* fmt = "%.2f") {
    ImGui::PushID(label);
    const float w = g_cleanCardContentW > 0 ? g_cleanCardContentW : std::max(1.0f, ImGui::GetContentRegionAvail().x);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();

    char valBuf[48];
    if (*v <= vMin && strstr(label, "time")) {
        strcpy_s(valBuf, "disabled");
    } else {
        snprintf(valBuf, sizeof(valBuf), fmt, *v);
    }
    const char* end = VisibleLabelEnd(label);
    ImVec2 valSz = ImGui::CalcTextSize(valBuf);

    d->AddText(p, IM_COL32(235, 238, 245, 255), label, end);
    d->AddText(ImVec2(p.x + w - valSz.x, p.y), IM_COL32(245, 245, 250, 255), valBuf);

    ImGui::Dummy(ImVec2(w, 14.f));
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));
    bool changed = ImGui::SliderFloat("##s", v, vMin, vMax, fmt);
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    float bW = bMax.x - bMin.x;
    bool hov = ImGui::IsItemHovered();
    bool act = ImGui::IsItemActive();

    float trackY = (bMin.y + bMax.y) * 0.5f;
    float frac = (vMax > vMin) ? std::clamp((*v - vMin) / (vMax - vMin), 0.0f, 1.0f) : 0.0f;
    float fillX = bMin.x + frac * bW;

    // Dark grey track line (height 2.5px)
    d->AddLine(ImVec2(bMin.x, trackY), ImVec2(bMax.x, trackY), IM_COL32(42, 45, 54, 255), 2.5f);
    // Filled progress line (lighter grey/white)
    if (fillX > bMin.x) {
        ImU32 fillCol = (hov || act) ? IM_COL32(225, 228, 238, 255) : IM_COL32(150, 154, 166, 255);
        d->AddLine(ImVec2(bMin.x, trackY), ImVec2(fillX, trackY), fillCol, 2.5f);
    }

    ImGui::PopStyleColor(6);
    ImGui::PopID();
    HAZZURE_OBSERVE_CONTROL(label, v);
    return changed;
}

static bool CleanColorRow(const char* label, float col[4]) {
    ImGui::PushID(label);
    const float w = g_cleanCardContentW;
    const float h = 24.f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();
    const char* end = VisibleLabelEnd(label);
    ImVec2 textSz = ImGui::CalcTextSize(label, end);

    d->AddText(ImVec2(p.x, p.y + (h - textSz.y) * 0.5f), IM_COL32(238, 240, 246, 255), label, end);
    ImGui::SetCursorScreenPos(ImVec2(p.x + w - 28.f, p.y + 2.f));
    bool changed = ImGui::ColorEdit4("##c", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4.f));
    ImGui::Dummy(ImVec2(w, 0.f));
    ImGui::PopID();
    return changed;
}

static bool CleanCombo(const char* label, int* current_item, const char* const items[], int items_count) {
    ImGui::PushID(label);
    const float w = g_cleanCardContentW;
    const float comboW = std::clamp(w * 0.48f, 105.f, 150.f);
    const float h = 26.f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    const char* end = VisibleLabelEnd(label);
    ImVec2 textSz = ImGui::CalcTextSize(label, end);

    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x, p.y + (h - textSz.y) * 0.5f), IM_COL32(240, 242, 248, 255), label, end);

    ImGui::SetCursorScreenPos(ImVec2(p.x + w - comboW, p.y));
    ImGui::SetNextItemWidth(comboW);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(24, 24, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(36, 36, 46, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(40, 40, 52, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(50, 52, 64, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(245, 246, 250, 255));
    bool changed = ImGui::Combo("##cmb", current_item, items, items_count);
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar();

    HAZZURE_OBSERVE_CONTROL(label, current_item);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 2.f));
    ImGui::Dummy(ImVec2(w, 0.f));
    ImGui::PopID();
    return changed;
}

static int g_bindingRenderedFrame = -1;
static bool CleanKeybind(const char* label, int* vkKey) {
    ImGui::PushID(label);
    const float w = g_cleanCardContentW;
    const float h = 26.f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();
    const char* end = VisibleLabelEnd(label);
    ImVec2 textSz = ImGui::CalcTextSize(label, end);
    d->AddText(ImVec2(p.x, p.y + (h - textSz.y) * 0.5f), IM_COL32(238, 240, 246, 255), label, end);

    bool waiting = (g_bindingTarget == vkKey);
    if (waiting) g_bindingRenderedFrame = ImGui::GetFrameCount();
    char tmp[24]{};
    char badge[40]{};
    snprintf(badge, sizeof(badge), waiting ? "[ ... ]" : "[ %s ]", KeyLabel(*vkKey, tmp, sizeof(tmp)));

    const float btnW = 92.f;
    ImVec2 bMin(p.x + w - btnW, p.y + 1.f);
    ImVec2 bMax(p.x + w, p.y + h - 1.f);
    ImGui::SetCursorScreenPos(bMin);
    bool clicked = ImGui::InvisibleButton("##kb", ImVec2(btnW, h - 2.f));
    bool hov = ImGui::IsItemHovered();

    // Muted red keyboard button (as requested: "o teclado meio apagado em vermelho, e aonde coloca as keybind")
    ImU32 bgCol = waiting ? IM_COL32(165, 30, 40, 255) : (hov ? IM_COL32(72, 22, 28, 255) : IM_COL32(44, 14, 18, 255));
    ImU32 brdCol = waiting ? IM_COL32(235, 60, 75, 255) : (hov ? IM_COL32(110, 32, 40, 255) : IM_COL32(72, 20, 26, 255));
    ImU32 badgeTxtCol = waiting ? IM_COL32(255, 255, 255, 255) : IM_COL32(235, 175, 180, 255);
    d->AddRectFilled(bMin, bMax, bgCol, 3.f);
    d->AddRect(bMin, bMax, brdCol, 3.f);
    ImVec2 bSz = ImGui::CalcTextSize(badge);
    d->AddText(ImVec2(bMin.x + (btnW - bSz.x) * 0.5f, bMin.y + ((h - 2.f) - bSz.y) * 0.5f), badgeTxtCol, badge);

    bool changed = false;
    if (clicked) {
        if (waiting) {
            g_bindingTarget = nullptr;
            g_bindingWaitRelease = false;
        } else {
            g_bindingTarget = vkKey;
            g_bindingWaitRelease = true;
            g_bindingRenderedFrame = ImGui::GetFrameCount();
        }
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        *vkKey = 0;
        g_bindingTarget = nullptr;
        g_bindingWaitRelease = false;
        changed = true;
    }
    if (waiting) {
        if (!ImGui::GetIO().AppFocusLost) {
            if (g_bindingWaitRelease) {
                if (!AnyBindInputDown()) g_bindingWaitRelease = false;
            } else {
                auto& io = ImGui::GetIO();
                auto commit = [&](int vk) {
                    *vkKey = vk;
                    g_bindingTarget = nullptr;
                    g_bindingWaitRelease = false;
                    changed = true;
                };
                if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || g_bindingKeyDown[VK_ESCAPE]) {
                    g_bindingTarget = nullptr;
                    g_bindingWaitRelease = false;
                } else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false) ||
                           g_bindingKeyDown[VK_DELETE] || g_bindingKeyDown[VK_BACK]) {
                    commit(0);
                } else {
                    static const int mouseVks[5] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
                    for (int m = 0; m < 5 && g_bindingTarget; ++m)
                        if (ImGui::IsMouseClicked(m) || io.MouseDown[m] || g_bindingKeyDown[mouseVks[m]]) commit(mouseVks[m]);
                    struct KeyPair { ImGuiKey key; int vk; };
                    static const KeyPair pairs[] = {
                        {ImGuiKey_Insert, VK_INSERT}, {ImGuiKey_Home, VK_HOME}, {ImGuiKey_End, VK_END},
                        {ImGuiKey_PageUp, VK_PRIOR}, {ImGuiKey_PageDown, VK_NEXT}, {ImGuiKey_Tab, VK_TAB},
                        {ImGuiKey_Space, VK_SPACE}, {ImGuiKey_CapsLock, VK_CAPITAL},
                        {ImGuiKey_LeftShift, VK_SHIFT}, {ImGuiKey_RightShift, VK_SHIFT},
                        {ImGuiKey_LeftCtrl, VK_CONTROL}, {ImGuiKey_RightCtrl, VK_CONTROL},
                        {ImGuiKey_LeftAlt, VK_MENU}, {ImGuiKey_RightAlt, VK_MENU}
                    };
                    for (const auto& pair : pairs)
                        if (g_bindingTarget && (ImGui::IsKeyPressed(pair.key, false) || ImGui::IsKeyDown(pair.key) || g_bindingKeyDown[pair.vk]))
                            commit(pair.vk);
                    for (int i = 0; i < 12 && g_bindingTarget; ++i) {
                        ImGuiKey fk = static_cast<ImGuiKey>(ImGuiKey_F1 + i);
                        if (ImGui::IsKeyPressed(fk, false) || ImGui::IsKeyDown(fk) || g_bindingKeyDown[VK_F1 + i]) commit(VK_F1 + i);
                    }
                    for (int i = 0; i < 10 && g_bindingTarget; ++i) {
                        ImGuiKey nk = static_cast<ImGuiKey>(ImGuiKey_0 + i);
                        if (ImGui::IsKeyPressed(nk, false) || ImGui::IsKeyDown(nk) || g_bindingKeyDown['0' + i]) commit('0' + i);
                    }
                    for (int i = 0; i < 26 && g_bindingTarget; ++i) {
                        ImGuiKey ak = static_cast<ImGuiKey>(ImGuiKey_A + i);
                        if (ImGui::IsKeyPressed(ak, false) || ImGui::IsKeyDown(ak) || g_bindingKeyDown['A' + i]) commit('A' + i);
                    }
                }
            }
        } else {
            g_bindingTarget = nullptr;
            g_bindingWaitRelease = false;
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 2.f));
    ImGui::Dummy(ImVec2(w, 0.f));
    ImGui::PopID();
    HAZZURE_OBSERVE_CONTROL(label, vkKey);
    return changed;
}
