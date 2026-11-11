// ---------- HUDs Flutuantes Discretos & Dados de Jogo ----------
using PlantedC4Data=HazzureTactical::Bomb;
static HazzureTactical::BombLocator g_bombLocator;

static void DrawBombTimerHUD(const PlantedC4Data& c4,HazzureTactical::Damage damage,int health,int screenW) {
    if(screenW<320)return;
    auto* draw=ImGui::GetForegroundDrawList();
    const float cardW=std::min(300.f,static_cast<float>(screenW)-16.f);
    const float cardH=c4.beingDefused?90.f:74.f,posX=(screenW-cardW)*.5f,posY=24.f;
    const ImU32 timerCol=c4.remaining<5.f?IM_COL32(239,68,68,255):
        (c4.remaining<10.f?IM_COL32(245,158,11,255):IM_COL32(243,243,247,255));
    draw->AddRectFilled({posX,posY},{posX+cardW,posY+cardH},IM_COL32(18,18,22,240),6.f);
    draw->AddRect({posX,posY},{posX+cardW,posY+cardH},IM_COL32(40,40,48,255),6.f);
    draw->AddRectFilled({posX+4,posY+6},{posX+7,posY+cardH-6},timerCol,2.f);
    char title[32],time[32];sprintf_s(title,"BOMBA [SITE %s]",c4.site==1?"B":"A");
    sprintf_s(time,"%.1fs",c4.remaining);
    draw->AddText({posX+14,posY+8},IM_COL32(160,160,175,255),title);
    draw->AddText({posX+cardW-14-ImGui::CalcTextSize(time).x,posY+8},timerCol,time);
    const float barW=cardW-28,barY=posY+26;
    draw->AddRectFilled({posX+14,barY},{posX+14+barW,barY+3},IM_COL32(35,35,42,255),1.5f);
    draw->AddRectFilled({posX+14,barY},{posX+14+barW*std::clamp(c4.remaining/c4.timerLength,0.f,1.f),barY+3},timerCol,1.5f);
    if(c4.isDefused){
        draw->AddText({posX+14,posY+32},IM_COL32(16,185,129,255),"BOMBA DESARMADA");
    }else if(c4.beingDefused){
        // Equality is too close to call: leave a server tick of margin.
        const bool wins=HazzureTactical::DefuseHasMargin(c4.remaining,c4.defuseLeft);
        char label[80];sprintf_s(label,"DEFUSE %.1fs / %.0fs: %s",c4.defuseLeft,c4.defuseLength,wins?"A TEMPO":"SEM MARGEM");
        const auto color=wins?IM_COL32(16,185,129,255):IM_COL32(239,68,68,255);
        draw->AddText({posX+14,posY+32},color,label);
        draw->AddRectFilled({posX+14,posY+50},{posX+14+barW,posY+53},IM_COL32(35,35,42,255),1.f);
        draw->AddRectFilled({posX+14,posY+50},{posX+14+barW*(1.f-std::clamp(c4.defuseLeft/c4.defuseLength,0.f,1.f)),posY+53},color,1.f);
    }else{
        const char* hint=!HazzureTactical::DefuseHasMargin(c4.remaining,5.f)?"SEM MARGEM PARA DEFUSE":
            (!HazzureTactical::DefuseHasMargin(c4.remaining,10.f)?"DEFUSE COM KIT: 5s":"DEFUSE: 5s COM KIT / 10s SEM KIT");
        draw->AddText({posX+14,posY+32},IM_COL32(160,160,175,255),hint);
    }
    char risk[96];ImU32 riskColor=IM_COL32(132,132,143,255);
    if(c4.isDefused)sprintf_s(risk,"EXPLOSAO CANCELADA");
    else if(health<=0)sprintf_s(risk,"DANO: JOGADOR LOCAL SEM VIDA");
    else if(!damage.available)sprintf_s(risk,"DANO: LEITURA INDISPONIVEL");
    else if(damage.amount>=health){sprintf_s(risk,"DANO: -%u | LETAL",static_cast<unsigned>(damage.amount));riskColor=IM_COL32(239,68,68,255);}
    else{sprintf_s(risk,"DANO: -%u | SOBRA: %d HP",static_cast<unsigned>(damage.amount),health-damage.amount);riskColor=IM_COL32(16,185,129,255);}
    draw->AddText({posX+14,posY+(c4.beingDefused?64.f:50.f)},riskColor,risk);
}

struct BombPlantNotice { int site=-1; ULONGLONG when=0; };
static BombPlantNotice g_bombPlantNotice;
static uint32_t g_lastPlantedHandle=0;

