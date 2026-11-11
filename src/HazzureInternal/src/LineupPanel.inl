// Included inside HazzureUI after profile persistence and standard widgets.
// Native CS:2 (Source 2 / Sub-tick / Ballistics / Visual Guide).

static void DrawLineupControls(uintptr_t client) {
    BeginCleanCard("Grenade Helper CS:2", "LINEUPS NATIVAS");
    CleanToggle("Marcadores de Lineup", &g_cfg.lineupHelper);
    CleanToggle("Reconhecer Mapa Automaticamente", &g_cfg.lineupAutoMap);
    CleanToggle("Alinhar e Arremessar por Tecla", &g_cfg.lineupAutoAlign);
    if (g_cfg.lineupAutoAlign) ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "[ESTADO] %s", g_lineupState);

    ImGui::TextUnformatted("Mapa selecionado");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##lineup-map", g_cfg.lineups.map, sizeof(g_cfg.lineups.map));

    static char label[64] = "Minha granada CS2";
    static const char* status = "";
    static int throwMode = 1; // 0=Stand, 1=Jumpthrow, 2=Runthrow, 3=Crouch, 4=Underhand, 5=Walk
    static int arcKind = 0;   // 0=Direct, 1=Bounce
    static int stance = 0;    // 0=Standing, 1=Crouched
    static float runMs = 0.0f;
    static float manualImpact[3] = { 0.0f, 0.0f, 0.0f };

    const char* modes[] = { "Em pe (Stand)", "Com salto (Jumpthrow)", "Em corrida (Runthrow)", "Abaixado (Crouch)", "Por baixo (Underhand)", "Andando (Walk)" };
    CleanCombo("Modalidade de Arremesso", &throwMode, modes, 6);

    const char* arcModes[] = { "Arco Direto (Direct - Solver Ativo)", "Tabelado (Bounce - Solver Desativado)" };
    CleanCombo("Tipo de Trajetoria", &arcKind, arcModes, 2);

    const char* stances[] = { "Em pe (Standing)", "Abaixado (Crouched)" };
    CleanCombo("Stance Exigida", &stance, stances, 2);

    if (throwMode == 2 || throwMode == 5) {
        CleanSliderFloat("Duracao da Corrida", &runMs, 0.f, 1000.f, "%.0f ms");
    }

    ImGui::TextUnformatted("Nome do Lineup");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##lineup-name", label, sizeof(label));

    if (ImGui::Button("Gravar Posicao, Mira e Impacto")) {
        auto& library = g_cfg.lineups;
        HazzureGame::Pawn local{};
        uintptr_t list{};
        if (library.count >= 64) {
            status = "Limite de 64 posicoes atingido";
        } else if (!HazzureLineups::Text(library.map, sizeof(library.map)) || !HazzureLineups::Text(label, sizeof(label))) {
            status = "Preencha mapa e nome";
        } else if (!HazzureWorld::ready.load() || !HazzureGame::ReadPawn(HazzureGame::LocalPawn(client), local) ||
                   !local.anglesValid || !SafeRead(client + HazzureOffsets::Globals::dwEntityList, &list)) {
            status = "Entre no mapa para gravar";
        } else {
            auto definition = ActiveWeaponDefinition(local.address, list);
            int flags = 0;
            SafeRead(local.address + HazzureOffsets::Schema::m_fFlags, &flags);
            bool ducked = (flags & 2) != 0;

            HazzureLineups::Entry entry{};
            strcpy_s(entry.map, library.map);
            strcpy_s(entry.name, label);
            entry.definition = definition;
            entry.position = local.origin;
            entry.eye_origin = { local.origin.x, local.origin.y, local.origin.z + (ducked ? 46.0f : 64.0f) };
            entry.angles = local.angles;
            entry.velocity = local.velocity;
            entry.throwMode = throwMode;
            entry.stance = ducked ? 1 : stance;
            entry.arcKind = arcKind;
            entry.runMs = static_cast<int>(runMs);
            entry.when = 0.0f;

            // Auto-captura de impacto a partir da simulacao de trajetoria viva
            HazzureGrenade::Result res{};
            if (HazzureGrenade::channel.Submit(HazzureGrenade::channel.request, GetTickCount64(), res) && res.count > 0) {
                entry.impact = res.points[res.count - 1];
            } else if (std::fabs(manualImpact[0]) > 0.1f || std::fabs(manualImpact[1]) > 0.1f) {
                entry.impact = { manualImpact[0], manualImpact[1], manualImpact[2] };
            }

            if (HazzureLineups::Valid(entry)) {
                library.entries[library.count++] = entry;
                status = "Lineup nativo gravado com sucesso; salve o perfil para persistir.";
            } else {
                status = "Segure uma granada valida antes de gravar.";
            }
        }
    }

    if (*status) ImGui::TextWrapped("%s", status);

    if (ImGui::Button("Adicionar Referencias Mirage CS2")) {
        int added = HazzureLineupCatalogue::Merge(g_cfg.lineups);
        status = added ? "Referencias adicionadas com sucesso" : "Referencias ja presentes ou biblioteca cheia";
    }

    ImGui::Separator();
    ImGui::TextDisabled("Lineups Armazenados no Mapa (%d/64):", g_cfg.lineups.count);

    for (int i = 0; i < g_cfg.lineups.count; ++i) {
        auto& entry = g_cfg.lineups.entries[i];
        ImGui::PushID(i);
        ImGui::TextWrapped("[%s] %s / %s",
            entry.throwMode == 1 ? "JUMP" : entry.throwMode == 2 ? "RUN" : entry.throwMode == 3 ? "CROUCH" : entry.throwMode == 4 ? "UNDER" : "STAND",
            entry.map, entry.name);
        ImGui::SameLine();
        if (ImGui::SmallButton("Remover")) {
            for (int j = i + 1; j < g_cfg.lineups.count; ++j) g_cfg.lineups.entries[j - 1] = g_cfg.lineups.entries[j];
            g_cfg.lineups.entries[--g_cfg.lineups.count] = {};
            --i;
            ImGui::PopID();
            continue;
        }

        CleanCombo("Tipo", &entry.throwMode, modes, 6);
        CleanCombo("Arco", &entry.arcKind, arcModes, 2);
        CleanCombo("Stance", &entry.stance, stances, 2);
        ImGui::Checkbox("Validado para execucao automatica", &entry.validated);
        ImGui::PopID();
    }

    EndCleanCard();
}

