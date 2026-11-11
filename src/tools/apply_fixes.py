# c:\Codex2\gpt\tools\apply_fixes.py
import re

# 1. Update InternalStartup.inl
path_startup = r"c:\Codex2\gpt\HazzureInternal\src\InternalStartup.inl"
with open(path_startup, "r", encoding="utf-8") as f:
    startup_content = f.read()

# Fix SkyUpdate hook
old_sky_update = """        if (memcmp(reinterpret_cast<void*>(client + 0x282370), skyPrologue, sizeof(skyPrologue)) != 0) {

            LogDebug("[AVISO] memcmp do SkyUpdate divergiu! Forcando hook mesmo assim");

        }

        MH_STATUS sSky = MH_CreateHook(reinterpret_cast<void*>(client + 0x282370), SkyUpdateHook, reinterpret_cast<void**>(&originalSkyUpdate));

        LogDebug("[HOOK] MH_CreateHook(SkyUpdate) status: %d", sSky);

        HazzureState::RegisterHook("SkyUpdate", reinterpret_cast<void*>(client + 0x282370), SkyUpdateHook, originalSkyUpdate);"""

new_sky_update = """        if (memcmp(reinterpret_cast<void*>(client + 0x282370), skyPrologue, sizeof(skyPrologue)) != 0) {
            LogDebug("[AVISO] memcmp do SkyUpdate divergiu! Pulando hook para estabilidade do engine.");
        } else {
            MH_STATUS sSky = MH_CreateHook(reinterpret_cast<void*>(client + 0x282370), SkyUpdateHook, reinterpret_cast<void**>(&originalSkyUpdate));
            LogDebug("[HOOK] MH_CreateHook(SkyUpdate) status: %d", sSky);
            if (sSky == MH_OK) HazzureState::RegisterHook("SkyUpdate", reinterpret_cast<void*>(client + 0x282370), SkyUpdateHook, originalSkyUpdate);
        }"""

# Fix SkyDraw hook
old_sky_draw = """        if (memcmp(skyDraw, skyDrawExpected, sizeof(skyDrawExpected)) != 0) {

            LogDebug("[AVISO] memcmp do SkyDraw divergiu! Forcando hook mesmo assim");

        }

        MH_STATUS sSkyDraw = MH_CreateHook(skyDraw, SkyDrawHook, reinterpret_cast<void**>(&originalSkyDraw));

        LogDebug("[HOOK] MH_CreateHook(SkyDraw) status: %d", sSkyDraw);

        HazzureState::RegisterHook("SkyDraw", skyDraw, SkyDrawHook, originalSkyDraw);"""

new_sky_draw = """        if (memcmp(skyDraw, skyDrawExpected, sizeof(skyDrawExpected)) != 0) {
            LogDebug("[AVISO] memcmp do SkyDraw divergiu! Pulando hook para estabilidade do engine.");
        } else {
            MH_STATUS sSkyDraw = MH_CreateHook(skyDraw, SkyDrawHook, reinterpret_cast<void**>(&originalSkyDraw));
            LogDebug("[HOOK] MH_CreateHook(SkyDraw) status: %d", sSkyDraw);
            if (sSkyDraw == MH_OK) HazzureState::RegisterHook("SkyDraw", skyDraw, SkyDrawHook, originalSkyDraw);
        }"""

# Normalize CRLF / LF for replacement
def normalize_and_replace(text, old_sub, new_sub, label):
    old_norm = old_sub.replace("\r\n", "\n")
    text_norm = text.replace("\r\n", "\n")
    if old_norm in text_norm:
        text_norm = text_norm.replace(old_norm, new_sub.replace("\r\n", "\n"), 1)
        print(f"[OK] {label}")
        return text_norm.replace("\n", "\r\n")
    else:
        print(f"[FAIL] {label}")
        return text

