#pragma once



// HazzureInternal/src/cards/CardCombat.inl — Frente 2.4 Modularizacao UI Enigma CS2

// Escopo: Aimbot, RCS, Triggerbot, Precisao, Multipoint, Frenagem





inline void DrawCombatAimbotCard() {

    BeginCleanCard("Aimbot", "BONE 3D");

    CleanToggle("Aimbot Ativo", &g_cfg.aimActive);

    CleanToggle("Somente Alvos Visiveis", &g_cfg.aimVisibleOnly);

    CleanToggle("Auto-Scope AWP / Scout", &g_cfg.autoScope);
    ImGui::TextUnformatted("Pausa apos eliminacao (ms)");ImGui::SetNextItemWidth(g_cleanCardContentW);ImGui::SliderInt("##aimKillDelayMs",&g_cfg.aimKillDelayMs,0,1000);
    ImGui::TextUnformatted("Pausa ao trocar alvo (ms)");ImGui::SetNextItemWidth(g_cleanCardContentW);ImGui::SliderInt("##aimSwitchDelayMs",&g_cfg.aimSwitchDelayMs,0,1000);


    CleanSliderFloat("Campo de Visao (FOV)", &g_cfg.aimFov, 0.5f, 25.0f, "%.1f deg");

    CleanSliderFloat("Suavizacao (Smooth)", &g_cfg.aimSmooth, 1.0f, 25.0f, "%.1f");

    CleanToggle("Aceleracao / Desaceleracao", &g_cfg.aimEase);

    if (g_cfg.aimEase) CleanSliderFloat("Pico de Velocidade", &g_cfg.aimEaseBias, .15f, .65f, "%.2f");

    const char* bones[] = { "Cabeca (Bone 6)", "Pescoco (Bone 5)", "Peito (Bone 4)", "Pelve (Bone 0)" };

    CleanCombo("Osso Alvo", &g_cfg.aimBone, bones, IM_ARRAYSIZE(bones));

    const char* targetModes[] = { "Osso Fixo", "Osso Mais Proximo", "Cabeca / Fallback Visivel" };

    CleanCombo("Selecao de Alvo", &g_cfg.aimTargetMode, targetModes, IM_ARRAYSIZE(targetModes));

    ImGui::Dummy(ImVec2(0.f, 4.f));

    CleanKeybind("Tecla de Mira", &g_cfg.aimKey);

    if (g_cfg.aimKey == 0) ImGui::TextColored(kTextMuted, "Ativacao: Sempre ativo no FOV");

    else if (g_cfg.aimKey == 1) ImGui::TextColored(kTextMuted, "Ativacao: Mouse 1 (Disparo)");

    else if (g_cfg.aimKey == 5) ImGui::TextColored(kTextMuted, "Ativacao: Mouse 5 (Lateral)");

    else ImGui::TextColored(kTextMuted, "Ativacao: VK %d", g_cfg.aimKey);

    EndCleanCard();

}



inline void DrawCombatTrajectoryCard() {

    BeginCleanCard("Trajetoria & Alcance", "GUNPLAY");

    CleanToggle("Curva Bezier", &g_cfg.aimBezier);

    if (g_cfg.aimBezier) CleanSliderFloat("Variacao da Curva", &g_cfg.aimCurveJitter, 0.f, .1f, "%.3f deg");

    CleanToggle("FOV por Distancia", &g_cfg.aimDistanceFov);

    if (g_cfg.aimDistanceFov) {

        CleanSliderFloat("Distancia de Referencia", &g_cfg.aimFovReferenceMetres, 5.f, 60.f, "%.0f m");

        CleanSliderFloat("Escala Minima", &g_cfg.aimFovMinimumScale, .05f, 1.f, "%.2f");

    }

    EndCleanCard();

}



