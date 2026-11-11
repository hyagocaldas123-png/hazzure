// Bounded movement decisions with native hull prediction. Never writes velocity or fall damage.

#pragma once

#include "PhysicsQueries.h"

#include "MovementCvars.h"

#include "MovementTech.h"

namespace HazzureMovementTech {

using HazzureGame::Vec3;

struct Strafe {bool valid{};float yaw{},gain{},angle{},applied{};int side{};};
inline std::atomic<int> g_strafeCommandSide{0};

inline Strafe OptimalStrafe(Vec3 velocity,float viewYaw,float dt,float acceleration,float cap,float wish,float friction,int turn){

    Strafe out{};float speed=std::hypot(velocity.x,velocity.y);

    if(!HazzureGame::Finite(velocity,20000)||!std::isfinite(viewYaw)||!std::isfinite(dt)||dt<=0||dt>.05f||

       !std::isfinite(acceleration)||acceleration<=0||acceleration>10000||!std::isfinite(cap)||cap<=0||cap>1000||

       !std::isfinite(wish)||wish<=0||wish>1000||!std::isfinite(friction)||friction<=0||friction>2||speed<1)return out;

    float available=acceleration*dt*wish*friction;

    float projection=std::max(0.f,cap-available),angle=std::acos(std::clamp(projection/speed,0.f,1.f))*57.2957795131f;

    float yaw=std::atan2(velocity.y,velocity.x)*57.2957795131f;

    float delta=HazzureGame::NormalizeAngle(viewYaw-yaw);int sign=turn?(turn>0?1:-1):(delta>=0?1:-1);

    float desired=yaw+angle*sign;

    out.valid=true;out.side=sign>0?-1:1;

    // Holding A points 90 degrees left of camera forward; D points 90 degrees right.

    out.yaw=HazzureGame::NormalizeAngle(desired-(sign>0?90.f:-90.f));

    float applied=std::min(available,std::max(0.f,cap-speed*std::cos(angle*.01745329252f)));

    out.angle=angle;out.applied=applied;out.gain=std::sqrt(speed*speed+applied*applied+2*speed*applied*std::cos(angle*.01745329252f))-speed;return out;

}

// tick: 0 | reason: Briefing A — pure StrafeSolver, velocity-based, outputs forwardmove/sidemove
struct StrafeCmd {float fwd{},side{};bool valid{};};
inline StrafeCmd SolveStrafe(Vec3 velocity,float viewYaw,int turn,float gainPct=80.f,float angleStep=89.f){
    StrafeCmd out{};float speed=std::hypot(velocity.x,velocity.y);
    if(speed<1||!HazzureGame::Finite(velocity,20000)||!std::isfinite(viewYaw))return out;
    float velYaw=std::atan2(velocity.y,velocity.x)*57.2957795131f;
    float delta=HazzureGame::NormalizeAngle(viewYaw-velYaw);
    int sign=turn?(turn>0?1:-1):(delta>=0?1:-1);
    float wishYaw=velYaw+angleStep*sign;
    float ang=HazzureGame::NormalizeAngle(wishYaw-viewYaw)*0.01745329252f;
    float scale=450.0f*(gainPct*0.01f);
    out.fwd=scale*std::cos(ang);out.side=scale*std::sin(ang);
    float len=std::hypot(out.fwd,out.side);
    if(len>450.0f){out.fwd=(out.fwd/len)*450.0f;out.side=(out.side/len)*450.0f;}
    out.valid=true;return out;
}

struct Prediction {bool valid{},landing{},edge{},wall{};float fraction=1;Vec3 normal{};};

inline bool Confirmed(const Prediction& p){
    if(!p.valid||!std::isfinite(p.fraction)||p.fraction<0||p.fraction>1)return false;
    if(p.fraction==1)return !p.landing&&!p.edge&&!p.wall;
    float length=std::sqrt(p.normal.x*p.normal.x+p.normal.y*p.normal.y+p.normal.z*p.normal.z);
    return HazzureGame::Finite(p.normal,1.1f)&&std::isfinite(length)&&length>=.95f&&length<=1.05f&&
        (!p.landing||p.normal.z>=.7f)&&(!p.wall||std::fabs(p.normal.z)<.2f)&&(!p.edge||p.landing);
}

struct Settings {bool active{},strafe{},jumpbug{},edgebug{},surf{};uint64_t time{};float dt=1.f/64.f;