static void DrawBombPlantedNotice(int screenW,int screenH) {
    if(screenW<320||screenH<120||g_bombPlantNotice.site<0)return;
    const ULONGLONG now=GetTickCount64();
    if(now<g_bombPlantNotice.when)return;
    const ULONGLONG duration=4200;
    const ULONGLONG elapsed=now-g_bombPlantNotice.when;
    if(elapsed>duration){g_bombPlantNotice.site=-1;return;}
    float alpha=1.f;
    if(elapsed>duration-900)alpha=std::clamp(static_cast<float>(duration-elapsed)/900.f,0.f,1.f);
    const int a=static_cast<int>(alpha*255.f);
    auto* draw=ImGui::GetForegroundDrawList();
    const char* label=g_bombPlantNotice.site==1?"BOMBA PLANTADA - SITE B":"BOMBA PLANTADA - SITE A";
    const ImU32 accent=g_bombPlantNotice.site==1?IM_COL32(239,68,68,a):IM_COL32(245,158,11,a);
    const ImVec2 sz=ImGui::CalcTextSize(label);
    const float w=sz.x+40.f,h=36.f,x=(screenW-w)*.5f,y=118.f;
    draw->AddRectFilled({x,y},{x+w,y+h},IM_COL32(18,18,22,static_cast<int>(a*235/255)),6.f);
    draw->AddRect({x,y},{x+w,y+h},IM_COL32(40,40,48,a),6.f);
    draw->AddCircleFilled({x+18,y+h*.5f},4,accent);
    draw->AddText({x+30,y+(h-ImGui::GetTextLineHeight())*.5f},IM_COL32(236,236,242,a),label);
}

static void RenderBombPlantedNotify(uintptr_t client,int screenW,int screenH) {
    if(!client||!g_cfg.bombPlantedNotify)return;
    HazzureTactical::Bomb c4{};
    if(g_bombLocator.Read(client,c4)){
        const bool tickingNow=c4.isTicking&&!c4.isDefused&&c4.handle!=0;
        if(tickingNow){
            if(c4.handle!=g_lastPlantedHandle){
                g_lastPlantedHandle=c4.handle;
                g_bombPlantNotice.site=c4.site;
                g_bombPlantNotice.when=GetTickCount64();
                LogDebug("[C4] plantada: site=%d handle=0x%X loc=%s cur=%.2f remaining=%.2f",
                    c4.site,c4.handle,g_bombLocator.lastLoc,g_bombLocator.lastCurrent,c4.remaining);
            }
        }else g_lastPlantedHandle=0;
    }else if(g_bombPlantNotice.site<0){
        g_lastPlantedHandle=0;
        return;
    }
    DrawBombPlantedNotice(screenW,screenH);
}

static void RenderBombTimerHUD(uintptr_t client,int screenW,int) {
    PlantedC4Data c4{};
    if(!g_cfg.showBombTimer){HazzureTactical::damageQueue.Reset();return;}
    if(!g_bombLocator.Read(client,c4)){
        HazzureTactical::damageQueue.Reset();
        static ULONGLONG lastFailDiag{};
        const auto now=GetTickCount64();
        if(now-lastFailDiag>=2000){lastFailDiag=now;
            LogDebug("[C4] timer sem bomba: loc=%s handleOk=%d simDelta=%.2f readBomb=%d cur=%.2f sim=%.2f dwPlantedC4=0x%llX",
                g_bombLocator.lastLoc,g_bombLocator.lastHandleOk?1:0,g_bombLocator.lastSimDelta,
                g_bombLocator.lastReadBomb?1:0,g_bombLocator.lastCurrent,g_bombLocator.lastSim,
                static_cast<unsigned long long>(g_bombLocator.lastGlobal));
        }
        return;
    }
    HazzureGame::Pawn local{};auto pawn=HazzureGame::LocalPawn(client);
    const bool alive=HazzureGame::ReadPawn(pawn,local);
    HazzureTactical::Damage damage{};HazzureTactical::DamageRequest request{};
    if(alive&&HazzureTactical::MakeDamageRequest(c4,pawn,request))damage=HazzureTactical::damageQueue.Submit(request,GetTickCount64());
    else HazzureTactical::damageQueue.Reset();
    static ULONGLONG lastDiagnostic{};const auto now=GetTickCount64();
    if(now-lastDiagnostic>=2000){lastDiagnostic=now;
        LogDebug("[C4] nativePrediction=%s damage=%u health=%d site=%d remaining=%.2f nameOk=%d loc=%s handleOk=%d simDelta=%.2f readBomb=%d cur=%.2f sim=%.2f bomb=0x%llX",
            damage.available?"ready":"pending-or-unavailable",static_cast<unsigned>(damage.amount),
            alive?local.health:0,c4.site,c4.remaining,HazzureTactical::DesignerName(c4.address,"planted_c4")?1:0,
            g_bombLocator.lastLoc,g_bombLocator.lastHandleOk?1:0,g_bombLocator.lastSimDelta,
            g_bombLocator.lastReadBomb?1:0,g_bombLocator.lastCurrent,g_bombLocator.lastSim,
            static_cast<unsigned long long>(c4.address));}
    DrawBombTimerHUD(c4,damage,alive?local.health:0,screenW);
}

