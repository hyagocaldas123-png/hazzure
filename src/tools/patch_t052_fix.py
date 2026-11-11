import sys

# =========================================================================
# 4. GameplayRuntime.h - Anti-Aim 180 Jitter + Auto-stop fix
# =========================================================================
path_gr = r'C:\Codex2\gpt\HazzureInternal\src\GameplayRuntime.h'
with open(path_gr, 'r', encoding='utf-8') as f:
    text_gr = f.read()

# Auto-stop fix: do not block 'ready' from firing
target_gr_stop = '''    if(ready&&rage&&RuntimeBool("rageAutoStop",g_cfg.rageAutoStop,"enemy",weaponClass)){
        float limit=std::clamp(RuntimeFloat("rageAutoStopSpeed",g_cfg.rageAutoStopSpeed,"enemy",weaponClass),0.f,300.f);
        if(!local.velocityValid||std::hypot(local.velocity.x,local.velocity.y)>limit){
            ready=false;g_frameDiagnostics.aimStatus="auto-stop";
        }
    }'''

repl_gr_stop = '''    if(ready&&rage&&RuntimeBool("rageAutoStop",g_cfg.rageAutoStop,"enemy",weaponClass)){
        float limit=std::clamp(RuntimeFloat("rageAutoStopSpeed",g_cfg.rageAutoStopSpeed,"enemy",weaponClass),0.f,300.f);
        if(!local.velocityValid||std::hypot(local.velocity.x,local.velocity.y)>limit){
            g_frameDiagnostics.aimStatus="auto-stop";
        }
    }'''

assert target_gr_stop in text_gr, 'target_gr_stop in GameplayRuntime.h not found'
text_gr = text_gr.replace(target_gr_stop, repl_gr_stop, 1)

# Anti-Aim: always backward 180 + jitter around 180
target_gr_aa = '''    int mode=std::clamp(g_cfg.rageAntiAimMode,0,2);
    float yawOffset=std::clamp(RuntimeFloat("rageAntiAimYaw",g_cfg.rageAntiAimYaw,"enemy",""),-180.f,180.f);
    static std::mt19937 rng(std::random_device{}());
    static float spin{};
    if(mode==1){
        float low=std::clamp(RuntimeFloat("rageAntiAimJitterMin",g_cfg.rageAntiAimJitterMin,"enemy",""),-180.f,180.f);
        float high=std::clamp(RuntimeFloat("rageAntiAimJitterMax",g_cfg.rageAntiAimJitterMax,"enemy",""),-180.f,180.f);
        if(low>high)std::swap(low,high);
        // Sorteio por decisao, nao um valor fixo: a cada frame o offset e outro
        // dentro do range, e o motor o soma ao angulo vivo daquele comando.
        std::uniform_real_distribution<float> spread(low,high);
        yawOffset=spread(rng);
    }else if(mode==2){
        float step=std::clamp(RuntimeFloat("rageAntiAimSpinStep",g_cfg.rageAntiAimSpinStep,"enemy",""),1.f,180.f);
        spin=HazzureGame::NormalizeAngle(spin+step);
        yawOffset=spin;
    }else{
        spin=0.f;
    }'''

repl_gr_aa = '''    int mode=std::clamp(g_cfg.rageAntiAimMode,0,2);
    float yawOffset=180.f;
    static std::mt19937 rng(std::random_device{}());
    static float spin{};
    if(mode==0){
        // Estatico: virado 180 de costas (backward) com offset
        yawOffset=HazzureGame::NormalizeAngle(180.f+g_cfg.rageAntiAimYaw);
    }else if(mode==1){
        // Jitter: virado 180 de costas e jitterando +- 35 graus
        float spreadRange=std::clamp(std::fabs(g_cfg.rageAntiAimJitterMax),15.f,60.f);
        std::uniform_real_distribution<float> spread(-spreadRange,spreadRange);
        yawOffset=HazzureGame::NormalizeAngle(180.f+spread(rng));
    }else if(mode==2){
        // Spin: rotacao continua 360
        float step=std::clamp(RuntimeFloat("rageAntiAimSpinStep",g_cfg.rageAntiAimSpinStep,"enemy",""),1.f,180.f);
        spin=HazzureGame::NormalizeAngle(spin+step);
        yawOffset=spin;
    }else{
        spin=0.f;
    }'''

assert target_gr_aa in text_gr, 'target_gr_aa in GameplayRuntime.h not found'
text_gr = text_gr.replace(target_gr_aa, repl_gr_aa, 1)

# Remove other-window block in anti-aim when menu is closed
target_gr_input = 'if(!client||!HazzureWorld::ready.load()||!IsGameInputActive(menuOpen)){ClearAntiAimCommand();return;}'
repl_gr_input = 'if(!client||!HazzureWorld::ready.load()||menuOpen){ClearAntiAimCommand();return;}'
assert target_gr_input in text_gr, 'target_gr_input in GameplayRuntime.h not found'
text_gr = text_gr.replace(target_gr_input, repl_gr_input, 1)

with open(path_gr, 'w', encoding='utf-8', newline='') as f:
    f.write(text_gr)
print('GameplayRuntime.h ok')


# =========================================================================
# 5. InternalStartup.inl - StampCommandStrafe into CUserCmd
# =========================================================================
path_is = r'C:\Codex2\gpt\HazzureInternal\src\InternalStartup.inl'
with open(path_is, 'r', encoding='utf-8') as f:
    text_is = f.read()

target_is_func = '''static void StampCommandJump(uintptr_t cmd,bool down) {'''
repl_is_func = '''static void StampCommandStrafe(uintptr_t cmd, int side) {
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

static void StampCommandJump(uintptr_t cmd,bool down) {'''

assert target_is_func in text_is, 'target_is_func in InternalStartup.inl not found'
text_is = text_is.replace(target_is_func, repl_is_func, 1)

target_is_call = '''        bool jumpDown = false;
        if (HazzureMove::ReadJumpCommand(jumpDown)) {
            StampCommandJump(reinterpret_cast<uintptr_t>(pCmd), jumpDown);
            InterlockedIncrement(&HazzureMove::jumpStampCount);
            InterlockedExchange(&HazzureMove::jumpStampDown, jumpDown ? 1 : 0);
        }'''

repl_is_call = '''        bool jumpDown = false;
        if (HazzureMove::ReadJumpCommand(jumpDown)) {
            StampCommandJump(reinterpret_cast<uintptr_t>(pCmd), jumpDown);
            InterlockedIncrement(&HazzureMove::jumpStampCount);
            InterlockedExchange(&HazzureMove::jumpStampDown, jumpDown ? 1 : 0);
        }
        int strafeSide = HazzureMovementTech::g_strafeCommandSide.load();
        if (strafeSide != 0) {
            StampCommandStrafe(reinterpret_cast<uintptr_t>(pCmd), strafeSide);
        }'''

assert target_is_call in text_is, 'target_is_call in InternalStartup.inl not found'
text_is = text_is.replace(target_is_call, repl_is_call, 1)

with open(path_is, 'w', encoding='utf-8', newline='') as f:
    f.write(text_is)
print('InternalStartup.inl ok')
