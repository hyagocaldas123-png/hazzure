
    uintptr_t chunks[64]{};
    if (!SafeRead(entityList + 0x10, chunks)) return;

    uintptr_t seenPawns[64]{};
    int seenCount = 0;

    // Alvo mais próximo para o Aimbot
    Vec2 bestAimPos{};
    float bestAimDist = 999999.f;
    float maxFovRadius = g_cfg.aimFov * 14.f;
    ImVec2 screenCenter((float)screenW * 0.5f, (float)screenH * 0.5f);

    // 3. Iteração em Chunks do CS2 (Estrutura Real do Source 2)
    for (int c = 0; c < 64; ++c) {
        uintptr_t chunk = chunks[c];
        if (!chunk) continue;

        for (int i = 0; i < 512; ++i) {
            uintptr_t identity = 0;
            if (!SafeRead(chunk + (i * 0x70), &identity) || !identity) continue;

            uintptr_t currentPawn = 0;
            if (!SafeRead(identity + 0x0, &currentPawn) || !currentPawn) currentPawn = identity;
            if (!currentPawn || currentPawn == localPawn) continue;

            // Deduplicação de Pawns já processados
            bool alreadySeen = false;
            for (int s = 0; s < seenCount; ++s) {
                if (seenPawns[s] == currentPawn) { alreadySeen = true; break; }
            }
            if (alreadySeen) continue;
            if (seenCount < 64) seenPawns[seenCount++] = currentPawn;

            // Leitura de vida e time
            int32_t health = 0;
            if (!SafeRead(currentPawn + HazzureOffsets::Schema::m_iHealth, &health) || health <= 0 || health > 100) continue;

            int32_t team = 0;
            if (!SafeRead(currentPawn + HazzureOffsets::Schema::m_iTeamNum, &team) || (team != 2 && team != 3)) continue;

            bool isEnemy = (team != localTeam);
            if (shared && shared->teamFilter && !isEnemy) continue;

            ImU32 colBase = isEnemy ? Float4ToImU32(g_cfg.colEnemy) : Float4ToImU32(g_cfg.colTeam);

            // Origem 3D do Pawn
            Vec3 origin{};
            if (!SafeRead(currentPawn + HazzureOffsets::Schema::m_vecAbsOrigin, &origin)) continue;
            if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z)) continue;

            Vec2 footScreen{}, headScreen{};
            bool onScreenFoot = WorldToScreen({ origin.x, origin.y, origin.z - 4.f }, matrix, screenW, screenH, footScreen);
            bool onScreenHead = WorldToScreen({ origin.x, origin.y, origin.z + 74.f }, matrix, screenW, screenH, headScreen);

            // Se estiver fora de tela, renderiza Off-Screen Arrows
            if (!onScreenFoot || !onScreenHead || footScreen.x < 15.f || footScreen.x > screenW - 15.f || footScreen.y < 15.f || footScreen.y > screenH - 15.f) {
                if (g_cfg.offScreenArrows && isEnemy) {
                    float cx = (float)screenW * 0.5f;
                    float cy = (float)screenH * 0.5f;

                    float w = matrix.m[3][0] * origin.x + matrix.m[3][1] * origin.y + matrix.m[3][2] * origin.z + matrix.m[3][3];
                    float x = matrix.m[0][0] * origin.x + matrix.m[0][1] * origin.y + matrix.m[0][2] * origin.z + matrix.m[0][3];
                    float y = matrix.m[1][0] * origin.x + matrix.m[1][1] * origin.y + matrix.m[1][2] * origin.z + matrix.m[1][3];

                    float angle = (w < 0.001f) ? std::atan2(-y, -x) : std::atan2(y, x);
                    float radius = (((cx < cy) ? cx : cy) - 35.f) * 0.88f;

                    float ax = cx + std::cos(angle) * radius;
                    float ay = cy - std::sin(angle) * radius;

                    float arrowSize = 13.f;
                    ImVec2 p1(ax + std::cos(angle) * arrowSize, ay - std::sin(angle) * arrowSize);
                    ImVec2 p2(ax + std::cos(angle + 2.4f) * (arrowSize * 0.7f), ay - std::sin(angle + 2.4f) * (arrowSize * 0.7f));
                    ImVec2 p3(ax + std::cos(angle - 2.4f) * (arrowSize * 0.7f), ay - std::sin(angle - 2.4f) * (arrowSize * 0.7f));

                    draw->AddTriangleFilled(p1, p2, p3, colBase);
                    draw->AddTriangle(p1, p2, p3, IM_COL32(0, 0, 0, 220), 1.2f);

                    float distM = std::hypot(origin.x - localOrigin.x, origin.y - localOrigin.y) / 52.5f;
                    char distBuf[16];
                    sprintf_s(distBuf, "%dm", (int)distM);
                    draw->AddText(ImVec2(ax - 10.f, ay + 10.f), IM_COL32(230, 230, 235, 220), distBuf);
                }
                continue;
            }

            float boxHeight = std::fabs(footScreen.y - headScreen.y);
            float boxWidth = boxHeight * 0.48f;
            float boxLeft = headScreen.x - (boxWidth * 0.5f);
            float boxRight = boxLeft + boxWidth;
            float boxTop = headScreen.y;
            float boxBottom = footScreen.y;

            // 4. Bounding Boxes (Normal ou Corner)
            if (g_cfg.espBoxes && g_cfg.espEnabled) {
                if (g_cfg.espBoxType == 0) {
                    draw->AddRect(ImVec2(boxLeft - 1, boxTop - 1), ImVec2(boxRight + 1, boxBottom + 1), IM_COL32(0, 0, 0, 180), 0.f, 0, 1.f);
                    draw->AddRect(ImVec2(boxLeft, boxTop), ImVec2(boxRight, boxBottom), colBase, 0.f, 0, 1.4f);
                    draw->AddRect(ImVec2(boxLeft + 1, boxTop + 1), ImVec2(boxRight - 1, boxBottom - 1), IM_COL32(0, 0, 0, 180), 0.f, 0, 1.f);
                } else {
                    float cW = boxWidth * 0.28f;
                    float cH = boxHeight * 0.22f;

                    auto DrawCorner = [&](float x1, float y1, float x2, float y2) {
                        draw->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), colBase, 1.6f);
                    };

                    DrawCorner(boxLeft, boxTop, boxLeft + cW, boxTop);
                    DrawCorner(boxLeft, boxTop, boxLeft, boxTop + cH);
                    DrawCorner(boxRight, boxTop, boxRight - cW, boxTop);
                    DrawCorner(boxRight, boxTop, boxRight, boxTop + cH);
                    DrawCorner(boxLeft, boxBottom, boxLeft + cW, boxBottom);
                    DrawCorner(boxLeft, boxBottom, boxLeft, boxBottom - cH);
                    DrawCorner(boxRight, boxBottom, boxRight - cW, boxBottom);
                    DrawCorner(boxRight, boxBottom, boxRight - cW, boxBottom - cH);
                }
            }

            // 5. Barra de Vida Dinâmica com Gradiente
            if (g_cfg.espHealth && g_cfg.espEnabled) {
                float frac = std::clamp(health / 100.f, 0.f, 1.f);
                float hH = boxHeight * frac;
                float bTop = boxBottom - hH;

                draw->AddRectFilled(ImVec2(boxLeft - 6.f, boxTop - 1.f), ImVec2(boxLeft - 2.f, boxBottom + 1.f), IM_COL32(10, 10, 14, 220));
                ImU32 hpCol = IM_COL32((int)((1.f - frac) * 255.f), (int)(frac * 255.f), 30, 255);
                draw->AddRectFilled(ImVec2(boxLeft - 5.f, bTop), ImVec2(boxLeft - 3.f, boxBottom), hpCol);
            }

            // 6. Linhas de Snap (Snaplines)
            if (g_cfg.espSnaplines && g_cfg.espEnabled) {
                draw->AddLine(ImVec2((float)screenW * 0.5f, (float)screenH), ImVec2(footScreen.x, footScreen.y), Float4ToImU32(g_cfg.colSnaplines), 1.0f);
            }

            // 7. Esqueleto 3D (Bone Array)
            if (g_cfg.espSkeleton && g_cfg.espEnabled) {
                uintptr_t gameScene = 0;
                if (!SafeRead(currentPawn + 0x328, &gameScene) || !gameScene) {
                    SafeRead(currentPawn + 0x310, &gameScene);
                }
                if (gameScene) {
                    uintptr_t boneArray = 0;
                    if (!SafeRead(gameScene + 0x1F0 + 0x80, &boneArray) || !boneArray) {
                        SafeRead(gameScene + 0x160 + 0x80, &boneArray);
                    }
                    if (boneArray) {
                        ImU32 skelCol = Float4ToImU32(g_cfg.colSkeleton);
                        for (const auto& link : kBoneLinks) {
                            Vec3 b1{}, b2{};
                            if (SafeRead(boneArray + link[0] * 32, &b1) && SafeRead(boneArray + link[1] * 32, &b2)) {
                                Vec2 s1{}, s2{};
                                if (WorldToScreen(b1, matrix, screenW, screenH, s1) && WorldToScreen(b2, matrix, screenW, screenH, s2)) {
                                    draw->AddLine(ImVec2(s1.x, s1.y), ImVec2(s2.x, s2.y), skelCol, 1.4f);
                                }
                            }
                        }
                    }
                }
            }

            // Seleção de alvo para Aimbot
            if (isEnemy) {
                Vec2 targetPoint = (g_cfg.aimBone == 0) ? headScreen : Vec2{ (headScreen.x + footScreen.x) * 0.5f, (headScreen.y + footScreen.y) * 0.5f };
                float d = std::hypot(targetPoint.x - screenCenter.x, targetPoint.y - screenCenter.y);
                if (d < maxFovRadius && d < bestAimDist) {
                    bestAimDist = d;
                    bestAimPos = targetPoint;
                }
            }
        }
    }

    // Execução do Aimbot Suave
    if (g_cfg.aimActive && bestAimDist < maxFovRadius && (GetAsyncKeyState(g_cfg.aimKey) & 0x8000)) {
        float dx = bestAimPos.x - screenCenter.x;
        float dy = bestAimPos.y - screenCenter.y;
        float smooth = std::clamp(g_cfg.aimSmooth, 1.0f, 30.0f);
        float stepX = dx / smooth;
        float stepY = dy / smooth;
        if (std::abs(stepX) > 0.1f || std::abs(stepY) > 0.1f) {
            mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(stepX), static_cast<DWORD>(stepY), 0, 0);
        }
    }

    // Renderiza complementos balísticos e movimentação
    RenderGrenadeTrajectory(draw, localPawn, matrix, screenW, screenH);
    RenderSpreadCircle(draw, screenW, screenH);
    RenderSpeedometer(draw, localPawn, screenW, screenH);
    ProcessMovementMechanics(localPawn);
}