inline void DrawCombatReactionCard() {

    BeginCleanCard("Reacao", "AQUISICAO");

    CleanToggle("Atraso de Reacao", &g_cfg.aimReactionDelay);

    if (g_cfg.aimReactionDelay) {

        float low = static_cast<float>(g_cfg.aimReactionMinMs), high = static_cast<float>(g_cfg.aimReactionMaxMs);

        if (CleanSliderFloat("Atraso Minimo", &low, 0.f, 500.f, "%.0f ms")) g_cfg.aimReactionMinMs = static_cast<int>(low);

        if (CleanSliderFloat("Atraso Maximo", &high, 0.f, 500.f, "%.0f ms")) g_cfg.aimReactionMaxMs = static_cast<int>(high);

        g_cfg.aimReactionMaxMs = std::max(g_cfg.aimReactionMinMs, g_cfg.aimReactionMaxMs);

    }

    EndCleanCard();

}



inline void DrawCombatRecoilCard() {

    BeginCleanCard("Controle de Recoil", "RCS");

    CleanToggle("RCS Ativo", &g_cfg.rcsActive);

    CleanSliderFloat("Compensacao Vertical (Pitch)", &g_cfg.rcsScaleX, 0.0f, 1.5f, "%.2f");

    CleanSliderFloat("Compensacao Horizontal (Yaw)", &g_cfg.rcsScaleY, 0.0f, 1.5f, "%.2f");

    CleanSliderFloat("Suavizacao RCS", &g_cfg.rcsSmooth, 1.f, 15.f, "%.1f");

    CleanSliderFloat("Intensidade RCS", &g_cfg.rcsStrength, 0.f, 1.f, "%.2f");

    float bulletDelay = static_cast<float>(g_cfg.rcsBulletDelay), rampBullets = static_cast<float>(g_cfg.rcsRampBullets);

    if (CleanSliderFloat("Balas Sem Compensacao", &bulletDelay, 0.f, 8.f, "%.0f")) g_cfg.rcsBulletDelay = static_cast<int>(bulletDelay);

    if (CleanSliderFloat("Rampa em Balas", &rampBullets, 0.f, 8.f, "%.0f")) g_cfg.rcsRampBullets = static_cast<int>(rampBullets);

    ImGui::TextWrapped("Compensa o spray mesmo com Aimbot desligado.");

    EndCleanCard();

}



inline void DrawCombatTriggerCard() {

    BeginCleanCard("Triggerbot", "DELAY / RECUPERACAO");

    CleanToggle("Triggerbot Ativo", &g_cfg.triggerActive);

    ImGui::TextUnformatted("Pausa apos eliminacao (ms)");ImGui::SetNextItemWidth(g_cleanCardContentW);ImGui::SliderInt("##aimKillDelayMs",&g_cfg.aimKillDelayMs,0,1000);
    ImGui::TextUnformatted("Pausa ao trocar alvo (ms)");ImGui::SetNextItemWidth(g_cleanCardContentW);ImGui::SliderInt("##aimSwitchDelayMs",&g_cfg.aimSwitchDelayMs,0,1000);


    CleanToggle("Anti-Smoke Estrito", &g_cfg.strictSmoke);
    CleanToggle("Nao Disparar Cego (Flash Check)", &g_cfg.triggerFlashCheck);
    CleanToggle("Nao Disparar no Ar (Air Check)", &g_cfg.triggerAirCheck);

    if (g_cfg.strictSmoke) {

        ImGui::TextWrapped("Consulta densidade volumetrica nativa. Dados ausentes ou antigos bloqueiam o trigger.");

    }

    CleanKeybind("Tecla Triggerbot", &g_cfg.triggerKey);

    if (g_cfg.triggerKey == 0) ImGui::TextColored(kTextMuted, "Ativacao: Automatico (sem tecla)");

    else if (g_cfg.triggerKey == 18) ImGui::TextColored(kTextMuted, "Ativacao: ALT (segurar)");

    else ImGui::TextColored(kTextMuted, "Ativacao: VK %d", g_cfg.triggerKey);

    int mode = static_cast<int>(HazzureCombatRefine::TriggerDelayMode(g_cfg.humanizedTrigger, g_cfg.triggerDistanceDelay));

    const char* modes[] = { "Aleatorio 40-90 ms", "Por Distancia", "Manual" };

    if (CleanCombo("Modo de Delay", &mode, modes, IM_ARRAYSIZE(modes))) {

        g_cfg.humanizedTrigger = mode != 2;

        g_cfg.triggerDistanceDelay = mode == 1;

    }

    if (mode == 0) ImGui::TextWrapped("Sorteia 40-90 ms ao adquirir o alvo; mantem o valor durante a espera.");

    if (mode == 1) ImGui::TextWrapped("60-90 ms perto; 150-220 ms a partir de 30 m.");

    if (mode == 2) CleanSliderFloat("Delay Disparo", &g_cfg.triggerDelayMs, 0.0f, 150.0f, "%.0f ms");

    ImGui::Separator();

    float pause = static_cast<float>(g_cfg.triggerPauseMs);

    if (CleanSliderFloat("Pausa Apos Disparo", &pause, 80.f, 1500.f, "%.0f ms")) g_cfg.triggerPauseMs = static_cast<int>(pause);

    ImGui::TextWrapped("A pausa entre disparos e a recuperacao do recuo sao separadas do delay de aquisicao.");

    ImGui::TextWrapped("Trigger: %s", g_frameDiagnostics.triggerStatus);

    ImGui::TextWrapped("Zoom: %s", g_frameDiagnostics.scopeStatus);

    EndCleanCard();

}



