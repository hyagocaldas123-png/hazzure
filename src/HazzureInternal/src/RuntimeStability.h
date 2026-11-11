#pragma once
#include "Gunplay.h"
namespace HazzureStable {
using HazzureGame::Vec2;
struct AimTracker {
    uintptr_t target{};Vec2 velocity{};
    void Reset(){target=0;velocity={};}
    Vec2 Step(uintptr_t id,Vec2 error,float smooth,float bias,float dt){
        if(!id||!std::isfinite(error.x)||!std::isfinite(error.y)||!std::isfinite(smooth)||
           !std::isfinite(bias)||!std::isfinite(dt)||dt<=0||dt>.1f){Reset();return {};}
        if(target!=id){Reset();target=id;}
        if(velocity.x*error.x+velocity.y*error.y<0)velocity={};
        float omega=120.f/std::clamp(smooth,1.f,30.f)*(1.f-std::clamp(bias,.15f,.65f)*.5f);
        float decay=std::exp(-omega*dt);Vec2 result{};
        auto axis=[&](float distance,float& speed){
            float term=(speed-omega*distance)*dt;
            float step=distance+(-distance+term)*decay;
            speed=(speed-omega*term)*decay;
            if(step*distance<=0){speed=0;return 0.f;}
            if(std::fabs(step)>std::fabs(distance)){speed=0;return distance;}
            return step;
        };
        result.x=axis(error.x,velocity.x);result.y=axis(error.y,velocity.y);return result;
    }
};
inline float TargetScore(float separation,bool retained){return separation*(retained?.75f:1.f);}
// Require two native observations below the limit. Once stopped, a small band avoids
// reapplying opposite directions for harmless velocity noise. Full Stop is practical rest.
inline constexpr float restSpeed=.5f;
struct StopGate {
    bool ready{};unsigned settled{};
    void Reset(){ready=false;settled=0;}
    bool Step(float speed,float requested){
        if(!std::isfinite(speed)||speed<0||!std::isfinite(requested)||requested<0){Reset();return false;}
        float limit=requested==0?restSpeed:requested;
        if(speed>limit+(ready?std::min(2.f,limit*.1f):0.f)){Reset();return false;}
        if(speed<=limit&&settled<2)++settled;ready=settled>=2;return ready;
    }
};
struct ShotPause {
    uint64_t until{},lastShot{};uintptr_t weapon{};
    void Reset(){until=lastShot=0;weapon=0;}
    bool Ready(uint64_t now,uintptr_t activeWeapon,bool settled){
        if(lastShot&&now<lastShot){Reset();return false;}
        return activeWeapon&&now>=until&&settled;
    }
    void Fired(uint64_t now,uintptr_t activeWeapon,uint32_t milliseconds){
        lastShot=now;weapon=activeWeapon;until=now+std::clamp(milliseconds,80u,1500u);
    }
};
}
