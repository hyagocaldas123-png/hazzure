// Native cooldown is sampled on the engine frame callback; Present consumes a copy.
#pragma once
#include "Visibility.h"
namespace HazzurePistol {
inline bool SemiAutomatic(unsigned definition){
    switch(definition){case 1:case 2:case 3:case 4:case 30:case 32:case 36:case 61:return true;default:return false;}
}
struct Snapshot {
    uintptr_t pawn{},weapon{};uint32_t handle{};uint64_t time{};
    int nextTick{},clip{};float nextFraction{};unsigned definition{};bool valid{},ready{},reload{};
};
inline SRWLOCK lock=SRWLOCK_INIT;inline Snapshot sample;
inline std::atomic<bool> triggerOwned{};
inline void Publish(const Snapshot& s){AcquireSRWLockExclusive(&lock);sample=s;ReleaseSRWLockExclusive(&lock);}
inline bool Valid(const Snapshot& s){return s.valid&&s.pawn&&s.weapon&&s.handle&&SemiAutomatic(s.definition)&&
    s.clip>=0&&s.clip<=100&&s.nextTick>=0&&std::isfinite(s.nextFraction)&&s.nextFraction>=0&&s.nextFraction<1;}
inline Snapshot Read(uintptr_t pawn,uintptr_t weapon,uint32_t handle,uint64_t now){
    AcquireSRWLockShared(&lock);auto s=sample;ReleaseSRWLockShared(&lock);
    return Valid(s)&&s.pawn==pawn&&s.weapon==weapon&&s.handle==handle&&now>=s.time&&now-s.time<=75?s:Snapshot{};
}
// Current installed client: 6ADA7340/29A0000. Registration offsets and getter
// disassembly are recorded in work/requests-20261027/runtime/native-cooldown.txt.
inline void Frame(uintptr_t client,uintptr_t pawn,bool enabled){
#ifdef INTERNAL_SELF_TEST
    (void)client;(void)pawn;(void)enabled;return;
#else
    using namespace HazzureGame;
    Snapshot s{};uintptr_t list{},services{};uint16_t definition{};uint8_t reload=2;
    if(!enabled||!HazzureVisibility::BuildMatches(client)||!pawn||LocalPawn(client)!=pawn||
       !HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||!list||
       !HazzureGame::Read(pawn+HazzureOffsets::Schema::m_pWeaponServices,services)||!services||
       !HazzureGame::Read(services+HazzureOffsets::Schema::m_hActiveWeapon,s.handle)||
       !(s.weapon=Entity(list,s.handle))||!HazzureGame::Read(s.weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex,definition)||
       !SemiAutomatic(definition)){Publish({});return;}
    // REX-prefixed push RBX (40 53), checked against the installed PE bytes.
    constexpr unsigned char readyCode[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0x51,0x10,0x48,0x8b,0xd9,
        0x48,0x8d,0x4c,0x24,0x38,0x8b,0x52,0x38,0xe8,0xc6,0xfd,0x15,0x00};
    if(!HazzureVisibility::Prefix(client+0x94ec90,readyCode,sizeof(readyCode))||
       !HazzureGame::Read(s.weapon+0x1918,s.nextTick)||!HazzureGame::Read(s.weapon+0x191c,s.nextFraction)||
       !HazzureGame::Read(s.weapon+0x1928,s.clip)||!HazzureGame::Read(s.weapon+0x1a3c,reload)||reload>1){Publish({});return;}
    s.pawn=pawn;s.definition=definition;s.reload=reload!=0;s.valid=true;
    if(!Valid(s)){Publish({});return;}
    __try{s.ready=reinterpret_cast<bool(*)(uintptr_t)>(client+0x94ec90)(s.weapon);}
    __except(EXCEPTION_EXECUTE_HANDLER){Publish({});return;}
    if(Entity(list,s.handle)!=s.weapon||LocalPawn(client)!=pawn){Publish({});return;}
    s.time=GetTickCount64();Publish(s);
#endif
}
struct Gate {
    uintptr_t pawn{},weapon{};uint32_t handle{};uint64_t pressed{},released{},last{};
    int nextTick{},clip{};float nextFraction{};bool owned{},down{},consumed{},cleanup{};
    void Failed(){cleanup=true;owned=true;}
    int Step(const Snapshot& s,bool active,bool physical,uint64_t now){
        bool changed=weapon&&(weapon!=s.weapon||pawn!=s.pawn||handle!=s.handle);
        if(cleanup||!active||!Valid(s)||changed||(last&&now<last)){
            bool restore=owned,forceRelease=cleanup||changed||(last&&now<last);
            *this={};return restore?(physical&&!forceRelease?65537:256):-1;
        }
        last=now;pawn=s.pawn;weapon=s.weapon;handle=s.handle;
        if(down){
            if(now-pressed<16)return 65537;
            down=false;released=now;return 256;
        }
        if(owned&&now-released<8)return 256;
        bool advanced=nextTick!=s.nextTick||nextFraction!=s.nextFraction||clip!=s.clip;
        if(s.ready&&!s.reload&&s.clip>0&&(!consumed||advanced)){
            nextTick=s.nextTick;nextFraction=s.nextFraction;clip=s.clip;consumed=true;owned=down=true;pressed=now;return 65537;
        }
        return owned?256:-1;
    }
};
}
