// Visual history only. No tick rewrites or promise of server lag-compensation rewind.
#pragma once
#include "CombatVisuals.h"
namespace HazzurePoses {
struct Pose {uint32_t handle{};uintptr_t pawn{};uint64_t seen{};HazzureGame::Vec3 origin{};
    std::array<HazzureCombat::Segment,32> bones{};size_t boneCount{};};
struct Track {uint32_t handle{};uintptr_t pawn{};std::array<Pose,16> frames{};size_t count{},next{};uint64_t last{};};
inline std::array<Track,64> tracks{};inline uintptr_t list{};
inline void Clear(){for(auto& track:tracks){track.handle=0;track.pawn=0;track.count=track.next=0;track.last=0;}list=0;}
inline void Store(uintptr_t entityList,const HazzureCombat::Ghost& ghost){
    if(!entityList||!ghost.handle||!ghost.pawn||!ghost.seen||ghost.boneCount>ghost.bones.size()||!HazzureGame::Finite(ghost.origin))return;
    if(list!=entityList){Clear();list=entityList;}
    for(auto& track:tracks)if(track.pawn==ghost.pawn&&track.handle!=ghost.handle){track.handle=0;track.pawn=0;track.count=track.next=0;track.last=0;}
    Track* slot=nullptr;for(auto& t:tracks)if(t.handle==ghost.handle){slot=&t;break;}
    if(!slot)for(auto& t:tracks)if(!t.handle||ghost.seen<t.last||ghost.seen-t.last>250){slot=&t;*slot={};break;}
    if(!slot)return;
    if(slot->pawn!=ghost.pawn||ghost.seen<slot->last){*slot={};slot->handle=ghost.handle;slot->pawn=ghost.pawn;}
    slot->handle=ghost.handle;slot->pawn=ghost.pawn;
    if(slot->last&&ghost.seen-slot->last<12)return;
    auto& frame=slot->frames[slot->next];frame.handle=ghost.handle;frame.pawn=ghost.pawn;frame.seen=ghost.seen;frame.origin=ghost.origin;
    frame.bones=ghost.bones;frame.boneCount=ghost.boneCount;
    slot->next=(slot->next+1)%slot->frames.size();slot->count=std::min(slot->count+1,slot->frames.size());slot->last=ghost.seen;
}
template<class Visitor> inline void Visit(uint64_t now,unsigned window,Visitor visitor){
    window=std::clamp(window,20u,200u);
    for(const auto& track:tracks)for(size_t i=1;i<track.count;i+=2){
        const auto& frame=track.frames[(track.next+track.frames.size()-1-i)%track.frames.size()];
        if(frame.seen&&now>=frame.seen&&now-frame.seen<=window)visitor(frame,1.f-static_cast<float>(now-frame.seen)/window);
    }
}
}
