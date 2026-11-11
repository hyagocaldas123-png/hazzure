#pragma once
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

// HazzureInternal/src/cards/CardEnvironment.inl — Frente 2.4 Modularizacao UI Enigma CS2
// Escopo: Camera, Aspect Ratio, Viewmodel, Particulas, Clima 3D, Skybox, Color Grading, Audio DSP


inline void DrawEnvironmentCameraCard() {
    BeginCleanCard("Camera & Viewmodel", "AJUSTES");
    CleanToggle("Alterar FOV da Camera", &g_cfg.cameraFovOverride);
    CleanSliderFloat("FOV Geral (Camera)", &g_cfg.cameraFov, 60.f, 140.f, "%.0f deg");
    ImGui::TextWrapped("Campo de visao do mundo. O zoom das snipers continua nativo.");
    if (g_cfg.cameraFovOverride) {
        ImGui::TextWrapped("%s", HazzureCamera::Label(GetTickCount64()));
        auto last = HazzureCamera::lastDelivery.load(); auto now = GetTickCount64();
        if (last && now >= last && now - last <= 1000) ImGui::TextDisabled("FOV entregue: %.1f deg", HazzureCamera::delivered.load());
    }
    ImGui::Separator();
    CleanToggle("Forcar Proporcao de Tela", &g_cfg.aspectRatioOverride);
    if (g_cfg.aspectRatioOverride) {
        CleanSliderFloat("Proporcao de Tela", &g_cfg.aspectRatio, 0.5f, 2.5f, "%.4f");
        if (ImGui::Button("4:3")) { g_cfg.aspectRatio = 4.f / 3.f; } ImGui::SameLine();
        if (ImGui::Button("16:9")) { g_cfg.aspectRatio = 16.f / 9.f; } ImGui::SameLine();
        if (ImGui::Button("16:10")) { g_cfg.aspectRatio = 16.f / 10.f; } ImGui::SameLine();
        if (ImGui::Button("5:4")) { g_cfg.aspectRatio = 5.f / 4.f; }
    }
    ImGui::Separator();
    CleanToggle("Viewmodel Personalizado", &g_cfg.customViewmodel);
    if (g_cfg.customViewmodel) {
        CleanSliderFloat("Arma X", &g_cfg.viewmodelX, -10.f, 10.f, "%.2f");
        CleanSliderFloat("Arma Y", &g_cfg.viewmodelY, -10.f, 10.f, "%.2f");
        CleanSliderFloat("Arma Z", &g_cfg.viewmodelZ, -10.f, 10.f, "%.2f");
        CleanSliderFloat("FOV da Arma", &g_cfg.viewmodelFov, 40.f, 120.f, "%.0f deg");
    }
    EndCleanCard();
}

inline void DrawEnvironmentStabilityCard() {
    BeginCleanCard("Misc / Estabilidade", "VISUAL");
    CleanToggle("Suprimir Recuo Visual", &g_cfg.noVisualRecoil);
    ImGui::TextWrapped("Camera e deslocamento por punch. RCS independente.");
    ImGui::BeginDisabled(); CleanToggle("Stream-Proof Interno", &g_cfg.streamProof); ImGui::EndDisabled();
    ImGui::TextWrapped("Indisponivel no overlay interno.");
    EndCleanCard();
}

inline void DrawEnvironmentParticlesCard(Shared* shared) {
    BeginCleanCard("Particulas & Otimizacao", "WORLD RENDERING");
    CleanToggle("Particulas de Neve na Tela", &g_cfg.snowParticles);
    g_cfg.cleanMap = false;
    ImGui::BeginDisabled(); CleanToggle("Clean Map (Sem Sombras)", &g_cfg.cleanMap); ImGui::EndDisabled();
    ImGui::TextDisabled("Sombras: campo nao validado nesta build");
    CleanToggle("Remover Clima Original", &g_cfg.removeOriginalWeather);




    if (shared) shared->cleanMap = g_cfg.cleanMap ? 1 : 0;
    ImGui::Separator();
    ImGui::TextWrapped("Cores nativas de particulas indisponiveis nesta build; contrato de recursos ainda nao validado.");
    ImGui::BeginDisabled();
    CleanColorRow("Cor da Fumaca", g_cfg.colSmoke);
    CleanColorRow("Cor do Molotov / Fogo", g_cfg.colMolotov);
    CleanColorRow("Cor do Disparo / Muzzle", g_cfg.colMuzzle);
    ImGui::EndDisabled();
    EndCleanCard();
}

