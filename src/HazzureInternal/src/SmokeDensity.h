// Query the live voxel density. An event sphere can block a ray, but cannot prove it clear.
#pragma once
#include "Visibility.h"
#include "TacticalHUD.h"
#include "GrenadeZones.h"
#include <atomic>
#include <algorithm>
#include "PerformanceMetrics.h"
namespace HazzureSmoke {
using Vec3=HazzureGame::Vec3;
enum class Verdict {Unknown,Clear,Blocked,Unavailable};
struct Request {uintptr_t local{},target{};Vec3 start{},end{};unsigned sample{};};
inline std::atomic<bool> enabled{false};
inline Verdict Classify(float density){
    if(!std::isfinite(density)||density<0||density>1000000)return Verdict::Unavailable;
    return density>0?Verdict::Blocked:Verdict::Clear;
}
inline bool RayIntersectsSmokeVolume(const Vec3& start,const Vec3& end,const Vec3& center,float radius){
    Vec3 d={end.x-start.x,end.y-start.y,end.z-start.z};
    float lineLenSq=d.x*d.x+d.y*d.y+d.z*d.z;
    if(lineLenSq<=0.0001f)return false;
    Vec3 m={start.x-center.x,start.y-center.y,start.z-center.z};
    float b=(m.x*d.x+m.y*d.y+m.z*d.z)/lineLenSq;
    float t=std::clamp(-b,0.0f,1.0f);
    Vec3 closest={start.x+t*d.x,start.y+t*d.y,start.z+t*d.z};
    float distSq=(closest.x-center.x)*(closest.x-center.x)+(closest.y-center.y)*(closest.y-center.y)+(closest.z-center.z)*(closest.z-center.z);
    return distSq<=(radius*radius);
}
inline Verdict FallbackQuery(uint64_t now,const Request& q){
    if(!HazzureGame::Finite(q.start)||!HazzureGame::Finite(q.end))return Verdict::Unavailable;
    auto activeZones=HazzureZones::Snapshot(now);
    for(const auto& zone:activeZones){
        if(zone.kind!=HazzureZones::Kind::Smoke||!zone.start)continue;
        if(!HazzureZones::Alive(zone,now))continue;
        float r=HazzureZones::Radius(HazzureZones::Kind::Smoke);
        if(RayIntersectsSmokeVolume(q.start,q.end,zone.point,r))return Verdict::Blocked;
    }
    return Verdict::Unknown;
}
inline Verdict NativeQuery(uintptr_t client,const Request& q){
    uint64_t now=GetTickCount64();
#ifdef INTERNAL_SELF_TEST
    (void)client;return FallbackQuery(now,q);
#else
    if(!HazzureVisibility::BuildMatches(client)||!q.local||!q.target||!HazzureGame::Finite(q.start)||!HazzureGame::Finite(q.end))return Verdict::Unavailable;
    HazzureGame::Pawn local{},target{};
    if(HazzureGame::LocalPawn(client)!=q.local||!HazzureGame::ReadPawn(q.local,local)||
       !HazzureGame::ReadPawn(q.target,target)||target.team==local.team)return Verdict::Unavailable;
    constexpr unsigned char prefix[]={0x40,0x55,0x53,0x56,0x41,0x55,0x41,0x57,0x48,0x8d,0x6c,0x24,0xe0,0x48,0x81,0xec,0x20,0x01,0,0};
    if(!HazzureVisibility::Prefix(client+0x7ce790,prefix,sizeof(prefix)))return FallbackQuery(now,q);
    uintptr_t head{},node{},visited[128]{};size_t count=0;
    if(!HazzureGame::Read(client+0x24c45b0,head))return FallbackQuery(now,q);
    node=head;
    while(node){
        if(count==128||!HazzureTactical::DesignerName(node,"smokegrenade_projectile"))return FallbackQuery(now,q);
        for(size_t i=0;i<count;++i)if(visited[i]==node)return FallbackQuery(now,q);
        visited[count++]=node;uint8_t effect{},received{};
        if(!HazzureGame::Read(node+0x1364,effect)||effect>1||!HazzureGame::Read(node+0x13a9,received)||received>1)return FallbackQuery(now,q);
        if(effect&&!received)return Verdict::Blocked;
        if(!HazzureGame::Read(node+0x1358,node))return FallbackQuery(now,q);
    }
    float density{};
    using Query=float(__fastcall*)(const Vec3*,const Vec3*,Vec3*);
    __try{density=reinterpret_cast<Query>(client+0x7ce790)(&q.start,&q.end,nullptr);}
    __except(EXCEPTION_EXECUTE_HANDLER){return FallbackQuery(now,q);}
    uintptr_t after{};if(!HazzureGame::Read(client+0x24c45b0,after)||after!=head)return FallbackQuery(now,q);
    Verdict nativeVerdict=Classify(density);
    if(nativeVerdict==Verdict::Unavailable)return FallbackQuery(now,q);
    return nativeVerdict;
#endif
}
struct Entry {Request requested{},evaluated{};Verdict verdict=Verdict::Unknown;uint64_t completed{};bool pending{};};
inline SRWLOCK lock=SRWLOCK_INIT;inline std::array<Entry,128> entries{};inline std::atomic<size_t> cursor{};inline uint64_t generation{};
inline bool Near(Vec3 a,Vec3 b){return std::fabs(a.x-b.x)<=.25f&&std::fabs(a.y-b.y)<=.25f&&std::fabs(a.z-b.z)<=.25f;}
inline void Clear(){AcquireSRWLockExclusive(&lock);entries={};cursor=0;++generation;ReleaseSRWLockExclusive(&lock);}
inline Verdict Submit(const Request& q,uint64_t now){
    if(!enabled.load()||!q.local||!q.target||!HazzureGame::Finite(q.start)||!HazzureGame::Finite(q.end))return Verdict::Unavailable;
    AcquireSRWLockExclusive(&lock);Entry* slot=nullptr;Verdict value=Verdict::Unknown;
    for(auto& e:entries)if(e.requested.local==q.local&&e.requested.target==q.target&&e.requested.sample==q.sample){slot=&e;break;}
    if(!slot)for(auto& e:entries)if(!e.requested.target||(!e.pending&&now>=e.completed&&now-e.completed>100)){slot=&e;*slot={};break;}
    if(slot){slot->requested=q;slot->pending=true;
        if(slot->completed&&now>=slot->completed&&now-slot->completed<=16&&Near(slot->evaluated.start,q.start)&&Near(slot->evaluated.end,q.end))value=slot->verdict;}
    ReleaseSRWLockExclusive(&lock);return value;
}
using Query=Verdict(*)(uintptr_t,const Request&);
inline void ProcessPending(uintptr_t client,Query query=NativeQuery){
    if(!enabled.load()){Clear();return;}
    auto started=HazzurePerformance::Clock();size_t start=cursor.load();unsigned processed=0;
    for(size_t n=0;n<entries.size();++n){size_t i=(start+n)%entries.size();Request q{};bool pending=false;uint64_t observed{};
        AcquireSRWLockExclusive(&lock);observed=generation;if(entries[i].pending){q=entries[i].requested;entries[i].pending=false;pending=true;}ReleaseSRWLockExclusive(&lock);
        if(!pending)continue;auto value=query(client,q);auto now=GetTickCount64();
        AcquireSRWLockExclusive(&lock);auto& e=entries[i];
        if(observed==generation&&e.requested.local==q.local&&e.requested.target==q.target&&e.requested.sample==q.sample){e.evaluated=q;e.verdict=value;e.completed=now;}
        ReleaseSRWLockExclusive(&lock);cursor=(i+1)%entries.size();if(++processed>=32||HazzurePerformance::Budget(started))break;
    }
    HazzurePerformance::End(HazzurePerformance::Stage::Smoke,started);
}
inline bool Allows(Verdict value){return value==Verdict::Clear;}
}