inline void DrawCombatAccuracyCard() {

    BeginCleanCard("Precisao & Pontos", "ESTIMATIVA");

    CleanToggle("Filtro de Hitchance no Trigger", &g_cfg.hitchanceEnabled);

    CleanSliderFloat("Chance Minima Estimada", &g_cfg.hitchancePercent, 0.f, 100.f, "%.0f %%");

    const char* samples[] = { "64 raios", "128 raios", "256 raios" };

    int quality = g_cfg.hitchanceSamples <= 64 ? 0 : g_cfg.hitchanceSamples <= 128 ? 1 : 2;

    if (CleanCombo("Amostras", &quality, samples, IM_ARRAYSIZE(samples))) g_cfg.hitchanceSamples = quality == 0 ? 64 : quality == 1 ? 128 : 256;

    CleanToggle("Multipoint", &g_cfg.aimMultipoint);

    if (g_cfg.aimMultipoint) CleanSliderFloat("Escala dos Pontos", &g_cfg.multipointScale, 0.f, .8f, "%.2f");

    CleanToggle("Preferir Corpo se Cabeca Incerta", &g_cfg.aimBodyFallback);
    CleanToggle("Forcar Body Aim (Baim)", &g_cfg.forceBodyAim);
    if (g_cfg.forceBodyAim) CleanKeybind("Tecla Force Baim", &g_cfg.forceBodyAimKey);
    CleanToggle("Baim on Peek (Alvo em Movimento)", &g_cfg.baimOnPeek);

    if (g_cfg.aimBodyFallback) CleanSliderFloat("Limiar da Cabeca", &g_cfg.bodyFallbackBelow, 0.f, 100.f, "%.0f %%");
    const char* safeModes[] = { "Desativado", "Preferir Safe Point", "Forcar Safe Point" };
    CleanCombo("Safe Point", &g_cfg.safePointMode, safeModes, IM_ARRAYSIZE(safeModes));
    CleanToggle("Extrapolacao por Ping", &g_cfg.aimExtrapolation);
    if (g_cfg.aimExtrapolation) CleanSliderFloat("Escala de Extrapolacao", &g_cfg.aimExtrapolationScale, 0.1f, 3.0f, "%.2fx");

    ImGui::TextWrapped("Dispersao nativa com volumes aproximados dos ossos.");

    if (g_frameDiagnostics.hitchance >= 0) ImGui::TextDisabled("Ultima estimativa: %.1f%%", g_frameDiagnostics.hitchance);

    if (g_cfg.hitchanceEnabled || g_cfg.aimMultipoint || g_cfg.aimBodyFallback || g_cfg.safePointMode > 0) {

        AcquireSRWLockShared(&HazzureWeaponAccuracy::lock);

        auto value = HazzureWeaponAccuracy::snapshot;

        ReleaseSRWLockShared(&HazzureWeaponAccuracy::lock);

        auto now = GetTickCount64();

        ImGui::TextDisabled(value.valid && now >= value.time && now - value.time <= 100 ? "Leitura da arma disponivel" : "Aguardando arma compativel");

    }

    EndCleanCard();

}