static void DrawSpectatorsHUD(const HazzureTactical::Spectators& specs,int screenW,int screenH) {
    HazzureTactical::Spectators effective = specs;
    if(!effective.count && g_menuOpen && g_cfg.showSpectators) {
        effective.count = 1;
        strcpy_s(effective.entries[0].name, "Preview_Espectador");
        effective.entries[0].mode = 4;
    }
    if(!effective.count||screenW<160||screenH<80)return;
    const auto& currentSpecs = effective;
    auto* draw=ImGui::GetForegroundDrawList();
    const float cardW=std::min(224.f,static_cast<float>(screenW)-16.f),posX=screenW-cardW-8.f,posY=screenW<800?124.f:24.f;
    if(screenH<posY+54.f)return;
    const size_t capacity=static_cast<size_t>((screenH-posY-36.f)/18.f);
    const bool overflow=currentSpecs.count>capacity;
    const size_t rows=overflow?capacity-1:currentSpecs.count;
    const float cardH=28.f+static_cast<float>(rows+(overflow?1:0))*18.f;
    draw->AddRectFilled({posX,posY},{posX+cardW,posY+cardH},IM_COL32(18,18,22,235),6.f);
    draw->AddRect({posX,posY},{posX+cardW,posY+cardH},IM_COL32(40,40,48,255),6.f);
    char header[40];sprintf_s(header,"ESPECTADORES (%zu)",currentSpecs.count);
    draw->AddText({posX+12,posY+7},IM_COL32(239,68,68,255),header);
    float y=posY+26;
    for(size_t i=0;i<rows;++i){const auto& entry=currentSpecs.entries[i];const bool first=entry.mode==4;
        const auto color=first?IM_COL32(239,68,68,255):IM_COL32(59,130,246,255);
        draw->AddCircleFilled({posX+16,y+6},3,color);
        draw->PushClipRect({posX+26,y},{posX+cardW-46,y+18},true);
        draw->AddText({posX+26,y},IM_COL32(236,236,242,255),entry.name);draw->PopClipRect();
        const char* tag=first?"[1P]":"[3P]";
        draw->AddText({posX+cardW-12-ImGui::CalcTextSize(tag).x,y},color,tag);y+=18;
    }
    if(overflow){char label[40];sprintf_s(label,"+%zu espectadores",currentSpecs.count-rows);
        draw->AddText({posX+26,y},IM_COL32(160,160,175,255),label);}
}

static void RenderSpectatorsHUD(uintptr_t client,uintptr_t localPawn,int screenW,int screenH) {
    if(!g_cfg.showSpectators||!client)return;
    uintptr_t list{},controller{};
    if(!SafeRead(client+HazzureOffsets::Globals::dwEntityList,&list)||!list)return;
    SafeRead(client+HazzureOffsets::Globals::dwLocalPlayerController,&controller);
    DrawSpectatorsHUD(HazzureTactical::CollectSpectators(list,localPawn,controller),screenW,screenH);
}

static void RenderPersistentKillFeedHUD(int screenW, int /*screenH*/) {
    HazzureKillFeed::enabled.store(g_cfg.killfeedEnabled);
    if (!g_cfg.killfeedEnabled || screenW < 320) return;
    auto* draw = ImGui::GetForegroundDrawList();
    const auto now = GetTickCount64();

    HazzureKillFeed::Settings kfSettings{};
    kfSettings.enabled = g_cfg.killfeedEnabled;
    kfSettings.localOnly = g_cfg.killfeedLocalOnly;
    kfSettings.duration = g_cfg.killfeedDuration;
    kfSettings.lines = g_cfg.killfeedLines;
    HazzureKillFeed::store.Frame(kfSettings, now, HazzureCombat::roundEpoch.load(), HazzureWorld::epoch.load());

    float startY = 48.f;
    if (g_cfg.showSpectators) startY += 84.f;

    size_t drawCount = std::min(HazzureKillFeed::store.count, static_cast<size_t>(g_cfg.killfeedLines));
    for (size_t i = 0; i < drawCount; ++i) {
        const auto& e = HazzureKillFeed::store.entries[i];
        float ageSec = (now >= e.time) ? (now - e.time) * 0.001f : 0.f;
        float alpha = 1.0f;
        if (g_cfg.killfeedDuration > 0.f && ageSec > (g_cfg.killfeedDuration - 1.5f)) {
            alpha = std::clamp((g_cfg.killfeedDuration - ageSec) / 1.5f, 0.f, 1.f);
        }
        if (alpha <= 0.01f) continue;

        ImU32 bgCol = IM_COL32(18, 18, 22, static_cast<int>(225.f * alpha));
        ImU32 borderCol = e.local ? IM_COL32(245, 158, 11, static_cast<int>(255.f * alpha)) : IM_COL32(40, 40, 48, static_cast<int>(220.f * alpha));
        ImU32 textAtkCol = e.local ? IM_COL32(245, 158, 11, static_cast<int>(255.f * alpha)) : IM_COL32(236, 236, 242, static_cast<int>(255.f * alpha));

        char rowText[320];
        if (e.headshot) {
            sprintf_s(rowText, "%s [%s HS] %s", e.attackerName[0] ? e.attackerName : "Inimigo", e.weapon[0] ? e.weapon : "arma", e.victimName[0] ? e.victimName : "Vitima");
        } else {
            sprintf_s(rowText, "%s [%s] %s", e.attackerName[0] ? e.attackerName : "Inimigo", e.weapon[0] ? e.weapon : "arma", e.victimName[0] ? e.victimName : "Vitima");
        }
        ImVec2 txtSz = ImGui::CalcTextSize(rowText);
        float badgeW = txtSz.x + 18.f;
        float badgeH = 22.f;
        float bX = static_cast<float>(screenW) - badgeW - 16.f;
        float bY = startY;

        draw->AddRectFilled({bX, bY}, {bX + badgeW, bY + badgeH}, bgCol, 4.f);
        draw->AddRect({bX, bY}, {bX + badgeW, bY + badgeH}, borderCol, 4.f);
        draw->AddText({bX + 9.f, bY + 3.f}, textAtkCol, rowText);

        startY += badgeH + 4.f;
    }
}