// ---------- Janela de Espectadores (HUD Flutuante) ----------
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
}

// ---------- Seletor Interativo de Teclas (Hotkeys) ----------
static bool DrawKeybindButton(const char* label, int* pKey) {
    static int bindingTarget = -1;
    char buf[64];

    if (bindingTarget == *pKey) {
        strcpy_s(buf, "[ PRESSIONE... ]");
        for (int k = 1; k < 256; ++k) {
            if (GetAsyncKeyState(k) & 1) {
                if (k == VK_ESCAPE) {
                    *pKey = 0;
                } else {
                    *pKey = k;
                }
                bindingTarget = -1;
                return true;
            }
        }
    } else {
        if (*pKey == 0) {
            strcpy_s(buf, "[ NENHUMA ]");
        } else if (*pKey == VK_INSERT) {
            strcpy_s(buf, "[ INSERT ]");
        } else if (*pKey == VK_LBUTTON) {
            strcpy_s(buf, "[ MOUSE 1 ]");
        } else if (*pKey == VK_RBUTTON) {
            strcpy_s(buf, "[ MOUSE 2 ]");
        } else if (*pKey == VK_XBUTTON1) {
            strcpy_s(buf, "[ MOUSE 4 ]");
        } else if (*pKey == VK_XBUTTON2) {
            strcpy_s(buf, "[ MOUSE 5 ]");
        } else {
            sprintf_s(buf, "[ TECLA 0x%X ]", *pKey);
        }
    }

    ImGui::Text("%s", label);
    ImGui::SameLine();
    if (ImGui::Button(buf, ImVec2(130.f, 22.f))) {
        bindingTarget = *pKey;
    }
    return false;
}

