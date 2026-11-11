#pragma once

// HazzureInternal/src/cards/CardProfiles.inl — Frente 2.4 Modularizacao UI Enigma CS2
// Escopo: Presets, Overrides v3, JSON IO, Diagnostico da Sessao e Telemetria


inline void DrawProfilesPresetsCard(Shared* shared) {
    BeginCleanCard("Presets & Configuracao", "CONFIG");
    CleanKeybind("Tecla do Menu", &g_cfg.menuKey);
    ImGui::Dummy(ImVec2(0.f, 4.f));
    if (ImGui::Button("Carregar Preset Legit", ImVec2(g_cleanCardContentW, 24.f))) { g_activeProfileIndex=-1; ApplyPreset(0); ApplySharedConfig(shared); }
    if (ImGui::Button("Carregar Preset Semi-Rage", ImVec2(g_cleanCardContentW, 24.f))) { g_activeProfileIndex=-1; ApplyPreset(1); ApplySharedConfig(shared); }
    if (ImGui::Button("Carregar Apenas Visuais", ImVec2(g_cleanCardContentW, 24.f))) { g_activeProfileIndex=-1; ApplyPreset(2); ApplySharedConfig(shared); }
    ImGui::Separator();
    CleanToggle("Efeito Glass Blur (Translucido)", &g_cfg.glassBlur);
    CleanToggle("HUD Telemetria (FPS/Ping/Tick)", &g_cfg.showTelemetryHUD);
    ImGui::Separator();
    if (ImGui::Button("Salvar Config em Disco (JSON)", ImVec2(g_cleanCardContentW, 24.f))) SaveConfigFile();
    if (ImGui::Button("Carregar Config do Disco", ImVec2(g_cleanCardContentW, 24.f))) LoadConfigFile();
    if (ImGui::Button("Copiar Share Code", ImVec2(g_cleanCardContentW, 24.f))) {
        std::string code;
        if (ExportShareCode(code)) { ImGui::SetClipboardText(code.c_str()); Persistence::Status("Share code copiado"); }
    }
    if (ImGui::Button("Importar Share Code", ImVec2(g_cleanCardContentW, 24.f))) {
        const char* code = ImGui::GetClipboardText();
        size_t length = code ? strnlen_s(code, HazzureShare::MaxCode + 1) : 0;
        if (!length || length > HazzureShare::MaxCode) Persistence::Status("Clipboard vazio ou perfil grande demais");
        else ImportShareCode(std::string(code, length));
    }
    CleanToggle("Overlay de Atalhos", &g_cfg.showKeybindsOverlay);
    if (ImGui::Button("Exportar Config por Hash (SHA-256)", ImVec2(g_cleanCardContentW, 24.f))) {
        std::string hash;
        if (ExportConfigByHash(hash)) ImGui::SetClipboardText(hash.c_str());
    }
    if (ImGui::Button("Importar Config por Hash", ImVec2(g_cleanCardContentW, 24.f))) {
        ImportConfigByHash(ImGui::GetClipboardText());
    }
    ImGui::TextColored(kMuted, "Status: %s", g_configStatusMsg);
    ImGui::Dummy(ImVec2(0.f, 4.f));
    if (ImGui::Button("Desativar Recursos (Modulo Residente)", ImVec2(g_cleanCardContentW, 24.f))) {
        HazzureState::g_unloadRequested.store(true);
    }
    EndCleanCard();
}

