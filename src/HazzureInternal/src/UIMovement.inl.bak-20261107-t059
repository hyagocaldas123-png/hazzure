static std::vector<float> g_speedHistory;



static void RenderSpeedometer(ImDrawList* draw, uintptr_t localPawn, int screenW, int screenH) {

    if (!g_cfg.showSpeedometer || !localPawn) return;



    Vec3 vel{};

    SafeRead(localPawn + HazzureOffsets::Schema::m_vecVelocity, &vel);

    float speed = std::hypot(vel.x, vel.y);

    if (!std::isfinite(speed) || speed > 20000.f) return;



    if (g_speedHistory.size() < 60) g_speedHistory.resize(60, 0.f);

    g_speedHistory.erase(g_speedHistory.begin());

    g_speedHistory.push_back(speed);



    float boxW = 148.f;

    float boxH = g_cfg.showMovementGraph ? 46.f : 28.f;

    float posX = screenW * 0.5f - boxW * 0.5f;

    float posY = screenH - 96.f;



    draw->AddRectFilled(ImVec2(posX, posY), ImVec2(posX + boxW, posY + boxH), IM_COL32(12, 12, 15, 220), 4.f);

    draw->AddRect(ImVec2(posX, posY), ImVec2(posX + boxW, posY + boxH), IM_COL32(36, 36, 44, 240), 4.f, 0, 1.f);



    char buf[32];

    sprintf_s(buf, "VEL  %d u/s", (int)speed);

    ImVec2 tSz = ImGui::CalcTextSize(buf);

    draw->AddText(ImVec2(posX + (boxW - tSz.x) * 0.5f, posY + 6.f), IM_COL32(240, 240, 246, 240), buf);



    if (g_cfg.showMovementGraph) {

        for (int i = 1; i < 60; ++i) {

            float s1 = g_speedHistory[i - 1];

            float s2 = g_speedHistory[i];

            float h1 = std::clamp(s1 / 380.f, 0.f, 1.f) * 16.f;

            float h2 = std::clamp(s2 / 380.f, 0.f, 1.f) * 16.f;



            float x1 = posX + 12.f + (i - 1) * 2.1f;

            float y1 = (posY + boxH - 5.f) - h1;

            float x2 = posX + 12.f + i * 2.1f;

            float y2 = (posY + boxH - 5.f) - h2;



            ImU32 lineCol = (s2 >= s1) ? IM_COL32(220, 220, 230, 210) : IM_COL32(120, 120, 132, 190);

            draw->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), lineCol, 1.2f);

        }

    }

}



static uintptr_t g_legacyStrafeClient;

static void ReleaseAutoStrafeKeys(int& activeStrafeDir) {

    if(activeStrafeDir&&g_legacyStrafeClient){

        bool left=activeStrafeDir<0;int key=left?'A':'D';

        if(!HazzureMove::Button(g_legacyStrafeClient+(left?HazzureOffsets::Buttons::left:HazzureOffsets::Buttons::right),

            left?"left":"right",(GetAsyncKeyState(key)&0x8000)?65537:256))return;

    }

    activeStrafeDir=0;g_legacyStrafeClient=0;

}



static bool WriteJumpButton(uintptr_t client,int state) {

    if(state<0||!client)return false;

#ifdef INTERNAL_SELF_TEST

    return true;

#else

    // Button globals are dead on 0x6AEBC4DA; the CreateMove hook stamps IN_JUMP
    // on the command, including clearing a physical space hold while owned.
    if(!HazzureVisibility::BuildMatches(client))return false;

    HazzureMove::PublishJumpCommand(state==65537);return true;

#endif

}

static bool LineupThrowReady(uintptr_t client,uintptr_t localPawn){

    if(!g_cfg.lineupHelper)return true;

    HazzureGame::Pawn local{};uintptr_t list{},services{};uint32_t handle{};uint16_t definition{};int flags{};

    if(!HazzureWorld::ready.load()||!HazzureGame::ReadPawn(localPawn,local)||!local.anglesValid||!local.velocityValid||

       std::hypot(local.velocity.x,local.velocity.y)>5||!SafeRead(localPawn+HazzureOffsets::Schema::m_fFlags,&flags)||!(flags&1)||

       !SafeRead(client+HazzureOffsets::Globals::dwEntityList,&list)||!SafeRead(localPawn+HazzureOffsets::Schema::m_pWeaponServices,&services)||!services||

       !SafeRead(services+HazzureOffsets::Schema::m_hActiveWeapon,&handle))return false;

    auto weapon=HazzureGame::Entity(list,handle);uint8_t pin{};float strength{};

    if(!weapon||!SafeRead(weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex,&definition)||

       !HazzureVisibility::BuildMatches(client)||!SafeRead(weapon+0x1f23,&pin)||pin!=1||!SafeRead(weapon+0x1f30,&strength)||strength<.99f)return false;

    auto selected=HazzureLineups::Nearest(g_cfg.lineups,definition,local.origin,3);

    return selected>=0&&HazzureLineups::Ready(g_cfg.lineups.entries[selected],g_cfg.lineups.map,definition,local.origin,local.angles);

}