inline void DrawEnvironmentWeather3DCard() {
    BeginCleanCard("Clima 3D no Mundo", "PRECIPITACAO GEOMETRICA");
    CleanToggle("Clima 3D Ativo", &g_cfg.weather3d);
    if (g_cfg.weather3d) {
        const char* weatherTypes[] = { "Chuva", "Neve", "Brasas (Embers)" };
        CleanCombo("Tipo de Precipitacao", &g_cfg.weatherKind, weatherTypes, 3);
        float countF = static_cast<float>(g_cfg.weatherCount);
        if (CleanSliderFloat("Particulas Ativas", &countF, 16.f, 192.f, "%.0f")) g_cfg.weatherCount = static_cast<int>(countF);
        CleanSliderFloat("Velocidade da Queda", &g_cfg.weatherSpeed, 80.f, 1000.f, "%.0f u/s");
        CleanSliderFloat("Vento X", &g_cfg.weatherWindX, -160.f, 160.f, "%.0f");
        CleanSliderFloat("Vento Y", &g_cfg.weatherWindY, -160.f, 160.f, "%.0f");
        ImGui::TextWrapped("Particulas geradas no espaco do mundo com verificacao de geometria.");
    }
    EndCleanCard();
}

inline void DrawEnvironmentSkyboxAtmosphereCard(Shared* shared) {
    BeginCleanCard("Skybox & Atmosfera", "SOURCE 2 VPK");
    bool wasNight = g_cfg.nightMode;
    CleanToggle("Modo Noturno Completo", &g_cfg.nightMode);
    if (g_cfg.nightMode && !wasNight) {
        g_cfg.darkWorld = true; g_cfg.skyActive = true;
        g_cfg.skyBrightness = 0.62f; g_cfg.worldBrightness = 0.65f;
        g_cfg.fogDensity = 0.06f;
        g_cfg.sceneColorEnabled = true; g_cfg.sceneSaturation = 0.6f;
        g_cfg.sceneTemperature = -0.3f; g_cfg.sceneColorCurve = 1;
        g_cfg.sceneContrast = 1.f; g_cfg.sceneTint = 0.f;
    }
    if (!g_cfg.nightMode && wasNight) {
        g_cfg.darkWorld = false; g_cfg.skyActive = false;
        g_cfg.skyBrightness = 0.7f; g_cfg.worldBrightness = 1.f;
        g_cfg.fogDensity = 0.f; g_cfg.sceneColorEnabled = false;
        g_cfg.sceneSaturation = 1.f; g_cfg.sceneTemperature = 0.f;
        g_cfg.sceneColorCurve = 0; g_cfg.sceneContrast = 1.f; g_cfg.sceneTint = 0.f;
    }
    ImGui::Separator();
    static const char* kSkyboxNames[] = {
        "Nublado Escuro (Overcast)", "Preto Noturno (Sky Black)", "Tempestade Noturna (Rain Night)",
        "Dust 2 Deserto (Daylight)", "Mirage Crepusculo (Afternoon)", "Nuke Industrial (Dusk)",
        "Vertigo Altitude (Morning)", "Office Inverno Nevado (Winter)", "Italy Sunset (Por do Sol)",
        "Daylight HDR (Sol Puro)"
    };
    int skyIdx = shared ? shared->skyboxIndex : 0;
    if (CleanCombo("Seletor de Skybox", &skyIdx, kSkyboxNames, 10)) {
        if (shared) { shared->skyboxIndex = skyIdx; shared->climateFlags |= 1; }
        g_cfg.skyboxIndex = skyIdx;
    }
    if (shared) {
        bool skyOn = (shared->climateFlags & 1) != 0;
        if (CleanToggle("Modulacao de Ceu Ativa", &skyOn)) {
            if (skyOn) shared->climateFlags |= 1; else shared->climateFlags &= ~1;
        }
        if (skyOn) CleanSliderFloat("Brilho do Ceu", &shared->skyBrightness, 0.05f, 1.5f, "%.2f");

        bool darkOn = (shared->climateFlags & 2) != 0;
        if (CleanToggle("Cenario Escuro (Night Mode)", &darkOn)) {
            if (darkOn) { shared->climateFlags |= 2; shared->worldBrightness = 0.25f; }
            else { shared->climateFlags &= ~2; }
        }
        if (darkOn) CleanSliderFloat("Exposicao Cenario", &shared->worldBrightness, 0.05f, 1.0f, "%.2f");
    }
    ImGui::Separator();
    CleanSliderFloat("Densidade (0 = Original)", &g_cfg.fogDensity, 0.0f, 1.0f, "%.2f");



    CleanColorRow("Cor da Nevoa (Fog Tint)", g_cfg.colFog);
    if (shared) {
        shared->fogDensity = g_cfg.fogDensity;
        shared->fogColour = Float4ToIpcCol(g_cfg.colFog);
    }
    EndCleanCard();
}

