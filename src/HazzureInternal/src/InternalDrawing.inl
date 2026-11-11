static uint32_t MixColour(uint32_t top,uint32_t bottom,float t) {

    auto channel=[](uint32_t value,int shift) { return static_cast<int>((value>>shift)&0xFF); };

    float clamped=t<0.f?0.f:(t>1.f?1.f:t);

    int b=static_cast<int>(channel(bottom,0)+(channel(top,0)-channel(bottom,0))*clamped);

    int g=static_cast<int>(channel(bottom,8)+(channel(top,8)-channel(bottom,8))*clamped);

    int r=static_cast<int>(channel(bottom,16)+(channel(top,16)-channel(bottom,16))*clamped);

    int a=static_cast<int>(channel(bottom,24)+(channel(top,24)-channel(bottom,24))*clamped);

    return static_cast<uint32_t>(b|(g<<8)|(r<<16)|(a<<24));

}

// Height of the element inside its own bounding box, 0 at the feet. Measured from the

// scene object bounds when they are readable and finite; a missing or degenerate box

// falls back to the flat colour instead of painting with a guessed tint.

static float NormalisedHeight(unsigned char* current) {

    __try {

        auto scene=*reinterpret_cast<uintptr_t*>(current+0x18);

        if(!scene)return -1.f;

        auto model=*reinterpret_cast<uintptr_t*>(scene+0x1D0);

        if(!model)return -1.f;

        float low=*reinterpret_cast<float*>(model+0x24),high=*reinterpret_cast<float*>(model+0x34);

        if(!(high>low))return -1.f;

        float origin=*reinterpret_cast<float*>(current+0x54);

        float t=(origin-low)/(high-low);

        if(!(t>=-0.5f&&t<=1.5f))return -1.f;

        return t;

    }__except(EXCEPTION_EXECUTE_HANDLER) { return -1.f; }

}

