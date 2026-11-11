using namespace HazzureUI;

// ========================================================================
// Hook CCSGOInput::CreateMove (0x6AEBC4DA: client.dll + 0xD04300)
// Pos-original. A mira e decidida no render thread (alvo, smoothing, RCS) e
// publicada; este hook apenas consome o comando e grava pelo acessor
// verificado do motor (CCSGOInput::SetViewAngles 0xB82890). Nao existe
// segunda logica de alvo aqui: nada de mira/trigger por crosshair sem delay.
// Trigger: um unico pedido por decisao; press e release pertencem ao hook.
// ========================================================================
typedef void(__fastcall* CreateMoveFn)(void* pCSGOInput, int nSlot, void* pCmd);
static CreateMoveFn originalCreateMove = nullptr;

inline constexpr uintptr_t kCreateMoveRva=0xD04300u;
inline constexpr unsigned char kCreateMovePrefix[]={0x48,0x8B,0xC4,0x4C,0x89,0x40,0x18,0x48,0x89,0x48,0x08,0x55,0x53,0x57,0x41,0x55};

static uint64_t g_hookAttackSince = 0;
// Telemetria de consumo do hook: prova se a escrita de mira e o stamp de
// trigger chegam ao CUserCmd vivo, sem alterar o gate fail-closed.
static volatile LONG g_aimWriteOk = 0, g_aimWriteFail = 0, g_triggerStamp = 0;
static volatile LONG g_antiAimRestoreOk = 0, g_antiAimRestoreFail = 0;