inline void DrawCombatBrakeCard() {

    BeginCleanCard("Frenagem do Trigger", "ANTES DO DISPARO");

    CleanToggle("Frear Antes de Disparar", &g_cfg.shotStop);

    CleanToggle("Esperar Repouso Estavel", &g_cfg.shotFullStop);

    CleanToggle("Preservar Apoio na Borda", &g_cfg.shotStopEdgeGuard);

    if (!g_cfg.shotFullStop) CleanSliderFloat("Velocidade Limite", &g_cfg.shotStopSpeed, 10.f, 150.f, "%.0f u/s");

    ImGui::TextWrapped("Contra-strafe por frame. O trigger aguarda a velocidade calculada.");

    if (g_cfg.shotStop) ImGui::TextWrapped("%s", HazzureBrake::Label());

    EndCleanCard();

}

// ============================================================================
// T073 — Aba Aim do Enigma v3 (replaca da GUI da foto): painel RAGE com 11
// rows na ordem exata do briefing + preview WHERE IT AIMS. Bindings reais:
// rageActive, rageFov, espFovCircle, hitchanceEnabled/Percent, aimTargetMode,
// aimBone, multipointScale, rageBacktrack(+Ticks), backtrackPlausibleOnly,
// backtrackAdaptive, rageMinimumDamage, autowallChancePenalty, rageKey e o
// override por classe de arma g_weaponClasses["pistols"]["rageActive"].
// ============================================================================

static bool EnigmaWeaponRowState() {
    auto pit = g_weaponClasses.find("pistols");
    if (pit == g_weaponClasses.end()) return false;
    auto vit = pit->second.find("rageActive");
    return vit != pit->second.end() && vit->second == "1";
}

static void EnigmaEraseWeaponOverride() {
    auto pit = g_weaponClasses.find("pistols");
    if (pit != g_weaponClasses.end()) pit->second.erase("rageActive");
}

static void EnigmaSectionReset() {
    g_cfg.rageActive = true;
    g_cfg.rageFov = 94.1f;
    g_cfg.espFovCircle = true;
    g_cfg.hitchanceEnabled = true;
    g_cfg.hitchancePercent = 60.f;
    g_cfg.rageMinimumDamage = 30.f;
    g_cfg.autowallChancePenalty = 30.f;
    g_cfg.rageTargetsFirst = 0;
    g_cfg.aimTargetMode = 0;
    g_cfg.safePointMode = 0;
    g_cfg.rageBacktrack = false;
    g_cfg.rageBacktrackTicks = 6;
    g_cfg.backtrackPlausibleOnly = true;
    g_cfg.backtrackAdaptive = true;
    EnigmaEraseWeaponOverride();
}

static void EnigmaDivider(float w) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + .5f), ImVec2(p.x + w, p.y + .5f), GSWidgets::IM_COL(EN_DIVIDER), 1.f);
    ImGui::Dummy(ImVec2(w, 1.f));
}

// Row de 38px com toggle 34x18 a direita.
static void EnigmaToggleRow(const char* label, const char* id, bool* v, float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 tsz = ImGui::CalcTextSize(label);
    d->AddText(ImVec2(p.x, p.y + (38.f - tsz.y) * .5f), GSWidgets::IM_COL(EN_TEXT), label);
    ImGui::Dummy(ImVec2(panelW, 38.f));
    ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 34.f, p.y + 10.f));
    GSWidgets::GS_EnToggle(id, v);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

// Row de 38px com combo fixo 170x26 a direita.
static void EnigmaComboRow(const char* label, int* v, const char* const items[], int count, float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 tsz = ImGui::CalcTextSize(label);
    d->AddText(ImVec2(p.x, p.y + (38.f - tsz.y) * .5f), GSWidgets::IM_COL(EN_TEXT), label);
    ImGui::Dummy(ImVec2(panelW, 38.f));
    ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 170.f, p.y + 6.f));
    GSWidgets::GS_EnCombo(label, v, items, count);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

// Row de 38px com o slider (label + valor + trilho) ocupando a largura toda.
static void EnigmaSliderRow(const char* label, float* v, float low, float high, const char* fmt, float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 6.f));
    GSWidgets::GS_EnSlider(label, v, low, high, fmt);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