    bool space{},jumpbugHeld{},edgebugHeld{},surfHeld{};HazzureMove::Settings mouse{};};

struct Decision {bool duck{},jump{},surf{};};

struct LandingGate {

    uintptr_t pawn{};bool jumpPrepared{},edgePrepared{};unsigned preparedFrames{};uint64_t preparedTime{},lastTime{};

    Decision Step(uintptr_t identity,bool active,bool ground,bool falling,const Prediction& p,bool jb,bool eb,bool surf,uint64_t now=0){

        if(!active||!identity||identity!=pawn||(now&&lastTime&&now<lastTime)){*this={};pawn=identity;}
        if(!active||!identity)return {};
        if(now&&preparedTime&&now-preparedTime>50){jumpPrepared=edgePrepared=false;preparedTime=0;}
        lastTime=now;const bool confirmed=Confirmed(p);
        if(!ground&&!confirmed){jumpPrepared=edgePrepared=false;preparedTime=0;preparedFrames=0;}

        Decision out{};

        if(jb){

            if(jumpPrepared&&ground){out.jump=true;jumpPrepared=false;preparedFrames=0;}

            else if(falling&&confirmed&&p.landing){out.duck=true;jumpPrepared=true;preparedFrames=0;preparedTime=now;}

            else if(jumpPrepared&&!now&&++preparedFrames>2)jumpPrepared=false;

            else if(ground||!falling)jumpPrepared=false;

        }else jumpPrepared=false;

        if(!jb&&eb){

            if(falling&&confirmed&&p.landing&&p.edge){out.duck=!edgePrepared;edgePrepared=true;preparedTime=now;}

            else if(ground||!falling||!confirmed)edgePrepared=false;

        }else edgePrepared=false;

        out.surf=!jb&&!eb&&surf&&falling&&confirmed&&p.wall;

        out.duck=out.duck||out.surf;return out;

    }

};

inline SRWLOCK lock=SRWLOCK_INIT;inline Settings settings;

inline LandingGate gate;inline uintptr_t ownerClient{},lastPawn{};inline bool directionOwned{},duckOwned{};

inline std::atomic<bool> jumpRequest{};

inline std::atomic<int> state{}; // 0 idle, 1 ready, 2 prediction unavailable, 3 assistance, 4 cvar unavailable

inline void Configure(const Settings& value){AcquireSRWLockExclusive(&lock);settings=value;ReleaseSRWLockExclusive(&lock);}

inline bool WriteDirection(uintptr_t client,int side,bool forward=false){

    directionOwned=true;ownerClient=client;bool good=true;

    good=HazzureMove::Button(client+HazzureOffsets::Buttons::left,"left",side<0?65537:256)&&good;

    good=HazzureMove::Button(client+HazzureOffsets::Buttons::right,"right",side>0?65537:256)&&good;

    good=HazzureMove::Button(client+HazzureOffsets::Buttons::forward,"forward",forward?65537:256)&&good;

    good=HazzureMove::Button(client+HazzureOffsets::Buttons::back,"back",256)&&good;return good;

}

inline void ReleaseDirections(){

    if(!directionOwned)return;

    const uintptr_t offsets[]={HazzureOffsets::Buttons::forward,HazzureOffsets::Buttons::back,HazzureOffsets::Buttons::left,HazzureOffsets::Buttons::right};

    const char* names[]={"forward","back","left","right"};const int keys[]={'W','S','A','D'};bool good=true;

    for(int i=0;i<4;++i)good=HazzureMove::Button(ownerClient+offsets[i],names[i],(GetAsyncKeyState(keys[i])&0x8000)?65537:256)&&good;

    if(good)directionOwned=false;

}

inline void ReleaseDuck(){if(duckOwned){bool physical=(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;

    if(HazzureMove::Button(ownerClient+HazzureOffsets::Buttons::duck,"duck",physical?65537:256))duckOwned=false;}}

inline void Stop(){ReleaseDirections();ReleaseDuck();gate={};lastPawn=0;jumpRequest=false;state=0;}

// -----------------------------------------------------------------------------
// T060: PredictLandingTick — Predição multi-tick (1..8 ticks) com integração
// simétrica de gravidade (StartGravity / FinishGravity: dt * 0.5) e sweep de hull real.
// -----------------------------------------------------------------------------
inline bool PredictLandingTick(uintptr_t client, uintptr_t pawn, const Vec3& origin, const Vec3& velocity,
                               const Vec3& mins, const Vec3& maxs, float gravity, float dt, int& outTick, Vec3* outNormal = nullptr) {
    if (!client || !pawn || !HazzureGame::Finite(origin) || !HazzureGame::Finite(velocity, 20000) || dt <= 0.0f)
        return false;
    Vec3 pos = origin;
    Vec3 vel = velocity;
    for (int t = 1; t <= 8; ++t) {
        vel.z -= gravity * dt * 0.5f;
        Vec3 next = { pos.x + vel.x * dt, pos.y + vel.y * dt, pos.z + vel.z * dt };
        HazzurePhysics::Hit hit{};
        if (HazzurePhysics::Sweep(client, pawn, pos, next, mins, maxs, HazzureMove::playerSolidMask, hit)) {
            if (hit.fraction < 1.0f && (hit.normal.z >= 0.7f || hit.normal.z == 0.0f)) {
                outTick = t;
                if (outNormal) *outNormal = hit.normal;
                return true;
            }
        }
        vel.z -= gravity * dt * 0.5f;
        pos = next;
    }
    return false;
}

// -----------------------------------------------------------------------------
// T064: PredictContact — Detecção precisa de contato no tick atual com hull
// duck-aware e integração de gravidade de acordo com o briefing CS2 sub-tick.
// -----------------------------------------------------------------------------
inline bool PredictContact(uintptr_t client, uintptr_t pawn, const Vec3& origin, const Vec3& velocity,
                           bool isDucked, float& outFraction, Vec3* outNormal = nullptr) {
    if (!client || !pawn || !HazzureGame::Finite(origin) || !HazzureGame::Finite(velocity, 20000))
        return false;

    // Se estiver subindo com velocidade positiva, não há impacto de solo
    if (velocity.z > 25.0f)
        return false;

    constexpr float dt = 0.015625f; // 64 tick
    constexpr float g = 800.0f;     // sv_gravity

    const Vec3 mins = { -16.0f, -16.0f, 0.0f };
    const Vec3 maxs = {  16.0f,  16.0f, isDucked ? 54.0f : 72.0f };

    // Simulação preditiva forward do movimento físico do tick (StartGravity -> Move -> CheckGround)
    Vec3 vel = velocity;
    vel.z -= g * dt * 0.5f; // StartGravity
    Vec3 next = { origin.x + vel.x * dt, origin.y + vel.y * dt, origin.z + vel.z * dt };

    HazzurePhysics::Hit hit{};
    if (HazzurePhysics::Sweep(client, pawn, origin, next, mins, maxs, HazzureMove::playerSolidMask, hit)) {
        if (hit.fraction < 1.0f && (hit.normal.z >= 0.7f || hit.normal.z == 0.0f)) {
            outFraction = hit.fraction;
            if (outNormal) *outNormal = hit.normal;
            return true;
        }
    }

    // Se em queda rápida (< -250 u/s), checa transição imediata para o tick seguinte
    if (vel.z < -250.0f) {
        Vec3 pos2 = next;
        vel.z -= g * dt * 0.5f; // FinishGravity tick 1
        vel.z -= g * dt * 0.5f; // StartGravity tick 2
        Vec3 next2 = { pos2.x + vel.x * dt, pos2.y + vel.y * dt, pos2.z + vel.z * dt };
        HazzurePhysics::Hit hit2{};
        if (HazzurePhysics::Sweep(client, pawn, pos2, next2, mins, maxs, HazzureMove::playerSolidMask, hit2)) {
            if (hit2.fraction < 0.20f && (hit2.normal.z >= 0.7f || hit2.normal.z == 0.0f)) {
                outFraction = 1.0f + hit2.fraction;
                if (outNormal) *outNormal = hit2.normal;
                return true;
            }
        }
    }

    return false;
}

inline std::atomic<int> g_predictedLandingTick{ -1 };
inline std::atomic<int> g_optimalStrafeSide{ 0 };
inline std::atomic<float> g_optimalForwardMove{ 0.0f };
inline std::atomic<float> g_optimalSideMove{ 0.0f };


inline Prediction Predict(uintptr_t client,const HazzureGame::Pawn& local,const HazzureCvars::Values& cvars,float dt){

    Prediction p{};Vec3 minimum{},maximum{};if(!HazzurePhysics::CollisionBounds(local.address,minimum,maximum))return p;

    Vec3 end{local.origin.x+local.velocity.x*dt,local.origin.y+local.velocity.y*dt,local.origin.z+local.velocity.z*dt-.5f*cvars.gravity*dt*dt};

    HazzurePhysics::Hit hit{};

    if(!HazzurePhysics::Sweep(client,local.address,local.origin,end,minimum,maximum,HazzureMove::playerSolidMask,hit)||hit.solid)return p;

    p.valid=true;p.fraction=hit.fraction;p.normal=hit.normal;p.landing=hit.fraction<1&&hit.normal.z>=.7f;

    p.wall=hit.fraction<1&&std::fabs(hit.normal.z)<.2f;

    if(p.landing){

        float contactZ=local.origin.z+(end.z-local.origin.z)*hit.fraction;

        Vec3 foot{end.x,end.y,contactZ+4},down=foot;down.z-=12;

        HazzurePhysics::Hit support{};

        if(HazzurePhysics::Sweep(client,local.address,foot,down,minimum,maximum,HazzureMove::playerSolidMask,support))p.edge=support.fraction==1&&!support.solid;

    }

    return Confirmed(p)?p:Prediction{};

}

inline void Frame(uintptr_t client){

    AcquireSRWLockShared(&lock);auto option=settings;ReleaseSRWLockShared(&lock);auto now=GetTickCount64();

    DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);

    HazzureGame::Pawn local{};auto pawn=HazzureGame::LocalPawn(client);int flags{};uint8_t move{};

    if(!option.active||HazzureMove::lineupBusy.load()||now<option.time||now-option.time>100||foreground!=GetCurrentProcessId()||

       !HazzureVisibility::BuildMatches(client)||!HazzureGame::ReadPawn(pawn,local)||local.health<=0||local.life!=0||!local.velocityValid||!local.anglesValid||

       !HazzureGame::Read(pawn+HazzureOffsets::Schema::m_fFlags,flags)||!HazzureGame::Read(pawn+HazzureMove::moveTypeOffset,move)||move!=2){Stop();return;}

    if(lastPawn&&lastPawn!=pawn)Stop();lastPawn=pawn;auto cvars=HazzureCvars::Read(client);

    if(!cvars.valid){Stop();state=4;return;}

    bool ground=(flags&1)!=0,falling=local.velocity.z<0;

    auto prediction=!ground&&(option.jumpbug||option.edgebug||option.surf)?Predict(client,local,cvars,std::clamp(option.dt,.001f,.05f)):Prediction{};

    auto decision=gate.Step(pawn,true,ground,falling,prediction,option.jumpbug&&option.jumpbugHeld,

        option.edgebug&&option.edgebugHeld,option.surf&&option.surfHeld,now);

    if(decision.jump)jumpRequest.store(true);

    if(decision.duck){duckOwned=true;ownerClient=client;if(!HazzureMove::Button(client+HazzureOffsets::Buttons::duck,"duck",65537)){Stop();return;}}

    else ReleaseDuck();

    state=prediction.valid?1:2;

    if(decision.surf){

        Vec3 target=local.angles;target.y=HazzureGame::NormalizeAngle(std::atan2(-prediction.normal.y,-prediction.normal.x)*57.2957795131f);

        if(!HazzureMove::SendAngles(local.angles,target,option.mouse,4)||!WriteDirection(client,0,true)){Stop();return;}

        state=3;return;

    }

    bool advancedAction=option.jumpbugHeld||option.edgebugHeld||option.surfHeld;

    if(option.strafe&&option.space&&!ground&&!advancedAction){

        uintptr_t list{},services{},weapon{},vdata{};uint32_t handle{};int mode{};float maximum{};

        bool haveWeapon=HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)&&

            HazzureGame::Read(pawn+HazzureOffsets::Schema::m_pWeaponServices,services)&&services&&

            HazzureGame::Read(services+HazzureOffsets::Schema::m_hActiveWeapon,handle)&&(weapon=HazzureGame::Entity(list,handle))&&

            HazzureGame::Read(weapon+0x388,vdata)&&vdata&&HazzureGame::Read(weapon+0x1a00,mode)&&mode>=0&&mode<=1&&

            HazzureGame::Read(vdata+0x748+mode*4,maximum)&&std::isfinite(maximum)&&maximum>1&&maximum<=350;

        // O wishspeed do ar e min(|input|, sv_air_max_wishspeed) = 30, nao a velocidade da
        // arma. Com 250 aqui, available=46,9 estourava o cap, projection zerava e o acos
        // devolvia 90 graus cravados em vez dos ~84 do angulo otimo, alem de inflar o ganho.
        auto choice=haveWeapon?OptimalStrafe(local.velocity,local.angles.y,option.dt,cvars.airAcceleration,cvars.airWishSpeed,cvars.airWishSpeed,1.f,0):Strafe{};

        if(choice.valid){
            static uint64_t strafeLogged{};
            if(now>=strafeLogged){strafeLogged=now+1000;
                LogDebug("[STRAFE] speed=%.1f angulo=%.1f aplicado=%.2f ganho=%.2f lado=%d wish=%.0f acc=%.0f dt=%.4f",
                    std::hypot(local.velocity.x,local.velocity.y),choice.angle,choice.applied,choice.gain,choice.side,
                    cvars.airWishSpeed,cvars.airAcceleration,option.dt);}

            // Air boost via CUserCmd (mira 100% livre como Neverlose/Gamesense)
            g_strafeCommandSide.store(choice.side);
            WriteDirection(client,choice.side);
            state=3;return;}

    }
    g_strafeCommandSide.store(0);