inline void DrawEnvironmentClimatePresetCard() {
    BeginCleanCard("Climate Preset", "PIPELINE");
    CleanToggle("Climate Ativo", &g_cfg.climateActive);
    if (g_cfg.climateActive) {
        const char* presets[] = { "Pink Violet", "Cold", "Mono", "Custom" };
        CleanCombo("Preset", &g_cfg.climatePreset, presets, 4);
        if (g_cfg.climatePreset == 3) {
            CleanSliderFloat("Hue Shift", &g_cfg.climateHueShift, 0.f, 360.f, "%.0f deg");
            CleanSliderFloat("Saturacao Boost", &g_cfg.climateSatBoost, -100.f, 100.f, "%.0f");
            CleanSliderFloat("Valor Boost", &g_cfg.climateValBoost, -50.f, 50.f, "%.0f");
        }
        CleanSliderFloat("Bloom", &g_cfg.climateBloom, 0.f, 2.f, "%.2f");
        CleanSliderFloat("Sombras", &g_cfg.climateShadowStr, 0.f, 2.f, "%.2f");
        const char* tonemaps[] = { "Linear", "Filmic", "ACES", "Reinhard" };
        CleanCombo("Tonemap", &g_cfg.climateTonemap, tonemaps, 4);
    }
    EndCleanCard();
}

inline void DrawEnvironmentColorGradingCard() {
    BeginCleanCard("Pos-Processamento", "COLOR GRADING");
    CleanToggle("Correcao de Cor da Cena", &g_cfg.sceneColorEnabled);
    if (g_cfg.sceneColorEnabled) {
        CleanSliderFloat("Saturacao", &g_cfg.sceneSaturation, 0.f, 2.f, "%.2f");
        CleanSliderFloat("Contraste", &g_cfg.sceneContrast, .5f, 2.f, "%.2f");
        CleanSliderFloat("Temperatura", &g_cfg.sceneTemperature, -1.f, 1.f, "%.2f");
        CleanSliderFloat("Tint Verde / Magenta", &g_cfg.sceneTint, -1.f, 1.f, "%.2f");
        const char* curves[] = { "Linear", "Filmic" };
        CleanCombo("Curva de Tons", &g_cfg.sceneColorCurve, curves, 2);
        if (ImGui::Button("Restaurar Cores Neutras")) {
            g_cfg.sceneSaturation = g_cfg.sceneContrast = 1.f;
            g_cfg.sceneTemperature = g_cfg.sceneTint = 0.f;
            g_cfg.sceneColorCurve = 0;
        }
        ImGui::TextWrapped("%s", HazzureSceneColor::scenePass.Status());
    }
    EndCleanCard();
}