static void RenderLineups(ImDrawList* draw, uintptr_t client, const HazzureGame::Pawn& local, const Matrix4& matrix, int width, int height) {
    if (!g_cfg.lineupHelper || !HazzureWorld::ready.load() || !draw) return;
    uintptr_t list{};
    if (!SafeRead(client + HazzureOffsets::Globals::dwEntityList, &list)) return;

    int definition = ActiveWeaponDefinition(local.address, list);
    if (definition < 43 || definition > 48) return; // apenas granadas

    int flags = 0;
    SafeRead(local.address + HazzureOffsets::Schema::m_fFlags, &flags);
    int currentStance = (flags & 2) ? 1 : 0;

    float matchScore = 0.0f;
    int selected = HazzureLineups::Nearest(g_cfg.lineups, definition, local.origin, currentStance, &matchScore);
    if (selected < 0) return;

    const auto& entry = g_cfg.lineups.entries[selected];
    HazzureGame::Vec3 eyeOrigin = { local.origin.x, local.origin.y, local.origin.z + ((currentStance == 1) ? 46.0f : 64.0f) };

    HazzureGame::Vec3 targetAngles{};
    bool solved = false;
    bool converged = HazzureLineups::Ready(entry, g_cfg.lineups.map, definition, local.origin, local.angles, eyeOrigin, targetAngles, solved);

    float dxy = HazzureLineups::HorizontalDistance(local.origin, entry.position);
    float dz = HazzureLineups::VerticalDistance(local.origin, entry.position);
    bool inTolerance = (dxy <= 12.0f && dz <= 16.0f);

    // Cor baseada no tipo de granada (Secao 6 do briefing)
    ImU32 grenadeColor = IM_COL32(180, 240, 255, 230); // Flash
    if (definition == 44) grenadeColor = IM_COL32(255, 140, 30, 230); // HE
    else if (definition == 45) grenadeColor = IM_COL32(130, 180, 255, 230); // Smoke
    else if (definition == 46 || definition == 48) grenadeColor = IM_COL32(255, 60, 60, 230); // Molotov / Incendiary
    else if (definition == 47) grenadeColor = IM_COL32(80, 240, 100, 230); // Decoy

    ImU32 ringColor = inTolerance ? IM_COL32(50, 255, 120, 240) : grenadeColor;

    // 1. Anel no chao em entry.position com raio 12 (Secao 6 do briefing)
    Vec2 floorPoint{};
    if (WorldToScreen(entry.position, matrix, width, height, floorPoint)) {
        draw->AddCircle({ floorPoint.x, floorPoint.y }, 12.0f, ringColor, 32, inTolerance ? 2.5f : 1.5f);
        char labelBuf[96];
        sprintf_s(labelBuf, "%s (Score: %.0f%%)", entry.name, std::max(0.0f, matchScore * 100.0f));
        CombatText(draw, { floorPoint.x + 16.0f, floorPoint.y - 6.0f }, ringColor, labelBuf, 1);
    }

    // 2. Indicador de mira 3D projetado a 400u no espaco de mundo
    float radPitch = targetAngles.x * 0.01745329252f;
    float radYaw = targetAngles.y * 0.01745329252f;
    HazzureGame::Vec3 aimDir = {
        std::cos(radPitch) * std::cos(radYaw),
        std::cos(radPitch) * std::sin(radYaw),
        -std::sin(radPitch)
    };
    HazzureGame::Vec3 aimTargetPos = { eyeOrigin.x + aimDir.x * 400.0f, eyeOrigin.y + aimDir.y * 400.0f, eyeOrigin.z + aimDir.z * 400.0f };
    Vec2 aimScreen{};
    if (WorldToScreen(aimTargetPos, matrix, width, height, aimScreen)) {
        ImU32 aimColor = converged ? IM_COL32(50, 255, 120, 255) : IM_COL32(255, 215, 0, 230);
        draw->AddCircle({ aimScreen.x, aimScreen.y }, 7.0f, aimColor, 16, 2.0f);
        draw->AddLine({ aimScreen.x - 12.0f, aimScreen.y }, { aimScreen.x + 12.0f, aimScreen.y }, aimColor, 1.5f);
        draw->AddLine({ aimScreen.x, aimScreen.y - 12.0f }, { aimScreen.x, aimScreen.y + 12.0f }, aimColor, 1.5f);
        if (converged) {
            draw->AddCircleFilled({ aimScreen.x, aimScreen.y }, 3.0f, aimColor);
        }
    }

    // 3. HUD status inferior centralizado
    const char* modeTag = "[STAND]";
    if (entry.throwMode == 1) modeTag = "[JUMPTHROW]";
    else if (entry.throwMode == 2) modeTag = "[RUNTHROW]";
    else if (entry.throwMode == 3) modeTag = "[CROUCH]";
    else if (entry.throwMode == 4) modeTag = "[UNDERHAND]";
    else if (entry.throwMode == 5) modeTag = "[WALK]";

    float dPitch = targetAngles.x - local.angles.x;
    float dYaw = HazzureGame::NormalizeAngle(targetAngles.y - local.angles.y);

    char text[256];
    sprintf_s(text, "%s %s | dxy: %.1fu dz: %.1fu | P: %+.1f deg Y: %+.1f deg | %s%s",
        modeTag, entry.name, dxy, dz, dPitch, dYaw,
        converged ? "CONVERGIDO (PRONTO)" : (inTolerance ? "ALINHAR MIRA" : "APROXIME-SE"),
        solved ? " [SOLVER BALISTICO OK]" : "");

    auto size = ImGui::CalcTextSize(text);
    ImU32 bannerColor = converged ? IM_COL32(50, 255, 120, 255) : (inTolerance ? IM_COL32(255, 215, 0, 240) : ringColor);
    CombatText(draw, { (width - size.x) * 0.5f, height * 0.72f }, bannerColor, text, 1);
}