static void Hook(void* a,void* b,void* mesh,int count,void* e,void* f,void* g,void* h) {
    if(!original)return;
    const auto renderOptions=HazzureRenderOptions::Read();

    InterlockedIncrement64(&shared->calls);
    if(HazzureState::g_unloading.load()||!HazzureWorld::ready.load(std::memory_order_acquire)){original(a,b,mesh,count,e,f,g,h);return;}

    if(nested||failed||!mesh||count<1||count>4096) { original(a,b,mesh,count,e,f,g,h);return; }

    nested=true;

    LONG flags=InterlockedCompareExchange(&shared->flags,0,0);



    if(((flags&7)!=0||renderOptions.weapon||renderOptions.dropped||ClimateAlive(1))&&preparing.load()==0) {

        unsigned expected=0;

        if(preparing.compare_exchange_strong(expected,1)) {

            bool ready=Prepare();preparing=ready?2:3;

            if(!ready)failed=true;

        }

    }

    // Inspect each batch element independently. The original receives the complete

    // batch in diagnostic mode and one element at a time only for actual overrides.

    if(((flags&7)==0&&!renderOptions.weapon&&!renderOptions.dropped)||preparing.load()!=2) {

        static ULONGLONG s_lastPassLog = 0;

        ULONGLONG nowPass = GetTickCount64();

        if (nowPass - s_lastPassLog >= 2000) {

            s_lastPassLog = nowPass;

            LogDebug("[HOOK PASS] Hook ativo | meshCount=%d | flags=%ld (Chams=%s) | preparing=%u",

                count, flags, (flags & 1) ? "ATIVO" : "DESATIVADO", preparing.load());

        }

        if((InterlockedCompareExchange64(&shared->calls,0,0)%500)==1) {

            auto scene=Get<uintptr_t>(reinterpret_cast<uintptr_t>(mesh)+0x18);

            auto handle=Get<uint32_t>(scene+0xC0);

            char name[96]{};auto ent=Entity(handle);

            if(ent&&ClassName(ent,name,sizeof(name)))Message(name);

        }

        original(a,b,mesh,count,e,f,g,h);nested=false;return;

    }



    int foundPawns = 0;

    int foundArms = 0;

    uintptr_t local = Get<uintptr_t>(client + HazzureOffsets::Globals::dwLocalPlayerPawn);

    int visStyle = shared ? InterlockedCompareExchange(&shared->chamsVisibleStyle, 0, 0) : 0;
    int hidStyle = shared ? InterlockedCompareExchange(&shared->chamsHiddenStyle, 0, 0) : 0;

    void* curVisMat = visibleMaterial;
    void* curHidMat = hiddenMaterial;
    if (visStyle == 1 && metallicVisibleMaterial) { curVisMat = metallicVisibleMaterial; }
    else if (visStyle == 2 && wireframeVisibleMaterial) { curVisMat = wireframeVisibleMaterial; }
    else if (visStyle == 3 && glowVisibleMaterial) { curVisMat = glowVisibleMaterial; }
    else if (visStyle == 4 && crystalVisibleMaterial) { curVisMat = crystalVisibleMaterial; }

    if (hidStyle == 1 && metallicHiddenMaterial) { curHidMat = metallicHiddenMaterial; }
    else if (hidStyle == 2 && wireframeHiddenMaterial) { curHidMat = wireframeHiddenMaterial; }
    else if (hidStyle == 3 && glowHiddenMaterial) { curHidMat = glowHiddenMaterial; }
    else if (hidStyle == 4 && crystalHiddenMaterial) { curHidMat = crystalHiddenMaterial; }



    // stage: 5 | reason: layer flags do IPC para history/onShot/local/team/glow
    LONG layerFlags = shared ? InterlockedCompareExchange(&shared->chamsLayerFlags, 0, 0) : 0;
    bool wantHistory = (layerFlags & 1) != 0, wantOnShot = (layerFlags & 2) != 0;
    bool wantLocal = (layerFlags & 4) != 0, wantTeam = (layerFlags & 8) != 0;
    bool wantGlow = (layerFlags & 16) != 0;
    float glowScale = shared ? shared->chamsGlowScale : 1.5f;

    struct TargetInfo {

        bool isPawn;

        bool isArms;

        bool isDroppedWeapon;

        bool isEnemyWeapon;

        bool isProp;

        bool isLocal;

        bool isTeam;

    };

    TargetInfo targets[128]{};

    int scanCount = (count <= 128) ? count : 128;

    int foundDropped = 0;

    int foundEnemyWeapon = 0;

    int foundProp = 0;



    for (int i = 0; i < scanCount; i++) {

        auto current = static_cast<unsigned char*>(mesh) + 0x70 * i;

        auto scene = Get<uintptr_t>(reinterpret_cast<uintptr_t>(current) + 0x18);

        auto handle = Get<uint32_t>(scene + 0xC0);

        auto entity = scene ? Entity(handle) : 0;

        char name[96]{}; bool valid = entity && ClassName(entity, name, sizeof(name));

        bool pawn = valid && (strcmp(name, ".?AVC_CSPlayerPawn@@") == 0 || strcmp(name, ".?AVC_CSPlayerPawnBase@@") == 0);

        bool arms = valid && (strcmp(name, ".?AVCBaseAnimGraph@@") == 0 || strcmp(name, ".?AVC_CS2HudModelArms@@") == 0);

        bool weaponClass = valid && (strstr(name, "Weapon") != nullptr);

        bool viewmodelClass = valid && (strstr(name, "ViewModel") != nullptr);

        bool weapon = false;

        bool dropped = false;

        bool enemyWeap = false;

        if (valid && (weaponClass || viewmodelClass)) {

            uint32_t owner = Get<uint32_t>(entity + HazzureOffsets::Schema::m_hOwnerEntity);

            if (renderOptions.weapon && (owner && owner != 0xffffffffu && Entity(owner) == local)) {

                weapon = true;

            } else if (renderOptions.enemyWeapon && owner && owner != 0xffffffffu && Entity(owner) != local && weaponClass) {

                enemyWeap = true;

            } else if (renderOptions.dropped && (owner == 0 || owner == 0xffffffffu) && weaponClass) {

                dropped = true;

            }

        }

        if (arms && Entity(Get<uint32_t>(entity + HazzureOffsets::Schema::m_hOwnerEntity)) != local) arms = false;

        // stage: 5 | reason: classificar local/team em vez de descartar
        bool isLocalEnt = pawn && (entity == local);
        bool sameTeam = pawn && !isLocalEnt && local && Get<unsigned char>(entity + HazzureOffsets::Schema::m_iTeamNum) == Get<unsigned char>(local + HazzureOffsets::Schema::m_iTeamNum);
        if (isLocalEnt) { pawn = false; if (wantLocal) { targets[i].isLocal = true; foundPawns++; } }
        else if (sameTeam && shared && shared->teamFilter) { pawn = false; if (wantTeam) { targets[i].isTeam = true; foundPawns++; } }
        else if (sameTeam) pawn = false;

        if (pawn && (flags & 1)) {

            targets[i].isPawn = true;

            foundPawns++;

        } else if ((arms && (flags & 4)) || weapon) {

            targets[i].isArms = true;

            foundArms++;

        } else if (dropped) {

            targets[i].isDroppedWeapon = true;

            foundDropped++;

        } else if (enemyWeap) {

            targets[i].isEnemyWeapon = true;

            foundEnemyWeapon++;

        } else if (renderOptions.prop && valid && (strstr(name, "Prop") != nullptr)) {

            targets[i].isProp = true;

            foundProp++;

        }

    }



    static ULONGLONG s_lastHookLog = 0;

    ULONGLONG nowHook = GetTickCount64();

    if (nowHook - s_lastHookLog >= 1000) {

        s_lastHookLog = nowHook;

        LogDebug("[HOOK ENTITY LOOP] batchCount=%d | pawnsEncontrados=%d | armsEncontrados=%d | dropped=%d | dwEntityList=0x%llX | localPawn=0x%llX",

            count, foundPawns, foundArms, foundDropped + foundEnemyWeapon,

            HazzureOffsets::Globals::dwEntityList,

            local);

    }



    // Se nenhuma entidade alvo foi encontrada no lote (cenario de mapas, props, sombras, etc.):

    // Repassa o lote inteiro de uma so vez para o original!

    // NUNCA fatiar lotes nao relacionados com chamadas unitarias count=1!

    if (foundPawns == 0 && foundArms == 0 && foundDropped == 0 && foundEnemyWeapon == 0 && foundProp == 0) {

        original(a, b, mesh, count, e, f, g, h);

        nested = false;

        return;

    }



    // Se o lote for unitario (count == 1, o caso padrao de renderizacao de jogadores e armas no CS2):

    if (count == 1) {

        auto current = static_cast<unsigned char*>(mesh);

        Backup saved{};

        if (targets[0].isPawn && Override(current, curVisMat, shared->visibleColour, &saved)) {

            if (flags & 2) {

                Backup ignored{}; Override(current, curHidMat, shared->hiddenColour, &ignored);

                original(a, b, current, 1, e, f, g, h);

                Override(current, curVisMat, shared->visibleColour, &ignored);

            }

            original(a, b, current, 1, e, f, g, h);

            if (renderOptions.dualLayer && wireframeVisibleMaterial) {
                Backup overlaySaved{};
                uint32_t overlayCol = (shared->hiddenColour & 0x00FFFFFFu) | 0xFF000000u;
                if (Override(current, wireframeVisibleMaterial, overlayCol, &overlaySaved)) {
                    original(a, b, current, 1, e, f, g, h);
                    Restore(current, &overlaySaved);
                }
            }

            if (InterlockedCompareExchange(&shared->gradient, 0, 0)) {

                float t = NormalisedHeight(current);

                if (t >= 0.f) {

                    Backup blended{};

                    uint32_t top = shared->visibleColour, bottom = static_cast<uint32_t>(shared->gradientColour);

                    float c = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);

                    float smooth = c * c * (3.f - 2.f * c);

                    uint32_t gradCol = (MixColour(top, bottom, smooth) & 0x00FFFFFFu) | 0xB0000000u;

                    void* gradMat = gradientMaterial ? gradientMaterial : hiddenMaterial;

                    if (Override(current, gradMat, gradCol, &blended)) {

                        original(a, b, current, 1, e, f, g, h);

                        Restore(current, &blended);

                    }

                }

            }

            // stage: 5 | reason: history layer — depth off, alpha 140
            if (wantHistory && curHidMat) {
                Backup hSaved{}; if (Override(current, curHidMat, shared->historyColour, &hSaved)) {
                    original(a, b, current, 1, e, f, g, h); Restore(current, &hSaved); }
            }
            // stage: 5 | reason: on_shot layer — flash na kill recente (500ms)
            if (wantOnShot) {
                static ULONGLONG lastKillTick = 0; static LONG lastKills = 0;
                LONG curKills = InterlockedCompareExchange(&shared->kills, 0, 0);
                if (curKills != lastKills) { lastKills = curKills; lastKillTick = GetTickCount64(); }
                if (GetTickCount64() - lastKillTick < 500) {
                    Backup osSaved{}; if (Override(current, curVisMat, shared->onShotColour, &osSaved)) {
                        original(a, b, current, 1, e, f, g, h); Restore(current, &osSaved); }
                }
            }
            // stage: 5 | reason: glow por mesh dilation
            if (wantGlow && glowVisibleMaterial) {
                Backup glSaved{}; uint32_t glowCol = (shared->visibleColour & 0x00FFFFFFu) | 0x80000000u;
                if (Override(current, glowVisibleMaterial, glowCol, &glSaved)) {
                    original(a, b, current, 1, e, f, g, h); Restore(current, &glSaved); }
            }

            Restore(current, &saved);

            InterlockedIncrement64(&shared->changed);

        // stage: 5 | reason: local player layer
        } else if (targets[0].isLocal && Override(current, curVisMat, shared->localColour, &saved)) {
            original(a, b, current, 1, e, f, g, h);
            Restore(current, &saved);
            InterlockedIncrement64(&shared->changed);

        // stage: 5 | reason: team layer
        } else if (targets[0].isTeam && Override(current, curVisMat, shared->teamColour, &saved)) {
            original(a, b, current, 1, e, f, g, h);
            Restore(current, &saved);
            InterlockedIncrement64(&shared->changed);

        } else if (targets[0].isArms && Override(current, armsMaterial, shared->armsColour, &saved)) {

            original(a, b, current, 1, e, f, g, h);

            if (renderOptions.dualLayer && wireframeVisibleMaterial) {
                Backup overlaySaved{};
                uint32_t overlayCol = (shared->armsColour & 0x00FFFFFFu) | 0xFF000000u;
                if (Override(current, wireframeVisibleMaterial, overlayCol, &overlaySaved)) {
                    original(a, b, current, 1, e, f, g, h);
                    Restore(current, &overlaySaved);
                }
            }

            Restore(current, &saved);

            InterlockedIncrement64(&shared->changed);

        } else if (targets[0].isDroppedWeapon && Override(current, glowVisibleMaterial ? glowVisibleMaterial : curVisMat, 0xDCFFFFFFu, &saved)) {

            original(a, b, current, 1, e, f, g, h);

            Restore(current, &saved);

            InterlockedIncrement64(&shared->changed);

        } else if (targets[0].isEnemyWeapon && Override(current, glowVisibleMaterial ? glowVisibleMaterial : curHidMat, shared->visibleColour, &saved)) {

            original(a, b, current, 1, e, f, g, h);

            Restore(current, &saved);

            InterlockedIncrement64(&shared->changed);

        } else if (targets[0].isProp && Override(current, propMaterial ? propMaterial : curVisMat, 0xB0C8C8C8u, &saved)) {

            original(a, b, current, 1, e, f, g, h);

            Restore(current, &saved);

            InterlockedIncrement64(&shared->changed);

        } else {

            original(a, b, current, 1, e, f, g, h);

        }

        nested = false;

        return;

    }



    // Para lotes multiplos contendo alvos (count > 1):

    // Realiza o Override in-place no array sem quebrar o lote ou desalinhar os descritores e/f/g/h

    struct BackupEntry {

        void* meshPtr;

        Backup backup;

        bool active;

    };

    BackupEntry batchBackups[128]{};



    for (int i = 0; i < scanCount; i++) {

        if (!targets[i].isPawn && !targets[i].isArms && !targets[i].isDroppedWeapon && !targets[i].isEnemyWeapon && !targets[i].isProp) continue;

        auto current = static_cast<unsigned char*>(mesh) + 0x70 * i;

        batchBackups[i].meshPtr = current;



        if (targets[i].isPawn) {

            batchBackups[i].active = Override(current, curVisMat, shared->visibleColour, &batchBackups[i].backup);

        } else if (targets[i].isArms) {

            batchBackups[i].active = Override(current, armsMaterial, shared->armsColour, &batchBackups[i].backup);

        } else if (targets[i].isDroppedWeapon) {

            batchBackups[i].active = Override(current, glowVisibleMaterial ? glowVisibleMaterial : curVisMat, 0xDCFFFFFFu, &batchBackups[i].backup);

        } else if (targets[i].isEnemyWeapon) {

            batchBackups[i].active = Override(current, glowVisibleMaterial ? glowVisibleMaterial : curHidMat, shared->visibleColour, &batchBackups[i].backup);

        } else if (targets[i].isProp) {

            batchBackups[i].active = Override(current, propMaterial ? propMaterial : curVisMat, 0xB0C8C8C8u, &batchBackups[i].backup);

        }

    }



    original(a, b, mesh, count, e, f, g, h);



    for (int i = 0; i < scanCount; i++) {

        if (batchBackups[i].active) {

            Restore(batchBackups[i].meshPtr, &batchBackups[i].backup);

            InterlockedIncrement64(&shared->changed);

        }

    }



    nested = false;

}



