// Weapon-independent hull queries using the installed build's validated physics ABI.
#pragma once
#include "GrenadePhysics.h"
namespace HazzureWorldTrace {
using Vec3=HazzureGame::Vec3;
struct Result {float fraction=-1;bool solid{};};
inline bool Valid(const Result& r){return std::isfinite(r.fraction)&&r.fraction>=0&&r.fraction<=1;}
inline Vec3 Along(const Vec3& a,const Vec3& b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
inline bool Native(uintptr_t client,uintptr_t pawn,const Vec3& start,const Vec3& end,float radius,Result& out){
#ifdef INTERNAL_SELF_TEST
    (void)client;(void)pawn;(void)start;(void)end;(void)radius;(void)out;return false;
#else
    if(!HazzureVisibility::BuildMatches(client)||!pawn||HazzureGame::LocalPawn(client)!=pawn||
       !HazzureGame::Finite(start)||!HazzureGame::Finite(end)||!std::isfinite(radius)||radius<0||radius>16)return false;
    constexpr unsigned char traceCode[]={0x48,0x89,0x54,0x24,0x10,0x48,0x89,0x4c,0x24,0x08,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57};
    constexpr unsigned char filterCode[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x0f,0xb6,0x41,0x39};
    if(!HazzureVisibility::Prefix(client+0xa1ae30,traceCode,sizeof(traceCode))||!HazzureVisibility::Prefix(client+0x352cc0,filterCode,sizeof(filterCode)))return false;
    uintptr_t world{},table{};if(!HazzureGame::Read(client+0x2229020,world)||!world||!HazzureGame::Read(world,table)||!table)return false;
    alignas(16) unsigned char filter[0x110]{};HazzureVisibility::Ray ray{};HazzureVisibility::Trace result{};result.fraction=-1;
    if(radius>0&&!HazzurePhysics::Box(ray,{-radius,-radius,-radius},{radius,radius,radius}))return false;
    using Filter=void*(*)(void*,uintptr_t,uint64_t,int,uint16_t);
    using Trace=bool(*)(uintptr_t,const HazzureVisibility::Ray*,const Vec3*,const Vec3*,const void*,HazzureVisibility::Trace*);
    __try{
        reinterpret_cast<Filter>(client+0x352cc0)(filter,pawn,HazzureGrenade::collisionMask,3,15);
        reinterpret_cast<Trace>(client+0xa1ae30)(world,&ray,&start,&end,filter,&result);
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    Result value{result.fraction,result.solid};if(!Valid(value))return false;out=value;return true;
#endif
}
}
