                    if (ImGui::Checkbox("Ativar Modificacao de Skins", &skinsEnabled)) {
                        InterlockedExchange(&shared->skinEnabled, skinsEnabled ? 1 : 0);
                        InterlockedIncrement(&shared->revision);
                    }

                    const char* knifeList[] = { "Padrao", "Karambit", "M9 Bayonet", "Butterfly", "Bayonet", "Flip Knife", "Skeleton" };
                    static int knifeIdx = 1;
                    if (ImGui::Combo("Modelo da Faca", &knifeIdx, knifeList, IM_ARRAYSIZE(knifeList))) {
                        shared->skins[0].definition = 507;
                        shared->skins[0].model = knifeIdx;
                        shared->skins[0].paint = 416; // Doppler
                        InterlockedIncrement(&shared->revision);
                    }

                    const char* gloveList[] = { "Padrao", "Sport Gloves", "Specialist Gloves", "Driver Gloves", "Moto Gloves" };
                    static int gloveIdx = 1;
                    if (ImGui::Combo("Modelo das Luvas", &gloveIdx, gloveList, IM_ARRAYSIZE(gloveList))) {
                        shared->skins[1].definition = 5030;
                        shared->skins[1].model = gloveIdx;
                        shared->skins[1].paint = 10018;
                        InterlockedIncrement(&shared->revision);
                    }
                }

            } else if (currentTab == 6) {
                // ==========================================
                // ABA 6: PERFIS & TECLAS
                // ==========================================
                ImGui::TextColored(kAccent, "Predefinicoes e Hotkeys");
                ImGui::Separator();
                ImGui::Spacing();

                DrawKeybindButton("Tecla Abrir Menu", &g_cfg.menuKey);

                ImGui::Spacing();
                ImGui::TextColored(kAccent, "Carregar Predefinicoes Prontas");
                ImGui::Separator();

                if (ImGui::Button("Carregar Preset Legit", ImVec2(180.f, 28.f))) {
                    ApplyPreset(0);
                }
                ImGui::SameLine();
                if (ImGui::Button("Carregar Preset Semi-Rage", ImVec2(180.f, 28.f))) {
                    ApplyPreset(1);
                }
                ImGui::SameLine();
                if (ImGui::Button("Apenas Visuais", ImVec2(150.f, 28.f))) {
                    ApplyPreset(2);
                }

                ImGui::Spacing();
                ImGui::TextColored(kAccent, "Telemetria de Sistema");
                ImGui::Separator();
                if (shared) {
                    ImGui::Text("Frames Interceptados (Draw): %lld", shared->calls);
                    ImGui::Text("Sobrescritas de Malha: %lld", shared->changed);
                    ImGui::Text("Falhas Protegidas: %lld", shared->faults);
                }
                ImGui::Text("Hook: IDXGISwapChain VMT Shadow (Slot 8)");
                ImGui::Text("FPS do Overlay: %.1f", ImGui::GetIO().Framerate);
            }
        }
        ImGui::EndChild();

        // 4. Footer Inferior Direito
        ImGui::SetCursorPos(ImVec2(sideW + 1.f, winSize.y - footerH));
        ImGui::BeginChild("##footer", ImVec2(contentW, footerH), false, ImGuiWindowFlags_NoScrollbar);
        {
            ImVec2 fp = ImGui::GetCursorScreenPos();
            draw->AddLine(ImVec2(winPos.x + sideW, fp.y), ImVec2(winPos.x + winSize.x, fp.y), IM_COL32(40, 40, 48, 255));

            ImGui::SetCursorPos(ImVec2(16.f, 8.f));
            draw->AddCircleFilled(ImVec2(winPos.x + sideW + 24.f, winPos.y + winSize.y - 17.f), 4.f, IM_COL32(245, 245, 250, 255));
            ImGui::SetCursorPosX(38.f);
            ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.97f, 1.0f), "Hazzure Overlay Conectado e Operacional");
        }
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

// ---------- Ponto de Entrada da Renderização do Frame ----------
static void RenderFrame(Shared* shared, uintptr_t client, bool* pShowMenu, int screenW, int screenH) {
    // 1. Overlay Visual no Mundo 3D (ESP, Caixas, Esqueleto, Balística, Velocímetro)
    RenderInGameESP(shared, client, screenW, screenH);

    // 2. HUDs Complementares
    RenderSpectatorsHUD(screenW, screenH);
    RenderBombTimerHUD(screenW, screenH);
    if (g_cfg.showHitmarker) {
        RenderHitmarker(ImGui::GetForegroundDrawList(), screenW, screenH);
    }

    // 3. Fundo Vinheta Escurecido ao Abrir Menu (Parte 2)
    if (pShowMenu && *pShowMenu && g_cfg.backgroundVignette) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0.f, 0.f), ImVec2((float)screenW, (float)screenH), IM_COL32(4, 4, 6, 140));
    }

    // 4. Painel Principal (Quando Visível)
    if (pShowMenu && *pShowMenu) {
        DrawPanel(shared, pShowMenu);
    }
}

} // namespace HazzureUI
