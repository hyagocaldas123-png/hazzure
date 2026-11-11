// C++ / Autowall.h / MSVC x64 C++17 / layered reach decisions, geometry only
// No penetration formula and no surface table: see Visibility.h TracePierce for exactly
// which trace fields this is allowed to rely on, and why they were measured.
#pragma once
#include "Visibility.h"
#include "DebugLogger.h"
#include "PerformanceMetrics.h"
#include <atomic>
#include <array>
#include <cmath>
namespace HazzureAutowall {
using Vec3=HazzureGame::Vec3;
using Verdict=HazzureVisibility::Verdict;
struct Query {uintptr_t local{},target{};Vec3 start{},end{};unsigned sample{};};
struct Answer {Verdict verdict=Verdict::Unknown;unsigned layers{};unsigned rays{};uint32_t contents{};};
inline std::atomic<bool> enabled{false};
inline std::atomic<unsigned> maxLayers{2u};
// Table entries resolved per game-thread pass. One entry already fires several native rays
// inside it, and the pass shares the Visibility performance stage on purpose.
inline constexpr unsigned kQueriesPerFrame=2u;
inline constexpr uint64_t kFreshMs=120;
inline constexpr float kDriftUnits=64.f;
struct Entry {Query requested{},evaluated{};Answer answer{};uint64_t completed{};bool pending{};};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::array<Entry,64> entries{};
inline std::atomic<size_t> cursor{};
inline uint64_t generation{};
inline bool Near(const Vec3& a,const Vec3& b){
    return std::fabs(a.x-b.x)<=kDriftUnits&&std::fabs(a.y-b.y)<=kDriftUnits&&std::fabs(a.z-b.z)<=kDriftUnits;
}
inline void Clear(){AcquireSRWLockExclusive(&lock);entries={};cursor=0;++generation;ReleaseSRWLockExclusive(&lock);}
inline Answer NativeQuery(uintptr_t client,const Query& q){
    Answer out{};
    if(!q.target||q.local==q.target||!HazzureGame::Finite(q.start)||!HazzureGame::Finite(q.end))return out;
    HazzureGame::Pawn local{},target{};
    if(HazzureGame::LocalPawn(client)!=q.local||!HazzureGame::ReadPawn(q.local,local)||
       !HazzureGame::ReadPawn(q.target,target)||local.team==target.team)return out;
    auto pierce=HazzureVisibility::TracePierce(client,q.start,q.end,q.local,q.target,
        std::max(1u,std::min(maxLayers.load(),4u)));
    out.verdict=pierce.verdict;out.layers=pierce.layers;out.rays=pierce.rays;out.contents=pierce.contents;
    return out;
}
// Fill the cache when the game thread reaches a pending entry, and hand the caller the
// last known answer. Unknown means "not evaluated yet", never "safe to shoot".
inline Verdict Submit(const Query& query,uint64_t now,Answer& out){
    out={};
    if(!enabled.load()||!query.local||!query.target||!HazzureGame::Finite(query.start)||!HazzureGame::Finite(query.end))
        return Verdict::Unavailable;
    AcquireSRWLockExclusive(&lock);
    Entry* slot=nullptr;
    for(auto& e:entries)if(e.requested.local==query.local&&e.requested.target==query.target&&e.requested.sample==query.sample){slot=&e;break;}
    if(!slot)for(auto& e:entries)if(!e.requested.target||(!e.pending&&now>=e.completed&&now-e.completed>100)){slot=&e;*slot={};break;}
    Verdict value=Verdict::Unknown;
    if(slot){
        slot->requested=query;slot->pending=true;
        if(slot->completed&&now>=slot->completed&&now-slot->completed<=kFreshMs&&
           Near(slot->evaluated.start,query.start)&&Near(slot->evaluated.end,query.end)){
            out=slot->answer;value=slot->answer.verdict;
        }
    }
    ReleaseSRWLockExclusive(&lock);
    return value;
}
// Both aim paths call this: a point is usable when the line is direct, or when the layered
// probe says the ray ends on the target after crossing no more than the configured layers.
inline bool Allows(Verdict verdict){return verdict==Verdict::Visible||verdict==Verdict::Reached;}
using Probe=Answer(*)(uintptr_t,const Query&);
inline void ProcessPending(uintptr_t client,Probe probe=NativeQuery){
    if(!enabled.load()){Clear();return;}
    auto started=HazzurePerformance::Clock();
    size_t first=cursor.load();unsigned processed=0;
    for(size_t offset=0;offset<entries.size();++offset){
        size_t i=(first+offset)%entries.size();
        Query query{};bool pending=false;uint64_t observed{};
        AcquireSRWLockExclusive(&lock);
        observed=generation;
        if(entries[i].pending){query=entries[i].requested;entries[i].pending=false;pending=true;}
        ReleaseSRWLockExclusive(&lock);
        if(!pending)continue;
        Answer answer=probe(client,query);
        uint64_t now=GetTickCount64();
        AcquireSRWLockExclusive(&lock);
        if(observed==generation&&entries[i].requested.local==query.local&&
           entries[i].requested.target==query.target&&entries[i].requested.sample==query.sample){
            entries[i].evaluated=query;entries[i].answer=answer;entries[i].completed=now;
        }
        ReleaseSRWLockExclusive(&lock);
        cursor=(i+1)%entries.size();
        // One line per decision that changed, throttled: contents is the only clue we have
        // about which surface family is doing the blocking, and that is not measured yet.
        static Verdict lastVerdict=Verdict::Unknown;static unsigned lastLayers=0,lastSample=0xFFFFFFFFu;
        if(answer.verdict!=lastVerdict||answer.layers!=lastLayers||query.sample!=lastSample){
            lastVerdict=answer.verdict;lastLayers=answer.layers;lastSample=query.sample;
            static uint64_t nextReport{};
            if(now>=nextReport){
                nextReport=now+1000;
                LogDebug("[AUTOWALL] sample=%u verdict=%d layers=%u rays=%u contents=0x%08X max=%u",
                    query.sample,static_cast<int>(answer.verdict),answer.layers,answer.rays,answer.contents,
                    maxLayers.load());
            }
        }
        if(++processed>=kQueriesPerFrame||HazzurePerformance::Budget(started))break;
    }
    HazzurePerformance::End(HazzurePerformance::Stage::Visibility,started);
}
}
