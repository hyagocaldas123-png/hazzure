// C++ / Gunplay.h / MSVC x64 C++17 / deterministic aiming math and acquisition state
#pragma once
#include "GameData.h"
namespace HazzureGunplay {
using Vec2=HazzureGame::Vec2;
inline float DistanceFov(float base,float metres,float reference,float floor){
    if(!std::isfinite(base)||!std::isfinite(metres)||!std::isfinite(reference)||!std::isfinite(floor))return 0;
    return std::clamp(base,.1f,90.f)*std::clamp(std::max(reference,1.f)/std::max(metres,std::max(reference,1.f)),std::clamp(floor,.05f,1.f),1.f);
}
inline uint32_t Random(uint32_t& state){if(!state)state=0x9e3779b9;state^=state<<13;state^=state>>17;state^=state<<5;return state;}
struct Acquisition {
    uintptr_t target{};uint64_t since{};uint32_t delay{};
    void Reset(){target=0;since=0;delay=0;}
    bool Ready(uintptr_t id,uint64_t now,uint32_t minimum,uint32_t maximum,uint32_t& seed){
        if(!id){Reset();return false;}
        minimum=std::min(minimum,1000u);maximum=std::clamp(maximum,minimum,1000u);
        if(id!=target||now<since){target=id;since=now;delay=minimum+Random(seed)%(maximum-minimum+1);}
        return now>=since&&now-since>=delay;
    }
};
inline Vec2 Point(Vec2 end,float t,float bend){
    t=std::clamp(t,0.f,1.f);float length=std::hypot(end.x,end.y);
    Vec2 perpendicular=length>.00001f?Vec2{-end.y/length,end.x/length}:Vec2{};
    float b=std::clamp(bend,-length*.15f,length*.15f),u=1-t;
    Vec2 p1{end.x*.24f+perpendicular.x*b,end.y*.24f+perpendicular.y*b};
    Vec2 p2{end.x*.76f+perpendicular.x*b*.5f,end.y*.76f+perpendicular.y*b*.5f};
    return {3*u*u*t*p1.x+3*u*t*t*p2.x+t*t*t*end.x,
            3*u*u*t*p1.y+3*u*t*t*p2.y+t*t*t*end.y};
}
// Integral of a smooth velocity envelope. Bias moves the velocity peak while
// keeping velocity and acceleration at zero at both ends of acquisition.
inline float EaseTime(float t,float bias){
    if(!std::isfinite(t)||!std::isfinite(bias))return 0.f;
    t=std::clamp(t,0.f,1.f);bias=std::clamp(bias,.15f,.65f);
    if(t<=bias){float u=t/bias;return bias*(2*u*u*u-u*u*u*u);}
    float u=(1-t)/(1-bias);return 1-(1-bias)*(2*u*u*u-u*u*u*u);
}
inline Vec2 BoundApproach(Vec2 step,Vec2 error){
    float errorSq=error.x*error.x+error.y*error.y,stepSq=step.x*step.x+step.y*step.y;
    float along=step.x*error.x+step.y*error.y;
    if(!std::isfinite(errorSq)||!std::isfinite(stepSq)||!std::isfinite(along)||errorSq<1e-10f||along<=0)return {};
    // Do not pass the plane through the target or increase distance to it.
    float scale=std::min(1.f,std::min(errorSq/along,2.f*along/std::max(stepSq,1e-10f)));
    return {step.x*scale,step.y*scale};
}
struct Curve {
    uintptr_t target{};Vec2 origin{};float elapsed{},duration{},bend{};
    void Reset(){target=0;elapsed=0;}
    Vec2 Step(uintptr_t id,const Vec2& current,const Vec2& error,float smooth,float jitter,float dt,uint32_t& seed,bool eased=false,float easeBias=.35f){
        if(!id||!std::isfinite(dt)||dt<=0||!std::isfinite(error.x)||!std::isfinite(error.y)||
           !std::isfinite(current.x)||!std::isfinite(current.y)||!std::isfinite(smooth)||!std::isfinite(jitter)){Reset();return {};}
        if(target!=id){
            target=id;origin=current;elapsed=0;duration=std::clamp(smooth*.035f,.04f,.6f);
            bend=(static_cast<float>(Random(seed)&0xffff)/32767.5f-1.f)*std::clamp(jitter,0.f,.25f);
        }
        if(elapsed>=duration){float a=HazzureGame::SmoothAlpha(smooth,dt);return {error.x*a,error.y*a};}
        Vec2 end{HazzureGame::NormalizeAngle(current.x+error.x-origin.x),HazzureGame::NormalizeAngle(current.y+error.y-origin.y)};
        elapsed=std::min(duration,elapsed+std::clamp(dt,0.f,.05f));
        float time=elapsed/duration;
        auto p=Point(end,eased?EaseTime(time,easeBias):time,bend);
        Vec2 step{HazzureGame::NormalizeAngle(origin.x+p.x-current.x),HazzureGame::NormalizeAngle(origin.y+p.y-current.y)};
        return eased?BoundApproach(step,error):step;
    }
};
struct Recoil {
    Vec2 previous{},pending{};bool valid{};
    void Reset(){previous={};pending={};valid=false;}
    Vec2 Step(const Vec2& punch,int shots,bool firing,float pitchScale,float yawScale,float smooth,float dt){
        if(!firing||shots<=1||!std::isfinite(punch.x)||!std::isfinite(punch.y)){Reset();return {};}
        if(!valid){previous={};valid=true;}
        pending.x-=(punch.x-previous.x)*2.f*pitchScale;pending.y-=(punch.y-previous.y)*2.f*yawScale;
        previous=punch;float alpha=HazzureGame::SmoothAlpha(smooth,dt);
        Vec2 output{pending.x*alpha,pending.y*alpha};pending.x-=output.x;pending.y-=output.y;return output;
    }
};
struct RecoilSettings {int bulletDelay=2,rampBullets=3;float strength=.94f,pitchScale=1.f,yawScale=1.f,smooth=1.f;};
inline float RecoilGain(int shots,const RecoilSettings& options){
    int delay=std::clamp(options.bulletDelay,0,8),ramp=std::clamp(options.rampBullets,0,8);
    if(shots<=delay||!std::isfinite(options.strength))return 0.f;
    float t=ramp?std::clamp(static_cast<float>(shots-delay)/ramp,0.f,1.f):1.f;
    return std::clamp(options.strength,0.f,1.f)*t*t*(3.f-2.f*t);
}
// One shared sample drives target compensation (offset) and standalone deltas.
// Ignored bullets advance the punch baseline; they are never paid back later.
struct RecoilControl {
    Vec2 previous{},pending{},offset{};uintptr_t weapon{};int lastShots{};bool valid{};
    void Reset(){previous={};pending={};offset={};weapon=0;lastShots=0;valid=false;}
    Vec2 Step(const Vec2& punch,int shots,bool firing,uintptr_t identity,const RecoilSettings& options,float dt){
        if(!firing||!identity||shots<=0||shots>200||!std::isfinite(dt)||dt<=0||dt>.25f||
           !std::isfinite(punch.x)||!std::isfinite(punch.y)||std::fabs(punch.x)>=90.f||std::fabs(punch.y)>=90.f||
           !std::isfinite(options.pitchScale)||!std::isfinite(options.yawScale)||!std::isfinite(options.smooth)){
            Reset();return {};
        }
        if(weapon!=identity||shots<lastShots){Reset();weapon=identity;}
        if(!valid){
            valid=true;weapon=identity;
            // Joining an already-running spray must not apply historical punch.
            if(shots>1){previous=punch;lastShots=shots;return {};}
        }
        float gain=RecoilGain(shots,options);
        pending.x-=(punch.x-previous.x)*2.f*std::clamp(options.pitchScale,0.f,1.5f)*gain;
        pending.y-=(punch.y-previous.y)*2.f*std::clamp(options.yawScale,0.f,1.5f)*gain;
        previous=punch;lastShots=shots;
        float alpha=HazzureGame::SmoothAlpha(options.smooth,dt);
        Vec2 output{pending.x*alpha,pending.y*alpha};
        pending.x-=output.x;pending.y-=output.y;offset.x+=output.x;offset.y+=output.y;
        return output;
    }
};
} // namespace HazzureGunplay