// ==========================================================
// WIDGETS STUDIO MINIMALISTAS (BASEADOS EM HAZZUREPANEL)
// ==========================================================
static float g_cleanCardContentW = 240.f;

static void RenderEventLogsHUD(int screenW, int screenH) {
    if (!g_cfg.eventLogsScreen || screenW < 320 || screenH < 200) return;
    auto* draw = ImGui::GetForegroundDrawList();
    const auto now = GetTickCount64();
    float x = 16.f, y = 16.f;
    int drawn = 0;

    if (g_bombPlantNotice.site >= 0 && now >= g_bombPlantNotice.when && (now - g_bombPlantNotice.when) <= 5000) {
        float age = static_cast<float>(now - g_bombPlantNotice.when) * 0.001f;
        float alpha = std::clamp(1.f - (age - 3.5f) / 1.5f, 0.f, 1.f);
        char msg[96];
        sprintf_s(msg, "[EVENT] Bomba plantada no Site %s", g_bombPlantNotice.site == 1 ? "B" : "A");
        ImVec2 sz = ImGui::CalcTextSize(msg);
        draw->AddRectFilled({x, y}, {x + sz.x + 18.f, y + 22.f}, IM_COL32(18, 18, 22, static_cast<int>(220 * alpha)), 4.f);
        draw->AddRectFilled({x, y}, {x + 3.f, y + 22.f}, IM_COL32(239, 68, 68, static_cast<int>(255 * alpha)), 2.f);
        draw->AddText({x + 10.f, y + 3.f}, IM_COL32(242, 242, 248, static_cast<int>(255 * alpha)), msg);
        y += 26.f;
        ++drawn;
    }

    for (size_t i = HazzureCombat::effects.count; i > 0 && drawn < 6; --i) {
        const auto& e = HazzureCombat::effects.entries[i - 1];
        if (e.kind != HazzureCombat::Kind::Damage && e.kind != HazzureCombat::Kind::Kill) continue;
        float age = HazzureCombat::Age(now, e.time);
        if (age < 0.f || age > 4.0f) continue;
        float alpha = std::clamp(1.f - (age - 2.8f) / 1.2f, 0.f, 1.f);
        char msg[128];
        ImU32 accent = IM_COL32(56, 189, 248, static_cast<int>(255 * alpha));
        if (e.kind == HazzureCombat::Kind::Kill) {
            sprintf_s(msg, "[KILL] Alvo eliminado%s", e.lethal ? " (Headshot)" : "");
            accent = IM_COL32(245, 158, 11, static_cast<int>(255 * alpha));
        } else {
            sprintf_s(msg, "[HIT] Dano causado: -%d HP%s", e.damage, e.lethal ? " [HS]" : "");
        }
        ImVec2 sz = ImGui::CalcTextSize(msg);
        draw->AddRectFilled({x, y}, {x + sz.x + 18.f, y + 22.f}, IM_COL32(14, 15, 20, static_cast<int>(215 * alpha)), 4.f);
        draw->AddRectFilled({x, y}, {x + 3.f, y + 22.f}, accent, 2.f);
        draw->AddText({x + 10.f, y + 3.f}, IM_COL32(236, 236, 244, static_cast<int>(255 * alpha)), msg);
        y += 26.f;
        ++drawn;
    }
}