// ---------- Presets Rápidos ----------
static void ApplyPreset(int type) {
    if (type == 0) { // Legit
        g_cfg.aimActive = true;
        g_cfg.aimFov = 2.5f;
        g_cfg.aimSmooth = 8.5f;
        g_cfg.rcsActive = true;
        g_cfg.espEnabled = true;
        g_cfg.espBoxes = true;
        g_cfg.espBoxType = 1;
        g_cfg.espSkeleton = false;
        g_cfg.espSnaplines = false;
        g_cfg.offScreenArrows = false;
    } else if (type == 1) { // Semi-Rage
        g_cfg.aimActive = true;
        g_cfg.aimFov = 6.5f;
        g_cfg.aimSmooth = 2.0f;
        g_cfg.rcsActive = true;
        g_cfg.espEnabled = true;
        g_cfg.espBoxes = true;
        g_cfg.espBoxType = 0;
        g_cfg.espSkeleton = true;
        g_cfg.espSnaplines = true;
        g_cfg.offScreenArrows = true;
    } else if (type == 2) { // Apenas Visuais
        g_cfg.aimActive = false;
        g_cfg.rcsActive = false;
        g_cfg.espEnabled = true;
        g_cfg.espBoxes = true;
        g_cfg.espBoxType = 1;
        g_cfg.espSkeleton = true;
        g_cfg.offScreenArrows = true;
    }
}