// Row expansivel de 38px (caret + label + valor a direita); sub-rows ficam
// indentados 24px quando aberta.
static void EnigmaExpandRow(const char* label, const char* value, bool* open, float panelW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 7.f));
    GSWidgets::GS_EnExpandRow(label, value, open);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
    ImGui::Dummy(ImVec2(panelW, 0.f));
}

inline void DrawEnigmaAimPanel(float panelW) {
    static bool s_openDamage = false, s_openPoints = false, s_openHistory = false, s_openAny = false;
    if (GSWidgets::GS_EnSectionBar("Rage", panelW, "##enAimReset")) EnigmaSectionReset();
    EnigmaDivider(panelW);
    ImGui::Dummy(ImVec2(panelW, 6.f));

    // Row 1 — master; as demais escurecem 0.45 e nao recebem clique com ele OFF.
    EnigmaToggleRow("Rage aim", "##enRageAim", &g_cfg.rageActive, panelW);
    const bool dim = !g_cfg.rageActive;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dim ? .45f : 1.f);
    ImGui::BeginDisabled(dim);

    // Row 2 — "Rage for these weapons" + reset + chip BIND + toggle (override
    // real de g_weaponClasses["pistols"]["rageActive"]).
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* d = ImGui::GetWindowDrawList();
        const char* lbl = "Rage for these weapons";
        ImVec2 tsz = ImGui::CalcTextSize(lbl);
        d->AddText(ImVec2(p.x, p.y + (38.f - tsz.y) * .5f), GSWidgets::IM_COL(EN_TEXT), lbl);
        ImGui::Dummy(ImVec2(panelW, 38.f));
        const bool wOn = EnigmaWeaponRowState();
        bool wWant = wOn;
        ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 34.f, p.y + 10.f));
        if (GSWidgets::GS_EnToggle("##enWeaponsRage", &wWant) && wWant != wOn) {
            if (wWant) g_weaponClasses["pistols"]["rageActive"] = "1";
            else EnigmaEraseWeaponOverride();
        }
        char bindLbl[32]{};
        const char* chipTxt = (g_bindingTarget == &g_cfg.rageKey) ? "..."
            : (g_cfg.rageKey == 0 ? "BIND" : KeyLabel(g_cfg.rageKey, bindLbl, sizeof(bindLbl)));
        const float chipW = ImGui::CalcTextSize(chipTxt).x + 16.f;
        ImGui::SetCursorScreenPos(ImVec2(p.x + panelW - 34.f - 8.f - chipW, p.y + 10.f));
        GSWidgets::GS_EnChip("##enWeaponsBind", &g_cfg.rageKey);
        const float rstX = p.x + panelW - 34.f - 8.f - chipW - 8.f - 16.f;
        ImGui::PushID("##enWeaponsReset");
        ImGui::SetCursorScreenPos(ImVec2(rstX, p.y + 11.f));
        ImGui::InvisibleButton("##rst", ImVec2(16.f, 16.f));
        if (ImGui::IsItemClicked()) EnigmaEraseWeaponOverride();
        GSWidgets::GS_NavIcon(d, 19, ImVec2(rstX, p.y + 11.f), GSWidgets::IM_COL(ImGui::IsItemHovered() ? EN_TEXT : EN_TEXT_DIM));
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 38.f));
        ImGui::Dummy(ImVec2(panelW, 0.f));
    }

    // Row 3 — Field of view, 94.1 graus por padrao.
    EnigmaSliderRow("Field of view", &g_cfg.rageFov, .5f, 360.f, "%.1f\xC2\xB0", panelW);

    // Row 4 — Draw field of view.
    EnigmaToggleRow("Draw field of view", "##enDrawFov", &g_cfg.espFovCircle, panelW);

    // Row 5 — Targets first.
    static const char* kTargetsFirst[] = { "Nearest", "Lowest health", "Highest damage", "Crosshair" };
    EnigmaComboRow("Targets first", &g_cfg.rageTargetsFirst, kTargetsFirst, 4, panelW);

    // Row 6 — Fires when (mestre do hitchance).
    static const char* kFiresWhen[] = { "Hit chance is reached", "Key pressed" };
    int firesWhen = g_cfg.hitchanceEnabled ? 0 : 1;
    EnigmaComboRow("Fires when", &firesWhen, kFiresWhen, 2, panelW);
    g_cfg.hitchanceEnabled = (firesWhen == 0);

    // Row 7 — Hit chance, 60% por padrao.
    EnigmaSliderRow("Hit chance", &g_cfg.hitchancePercent, 0.f, 100.f, "%.0f%%", panelW);

    // Row 8 — Damage (expansivel).
    {
        char dmgVal[96];
        snprintf(dmgVal, sizeof(dmgVal), "Minimum damage %.0f \xC2\xB7 Through walls %.0f", g_cfg.rageMinimumDamage, g_cfg.autowallChancePenalty);
        EnigmaExpandRow("Damage", dmgVal, &s_openDamage, panelW);
        if (s_openDamage) {
            ImGui::Indent(24.f);
            EnigmaSliderRow("Minimum damage", &g_cfg.rageMinimumDamage, 0.f, 100.f, "%.0f", panelW - 24.f);
            EnigmaSliderRow("Through walls", &g_cfg.autowallChancePenalty, 0.f, 100.f, "%.0f", panelW - 24.f);
            ImGui::Unindent(24.f);
        }
    }

    // Row 9 — Points on a part (expansivel, valor "Auto").
    {
        EnigmaExpandRow("Points on a part", "Auto", &s_openPoints, panelW);
        if (s_openPoints) {
            ImGui::Indent(24.f);
            static const char* kPointModes[] = { "Auto", "Closest bone", "Head fallback" };
            EnigmaComboRow("Selection", &g_cfg.aimTargetMode, kPointModes, 3, panelW - 24.f);
            static const char* kBones[] = { "Head (Bone 6)", "Neck (Bone 5)", "Chest (Bone 4)", "Pelvis (Bone 0)" };
            EnigmaComboRow("Bone", &g_cfg.aimBone, kBones, 4, panelW - 24.f);
            EnigmaSliderRow("Multipoint scale", &g_cfg.multipointScale, 0.f, .8f, "%.2f", panelW - 24.f);
            ImGui::Unindent(24.f);
        }
    }

    // Row 10 — History (expansivel).
    {
        EnigmaExpandRow("History", "backtrack, plausible only, adaptive", &s_openHistory, panelW);
        if (s_openHistory) {
            ImGui::Indent(24.f);
            EnigmaToggleRow("Backtrack", "##enBacktrack", &g_cfg.rageBacktrack, panelW - 24.f);
            float ticks = static_cast<float>(g_cfg.rageBacktrackTicks);
            ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 6.f));
            GSWidgets::GS_EnSlider("Backtrack ticks", &ticks, 1.f, 12.f, "%.0f");
            g_cfg.rageBacktrackTicks = static_cast<int>(ticks);
            ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 6.f));
            ImGui::Dummy(ImVec2(panelW - 24.f, 0.f));
            EnigmaToggleRow("Plausible only", "##enPlausible", &g_cfg.backtrackPlausibleOnly, panelW - 24.f);
            EnigmaToggleRow("Adaptive", "##enAdaptive", &g_cfg.backtrackAdaptive, panelW - 24.f);
            ImGui::Unindent(24.f);
        }
    }

    // Row 11 — For any weapon (expansivel, valor vazio): replica os valores
    // atuais para todas as classes de arma.
    {
        EnigmaExpandRow("For any weapon", "", &s_openAny, panelW);
        if (s_openAny) {
            ImGui::Indent(24.f);
            static bool sApplyAll = false;
            EnigmaToggleRow("Copy to all weapon classes", "##enApplyAll", &sApplyAll, panelW - 24.f);
            if (sApplyAll) {
                char fov[24];
                snprintf(fov, sizeof(fov), "%.1f", g_cfg.rageFov);
                for (const char* cls : { "rifles", "pistols", "snipers" }) {
                    g_weaponClasses[cls]["rageActive"] = "1";
                    g_weaponClasses[cls]["aimFov"] = fov;
                }
                sApplyAll = false;
            }
            ImGui::Unindent(24.f);
        }
    }

    ImGui::EndDisabled();
    ImGui::PopStyleVar();
}

