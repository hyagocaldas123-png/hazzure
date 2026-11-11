// Event-driven approximate zones. This never replaces smoke visibility or fire-cell geometry.
#pragma once
#include "GameData.h"
#include <array>
#include <atomic>
namespace HazzureZones {
using HazzureGame::Vec3;
enum class Kind {Smoke,Fire,HE,SmokeEnd,FireEnd,Reset};
struct Event {Kind kind{};int id{};Vec3 point{};uint64_t time{};};
struct Zone {Kind kind{};int id{};Vec3 point{};uint64_t start{};};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::atomic<bool> enabled{};
inline std::array<Zone,32> zones{};
inline float Duration(Kind kind){return kind==Kind::Smoke?18.f:kind==Kind::Fire?7.f:kind==Kind::HE?.75f:0;}
inline float Radius(Kind kind){return kind==Kind::Smoke?144.f:kind==Kind::Fire?120.f:kind==Kind::HE?350.f:0;}
inline bool Alive(const Zone& zone,uint64_t now){return zone.start&&now>=zone.start&&now-zone.start<static_cast<uint64_t>(Duration(zone.kind)*1000);}
inline void Clear(){AcquireSRWLockExclusive(&lock);zones={};ReleaseSRWLockExclusive(&lock);}
inline void Push(const Event& e){
    if(e.kind==Kind::Reset){Clear();return;}
    if(!enabled.load()||!e.time||!HazzureGame::Finite(e.point))return;
    AcquireSRWLockExclusive(&lock);
    if(e.kind==Kind::SmokeEnd||e.kind==Kind::FireEnd){
        Kind kind=e.kind==Kind::SmokeEnd?Kind::Smoke:Kind::Fire;
        if(e.id>0)for(auto& zone:zones)if(zone.kind==kind&&zone.id==e.id)zone={};
    }else{
        Zone* slot=nullptr;
        for(auto& zone:zones)if(e.id>0&&zone.id==e.id&&zone.kind==e.kind&&Alive(zone,e.time)){slot=&zone;break;}
        if(!slot)for(auto& zone:zones)if(!Alive(zone,e.time)){slot=&zone;break;}
        if(!slot)slot=&*std::min_element(zones.begin(),zones.end(),[](const Zone& a,const Zone& b){return a.start<b.start;});
        *slot={e.kind,e.id,e.point,e.time};
    }
    ReleaseSRWLockExclusive(&lock);
}
inline std::array<Zone,32> Snapshot(uint64_t now){AcquireSRWLockExclusive(&lock);
    for(auto& zone:zones)if(!Alive(zone,now))zone={};auto out=zones;ReleaseSRWLockExclusive(&lock);return out;}
}
