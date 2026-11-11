// Incremental 3-D grenade preview. Present submits snapshots; only Frame processes physics.
// Native swept AABB. Launch parameters are read where available; fuse/material rules
// still require calibration before claiming server-equivalent prediction.
#pragma once
#include "Visibility.h"
#include "PhysicsQueries.h"
#include "MovementCvars.h"
#include <array>

namespace HazzureGrenade {
using HazzureGame::Vec3;
enum class Kind { None,Flash,HE,Smoke,Fire,Decoy };
enum class Finish { Pending,DetonationEstimate,RestEstimate,Horizon,Stuck,Unavailable };
inline Kind Type(uint16_t definition){switch(definition){case 43:return Kind::Flash;case 44:return Kind::HE;
    case 45:return Kind::Smoke;case 46:case 48:return Kind::Fire;case 47:return Kind::Decoy;default:return Kind::None;}}
inline Vec3 Add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 Scale(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float Length(Vec3 a){return std::sqrt(Dot(a,a));}
inline float Separation(Vec3 a,Vec3 b){return Length(Add(a,Scale(b,-1)));}
struct Parameters {float step=1.f/64.f,gravity=320.f,elasticity=.45f,horizon=6.f,fireFuse=3.5f;};
struct Request {
    uintptr_t pawn{},weapon{},list{};uint32_t weaponHandle{};uint16_t definition{};
    Vec3 eye{},forward{},velocity{};float strength=1.f,simulation{};uint64_t epoch{};bool engineStrength{};
    float throwVelocity=750.f,gravity=320.f;bool nativeParameters{};
};
struct Hit {float fraction=1.f;Vec3 normal{};bool solid{};};
using Query=bool(*)(void*,Vec3,Vec3,Hit&);
struct Result {
    std::array<Vec3,512> points{};std::array<Vec3,48> bounces{};size_t count{},bounceCount{};
    Finish finish=Finish::Pending;float time{};bool complete{};
};
inline Vec3 Reflect(Vec3 velocity,Vec3 normal,float elasticity){
    float length=Length(normal);if(!std::isfinite(length)||length<.5f)return {};
    normal=Scale(normal,1.f/length);float into=Dot(velocity,normal);
    if(into>=0)return velocity;
    Vec3 out=Scale(Add(velocity,Scale(normal,-2.f*into)),std::clamp(elasticity,0.f,.9f));
    if(std::fabs(out.x)<.1f)out.x=0;if(std::fabs(out.y)<.1f)out.y=0;if(std::fabs(out.z)<.1f)out.z=0;
    return out;
}
inline bool Valid(const Request& q){
    float forwardLength=Length(q.forward);
    return q.pawn&&q.weapon&&q.list&&Type(q.definition)!=Kind::None&&HazzureGame::Finite(q.eye)&&
        HazzureGame::Finite(q.velocity,10000)&&HazzureGame::Finite(q.forward,1.1f)&&
        std::isfinite(forwardLength)&&forwardLength>.99f&&forwardLength<1.01f&&
        std::isfinite(q.strength)&&q.strength>=0&&q.strength<=1&&std::isfinite(q.simulation);
}
inline bool Same(const Request& a,const Request& b){
    return a.pawn==b.pawn&&a.weapon==b.weapon&&a.list==b.list&&a.weaponHandle==b.weaponHandle&&
        a.definition==b.definition&&a.epoch==b.epoch&&b.simulation+.25f>=a.simulation&&
        Separation(a.eye,b.eye)<=2.f&&Separation(a.forward,b.forward)<=.004f&&
        Separation(a.velocity,b.velocity)<=5.f&&std::fabs(a.strength-b.strength)<.01f&&
        a.nativeParameters==b.nativeParameters&&std::fabs(a.gravity-b.gravity)<.01f&&std::fabs(a.throwVelocity-b.throwVelocity)<.01f;
}
struct Simulation {
    Request request{};Parameters parameters{};Result result{};Vec3 position{},velocity{};
    float remaining{};unsigned contacts{},zeroProgress{};bool launched{},resting{};
    void FinishWith(Finish why){result.finish=why;result.complete=true;}
    void Point(Vec3 point){if(result.count<result.points.size())result.points[result.count++]=point;else FinishWith(Finish::Horizon);}
    bool Begin(const Request& q,Parameters p={}){
        *this={};request=q;parameters=p;
        if(!Valid(q)||!std::isfinite(p.step)||p.step<.001f||p.step>.05f||!std::isfinite(p.gravity)||p.gravity<0||p.gravity>2000||
           !std::isfinite(p.elasticity)||p.elasticity<0||p.elasticity>.9f||!std::isfinite(p.horizon)||p.horizon<=0||p.horizon>7||
           !std::isfinite(p.fireFuse)||p.fireFuse<.1f||p.fireFuse>7){FinishWith(Finish::Unavailable);return false;}
        position=q.eye;position.z+=q.strength*12.f-12.f;
        if(!std::isfinite(q.throwVelocity)||q.throwVelocity<1||q.throwVelocity>3000||!std::isfinite(q.gravity)||q.gravity<0||q.gravity>4000){FinishWith(Finish::Unavailable);return false;}
        if(q.nativeParameters)parameters.gravity=q.gravity;
        velocity=Add(Scale(q.forward,.9f*q.throwVelocity*(.3f+.7f*q.strength)),Scale(q.velocity,1.25f));
        Point(position);return true;
    }
    float Fuse()const {auto type=Type(request.definition);return type==Kind::Flash||type==Kind::HE?1.5f:type==Kind::Fire?parameters.fireFuse:parameters.horizon;}
    bool ReadHit(Query query,void* context,Vec3 end,Hit& hit){
        if(!query||!query(context,position,end,hit)||!std::isfinite(hit.fraction)||hit.fraction<0||hit.fraction>1||hit.solid){
            FinishWith(Finish::Unavailable);return false;}
        if(hit.fraction<1.f){float length=Length(hit.normal);
            if(!HazzureGame::Finite(hit.normal,1.1f)||!std::isfinite(length)||length<.9f||length>1.1f){FinishWith(Finish::Unavailable);return false;}
            hit.normal=Scale(hit.normal,1.f/length);}
        return true;
    }
    // The budget counts native queries, including release clearance and residual sweeps.
    // Retaining remaining time allows corners to be resolved over multiple callbacks.
    unsigned Advance(Query query,void* context,unsigned budget){
        unsigned used=0;
        while(!result.complete&&used<budget){
            if(!launched){Hit hit{};Vec3 end=Add(position,Scale(request.forward,22.f));++used;
                if(!ReadHit(query,context,end,hit))break;
                float distance=std::max(0.f,22.f*hit.fraction-6.f);
                position=Add(position,Scale(request.forward,distance));launched=true;Point(position);continue;}
            const auto type=Type(request.definition);float fuse=Fuse();
            if(result.time>=fuse-.00001f){FinishWith(type==Kind::Flash||type==Kind::HE||type==Kind::Fire?Finish::DetonationEstimate:Finish::Horizon);break;}
            if(resting){
                if(type==Kind::Smoke||type==Kind::Decoy){FinishWith(Finish::RestEstimate);break;}
                result.time=fuse;FinishWith(Finish::DetonationEstimate);break;
            }
            if(remaining<=.000001f){remaining=std::min(parameters.step,fuse-result.time);contacts=0;}
            Vec3 end=Add(position,Scale(velocity,remaining));end.z-=.5f*parameters.gravity*remaining*remaining;
            Hit hit{};++used;if(!ReadHit(query,context,end,hit))break;
            float elapsed=remaining*hit.fraction;
            position=Add(position,Scale(Add(end,Scale(position,-1)),hit.fraction));
            velocity.z-=parameters.gravity*elapsed;result.time+=elapsed;remaining-=elapsed;
            if(!HazzureGame::Finite(position)||!HazzureGame::Finite(velocity,10000)){FinishWith(Finish::Unavailable);break;}
            Point(position);if(result.complete)break;
            if(hit.fraction==1.f){remaining=0;zeroProgress=0;continue;}
            if(result.bounceCount<result.bounces.size())result.bounces[result.bounceCount++]=position;
            if(type==Kind::Fire&&hit.normal.z>=.70710678f){FinishWith(Finish::DetonationEstimate);break;}
            if(elapsed<.00001f)++zeroProgress;else zeroProgress=0;
            if(++contacts>4||zeroProgress>=3||result.bounceCount==result.bounces.size()){FinishWith(Finish::Stuck);break;}
            float incomingSpeed=Length(velocity),incomingCos=incomingSpeed>.0001f?-Dot(velocity,hit.normal)/incomingSpeed:0.f;
            velocity=Reflect(velocity,hit.normal,parameters.elasticity);
            if(hit.normal.z>.7f&&incomingSpeed>690.f&&incomingCos>.5f)velocity=Scale(velocity,1.5f-incomingCos);
            if(hit.normal.z>.7f&&Length(velocity)<19.685f){velocity={};resting=true;remaining=0;}
            // Small normal offset prevents immediately re-hitting the identical plane.
            position=Add(position,Scale(hit.normal,.03125f));
        }
        return used;
    }
};

struct NativeContext {uintptr_t client{},world{};alignas(16) unsigned char filter[0x110]{};};
// SKY and PLAYER_CLIP are deliberately excluded; grenadeclip is Source 2 layer 33.
// This mask does not reproduce every per-material exclusion of server grenades.
inline constexpr uint64_t collisionMask=(1ull<<0)|(1ull<<12)|(1ull<<13)|(1ull<<21)|(1ull<<33);
inline bool PrepareNative(uintptr_t client,const Request& q,NativeContext& context){
#ifdef INTERNAL_SELF_TEST
    (void)client;(void)q;(void)context;return false;
#else
    if(!HazzureVisibility::BuildMatches(client)||!Valid(q)||HazzureGame::LocalPawn(client)!=q.pawn)return false;
    uintptr_t list{},services{};uint32_t handle{};HazzureGame::Pawn pawn{};
    if(!HazzureGame::ReadPawn(q.pawn,pawn)||!HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||list!=q.list||
       !HazzureGame::Read(q.pawn+HazzureOffsets::Schema::m_pWeaponServices,services)||!services||
       !HazzureGame::Read(services+HazzureOffsets::Schema::m_hActiveWeapon,handle)||handle!=q.weaponHandle||HazzureGame::Entity(list,handle)!=q.weapon)return false;
    auto profile=HazzureVisibility::SelectTraceProfile(client);
    if(!profile.trace||!profile.filter||!profile.world)return false;
    constexpr unsigned char traceCode[]={0x48,0x89,0x54,0x24,0x10,0x48,0x89,0x4c,0x24,0x08,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57};
    constexpr unsigned char filterCode[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x0f,0xb6,0x41,0x39};
    if(!HazzureVisibility::Prefix(client+profile.trace,traceCode,sizeof(traceCode))||!HazzureVisibility::Prefix(client+profile.filter,filterCode,sizeof(filterCode)))return false;
    uintptr_t object{};if(!HazzureGame::Read(client+profile.world,context.world)||!context.world||!HazzureGame::Read(context.world,object)||!object)return false;
    context.client=client;using Filter=void*(*)(void*,uintptr_t,uint64_t,int,uint16_t);
    __try{reinterpret_cast<Filter>(client+profile.filter)(context.filter,q.pawn,collisionMask,3,15);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
#endif
}
inline bool NativeQuery(void* opaque,Vec3 start,Vec3 end,Hit& hit){
#ifdef INTERNAL_SELF_TEST
    (void)opaque;(void)start;(void)end;(void)hit;return false;
#else
    auto context=static_cast<NativeContext*>(opaque);if(!context||!context->world||!HazzureGame::Finite(start)||!HazzureGame::Finite(end))return false;
    auto profile=HazzureVisibility::SelectTraceProfile(context->client);
    if(!profile.trace)return false;
    HazzureVisibility::Ray ray{};HazzureVisibility::Trace trace{};trace.fraction=-1.f;
    if(!HazzurePhysics::Box(ray,{-2,-2,-2},{2,2,2}))return false;
    using TraceShape=bool(*)(uintptr_t,const HazzureVisibility::Ray*,const Vec3*,const Vec3*,const void*,HazzureVisibility::Trace*);
    __try{reinterpret_cast<TraceShape>(context->client+profile.trace)(context->world,&ray,&start,&end,context->filter,&trace);}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    hit.fraction=trace.fraction;hit.solid=trace.solid;
    // Installed PE: 9D3F31/9D3F37/9D3F43 copy the impact normal into +90/+98.
    memcpy(&hit.normal,reinterpret_cast<const unsigned char*>(&trace)+0x90,sizeof(hit.normal));return true;
#endif
}
struct Channel {
    SRWLOCK lock=SRWLOCK_INIT;Request request{},evaluated{};Result output{},visible{};bool hasVisible{};
    uint64_t generation{},published{},completed{};bool active{};
    void Clear(){AcquireSRWLockExclusive(&lock);active=false;output={};visible={};hasVisible=false;++generation;ReleaseSRWLockExclusive(&lock);}
    bool Submit(const Request& q,uint64_t now,Result& result){
        if(!Valid(q)){Clear();result={};return false;}
        AcquireSRWLockExclusive(&lock);
        if(!active||!Same(request,q)||now<published||now-published>200){++generation;completed=0;}
        request=q;published=now;active=true;
        bool ready=completed&&now>=completed&&now-completed<=200&&Same(evaluated,q);
        if(ready){visible=output;hasVisible=true;}
        // Keep the last complete trajectory on screen while a new one is recomputed;
        // an empty result would make the line blink out on every aim adjustment.
        result=hasVisible?visible:Result{};
        ReleaseSRWLockExclusive(&lock);return ready;
    }
};
inline Channel channel;
inline Simulation work;
inline uint64_t workingGeneration{},lastRefresh{};
inline void Clear(){channel.Clear();}
inline void ProcessPending(uintptr_t client,Query query=nullptr,void* suppliedContext=nullptr,unsigned budget=24){
    uint64_t now=GetTickCount64(),generation{},published{};Request request{};bool active{};
    AcquireSRWLockShared(&channel.lock);active=channel.active;request=channel.request;generation=channel.generation;published=channel.published;ReleaseSRWLockShared(&channel.lock);
    if(!active||now<published||now-published>200){work={};workingGeneration=0;return;}
    if(generation!=workingGeneration||(work.result.complete&&now-lastRefresh>=150)){
        work.Begin(request);workingGeneration=generation;lastRefresh=now;
    }
    if(!work.result.complete){
        NativeContext native{};void* context=suppliedContext;
        if(!query){if(!PrepareNative(client,request,native)){work.FinishWith(Finish::Unavailable);}
            else {query=NativeQuery;context=&native;}}
        if(query){const auto begin=GetTickCount64();
            for(unsigned i=0;i<std::min(budget,160u)&&!work.result.complete;++i){
                work.Advance(query,context,1);if(GetTickCount64()-begin>=2)break;
            }}
    }
    AcquireSRWLockExclusive(&channel.lock);
    if(channel.active&&channel.generation==generation&&Same(work.request,channel.request)){
        channel.evaluated=work.request;channel.output=work.result;channel.completed=GetTickCount64();}
    ReleaseSRWLockExclusive(&channel.lock);
}
inline bool MakeRequest(uintptr_t client,uintptr_t local,float strength,uint64_t epoch,Request& request){
    using namespace HazzureOffsets::Schema;Request q{};q.pawn=local;q.strength=strength;q.epoch=epoch;
    HazzureGame::Pawn pawn{};uintptr_t services{};
    if(!HazzureGame::ReadPawn(local,pawn)||!pawn.anglesValid||!pawn.velocityValid||
       !HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,q.list)||!q.list||
       !HazzureGame::Read(local+m_pWeaponServices,services)||!services||!HazzureGame::Read(services+m_hActiveWeapon,q.weaponHandle))return false;
    q.weapon=HazzureGame::Entity(q.list,q.weaponHandle);
    if(!q.weapon||!HazzureGame::Read(q.weapon+m_iItemDefinitionIndex,q.definition)||Type(q.definition)==Kind::None)return false;
    // These fields were uniquely recovered from SchemaClassFieldData in the installed
    // fingerprint, not copied blindly from an older public dump. Before pulling the
    // pin, keys still preview the intended throw; while primed use actual interpolation.
    if(HazzureVisibility::BuildMatches(client)){
        uint8_t pin{};float nativeStrength{};
        if(HazzureGame::Read(q.weapon+0x1f23,pin)&&pin==1&&HazzureGame::Read(q.weapon+0x1f30,nativeStrength)&&
           std::isfinite(nativeStrength)&&nativeStrength>=0&&nativeStrength<=1){q.strength=nativeStrength;q.engineStrength=true;}
        uintptr_t vdata{};float throwVelocity{},gravity{};
        if(HazzureGame::Read(q.weapon+0x388,vdata)&&vdata&&HazzureGame::Read(vdata+0x858,throwVelocity)&&
           std::isfinite(throwVelocity)&&throwVelocity>0&&throwVelocity<=3000&&HazzureCvars::Value(client,0x23bf1c0,0,10000,gravity)){
            q.throwVelocity=throwVelocity;q.gravity=gravity*.4f;q.nativeParameters=true;}
    }
    q.eye=pawn.eye;q.velocity=pawn.velocity;
    float pitch=pawn.angles.x-(90.f-std::fabs(pawn.angles.x))*(10.f/90.f),yaw=pawn.angles.y;
    pitch*=.01745329252f;yaw*=.01745329252f;
    q.forward={std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch)};
    if(!HazzureGame::Read(local+0x3b8,q.simulation)||!std::isfinite(q.simulation))return false;
    if(!Valid(q))return false;request=q;return true;
}
} // namespace HazzureGrenade