static void RenderScoreboardOverrideHUD(int screenW, int screenH) {
    if (!g_cfg.scoreboardRevealAll || !(GetAsyncKeyState(VK_TAB) & 0x8000) || screenW < 480 || screenH < 300) return;
    auto* draw = ImGui::GetForegroundDrawList();
    const float w = std::min(560.f, static_cast<float>(screenW) - 40.f);
    int activeRows = 0;
    for (const auto& p : HazzureCombat::history.players) {
        if (p.handle && p.health > 0 && p.name[0]) ++activeRows;
    }
    const float h = 44.f + std::max(1, activeRows) * 22.f;
    const float x = (screenW - w) * 0.5f;
    const float y = std::max(36.f, (screenH - h) * 0.22f);

    draw->AddRectFilled({x, y}, {x + w, y + h}, IM_COL32(12, 13, 18, 242), 6.f);
    draw->AddRect({x, y}, {x + w, y + h}, IM_COL32(122, 90, 248, 220), 6.f, 0, 1.2f);
    draw->AddText({x + 14.f, y + 10.f}, IM_COL32(240, 240, 250, 255), "HAZZURE TACTICAL SCOREBOARD (REVEAL)");
    draw->AddLine({x + 10.f, y + 32.f}, {x + w - 10.f, y + 32.f}, IM_COL32(42, 44, 56, 255), 1.f);

    float rowY = y + 38.f;
    if (activeRows == 0) {
        draw->AddText({x + 14.f, rowY}, IM_COL32(150, 152, 168, 255), "Nenhum jogador vivo rastreado no frame atual.");
        return;
    }
    for (const auto& p : HazzureCombat::history.players) {
        if (!p.handle || p.health <= 0 || !p.name[0]) continue;
        ImU32 teamCol = (p.team == 2) ? IM_COL32(245, 158, 11, 255) : IM_COL32(56, 189, 248, 255);
        char line[256];
        sprintf_s(line, "[%s] %-20.20s | %3d HP | %3d ARM | %s%s",
            p.team == 2 ? "TR" : "CT", p.name, p.health, p.armor,
            p.weapon[0] ? p.weapon : "Arma", (p.flagsMask & 2) ? " [KIT]" : "");
        draw->AddText({x + 14.f, rowY}, teamCol, line);
        rowY += 22.f;
    }
}

