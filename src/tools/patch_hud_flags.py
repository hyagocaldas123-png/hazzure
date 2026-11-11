path = r'c:\Codex2\gpt\HazzureInternal\src\HazzureUI.h'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# Replace HUD definitions
old_huds = '''// ---------- Janela de Espectadores (HUD Flutuante) ----------
static void RenderSpectatorsHUD(int screenW, int) {
    if (!g_cfg.showSpectators) return;

    ImGui::SetNextWindowPos(ImVec2((float)screenW - 200.f, 25.f), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.07f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
    if (ImGui::Begin("Espectadores##hudSpec", nullptr, flags)) {
        ImGui::TextColored(kAccent, "ESPECTADORES (0)");
        ImGui::Separator();
        ImGui::TextColored(kMuted, "Nenhum observador ativo");
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
}

// ---------- Temporizador de C4 / Bomba (HUD Flutuante) ----------
static float g_c4Remaining = 34.2f;
static ULONGLONG g_lastC4Beep = 0;

static void RenderBombTimerHUD(int screenW, int) {
    if (!g_cfg.showBombTimer) return;

    ImGui::SetNextWindowPos(ImVec2((float)screenW * 0.5f - 110.f, 24.f), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.07f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
    if (ImGui::Begin("C4Timer##hudBomb", nullptr, flags)) {
        char buf[32];
        sprintf_s(buf, "C4 ARMADA: %.1fs", g_c4Remaining);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", buf);
        ImGui::ProgressBar(g_c4Remaining / 40.0f, ImVec2(180.f, 6.f), "");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Desarme Possivel (+Kit)");

        if (g_cfg.bombBeepAudio) {
            ULONGLONG now = GetTickCount64();
            float intervalMs = std::clamp((g_c4Remaining / 40.0f) * 1000.f, 120.f, 1200.f);
            if (now - g_lastC4Beep > static_cast<ULONGLONG>(intervalMs)) {
                PlayProceduralC4Beep();
                g_lastC4Beep = now;
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
}'''

new_huds = '''// ---------- Constante de Flags Seguras para HUDs ----------
static const ImGuiWindowFlags kHudFlags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;

// ---------- Janela de Espectadores (HUD Flutuante) ----------
static void RenderSpectatorsHUD(uintptr_t localPawn, int screenW, int) {
    if (!g_cfg.showSpectators || !localPawn) return;

    ImGui::SetNextWindowPos(ImVec2((float)screenW - 220.f, 25.f), ImGuiCond_Always);
    if (ImGui::Begin("##hudSpec", nullptr, kHudFlags)) {
        ImGui::TextColored(kAccent, "ESPECTADORES (0)");
        ImGui::TextColored(kMuted, "Nenhum observador ativo");
    }
    ImGui::End();
}

// ---------- Temporizador de C4 / Bomba (HUD Flutuante) ----------
static float g_c4Remaining = 34.2f;
static ULONGLONG g_lastC4Beep = 0;

static void RenderBombTimerHUD(uintptr_t client, int screenW, int) {
    if (!g_cfg.showBombTimer || !client) return;

    ImGui::SetNextWindowPos(ImVec2((float)screenW * 0.5f - 100.f, 25.f), ImGuiCond_Always);
    if (ImGui::Begin("##hudBomb", nullptr, kHudFlags)) {
        char buf[32];
        sprintf_s(buf, "C4 ARMADA: %.1fs", g_c4Remaining);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", buf);
        ImGui::ProgressBar(g_c4Remaining / 40.0f, ImVec2(180.f, 6.f), "");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Desarme Possivel (+Kit)");

        if (g_cfg.bombBeepAudio) {
            ULONGLONG now = GetTickCount64();
            float intervalMs = std::clamp((g_c4Remaining / 40.0f) * 1000.f, 120.f, 1200.f);
            if (now - g_lastC4Beep > static_cast<ULONGLONG>(intervalMs)) {
                PlayProceduralC4Beep();
                g_lastC4Beep = now;
            }
        }
    }
    ImGui::End();
}'''

assert old_huds in text, "old_huds not found"
text = text.replace(old_huds, new_huds)

# Also update RenderFrame call sites in HazzureUI.h
old_render_frame = '''// ---------- Ponto de Entrada da Renderização do Frame ----------
static void RenderFrame(Shared* shared, uintptr_t client, bool* pShowMenu, int screenW, int screenH) {
    // 1. Overlay Visual no Mundo 3D (ESP, Caixas, Esqueleto, Balística, Velocímetro)
    RenderInGameESP(shared, client, screenW, screenH);

    // 2. HUDs Complementares
    RenderSpectatorsHUD(screenW, screenH);
    RenderBombTimerHUD(screenW, screenH);
    if (g_cfg.showHitmarker) {
        RenderHitmarker(ImGui::GetForegroundDrawList(), screenW, screenH);
    }'''

new_render_frame = '''// ---------- Ponto de Entrada da Renderização do Frame ----------
static void RenderFrame(Shared* shared, uintptr_t client, bool* pShowMenu, int screenW, int screenH) {
    if (!client) {
        client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    }

    uintptr_t localPawn = 0;
    if (client) {
        SafeRead(client + HazzureOffsets::Globals::dwLocalPlayerPawn, &localPawn);
    }

    // 1. Overlay Visual no Mundo 3D (ESP, Caixas, Esqueleto, Balística, Velocímetro)
    RenderInGameESP(shared, client, screenW, screenH);

    // 2. HUDs Complementares (Protegidos por ponteiro e flags no-input)
    if (localPawn) {
        RenderSpectatorsHUD(localPawn, screenW, screenH);
    }
    if (client) {
        RenderBombTimerHUD(client, screenW, screenH);
    }
    if (g_cfg.showHitmarker) {
        RenderHitmarker(ImGui::GetForegroundDrawList(), screenW, screenH);
    }'''

assert old_render_frame in text, "old_render_frame not found"
text = text.replace(old_render_frame, new_render_frame)

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)
print("HazzureUI.h patched with non-capturing HUD flags and pointer validations!")
