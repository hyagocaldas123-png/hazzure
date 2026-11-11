// Scene presets use only the existing, validated sky/exposure/fog/weather channels.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace HazzureWorldAppearance {
struct Settings {
    bool skyActive{},darkWorld{},removeOriginalWeather{},snowParticles{};
    int skyboxIndex{};float skyBrightness=1.f,worldBrightness=1.f,fogDensity{};
    float sky[4]={1,1,1,1},fog[4]={.70f,.75f,.82f,1};
};
struct Preset {const char* name;Settings values;float zenith[3],horizon[3];};
inline constexpr Preset Presets[]={
    {"Original",{false,false,false,false,0,1,1,0,{1,1,1,1},{.70f,.75f,.82f,1}},
        {.39f,.55f,.73f},{.80f,.86f,.91f}},
    {"Noite fria",{true,true,false,false,2,.62f,.65f,.06f,{.58f,.70f,.92f,1},{.18f,.25f,.38f,1}},
        {.07f,.11f,.22f},{.24f,.34f,.49f}},
    {"Entardecer",{true,true,false,false,8,.92f,.88f,.035f,{1,.81f,.66f,1},{.72f,.52f,.40f,1}},
        {.31f,.28f,.46f},{.96f,.60f,.36f}},
    {"Contraste suave",{true,true,true,false,9,.95f,.92f,.02f,{.92f,.97f,1,1},{.70f,.77f,.84f,1}},
        {.33f,.54f,.73f},{.77f,.85f,.90f}},
    {"Neblina fria",{true,true,false,false,0,.80f,.86f,.16f,{.81f,.87f,.94f,1},{.58f,.67f,.76f,1}},
        {.31f,.39f,.48f},{.66f,.74f,.81f}}
};
inline constexpr size_t Count=sizeof(Presets)/sizeof(Presets[0]);
inline bool ColourValid(const float* colour){
    if(!colour)return false;for(unsigned i=0;i<4;++i)if(!std::isfinite(colour[i])||colour[i]<0||colour[i]>1)return false;return true;
}
inline bool Valid(const Settings& v){
    return v.skyboxIndex>=0&&v.skyboxIndex<10&&std::isfinite(v.skyBrightness)&&v.skyBrightness>=.05f&&v.skyBrightness<=2&&
        std::isfinite(v.worldBrightness)&&v.worldBrightness>=.05f&&v.worldBrightness<=1&&std::isfinite(v.fogDensity)&&v.fogDensity>=0&&v.fogDensity<=1&&
        ColourValid(v.sky)&&ColourValid(v.fog);
}
inline bool Select(int index,Settings& output){
    if(index<0||static_cast<size_t>(index)>=Count||!Valid(Presets[index].values))return false;
    output=Presets[index].values;return true;
}
inline bool Near(float a,float b){return std::isfinite(a)&&std::isfinite(b)&&std::fabs(a-b)<.0001f;}
inline bool Matches(const Settings& v,int index){
    if(!Valid(v)||index<0||static_cast<size_t>(index)>=Count)return false;const auto& p=Presets[index].values;
    if(v.skyActive!=p.skyActive||v.darkWorld!=p.darkWorld||v.removeOriginalWeather!=p.removeOriginalWeather||v.snowParticles!=p.snowParticles||v.skyboxIndex!=p.skyboxIndex||
       !Near(v.skyBrightness,p.skyBrightness)||!Near(v.worldBrightness,p.worldBrightness)||!Near(v.fogDensity,p.fogDensity))return false;
    for(unsigned i=0;i<4;++i)if(!Near(v.sky[i],p.sky[i])||!Near(v.fog[i],p.fog[i]))return false;return true;
}
inline bool HeartbeatAlive(int64_t heartbeat,uint64_t now){return heartbeat>0&&now>=static_cast<uint64_t>(heartbeat)&&now-static_cast<uint64_t>(heartbeat)<3000;}
}