static void HookReleaseAttack() {
    InterlockedExchange(&g_hookAttackDown, 0);
    if (client) {
        uintptr_t attackBtn = client + HazzureOffsets::Buttons::attack;
        if (attackBtn) *reinterpret_cast<int32_t*>(attackBtn) = 256;
    }
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

// Build 0x6AEBC4DA: the button globals (0x2727E30..0x2727FE0) are only written
// by static initializers and never read back, so they cannot drive input. The
// engine consumes the CUserCmd returned by CreateMove, which is where the press
// is stamped (same fields as the reference internal: CInButtonState.buttonstate1
// and base.buttons_pb.buttonstate1 + attack1_start_history_index).
static void StampCommandAttack(uintptr_t cmd) {
    const uintptr_t pb=cmd+0x10;
    *reinterpret_cast<uint64_t*>(cmd+0x60)|=1ull;
    const uintptr_t base=*reinterpret_cast<const uintptr_t*>(pb+0x30);
    if(!base)return;
    *reinterpret_cast<uint32_t*>(pb+0x10)|=0x1u;
    const uintptr_t buttons=*reinterpret_cast<const uintptr_t*>(base+0x38);
    if(buttons){
        *reinterpret_cast<uint32_t*>(base+0x10)|=0x2u;
        *reinterpret_cast<uint32_t*>(buttons+0x10)|=0x1u;
        *reinterpret_cast<uint64_t*>(buttons+0x18)|=1ull;
    }
    if(*reinterpret_cast<const int32_t*>(pb+0x20)>0){
        *reinterpret_cast<uint32_t*>(pb+0x10)|=0x20u;
        *reinterpret_cast<int32_t*>(pb+0x3C)=0;
    }
}

// IN_JUMP is bit 1 of CInButtonState. Unlike the trigger this both sets and
// clears the bit: bhop has to break the physical space hold to re-press after
// landing, and the engine already populated the command from the real key.
static void StampCommandStrafe(uintptr_t cmd, int side) {
    if (!cmd || side == 0) return;
    uint64_t& state = *reinterpret_cast<uint64_t*>(cmd + 0x60);
    constexpr uint64_t kLeft = (1ull << 9);
    constexpr uint64_t kRight = (1ull << 10);
    if (side < 0) {
        state = (state | kLeft) & ~kRight;
    } else if (side > 0) {
        state = (state | kRight) & ~kLeft;
    }
    const uintptr_t pb = cmd + 0x10;
    const uintptr_t base = *reinterpret_cast<const uintptr_t*>(pb + 0x30);
    if (!base) return;
    const uintptr_t buttons = *reinterpret_cast<const uintptr_t*>(base + 0x38);
    if (buttons) {
        *reinterpret_cast<uint32_t*>(base + 0x10) |= 0x2u;
        *reinterpret_cast<uint32_t*>(buttons + 0x10) |= 0x1u;
        uint64_t& mirrored = *reinterpret_cast<uint64_t*>(buttons + 0x18);
        if (side < 0) {
            mirrored = (mirrored | kLeft) & ~kRight;
        } else if (side > 0) {
            mirrored = (mirrored | kRight) & ~kLeft;
        }
    }
}

static void StampCommandJump(uintptr_t cmd, bool down, float subTickFraction = 0.0f) {
    if (!cmd) return;
    uint64_t& state = *reinterpret_cast<uint64_t*>(cmd + 0x60);
    state = down ? (state | 2ull) : (state & ~2ull);

    uint64_t& changed = *reinterpret_cast<uint64_t*>(cmd + 0x68);
    changed = down ? (changed | 2ull) : (changed & ~2ull);

    const uintptr_t pb = cmd + 0x10;
    if (!pb) return;

    // Resolução segura de base_ em CSGOUserCmdPB (tenta pb+0x30 e pb+0x28 com validação)
    uintptr_t base = 0;
    uintptr_t candBase = 0;
    if (HazzureGame::Read(pb + 0x30, candBase) && candBase > 0x10000 && !IsBadReadPtr(reinterpret_cast<const void*>(candBase), 0x48)) {
        base = candBase;
    } else if (HazzureGame::Read(pb + 0x28, candBase) && candBase > 0x10000 && !IsBadReadPtr(reinterpret_cast<const void*>(candBase), 0x48)) {
        base = candBase;
    }

    if (base) {
        // buttons_pb_ em CBaseUserCmdPB fica estritamente em base + 0x38 (base + 0x30 é move_crc_!)
        uintptr_t buttons = 0;
        if (HazzureGame::Read(base + 0x38, buttons) && buttons > 0x10000 && !IsBadReadPtr(reinterpret_cast<const void*>(buttons), 0x30)) {
            // Em CInButtonStatePB:
            // +0x10: _has_bits_ (uint32)
            // +0x18: buttonstate1_ (uint64)
            // +0x20: buttonstate2_ (uint64)
            // +0x28: buttonstate3_ (uint64)
            *reinterpret_cast<uint32_t*>(base + 0x10) |= 0x2u; // has_bits para buttons_pb_
            *reinterpret_cast<uint32_t*>(buttons + 0x10) |= 0x7u; // has_bits para buttonstate 1, 2 e 3
            uint64_t& b1 = *reinterpret_cast<uint64_t*>(buttons + 0x18);
            b1 = down ? (b1 | 2ull) : (b1 & ~2ull);
            uint64_t& b2 = *reinterpret_cast<uint64_t*>(buttons + 0x20);
            b2 = down ? (b2 | 2ull) : (b2 & ~2ull);
        }

        // Sub-tick move steps em subtick_moves_ (base + 0x18)
        // Se o usuário está segurando o espaço fisicamente, a engine coloca CSubtickMoveStep com button = 2, pressed = 1.
        // Se down == false, desarmamos esse step para que o servidor veja a tecla solta antes do pouso!
        // Se down == true, asseguramos que o step envie pressed = 1.
        int32_t currentSize = 0;
        if (HazzureGame::Read(base + 0x20, currentSize) && currentSize > 0 && currentSize <= 32) {
            uintptr_t rep = 0;
            if (HazzureGame::Read(base + 0x28, rep) && rep) {
                for (int i = 0; i < currentSize; ++i) {
                    uintptr_t step = 0;
                    if (HazzureGame::Read(rep + 0x08 + static_cast<size_t>(i) * sizeof(void*), step) && step && !IsBadReadPtr(reinterpret_cast<const void*>(step), 0x28)) {
                        uint64_t& btn = *reinterpret_cast<uint64_t*>(step + 0x18);
                        if (btn & 2ull) {
                            if (!down) {
                                btn &= ~2ull;
                                *reinterpret_cast<uint8_t*>(step + 0x20) = 0; // pressed_ = false
                            } else {
                                *reinterpret_cast<uint8_t*>(step + 0x20) = 1; // pressed_ = true
                                *reinterpret_cast<float*>(step + 0x24) = std::clamp(subTickFraction, 0.0f, 0.99f);
                            }
                        }
                    }
                }
            }
        }
    }
}

static void StampCommandButtonMasks(uintptr_t cmd, uint64_t setMask, uint64_t clearMask) {
    if (!cmd || (setMask | clearMask) == 0) return;
    uint64_t& state = *reinterpret_cast<uint64_t*>(cmd + 0x60);
    state = (state | setMask) & ~clearMask;

    uint64_t& changed = *reinterpret_cast<uint64_t*>(cmd + 0x68);
    changed = (changed | setMask) & ~clearMask;

    const uintptr_t pb = cmd + 0x10;
    if (!pb) return;

    uintptr_t base = 0;
    uintptr_t candBase = 0;
    if (HazzureGame::Read(pb + 0x30, candBase) && candBase > 0x10000 && !IsBadReadPtr(reinterpret_cast<const void*>(candBase), 0x48)) {
        base = candBase;
    } else if (HazzureGame::Read(pb + 0x28, candBase) && candBase > 0x10000 && !IsBadReadPtr(reinterpret_cast<const void*>(candBase), 0x48)) {
        base = candBase;
    }

    if (!base) return;
    uintptr_t buttons = 0;
    if (!HazzureGame::Read(base + 0x38, buttons) || !buttons || IsBadReadPtr(reinterpret_cast<const void*>(buttons), 0x30)) return;

    *reinterpret_cast<uint32_t*>(base + 0x10) |= 0x2u;
    *reinterpret_cast<uint32_t*>(buttons + 0x10) |= 0x7u;
    uint64_t& b1 = *reinterpret_cast<uint64_t*>(buttons + 0x18);
    b1 = (b1 | setMask) & ~clearMask;
    uint64_t& b2 = *reinterpret_cast<uint64_t*>(buttons + 0x20);
    b2 = (b2 | setMask) & ~clearMask;
}

// ============================================================================
// T069: Serializacao Direta de Angulos no Sub-tick (CSGOUserCmdPB.input_history_)
// ============================================================================
static bool StampSubtickViewAngles(uintptr_t cmd, float pitch, float yaw) {
    if (!cmd) return false;
    const uintptr_t pb = cmd + 0x10;
    if (!pb) return false;

    int32_t currentSize = 0;
    if (!HazzureGame::Read(pb + 0x20, currentSize) || currentSize <= 0 || currentSize > 64) {
        return false;
    }

    uintptr_t rep = 0;
    if (!HazzureGame::Read(pb + 0x28, rep) || !rep || IsBadReadPtr(reinterpret_cast<const void*>(rep), 0x20)) {
        return false;
    }

    bool anyWritten = false;
    for (int i = 0; i < currentSize; ++i) {
        uintptr_t entry = 0;
        if (!HazzureGame::Read(rep + 0x08 + static_cast<size_t>(i) * sizeof(void*), entry) || !entry || IsBadReadPtr(reinterpret_cast<const void*>(entry), 0x30)) {
            continue;
        }

        uintptr_t qAngle = 0;
        if (!HazzureGame::Read(entry + 0x18, qAngle) || !qAngle || IsBadReadPtr(reinterpret_cast<const void*>(qAngle), 0x28)) {
            continue;
        }

        // 0x1 = VIEWANGLES. O bit 0x2 e SHOOTPOSITION: marca o campo 2 como presente,
        // e o Clear() do motor (client.dll+0x7B0C80) le [entry+0x20] e grava em
        // [rax+0x18] - ponteiro NULL, 0xC0000005. Medido em dois dumps de 08/11.
        *reinterpret_cast<uint32_t*>(entry + 0x10) |= 0x1u; // has view_angles
        *reinterpret_cast<uint32_t*>(qAngle + 0x10) |= 0x7u; // has x, y, z
        *reinterpret_cast<float*>(qAngle + 0x18) = std::clamp(pitch, -89.0f, 89.0f);
        *reinterpret_cast<float*>(qAngle + 0x1C) = HazzureGame::NormalizeAngle(yaw);
        *reinterpret_cast<float*>(qAngle + 0x20) = 0.0f;
        anyWritten = true;
    }
    return anyWritten;
}

void __fastcall Hook_CreateMove(void* pCSGOInput, int nSlot, void* pCmd) {
    const uintptr_t input=reinterpret_cast<uintptr_t>(pCSGOInput);
    const uintptr_t cmd=reinterpret_cast<uintptr_t>(pCmd);
    // Camada de anti-aim / silent aim: os angulos falsos existem apenas enquanto o
    // motor constrói este comando. O original serializa a partir do estado vivo, mas
    // o que o servidor ve e o que esta dentro do CUserCmd — entao, com o campo de
    // angulos do comando ja conhecido, o vencedor do tick e escrito la. Enquanto o
    // probe nao fecha, vale o caminho antigo pelo SetViewAngles (escrever antes do
    // original e devolver depois), que e o unico jeito disponivel sem o offset.
    AntiAimCommand antiAim{};
    HazzureGame::Vec3 realView{},fakeView{},commanded{};
    LONG64 sequence=0;
    static LONG64 appliedSilentSequence=0;
    static HazzureInputView::CmdAnglesProbe cmdProbe{};
    static int fakeLagCounter=0;
    bool antiAimApplied=false;
    bool silentAimApplied=false;
    bool shotTick=false,aimFresh=false,antiAimWanted=false,silentOwnsTick=false,aimOwnsTick=false;
    __try{
        if(pCSGOInput&&client&&HazzureInputView::ReadSnapshot(input,realView)){
            // Dono do tick, uma vez e no lugar certo:
            //  - tick de disparo (pedido de auto-fire/trigger, janela de 40 ms aberta,
            //    ou botao fisico) com rage+Silent Aim: o angulo do tiro manda;
            //  - rage sem Silent Aim com comando de mira fresco: a camera E a mira,
            //    entao o anti-aim nao entra (atiraria de costas);
            //  - todo o resto: o anti-aim aplica.
            aimFresh=ReadAimCommand(commanded,&sequence);
            antiAimWanted=ReadAntiAimCommand(antiAim);
            shotTick=PeekTriggerFire()||g_hookAttackDown!=0||
                (GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
            InterlockedExchange(&g_shotTickLast,shotTick?1:0);
            silentOwnsTick=shotTick&&g_cfg.rageActive&&g_cfg.rageSilentAim&&
                aimFresh&&sequence!=appliedSilentSequence;
            aimOwnsTick=g_cfg.rageActive&&!g_cfg.rageSilentAim&&aimFresh;
            if(cmdProbe.offset<0){
                if(silentOwnsTick){
                    // Silent Aim real: escreve o angulo resolvido apenas no comando que leva
                    // o disparo e restaura a visao do jogador logo apos o original.
                    // Em Perfect Silent, NUNCA tocamos em pCSGOInput (zero camera snap).
                    if (!g_cfg.ragePerfectSilent) {
                        fakeView=commanded;
                        silentAimApplied=HazzureInputView::Write(client,input,fakeView);
                        if(silentAimApplied){
                            appliedSilentSequence=sequence;
                            InterlockedIncrement(&g_aimWriteOk);
                        }
                    }
                }else if(!aimOwnsTick&&antiAimWanted&&!g_cfg.rageDesync){
                    fakeView={std::clamp(antiAim.pitch,-89.f,89.f),HazzureGame::NormalizeAngle(realView.y+antiAim.yawOffset),0.f};
                    antiAimApplied=HazzureInputView::Write(client,input,fakeView);
                    if(antiAimApplied)InterlockedIncrement(&g_antiAimApplyOk);
                    else InterlockedIncrement(&g_antiAimApplyFail);
                }
                InterlockedExchange(&g_silentAimAppliedLast,silentAimApplied?1:0);
                InterlockedExchange(&g_antiAimAppliedLast,antiAimApplied?1:0);
            }
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){antiAimApplied=false;silentAimApplied=false;}

    // Fake Lag: suppress commands to desync networked position
    if(g_cfg.rageActive&&g_cfg.rageFakeLag&&!shotTick){
        if(fakeLagCounter<g_cfg.rageFakeLagTicks){
            ++fakeLagCounter;
            if(antiAimApplied||silentAimApplied)__try{HazzureInputView::Write(client,input,realView);}__except(EXCEPTION_EXECUTE_HANDLER){}
            return; // choked: server never sees this tick
        }
        fakeLagCounter=0;
    }else{fakeLagCounter=0;}

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
        // A devolucao do anti-aim vem PRIMEIRO. Rodar depois da escrita da mira fazia a
        // subtracao do offset incidir sobre o angulo que o aimbot acabou de gravar,
        // girando a mira ate 180 graus a cada comando — a interferencia relatada.
        if (antiAimApplied) {
            HazzureGame::Vec3 after{};
            if (HazzureInputView::ReadSnapshot(input, after)) {
                HazzureGame::Vec3 restored{std::clamp(after.x+(realView.x-fakeView.x),-89.f,89.f),
                    HazzureGame::NormalizeAngle(after.y+HazzureGame::NormalizeAngle(realView.y-fakeView.y)),0.f};
                if (HazzureInputView::Write(client,input,restored)) InterlockedIncrement(&g_antiAimRestoreOk);
                else InterlockedIncrement(&g_antiAimRestoreFail);
            }
        }

        if (silentAimApplied) {
            // Restaura camera do jogador imediatamente - tela 100% estavel sem snap
            HazzureInputView::Write(client, input, realView);
        }

        // Probe do campo de angulos do comando: o CUserCmd recem-preenchido contem ou
        // a vista real ou o angulo falso que o caminho legado escreveu antes do
        // original. Registrar QUAL dos dois apareceu e a unica prova disponivel de que
        // o angulo enviado ao servidor existe — e e ela que decide o proximo passo.
        HazzureInputView::CmdAngleRef refs[2];
        refs[0].angles=realView; refs[0].fake=false;
        int refCount=1;
        if (antiAimApplied||silentAimApplied) { refs[1].angles=fakeView; refs[1].fake=true; refCount=2; }
        HazzureInputView::ProbeCmdAngles(cmdProbe,cmd,refs,refCount);
        InterlockedExchange(&g_antiAimCmdOffset,cmdProbe.offset);
        InterlockedExchange(&g_antiAimCmdRuns,cmdProbe.runs);
        InterlockedExchange(&g_antiAimCmdHits,cmdProbe.bestHits);
        InterlockedExchange(&g_antiAimCmdVaried,cmdProbe.variedRuns);
        InterlockedExchange(&g_antiAimCmdFakeHits,cmdProbe.bestFakeHits);

        bool silentInCmd=false,antiAimInCmd=false,desyncInCmd=false;
        float desyncYawSent=realView.y;
        InterlockedExchange(&g_desyncAppliedLast,0);
        if (silentOwnsTick) {
            // T069: Perfect Silent Sub-tick (Zero Snap)
            bool subtickWritten = StampSubtickViewAngles(cmd, commanded.x, commanded.y);
            if (cmdProbe.offset >= 0) {
                silentInCmd = HazzureInputView::WriteCmdAngles(cmd, cmdProbe.offset, commanded.x, commanded.y, nullptr);
            }
            if (subtickWritten || silentInCmd || silentAimApplied) {
                appliedSilentSequence = sequence;
                InterlockedIncrement(&g_aimWriteOk);
                InterlockedExchange(&g_silentAimAppliedLast, 1);
                InterlockedExchange(&g_perfectSilentApplied, 1);
                RecordFiringAngle(commanded);
            }
        } else if (!aimOwnsTick && antiAimWanted && g_cfg.rageDesync) {
            // T071: Desync. O tick e do anti-aim, mas o angulo nao vai para a camera e
            // sim direto para o sub-tick que o servidor consome (mesmo canal provado na
            // T070). O offset e limitado pelo usuario porque o gap real-vs-enviado acima
            // do que o AnimGraph do servidor absorve gira o corpo de verdade — e o
            // numero exato desse limite nao esta medido por nos, por isso virou slider.
            const float fakeLimit = std::clamp(g_cfg.rageDesyncFakeLimit, 0.f, 60.f);
            float aaOffset = std::clamp(antiAim.yawOffset, -fakeLimit, fakeLimit);
            // Body Yaw: alterna o lado do corpo a cada tick
            if(g_cfg.rageBodyYaw){static bool bodyYawSide=false;bodyYawSide=!bodyYawSide;aaOffset=bodyYawSide?std::fabs(aaOffset):-std::fabs(aaOffset);}
            desyncYawSent = HazzureGame::NormalizeAngle(realView.y + aaOffset);
            desyncInCmd = StampSubtickViewAngles(cmd, std::clamp(antiAim.pitch, -89.f, 89.f), desyncYawSent);
            InterlockedExchange(&g_desyncAppliedLast, desyncInCmd ? 1 : 0);
            InterlockedExchange(&g_antiAimAppliedLast, desyncInCmd ? 1 : 0);
            if (desyncInCmd) {
                InterlockedIncrement(&g_desyncStampOk);
                InterlockedExchange(&g_desyncYawReal, static_cast<LONG>(realView.y * 100));
                InterlockedExchange(&g_desyncYawSent, static_cast<LONG>(desyncYawSent * 100));
            } else InterlockedIncrement(&g_desyncStampFail);
        } else if (cmdProbe.offset>=0) {
            if (!aimOwnsTick&&antiAimWanted) {
                HazzureGame::Vec3 readBack{};
                antiAimInCmd=HazzureInputView::WriteCmdAngles(cmd,cmdProbe.offset,
                    std::clamp(antiAim.pitch,-89.f,89.f),
                    HazzureGame::NormalizeAngle(realView.y+antiAim.yawOffset),&readBack);
                InterlockedExchange(&g_antiAimAppliedLast,antiAimInCmd?1:0);
                if(antiAimInCmd){
                    // Leitura de volta do que ficou no comando, nao a afirmação de quem
                    // chamou a escrita.
                    InterlockedIncrement(&g_antiAimCmdOk);
                    InterlockedExchange(&g_antiAimCmdPitch,static_cast<LONG>(readBack.x*100));
                    InterlockedExchange(&g_antiAimCmdYaw,static_cast<LONG>(readBack.y*100));
                } else InterlockedIncrement(&g_antiAimCmdFail);
            } else InterlockedExchange(&g_antiAimAppliedLast,0);
        } else InterlockedExchange(&g_antiAimAppliedLast,0);

        // Camera do aimbot. O portao antigo era `!rageSilentAim` isolado, e isso
        // desligava o aimbot LEGITIMO sempre que o Silent Aim — que e uma chave do
        // rage — estava ligado: `aimWriteOk` ficou congelado a sessao inteira com
        // `[GAMEPLAY] active=1 target=1`. Agora apenas o rage com Silent Aim deixa de
        // mover a camera; sem rage o aimbot escreve independente do Silent Aim.
        if (!(g_cfg.rageActive&&g_cfg.rageSilentAim)) {
            HazzureGame::Vec3 view{};
            if (HazzureInputView::ReadSnapshot(input, view)) {
                HazzureGame::Vec3 aimCommand{};
                LONG64 aimSequence = 0;
                static LONG64 appliedSequence = 0;
                const bool wantWrite = ReadAimCommand(aimCommand, &aimSequence) && aimSequence != appliedSequence;
                if (wantWrite) {
                    if (HazzureInputView::Write(client, input, aimCommand)) {
                        appliedSequence = aimSequence;
                        InterlockedIncrement(&g_aimWriteOk);
                    } else {
                        InterlockedIncrement(&g_aimWriteFail);
                    }
                }
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
        }
        // While the click window is open every command carries the bit; the
        // first command built after the hold elapses is clean, so release needs
        // no extra write.
        if (g_hookAttackDown) {
            StampCommandAttack(reinterpret_cast<uintptr_t>(pCmd));
            InterlockedIncrement(&g_triggerStamp);
        }

        // =====================================================================
        // T062: Execucao de Lineup Nativa CS2 (Sub-tick CUserCmd / IN_ATTACK + IN_JUMP)
        // =====================================================================
        if (HazzureLineupAssist::g_pendingLineup.armed.load()) {
            int throwMode = HazzureLineupAssist::g_pendingLineup.throwMode;
            float when = HazzureLineupAssist::g_pendingLineup.when;
            const uintptr_t pb = cmd + 0x10;

            if (pb && *reinterpret_cast<const int32_t*>(pb + 0x20) > 0) {
                *reinterpret_cast<uint32_t*>(pb + 0x10) |= 0x20u; // has attack1_start_history_index
                *reinterpret_cast<int32_t*>(pb + 0x3C) = 0;        // index 0
            }

            if (throwMode == 1) { // JUMPTHROW: IN_JUMP e IN_ATTACK no mesmo sub-tick
                StampCommandJump(reinterpret_cast<uintptr_t>(pCmd), true);
                StampCommandAttack(reinterpret_cast<uintptr_t>(pCmd));
            } else if (throwMode == 4) { // UNDERHAND: attack2
                if (pb && *reinterpret_cast<const int32_t*>(pb + 0x20) > 0) {
                    *reinterpret_cast<uint32_t*>(pb + 0x10) |= 0x40u; // has attack2_start_history_index
                    *reinterpret_cast<int32_t*>(pb + 0x40) = 0;        // index 0
                }
                *reinterpret_cast<uint64_t*>(cmd + 0x60) |= 2048ull; // IN_ATTACK2
                const uintptr_t base = *reinterpret_cast<const uintptr_t*>(pb + 0x30);
                if (base) {
                    const uintptr_t buttons = *reinterpret_cast<const uintptr_t*>(base + 0x38);
                    if (buttons) *reinterpret_cast<uint64_t*>(buttons + 0x18) |= 2048ull;
                }
            } else { // STAND / CROUCH / RUNTHROW
                StampCommandAttack(reinterpret_cast<uintptr_t>(pCmd));
            }

            LogDebug("[LINEUP] map=%s name=%s mode=%d dxy=%.1f when=%.3f executed_subtick=0",
                HazzureLineupAssist::g_pendingLineup.map,
                HazzureLineupAssist::g_pendingLineup.name,
                throwMode,
                HazzureLineupAssist::g_pendingLineup.dxy,
                when);

            HazzureLineupAssist::g_pendingLineup.armed.store(false);
        }
        // =====================================================================
        // T060: Motor de Movimento Físico no CUserCmd (Autobhop + Strafe + Fix)
        // =====================================================================
        const uintptr_t localPawn = HazzureGame::LocalPawn(client);
        HazzureGame::Pawn localPawnData{};
        bool localPawnValid = localPawn && HazzureGame::ReadPawn(localPawn, localPawnData) &&
                              localPawnData.health > 0 && localPawnData.life == 0;
        int pawnFlags = 0;
        if (localPawnValid) {
            HazzureGame::Read(localPawn + HazzureOffsets::Schema::m_fFlags, pawnFlags);
        }
        const bool pawnOnGround = (pawnFlags & 1) != 0;
        const bool spacePhysicallyHeld = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        static bool s_lastBhopAsserted = false;

        // tick: 1 | reason: Briefing A — Bhop + StrafeSolver unificado
        static int s_groundTicks = 0;
        static bool s_wasAir = true;
        static int s_holdTicksLeft = 0;
        static uint64_t s_humanizeDelay = 0;
        static float s_prevViewYaw = 0.0f;
        static int s_turnSign = 1;
        static bool s_bhopToggled = false;

        // tick: 2 | reason: mode resolve (hold/toggle/always)
        bool bhopWant = false;
        if (g_cfg.bhopActive && localPawnValid) {
            if (g_cfg.bhopMode == 2) bhopWant = true; // always
            else if (g_cfg.bhopMode == 1) { // toggle
                if (spacePhysicallyHeld && !s_bhopToggled) { s_bhopToggled = true; bhopWant = true; }
                else if (!spacePhysicallyHeld) s_bhopToggled = false;
                else bhopWant = s_bhopToggled;
            } else bhopWant = spacePhysicallyHeld; // hold
        }
        // tick: 3 | reason: onlyMoving + minSpeedGate
        float hSpeed = localPawnValid ? std::hypot(localPawnData.velocity.x, localPawnData.velocity.y) : 0.f;
        if (bhopWant && g_cfg.bhopOnlyMoving && hSpeed < 5.0f &&
            !(GetAsyncKeyState('W') & 0x8000) && !(GetAsyncKeyState('A') & 0x8000) &&
            !(GetAsyncKeyState('S') & 0x8000) && !(GetAsyncKeyState('D') & 0x8000))
            bhopWant = false;

        // tick: 4 | reason: ground tracking
        if (localPawnValid) {
            if (pawnOnGround) { if (s_wasAir) { s_groundTicks = 1; s_wasAir = false; } else s_groundTicks++; }
            else { s_groundTicks = 0; s_wasAir = true; }
        }

        // tick: 5 | reason: jump prediction — emit IN_JUMP 1 tick before landing
        bool predictJump = false;
        if (bhopWant && !pawnOnGround && localPawnValid && localPawnData.velocityValid) {
            float landFrac = 1.0f;
            if (HazzureMovementTech::PredictContact(client, localPawn, localPawnData.origin,
                    localPawnData.velocity, false, landFrac) && landFrac < 0.5f)
                predictJump = true;
        }

        // tick: 6 | reason: humanize delay on re-press
        bool forcePress = false;
        if (bhopWant && localPawnValid) {
            if (pawnOnGround || predictJump) {
                if (s_humanizeDelay == 0 && g_cfg.bhopHumanizeMs > 0 && s_groundTicks == 1)
                    s_humanizeDelay = nowMs + (std::rand() % (g_cfg.bhopHumanizeMs + 1));
                if (s_humanizeDelay == 0 || nowMs >= s_humanizeDelay) {
                    forcePress = true;
                    s_holdTicksLeft = g_cfg.bhopHoldTicks;
                    s_humanizeDelay = 0;
                }
            } else if (s_holdTicksLeft > 0) {
                forcePress = true; s_holdTicksLeft--;
            } else forcePress = false;

            StampCommandJump(reinterpret_cast<uintptr_t>(pCmd), forcePress, 0.01f);
            InterlockedIncrement(&HazzureMove::jumpStampCount);
            InterlockedExchange(&HazzureMove::jumpStampDown, forcePress ? 1 : 0);
        } else {
            s_holdTicksLeft = 0; s_humanizeDelay = 0;
            HazzureMovementTech::g_predictedLandingTick.store(-1);
            bool jumpDown = false;
            if (HazzureMove::ReadJumpCommand(jumpDown)) {
                StampCommandJump(reinterpret_cast<uintptr_t>(pCmd), jumpDown);
                InterlockedIncrement(&HazzureMove::jumpStampCount);
                InterlockedExchange(&HazzureMove::jumpStampDown, jumpDown ? 1 : 0);
            }
        }

        // tick: 7 | reason: friction suppression — zero move buttons on ground contact tick
        if (bhopWant && g_cfg.bhopPreserveOnLand && pawnOnGround && s_groundTicks == 1) {
            uint64_t frictionClr = (1ull<<3)|(1ull<<4)|(1ull<<9)|(1ull<<10);
            StampCommandButtonMasks(reinterpret_cast<uintptr_t>(pCmd), 0, frictionClr);
        }
        // tick: 8 | reason: air strafe via SolveStrafe puro
        else if (localPawnValid && !pawnOnGround && localPawnData.velocityValid &&
                 hSpeed > g_cfg.bhopMinSpeedGate &&
                 (bhopWant || g_cfg.autoStrafePhysics || g_cfg.bhopBoost || g_cfg.autoStrafe)) {
            float curViewYaw = realView.y;
            float mouseDelta = HazzureGame::NormalizeAngle(curViewYaw - s_prevViewYaw);
            s_prevViewYaw = curViewYaw;
            bool aHeld = (GetAsyncKeyState('A') & 0x8000) != 0;
            bool dHeld = (GetAsyncKeyState('D') & 0x8000) != 0;
            if (aHeld && !dHeld) s_turnSign = 1;
            else if (dHeld && !aHeld) s_turnSign = -1;
            else if (std::fabs(mouseDelta) > 0.04f) s_turnSign = (mouseDelta > 0.0f) ? 1 : -1;

            auto sc = HazzureMovementTech::SolveStrafe(
                localPawnData.velocity, curViewYaw, s_turnSign,
                static_cast<float>(g_cfg.bhopStrafeGain), g_cfg.bhopAngleStep);
            if (sc.valid) {
                uint64_t setS = 0, clrS = 0;
                if (sc.fwd > 30.0f) { setS |= (1ull << 3); clrS |= (1ull << 4); }
                else if (sc.fwd < -30.0f) { setS |= (1ull << 4); clrS |= (1ull << 3); }
                if (sc.side < -30.0f) { setS |= (1ull << 9); clrS |= (1ull << 10); }
                else if (sc.side > 30.0f) { setS |= (1ull << 10); clrS |= (1ull << 9); }
                StampCommandButtonMasks(reinterpret_cast<uintptr_t>(pCmd), setS, clrS);
            }
        }

        // Outras máscaras de botões (Crouch-Jump, Fast Stop, etc.)
        uint64_t setBits = 0, clearBits = 0;
        if (HazzureMovementTech::ReadCmdButtonMasks(setBits, clearBits)) {
            StampCommandButtonMasks(reinterpret_cast<uintptr_t>(pCmd), setBits, clearBits);
        }

        // 3. Movement Fix / Reprojeção 2D de Inputs (Item 10 do Briefing Rage)
        // O servidor resolve forwardmove/sidemove contra o m_view_angles enviado.
        // Se qualquer ângulo de mira ou Anti-Aim foi aplicado, reprojetamos o vetor de movimento
        // da base real para a base enviada, garantindo linha reta no WASD e no strafe.
        const bool angleDiverged = (antiAimInCmd || silentInCmd || antiAimApplied || silentAimApplied || silentOwnsTick || desyncInCmd);
        float sentYaw = realView.y;
        if (silentInCmd || silentAimApplied || silentOwnsTick) {
            sentYaw = commanded.y;
        } else if (desyncInCmd) {
            sentYaw = desyncYawSent;
        } else if (antiAimInCmd || antiAimApplied) {
            sentYaw = HazzureGame::NormalizeAngle(realView.y + antiAim.yawOffset);
        }

        const float diffYaw = HazzureGame::NormalizeAngle(sentYaw - realView.y);
        if (angleDiverged && std::fabs(diffYaw) > 0.05f) {
            uint64_t& cmdBtnState = *reinterpret_cast<uint64_t*>(cmd + 0x60);
            float origFwd = 0.0f, origSide = 0.0f;
            if (cmdBtnState & (1ull << 3)) origFwd += 450.0f;
            if (cmdBtnState & (1ull << 4)) origFwd -= 450.0f;
            if (cmdBtnState & (1ull << 10)) origSide += 450.0f;
            if (cmdBtnState & (1ull << 9)) origSide -= 450.0f;

            if (origFwd != 0.0f || origSide != 0.0f) {
                float rotFwd = origFwd;
                float rotSide = origSide;
                HazzureMovementTech::RotateInputs(rotFwd, rotSide, realView.y, sentYaw);

                uint64_t mFixSet = 0, mFixClr = 0;
                if (rotFwd > 30.0f) { mFixSet |= (1ull << 3); mFixClr |= (1ull << 4); }
                else if (rotFwd < -30.0f) { mFixSet |= (1ull << 4); mFixClr |= (1ull << 3); }
                else { mFixClr |= (1ull << 3) | (1ull << 4); }

                if (rotSide > 30.0f) { mFixSet |= (1ull << 10); mFixClr |= (1ull << 9); }
                else if (rotSide < -30.0f) { mFixSet |= (1ull << 9); mFixClr |= (1ull << 10); }
                else { mFixClr |= (1ull << 9) | (1ull << 10); }

                StampCommandButtonMasks(reinterpret_cast<uintptr_t>(pCmd), mFixSet, mFixClr);
            }
        }
        // Throttled consumption proof: shows whether the aim write and the
        // trigger stamp actually reach the live CUserCmd during a match.
        static uint64_t desyncLogged = 0;
        if (nowMs - desyncLogged >= 1000) {
            desyncLogged = nowMs;
            const float loggedReal = InterlockedCompareExchange(&g_desyncYawReal, 0, 0) / 100.0f;
            const float loggedSent = InterlockedCompareExchange(&g_desyncYawSent, 0, 0) / 100.0f;
            LogDebug("[DESYNC] cfg=%d aplicado=%d yawReal=%.2f yawSent=%.2f delta=%.2f limit=%.0f ok=%ld fail=%ld",
                g_cfg.rageDesync ? 1 : 0,
                (int)InterlockedCompareExchange(&g_desyncAppliedLast, 0, 0),
                loggedReal, loggedSent,
                HazzureGame::NormalizeAngle(loggedSent - loggedReal),
                std::clamp(g_cfg.rageDesyncFakeLimit, 0.f, 60.f),
                InterlockedCompareExchange(&g_desyncStampOk, 0, 0),
                InterlockedCompareExchange(&g_desyncStampFail, 0, 0));
        }
        static uint64_t aimLogged = 0;
        if (nowMs - aimLogged >= 2000) {
            aimLogged = nowMs;
            LogDebug("[AIMTRIG] aimWriteOk=%ld aimWriteFail=%ld triggerStamp=%ld attackDown=%d aaRestoreOk=%ld aaRestoreFail=%ld aaApplyOk=%ld aaApplyFail=%ld aaApplied=%d silentApplied=%d shotTick=%d cmdOff=%ld cmdFake=%ld cmdOk=%ld cmdRead=%.2f/%.2f desyncOk=%ld desyncFail=%ld",
                g_aimWriteOk, g_aimWriteFail, g_triggerStamp, (int)g_hookAttackDown,
                g_antiAimRestoreOk, g_antiAimRestoreFail,
                InterlockedCompareExchange(&g_antiAimApplyOk,0,0),InterlockedCompareExchange(&g_antiAimApplyFail,0,0),
                (int)InterlockedCompareExchange(&g_antiAimAppliedLast,0,0),
                (int)InterlockedCompareExchange(&g_silentAimAppliedLast,0,0),
                (int)InterlockedCompareExchange(&g_shotTickLast,0,0),
                InterlockedCompareExchange(&g_antiAimCmdOffset,-1,-1),
                InterlockedCompareExchange(&g_antiAimCmdFakeHits,0,0),
                InterlockedCompareExchange(&g_antiAimCmdOk,0,0),
                InterlockedCompareExchange(&g_antiAimCmdPitch,0,0)/100.0,
                InterlockedCompareExchange(&g_antiAimCmdYaw,0,0)/100.0,
                InterlockedCompareExchange(&g_desyncStampOk,0,0),
                InterlockedCompareExchange(&g_desyncStampFail,0,0));
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        HookReleaseAttack();
    }
}

static void __fastcall Hook_DrawScopeOverlay(void* pPawn, void* pScopeData) {
    if (!HazzureScopeOverlay::originalDrawScopeOverlay) return;

    if (!g_cfg.noScopeOverlay || !pPawn || !client) {
        HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
        return;
    }

    __try {
        // 1. Estado necessario: C_CSPlayerPawnBase::m_bIsScoped
        uint8_t isScoped = 0;
        uintptr_t scpOff = HazzureSchema::g_mIsScoped.offset ? HazzureSchema::g_mIsScoped.offset : 0x1EA0u;
        if (!SafeRead(reinterpret_cast<uintptr_t>(pPawn) + scpOff, &isScoped) || isScoped != 1) {
            HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
            return;
        }

        // 2. Verificar se e o local pawn
        uintptr_t localPawn = HazzureGame::LocalPawn(client);
        if (reinterpret_cast<uintptr_t>(pPawn) != localPawn) {
            HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
            return;
        }

        // 3. Verificar arma ativa
        uintptr_t entityList = 0;
        if (!SafeRead(client + HazzureOffsets::Globals::dwEntityList, &entityList) || !entityList) {
            HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
            return;
        }

        uintptr_t activeWeapon = 0;
        uint16_t def = ActiveWeaponDefinition(localPawn, entityList, &activeWeapon);
        // weapon_is_sniper cobre AWP (9), SSG08 (40), SCAR-20 (38), G3SG1 (11), AUG (8), SG553 (39)
        bool isSniper = (def == 9 || def == 40 || def == 38 || def == 11);
        bool isAugSg = (def == 8 || def == 39);

        if (!isSniper && !isAugSg) {
            HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
            return;
        }

        // 4. AUG/SG553 usam dot em vez de overlay completo:
        // Se dot customizado nao estiver ativo, preservar o desenho original da reticula nativa
        if (isAugSg && !g_cfg.scopeCustomDotAug) {
            HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
            return;
        }

        // 5. Diferenciacao de chamadores:
        // Caller 1 (~client + 0xcf1e7f em ClientModeCSNormal): calcula Zoom FOV e reticula da arma.
        // Deve executar originalDrawScopeOverlay para manter os calculos de zoom intactos.
        // Caller 2 (~client + 0x11dece1 em CRenderingPipelineCsgo): desenha a vinheta preta (legacy_sniper_scope.vmat).
        // Aqui suprimimos: zeramos pScopeData + 4 para anular a mascara preta do pos-processamento.
        uintptr_t retAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
        uintptr_t retRva = (retAddr > client) ? (retAddr - client) : 0;
        if (retRva >= 0xce0000 && retRva <= 0xd10000) {
            HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
            return;
        }

        // Suprimir quad de vinheta preta do pipeline de pos-processamento
        if (pScopeData) {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pScopeData) + 4) = 0;
        }

        HazzureScopeOverlay::suppressedFrames.fetch_add(1, std::memory_order_relaxed);
        // Retorno sem chamar o original: remove o preto do scope nativo!
        return;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (HazzureScopeOverlay::originalDrawScopeOverlay) {
            HazzureScopeOverlay::originalDrawScopeOverlay(pPawn, pScopeData);
        }
    }
}

static DWORD WINAPI Start(void*) {


    LogDebug("==================================================");


    LogDebug("[START] Hazzure Internal Start() iniciado (PID=%lu)", GetCurrentProcessId());

    LogDebug("[BUILD] revision=requests-20261027-v1 compiled=%s %s", __DATE__, __TIME__);


    wchar_t name[100]; swprintf_s(name, L"Local\\HazzureInternal.%lu", GetCurrentProcessId());


    mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 4096, name);


    if (mapping) {


        shared = static_cast<Shared*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared)));


        if (shared) {


            shared->magic = 0x485a4931; shared->version = 2; shared->visibleColour = 0xffffff00; shared->hiddenColour = 0xffff00aa;


            shared->armsColour = 0x38ffffff; shared->teamFilter = 1; shared->status = 1; shared->gradient = 0; shared->gradientColour = 0xff30c0ff;


            InterlockedExchange(&shared->flags, 1 | 8 | 16 | 32);


            InterlockedExchange(&shared->uiConfigOwner, 1);


            LogDebug("[START] Memoria compartilhada IPC conectada com sucesso");


        }


    }


    // Validacao de modulos SEM early return ou bloqueio de hooks


    auto scene = GetModuleHandleW(L"scenesystem.dll");


    auto materials = GetModuleHandleW(L"materialsystem2.dll");


    auto cl = GetModuleHandleW(L"client.dll");


    auto tier = GetModuleHandleW(L"tier0.dll");


    auto resources = GetModuleHandleW(L"resourcesystem.dll");


    LogDebug("[START] Modulos carregados: client=0x%llX, scene=0x%llX, materials=0x%llX, tier=0x%llX, resources=0x%llX",
        reinterpret_cast<uintptr_t>(cl), reinterpret_cast<uintptr_t>(scene),
        reinterpret_cast<uintptr_t>(materials), reinterpret_cast<uintptr_t>(tier),
        reinterpret_cast<uintptr_t>(resources));


    bool sceneMatched = MatchModule(scene, SCENE_STAMP, SCENE_SIZE);


    bool matMatched = MatchModule(materials, MATERIAL_STAMP, MATERIAL_SIZE);


    bool clientMatched = MatchModule(cl, CLIENT_STAMP, CLIENT_SIZE);


    bool modulesMatched = sceneMatched && matMatched && clientMatched;


    if (!modulesMatched) {


        LogDebug("[AVISO] Assinatura do modulo divergiu (scene=%d, mat=%d, client=%d). Continuando e forcando hooks...",
            sceneMatched, matMatched, clientMatched);


        Message("Aviso: Module identity mudou, forcando hooks...");


        if (shared) shared->status = -2;


    } else {


        LogDebug("[START] Todos os modulos conferem com a assinatura conhecida");


    }


    client = reinterpret_cast<uintptr_t>(cl);


    // Executa o Pattern Scanner em memoria no client.dll e grava no debug_log.txt


    if (client) {


        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(client);


        auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(client + dos->e_lfanew);


        size_t clientSize = nt->OptionalHeader.SizeOfImage;


        HazzureScanner::RunPatternScan(client, clientSize);


    } else {


        LogDebug("[ERRO] client.dll nao encontrado para escaneamento de padroes!");


    }


    auto factory = materials ? reinterpret_cast<Factory>(GetProcAddress(materials, "CreateInterface")) : nullptr;


    materialSystem = factory ? factory("VMaterialSystem2_001", nullptr) : nullptr;


    if (materialSystem) {


        auto table = Get<uintptr_t>(reinterpret_cast<uintptr_t>(materialSystem));


        auto target = Get<uintptr_t>(table + 29 * 8);


        createMaterial = reinterpret_cast<CreateMaterial>(target);


        LogDebug("[START] MaterialSystem2 conectado. createMaterial=0x%llX", target);


        auto findTarget = Get<uintptr_t>(table + 14 * 8);


        findMaterialFromResource = reinterpret_cast<FindMaterialFromResource>(findTarget);


        LogDebug("[START] MaterialSystem2 findMaterialFromResource=0x%llX", findTarget);

    } else {


        LogDebug("[AVISO] MaterialSystem2 interface missing");


    }


    auto resourceFactory = resources ? reinterpret_cast<Factory>(GetProcAddress(resources, "CreateInterface")) : nullptr;


    resourceSystem = resourceFactory ? resourceFactory("ResourceSystem013", nullptr) : nullptr;


    if (resourceSystem) {


        auto resourceTable = Get<uintptr_t>(reinterpret_cast<uintptr_t>(resourceSystem));


        auto precacheTarget = Get<uintptr_t>(resourceTable + 40 * 8);


        precacheMaterial = reinterpret_cast<PrecacheMaterial>(precacheTarget);


        LogDebug("[START] ResourceSystem conectado. precacheMaterial=0x%llX", precacheTarget);


    } else {


        LogDebug("[AVISO] ResourceSystem013 interface missing");


    }


    insertPath = tier ? reinterpret_cast<InsertPath>(GetProcAddress(tier, "?Insert@CBufferString@@QEAAPEBDHPEBDH_N@Z")) : nullptr;


    purgePath = tier ? reinterpret_cast<PurgePath>(GetProcAddress(tier, "?Purge@CBufferString@@QEAAXH@Z")) : nullptr;


    loadKV = tier ? reinterpret_cast<LoadKV>(GetProcAddress(tier, "?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z")) : nullptr;


    arenaCtor = tier ? reinterpret_cast<ArenaCtor>(GetProcAddress(tier, "??0CUtlMemoryArena@@QEAA@I_N@Z")) : nullptr;


    LogDebug("[START] tier0 helpers: insert=%d purge=%d kv=%d arena=%d", insertPath != nullptr,
        purgePath != nullptr, loadKV != nullptr, arenaCtor != nullptr);


    // MinHook must be initialized before the first MH_CreateHook in this Start().
    MH_STATUS mhStatus = MH_Initialize();
    LogDebug("[HOOK] MH_Initialize status: %d", mhStatus);

    // Callback dependencies still belong to the reviewed installed PE/schema.


    uint32_t skinStamp{},skinSize{};
    const bool skinContract=HazzureSchema::GetModuleFingerprint("client.dll",skinStamp,skinSize)&&
        ((skinStamp==0x6AEBC4DAu&&skinSize==0x29A3000u)||(skinStamp==0x6ADA7340u&&skinSize==0x29A0000u)||
         ((skinStamp==0x6AE4FB84u||skinStamp==0x6AE91E98u)&&skinSize==0x29A2000u))&&
        HazzureVisibility::Prefix(client+SKIN_FRAME.rva,SKIN_FRAME.bytes,sizeof(SKIN_FRAME.bytes));
    if(!skinContract)LogDebug("[HOOK] SkinFrame disabled: dependent contracts require revalidation");
    const bool inventoryContract=HazzureNativeInventory::CurrentProfile::Supported(client,true);
    if(!inventoryContract)LogDebug("[HOOK] Native inventory unavailable: PE/ABI mismatch");
    // Single frame callback at 0xB6EFE0: it owns climate/fog/exposure, visibility
    // queries, movement stages and the native inventory tick. Hooking it twice is
    // impossible, and a separate inventory-only callback silently disabled all the
    // rest (sky/fog never applied while native inventory was active).
    bool nativeCallbackCreated=false;
    if(client&&skinContract){
        void* target=reinterpret_cast<void*>(client+SKIN_FRAME.rva);
        auto sFrame=MH_CreateHook(target,FrameHook,reinterpret_cast<void**>(&originalFrame));
        nativeCallbackCreated=sFrame==MH_OK;
        if(nativeCallbackCreated)HazzureState::RegisterHook("Frame",target,FrameHook,originalFrame);
        LogDebug("[HOOK] MH_CreateHook(Frame) status: %d",sFrame);
    }else LogDebug("[HOOK] Frame unavailable: PE/ABI mismatch");
    // CreateMove: single post-original consumer for the aim write and the
    // trigger press/release pair (no second target logic on this thread).
    if(client&&HazzureVisibility::BuildMatches(client)&&
       HazzureVisibility::Prefix(client+kCreateMoveRva,kCreateMovePrefix,sizeof(kCreateMovePrefix))){
        void* target=reinterpret_cast<void*>(client+kCreateMoveRva);
        auto result=MH_CreateHook(target,Hook_CreateMove,reinterpret_cast<void**>(&originalCreateMove));
        if(result==MH_OK)HazzureState::RegisterHook("CreateMove",target,Hook_CreateMove,originalCreateMove);
        LogDebug("[HOOK] CreateMove create=%d",result);
    }else LogDebug("[HOOK] CreateMove unavailable: PE/ABI mismatch");

    // Camera FOV getter: preserve the native result, override the local camera only.
    bool cameraFovHook=false;
    if(HazzureCamera::Supported(client)){
        void* target=reinterpret_cast<void*>(client+HazzureCamera::getterRva);
        auto created=MH_CreateHook(target,CameraFovHook,reinterpret_cast<void**>(&originalCamera));
        if(created==MH_OK){
            cameraFovHook=true;
            HazzureState::RegisterHook("CameraFov",target,CameraFovHook,originalCamera);
        }
        LogDebug("[HOOK] CameraFov create=%d",created);
    }else LogDebug("[HOOK] CameraFov unavailable: getter prefix mismatch");

    // Sky update: climate writes the fields, then this native callback
    // propagates the strong material handle to the existing scene object.
    if(client&&SkyUpdateSupported(client)){
        void* target=reinterpret_cast<void*>(client+kSkyUpdateRva);
        auto result=MH_CreateHook(target,SkyUpdateHook,reinterpret_cast<void**>(&originalSkyUpdate));
        if(result==MH_OK)HazzureState::RegisterHook("SkyUpdate",target,SkyUpdateHook,originalSkyUpdate);
        LogDebug("[HOOK] SkyUpdate create=%d",result);
    }else LogDebug("[HOOK] SkyUpdate unavailable: callback prefix mismatch");

    // Chams draw callback: scenesystem.dll batch draw, gated by its prologue.
    if (scene) {
        auto draw = reinterpret_cast<unsigned char*>(scene) + 0x59FC0;
        const unsigned char expected[] = { 0x48, 0x8B, 0xC4, 0x48, 0x89, 0x50, 0x10, 0x48, 0x89, 0x48, 0x08 };
        if (memcmp(draw, expected, sizeof(expected)) != 0) {
            LogDebug("[HOOK] Draw prologue mismatch at 0x%llX; chams disabled", reinterpret_cast<uintptr_t>(draw));
        } else {
            auto sDraw = MH_CreateHook(draw, Hook, reinterpret_cast<void**>(&original));
            LogDebug("[HOOK] MH_CreateHook(Draw) status: %d", sDraw);
            if (sDraw == MH_OK) HazzureState::RegisterHook("Draw", draw, Hook, original);
        }
    } else LogDebug("[HOOK] scenesystem.dll missing; chams disabled");

    // DrawScopeOverlay hook: supressao nativa do overlay preto do scope
    bool scopeHookCreated = false;
    if (client) {
        uintptr_t targetScope = HazzureScopeOverlay::FindDrawScopeOverlay(client);
        if (targetScope) {
            auto sScope = MH_CreateHook(reinterpret_cast<void*>(targetScope),
                Hook_DrawScopeOverlay,
                reinterpret_cast<void**>(&HazzureScopeOverlay::originalDrawScopeOverlay));
            if (sScope == MH_OK) {
                scopeHookCreated = true;
                HazzureState::RegisterHook("DrawScopeOverlay", reinterpret_cast<void*>(targetScope),
                    Hook_DrawScopeOverlay, HazzureScopeOverlay::originalDrawScopeOverlay);
                HazzureScopeOverlay::hookInstalled.store(true);
            }
            LogDebug("[HOOK] DrawScopeOverlay create=%d target=0x%llX", sScope, targetScope);
        } else {
            LogDebug("[HOOK] DrawScopeOverlay unavailable: signature mismatch");
        }
    }

    MH_STATUS enableAll = MH_EnableHook(MH_ALL_HOOKS);
    HazzureCamera::installed.store(cameraFovHook&&enableAll==MH_OK);
    HazzureNativeInventory::backendAvailable.store(enableAll==MH_OK&&nativeCallbackCreated&&inventoryContract);

    LogDebug("[HOOK] MH_EnableHook(MH_ALL_HOOKS) status: %d", enableAll);

    // Camera hooks (ClientMode::OverrideView + DrawEntity)
    if (HazzureNativeCamera::Supported(client)) {
        void* viewTarget = reinterpret_cast<void*>(client + HazzureNativeCamera::viewRva);
        void* drawTarget = reinterpret_cast<void*>(client + HazzureNativeCamera::drawRva);
        auto cv = MH_CreateHook(viewTarget, NativeViewHook, reinterpret_cast<void**>(&originalViewSetup));
        auto cd = MH_CreateHook(drawTarget, NativeDrawEntityHook, reinterpret_cast<void**>(&originalDrawEntity));
        auto ev = cv == MH_OK ? MH_EnableHook(viewTarget) : cv;
        auto ed = cd == MH_OK ? MH_EnableHook(drawTarget) : cd;
        if (cv == MH_OK && cd == MH_OK && ev == MH_OK && ed == MH_OK) {
            HazzureState::RegisterHook("NativeView", viewTarget, NativeViewHook, originalViewSetup);
            HazzureState::RegisterHook("NativeLocalBody", drawTarget, NativeDrawEntityHook, originalDrawEntity);
            HazzureNativeCamera::installed.store(true);
            LogDebug("[HOOK] Camera hooks installed successfully");
        } else {
            if (cv == MH_OK) { MH_DisableHook(viewTarget); MH_RemoveHook(viewTarget); originalViewSetup = nullptr; }
            if (cd == MH_OK) { MH_DisableHook(drawTarget); MH_RemoveHook(drawTarget); originalDrawEntity = nullptr; }
            LogDebug("[HOOK] Camera hooks failed: cv=%d cd=%d ev=%d ed=%d", cv, cd, ev, ed);
        }
    } else {
        LogDebug("[HOOK] Camera hooks unavailable: PE/ABI mismatch");
    }

    // Auto-aplicar preset de skins salvo na injeção
    if (enableAll == MH_OK && inventoryContract && shared && shared->status > 0) {
        // The legacy skinchanger toggle is retired; the native SOCache is the
        // only cosmetic path, so a persisted collection must be requested here.
        if (g_cfg.inventory.count > 0) {
            HazzureNativeInventory::Request(g_cfg.inventory);
            LogDebug("[START] Inventory preset aplicado na injeção (count=%zu)", g_cfg.inventory.count);
        }
    }

    // Kiero D3D11 hook
    if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
        bool presentationReady=InstallPresentationHooks(reinterpret_cast<void**>(kiero::getMethodsTable()));
        LogDebug("[KIERO] Resident Present/ResizeBuffers ready=%d",presentationReady);
        Message(presentationReady?"Kiero D3D11 Present/ResizeBuffers residentes ativos":"Aviso: Falha ao instalar Present/ResizeBuffers");
    } else {
        LogDebug("[KIERO] [ERRO] Falha ao inicializar Kiero D3D11");
        Message("Aviso: Falha ao inicializar Kiero D3D11");
    }

    Message("Draw diagnostic active; effects OFF");

    if (shared) shared->status = 2;

    LogDebug("[START] Start() finalizado com sucesso.");
    LogDebug("==================================================");

    return 0;
}