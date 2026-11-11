// Read-only CConVarRef values. Declarations recovered from the installed PE's initializers.
#pragma once
#include "Visibility.h"
namespace HazzureCvars {
struct Values {float airAcceleration{12.f},airWishSpeed{30.f},acceleration{5.5f},friction{5.2f},stopSpeed{100.f},gravity{800.f};bool valid{true};};
inline bool Value(uintptr_t client,uintptr_t rva,float minimum,float maximum,float& result){
    uintptr_t data{};float value{};
    constexpr unsigned char getter[]={0x48,0x8b,0x41,0x08,0xf3,0x0f,0x10,0x40,0x58,0xc3};
    uintptr_t getterRva = HazzureVisibility::Build14194Matches(client) ? 0x160c10 : 0x160af0;
    if(!HazzureVisibility::BuildMatches(client)||!HazzureVisibility::Prefix(client+getterRva,getter,sizeof(getter))||
       !HazzureGame::Read(client+rva+8,data)||!data||!HazzureGame::Read(data+0x58,value)||
       !std::isfinite(value)||value<minimum||value>maximum)return false;
    result=value;return true;
}
inline Values Read(uintptr_t client){
    Values v{};
    // Standard competitive CS2 defaults as fallback
    v.airAcceleration = 12.f;
    v.airWishSpeed = 30.f;
    v.acceleration = 5.5f;
    v.friction = 5.2f;
    v.stopSpeed = 100.f;
    v.gravity = 800.f;
    v.valid = true;

    if(HazzureVisibility::Build14194Matches(client)){
        float val{};
        if(Value(client,0x2566aa0,0,10000,val)) v.airAcceleration=val;
        if(Value(client,0x2536600,1,1000,val)) v.airWishSpeed=val;
        if(Value(client,0x2566ab0,0,1000,val)) v.acceleration=val;
        if(Value(client,0x2566ae0,0,100,val)) v.friction=val;
        if(Value(client,0x2566af0,0,1000,val)) v.stopSpeed=val;
        if(Value(client,0x23c1350,0,10000,val)) v.gravity=val;
    }else if(HazzureVisibility::BuildMatches(client)){
        float val{};
        if(Value(client,0x2566e50,0,10000,val)) v.airAcceleration=val;
        if(Value(client,0x2534470,1,1000,val)) v.airWishSpeed=val;
        if(Value(client,0x2566e40,0,1000,val)) v.acceleration=val;
        if(Value(client,0x2566eb0,0,100,val)) v.friction=val;
        if(Value(client,0x2566db0,0,1000,val)) v.stopSpeed=val;
        if(Value(client,0x23bf1c0,0,10000,val)) v.gravity=val;
    }
    return v;
}
}
