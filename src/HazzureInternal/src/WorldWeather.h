// World-space precipitation; only geometry-confirmed particles are published.
#pragma once
#include "WorldTrace.h"
#include <array>
namespace HazzureWeather {
using Vec3=HazzureGame::Vec3;
struct Settings {bool enabled{};int kind{},count=96;float windX=18,windY=5,speed=550;};
inline bool Valid(const Settings& s){return s.kind>=0&&s.kind<=2&&s.count>=16&&s.count<=192&&std::isfinite(s.windX)&&std::fabs(s.windX)<=160&&
    std::isfinite(s.windY)&&std::fabs(s.windY)<=160&&std::isfinite(s.speed)&&s.speed>=80&&s.speed<=1000;}
struct Particle {Vec3 point{},checked{};uint64_t visibleUntil{};bool active{},sky{};};
inline bool Visible(const Particle& p,uint64_t now){return p.active&&p.visibleUntil&&now<=p.visibleUntil&&HazzureGame::Finite(p.point);}
using Query=bool(*)(void*,const Vec3&,const Vec3&,HazzureWorldTrace::Result&);
struct System {
    std::array<Particle,192> particles{};Vec3 anchor{};uint64_t time{},session{};size_t cursor{};uint32_t random=0x71ac381d;unsigned queries{};
    void Clear(){particles={};time=0;cursor=0;}
    float Random(){random^=random<<13;random^=random>>17;random^=random<<5;return (random&0xffffff)/16777215.f;}
    void Spawn(Particle& p,const Vec3& eye){p={};p.active=true;p.point={eye.x+(Random()-.5f)*1100,eye.y+(Random()-.5f)*1100,eye.z+80+Random()*600};p.checked=p.point;}
    void Step(const Settings& s,const Vec3& eye,uint64_t now,uint64_t world,Query query,void* context){
        queries=0;float drift=std::hypot(std::hypot(eye.x-anchor.x,eye.y-anchor.y),eye.z-anchor.z);
        if(!s.enabled||!Valid(s)||!HazzureGame::Finite(eye)||session!=world||drift>900||now<time){Clear();session=world;anchor=eye;if(!s.enabled||!Valid(s)||!HazzureGame::Finite(eye))return;}
        float dt=time&&now>=time?std::min((now-time)*.001f,.05f):0;time=now;anchor=eye;
        for(size_t i=0;i<particles.size();++i){auto& p=particles[i];if(i>=static_cast<size_t>(s.count)){p={};continue;}
            if(!p.active||!HazzureGame::Finite(p.point)||p.point.z<eye.z-260||std::hypot(p.point.x-eye.x,p.point.y-eye.y)>900)Spawn(p,eye);
            p.point.x+=s.windX*dt;p.point.y+=s.windY*dt;
            if(s.kind==2){p.point.z+=s.speed*.14f*dt;if(p.point.z>eye.z+500.f)Spawn(p,eye);}
            else p.point.z-=s.speed*(s.kind?.23f:1.f)*dt;
            if(!HazzureGame::Finite(p.point)){Spawn(p,eye);continue;}
        }
        // At most twelve native queries per engine frame. Unknown results never authorize drawing.
        const size_t safeCount=std::clamp(static_cast<size_t>(s.count),size_t(1),size_t(192));
        for(size_t processed=0;processed<32&&queries<12;++processed){
            auto& p=particles[cursor++%safeCount];p.visibleUntil=0;
            if(!p.active||!HazzureGame::Finite(p.point)){p.active=false;continue;}
            HazzureWorldTrace::Result result{};
            if(!p.sky){
                auto top=p.point;top.z+=1800;
                if(!HazzureGame::Finite(top)){p.active=false;continue;}
                if(queries>=12)break;++queries;
                if(!query||!query(context,p.point,top,result)||result.solid||!std::isfinite(result.fraction)||result.fraction<.999f||result.fraction>1.f){p.active=false;continue;}
                p.sky=true;
            }
            if(queries>=12)continue;
            if(!HazzureGame::Finite(p.checked))p.checked=p.point;
            ++queries;if(!query||!query(context,p.checked,p.point,result)||result.solid||!std::isfinite(result.fraction)||result.fraction<.999f||result.fraction>1.f){p.active=false;continue;}p.checked=p.point;
            if(queries>=12)continue;++queries;
            if(query&&HazzureGame::Finite(eye)&&query(context,eye,p.point,result)&&!result.solid&&std::isfinite(result.fraction)&&result.fraction>=.999f&&result.fraction<=1.f)p.visibleUntil=now+350;
        }
    }
};
inline SRWLOCK lock=SRWLOCK_INIT;inline Settings settings;inline std::array<Particle,192> published{};inline uint64_t publishedAt{};
inline std::atomic<unsigned> queryCount{},visibleCount{};
inline void Configure(const Settings& value){AcquireSRWLockExclusive(&lock);settings=Valid(value)?value:Settings{};ReleaseSRWLockExclusive(&lock);}
inline void Clear(){AcquireSRWLockExclusive(&lock);published={};publishedAt=0;ReleaseSRWLockExclusive(&lock);}
inline void Frame(uintptr_t client,bool ready,uint64_t epoch){
    static System system;Settings value;AcquireSRWLockShared(&lock);value=settings;ReleaseSRWLockShared(&lock);
    HazzureGame::Pawn pawn{};auto local=HazzureGame::LocalPawn(client);auto now=GetTickCount64();
    if(!ready||!HazzureGame::ReadPawn(local,pawn)||!HazzureGame::Finite(pawn.eye)){system.Clear();Clear();return;}
    struct Context{uintptr_t client,pawn;};Context context{client,local};
    auto query=[](void* opaque,const Vec3& a,const Vec3& b,HazzureWorldTrace::Result& out){auto* c=static_cast<Context*>(opaque);return HazzureWorldTrace::Native(c->client,c->pawn,a,b,0,out);};
    system.Step(value,pawn.eye,now,epoch,query,&context);
    unsigned count=0;for(const auto& p:system.particles)if(Visible(p,now))++count;
    queryCount.store(system.queries);visibleCount.store(count);AcquireSRWLockExclusive(&lock);published=system.particles;publishedAt=now;ReleaseSRWLockExclusive(&lock);
}
inline std::array<Particle,192> Snapshot(uint64_t now){AcquireSRWLockShared(&lock);auto values=published;auto at=publishedAt;ReleaseSRWLockShared(&lock);
    if(!at||now<at||now-at>500)values={};return values;}
}
