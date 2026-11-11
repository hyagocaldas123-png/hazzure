// Native attachment refresh scheduling; callbacks allow isolated identity tests.
#pragma once
#include <cstdint>
#include <initializer_list>
namespace HazzureArmoryVisual {
enum class Module { StatTrak,NameTag,Keychain };
enum class Status { Pending,Dispatched,WaitingHud,Unavailable,Stale,Fault };
struct Token {
    uintptr_t weapon{},item{},hud{};uint32_t weaponHandle{},hudHandle{};
    int skinRevision{};uint64_t armoryRevision{};bool restoring{};
};
inline bool Same(const Token& a,const Token& b){
    return a.weapon==b.weapon&&a.item==b.item&&a.hud==b.hud&&a.weaponHandle==b.weaponHandle&&a.hudHandle==b.hudHandle&&
        a.skinRevision==b.skinRevision&&a.armoryRevision==b.armoryRevision&&a.restoring==b.restoring;
}
struct State {Token token{};bool started{},worldDone{},hudDone{};uint64_t nextAttempt{};Status status=Status::Pending;};
inline bool OwnershipAllowed(bool restoring,bool hud,bool localOwner,bool held){
    // Restoring a known weapon is also required after drop or owner death.
    // HUD entities may be reused; only the current local binding can be touched.
    return restoring&&!hud?true:localOwner&&held;
}
inline uint32_t CanonicalHandle(uint32_t raw,uint32_t flags){
    if(raw==0xffffffffu)return raw;
    return (((raw>>15)-(flags&1u))<<15)|(raw&0x7fffu);
}
using Available=bool(*)(void*);
using Validate=bool(*)(void*,const Token&,uintptr_t);
using Invoke=bool(*)(void*,uintptr_t,uintptr_t,Module);
struct Calls {void* context{};Available available{};Validate validate{};Invoke invoke{};};
inline Status Refresh(State& state,const Token& token,uint64_t now,const Calls& calls){
    if(!state.started||!Same(state.token,token)){state={};state.token=token;state.started=true;}
    if(state.worldDone&&(!token.hud||state.hudDone)){
        if(token.hud){state.status=Status::Dispatched;return state.status;}
        state.status=Status::WaitingHud;return state.status;
    }
    if(now<state.nextAttempt)return state.status;
    state.nextAttempt=now+200;
    if(!token.weapon||!token.item||!token.weaponHandle||!calls.available||!calls.validate||!calls.invoke||!calls.available(calls.context))
        return state.status=Status::Unavailable;
    auto apply=[&](uintptr_t target){
        for(auto module:{Module::StatTrak,Module::NameTag,Module::Keychain}){
            if(!calls.validate(calls.context,token,target)){state.status=Status::Stale;return false;}
            if(!calls.invoke(calls.context,target,token.item,module)){state.status=Status::Fault;return false;}
        }
        return true;
    };
    if(!state.worldDone){if(!apply(token.weapon))return state.status;state.worldDone=true;}
    if(!token.hud)return state.status=Status::WaitingHud;
    if(!state.hudDone){if(!apply(token.hud))return state.status;state.hudDone=true;}
    return state.status=Status::Dispatched;
}
inline const char* Label(Status status){
    switch(status){case Status::Dispatched:return "native attachment refresh dispatched";
    case Status::WaitingHud:return "waiting for held weapon HUD model";
    case Status::Unavailable:return "attachment backend unavailable";
    case Status::Stale:return "attachment identity/ownership pending";
    case Status::Fault:return "native attachment refresh failed";
    default:return "attachment refresh pending";}
}
} // namespace HazzureArmoryVisual
