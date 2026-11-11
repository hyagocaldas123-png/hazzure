// Parent edges verified from installed agent model DATA (26/10/2026).
// root_motion(0), jiggle/weapon/eye/twist helpers are not anatomical ESP links.
#pragma once
#include "GameData.h"
#include <array>
namespace HazzureSkeleton {
using Vec3=HazzureGame::Vec3;
inline constexpr int pelvis=1,spine0=2,spine1=3,spine2=4,spine3=5,neck=6,head=7;
inline constexpr int ankleLeft=19,ankleRight=22;
inline constexpr int links[][2]={
    {7,6},{6,5},{5,4},{4,3},{3,2},{2,1},
    {5,8},{8,9},{9,10},{10,11},
    {5,12},{12,13},{13,14},{14,15},
    {1,17},{17,18},{18,19},
    {1,20},{20,21},{21,22}
};
inline constexpr size_t jointCount=23,linkCount=sizeof(links)/sizeof(links[0]);
struct Segment {Vec3 a{},b{};int from{},to{};};
struct Pose {std::array<Vec3,jointCount> joints{};std::array<bool,jointCount> present{};
    std::array<Segment,linkCount> segments{};size_t count{};bool core{};};
inline float Distance(Vec3 a,Vec3 b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
inline bool Plausible(Vec3 p,Vec3 origin){return HazzureGame::Finite(p)&&HazzureGame::Finite(origin)&&Distance(p,origin)<160.f;}
inline bool Core(Vec3 hip,Vec3 chest,Vec3 skull,Vec3 origin){
    return Plausible(hip,origin)&&Plausible(chest,origin)&&Plausible(skull,origin)&&
        skull.z-origin.z>=12.f&&skull.z-origin.z<=96.f&&skull.z-hip.z>=8.f&&
        Distance(hip,skull)>=10.f&&Distance(hip,skull)<=80.f&&Distance(hip,chest)>=1.f&&
        Distance(hip,chest)<=50.f&&std::hypot(hip.x-origin.x,hip.y-origin.y)<64.f;
}
inline bool LinkValid(int from,int to,Vec3 a,Vec3 b,Vec3 origin){
    if(from<=0||to<=0||from>=static_cast<int>(jointCount)||to>=static_cast<int>(jointCount)||
       !Plausible(a,origin)||!Plausible(b,origin))return false;
    float length=Distance(a,b);if(length<.05f)return false;
    // Core vertebrae cannot span the entire pawn; limbs tolerate crouches/animations.
    float maximum=(from<=7&&to<=7)?24.f:(from>=17||to>=17)?48.f:42.f;
    return length<=maximum;
}
template<class Read> inline Pose Capture(Vec3 origin,Read read){
    Pose out{};
    for(size_t i=1;i<jointCount;++i)if(i!=16)out.present[i]=read(static_cast<int>(i),out.joints[i])&&Plausible(out.joints[i],origin);
    out.core=out.present[pelvis]&&out.present[spine2]&&out.present[head]&&Core(out.joints[pelvis],out.joints[spine2],out.joints[head],origin);
    if(!out.core)return out;
    for(const auto& edge:links){int from=edge[0],to=edge[1];
        if(out.present[from]&&out.present[to]&&LinkValid(from,to,out.joints[from],out.joints[to],origin))
            out.segments[out.count++]={out.joints[from],out.joints[to],from,to};
    }
    return out;
}
struct ImageRect {float left{},top{},right{},bottom{};bool valid{};};
inline ImageRect FitImage(float center,float top,float maximumWidth,float maximumHeight,unsigned width,unsigned height){
    if(!std::isfinite(center)||!std::isfinite(top)||!std::isfinite(maximumWidth)||!std::isfinite(maximumHeight)||
       maximumWidth<=0||maximumHeight<=0||!width||!height)return {};
    float scale=std::min(maximumWidth/static_cast<float>(width),maximumHeight/static_cast<float>(height));
    float w=width*scale,h=height*scale;return {center-w*.5f,top,center+w*.5f,top+h,true};
}
}