startup_content = normalize_and_replace(startup_content, old_sky_update, new_sky_update, "InternalStartup.inl SkyUpdate")
startup_content = normalize_and_replace(startup_content, old_sky_draw, new_sky_draw, "InternalStartup.inl SkyDraw")

with open(path_startup, "w", encoding="utf-8") as f:
    f.write(startup_content)

# 2. Update Climate.h
path_climate = r"c:\Codex2\gpt\HazzureInternal\src\Climate.h"
with open(path_climate, "r", encoding="utf-8") as f:
    climate_content = f.read()

# ExposureFrame ready check
old_exposure = """static void ExposureFrame(){
 if(!shared)return;
 auto beat=InterlockedCompareExchange64(&shared->heartbeat,0,0);
 float amount=shared->worldBrightness;
 if(!(shared->climateFlags&2)||!HazzureWorldAppearance::HeartbeatAlive(beat,GetTickCount64())||!(amount>=.2f&&amount<=1.f)||
    !ClimateDataSchemaMatches()){RestoreExposure();return;}"""

new_exposure = """static void ExposureFrame(){
 if(!shared||!HazzureWorld::ready.load())return;
 auto beat=InterlockedCompareExchange64(&shared->heartbeat,0,0);
 float amount=shared->worldBrightness;
 if(!(shared->climateFlags&2)||!HazzureWorldAppearance::HeartbeatAlive(beat,GetTickCount64())||!(amount>=.2f&&amount<=1.f)||
    !ClimateDataSchemaMatches()){RestoreExposure();return;}"""

# PutSkyFields - do not overwrite m_hSkyMaterial to avoid material corruption crashes
old_put_sky = """static bool PutSkyFields(uintptr_t entity,uintptr_t material,uint32_t tint,float brightness){
 bool written=ClimatePut(entity+HazzureOffsets::Schema::m_hSkyMaterial,material);
 written&=ClimatePut(entity+HazzureOffsets::Schema::m_vTintColor,tint);
 written&=ClimatePut(entity+HazzureOffsets::Schema::m_flBrightnessScale,brightness);return written;
}"""

new_put_sky = """static bool PutSkyFields(uintptr_t entity,uintptr_t material,uint32_t tint,float brightness){
 // Do not mutate m_hSkyMaterial to prevent material corruption or unreferenced resource crash
 (void)material;
 bool written=ClimatePut(entity+HazzureOffsets::Schema::m_vTintColor,tint);
 written&=ClimatePut(entity+HazzureOffsets::Schema::m_flBrightnessScale,brightness);return written;
}"""

# RestoreClimate
old_restore_sky = """static bool RestoreClimate(){
 bool complete=true;
 for(auto& s:climateSkies)if(s.entity){
  if(Entity(s.handle)==s.entity&&SkyEntity(s.entity)){
   if(!PutSkyFields(s.entity,s.material,s.tint,s.brightness)||!UpdateSky(s.entity)){complete=false;continue;}
  }s={};
 }
 return complete;
}"""

new_restore_sky = """static bool RestoreClimate(){
 bool complete=true;
 for(auto& s:climateSkies)if(s.entity){
  if(Entity(s.handle)==s.entity&&SkyEntity(s.entity)){
   if(!PutSkyFields(s.entity,0,s.tint,s.brightness)){complete=false;continue;}
  }s={};
 }
 return complete;
}"""

# ClimateFrameWork
old_climate_loop = """   if(Get<uintptr_t>(e+HazzureOffsets::Schema::m_hSkyMaterial)!=material||Get<uint32_t>(e+HazzureOffsets::Schema::m_vTintColor)!=tint||Get<float>(e+HazzureOffsets::Schema::m_flBrightnessScale)!=brightness){
     if(!PutSkyFields(e,material,tint,brightness)||!UpdateSky(e))continue;
     InterlockedIncrement64(&shared->climateChanges);
    }active++;"""

