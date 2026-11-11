import sys

path = r'C:\Codex2\gpt\HazzureInternal\src\InternalStartup.inl'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update HookReleaseAttack to clear button & mouse
target_rel = '''static void HookReleaseAttack() {
    InterlockedExchange(&g_hookAttackDown, 0);
}'''

repl_rel = '''static void HookReleaseAttack() {
    InterlockedExchange(&g_hookAttackDown, 0);
    if (client) {
        uintptr_t attackBtn = client + HazzureOffsets::Buttons::attack;
        if (attackBtn) *reinterpret_cast<int32_t*>(attackBtn) = 256;
    }
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}'''

assert target_rel in text, 'target_rel not found'
text = text.replace(target_rel, repl_rel, 1)

# 2. Update Hook_CreateMove
target_cm = '''    AntiAimCommand antiAim{};
    HazzureGame::Vec3 realView{},fakeView{};
    bool antiAimApplied=false;
    __try{
        if(pCSGOInput&&client&&ReadAntiAimCommand(antiAim)&&HazzureInputView::ReadSnapshot(input,realView)){
            fakeView={std::clamp(antiAim.pitch,-89.f,89.f),HazzureGame::NormalizeAngle(realView.y+antiAim.yawOffset),0.f};
            antiAimApplied=HazzureInputView::Write(client,input,fakeView);
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){antiAimApplied=false;}

    __try {
        originalCreateMove(pCSGOInput, nSlot, pCmd);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (antiAimApplied) __try { HazzureInputView::Write(client,input,realView); } __except(EXCEPTION_EXECUTE_HANDLER){}
        return;
    }
    if (!pCmd || !pCSGOInput || !client) {
        HookReleaseAttack();
        return;
    }

    __try {
        HazzureGame::Vec3 view{};
        bool aimApplied=false;
        if (HazzureInputView::ReadSnapshot(input, view)) {
            HazzureGame::Vec3 commanded{};
            LONG64 sequence = 0;
            // One write per published decision: CreateMove runs several times per
            // tick (subtick), and re-applying the same absolute angle each call
            // overwrote the user's mouse samples and felt like a locked mouse.
            static LONG64 appliedSequence = 0;
            const bool wantWrite = ReadAimCommand(commanded, &sequence) && sequence != appliedSequence;
            if (wantWrite) {
                if (HazzureInputView::Write(client, input, commanded)) {
                    appliedSequence = sequence;
                    aimApplied = true;
                    InterlockedIncrement(&g_aimWriteOk);
                } else {
                    InterlockedIncrement(&g_aimWriteFail);
                }
            }
        }
        // Devolucao relativa: o CreateMove do motor ja consumiu as amostras de mouse
        // desta frame dentro dos angulos falsos. Restaurar o valor absoluto antigo as
        // descartaria e a mira travaria; somar a delta preserva o movimento real e
        // devolve a camera para onde o jogador realmente olha.
        if (antiAimApplied && !aimApplied) {
            HazzureGame::Vec3 after{};
            if (HazzureInputView::ReadSnapshot(input, after)) {
                // Clamp before the setter: |pitch| > 89.5 would make Write refuse the
                // restore and leave the camera on the fake angle, which is the exact
                // lock this layer is supposed to remove.
                HazzureGame::Vec3 restored{std::clamp(after.x+(realView.x-fakeView.x),-89.f,89.f),
                    HazzureGame::NormalizeAngle(after.y+HazzureGame::NormalizeAngle(realView.y-fakeView.y)),0.f};
                if (HazzureInputView::Write(client,input,restored)) InterlockedIncrement(&g_antiAimRestoreOk);
                else InterlockedIncrement(&g_antiAimRestoreFail);
            }
        }

        const bool fire = ConsumeTriggerFire();
        const uint64_t nowMs = GetTickCount64();
        if (fire) {
            g_hookAttackDown = 1;
            g_hookAttackSince = nowMs;
        } else if (g_hookAttackDown && nowMs >= g_hookAttackSince && nowMs - g_hookAttackSince >= 40) {
            HookReleaseAttack();
        }'''