static void RenderCleanSniperScopeLines(uintptr_t client, uintptr_t localPawn, int screenW, int screenH) {
    if (!g_cfg.noScopeOverlay || !client || !localPawn || screenW <= 0 || screenH <= 0) return;

    // 1. Estado necessario: C_CSPlayerPawnBase::m_bIsScoped
    uint8_t scoped = 0;
    uintptr_t scpOff = HazzureSchema::g_mIsScoped.offset ? HazzureSchema::g_mIsScoped.offset : 0x1EA0u;
    if (!SafeRead(localPawn + scpOff, &scoped) || scoped != 1) return;

    // 2. Verificar arma ativa
    uintptr_t entityList = 0;
    if (!SafeRead(client + HazzureOffsets::Globals::dwEntityList, &entityList) || !entityList) return;
    uintptr_t activeWeapon = 0;
    uint16_t def = ActiveWeaponDefinition(localPawn, entityList, &activeWeapon);
    bool isSniper = (def == 9 || def == 40 || def == 38 || def == 11);
    bool isAugSg = (def == 8 || def == 39);
    if (!isSniper && !isAugSg) return;

    // Se for AUG/SG553 e nao estiver configurado para dot proprio, o desenho nativo permanece
    if (isAugSg && !g_cfg.scopeCustomDotAug) return;

    // 3. Resoluçao nativa sobre o swapchain / ImGui
    auto* draw = ImGui::GetForegroundDrawList();
    if (!draw) return;

    const ImVec2 baseCenter(static_cast<float>(screenW) * 0.5f, static_cast<float>(screenH) * 0.5f);
    ImVec2 finalCenter = baseCenter;

    // 4. Obter FOV atual para projeçao angular do punch
    float fovAtual = 40.0f;
    uintptr_t cameraServices = 0;
    if (SafeRead(localPawn + HazzureOffsets::Schema::m_pCameraServices, &cameraServices) && cameraServices) {
        uint32_t fovVal = 0;
        if (SafeRead(cameraServices + 0x298, &fovVal) && fovVal > 0 && fovVal <= 150) {
            fovAtual = static_cast<float>(fovVal);
        } else if (SafeRead(cameraServices + 0x178, &fovVal) && fovVal > 0 && fovVal <= 150) {
            fovAtual = static_cast<float>(fovVal);
        }
    }
    float deliveredFov = HazzureCamera::delivered.load();
    if (deliveredFov > 5.0f && deliveredFov <= 140.0f) {
        fovAtual = deliveredFov;
    }

    // 5. Compensaçao de punch / recuo
    if (g_cfg.scopePunchCompensation) {
        HazzureGame::Vec2 punch{};
        int32_t shots = 0;
        if (ReadLocalAimPunch(localPawn, &punch, &shots)) {
            if (std::isfinite(punch.x) && std::isfinite(punch.y)) {
                constexpr float kDegToRad = 0.017453292519943295f;
                float aspect = static_cast<float>(screenW) / static_cast<float>(screenH);
                float halfFovXRad = (fovAtual * 0.5f) * kDegToRad;
                float tanHalfFovX = std::tan(halfFovXRad);
                if (tanHalfFovX > 0.0001f) {
                    float tanHalfFovY = tanHalfFovX / aspect;
                    float radX = (punch.y * 2.0f) * kDegToRad;
                    float radY = (punch.x * 2.0f) * kDegToRad;
                    float dx = -(std::tan(radX) / tanHalfFovX) * (static_cast<float>(screenW) * 0.5f);
                    float dy = (std::tan(radY) / tanHalfFovY) * (static_cast<float>(screenH) * 0.5f);
                    if (std::isfinite(dx) && std::isfinite(dy)) {
                        finalCenter.x += dx;
                        finalCenter.y += dy;
                    }
                }
            }
        }
    }

    // 6. Vinheta radial opcional (Nivel C)
    if (g_cfg.scopeVignette && g_cfg.scopeVignetteAlpha > 0.001f) {
        float maxAlpha = std::clamp(g_cfg.scopeVignetteAlpha, 0.0f, 0.60f);
        float innerRadius = (static_cast<float>(screenH) * 0.40f) * (fovAtual / 60.0f);
        innerRadius = std::clamp(innerRadius, static_cast<float>(screenH) * 0.20f, static_cast<float>(screenH) * 0.65f);
        float outerRadius = innerRadius * 1.55f;

        const int rings = 6;
        const int segments = 32;
        float alphaStep = maxAlpha / static_cast<float>(rings);
        for (int r = 0; r < rings; ++r) {
            float r0 = innerRadius + (outerRadius - innerRadius) * (static_cast<float>(r) / rings);
            float r1 = innerRadius + (outerRadius - innerRadius) * (static_cast<float>(r + 1) / rings);
            int a = static_cast<int>((static_cast<float>(r) + 0.5f) * alphaStep * 255.0f);
            ImU32 col = IM_COL32(0, 0, 0, a);
            for (int s = 0; s < segments; ++s) {
                float th0 = (static_cast<float>(s) * 6.2831853f) / static_cast<float>(segments);
                float th1 = (static_cast<float>(s + 1) * 6.2831853f) / static_cast<float>(segments);
                ImVec2 p0(finalCenter.x + std::cos(th0) * r0, finalCenter.y + std::sin(th0) * r0);
                ImVec2 p1(finalCenter.x + std::cos(th1) * r0, finalCenter.y + std::sin(th1) * r0);
                ImVec2 p2(finalCenter.x + std::cos(th1) * r1, finalCenter.y + std::sin(th1) * r1);
                ImVec2 p3(finalCenter.x + std::cos(th0) * r1, finalCenter.y + std::sin(th0) * r1);
                draw->AddQuadFilled(p0, p1, p2, p3, col);
            }
        }
    }

    // 7. Reticula / Crosshair com contraste automatico
    if (isAugSg) {
        if (g_cfg.scopeCustomDotAug) {
            draw->AddCircleFilled(finalCenter, 2.4f, IM_COL32(0, 0, 0, 230));
            draw->AddCircleFilled(finalCenter, 1.5f, IM_COL32(40, 245, 60, 255));
        }
        return;
    }

    if (isSniper) {
        if (!g_cfg.cleanSniperScopeLines) return;
        const float cx = finalCenter.x, cy = finalCenter.y;
        const float gap = (g_cfg.scopeCrosshairGap > 0.0f) ? g_cfg.scopeCrosshairGap : 0.0f;
        const float len = g_cfg.scopeCrosshairLen;
        // Linha normal do CS2: preto solido 1.0px (RGBA: 0, 0, 0, 255) sem sombras artificiais
        const ImU32 lineCol = IM_COL32(0, 0, 0, 255);

        if (len <= 0.0f) {
            // Linha continua de ponta a ponta na tela (padrao 1:1 CS2)
            if (gap <= 0.001f) {
                draw->AddLine({0.f, cy}, {static_cast<float>(screenW), cy}, lineCol, 1.0f);
                draw->AddLine({cx, 0.f}, {cx, static_cast<float>(screenH)}, lineCol, 1.0f);
            } else {
                draw->AddLine({0.f, cy}, {cx - gap, cy}, lineCol, 1.0f);
                draw->AddLine({cx + gap, cy}, {static_cast<float>(screenW), cy}, lineCol, 1.0f);
                draw->AddLine({cx, 0.f}, {cx, cy - gap}, lineCol, 1.0f);
                draw->AddLine({cx, cy + gap}, {cx, static_cast<float>(screenH)}, lineCol, 1.0f);
            }
        } else {
            // Reticula com comprimento configuravel
            draw->AddLine({cx - len, cy}, {cx - gap, cy}, lineCol, 1.0f);
            draw->AddLine({cx + gap, cy}, {cx + len, cy}, lineCol, 1.0f);
            draw->AddLine({cx, cy - len}, {cx, cy - gap}, lineCol, 1.0f);
            draw->AddLine({cx, cy + gap}, {cx, cy + len}, lineCol, 1.0f);
        }

        if (g_cfg.scopeCrosshairDot) {
            draw->AddCircleFilled(finalCenter, 1.4f, IM_COL32(255, 65, 65, 240));
        }
    }
}

