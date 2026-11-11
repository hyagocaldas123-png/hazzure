// C++ / Visibility.h / MSVC x64 C++17 / build 14192/14194 physics queries on frame callback
#pragma once
#include "GameData.h"
#include <array>
#include <atomic>
#include "PerformanceMetrics.h"
namespace HazzureVisibility {
using Vec3=HazzureGame::Vec3;
// Reached is only produced by TracePierce: the ray ended on the target after crossing at
// least one surface. Every existing consumer compares against Visible, so fail-closed
// paths (trigger, hitchance, ESP acquisition) keep demanding a direct line.
enum class Verdict { Unknown,Visible,Blocked,Unavailable,Reached };
struct Request {uintptr_t local{},target{};Vec3 start{},end{};unsigned sample{};};
struct Ray {unsigned char bytes[0x40]{};};
struct Trace {
    uintptr_t surface{},entity{};unsigned hitbox{},contents{};unsigned char pad[0x94]{};
    float fraction{};unsigned char tail[0xb]{};bool solid{};unsigned char end[4]{};
};
// contents@0x14 and solid@0xbb are not guesses: on this build TraceShape itself reads
// [r14+0xac] with comiss and cmp byte [r14+0xbb], and the fill helper tests [trace+0x14]
// against 0x40000000. Max byte the engine writes is 0xB7, so 0xC0 really is the size here.
static_assert(offsetof(Trace,hitbox)==0x10&&offsetof(Trace,contents)==0x14&&
    offsetof(Trace,fraction)==0xac&&offsetof(Trace,solid)==0xbb&&sizeof(Trace)==0xc0);
inline Verdict Classify(const Trace& t,uintptr_t target){
    if(!target||!std::isfinite(t.fraction)||t.fraction<0.f||t.fraction>1.f)return Verdict::Unavailable;
    if(t.solid)return Verdict::Blocked;
    if(t.entity==target)return Verdict::Visible;
    return t.entity==0&&t.fraction==1.f?Verdict::Visible:Verdict::Blocked;
}
inline bool CurrentBuild(uintptr_t client){
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    return HazzureGame::Read(client,dos)&&dos.e_magic==IMAGE_DOS_SIGNATURE&&dos.e_lfanew>0&&dos.e_lfanew<0x10000&&
        HazzureGame::Read(client+dos.e_lfanew,nt)&&nt.Signature==IMAGE_NT_SIGNATURE&&
        nt.FileHeader.TimeDateStamp==0x6AEBC4DAu&&nt.OptionalHeader.SizeOfImage==0x29A3000u;
}
inline bool BuildMatches(uintptr_t client){
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    return HazzureGame::Read(client,dos)&&dos.e_magic==IMAGE_DOS_SIGNATURE&&dos.e_lfanew>0&&dos.e_lfanew<0x10000&&
        HazzureGame::Read(client+dos.e_lfanew,nt)&&nt.Signature==IMAGE_NT_SIGNATURE&&
        ((nt.FileHeader.TimeDateStamp==0x6AEBC4DAu&&nt.OptionalHeader.SizeOfImage==0x29A3000u)||
         (nt.FileHeader.TimeDateStamp==0x6ADA7340||nt.FileHeader.TimeDateStamp==0x6AE217A2u||(nt.FileHeader.TimeDateStamp==0x6AE4FB84u||nt.FileHeader.TimeDateStamp==0x6AE91E98u))&&
         (nt.OptionalHeader.SizeOfImage==0x29A0000||nt.OptionalHeader.SizeOfImage==0x29A2000u));
}
// Identity only: legacy consumers keep BuildMatches; each current caller proves its own ABI.
inline bool Build14194Matches(uintptr_t client){
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    return HazzureGame::Read(client,dos)&&dos.e_magic==IMAGE_DOS_SIGNATURE&&dos.e_lfanew>0&&dos.e_lfanew<0x10000&&
        HazzureGame::Read(client+dos.e_lfanew,nt)&&nt.Signature==IMAGE_NT_SIGNATURE&&
        (nt.FileHeader.TimeDateStamp==0x6AE4FB84u||nt.FileHeader.TimeDateStamp==0x6AE91E98u)&&nt.OptionalHeader.SizeOfImage==0x29A2000u;
}
struct TraceProfile {uintptr_t trace{},filter{},world{};};
inline TraceProfile SelectTraceProfile(uintptr_t client){
    if(CurrentBuild(client))return {0xa1b8d0,0x352e20,0x222bfa8};
    if(Build14194Matches(client))return {0xa1b750,0x352de0,0x222afa8};
    if(BuildMatches(client))return {0xa1ae30,0x352cc0,0x2229020};
    return {};
}
inline bool Prefix(uintptr_t address,const unsigned char* expected,size_t n){
    unsigned char bytes[32]{};return n<=sizeof(bytes)&&HazzureGame::ReadBytes(address,bytes,n)&&!memcmp(bytes,expected,n);
}
// 14194 trace/filter + result entity at +8: trace-pointer-callers / trace-entity-rtti evidence.
static const unsigned char kTracePrefix[]={0x48,0x89,0x54,0x24,0x10,0x48,0x89,0x4c,0x24,0x08,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57};
static const unsigned char kFilterPrefix[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x0f,0xb6,0x41,0x39};
using MakeFilter=void*(__fastcall*)(void*,uintptr_t,uint64_t,int,uint16_t);
using TraceShape=bool(__fastcall*)(uintptr_t,const Ray*,const Vec3*,const Vec3*,const void*,Trace*);
// Resolve and gate the native pair once. Nothing may call the engine without both
// 16/20-byte prefixes matching, so a game update turns the whole path off by itself.
inline bool TraceReady(uintptr_t client,TraceProfile& profile,uintptr_t& world){
    profile=SelectTraceProfile(client);world=0;
    if(!profile.trace||!profile.filter||!profile.world)return false;
    if(!Prefix(client+profile.trace,kTracePrefix,sizeof(kTracePrefix))||
       !Prefix(client+profile.filter,kFilterPrefix,sizeof(kFilterPrefix)))return false;
    uintptr_t object{};
    return HazzureGame::Read(client+profile.world,world)&&world&&HazzureGame::Read(world,object)&&object;
}
// One raw ray. The filter block belongs to the caller because a layered probe fires
// several rays per decision and rebuilding 0x110 bytes each time is pure waste.
inline bool TraceSegment(uintptr_t client,const TraceProfile& profile,uintptr_t world,uintptr_t local,
                         const Vec3& start,const Vec3& end,void* filter,Trace& result){
    Ray ray{};result={};result.fraction=-1.f;
    __try{
        reinterpret_cast<MakeFilter>(client+profile.filter)(filter,local,0x1C1003,3,15);
        // The native wrapper initializes Trace and returns false for a clear ray.
        reinterpret_cast<TraceShape>(client+profile.trace)(world,&ray,&start,&end,filter,&result);
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    return std::isfinite(result.fraction)&&result.fraction>=0.f&&result.fraction<=1.f;
}
// One traced ray on the game thread. Both callers prove the same ABI before jumping.
inline Verdict TraceRay(uintptr_t client,const Vec3& start,const Vec3& end,uintptr_t local,uintptr_t target){
    if(!local||!HazzureGame::Finite(start)||!HazzureGame::Finite(end))return Verdict::Unavailable;
    TraceProfile profile{};uintptr_t world{};
    if(!TraceReady(client,profile,world))return Verdict::Unavailable;
    alignas(16) unsigned char filter[0x110]{};Trace result{};
    if(!TraceSegment(client,profile,world,local,start,end,filter,result))return Verdict::Unavailable;
    return Classify(result,target);
}
// Layered reach with only what this build proves: fraction, the solid byte and contents.
// It answers "does the ray end on the target", never "what damage is left" — the
// penetration modifiers behind trace+0x00 are still unmeasured, so they are not used.
// Design constraint: no ray ever starts inside geometry. A step past a surface is only
// accepted when the run from there to the target is clean, which is self-validating and
// sidesteps whatever the engine reports for a start-solid ray.
struct Pierce {
    Verdict verdict=Verdict::Unavailable;unsigned layers{};unsigned rays{};
    uint32_t contents{};float blockedAt{};
};
inline constexpr float kPierceSteps[]={4.f,16.f,64.f};   // wall thickness probed, units
inline constexpr unsigned kPierceMaxRays=10u;
inline Pierce TracePierce(uintptr_t client,const Vec3& start,const Vec3& end,
                          uintptr_t local,uintptr_t target,unsigned maxLayers){
    Pierce out{};
    if(!local||!target||local==target||!HazzureGame::Finite(start)||!HazzureGame::Finite(end))return out;
    TraceProfile profile{};uintptr_t world{};
    if(!TraceReady(client,profile,world))return out;
    alignas(16) unsigned char filter[0x110]{};
    Vec3 dir={end.x-start.x,end.y-start.y,end.z-start.z};
    float total=std::sqrt(dir.x*dir.x+dir.y*dir.y+dir.z*dir.z);
    if(total<2.f){out.verdict=Verdict::Blocked;return out;}
    dir={dir.x/total,dir.y/total,dir.z/total};
    if(maxLayers>4u)maxLayers=4u;
    // Everything is measured along the single start->end line, in units from start, so no
    // ray is ever launched from a point we have not just seen free air at.
    auto pointAt=[&](float along){return Vec3{start.x+dir.x*along,start.y+dir.y*along,start.z+dir.z*along};};
    float cursor=0.f;Trace hit{};bool reuse=false;
    for(unsigned layer=0;layer<=maxLayers;++layer){
        if(!reuse){
            if(out.rays>=kPierceMaxRays){out.verdict=Verdict::Blocked;out.layers=layer;return out;}
            ++out.rays;
            if(!TraceSegment(client,profile,world,local,pointAt(cursor),end,filter,hit)){out.verdict=Verdict::Unavailable;return out;}
        }
        reuse=false;
        out.blockedAt=hit.fraction;
        if(hit.solid){out.verdict=Verdict::Blocked;out.layers=layer;out.contents|=hit.contents;return out;}
        if(hit.entity==target||hit.fraction>=1.f){out.verdict=layer?Verdict::Reached:Verdict::Visible;out.layers=layer;return out;}
        if(hit.fraction<=0.f){out.verdict=Verdict::Blocked;out.layers=layer;out.contents|=hit.contents;return out;}
        float remaining=total-cursor;
        float blocked=cursor+hit.fraction*remaining;
        out.contents|=hit.contents;
        for(float step:kPierceSteps){
            float next=blocked+step;
            if(next>=total-1.f||out.rays>=kPierceMaxRays)break;
            Trace past{};++out.rays;
            if(!TraceSegment(client,profile,world,local,pointAt(next),end,filter,past)){out.verdict=Verdict::Unavailable;return out;}
            if(past.solid)continue;                                  // still buried in the brush
            if(past.entity==target||past.fraction>=1.f){out.verdict=Verdict::Reached;out.layers=layer+1;return out;}
            if(past.fraction<=0.f)break;
            // Identity is useless for map geometry (every brush reports the same entity and
            // contents), so a surface is "the same one" when it sits at the same distance:
            // the step did not clear it. Anything further along is the next layer.
            float pastBlocked=next+past.fraction*(total-next);
            if(pastBlocked<=blocked+1.f)continue;
            cursor=next;hit=past;reuse=true;break;
        }
        if(!reuse){out.verdict=Verdict::Blocked;out.layers=layer;return out;}
    }
    out.verdict=Verdict::Blocked;return out;
}
inline Verdict NativeQuery(uintptr_t client,const Request& q){
    if(!q.target||q.local==q.target)return Verdict::Unavailable;
    HazzureGame::Pawn local{},target{};
    if(HazzureGame::LocalPawn(client)!=q.local||!HazzureGame::ReadPawn(q.local,local)||
       !HazzureGame::ReadPawn(q.target,target)||local.team==target.team)return Verdict::Unavailable;
    return TraceRay(client,q.start,q.end,q.local,q.target);
}
// Sky: the moon has no target entity, so a clean ray may only end in nothing. Handle 1
// never belongs to a real entity index, so Classify returns Visible only through the
// `entity==0 && fraction==1` branch.
inline Verdict SkyQuery(uintptr_t client,const Vec3& start,const Vec3& end){
    uintptr_t local=HazzureGame::LocalPawn(client);
    if(!local)return Verdict::Unavailable;
    return TraceRay(client,start,end,local,1u);
}
// A previous verdict is reused only while it is fresh. The old 2 u geometry
// gate invalidated the cache as soon as either pawn moved, so moving targets
// stayed Unknown forever and aim/trigger (fail-closed) never acquired.
inline constexpr uint64_t kVerdictFreshMs=120;
inline constexpr float kVerdictDriftUnits=64.f;
inline bool Near(const Vec3& a,const Vec3& b){return std::fabs(a.x-b.x)<=kVerdictDriftUnits&&std::fabs(a.y-b.y)<=kVerdictDriftUnits&&std::fabs(a.z-b.z)<=kVerdictDriftUnits;}
struct Entry {Request requested{},evaluated{};Verdict verdict=Verdict::Unknown;uint64_t completed{};bool pending{};};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::array<Entry,128> entries{};
inline std::atomic<size_t> nextEntry{};inline uint64_t generation{};
inline void Clear(){AcquireSRWLockExclusive(&lock);entries={};nextEntry=0;++generation;ReleaseSRWLockExclusive(&lock);}
inline Verdict Submit(const Request& request,uint64_t now){
    AcquireSRWLockExclusive(&lock);
    Entry* slot=nullptr;for(auto& e:entries)if(e.requested.local==request.local&&e.requested.target==request.target&&e.requested.sample==request.sample){slot=&e;break;}
    if(!slot)for(auto& e:entries)if(!e.pending&&(!e.requested.target||now-e.completed>100)){slot=&e;*slot={};break;}
    Verdict value=Verdict::Unknown;
    if(slot){
        slot->requested=request;slot->pending=true;
        if(slot->completed&&now>=slot->completed&&now-slot->completed<=kVerdictFreshMs&&Near(slot->evaluated.start,request.start)&&Near(slot->evaluated.end,request.end))value=slot->verdict;
    }
    ReleaseSRWLockExclusive(&lock);return value;
}
using Query=Verdict(*)(uintptr_t,const Request&);
inline void ProcessPending(uintptr_t client,Query query=NativeQuery){
    auto started=HazzurePerformance::Clock();unsigned processed=0;size_t first=nextEntry.load();
    for(size_t offset=0;offset<entries.size();++offset){size_t i=(first+offset)%entries.size();
        Request request{};bool pending=false;uint64_t observed{};
        AcquireSRWLockExclusive(&lock);
        observed=generation;
        if(entries[i].pending){request=entries[i].requested;entries[i].pending=false;pending=true;}
        ReleaseSRWLockExclusive(&lock);if(!pending)continue;
        auto value=query(client,request);uint64_t now=GetTickCount64();
        AcquireSRWLockExclusive(&lock);
        if(observed==generation&&entries[i].requested.local==request.local&&entries[i].requested.target==request.target&&entries[i].requested.sample==request.sample){
            entries[i].evaluated=request;entries[i].verdict=value;entries[i].completed=now;
        }
        ReleaseSRWLockExclusive(&lock);
        nextEntry=(i+1)%entries.size();if(++processed>=32||HazzurePerformance::Budget(started))break;
    }
    HazzurePerformance::End(HazzurePerformance::Stage::Visibility,started);
}
} // namespace HazzureVisibility