// Preview WHERE IT AIMS: gradiente radial #5A1010 -> #140A0A -> #101012,
// silhueta CT ~260px alinhada a base, 7 marcadores de cruz brancos, hint no
// canto superior esquerdo, texto central no peito e rodape "No part picked".
inline void DrawEnigmaPreview(float previewW, float previewH) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float headerH = 34.f, footerH = 34.f;
    const float bodyY = p.y + headerH;
    const float bodyH = std::max(120.f, previewH - headerH - footerH);

    // Header 34px: hamburger + titulo maiusculo.
    GSWidgets::GS_NavIcon(d, 17, ImVec2(p.x + 12.f, bodyY - headerH + 9.f), GSWidgets::IM_COL(EN_TEXT_DIM));
    d->AddText(ImVec2(p.x + 38.f, bodyY - headerH + 9.f), GSWidgets::IM_COL(EN_TEXT), "WHERE IT AIMS");
    d->AddLine(ImVec2(p.x, bodyY - .5f), ImVec2(p.x + previewW, bodyY - .5f), GSWidgets::IM_COL(EN_DIVIDER), 1.f);

    // Corpo: gradiente radial (anel de elipses do centro 55% pra fora).
    const ImVec2 center(p.x + previewW * .5f, bodyY + bodyH * .55f);
    const float maxR = std::max(previewW, bodyH) * .78f;
    constexpr int rings = 28;
    for (int i = rings; i >= 0; --i) {
        const float t = static_cast<float>(i) / static_cast<float>(rings);
        float rC, gC, bC;
        if (t < .45f) {
            const float k = t / .45f;
            rC = .353f + (.078f - .353f) * k; gC = .063f + (.039f - .063f) * k; bC = .063f + (.039f - .063f) * k;
        } else {
            const float k = (t - .45f) / .55f;
            rC = .078f + (.063f - .078f) * k; gC = .039f; bC = .039f + (.071f - .039f) * k;
        }
        d->AddEllipseFilled(center, ImVec2(maxR * t, maxR * t * .92f), IM_COL32(int(rC * 255.f), int(gC * 255.f), int(bC * 255.f), 255), 0, 48);
    }

    // Silhueta CT ~260px alinhada a base, centro horizontal.
    const ImU32 model = GSWidgets::IM_COL(EN_MODEL);
    const float cx = center.x;
    const float baseY = bodyY + bodyH;
    const float topY = baseY - 260.f;
    const auto limb = [&](ImVec2 a, ImVec2 b, float wdt) {
        d->AddLine(a, b, model, wdt);
        d->AddCircleFilled(a, wdt * .5f, model, 12);
        d->AddCircleFilled(b, wdt * .5f, model, 12);
    };
    d->AddCircleFilled(ImVec2(cx, topY + 18.f), 13.f, model, 24);
    d->AddRectFilled(ImVec2(cx - 3.f, topY + 29.f), ImVec2(cx + 3.f, topY + 44.f), model, 2.f);
    d->AddRectFilled(ImVec2(cx - 26.f, topY + 44.f), ImVec2(cx + 26.f, topY + 124.f), model, 8.f);
    d->AddRectFilled(ImVec2(cx - 20.f, topY + 124.f), ImVec2(cx + 20.f, topY + 142.f), model, 4.f);
    limb(ImVec2(cx - 24.f, topY + 50.f), ImVec2(cx - 46.f, topY + 108.f), 11.f);
    limb(ImVec2(cx - 46.f, topY + 108.f), ImVec2(cx - 40.f, topY + 158.f), 9.f);
    limb(ImVec2(cx + 24.f, topY + 50.f), ImVec2(cx + 46.f, topY + 108.f), 11.f);
    limb(ImVec2(cx + 46.f, topY + 108.f), ImVec2(cx + 40.f, topY + 158.f), 9.f);
    limb(ImVec2(cx - 12.f, topY + 142.f), ImVec2(cx - 16.f, topY + 200.f), 12.f);
    limb(ImVec2(cx - 16.f, topY + 200.f), ImVec2(cx - 16.f, topY + 254.f), 10.f);
    limb(ImVec2(cx + 12.f, topY + 142.f), ImVec2(cx + 16.f, topY + 200.f), 12.f);
    limb(ImVec2(cx + 16.f, topY + 200.f), ImVec2(cx + 16.f, topY + 254.f), 10.f);
    d->AddRectFilled(ImVec2(cx - 26.f, topY + 254.f), ImVec2(cx - 6.f, topY + 260.f), model, 2.f);
    d->AddRectFilled(ImVec2(cx + 6.f, topY + 254.f), ImVec2(cx + 26.f, topY + 260.f), model, 2.f);

    // 7 marcadores de mira: cruz 9px branca, opacidade .85.
    const ImU32 mark = IM_COL32(255, 255, 255, 217);
    const ImVec2 marks[7] = {
        ImVec2(cx, topY + 18.f),
        ImVec2(cx, topY + 78.f),
        ImVec2(cx, topY + 132.f),
        ImVec2(cx - 34.f, topY + 58.f),
        ImVec2(cx + 34.f, topY + 58.f),
        ImVec2(cx - 16.f, topY + 224.f),
        ImVec2(cx + 16.f, topY + 224.f)
    };
    for (const ImVec2& m : marks) {
        d->AddLine(ImVec2(m.x - 4.5f, m.y), ImVec2(m.x + 4.5f, m.y), mark, 1.4f);
        d->AddLine(ImVec2(m.x, m.y - 4.5f), ImVec2(m.x, m.y + 4.5f), mark, 1.4f);
    }

    // Texto central no peito com sombra.
    {
        const char* msg = "Turn on Rage aim, and rage for this weapon";
        ImVec2 msz = ImGui::CalcTextSize(msg);
        const ImVec2 tp(center.x - msz.x * .5f, topY + 90.f - msz.y * .5f);
        d->AddText(ImVec2(tp.x + 1.f, tp.y + 1.f), IM_COL32(0, 0, 0, 180), msg);
        d->AddText(tp, IM_COL32(255, 255, 255, 255), msg);
    }

    // Hint no canto superior esquerdo (margem 12px, largura max 170px, quebra
    // manual em duas linhas).
    {
        const float hx = p.x + 12.f, hy = bodyY + 12.f;
        d->AddCircle(ImVec2(hx + 7.f, hy + 7.f), 7.f, GSWidgets::IM_COL(EN_TEXT_DIM), 16, 1.2f);
        char num[2] = { '1', 0 };
        ImVec2 nsz = ImGui::CalcTextSize(num);
        d->AddText(ImVec2(hx + 7.f - nsz.x * .5f, hy + 7.f - nsz.y * .5f), GSWidgets::IM_COL(EN_TEXT_DIM), num);
        const char* bold = "Rage for these weapons";
        const char* line1 = "Turn on Rage for these weapons";
        const char* line2 = "to tune this weapon up";
        ImVec2 bsz = ImGui::CalcTextSize(bold);
        d->AddText(ImVec2(hx + 22.f, hy), IM_COL32(255, 255, 255, 255), bold);
        d->AddText(ImVec2(hx + 22.f, hy + bsz.y + 2.f), GSWidgets::IM_COL(EN_TEXT_MUTE), line1);
        d->AddText(ImVec2(hx + 22.f, hy + bsz.y + 2.f + 13.f), GSWidgets::IM_COL(EN_TEXT_MUTE), line2);
    }

    // Rodape 34px.
    const float footY = bodyY + bodyH;
    d->AddLine(ImVec2(p.x, footY - .5f), ImVec2(p.x + previewW, footY - .5f), GSWidgets::IM_COL(EN_DIVIDER), 1.f);
    d->AddText(ImVec2(p.x + 12.f, footY + (footerH - ImGui::CalcTextSize("No part picked").y) * .5f), GSWidgets::IM_COL(EN_TEXT_MUTE), "No part picked");

    ImGui::SetCursorScreenPos(ImVec2(p.x, footY + footerH));
    ImGui::Dummy(ImVec2(previewW, 0.f));
}