static void RenderForceSniperCrosshair(uintptr_t client, uintptr_t localPawn, int screenW, int screenH) {
    if (!g_cfg.forceSniperCrosshair || !client || !localPawn || screenW <= 0 || screenH <= 0) return;
    uint8_t scoped = 0;
    uintptr_t scpOff = HazzureSchema::g_mIsScoped.offset ? HazzureSchema::g_mIsScoped.offset : 0x1EA0u;
    if (SafeRead(localPawn + scpOff, &scoped) && scoped == 1) return;

    uintptr_t list = 0;
    if (!HazzureGame::Read(client + HazzureOffsets::Globals::dwEntityList, list) || !list) return;
    uintptr_t weapon = 0;
    uint16_t def = ActiveWeaponDefinition(localPawn, list, &weapon);
    if (def != 9 && def != 40 && def != 38 && def != 11) return;

    auto* draw = ImGui::GetForegroundDrawList();
    const float cx = screenW * 0.5f, cy = screenH * 0.5f;
    const float len = 9.f, gap = 3.f;
    const ImU32 col = IM_COL32(255, 255, 255, 230);
    const ImU32 shadow = IM_COL32(0, 0, 0, 180);
    draw->AddLine({cx - len, cy + 1.f}, {cx - gap, cy + 1.f}, shadow, 2.f);
    draw->AddLine({cx + gap, cy + 1.f}, {cx + len, cy + 1.f}, shadow, 2.f);
    draw->AddLine({cx + 1.f, cy - len}, {cx + 1.f, cy - gap}, shadow, 2.f);
    draw->AddLine({cx + 1.f, cy + gap}, {cx + 1.f, cy + len}, shadow, 2.f);
    draw->AddLine({cx - len, cy}, {cx - gap, cy}, col, 1.5f);
    draw->AddLine({cx + gap, cy}, {cx + len, cy}, col, 1.5f);
    draw->AddLine({cx, cy - len}, {cx, cy - gap}, col, 1.5f);
    draw->AddLine({cx, cy + gap}, {cx, cy + len}, col, 1.5f);
    draw->AddCircleFilled({cx, cy}, 1.2f, IM_COL32(235, 50, 50, 240));
}

static void RenderPenetrationCrosshair(uintptr_t client, uintptr_t localPawn, int screenW, int screenH) {
    if (!g_cfg.penetrationCrosshair || !client || !localPawn || screenW <= 0 || screenH <= 0) return;
    HazzureGame::Pawn local{};
    if (!HazzureGame::ReadPawn(localPawn, local) || !local.anglesValid) return;
    auto* draw = ImGui::GetForegroundDrawList();
    const float cx = screenW * 0.5f, cy = screenH * 0.5f;

    float pitchRad = local.angles.x * 0.01745329252f;
    float yawRad = local.angles.y * 0.01745329252f;
    Vec3 forward{ std::cos(pitchRad) * std::cos(yawRad), std::cos(pitchRad) * std::sin(yawRad), -std::sin(pitchRad) };
    Vec3 target{ local.eye.x + forward.x * 1200.f, local.eye.y + forward.y * 1200.f, local.eye.z + forward.z * 1200.f };

    auto pierce = HazzureVisibility::TracePierce(client, local.eye, target, localPawn, 1u, 2u);
    bool canPen = (pierce.verdict == HazzureVisibility::Verdict::Reached);
    bool isBlocked = (pierce.verdict == HazzureVisibility::Verdict::Blocked);
    ImU32 dotCol = canPen ? IM_COL32(74, 222, 128, 255) : (isBlocked ? IM_COL32(248, 113, 113, 220) : IM_COL32(200, 200, 200, 180));
    draw->AddCircle({cx, cy}, 3.5f, IM_COL32(0, 0, 0, 200), 12, 2.f);
    draw->AddCircleFilled({cx, cy}, 2.5f, dotCol);
}

