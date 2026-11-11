import sys

path = r'C:\Codex2\gpt\HazzureInternal\src\GSLayout.inl'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update DrawMiscellaneousMemeSense
target_misc = '''static void DrawMiscellaneousMemeSense(int column, Shared* /*shared*/) {
    if (column == 0) {
        GS_SectionHeader("Interface");
        GS_Toggle("In-game radar", &g_cfg.nativeRadar);
        GS_Toggle("Preserve kill-feed", &g_cfg.killfeedEnabled);
        GS_SectionHeader("Movement");
        GS_Toggle("Bhop", &g_cfg.bhopActive);
        if (ImGui::CollapsingHeader("Movement options")) {
            DrawMovementBasicCard();
            DrawAdvancedMovementControls();
            DrawMovementAutomationCard();
        }
    } else {
        DrawEnvironmentAudioCard();
        if (ImGui::Button("Unload cheat", ImVec2(ImGui::GetContentRegionAvail().x, 28.0f)))
            HazzureState::g_unloadRequested.store(true);
        HAZZURE_OBSERVE_CONTROL("Unload cheat", &HazzureState::g_unloadRequested);
    }
}'''

repl_misc = '''static void DrawMiscellaneousMemeSense(int column, Shared* /*shared*/) {
    if (column == 0) {
        GS_SectionHeader("Movement (Bhop & Air Boost)");
        GS_Toggle("Auto Bunnyhop", &g_cfg.bhopActive);
        GS_Toggle("Air Boost (Aceleracao Maxima Valve)", &g_cfg.bhopBoost);
        GS_Toggle("Auto-Strafe", &g_cfg.autoStrafe);
        GS_Toggle("Auto-Strafe Direcional (WASD)", &g_cfg.autoStrafePhysics);
        ImGui::Spacing();
        GS_SectionHeader("Assistencia Avancada");
        DrawMovementBasicCard();
        DrawAdvancedMovementControls();
        DrawMovementAutomationCard();
        ImGui::Spacing();
        GS_SectionHeader("Velocimetro & Grafico");
        GS_Toggle("Exibir Velocimetro (Speedometer)", &g_cfg.showSpeedometer);
        GS_Toggle("Exibir Grafico de Movimento", &g_cfg.showMovementGraph);
    } else {
        GS_SectionHeader("Interface & Radar");
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
}'''

assert target_misc in text, 'target_misc not found'
text = text.replace(target_misc, repl_misc, 1)

# 2. Add Silent Aim toggle in Rage Aim
target_rage_aim = '''                    GS_Toggle("Ignore Smoke", &g_cfg.rageIgnoreSmoke);
                    GS_Toggle("Auto-fire", &g_cfg.rageAutoFire);'''

repl_rage_aim = '''                    GS_Toggle("Ignore Smoke", &g_cfg.rageIgnoreSmoke);
                    GS_Toggle("Auto-fire", &g_cfg.rageAutoFire);
                    GS_Toggle("Silent Aim", &g_cfg.rageSilentAim);'''

assert target_rage_aim in text, 'target_rage_aim not found'
text = text.replace(target_rage_aim, repl_rage_aim, 1)

# 3. Clarify Pitch slider label in Anti-Aim
target_aa_pitch = '''                    GS_SliderFloat("Pitch", &g_cfg.rageAntiAimPitch, -89.0f, 89.0f, "%.0f deg");'''
repl_aa_pitch = '''                    GS_SliderFloat("Pitch (+89 = Chao/Guardado)", &g_cfg.rageAntiAimPitch, -89.0f, 89.0f, "%.0f deg");'''

assert target_aa_pitch in text, 'target_aa_pitch not found'
text = text.replace(target_aa_pitch, repl_aa_pitch, 1)

with open(path, 'w', encoding='utf-8', newline='') as f:
    f.write(text)
print('GSLayout.inl ok')