new_climate_loop = """   if(Get<uint32_t>(e+HazzureOffsets::Schema::m_vTintColor)!=tint||Get<float>(e+HazzureOffsets::Schema::m_flBrightnessScale)!=brightness){
     if(!PutSkyFields(e,0,tint,brightness))continue;
     InterlockedIncrement64(&shared->climateChanges);
    }active++;"""

climate_content = normalize_and_replace(climate_content, old_exposure, new_exposure, "Climate.h ExposureFrame")
climate_content = normalize_and_replace(climate_content, old_put_sky, new_put_sky, "Climate.h PutSkyFields")
climate_content = normalize_and_replace(climate_content, old_restore_sky, new_restore_sky, "Climate.h RestoreClimate")
climate_content = normalize_and_replace(climate_content, old_climate_loop, new_climate_loop, "Climate.h ClimateFrameWork loop")

with open(path_climate, "w", encoding="utf-8") as f:
    f.write(climate_content)

# 3. Update cards/CardCombat.inl
path_combat = r"c:\Codex2\gpt\HazzureInternal\src\cards\CardCombat.inl"
with open(path_combat, "r", encoding="utf-8") as f:
    combat_content = f.read()

old_aimbot_card = """inline void DrawCombatAimbotCard() {
    BeginCleanCard("Aimbot", "BONE 3D");
    CleanToggle("Aimbot Ativo", &g_cfg.aimActive);
    CleanToggle("Somente Alvos Visiveis", &g_cfg.aimVisibleOnly);
    CleanSliderFloat("Campo de Visao (FOV)", &g_cfg.aimFov, 0.5f, 25.0f, "%.1f deg");
    CleanSliderFloat("Suavizacao (Smooth)", &g_cfg.aimSmooth, 1.0f, 25.0f, "%.1f");
    CleanToggle("Aceleracao / Desaceleracao", &g_cfg.aimEase);
    if (g_cfg.aimEase) CleanSliderFloat("Pico de Velocidade", &g_cfg.aimEaseBias, .15f, .65f, "%.2f");
    const char* bones[] = { "Cabeca (Bone 6)", "Pescoco (Bone 5)", "Peito (Bone 4)", "Pelve (Bone 0)" };
    CleanCombo("Osso Alvo", &g_cfg.aimBone, bones, IM_ARRAYSIZE(bones));
    const char* targetModes[] = { "Osso Fixo", "Osso Mais Proximo", "Cabeca / Fallback Visivel" };
    CleanCombo("Selecao de Alvo", &g_cfg.aimTargetMode, targetModes, IM_ARRAYSIZE(targetModes));
    ImGui::Dummy(ImVec2(0.f, 4.f));
    CleanKeybind("Tecla de Mira", &g_cfg.aimKey);
    EndCleanCard();
}"""

new_aimbot_card = """inline void DrawCombatAimbotCard() {
    BeginCleanCard("Aimbot", "BONE 3D");
    CleanToggle("Aimbot Ativo", &g_cfg.aimActive);
    CleanToggle("Somente Alvos Visiveis", &g_cfg.aimVisibleOnly);
    CleanToggle("Auto-Scope AWP / Scout", &g_cfg.autoScope);
    CleanSliderFloat("Campo de Visao (FOV)", &g_cfg.aimFov, 0.5f, 25.0f, "%.1f deg");
    CleanSliderFloat("Suavizacao (Smooth)", &g_cfg.aimSmooth, 1.0f, 25.0f, "%.1f");
    CleanToggle("Aceleracao / Desaceleracao", &g_cfg.aimEase);
    if (g_cfg.aimEase) CleanSliderFloat("Pico de Velocidade", &g_cfg.aimEaseBias, .15f, .65f, "%.2f");
    const char* bones[] = { "Cabeca (Bone 6)", "Pescoco (Bone 5)", "Peito (Bone 4)", "Pelve (Bone 0)" };
    CleanCombo("Osso Alvo", &g_cfg.aimBone, bones, IM_ARRAYSIZE(bones));
    const char* targetModes[] = { "Osso Fixo", "Osso Mais Proximo", "Cabeca / Fallback Visivel" };
    CleanCombo("Selecao de Alvo", &g_cfg.aimTargetMode, targetModes, IM_ARRAYSIZE(targetModes));
    ImGui::Dummy(ImVec2(0.f, 4.f));
    CleanKeybind("Tecla de Mira", &g_cfg.aimKey);
    if (g_cfg.aimKey == 0) ImGui::TextColored(kTextMuted, "Ativacao: Sempre ativo no FOV");
    else if (g_cfg.aimKey == 1) ImGui::TextColored(kTextMuted, "Ativacao: Mouse 1 (Disparo)");
    else if (g_cfg.aimKey == 5) ImGui::TextColored(kTextMuted, "Ativacao: Mouse 5 (Lateral)");
    else ImGui::TextColored(kTextMuted, "Ativacao: VK %d", g_cfg.aimKey);
    EndCleanCard();
}"""