inline void DrawProfilesOverridesCard() {
    BeginCleanCard("Perfis & Overrides (v3)", "PERFIS");
    ImGui::TextColored(kMuted, "Perfil Ativo:");
    for (size_t p = 0; p < g_profiles.size(); ++p) {
        bool isSel = (g_activeProfileIndex == static_cast<int>(p));
        char pBtn[64];
        sprintf_s(pBtn, "%s%s", isSel ? "> " : "  ", g_profiles[p].name.c_str());
        if (ImGui::Button(pBtn, ImVec2(g_cleanCardContentW, 22.f))) {
            OnProfileApplied(static_cast<int>(p));
        }
    }
    ImGui::Separator();
    ImGui::TextColored(kMuted, "Editor de Overrides:");
    static int s_targetType = 0;
    static int s_subIndex = 0;
    static int s_varIndex = 0;
    static char s_overrideVal[64] = "3.5";

    static const char* kTargetTypes[] = { "Contexto Global", "Classe de Arma Global", "Perfil Ativo" };
    static const char* kContextNames[] = { "enemy", "ally", "local" };
    static const char* kWeaponClasses[] = { "rifles", "pistols", "snipers" };
    static const char* kVarNames[] = { "aimFov", "aimSmooth", "aimKillDelayMs", "aimSwitchDelayMs", "hitchanceEnabled", "hitchancePercent", "autoScope", "rcsActive", "triggerDelayMs", "glowOutline", "espEnabled", "espBoxes" };

    if (ImGui::Combo("Alvo", &s_targetType, kTargetTypes, IM_ARRAYSIZE(kTargetTypes))) s_subIndex = 0;
    if (s_targetType == 0) ImGui::Combo("Contexto", &s_subIndex, kContextNames, IM_ARRAYSIZE(kContextNames));
    else if (s_targetType == 1) ImGui::Combo("Classe Arma", &s_subIndex, kWeaponClasses, IM_ARRAYSIZE(kWeaponClasses));

    ImGui::Combo("Variavel", &s_varIndex, kVarNames, IM_ARRAYSIZE(kVarNames));
    ImGui::InputText("Valor", s_overrideVal, sizeof(s_overrideVal));

    if (ImGui::Button("Definir Override", ImVec2(g_cleanCardContentW * 0.48f, 22.f))) {
        std::string varName = kVarNames[s_varIndex];
        if (s_targetType == 0) g_contexts[kContextNames[s_subIndex]][varName] = s_overrideVal;
        else if (s_targetType == 1) g_weaponClasses[kWeaponClasses[s_subIndex]][varName] = s_overrideVal;
        else if (s_targetType == 2 && g_activeProfileIndex >= 0 && g_activeProfileIndex < (int)g_profiles.size()) {
            g_profiles[g_activeProfileIndex].overrides[varName] = s_overrideVal;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Limpar Override", ImVec2(g_cleanCardContentW * 0.48f, 22.f))) {
        std::string varName = kVarNames[s_varIndex];
        if (s_targetType == 0) g_contexts[kContextNames[s_subIndex]].erase(varName);
        else if (s_targetType == 1) g_weaponClasses[kWeaponClasses[s_subIndex]].erase(varName);
        else if (s_targetType == 2 && g_activeProfileIndex >= 0 && g_activeProfileIndex < (int)g_profiles.size()) {
            g_profiles[g_activeProfileIndex].overrides.erase(varName);
        }
    }
    EndCleanCard();
}

inline void DrawProfilesDiagnosticsCard() {
    BeginCleanCard("Prontidao de Partida & Runtime", "DIAGNOSTICO AO VIVO");
    bool gameReady = HazzureWorld::ready.load();
    uint64_t epoch = HazzureWorld::epoch.load();
    ImGui::TextColored(gameReady ? ImVec4(0.2f, 0.95f, 0.4f, 1.0f) : ImVec4(0.95f, 0.4f, 0.2f, 1.0f),
        "Status da Partida: %s", gameReady ? "CONECTADO A SESSAO ATIVA" : "AGUARDANDO PARTIDA / MAPA");
    ImGui::TextDisabled("Sessao / Epoca do Mundo: %llu", epoch);

    uintptr_t client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (client) {
        auto localPawn = HazzureGame::LocalPawn(client);
        HazzureGame::Pawn pawnData{};
        bool pawnValid = HazzureGame::ReadPawn(localPawn, pawnData, false);
        ImGui::TextColored(kTextPrimary, "Pawn Local: 0x%llX | Vida: %d | Equipe: %d", localPawn, pawnValid ? pawnData.health : 0, pawnValid ? pawnData.team : 0);
    } else {
        ImGui::TextDisabled("client.dll: nao carregada no espaco de enderecamento");
    }
    ImGui::Separator();
    ImGui::TextWrapped("Camera solicitada: %s | FOV %.0f", g_cfg.cameraFovOverride ? "personalizada" : "padrao", g_cfg.cameraFov);
    ImGui::TextWrapped("Camera observada: %s", HazzureNativeCamera::Label());
    ImGui::Text("Distancia entregue: %.1f u | Proporcao: %.4f", HazzureNativeCamera::actualDistance.load(), HazzureNativeCamera::actualRatio.load());
    // Snapshot clears expired particle evidence. A desired checkbox never proves drawing.
    unsigned visible=0;auto now=GetTickCount64();
    for(const auto& particle:HazzureWeather::Snapshot(now))if(HazzureWeather::Visible(particle,now))++visible;
    ImGui::TextWrapped("Clima: %s", Workflow::Observed(g_cfg.weather3d,gameReady,visible>0,
        "Aguardando particulas confirmadas pela geometria"));
    ImGui::TextDisabled("Solicitado: %s | Particulas observadas: %u",g_cfg.weather3d?"sim":"nao",visible);
    auto inventory=HazzureNativeInventory::Snapshot();
    ImGui::TextWrapped("Inventario nativo: %s",HazzureNativeInventory::Label(inventory.diagnostic.status));
    ImGui::TextDisabled("Colecao local: %u itens | Sincronizacao solicitada: %s",static_cast<unsigned>(g_cfg.inventory.count),inventory.enabled?"sim":"nao");
    ImGui::TextWrapped("Killfeed: %s | %zu eventos no buffer",g_cfg.killfeedEnabled?"Habilitado (preferencia)":"Desativado",HazzureKillFeed::store.count);
    EndCleanCard();
}

inline void DrawProfilesTelemetryCard(Shared* shared) {
    BeginCleanCard("Telemetria Interna", "METRICAS");
    ImGui::TextUnformatted("Tempo por recurso");
    const HazzurePerformance::Stage stages[] = {
        HazzurePerformance::Stage::Overlay, HazzurePerformance::Stage::Aim,
        HazzurePerformance::Stage::Trigger, HazzurePerformance::Stage::Visibility,
        HazzurePerformance::Stage::Smoke
    };
    const char* labels[] = { "Overlay", "Mira", "Trigger", "Visibilidade", "Fumaca" };
    for (size_t i = 0; i < 5; ++i) {
        auto timing = HazzurePerformance::Read(stages[i]);
        if (timing.samples) ImGui::TextWrapped("%s: p95 %.3f / p99 %.3f ms (%u amostras)", labels[i], timing.p95, timing.p99, static_cast<unsigned>(timing.samples));
        else ImGui::TextDisabled("%s: aguardando medicao", labels[i]);
    }
    ImGui::Separator();
    ImGui::TextColored(kMuted, "Trigger Cliques/Min: %.1f", HazzureTelemetry::g_metrics.triggerClicksPerMinute);
    ImGui::TextColored(kMuted, "Ultimo Delay Trigger: %d ms", HazzureTelemetry::g_metrics.triggerLastDelayMs);
    ImGui::TextColored(kMuted, "Delay Medio: %.1f ms", HazzureTelemetry::g_metrics.triggerAvgDelayMs);
    ImGui::TextColored(kMuted, "Chams Overrides/s: %.1f", HazzureTelemetry::g_metrics.chamsOverridesPerSec);
    ImGui::Separator();
    ImGui::TextColored(kMuted, "m_Glow: 0x%X (%s)", (unsigned)HazzureSchema::g_mGlow.offset, HazzureSchema::g_mGlow.fromFallback ? "Fallback" : (HazzureSchema::g_mGlow.fromCache ? "Cache" : "Nativo"));
    ImGui::TextColored(kMuted, "m_flMaxShadowDistance: 0x%X", (unsigned)HazzureSchema::g_mMaxShadowDist.offset);
    ImGui::TextColored(kMuted, "m_iPing: 0x%X (%s)", (unsigned)HazzureSchema::g_mPing.offset, HazzureSchema::g_mPing.fromFallback ? "Fallback" : (HazzureSchema::g_mPing.fromCache ? "Cache" : "Nativo"));
    if (shared) {
        ImGui::Separator();
        ImGui::TextColored(kMuted, "Draw Calls: %lld", shared->calls);
        ImGui::TextColored(kMuted, "Chams Overrides: %lld", shared->changed);
        ImGui::TextColored(kMuted, "Falhas SEH: %lld", shared->faults);
    }
    EndCleanCard();
}


