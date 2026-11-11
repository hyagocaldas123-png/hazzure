// Client 14191: local player_hurt events (headshots) and player_death (kills).
#pragma once
using FireEvent=bool(__fastcall*)(void*,void*);
static FireEvent originalEvent;
struct AudioToken { uint32_t hash; int padding=-1; const char* debug; };
static uint32_t AudioHash(const char* name) {
    const auto* bytes=reinterpret_cast<const unsigned char*>(name);
    int length=static_cast<int>(strlen(name)),left=length;
    uint32_t hash=0x31415926u^length;
    while(left>=4) { uint32_t k;memcpy(&k,bytes,4);k*=0x5bd1e995;k^=k>>24;k*=0x5bd1e995;hash=hash*0x5bd1e995^k;bytes+=4;left-=4; }
    if(left==3)hash^=bytes[2]<<16;
    if(left>=2)hash^=bytes[1]<<8;
    if(left>=1){hash^=bytes[0];hash*=0x5bd1e995;}
    hash^=hash>>13;hash*=0x5bd1e995;hash^=hash>>15;return hash;
}
static bool IsLocalHeadshot(const char* name,int group,int damage,uintptr_t attacker,uintptr_t victim,uintptr_t local) {
    return name&&strcmp(name,"player_hurt")==0&&group==1&&damage>0&&local&&attacker==local&&victim&&victim!=local;
}
static bool IsLocalKill(const char* name,uintptr_t attacker,uintptr_t victim,uintptr_t local) {
    return name&&strcmp(name,"player_death")==0&&local&&attacker==local&&victim&&victim!=local;
}
static bool AudioAddressesValid() {
    const unsigned char fire[]={0x40,0x53,0x56,0x41,0x54,0x48,0x83,0xec,0x30,0x48,0x8b,0xf2};
    const unsigned char name[]={0x8b,0x41,0x14,0x0f,0xba,0xe0,0x1e,0x73,0x05,0x48,0x8d,0x41,0x18,0xc3};
    const unsigned char controller[]={0x48,0x83,0xec,0x38,0x8b,0x02,0x4c,0x8d,0x44,0x24,0x20};
    const unsigned char integer[]={0x48,0x89,0x5c,0x24};
    const unsigned char floating[]={0x48,0x83,0xec,0x38,0x8b,0x02,0x48,0x83,0xc1,0x58};
    return !memcmp(reinterpret_cast<void*>(client+0x9e3790),fire,sizeof(fire))&&
        !memcmp(reinterpret_cast<void*>(client+0x9e8c80),name,sizeof(name))&&
        !memcmp(reinterpret_cast<void*>(client+0x9e9090),controller,sizeof(controller))&&
        !memcmp(reinterpret_cast<void*>(client+0xaf19a0),integer,sizeof(integer))&&
        !memcmp(reinterpret_cast<void*>(client+0x9e7b70),floating,sizeof(floating))&&
        !memcmp(reinterpret_cast<void*>(client+0x9ea380),floating,sizeof(floating))&&
        Get<uintptr_t>(client+0x1c334f0+9*8)==client+0x9e7b70&&
        Get<uintptr_t>(client+0x1c334f0+10*8)==client+0x9ea380;
}
static void ObserveAudioEvent(void* event) {
    if(!event||shared->audioProtocol!=1)return;
    auto name=reinterpret_cast<const char*(*)(void*)>(client+0x9e8c80)(event);
    if(!name)return;
    auto controller=reinterpret_cast<uintptr_t(*)(void*,const AudioToken*)>(client+0x9e9090);
    uintptr_t local=Get<uintptr_t>(client+HazzureOffsets::Globals::dwLocalPlayerController);
    if(!strcmp(name,"player_hurt")) {
        auto integer=reinterpret_cast<int(*)(void*,const char*,int)>(client+0xaf19a0);
        int group=integer(event,"hitgroup",0),damage=integer(event,"dmg_health",0);
        if(damage<=0)return;
        AudioToken attacker{AudioHash("attacker"),-1,"attacker"},victim{AudioHash("userid"),-1,"userid"};
        uintptr_t attacking=controller(event,&attacker),damaged=controller(event,&victim);
        if(local&&attacking==local&&damaged&&damaged!=local){
            if(group==1)InterlockedIncrement64(&shared->headshots);
            else InterlockedIncrement64(&shared->bodyHits);
        }
    } else if(!strcmp(name,"player_death")) {
        AudioToken attacker{AudioHash("attacker"),-1,"attacker"},victim{AudioHash("userid"),-1,"userid"};
        auto damagedController=controller(event,&victim);
        if(IsLocalKill(name,controller(event,&attacker),damagedController,local)){
            InterlockedIncrement(&shared->kills);
            uintptr_t list{};uint8_t victimTeam{};HazzureGame::Pawn localState{};
            auto localPawn=HazzureGame::LocalPawn(client);
            if(HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)&&
               HazzureGame::Read(damagedController+HazzureOffsets::Schema::m_iTeamNum,victimTeam)&&(victimTeam==2||victimTeam==3)&&
               HazzureGame::ReadPawn(localPawn,localState,false)&&localState.team!=victimTeam)
            {
                AudioToken weaponToken{AudioHash("weapon"),-1,"weapon"};
                const char* weapon=reinterpret_cast<const char*(*)(void*,const AudioToken*)>(client+0x9ea380)(event,&weaponToken);
                int definition=HazzureArmory::KillWeapon(weapon,HazzureUI::ActiveWeaponDefinition(localPawn,list));
                if(definition)HazzureArmory::RecordKill(definition);
            }
        }
    }
}
#include "CombatEvents.h"
static bool AudioEventHook(void* manager,void* event) {
    __try { if(!HazzureState::g_unloading.load())ObserveCombatEvent(event); }
    __except(EXCEPTION_EXECUTE_HANDLER) { InterlockedIncrement64(&shared->faults); }
    __try { ObserveAudioEvent(event); }
    __except(EXCEPTION_EXECUTE_HANDLER) { InterlockedExchange(&shared->audioProtocol,-1);InterlockedIncrement64(&shared->faults); }
    return originalEvent(manager,event);
}