old_trigger_card = """    CleanKeybind("Tecla Triggerbot", &g_cfg.triggerKey);"""
new_trigger_card = """    CleanKeybind("Tecla Triggerbot", &g_cfg.triggerKey);
    if (g_cfg.triggerKey == 0) ImGui::TextColored(kTextMuted, "Ativacao: Automatico (sem tecla)");
    else if (g_cfg.triggerKey == 18) ImGui::TextColored(kTextMuted, "Ativacao: ALT (segurar)");
    else ImGui::TextColored(kTextMuted, "Ativacao: VK %d", g_cfg.triggerKey);"""

combat_content = normalize_and_replace(combat_content, old_aimbot_card, new_aimbot_card, "CardCombat.inl DrawCombatAimbotCard")
combat_content = normalize_and_replace(combat_content, old_trigger_card, new_trigger_card, "CardCombat.inl DrawCombatTriggerCard")

with open(path_combat, "w", encoding="utf-8") as f:
    f.write(combat_content)

# 4. Update UIMovement.inl (Fixed 64Hz subtick rate)
path_uimove = r"c:\Codex2\gpt\HazzureInternal\src\UIMovement.inl"
with open(path_uimove, "r", encoding="utf-8") as f:
    uimove_content = f.read()

old_uimove_dt = "technique.time=now;technique.dt=std::clamp(ImGui::GetIO().DeltaTime,.001f,.05f);technique.mouse=movement;"
new_uimove_dt = "technique.time=now;technique.dt=1.f/64.f;technique.mouse=movement;"

uimove_content = normalize_and_replace(uimove_content, old_uimove_dt, new_uimove_dt, "UIMovement.inl fixed 64Hz dt")

with open(path_uimove, "w", encoding="utf-8") as f:
    f.write(uimove_content)

# 5. Update AdvancedMovement.h (EdgeBug hull extent)
path_advmove = r"c:\Codex2\gpt\HazzureInternal\src\AdvancedMovement.h"
with open(path_advmove, "r", encoding="utf-8") as f:
    advmove_content = f.read()

old_edge_sweep = "if(HazzurePhysics::Sweep(client,local.address,foot,down,{-1,-1,0},{1,1,1},HazzureMove::playerSolidMask,support))p.edge=support.fraction==1&&!support.solid;"
new_edge_sweep = "if(HazzurePhysics::Sweep(client,local.address,foot,down,minimum,maximum,HazzureMove::playerSolidMask,support))p.edge=support.fraction==1&&!support.solid;"

advmove_content = normalize_and_replace(advmove_content, old_edge_sweep, new_edge_sweep, "AdvancedMovement.h EdgeBug hull sweep")

with open(path_advmove, "w", encoding="utf-8") as f:
    f.write(advmove_content)

print("[COMPLETE] All patch scripts finished.")
