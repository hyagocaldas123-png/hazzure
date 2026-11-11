// Fingerprinted ClientMode::OverrideView; camera fields consumed by native projection.
#pragma once
#include "CameraFov.h"
#include "WorldTrace.h"
#include "DynamicSchema.h"
#include "TacticalHUD.h"
namespace HazzureNativeCamera {
using Vec3=HazzureGame::Vec3;
inline constexpr uintptr_t viewRva=0xB9E390,drawRva=0xB9EF20;
inline constexpr unsigned char viewPrefix[]={0x4c,0x8b,0xdc,0x49,0x89,0x4b,0x08,0x53,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xc8,0,0,0};
inline constexpr unsigned char drawPrefix[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xca,0x48,0x8b,0xda};
inline bool Supported(uintptr_t client){return HazzureVisibility::BuildMatches(client)&&
    HazzureVisibility::Prefix(client+viewRva,viewPrefix,sizeof(viewPrefix))&&HazzureVisibility::Prefix(client+drawRva,drawPrefix,sizeof(drawPrefix));}
struct Settings {bool thirdperson{},aspectOverride{},customViewmodel{},customCameraFov{},allowed{};float distance=120,ratio=4.f/3.f,viewmodelFov=68.f,cameraFov=105.f;};
inline bool Valid(const Settings& s){return std::isfinite(s.distance)&&s.distance>=30&&s.distance<=300&&std::isfinite(s.ratio)&&s.ratio>=.5f&&s.ratio<=2.5f&&(!s.customViewmodel||(std::isfinite(s.viewmodelFov)&&s.viewmodelFov>=40.f&&s.viewmodelFov<=140.f))&&(!s.customCameraFov||(std::isfinite(s.cameraFov)&&s.cameraFov>=50.f&&s.cameraFov<=150.f));}
struct Activation {
    int key=-1;bool previous{},latched{};
    bool Update(bool enabled,int nextKey,bool down,bool allowed){
        if(!enabled||!allowed){key=nextKey;previous=down;latched=false;return false;}
        if(nextKey==0){key=0;previous=false;latched=true;return true;}
        if(key!=nextKey){key=nextKey;previous=down;latched=false;return false;}
        if(down&&!previous)latched=!latched;previous=down;return latched;
    }
};
enum class Status {Disabled,Waiting,Scoped,Applied,AspectOnly,TraceUnavailable,InvalidView,Unavailable};
inline std::atomic<bool> installed{};inline std::atomic<Status> status{Status::Waiting};
inline SRWLOCK lock=SRWLOCK_INIT;inline Settings configured{};inline uint64_t configuredAt{};
inline std::atomic<uintptr_t> bodyPawn{};inline std::atomic<uint32_t> bodyHandle{};inline std::atomic<uint64_t> bodyEpoch{},bodyAt{};
inline std::atomic<float> actualDistance{},actualRatio{};
inline std::atomic<uint64_t> bodyCalls{},viewSuppressions{};
// Terceira pessoa de verdade tambem esconde o viewmodel: com a camera atrasada e as maos
// ainda presas a view, o resultado e o viewmodel esticado na tela, nao o boneco.
inline bool ViewModelSuppress(){
    Settings s{};AcquireSRWLockShared(&lock);s=configured;ReleaseSRWLockShared(&lock);
    return s.thirdperson&&s.allowed&&status.load()==Status::Applied&&actualDistance.load()>=16.f;
}
inline void ClearBody(){bodyAt.store(0,std::memory_order_release);bodyPawn.store(0);bodyHandle.store(0);}
inline void Configure(const Settings& s,uint64_t now){AcquireSRWLockExclusive(&lock);configured=Valid(s)?s:Settings{};configuredAt=now;ReleaseSRWLockExclusive(&lock);}
inline const char* Label(){
    if(!installed.load())return "Camera nativa indisponivel neste build";
    switch(status.load()){case Status::Disabled:return "Desativada";case Status::Scoped:return "Zoom nativo preservado";
    case Status::Applied:return "Aplicada na projecao nativa";case Status::AspectOnly:return "Proporcao aplicada; camera aguardando geometria";case Status::TraceUnavailable:return "Aguardando geometria da camera";
    case Status::InvalidView:return "View nativa invalida; original preservada";case Status::Unavailable:return "Build nao suportado";default:return "Aguardando camera local";}
}
struct View {int width{},height{};float fov{},handFov{},aspect{};Vec3 origin{},angles{},originA{},originB{};};
inline bool ViewValid(const View& v){return v.width>0&&v.height>0&&v.width<=16384&&v.height<=16384&&
    std::isfinite(v.fov)&&v.fov>1&&v.fov<179&&std::isfinite(v.handFov)&&v.handFov>1&&v.handFov<179&&
    std::isfinite(v.aspect)&&v.aspect>.1f&&v.aspect<10&&HazzureGame::Finite(v.origin)&&HazzureGame::Finite(v.angles,36000);}
inline bool ReadView(uintptr_t at,View& v){return at&&HazzureGame::Read(at+0x434,v.width)&&HazzureGame::Read(at+0x43c,v.height)&&
    HazzureGame::Read(at+0x498,v.fov)&&HazzureGame::Read(at+0x49c,v.handFov)&&HazzureGame::Read(at+0x4d4,v.aspect)&&
    HazzureGame::Read(at+0x4a0,v.origin)&&HazzureGame::Read(at+0x4b8,v.angles)&&
    HazzureGame::Read(at+0x554,v.originA)&&HazzureGame::Read(at+0x56c,v.originB)&&ViewValid(v);}
inline float RebaseFov(float fov,float from,float to){
    constexpr float radians=.0174532925199433f;
    return std::atan(std::tan(fov*radians*.5f)*(to/from))*2.f/radians;
}
using Query=bool(*)(void*,const Vec3&,const Vec3&,HazzureWorldTrace::Result&);
inline bool Compose(const View& native,const Settings& s,Query query,void* context,View& out,float& distance){
    if(!ViewValid(native)||!Valid(s))return false;out=native;distance=0;
    if(s.customCameraFov&&std::isfinite(s.cameraFov)&&s.cameraFov>=50.f&&s.cameraFov<=150.f)out.fov=s.cameraFov;
    if(s.aspectOverride){out.fov=RebaseFov(out.fov,native.aspect,s.ratio);out.handFov=RebaseFov(out.handFov,native.aspect,s.ratio);out.aspect=s.ratio;}
    if(s.customViewmodel&&std::isfinite(s.viewmodelFov)&&s.viewmodelFov>=40.f&&s.viewmodelFov<=140.f)out.handFov=s.viewmodelFov;
    if(s.thirdperson){
        float pitch=native.angles.x*.01745329252f,yaw=native.angles.y*.01745329252f;
        Vec3 forward{std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch)};
        Vec3 desired{native.origin.x-forward.x*s.distance,native.origin.y-forward.y*s.distance,native.origin.z-forward.z*s.distance};
        HazzureWorldTrace::Result hit{};
        if(!query||!query(context,native.origin,desired,hit)||!HazzureWorldTrace::Valid(hit)){
            distance=s.distance;out.origin=desired;out.originA=out.originB=out.origin;
        } else {
            float fraction=hit.solid?0.f:std::clamp(hit.fraction-(hit.fraction<1.f?4.f/s.distance:0.f),0.f,1.f);
            distance=s.distance*fraction;out.origin=HazzureWorldTrace::Along(native.origin,desired,fraction);
            out.originA=out.originB=out.origin;
        }
    }
    return ViewValid(out);
}
template<class T>inline bool Put(uintptr_t at,const T& value){__try{memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
inline bool Commit(uintptr_t at,const View& old,const View& value){
    bool ok=Put(at+0x498,value.fov)&&Put(at+0x49c,value.handFov)&&Put(at+0x4d4,value.aspect)&&
        Put(at+0x4a0,value.origin)&&Put(at+0x554,value.originA)&&Put(at+0x56c,value.originB);
    if(!ok){Put(at+0x498,old.fov);Put(at+0x49c,old.handFov);Put(at+0x4d4,old.aspect);
        Put(at+0x4a0,old.origin);Put(at+0x554,old.originA);Put(at+0x56c,old.originB);}
    return ok;
}
inline void Apply(uintptr_t client,uintptr_t view,bool worldReady,uint64_t epoch,uint64_t now){
    ClearBody();Settings s;uint64_t at{};AcquireSRWLockShared(&lock);s=configured;at=configuredAt;ReleaseSRWLockShared(&lock);
    if(!s.thirdperson&&!s.aspectOverride&&!s.customViewmodel&&!s.customCameraFov){status.store(Status::Disabled);return;}
    if(!worldReady||!s.allowed||!at||now<at||now-at>1000){status.store(Status::Waiting);return;}
    if(!HazzureVisibility::BuildMatches(client)){status.store(Status::Unavailable);return;}
    auto pawn=HazzureGame::LocalPawn(client);HazzureGame::Pawn player{};uint8_t scoped{};uintptr_t list{};
    if(!HazzureGame::ReadPawn(pawn,player)||!HazzureTuning::PlayerClass(pawn)||
       !HazzureGame::Read(pawn+(HazzureSchema::g_mIsScoped.offset ? HazzureSchema::g_mIsScoped.offset : 0x1ea0),scoped)||!HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)){
        status.store(Status::Waiting);return;}
    if(scoped){status.store(Status::Scoped);return;}
    auto handle=HazzureTactical::Handle(list,pawn);
    if(!HazzureTuning::Matches(list,handle,pawn)){status.store(Status::Waiting);return;}
    View native{};if(!ReadView(view,native)){status.store(Status::InvalidView);return;}
    float eyeDistance=std::hypot(std::hypot(native.origin.x-player.eye.x,native.origin.y-player.eye.y),native.origin.z-player.eye.z);
    float maxEyeDist = s.thirdperson ? (s.distance + 300.f) : 128.f;
    if(!std::isfinite(eyeDistance)||eyeDistance>maxEyeDist){status.store(Status::Waiting);return;}
    struct Context{uintptr_t client,pawn;} context{client,pawn};
    auto query=[](void* data,const Vec3& a,const Vec3& b,HazzureWorldTrace::Result& result){auto* c=static_cast<Context*>(data);return HazzureWorldTrace::Native(c->client,c->pawn,a,b,4.f,result);};
    View next{};float distance{};
    if(!Compose(native,s,query,&context,next,distance)){status.store(Status::TraceUnavailable);return;}
    if(!Commit(view,native,next)){status.store(Status::InvalidView);return;}
    actualDistance.store(std::max(0.f,distance));actualRatio.store(next.aspect);status.store(distance<0?Status::AspectOnly:Status::Applied);
    if(s.thirdperson&&distance>=16){bodyPawn.store(pawn);bodyHandle.store(handle);bodyEpoch.store(epoch);bodyAt.store(now,std::memory_order_release);}
    static uint64_t probeLogged{};
    if(s.thirdperson&&now>=probeLogged){probeLogged=now+2000;
        LogDebug("[3P] distance=%.0f pawnPerguntado=%llu viewmodelSuprimido=%llu status=%d",
            actualDistance.load(),bodyCalls.load(),viewSuppressions.load(),static_cast<int>(status.load()));}
}
inline bool DrawLocal(uintptr_t client,uintptr_t entity,bool worldReady,uint64_t epoch,uint64_t now){
    auto at=bodyAt.load(std::memory_order_acquire);if(!worldReady||!at||now<at||now-at>100||epoch!=bodyEpoch.load()||entity!=bodyPawn.load())return false;
    auto pawn=HazzureGame::LocalPawn(client);uintptr_t list{};HazzureGame::Pawn state{};
    return pawn==entity&&HazzureGame::ReadPawn(pawn,state)&&HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)&&
        HazzureTuning::Matches(list,bodyHandle.load(),pawn);
}
}
