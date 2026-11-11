// Pure auto-scope decisions; input is emitted only by the guarded UI integration.
#pragma once
#include <cstdint>
namespace HazzureConvenience {
inline bool ScopedWeapon(unsigned definition){return definition==9||definition==40||definition==11||definition==38;}
inline bool CombatAllowed(bool context,bool lineup){return context&&!lineup;}
enum class TriggerReason {Disabled,Context,Lineup,Key,Manual,Target,Weapon,Scope,SmokeUnknown,SmokeBlocked,Pause,Recoil,Brake,Reaction,Accuracy,Hitchance,Ready,Sent,InputFailed};
inline const char* TriggerLabel(TriggerReason r){switch(r){
    case TriggerReason::Disabled:return "Desativado";case TriggerReason::Context:return "Aguardando partida / foco";
    case TriggerReason::Lineup:return "Lineup controla os comandos";case TriggerReason::Key:return "Aguardando tecla";
    case TriggerReason::Manual:return "Disparo manual";case TriggerReason::Target:return "Sem inimigo sob a mira";
    case TriggerReason::Weapon:return "Arma ou municao indisponivel";case TriggerReason::Scope:return "Aguardando zoom confirmado";
    case TriggerReason::SmokeUnknown:return "Fumaca: dados pendentes / indisponiveis";case TriggerReason::SmokeBlocked:return "Fumaca bloqueia o disparo";
    case TriggerReason::Pause:return "Pausa apos disparo";case TriggerReason::Recoil:return "Aguardando recuperacao do recuo";
    case TriggerReason::Brake:return "Aguardando frenagem";case TriggerReason::Reaction:return "Tempo de reacao";
    case TriggerReason::Accuracy:return "Aguardando dados de precisao / visibilidade";case TriggerReason::Hitchance:return "Hitchance abaixo do limiar";
    case TriggerReason::Ready:return "Pronto";case TriggerReason::Sent:return "Disparo enviado";default:return "Falha ao enviar comando";}}
struct ZoomSettle {
    uintptr_t pawn{},weapon{};uint64_t since{};bool observed{};
    void Reset(){*this={};}
    bool Ready(uintptr_t p,uintptr_t w,bool scoped,uint64_t now){
        if(!p||!w||!scoped){Reset();return false;}
        if(!observed||p!=pawn||w!=weapon||now<since){pawn=p;weapon=w;since=now;observed=true;return false;}
        return now-since>=50;
    }
};
struct ScopeInput {bool enabled{},context{},visible{},scoped{},manualAttack{},manualZoom{};uintptr_t pawn{},weapon{},target{};unsigned definition{};uint64_t now{};};
struct ScopeGate {
    uintptr_t pawn{},weapon{},target{};uint64_t attempted{};bool pending{},latched{};
    void Reset(){*this={};}
    bool Step(const ScopeInput& q){
        if(!q.enabled||!q.context||!q.pawn||!q.weapon||!q.target||!q.visible||!ScopedWeapon(q.definition)){
            Reset();return false;
        }
        if(q.pawn!=pawn||q.weapon!=weapon||q.target!=target){Reset();pawn=q.pawn;weapon=q.weapon;}
        target=q.target;
        if(q.scoped){pending=false;latched=true;return false;}
        if(pending&&(q.now<attempted||q.now-attempted>=750)){pending=false;latched=true;return false;}
        if(q.manualZoom||q.manualAttack)return false;
        // Permitir retentativa de zoom caso latched esteja ativo e ja tenham se passado mais de 500ms
        if(latched && !q.scoped && q.now >= attempted && q.now - attempted >= 500){
            latched = false;
        }
        // One toggle per target acquisition. No repeating zoom levels while the engine catches up.
        if(pending||latched)return false;
        pending=true;attempted=q.now;return true;
    }
    void SendFailed(){pending=false;latched=true;}
    bool BlocksShot(uintptr_t currentPawn,uintptr_t currentWeapon)const{return pending&&pawn==currentPawn&&weapon==currentWeapon;}
};
}