    ReleaseDirections();

}

inline const char* Label(){switch(state.load()){case 1:return "Predicao pronta";case 2:return "Aguardando predicao de contato";

    case 3:return "Assistencia ativa";case 4:return "Parametros fisicos indisponiveis";default:return "Inativo";}}

inline std::atomic<uint64_t> g_cmdButtonSetMask{0};
inline std::atomic<uint64_t> g_cmdButtonClearMask{0};
inline std::atomic<uint64_t> g_cmdButtonStamp{0};
inline void PublishCmdButtonMasks(uint64_t setBits, uint64_t clearBits) {
    g_cmdButtonSetMask.store(setBits);
    g_cmdButtonClearMask.store(clearBits);
    g_cmdButtonStamp.store(setBits || clearBits ? GetTickCount64() : 0);
}
inline bool ReadCmdButtonMasks(uint64_t& setBits, uint64_t& clearBits) {
    const uint64_t stamp = g_cmdButtonStamp.load();
    if (!stamp || GetTickCount64() - stamp >= 120) return false;
    setBits = g_cmdButtonSetMask.load();
    clearBits = g_cmdButtonClearMask.load();
    return (setBits | clearBits) != 0;
}


// Reprojeção 2D de comandos de movimento (Item 10 do Briefing Rage)
// Projeta forwardmove e sidemove calculados no yaw real para o yaw enviado com Anti-Aim
inline void RotateInputs(float& fwd, float& side, float realYaw, float sentYaw) {
    float diff = HazzureGame::NormalizeAngle(sentYaw - realYaw);
    if (std::fabs(diff) < 0.001f) return;
    float rad = -diff * 0.017453292519943295f;
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);
    float origFwd = fwd;
    float origSide = side;
    fwd = origFwd * cosR - origSide * sinR;
    side = origFwd * sinR + origSide * cosR;
}
}