repl_cm = '''    AntiAimCommand antiAim{};
    HazzureGame::Vec3 realView{},fakeView{};
    bool antiAimApplied=false;
    bool silentAimApplied=false;
    __try{
        if(pCSGOInput&&client&&HazzureInputView::ReadSnapshot(input,realView)){
            HazzureGame::Vec3 commanded{};
            LONG64 sequence = 0;
            static LONG64 appliedSilentSequence = 0;
            // Silent Aim no Rage: se houver comando engatilhado, escreve o angulo
            // no CUserCmd do frame e restaura a visao real do jogador apos CreateMove.
            if(g_cfg.rageActive&&g_cfg.rageSilentAim&&ReadAimCommand(commanded,&sequence)&&sequence!=appliedSilentSequence){
                fakeView=commanded;
                silentAimApplied=HazzureInputView::Write(client,input,fakeView);
                if(silentAimApplied){
                    appliedSilentSequence=sequence;
                    InterlockedIncrement(&g_aimWriteOk);
                }
            }else if(ReadAntiAimCommand(antiAim)){
                fakeView={std::clamp(antiAim.pitch,-89.f,89.f),HazzureGame::NormalizeAngle(realView.y+antiAim.yawOffset),0.f};
                antiAimApplied=HazzureInputView::Write(client,input,fakeView);
            }
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){antiAimApplied=false;silentAimApplied=false;}

    __try {
        originalCreateMove(pCSGOInput, nSlot, pCmd);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (antiAimApplied || silentAimApplied) __try { HazzureInputView::Write(client,input,realView); } __except(EXCEPTION_EXECUTE_HANDLER){}
        return;
    }
    if (!pCmd || !pCSGOInput || !client) {
        HookReleaseAttack();
        return;
    }

    __try {
        if (silentAimApplied) {
            // Restaura camera do jogador imediatamente - tela 100% estavel sem snap
            HazzureInputView::Write(client, input, realView);
        } else if (!g_cfg.rageSilentAim) {
            HazzureGame::Vec3 view{};
            if (HazzureInputView::ReadSnapshot(input, view)) {
                HazzureGame::Vec3 commanded{};
                LONG64 sequence = 0;
                static LONG64 appliedSequence = 0;
                const bool wantWrite = ReadAimCommand(commanded, &sequence) && sequence != appliedSequence;
                if (wantWrite) {
                    if (HazzureInputView::Write(client, input, commanded)) {
                        appliedSequence = sequence;
                        InterlockedIncrement(&g_aimWriteOk);
                    } else {
                        InterlockedIncrement(&g_aimWriteFail);
                    }
                }
            }
        }

        // Devolucao relativa do anti-aim quando nao houve silent aim
        if (antiAimApplied && !silentAimApplied) {
            HazzureGame::Vec3 after{};
            if (HazzureInputView::ReadSnapshot(input, after)) {
                HazzureGame::Vec3 restored{std::clamp(after.x+(realView.x-fakeView.x),-89.f,89.f),
                    HazzureGame::NormalizeAngle(after.y+HazzureGame::NormalizeAngle(realView.y-fakeView.y)),0.f};
                if (HazzureInputView::Write(client,input,restored)) InterlockedIncrement(&g_antiAimRestoreOk);
                else InterlockedIncrement(&g_antiAimRestoreFail);
            }
        }

        const bool fire = ConsumeTriggerFire();
        const uint64_t nowMs = GetTickCount64();
        if (fire) {
            g_hookAttackDown = 1;
            g_hookAttackSince = nowMs;
            uintptr_t attackBtn = client + HazzureOffsets::Buttons::attack;
            if (attackBtn) *reinterpret_cast<int32_t*>(attackBtn) = 65537;
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        } else if (g_hookAttackDown && nowMs >= g_hookAttackSince && nowMs - g_hookAttackSince >= 40) {
            HookReleaseAttack();
        }'''

assert target_cm in text, 'target_cm not found'
text = text.replace(target_cm, repl_cm, 1)

with open(path, 'w', encoding='utf-8', newline='') as f:
    f.write(text)
print('InternalStartup.inl ok')
