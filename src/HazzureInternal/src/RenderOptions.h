#pragma once
#include <atomic>
namespace HazzureRenderOptions {
struct Options {bool weapon{},dualLayer{},dropped{},enemyWeapon{},prop{};};
inline std::atomic<unsigned> bits{0};
inline void Publish(bool weapon,bool dualLayer,bool dropped=false,bool enemyWeapon=false,bool prop=false){bits.store((weapon?1u:0u)|(dualLayer?2u:0u)|(dropped?4u:0u)|(enemyWeapon?8u:0u)|(prop?16u:0u),std::memory_order_release);}
inline Options Read(){auto value=bits.load(std::memory_order_acquire);return {(value&1)!=0,(value&2)!=0,(value&4)!=0,(value&8)!=0,(value&16)!=0};}
}