inline void DrawEnvironmentAudioCard() {
    BeginCleanCard("Audio DSP", "44.1 KHZ");
    CleanToggle("Audio Procedural", &g_cfg.proceduralAudio);
    CleanToggle("Som de Acerto", &g_cfg.hitSound);
    CleanCombo("Preset de Acerto",&g_cfg.hitSoundPreset,kHitPresetNames,IM_ARRAYSIZE(kHitPresetNames));
    ImGui::TextWrapped("Presets sintetizados originais. Um WAV personalizado valido tem prioridade.");
    CleanToggle("Som de Eliminacao", &g_cfg.killSound);
    CleanToggle("Hitmarker na Mira", &g_cfg.showHitmarker);
    CleanToggle("Beep Acustico da C4", &g_cfg.bombBeepAudio);
    CleanSliderFloat("Volume", &g_cfg.soundVolume, 0.0f, 1.0f, "%.2f");

    ImGui::Separator();
    ImGui::TextUnformatted("Sons personalizados (WAV)");
    ImGui::TextWrapped("Coloque o arquivo em C:\\HazzureKernel\\sounds e informe so o nome (ex.: hit.wav). Caminho absoluto tambem funciona.");
    if (ImGui::Button("Abrir pasta de sons", ImVec2(g_cleanCardContentW, 22.f))) {
        std::error_code ec;
        std::filesystem::create_directories(kSoundsDir, ec);
        ShellExecuteA(nullptr, "open", kSoundsDir, nullptr, nullptr, SW_SHOWNORMAL);
    }

    CleanToggle("WAV de acerto", &g_cfg.customHitSound);
    if (g_cfg.customHitSound) {
        ImGui::TextUnformatted("Arquivo de acerto");
        ImGui::SetNextItemWidth(g_cleanCardContentW);
        ImGui::InputText("##hitSoundFile", g_cfg.hitSoundFile, sizeof(g_cfg.hitSoundFile));
    }
    CleanToggle("WAV de headshot", &g_cfg.customHeadshotSound);
    if (g_cfg.customHeadshotSound) {
        ImGui::TextUnformatted("Arquivo de headshot");
        ImGui::SetNextItemWidth(g_cleanCardContentW);
        ImGui::InputText("##headshotSoundFile", g_cfg.headshotSoundFile, sizeof(g_cfg.headshotSoundFile));
    }
    CleanToggle("WAV de eliminacao", &g_cfg.customKillSound);
    if (g_cfg.customKillSound) {
        ImGui::TextUnformatted("Arquivo de eliminacao");
        ImGui::SetNextItemWidth(g_cleanCardContentW);
        ImGui::InputText("##killSoundFile", g_cfg.killSoundFile, sizeof(g_cfg.killSoundFile));
    }

    if (ImGui::Button("Carregar / recarregar WAVs", ImVec2(g_cleanCardContentW, 22.f))) RefreshCustomAudio(true);
    ImGui::TextWrapped("Acerto: %s", CustomAudioStatus(HazzureCustomAudio::Kind::Hit));
    ImGui::TextWrapped("Headshot: %s", CustomAudioStatus(HazzureCustomAudio::Kind::Headshot));
    ImGui::TextWrapped("Eliminacao: %s", CustomAudioStatus(HazzureCustomAudio::Kind::Kill));

    ImGui::Dummy(ImVec2(0.f, 4.f));
    if (ImGui::Button("Testar Som Headshot", ImVec2(g_cleanCardContentW, 22.f))) TriggerHitmarker(true);
    if (ImGui::Button("Testar Som Bodyshot", ImVec2(g_cleanCardContentW, 22.f))) TriggerHitmarker(false);
    EndCleanCard();
}


