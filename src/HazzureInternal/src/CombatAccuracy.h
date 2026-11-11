// Bounded, deterministic spread estimate against explicit world-space capsules.
// Two independent uniform disks are a model, not the server's RNG or hit registration.
#pragma once
#include "GameData.h"
#include <array>
namespace HazzureAccuracy {
using HazzureGame::Vec3;
inline Vec3 Add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 Sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 Scale(Vec3 a,float k){return {a.x*k,a.y*k,a.z*k};}
inline float Dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool Unit(Vec3 a,Vec3& result){float n=std::sqrt(Dot(a,a));if(!std::isfinite(n)||n<1e-6f)return false;result=Scale(a,1/n);return true;}
struct Capsule {Vec3 a{},b{};float radius{};int bone{};};
inline bool Valid(const Capsule& c){return HazzureGame::Finite(c.a)&&HazzureGame::Finite(c.b)&&std::isfinite(c.radius)&&c.radius>0&&c.radius<=32;}
// Closest points of two finite segments, including point-like segments and parallel axes.
inline float DistanceSquared(Vec3 p,Vec3 q,Vec3 a,Vec3 b){
    Vec3 d=Sub(q,p),e=Sub(b,a),r=Sub(p,a);float dd=Dot(d,d),ee=Dot(e,e),f=Dot(e,r),s=0,t=0;
    if(dd<=1e-10f&&ee<=1e-10f)return Dot(r,r);
    if(dd<=1e-10f)t=std::clamp(f/ee,0.f,1.f);
    else {float c=Dot(d,r);
        if(ee<=1e-10f)s=std::clamp(-c/dd,0.f,1.f);
        else {float de=Dot(d,e),denom=dd*ee-de*de;
            if(denom>1e-10f)s=std::clamp((de*f-c*ee)/denom,0.f,1.f);
            t=(de*s+f)/ee;
            if(t<0){t=0;s=std::clamp(-c/dd,0.f,1.f);}
            else if(t>1){t=1;s=std::clamp((de-c)/dd,0.f,1.f);}
        }
    }
    auto delta=Sub(Add(p,Scale(d,s)),Add(a,Scale(e,t)));return std::max(0.f,Dot(delta,delta));
}
inline bool Intersects(Vec3 eye,Vec3 direction,float range,const Capsule& c){
    return Valid(c)&&DistanceSquared(eye,Add(eye,Scale(direction,range)),c.a,c.b)<=c.radius*c.radius;
}
inline bool Basis(Vec3 angles,Vec3& forward,Vec3& right,Vec3& up){
    if(!HazzureGame::Finite(angles,36000)||std::fabs(angles.x)>90)return false;
    constexpr float rad=.0174532925199433f;
    float p=angles.x*rad,y=angles.y*rad;
    forward={std::cos(p)*std::cos(y),std::cos(p)*std::sin(y),-std::sin(p)};
    right={std::sin(y),-std::cos(y),0};up={std::sin(p)*std::cos(y),std::sin(p)*std::sin(y),std::cos(p)};return true;
}
inline uint32_t Next(uint32_t& seed){if(!seed)seed=0x9e3779b9u;seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
inline float Uniform(uint32_t& seed){return (static_cast<float>(Next(seed)>>8)+.5f)/16777216.f;}
struct Estimate {bool valid{};unsigned hits{},samples{};float percent{};};
inline Estimate Evaluate(Vec3 eye,Vec3 angles,float inaccuracy,float spread,float range,
                         const Capsule* capsules,size_t count,unsigned samples=128,uint32_t seed=1337){
    Estimate out{};Vec3 forward{},right{},up{};
    if(!HazzureGame::Finite(eye)||!Basis(angles,forward,right,up)||!std::isfinite(inaccuracy)||inaccuracy<0||inaccuracy>1||
       !std::isfinite(spread)||spread<0||spread>1||!std::isfinite(range)||range<=0||range>32768||
       !capsules||!count||count>16||samples<32||samples>256)return out;
    for(size_t i=0;i<count;++i)if(!Valid(capsules[i]))return out;
    out.valid=true;out.samples=samples;
    for(unsigned i=0;i<samples;++i){
        float r1=std::sqrt(Uniform(seed))*inaccuracy,t1=Uniform(seed)*6.28318530718f;
        float r2=std::sqrt(Uniform(seed))*spread,t2=Uniform(seed)*6.28318530718f;
        float x=std::cos(t1)*r1+std::cos(t2)*r2,y=std::sin(t1)*r1+std::sin(t2)*r2;Vec3 ray{};
        if(!Unit(Add(forward,Add(Scale(right,x),Scale(up,y))),ray))return {};
        for(size_t j=0;j<count;++j)if(Intersects(eye,ray,range,capsules[j])){++out.hits;break;}
    }
    out.percent=100.f*static_cast<float>(out.hits)/samples;return out;
}
inline bool Pass(const Estimate& e,float requested){return e.valid&&std::isfinite(requested)&&requested>=0&&requested<=100&&e.percent>=requested;}
struct Points {std::array<Vec3,7> values{};size_t count{};};
inline Points Multipoints(const Capsule& c,Vec3 eye,float fraction){
    Points out{};if(!Valid(c)||!HazzureGame::Finite(eye)||!std::isfinite(fraction)||fraction<0||fraction>.8f)return out;
    Vec3 center=Scale(Add(c.a,c.b),.5f);out.values[out.count++]=center;
    if(fraction==0)return out;
    Vec3 toward{},right{};if(!Unit(Sub(center,eye),toward)||!Unit({toward.y,-toward.x,0},right))right={0,1,0};
    Vec3 up{};if(!Unit({right.y*toward.z,-right.x*toward.z,right.x*toward.y-right.y*toward.x},up))up={0,0,1};
    for(auto axis:{right,up})for(float sign:{-1.f,1.f})out.values[out.count++]=Add(center,Scale(axis,sign*c.radius*fraction));
    out.values[out.count++]=c.a;out.values[out.count++]=c.b;return out;
}
struct StopCorrection {bool valid{},brake{};float forward{},side{},speed{};};
inline StopCorrection CounterStrafe(Vec3 velocity,float yaw,float limit){
    StopCorrection out{};
    if(!HazzureGame::Finite(velocity,20000)||!std::isfinite(yaw)||!std::isfinite(limit)||limit<0||limit>350)return out;
    out.valid=true;out.speed=std::hypot(velocity.x,velocity.y);out.brake=out.speed>limit;
    if(!out.brake||out.speed<1e-5f)return out;
    float angle=yaw*.0174532925199433f;
    out.forward=-(velocity.x*std::cos(angle)+velocity.y*std::sin(angle))/out.speed;
    out.side=-(velocity.x*std::sin(angle)-velocity.y*std::cos(angle))/out.speed;return out;
}
}
