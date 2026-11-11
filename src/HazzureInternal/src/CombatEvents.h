// Included alongside AudioEvents in the native event callback.
#pragma once
#include "CombatVisuals.h"
#include "GrenadeZones.h"
#include "MapContext.h"
#include "CombatRefinements.h"
struct CombatShot {uint32_t actor{};uint64_t time{};HazzureGame::Vec3 eye{};};
static std::array<CombatShot,128> combatShots;
static uintptr_t CombatPawn(uintptr_t list,uintptr_t controller){
    uint32_t handle{};return controller&&HazzureGame::Read(controller+HazzureOffsets::Schema::m_hPlayerPawn,handle)?HazzureTactical::ExactEntity(list,handle):0;
}
static float CombatFloat(void* event,const char* key){
    // Native CGameEvent vtable slot 9 was checked in client+1C334F0.
    AudioToken token{AudioHash(key),-1,key};
    return reinterpret_cast<float(*)(void*,const AudioToken*)>(client+0x9e7b70)(event,&token);
}
static void ObserveCombatEvent(void* event){
    using namespace HazzureCombat;
    if(!event)return;
    const char* name=reinterpret_cast<const char*(*)(void*)>(client+0x9e8c80)(event);
    if(!name)return;
    const auto observedEpoch=roundEpoch.load();
    if(!strcmp(name,"game_newmap")){
        AudioToken mapToken{AudioHash("mapname"),-1,"mapname"};
        const char* map=reinterpret_cast<const char*(*)(void*,const AudioToken*)>(client+0x9ea380)(event,&mapToken);
        if(!HazzureMap::Publish(map))HazzureMap::Clear();
    }
    if(!strcmp(name,"round_start")||!strcmp(name,"round_end")||!strcmp(name,"game_newmap")){
        combatShots={};HazzureZones::Clear();HazzureCombatRefine::ConfirmKill({});roundEpoch.fetch_add(1);inbox.Push({Kind::Reset,GetTickCount64()});return;
    }
    if(HazzureZones::enabled.load()){
        HazzureZones::Kind kind{};bool known=true,end=false;
        if(!strcmp(name,"smokegrenade_detonate"))kind=HazzureZones::Kind::Smoke;
        else if(!strcmp(name,"smokegrenade_expired")){kind=HazzureZones::Kind::SmokeEnd;end=true;}
        else if(!strcmp(name,"inferno_startburn"))kind=HazzureZones::Kind::Fire;
        else if(!strcmp(name,"inferno_expire")){kind=HazzureZones::Kind::FireEnd;end=true;}
        else if(!strcmp(name,"hegrenade_detonate"))kind=HazzureZones::Kind::HE;
        else known=false;
        if(known){auto integer=reinterpret_cast<int(*)(void*,const char*,int)>(client+0xaf19a0);
            HazzureGame::Vec3 position{};if(!end)position={CombatFloat(event,"x"),CombatFloat(event,"y"),CombatFloat(event,"z")};
            HazzureZones::Push({kind,integer(event,"entityid",0),position,GetTickCount64()});}
    }
    unsigned features=enabled.load();if(!features)return;
    uintptr_t list{},localPawn=HazzureGame::LocalPawn(client);HazzureGame::Pawn local{};
    if(!HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)||!list||!HazzureGame::ReadPawn(localPawn,local,false))return;
    auto integer=reinterpret_cast<int(*)(void*,const char*,int)>(client+0xaf19a0);
    auto controller=reinterpret_cast<uintptr_t(*)(void*,const AudioToken*)>(client+0x9e9090);
    AudioToken userid{AudioHash("userid"),-1,"userid"};
    uintptr_t pawn=CombatPawn(list,controller(event,&userid));HazzureGame::Pawn player{};
    bool havePlayer=HazzureGame::ReadPawn(pawn,player,false);
    auto now=GetTickCount64();auto actor=HazzureTactical::Handle(list,pawn);
    bool enemy=havePlayer&&pawn!=localPawn&&player.team!=local.team;
    if((features&Sounds)&&enemy){
        Kind kind{};bool sound=true;
        if(!strcmp(name,"player_footstep"))kind=Kind::Footstep;
        else if(!strcmp(name,"player_jump"))kind=Kind::Jump;
        else if(!strcmp(name,"weapon_reload"))kind=Kind::Reload;
        else if(!strcmp(name,"weapon_fire"))kind=Kind::Fire;
        else sound=false;
        if(sound)inbox.Push({kind,now,actor,0,player.origin});
    }
    if((features&Sounds)&&(!strcmp(name,"door_moving")||!strcmp(name,"door_open"))){
        int index=integer(event,"entindex",0);HazzureGame::Vec3 position{};
        if(index>0&&index<0x8000&&HazzureTactical::Origin(HazzureGame::Entity(list,static_cast<uint32_t>(index)),position))
            inbox.Push({Kind::Door,now,actor,0,position});
    }
    if(!strcmp(name,"player_death")){
        if(HazzureKillFeed::enabled.load()){
            AudioToken atkTok{AudioHash("attacker"),-1,"attacker"};
            AudioToken vicTok{AudioHash("userid"),-1,"userid"};
            AudioToken astTok{AudioHash("assister"),-1,"assister"};
            AudioToken wepTok{AudioHash("weapon"),-1,"weapon"};
            uintptr_t atkCtrl=controller(event,&atkTok);
            uintptr_t vicCtrl=controller(event,&vicTok);
            uintptr_t astCtrl=controller(event,&astTok);
            const char* wepName=reinterpret_cast<const char*(*)(void*,const AudioToken*)>(client+0x9ea380)(event,&wepTok);
            bool hs=integer(event,"headshot",0)!=0;
            HazzureKillFeed::Entry kf{};
            kf.time=now;
            kf.epoch=observedEpoch;
            kf.headshot=hs;
            kf.attacker=static_cast<uint32_t>(atkCtrl);
            kf.victim=static_cast<uint32_t>(vicCtrl);
            kf.assistant=static_cast<uint32_t>(astCtrl);
            uintptr_t localCtrl=Get<uintptr_t>(client+HazzureOffsets::Globals::dwLocalPlayerController);
            kf.local=(localCtrl&&(atkCtrl==localCtrl||vicCtrl==localCtrl));
            if(wepName)strncpy_s(kf.weapon,sizeof(kf.weapon),wepName,_TRUNCATE);
            if(atkCtrl)HazzureGame::ReadBytes(atkCtrl+HazzureOffsets::Schema::m_iszPlayerName,kf.attackerName,sizeof(kf.attackerName)-1);
            if(vicCtrl)HazzureGame::ReadBytes(vicCtrl+HazzureOffsets::Schema::m_iszPlayerName,kf.victimName,sizeof(kf.victimName)-1);
            if(astCtrl)HazzureGame::ReadBytes(astCtrl+HazzureOffsets::Schema::m_iszPlayerName,kf.assistantName,sizeof(kf.assistantName)-1);
            HazzureKillFeed::Push(kf);
        }
    }
    if((features&Kills)&&!strcmp(name,"player_death")&&enemy&&actor){
        AudioToken attacker{AudioHash("attacker"),-1,"attacker"};
        uintptr_t attacking=CombatPawn(list,controller(event,&attacker));
        if(attacking==localPawn){
            auto localHandle=HazzureTactical::Handle(list,localPawn);
            if(localHandle)HazzureCombatRefine::ConfirmKill({localPawn,localHandle,observedEpoch,now,actor});
            auto position=player.origin;position.z+=40.f;
            if(localHandle&&HazzureGame::Finite(position))inbox.Push({Kind::Kill,now,localHandle,actor,position,{},0,true,observedEpoch});
        }
    }
    if((features&DamageText)&&!strcmp(name,"player_hurt")){
        AudioToken attacker{AudioHash("attacker"),-1,"attacker"};
        uintptr_t attacking=CombatPawn(list,controller(event,&attacker));int damage=integer(event,"dmg_health",0);int hitgroup=integer(event,"hitgroup",0);bool isDead=(integer(event,"health",-1)==0);
        if(attacking==localPawn&&enemy&&damage>0&&damage<=10000){
            HazzureUI::ResolverConfirmHit(actor);
            auto head=player.eye;head.z+=8;
            auto bones=HazzureUI::ResolveValidatedBoneArray(player.scene,player.origin);
            HazzureGame::Vec3 bone{};
            if(bones&&HazzureUI::ReadBonePos(bones,HazzureUI::BONE_HEAD,&bone)&&HazzureGame::Finite(bone)){head=bone;head.z+=10;}
            inbox.Push({Kind::Damage,now,HazzureTactical::Handle(list,localPawn),actor,head,{},damage,hitgroup==1||isDead});
        }
    }
    if(!(features&(Tracers|Impacts))||!havePlayer||(!enemy&&pawn!=localPawn)||!actor)return;
    if(!strcmp(name,"weapon_fire")){
        auto& shot=combatShots[actor%combatShots.size()];shot={actor,now,player.eye};
    }else if(!strcmp(name,"bullet_impact")){
        HazzureGame::Vec3 point{CombatFloat(event,"x"),CombatFloat(event,"y"),CombatFloat(event,"z")};
        if(!HazzureGame::Finite(point))return;
        const auto& shot=combatShots[actor%combatShots.size()];
        // Never reconstruct a historical shot from the player's new position.
        if(shot.actor!=actor||now<shot.time||now-shot.time>250)return;
        inbox.Push({Kind::Impact,now,actor,0,point,shot.eye});
    }
}