// ---------- Painel Principal de Configuração (Menu Obsidian) ----------
static void DrawPanel(Shared* shared, bool* pOpen) {
    if (!pOpen || !*pOpen) return;

    ImGui::SetNextWindowSize(ImVec2(760.f, 490.f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, kDarkBg);
    ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    if (ImGui::Begin("##HazzureMasterPanel", pOpen, winFlags)) {
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // 1. Partículas de Neve no Fundo do Painel
        RenderSnow(draw, winPos, winSize);

        const float sideW = 190.f;
        const float headerH = 50.f;
        const float footerH = 34.f;
        const float contentW = winSize.x - sideW;
        const float contentH = winSize.y - headerH - footerH;

        // 2. Barra Lateral (Sidebar com Abas)
        ImGui::SetCursorPos(ImVec2(0.f, 0.f));
        ImGui::BeginChild("##sidebar", ImVec2(sideW, winSize.y), false, ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::SetCursorPos(ImVec2(18.f, 16.f));
            ImGui::TextColored(kAccent, "HAZZURE");
            ImGui::SameLine();
            ImGui::TextColored(kMuted, "OBSIDIAN");

            ImGui::SetCursorPos(ImVec2(18.f, 32.f));
            ImGui::TextColored(ImVec4(0.38f, 0.38f, 0.42f, 1.0f), "Internal DX11 Suite");

            ImGui::SetCursorPosY(headerH + 8.f);

            static const char* kTabs[] = {
                "ESP & Visual",
                "Chams & Cores",
                "Mira / Aimbot",
                "Movimentacao",
                "Audio & Hitsound",
                "Skins & Facas",
                "Perfis & Teclas"
            };

            for (int i = 0; i < IM_ARRAYSIZE(kTabs); ++i) {
                bool isSelected = (g_currentTab == i);
                ImGui::PushID(i);

                if (isSelected) {
                    ImVec2 p = ImGui::GetCursorScreenPos();
                    draw->AddRectFilled(p, ImVec2(p.x + sideW - 12.f, p.y + 30.f), IM_COL32(35, 35, 42, 220), 4.f);
                    draw->AddRectFilled(p, ImVec2(p.x + 3.f, p.y + 30.f), IM_COL32(245, 245, 250, 255), 2.f);
                    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
                }

                ImGui::SetCursorPosX(16.f);
                if (ImGui::Selectable(kTabs[i], isSelected, 0, ImVec2(sideW - 32.f, 30.f))) {
                    g_currentTab = i;
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        // Linha divisória vertical
        draw->AddLine(ImVec2(winPos.x + sideW, winPos.y), ImVec2(winPos.x + sideW, winPos.y + winSize.y), IM_COL32(40, 40, 48, 255));

        // 3. Conteúdo da Aba Ativa
        ImGui::SetCursorPos(ImVec2(sideW + 16.f, headerH));
        ImGui::BeginChild("##tabContent", ImVec2(contentW - 32.f, contentH), false);
        {
            if (g_currentTab == 0) {
                // ==========================================
                // ABA 0: ESP & VISUAL
                // ==========================================
                ImGui::TextColored(kAccent, "Renderizacao no Mundo (DirectX 11)");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("ESP Geral Ativo", &g_cfg.espEnabled);
                ImGui::SameLine(180.f);
                ImGui::Checkbox("Setas Fora de Tela (Off-Screen)", &g_cfg.offScreenArrows);

                ImGui::Checkbox("Caixas Delimitadoras (Boxes)", &g_cfg.espBoxes);
                ImGui::SameLine(180.f);
                ImGui::RadioButton("Box Simples", &g_cfg.espBoxType, 0);
                ImGui::SameLine(290.f);
                ImGui::RadioButton("Cantoneiras (Corner)", &g_cfg.espBoxType, 1);

                ImGui::Checkbox("Esqueleto 3D dos Ossos", &g_cfg.espSkeleton);
                ImGui::SameLine(180.f);
                ImGui::Checkbox("Barra de Vida Dinamica", &g_cfg.espHealth);

                ImGui::Checkbox("Linhas de Snap (Snaplines)", &g_cfg.espSnaplines);
                ImGui::SameLine(180.f);
                ImGui::Checkbox("Circulo de FOV da Mira", &g_cfg.espFovCircle);

                ImGui::Checkbox("Trajetoria de Granadas (Fisica)", &g_cfg.showGrenadeTrajectory);
                ImGui::SameLine(180.f);
                ImGui::Checkbox("Mira Dinamica de Dispersao (Spread)", &g_cfg.showSpreadCircle);

                ImGui::Spacing();
                ImGui::TextColored(kAccent, "Cores dos Elementos");
                ImGui::Separator();
                ImGui::ColorEdit4("Inimigos (Visivel)", g_cfg.colEnemy, ImGuiColorEditFlags_NoInputs);
                ImGui::ColorEdit4("Aliados (Team)", g_cfg.colTeam, ImGuiColorEditFlags_NoInputs);
                ImGui::ColorEdit4("Esqueleto", g_cfg.colSkeleton, ImGuiColorEditFlags_NoInputs);
                ImGui::ColorEdit4("Snaplines", g_cfg.colSnaplines, ImGuiColorEditFlags_NoInputs);

            } else if (g_currentTab == 1) {
                // ==========================================
                // ABA 1: CHAMS & CORES
                // ==========================================
                ImGui::TextColored(kAccent, "Sobrescrita de Materiais (Chams)");
                ImGui::Separator();
                ImGui::Spacing();

                if (shared) {
                    bool visChams = (shared->flags & 1) != 0;
                    if (ImGui::Checkbox("Chams Visivel (Inimigos)", &visChams)) {
                        shared->flags = visChams ? (shared->flags | 1) : (shared->flags & ~1);
                    }
                    float cVis[4];
                    IpcColToFloat4(shared->visibleColour, cVis);
                    if (ImGui::ColorEdit4("Cor Chams Visivel", cVis, ImGuiColorEditFlags_NoInputs)) {
                        shared->visibleColour = Float4ToIpcCol(cVis);
                    }

                    bool hidChams = (shared->flags & 2) != 0;
                    if (ImGui::Checkbox("Chams Atraves da Parede (Hidden)", &hidChams)) {
                        shared->flags = hidChams ? (shared->flags | 2) : (shared->flags & ~2);
                    }
                    float cHid[4];
                    IpcColToFloat4(shared->hiddenColour, cHid);
                    if (ImGui::ColorEdit4("Cor Chams Oculto", cHid, ImGuiColorEditFlags_NoInputs)) {
                        shared->hiddenColour = Float4ToIpcCol(cHid);
                    }

                    bool armsChams = (shared->flags & 4) != 0;
                    if (ImGui::Checkbox("Chams Bracos e Luvas", &armsChams)) {
                        shared->flags = armsChams ? (shared->flags | 4) : (shared->flags & ~4);
                    }
                }

                ImGui::Spacing();
                ImGui::TextColored(kAccent, "Estetica do Painel");
                ImGui::Separator();
                ImGui::Checkbox("Fundo Escurecido com Vinheta", &g_cfg.backgroundVignette);

            } else if (g_currentTab == 2) {
                // ==========================================
                // ABA 2: MIRA / AIMBOT
                // ==========================================
                ImGui::TextColored(kAccent, "Assistente de Disparo e Aimbot");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Aimbot Ativo", &g_cfg.aimActive);
                ImGui::SliderFloat("Campo de Visao (FOV)", &g_cfg.aimFov, 0.5f, 25.0f, "%.1f px");
                ImGui::SliderFloat("Suavizacao (Smooth)", &g_cfg.aimSmooth, 1.0f, 25.0f, "%.1f");

                const char* bones[] = { "Cabeca (Head)", "Peito (Chest)" };
                ImGui::Combo("Osso de Referencia", &g_cfg.aimBone, bones, IM_ARRAYSIZE(bones));

                ImGui::Checkbox("Compensacao de Recoil (RCS)", &g_cfg.rcsActive);

                ImGui::Spacing();
                DrawKeybindButton("Tecla do Aimbot", &g_cfg.aimKey);
