#pragma once
#include "TacticalHUD.h"
#include "GrenadeZones.h"
namespace HazzureInferno {
using HazzureGame::Vec3;
struct Cells {int id{};uintptr_t entity{};uint32_t handle{};uint64_t time{};std::array<Vec3,64> positions{};size_t count{};float halfWidth{};};
inline SRWLOCK lock=SRWLOCK_INIT;inline std::array<Cells,32> snapshots{};
inline void Clear(){AcquireSRWLockExclusive(&lock);snapshots={};ReleaseSRWLockExclusive(&lock);}
inline bool Validate(const Cells& value){
    if(!value.entity||!value.handle||!value.count||value.count>64||!std::isfinite(value.halfWidth)||value.halfWidth<1||value.halfWidth>128)return false;
    for(size_t i=0;i<value.count;++i)if(!HazzureGame::Finite(value.positions[i]))return false;return true;
}
inline bool Read(uintptr_t client,uintptr_t list,uintptr_t entity,int id,uint64_t now,Cells& out){
    if(!HazzureVisibility::BuildMatches(client)||!HazzureTactical::DesignerName(entity,"inferno"))return false;
    Cells value{};value.id=id;value.entity=entity;value.handle=HazzureTactical::Handle(list,entity);value.time=now;
    int count{};Vec3 origin{};float width{};
    if(!value.handle||!HazzureTactical::Origin(entity,origin)||!HazzureGame::Read(entity+0x1a48,count)||count<1||count>64||
       !HazzureGame::Read(entity+0x866c,width)||!std::isfinite(width)||width<1||width>128)return false;
    value.halfWidth=width;
    for(int i=0;i<count;++i){uint8_t burning{};Vec3 point{};
        if(!HazzureGame::Read(entity+0x1708+i,burning)||burning>1||!HazzureGame::Read(entity+0x1108+i*sizeof(Vec3),point))return false;
        if(!burning)continue;
        // Accept only the installed schema's world positions, never guess relative coordinates.
        if(!HazzureGame::Finite(point)||std::hypot(std::hypot(point.x-origin.x,point.y-origin.y),point.z-origin.z)>2048)return false;
        value.positions[value.count++]=point;
    }
    if(!Validate(value))return false;out=value;return true;
}
inline void Frame(uintptr_t client,const std::array<HazzureZones::Zone,32>& zones){
    uintptr_t list{};auto now=GetTickCount64();std::array<Cells,32> next{};
    if(!HazzureVisibility::BuildMatches(client)||!HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||!list){Clear();return;}
    size_t count=0;
    for(const auto& zone:zones){if(zone.kind!=HazzureZones::Kind::Fire||!zone.start||zone.id<=0||zone.id>0x7fff||count==next.size())continue;
        auto entity=HazzureGame::Entity(list,static_cast<uint32_t>(zone.id));Cells cells{};
        if(Read(client,list,entity,zone.id,now,cells))next[count++]=cells;
    }
    AcquireSRWLockExclusive(&lock);snapshots=next;ReleaseSRWLockExclusive(&lock);
}
inline Cells Snapshot(int id,uint64_t now){AcquireSRWLockShared(&lock);Cells out{};
    for(const auto& value:snapshots)if(value.id==id&&value.time&&now>=value.time&&now-value.time<=100){out=value;break;}
    ReleaseSRWLockShared(&lock);return out;
}
}
