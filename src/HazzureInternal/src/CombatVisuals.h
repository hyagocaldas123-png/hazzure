// Bounded event transport and world-space history shared by event/render callbacks.
#pragma once
#include "TacticalHUD.h"
#include "VisualGeometry.h"
#include <atomic>
#include <array>
namespace HazzureCombat {
using HazzureGame::Vec3;
enum class Kind { Footstep,Jump,Reload,Door,Damage,Impact,Fire,Reset,Kill };
enum class SoundHeight {Unknown,Level,Above,Below};
inline SoundHeight Height(float source,float listener){
    if(!std::isfinite(source)||!std::isfinite(listener))return SoundHeight::Unknown;
    float difference=source-listener;
    return difference>48?SoundHeight::Above:difference< -48?SoundHeight::Below:SoundHeight::Level;
}
inline const char* HeightLabel(SoundHeight height){
    return height==SoundHeight::Above?"ACIMA":height==SoundHeight::Below?"ABAIXO":height==SoundHeight::Level?"MESMO NIVEL":"ALTURA DESCONHECIDA";
}
inline const char* SoundLabel(Kind kind){
    return kind==Kind::Door?"PORTA":kind==Kind::Reload?"RECARGA":kind==Kind::Jump?"PULO":kind==Kind::Fire?"DISPARO":"PASSO";
}
enum Feature : unsigned { Sounds=1,DamageText=2,Tracers=4,Impacts=8,Kills=16 };
struct Event {Kind kind{};uint64_t time{};uint32_t actor{},target{};Vec3 point{},start{};int damage{};bool lethal{};uint64_t epoch{};};
inline std::atomic<unsigned> enabled{};
inline std::atomic<uint64_t> roundEpoch{};
struct Inbox {
    SRWLOCK lock=SRWLOCK_INIT;std::array<Event,256> events{};size_t size{};uint64_t dropped{};
    void Push(const Event& event){
        if(event.kind!=Kind::Reset&&(!HazzureGame::Finite(event.point)||!HazzureGame::Finite(event.start)))return;
        AcquireSRWLockExclusive(&lock);
        if(size==events.size()){std::move(events.begin()+1,events.end(),events.begin());--size;++dropped;}
        events[size++]=event;ReleaseSRWLockExclusive(&lock);
    }
    size_t Drain(std::array<Event,256>& out){AcquireSRWLockExclusive(&lock);auto n=size;
        std::copy_n(events.begin(),n,out.begin());size=0;ReleaseSRWLockExclusive(&lock);return n;}
    void Clear(){AcquireSRWLockExclusive(&lock);size=0;ReleaseSRWLockExclusive(&lock);}
};
inline Inbox inbox;
inline float Age(uint64_t now,uint64_t then){return now>=then?static_cast<float>(now-then)*.001f:1000.f;}
inline float Fade(float age,float duration){return std::isfinite(age)&&std::isfinite(duration)&&age>=0&&duration>0?std::clamp(1.f-age/duration,0.f,1.f):0.f;}
struct Effects {
    std::array<Event,256> entries{};size_t count{};
    HazzureVisualGeometry::KillGate killGate;
    void Clear(){count=0;killGate.Clear();}
    void Add(const Event& e){
        if(e.kind==Kind::Reset){Clear();return;}
        if(e.kind==Kind::Kill&&(e.epoch!=roundEpoch.load()||!killGate.Accept(e.target,e.actor,e.actor,true,e.epoch)))return;
        if(e.kind==Kind::Footstep)for(size_t i=count;i>0;--i){auto& old=entries[i-1];
            if(old.kind==e.kind&&old.actor==e.actor&&e.time>=old.time&&e.time-old.time<100)return;}
        if(count==entries.size()){std::move(entries.begin()+1,entries.end(),entries.begin());--count;}
        entries[count++]=e;
    }
    void Prune(uint64_t now){size_t kept=0;for(size_t i=0;i<count;++i)if(Age(now,entries[i].time)<3.f)entries[kept++]=entries[i];count=kept;}
};
struct Segment {Vec3 a{},b{};};
struct Ghost {
    uintptr_t pawn{};uint32_t handle{};uint8_t team{};int health{},armor{};uint8_t flagsMask{};float healthShown=-1.f;
    uint16_t weaponDefinition{};int weaponPaint=-1;int ammo=-1,reserve=-1;Vec3 origin{},head{};
    std::array<Segment,32> bones{};size_t boneCount{};char name[128]{},weapon[32]{};uint64_t seen{};bool visited{};
};
struct History {
    std::array<Ghost,64> players{};uintptr_t list{};float simulation{};
    void Clear(){players={};list=0;simulation=0;}
    void Begin(uintptr_t entityList,float clock){
        if(entityList!=list||(std::isfinite(clock)&&clock+.25f<simulation))Clear();
        list=entityList;simulation=clock;for(auto& p:players)p.visited=false;
    }
    void Forget(uint32_t handle){for(auto& p:players)if(p.handle==handle)p={};}
    void Store(const Ghost& value){
        Ghost* slot=nullptr;for(auto& p:players)if(p.handle==value.handle){slot=&p;break;}
        if(!slot)for(auto& p:players)if(!p.handle){slot=&p;break;}
        if(!slot)slot=&*std::min_element(players.begin(),players.end(),[](const Ghost& a,const Ghost& b){return a.seen<b.seen;});
        *slot=value;slot->visited=true;
    }
};
// Camera-space projection keeps enemies behind the camera on the correct side.
inline bool EdgeArrow(const Vec3& origin,const Vec3& eye,float yaw,int width,int height,HazzureGame::Vec2& point,float& angle){
    if(width<80||height<80||!HazzureGame::Finite(origin)||!HazzureGame::Finite(eye)||!std::isfinite(yaw))return false;
    float radians=yaw*.01745329252f,dx=origin.x-eye.x,dy=origin.y-eye.y;
    float right=dx*std::sin(radians)-dy*std::cos(radians),forward=dx*std::cos(radians)+dy*std::sin(radians);
    if(std::hypot(right,forward)<.01f)return false;
    angle=std::atan2(-forward,right);float x=std::cos(angle),y=std::sin(angle);
    float halfW=width*.5f-28.f,halfH=height*.5f-28.f;
    float scale=std::min(halfW/std::max(std::fabs(x),.00001f),halfH/std::max(std::fabs(y),.00001f));
    point={width*.5f+x*scale,height*.5f+y*scale};return true;
}
inline bool ProjectSegment(Vec3 a,Vec3 b,const HazzureGame::Matrix4& matrix,int width,int height,HazzureGame::Vec2& sa,HazzureGame::Vec2& sb){
    if(!HazzureGame::Finite(a)||!HazzureGame::Finite(b))return false;
    auto w=[&](const Vec3& p){const auto& r=matrix.m[3];return r[0]*p.x+r[1]*p.y+r[2]*p.z+r[3];};
    float wa=w(a),wb=w(b);constexpr float clipDepth=.01f;
    if(!std::isfinite(wa)||!std::isfinite(wb)||(wa<clipDepth&&wb<clipDepth))return false;
    auto mix=[](const Vec3& p,const Vec3& q,float t){return Vec3{p.x+(q.x-p.x)*t,p.y+(q.y-p.y)*t,p.z+(q.z-p.z)*t};};
    if(wa<clipDepth)a=mix(a,b,(clipDepth-wa)/(wb-wa));else if(wb<clipDepth)b=mix(b,a,(clipDepth-wb)/(wa-wb));
    if(!HazzureGame::Project(a,matrix,width,height,sa)||!HazzureGame::Project(b,matrix,width,height,sb))return false;
    float dx=sb.x-sa.x,dy=sb.y-sa.y,low=0,high=1;
    auto clip=[&](float p,float q){if(std::fabs(p)<1e-7f)return q>=0;float r=q/p;
        if(p<0){if(r>high)return false;low=std::max(low,r);}else{if(r<low)return false;high=std::min(high,r);}return true;};
    if(!clip(-dx,sa.x)||!clip(dx,width-sa.x)||!clip(-dy,sa.y)||!clip(dy,height-sa.y))return false;
    sb={sa.x+high*dx,sa.y+high*dy};sa={sa.x+low*dx,sa.y+low*dy};return true;
}
inline Effects effects;
inline History history;
inline void Reset(){inbox.Clear();effects.Clear();history.Clear();}
} // namespace HazzureCombat