// ============================================================================
// T069: Firing Crosshair no Swapchain com Compensacao de Punch (Item 11/12 Briefing)
// Desenha a mira na projecao real do disparo de Perfect Silent, sem cegar o jogador.
// ============================================================================
static void RenderFiringCrosshair(uintptr_t client, uintptr_t localPawn, int screenW, int screenH) {
    if (!g_cfg.rageActive || !g_cfg.rageSilentAim || !g_cfg.rageFiringCrosshair || !client || !localPawn || screenW <= 0 || screenH <= 0) return;

    HazzureGame::Vec3 firingAngle{};
    static HazzureGame::Vec3 s_persistAngle{};
    static bool s_hasEverFired = false;
    const bool isFiring = (g_hookAttackDown != 0) || ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    const bool hasFreshAngle = ReadFiringAngle(firingAngle, isFiring ? 400 : 160);
    if (hasFreshAngle) { s_persistAngle = firingAngle; s_hasEverFired = true; }
    else if (s_hasEverFired) { firingAngle = s_persistAngle; }
    else return;

    HazzureGame::Pawn local{};
    if (!HazzureGame::ReadPawn(localPawn, local) || !local.anglesValid) return;

    constexpr float kDegToRad = 0.017453292519943295f;
    float radP = firingAngle.x * kDegToRad;
    float radY = firingAngle.y * kDegToRad;
    float cp = std::cos(radP);
    HazzureGame::Vec3 forwardDir {
        cp * std::cos(radY),
        cp * std::sin(radY),
        -std::sin(radP)
    };
    HazzureGame::Vec3 worldTarget {
        local.eye.x + forwardDir.x * 2000.0f,
        local.eye.y + forwardDir.y * 2000.0f,
        local.eye.z + forwardDir.z * 2000.0f
    };

    Matrix4 matrix{};
    HazzureGame::Vec2 screenPoint{};
    bool projected = false;
    if (SafeRead(client + HazzureOffsets::Globals::dwViewMatrix, &matrix)) {
        projected = WorldToScreen(worldTarget, matrix, screenW, screenH, screenPoint);
    }

    if (!projected) {
        // Fallback trigonometrico com FOV e AimPunch
        float fovAtual = 90.0f;
        uintptr_t cameraServices = 0;
        if (SafeRead(localPawn + 0x11E0, &cameraServices) && cameraServices) {
            uint32_t fovVal = 0;
            if (SafeRead(cameraServices + 0x210, &fovVal) && fovVal > 0 && fovVal <= 150) {
                fovAtual = static_cast<float>(fovVal);
            } else if (SafeRead(cameraServices + 0x178, &fovVal) && fovVal > 0 && fovVal <= 150) {
                fovAtual = static_cast<float>(fovVal);
            }
        }
        float deliveredFov = HazzureCamera::delivered.load();
        if (deliveredFov > 5.0f && deliveredFov <= 140.0f) {
            fovAtual = deliveredFov;
        }

        float aspect = static_cast<float>(screenW) / static_cast<float>(screenH);
        float halfFovXRad = (fovAtual * 0.5f) * kDegToRad;
        float tanHalfFovX = std::tan(halfFovXRad);
        if (tanHalfFovX > 0.0001f) {
            float tanHalfFovY = tanHalfFovX / aspect;
            float dYaw = HazzureGame::NormalizeAngle(firingAngle.y - local.angles.y);
            float dPitch = firingAngle.x - local.angles.x;
            float radDeltaX = dYaw * kDegToRad;
            float radDeltaY = dPitch * kDegToRad;
            float dx = (std::tan(radDeltaX) / tanHalfFovX) * (static_cast<float>(screenW) * 0.5f);
            float dy = (std::tan(radDeltaY) / tanHalfFovY) * (static_cast<float>(screenH) * 0.5f);
            if (std::isfinite(dx) && std::isfinite(dy)) {
                screenPoint.x = (static_cast<float>(screenW) * 0.5f) + dx;
                screenPoint.y = (static_cast<float>(screenH) * 0.5f) + dy;
                projected = (screenPoint.x >= 0 && screenPoint.x <= screenW && screenPoint.y >= 0 && screenPoint.y <= screenH);
            }
        }
    }

    if (!projected) return;

    auto* draw = ImGui::GetForegroundDrawList();
    const float fx = screenPoint.x;
    const float fy = screenPoint.y;

    // Reticula Tatica:
    // Ponto central de impacto real (cyan vibrante com borda preta)
    draw->AddCircleFilled(ImVec2(fx, fy), 2.5f, IM_COL32(0, 0, 0, 220));
    draw->AddCircleFilled(ImVec2(fx, fy), 1.6f, IM_COL32(45, 235, 255, 255));

    // Braquetes de mira tatica (4 ticks com gap central e outline)
    // Cor do border indica estado do subtick: verde = probe confirmado, amarelo = probing
    constexpr float tickLen = 7.0f;
    constexpr float tickGap = 4.0f;
    const bool subTickReady = (InterlockedCompareExchange(&g_antiAimCmdOffset, -1, -1) >= 0);
    const ImU32 tickCol = subTickReady ? IM_COL32(45, 235, 130, 220) : IM_COL32(255, 220, 45, 220);
    const ImU32 shadowCol = IM_COL32(10, 12, 16, 200);

    // Linhas pretas de contraste (+1px)
    draw->AddLine(ImVec2(fx - tickGap - tickLen, fy), ImVec2(fx - tickGap, fy), shadowCol, 2.8f);
    draw->AddLine(ImVec2(fx + tickGap, fy), ImVec2(fx + tickGap + tickLen, fy), shadowCol, 2.8f);
    draw->AddLine(ImVec2(fx, fy - tickGap - tickLen), ImVec2(fx, fy - tickGap), shadowCol, 2.8f);
    draw->AddLine(ImVec2(fx, fy + tickGap), ImVec2(fx, fy + tickGap + tickLen), shadowCol, 2.8f);

    // Linhas ciano frontal
    draw->AddLine(ImVec2(fx - tickGap - tickLen, fy), ImVec2(fx - tickGap, fy), tickCol, 1.4f);
    draw->AddLine(ImVec2(fx + tickGap, fy), ImVec2(fx + tickGap + tickLen, fy), tickCol, 1.4f);
    draw->AddLine(ImVec2(fx, fy - tickGap - tickLen), ImVec2(fx, fy - tickGap), tickCol, 1.4f);
    draw->AddLine(ImVec2(fx, fy + tickGap), ImVec2(fx, fy + tickGap + tickLen), tickCol, 1.4f);
}

