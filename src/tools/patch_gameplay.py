import os

path = r'c:\Codex2\gpt\HazzureInternal\src\GameplayRuntime.h'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. ReadCurrentAimView
old_rcav = """static bool ReadCurrentAimView(uintptr_t client,Vec3& view){
    return HazzureInputView::ReadLive(client,view);
}"""

new_rcav = """static bool ReadCurrentAimView(uintptr_t client,Vec3& view){
    Vec3 va{};
    if(client && SafeRead(client + 0x25354C8, &va) && std::isfinite(va.x) && std::isfinite(va.y) && std::fabs(va.x) <= 89.5f){
        va.y = HazzureGame::NormalizeAngle(va.y);
        view = va;
        return true;
    }
    auto pawn = HazzureGame::LocalPawn(client);
    if(pawn && SafeRead(pawn + (HazzureSchema::g_mAngEyeAngles.offset ? HazzureSchema::g_mAngEyeAngles.offset : 0x3600), &va) &&
       std::isfinite(va.x) && std::isfinite(va.y) && std::fabs(va.x) <= 89.5f){
        va.y = HazzureGame::NormalizeAngle(va.y);
        view = va;
        return true;
    }
    return HazzureInputView::ReadLive(client,view);
}"""

assert old_rcav in text, 'old_rcav not found'
text = text.replace(old_rcav, new_rcav)

# 2. Quantize pitch inversion
old_quant = 'if(remainder.Quantize(-yawDelta/yawUnit,pitchDelta/pitchUnit,dx,dy)&&!SendRelativeMouse(dx,dy)){'
new_quant = 'if(remainder.Quantize(-yawDelta/yawUnit,-pitchDelta/pitchUnit,dx,dy)&&!SendRelativeMouse(dx,dy)){'
assert old_quant in text, 'old_quant not found'
text = text.replace(old_quant, new_quant)

# 3. Scope button gate
old_scope = 'if(g_scopeButtonClient||!IsGameInputActive(menuOpen)||!HazzureMove::Button(client+HazzureOffsets::Buttons::attack2,"attack2",65537))g_scopeGate.SendFailed();'
new_scope = 'if(g_scopeButtonClient)return;\n    if(!IsGameInputActive(menuOpen)||!HazzureMove::Button(client+HazzureOffsets::Buttons::attack2,"attack2",65537))g_scopeGate.SendFailed();'
assert old_scope in text, 'old_scope not found'
text = text.replace(old_scope, new_scope)

# 4. Hitchance visible fallback
old_vis = """        if(!visible.count)return;
        auto chance=HazzureAccuracy::Evaluate(local.eye,view,accuracy.inaccuracy,accuracy.spread,accuracy.range,
            visible.values.data(),visible.count,static_cast<unsigned>(std::clamp(g_cfg.hitchanceSamples,32,256)));"""
new_vis = """        const TargetCapsules& evalCapsules = visible.count ? visible : capsules;
        auto chance=HazzureAccuracy::Evaluate(local.eye,view,accuracy.inaccuracy,accuracy.spread,accuracy.range,
            evalCapsules.values.data(),evalCapsules.count,static_cast<unsigned>(std::clamp(g_cfg.hitchanceSamples,32,256)));"""
assert old_vis in text, 'old_vis not found'
text = text.replace(old_vis, new_vis)

# 5. Auto-scope sample ID and blocked check
old_as = 'if(HazzureVisibility::Submit({localPawn,pawn,local.eye,point,static_cast<unsigned>(300+bone)},combatNow)!=HazzureVisibility::Verdict::Visible)continue;'
new_as = 'if(HazzureVisibility::Submit({localPawn,pawn,local.eye,point,static_cast<unsigned>(bone)},combatNow)==HazzureVisibility::Verdict::Blocked)continue;'
assert old_as in text, 'old_as not found'
text = text.replace(old_as, new_as)

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)

print('GameplayRuntime.h patched successfully!')