#include "UILineupAutomation.inl"

static void ProcessMovementMechanics(uintptr_t client, uintptr_t localPawn, bool menuOpen) {

    static HazzureMove::JumpPulse jump;
    static HazzurePistol::Gate pistolGate;

    static uintptr_t jumpClient{},jumpOwnerClient{},autoPistolClient{};

    static int activeStrafeDir{};

    static bool hasPrevYaw{},previousThrowKey{},legacyThrowPending{};

    static float previousYaw{};

    static uint64_t legacyThrowTime{};

    static uintptr_t legacyThrowPawn{},legacyThrowWeapon{};

    auto now=GetTickCount64();

    HazzureGame::Pawn local{};uint8_t moveType{};int32_t flags{};

    bool context=client&&HazzureWorld::ready.load()&&IsGameInputActive(menuOpen)&&

        HazzureVisibility::BuildMatches(client)&&HazzureGame::ReadPawn(localPawn,local)&&

        local.health>0&&local.life==0&&SafeRead(localPawn+HazzureMove::moveTypeOffset,&moveType)&&

        SafeRead(localPawn+HazzureOffsets::Schema::m_fFlags,&flags);

    bool walking=context&&moveType==2,onGround=(flags&1)!=0;

    bool spaceHeld=(GetAsyncKeyState(VK_SPACE)&0x8000)!=0;

    bool physicalAttack=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;

    if(client)jumpClient=client;

    HazzureMove::Settings movement;

    movement.active=context;movement.bhop=g_cfg.bhopActive;movement.edge=g_cfg.edgeJump||

        (g_cfg.shotStop&&g_cfg.shotStopEdgeGuard);movement.ladder=g_cfg.fastLadder;movement.published=now;

    for(const char* group:{"rifles","pistols","snipers"})movement.edge=movement.edge||

        (RuntimeBool("shotStop",g_cfg.shotStop,"local",group)&&RuntimeBool("shotStopEdgeGuard",g_cfg.shotStopEdgeGuard,"local",group));

    float sensitivity{};uintptr_t sensitivityObject{};
    bool sensitivityValid=client&&SafeRead(client+HazzureOffsets::Globals::dwSensitivity,&sensitivityObject)&&sensitivityObject&&
        SafeRead(sensitivityObject+HazzureOffsets::Schema::dwSensitivity_sensitivity,&sensitivity)&&
        std::isfinite(sensitivity)&&sensitivity>=.01f&&sensitivity<=100.f;
    movement.yawUnit=sensitivityValid?sensitivity*std::clamp(g_cfg.mouseYawScale,.001f,.1f):0.f;
    movement.pitchUnit=sensitivityValid?sensitivity*std::clamp(g_cfg.mousePitchScale,.001f,.1f):0.f;
    movement.ladder=movement.ladder&&sensitivityValid;
    movement.autoPistol=g_cfg.autoPistol;
    // "mira livre" (default) suprime o steer de ângulo do auto-strafe no ar; o pulo do bhop segue intacto.
    // bhopBoost liga o steer fisico independente do bhopViewLock: e ele que ganha speed no ar.
    movement.viewLockJump=g_cfg.autoStrafePhysics||g_cfg.bhopViewLock||g_cfg.bhopBoost;

    // Cancel previous direction owners before a lineup can acquire W or steer.

    bool throwKey=g_cfg.jumpthrowKey>0&&(GetAsyncKeyState(g_cfg.jumpthrowKey)&0x8000)!=0;

    bool lineupStarting=g_cfg.lineupHelper&&g_cfg.lineupAutoAlign&&g_cfg.jumpthrowActive&&throwKey;

    bool lineupAcquiring=HazzureMove::lineupBusy.load()||lineupStarting;

    if(lineupAcquiring){

        HazzureMovementTech::Stop();HazzureMove::ReleaseLadder(jumpClient,(GetAsyncKeyState('D')&0x8000)!=0);

        ReleaseAutoStrafeKeys(activeStrafeDir);hasPrevYaw=false;

    }

    bool inputOwnersReady=activeStrafeDir==0&&!HazzureMovementTech::directionOwned&&!HazzureMove::rightOwned;

    bool lineupJump=ProcessLineupAutomation(client,localPawn,walking&&sensitivityValid&&(!lineupAcquiring||inputOwnersReady),movement);

    bool lineupBusy=HazzureMove::lineupBusy.load();

    movement.ladder=movement.ladder&&!lineupBusy;

    HazzureMove::Configure(movement);

    HazzureMovementTech::Settings technique;

    technique.active=walking&&!lineupBusy&&(g_cfg.autoStrafePhysics||g_cfg.bhopBoost||g_cfg.jumpBug||g_cfg.edgeBug||g_cfg.pixelSurf);

    technique.strafe=(g_cfg.autoStrafePhysics||g_cfg.bhopBoost||g_cfg.autoStrafe)&&sensitivityValid;technique.jumpbug=g_cfg.jumpBug;technique.edgebug=g_cfg.edgeBug;technique.surf=g_cfg.pixelSurf&&sensitivityValid;

    technique.time=now;technique.dt=1.f/64.f;technique.mouse=movement;

    // Intencao de pular nao e so a tecla fisica: quando o auto-jump e nosso (jump.owned) o
    // steer aereo tambem vale, senao o strafe so tinha chance nos frames com a tecla pressionada.
    technique.space=spaceHeld||jump.owned;

    technique.jumpbugHeld=g_cfg.jumpBug&&(GetAsyncKeyState(g_cfg.jumpBugKey)&0x8000)!=0;

    technique.edgebugHeld=g_cfg.edgeBug&&(GetAsyncKeyState(g_cfg.edgeBugKey)&0x8000)!=0;

    technique.surfHeld=g_cfg.pixelSurf&&(GetAsyncKeyState(g_cfg.pixelSurfKey)&0x8000)!=0;

    HazzureMovementTech::Configure(technique);

    bool techniqueJump=HazzureMovementTech::jumpRequest.exchange(false)&&technique.active;

    if(!technique.active)HazzureMovementTech::Stop();

    uintptr_t list{},services{},weapon{};uint32_t handle{};uint16_t definition{};

    if(context&&SafeRead(client+HazzureOffsets::Globals::dwEntityList,&list)&&

       SafeRead(localPawn+HazzureOffsets::Schema::m_pWeaponServices,&services)&&services&&

       SafeRead(services+HazzureOffsets::Schema::m_hActiveWeapon,&handle))weapon=HazzureGame::Entity(list,handle);

    if(weapon)SafeRead(weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex,&definition);

    bool legacyPressed=throwKey&&!previousThrowKey;previousThrowKey=throwKey;

    bool legacyJump=false;

    if(!walking||g_cfg.lineupAutoAlign||!g_cfg.jumpthrowActive||localPawn!=legacyThrowPawn||weapon!=legacyThrowWeapon||

       now<legacyThrowTime||now-legacyThrowTime>250)legacyThrowPending=false;

    if(walking&&!lineupBusy&&g_cfg.jumpthrowActive&&!g_cfg.lineupAutoAlign&&legacyPressed&&onGround&&

       definition>=43&&definition<=48&&LineupThrowReady(client,localPawn)){

        uint8_t pin{};if(weapon&&SafeRead(weapon+0x1f23,&pin)&&pin==1){

            legacyThrowPending=true;legacyJump=true;legacyThrowTime=now;legacyThrowPawn=localPawn;legacyThrowWeapon=weapon;

        }

    }

    if(legacyThrowPending&&!onGround){

        bool released=HazzureMove::Button(client+HazzureOffsets::Buttons::attack,"attack",256);

        released=HazzureMove::Button(client+HazzureOffsets::Buttons::attack2,"attack2",256)&&released;

        if(released)legacyThrowPending=false;

    }

    HazzureMove::lineupBusy.store(lineupBusy||legacyThrowPending);

    float speed=local.velocityValid?std::hypot(local.velocity.x,local.velocity.y):0;

    auto support=HazzureMove::Support::Unavailable;

    if(walking&&!lineupBusy&&g_cfg.edgeJump&&onGround&&speed>40&&local.velocityValid&&local.velocity.z<=1.f)

        support=HazzureMove::ReadSupport(localPawn,local.scene,local.origin,local.velocity,now);

    bool edgeJump=HazzureMove::edge.Update(localPawn,walking&&!lineupBusy&&g_cfg.edgeJump,onGround,walking,speed,support);

    bool bhop=walking&&!lineupBusy&&g_cfg.bhopActive&&spaceHeld;

    bool contextChanged=jumpOwnerClient&&jumpClient!=jumpOwnerClient;

    if(contextChanged&&jump.owned){WriteJumpButton(jumpOwnerClient,256);jump.Reset();}

    uint64_t penalty=HazzureMove::JumpSpamPenaltyMs(client);

    int jumpState=jump.Step({localPawn,context,walking,onGround,bhop,lineupJump||legacyJump||techniqueJump||edgeJump,spaceHeld,now,penalty});

    if(jumpState>=0&&jumpClient){

        if(WriteJumpButton(jumpClient,jumpState))jumpOwnerClient=jump.owned?jumpClient:0;

        else {jump.WriteFailed();jumpOwnerClient=jumpClient;}

    }

    if(!walking){ReleaseAutoStrafeKeys(activeStrafeDir);hasPrevYaw=false;HazzureMove::edge={};}

    if(walking&&!lineupBusy&&!legacyThrowPending&&g_cfg.autoStrafe&&!g_cfg.autoStrafePhysics&&!g_cfg.pixelSurf&&spaceHeld&&!onGround&&speed>25){

        Vec3 view{};float delta=0;

        if(HazzureInputView::ReadLive(client,view)){

            if(hasPrevYaw)delta=HazzureGame::NormalizeAngle(view.y-previousYaw);

            previousYaw=view.y;hasPrevYaw=true;

        }

        int desired=delta>.35f?-1:delta<-.35f?1:activeStrafeDir;

        if(desired!=activeStrafeDir){ReleaseAutoStrafeKeys(activeStrafeDir);if(activeStrafeDir)desired=0;}

        if(desired){

            bool left=desired<0;

            if(HazzureMove::Button(client+(left?HazzureOffsets::Buttons::left:HazzureOffsets::Buttons::right),left?"left":"right",65537)){

                activeStrafeDir=desired;g_legacyStrafeClient=client;

            }

        }

    }else{ReleaseAutoStrafeKeys(activeStrafeDir);hasPrevYaw=false;}

    auto cooldown=HazzurePistol::Read(localPawn,weapon,handle,now);
    bool autoPistol=context&&!HazzureMove::lineupBusy.load()&&!HazzurePistol::triggerOwned.load()&&g_cfg.autoPistol&&physicalAttack&&
        HazzurePistol::SemiAutomatic(definition);
    int attackState=pistolGate.Step(cooldown,autoPistol,context&&physicalAttack&&!HazzureMove::lineupBusy.load(),now);
    if(attackState>=0){
        uintptr_t destination=autoPistolClient?autoPistolClient:client;
        if(destination&&HazzureMove::Button(destination+HazzureOffsets::Buttons::attack,"attack",attackState))
            autoPistolClient=pistolGate.owned?client:0;
        else {pistolGate.Failed();autoPistolClient=destination;}
    }

    uint64_t cmdSetBits=0,cmdClearBits=0;
    if(context&&walking&&!lineupBusy){
        constexpr uint64_t kBtnAttack=1ull<<0,kBtnJump=1ull<<1,kBtnDuck=1ull<<2,kBtnForward=1ull<<3,
            kBtnBack=1ull<<4,kBtnMoveLeft=1ull<<9,kBtnMoveRight=1ull<<10,kBtnAttack2=1ull<<11,kBtnSpeed=1ull<<16;
        bool wHeld=(GetAsyncKeyState('W')&0x8000)!=0,sHeld=(GetAsyncKeyState('S')&0x8000)!=0;
        bool aHeld=(GetAsyncKeyState('A')&0x8000)!=0,dHeld=(GetAsyncKeyState('D')&0x8000)!=0;
        bool ctrlHeld=(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;
        bool anyMoveHeld=wHeld||sHeld||aHeld||dHeld;
        static uint32_t s_moveTick=0;++s_moveTick;

        // 1. Fake Duck: oscila IN_DUCK em ciclo de 8 comandos quando ativo e no chao
        bool fakeDuckOn=RuntimeBool("fakeDuck",g_cfg.fakeDuck,"local","")&&
            (g_cfg.fakeDuckKey==0||(GetAsyncKeyState(g_cfg.fakeDuckKey)&0x8000)!=0);
        if(fakeDuckOn&&onGround){
            if((s_moveTick&7u)<4u)cmdSetBits|=kBtnDuck;
            else cmdClearBits|=kBtnDuck;
        }

        // 2. Crouch&Jump / Stand&Jump auto
        if(RuntimeBool("crouchJumpAuto",g_cfg.crouchJumpAuto,"local","")&&spaceHeld){
            if(onGround||local.velocity.z>5.f)cmdSetBits|=(kBtnDuck|kBtnJump);
        }
        if(RuntimeBool("standJumpAuto",g_cfg.standJumpAuto,"local","")&&spaceHeld&&ctrlHeld&&onGround){
            cmdClearBits|=kBtnDuck;cmdSetBits|=kBtnJump;
        }

        // 3. Slowwalk: mantem IN_SPEED e modula WASD se passar do limite silencioso (~130 u/s)
        bool slowWalkOn=RuntimeBool("slowWalk",g_cfg.slowWalk,"local","")&&
            (g_cfg.slowWalkKey==0||(GetAsyncKeyState(g_cfg.slowWalkKey)&0x8000)!=0);
        if(slowWalkOn&&onGround&&anyMoveHeld){
            float swLimit=std::clamp(RuntimeFloat("slowWalkSpeed",g_cfg.slowWalkSpeed,"local",""),30.f,160.f);
            cmdSetBits|=kBtnSpeed;
            if(speed>swLimit){
                cmdClearBits|=(kBtnForward|kBtnBack|kBtnMoveLeft|kBtnMoveRight);
            }
        }

        // 4. Fast Stop: contra-strafe no CUserCmd ao soltar WASD no chao
        if(RuntimeBool("fastStop",g_cfg.fastStop,"local","")&&onGround&&!anyMoveHeld&&!spaceHeld&&speed>30.f&&local.anglesValid){
            auto brake=HazzureAccuracy::CounterStrafe(local.velocity,local.angles.y,18.f);
            if(brake.valid&&brake.brake){
                if(brake.forward>.15f)cmdSetBits|=kBtnForward;
                else if(brake.forward<-.15f)cmdSetBits|=kBtnBack;
                if(brake.side<-.15f)cmdSetBits|=kBtnMoveLeft;
                else if(brake.side>.15f)cmdSetBits|=kBtnMoveRight;
            }
        }

        // 5. Slide walk: alterna bit de speed em movimento terrestre
        if(RuntimeBool("slideWalk",g_cfg.slideWalk,"local","")&&onGround&&anyMoveHeld&&!slowWalkOn){
            if(s_moveTick&1u)cmdSetBits|=kBtnSpeed;
            else cmdClearBits|=kBtnSpeed;
        }

        // 6. Anti-AFK: micro-strafe periodico quando sem input fisico por > 8 s
        static uint64_t s_lastUserActivity=0;
        if(anyMoveHeld||spaceHeld||physicalAttack||(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0)s_lastUserActivity=now;
        if(RuntimeBool("antiAfk",g_cfg.antiAfk,"local","")&&now>s_lastUserActivity+8000){
            uint64_t phase=(now-s_lastUserActivity)%4000;
            if(phase<60)cmdSetBits|=kBtnMoveLeft;
            else if(phase>=2000&&phase<2060)cmdSetBits|=kBtnMoveRight;
        }

        // 7. Automatic Grenade Release: solta granada armada automaticamente no ponto de arremesso
        if(RuntimeBool("autoGrenadeRelease",g_cfg.autoGrenadeRelease,"local","")&&definition>=43&&definition<=48&&weapon){
            uint8_t pin=0;float strength=0.f;
            if(SafeRead(weapon+0x1f23,&pin)&&pin==1&&SafeRead(weapon+0x1f30,&strength)&&strength>=.99f&&LineupThrowReady(client,localPawn)){
                cmdClearBits|=(kBtnAttack|kBtnAttack2);
            }
        }

        // Auto-Buy vitrine removida na T058 (sem consumidor de compra em engine)
    }
    HazzureMovementTech::PublishCmdButtonMasks(cmdSetBits,cmdClearBits);

    static uint64_t logged{};

    if(g_cfg.bhopActive&&now>=logged&&now-logged>=2000){logged=now;

        LogDebug("[MOVEMENT] context=%d walking=%d space=%d ground=%d jumpOwned=%d phaseDown=%d lineup=%d publish=%ld stamp=%ld down=%ld penalty=%llu lifted=%d",context,walking,spaceHeld,onGround,jump.owned,jump.down,HazzureMove::lineupBusy.load(),

            HazzureMove::jumpPublishCount,HazzureMove::jumpStampCount,HazzureMove::jumpStampDown,penalty,(int)jump.lifted);}

}

