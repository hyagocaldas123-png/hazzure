#pragma once

// HazzureInternal/src/cards/CardCosmetics.inl — Frente 2.4 Modularizacao UI Enigma CS2
// Escopo: Skins, Facas, Luvas, Catalogo Local e Agentes CT / TR


inline void DrawCosmeticsSkinsCard(Shared* shared) {
    BeginCleanCard("Inventory", "COLECAO LOCAL");
    DrawSkinSelections(shared);
    EndCleanCard();
}

inline void DrawCosmeticsAgentsCard(Shared* shared) {
    BeginCleanCard("Agent Changer", "CT / TR");
    if (shared) {
        bool agEnabled = InterlockedCompareExchange(&shared->agentEnabled, 0, 0) != 0;
        if (CleanToggle("Ativar Agent Changer", &agEnabled)) {
            InterlockedExchange(&shared->agentEnabled, agEnabled ? 1 : 0);
        }

        const char* kAgentsCT[IM_ARRAYSIZE(kPreviewAgentsCT)]{};
        for (size_t i = 0; i < IM_ARRAYSIZE(kPreviewAgentsCT); ++i) kAgentsCT[i] = kPreviewAgentsCT[i].name;
        int ctIdx = static_cast<int>(InterlockedCompareExchange(&shared->agentCT, 0, 0));
        if (ctIdx < 0 || ctIdx >= IM_ARRAYSIZE(kAgentsCT)) ctIdx = 0;
        if (CleanCombo("Agente Contra-Terrorista (CT)", &ctIdx, kAgentsCT, IM_ARRAYSIZE(kAgentsCT))) {
            InterlockedExchange(&shared->agentCT, ctIdx);
        }

        const char* kAgentsT[IM_ARRAYSIZE(kPreviewAgentsT)]{};
        for (size_t i = 0; i < IM_ARRAYSIZE(kPreviewAgentsT); ++i) kAgentsT[i] = kPreviewAgentsT[i].name;
        int tIdx = static_cast<int>(InterlockedCompareExchange(&shared->agentT, 0, 0));
        if (tIdx < 0 || tIdx >= IM_ARRAYSIZE(kAgentsT)) tIdx = 0;
        if (CleanCombo("Agente Terrorista (TR)", &tIdx, kAgentsT, IM_ARRAYSIZE(kAgentsT))) {
            InterlockedExchange(&shared->agentT, tIdx);
        }
    }
    EndCleanCard();
}


