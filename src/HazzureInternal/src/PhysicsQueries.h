// Installed build 14194 / 14192. Native trace and sweep queries.
#pragma once
#include "Visibility.h"
namespace HazzurePhysics {
using HazzureGame::Vec3;
struct Hit {float fraction=-1;Vec3 normal{};uintptr_t entity{},surface{};bool solid{};};
inline bool Box(HazzureVisibility::Ray& shape, Vec3 minimum, Vec3 maximum, Vec3 start = {}, Vec3 end = {}){
    if(!HazzureGame::Finite(minimum,1024)||!HazzureGame::Finite(maximum,1024)||
       minimum.x>maximum.x||minimum.y>maximum.y||minimum.z>maximum.z)return false;
    shape={};
    memcpy(shape.bytes, &start, sizeof(start));
    memcpy(shape.bytes + 0x0c, &end, sizeof(end));
    memcpy(shape.bytes + 0x18, &minimum, sizeof(minimum));
    memcpy(shape.bytes + 0x24, &maximum, sizeof(maximum));
    shape.bytes[0x34] = 2;
    return true;
}
inline bool Valid(const Hit& hit){return std::isfinite(hit.fraction)&&hit.fraction>=0&&hit.fraction<=1&&
    (hit.fraction==1||HazzureGame::Finite(hit.normal,1.1f));}

inline bool Sweep(uintptr_t client,uintptr_t ignore,Vec3 start,Vec3 end,Vec3 minimum,Vec3 maximum,uint64_t mask,Hit& out){
#ifdef INTERNAL_SELF_TEST
    (void)client;(void)ignore;(void)start;(void)end;(void)minimum;(void)maximum;(void)mask;(void)out;return false;
#else
    if(!ignore||!HazzureGame::Finite(start)||!HazzureGame::Finite(end))return false;
    HazzureVisibility::TraceProfile profile{};
    uintptr_t world = 0;
    if(!HazzureVisibility::TraceReady(client, profile, world)) return false;

    HazzureVisibility::Ray shape{};
    if(!Box(shape, minimum, maximum, start, end)) return false;

    HazzureVisibility::Trace trace{}; trace.fraction = -1.0f;
    alignas(16) unsigned char filter[0x110]{};

    __try {
        reinterpret_cast<HazzureVisibility::MakeFilter>(client + profile.filter)(filter, ignore, mask, 3, 15);
        reinterpret_cast<HazzureVisibility::TraceShape>(client + profile.trace)(world, &shape, &start, &end, filter, &trace);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    Hit result{};
    result.fraction = trace.fraction;
    result.solid = trace.solid;
    result.entity = trace.entity;
    result.surface = trace.surface;
    memcpy(&result.normal, reinterpret_cast<const unsigned char*>(&trace) + 0x90, sizeof(result.normal));
    if(!Valid(result)) return false;
    out = result;
    return true;
#endif
}

inline bool CollisionBounds(uintptr_t pawn,Vec3& minimum,Vec3& maximum){
    uintptr_t collision{};
    return HazzureGame::Read(pawn+0x340,collision)&&collision&&HazzureGame::Read(collision+0x40,minimum)&&HazzureGame::Read(collision+0x4c,maximum)&&
        HazzureGame::Finite(minimum,128)&&HazzureGame::Finite(maximum,128)&&minimum.x<maximum.x&&minimum.y<maximum.y&&minimum.z<maximum.z;
}
}
