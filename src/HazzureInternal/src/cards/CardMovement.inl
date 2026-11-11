#pragma once

// HazzureInternal/src/cards/CardMovement.inl — Frente 2.4 Modularizacao UI Enigma CS2
// Escopo: Movimentacao (BunnyHop, Auto-Strafe, DuckJump, Slowwalk, Fast Stop, Velocimetro) e Automacao (Jumpthrow, Auto-Pistol, Anti-AFK)


inline void DrawMovementBasicCard() {
    BeginCleanCard("Movimentacao", "FISICA");
    CleanToggle("BunnyHop Automatico", &g_cfg.bhopActive);
    if (g_cfg.bhopActive) {
        const char* modeLabels[] = {"Hold","Toggle","Always"};
        CleanCombo("Modo", &g_cfg.bhopMode, modeLabels, 3);
        if (g_cfg.bhopMode != 2) CleanKeybind("Tecla Bhop", &g_cfg.bhopKey);
        {float v=(float)g_cfg.bhopStrafeGain;if(CleanSliderFloat("Strafe Gain %",&v,0,100,"%.0f"))g_cfg.bhopStrafeGain=(int)v;}
        CleanSliderFloat("Angulo Step", &g_cfg.bhopAngleStep, 45.0f, 90.0f, "%.1f");
        {float v=(float)g_cfg.bhopHumanizeMs;if(CleanSliderFloat("Humanize (ms)",&v,0,40,"%.0f"))g_cfg.bhopHumanizeMs=(int)v;}
        {float v=(float)g_cfg.bhopHoldTicks;if(CleanSliderFloat("Hold Ticks",&v,1,4,"%.0f"))g_cfg.bhopHoldTicks=(int)v;}
        CleanToggle("Auto Edge-Jump", &g_cfg.bhopAutoEdgeJump);
        CleanToggle("Auto JumpBug", &g_cfg.bhopAutoJumpBug);
        CleanToggle("Prestrafe Align", &g_cfg.bhopPrestrafeAlign);
        CleanToggle("Preservar Speed no Solo", &g_cfg.bhopPreserveOnLand);
        CleanToggle("Apenas em Movimento", &g_cfg.bhopOnlyMoving);
        CleanSliderFloat("Velocidade Minima", &g_cfg.bhopMinSpeedGate, 0.0f, 100.0f);
    }
    EndCleanCard();
}

inline void DrawMovementTechCard() {
}

inline void DrawMovementAutomationCard() {
    BeginCleanCard("Automacao", "INPUT");
    CleanToggle("Jumpthrow por Tecla", &g_cfg.jumpthrowActive);
    if (g_cfg.jumpthrowActive) {
        CleanKeybind("Tecla Jumpthrow", &g_cfg.jumpthrowKey);
    }
    CleanToggle("Soltar Granada Automatico (Release)", &g_cfg.autoGrenadeRelease);
    CleanToggle("Auto-Pistol", &g_cfg.autoPistol);
    CleanToggle("Anti-AFK Automatico", &g_cfg.antiAfk);
    EndCleanCard();
}
