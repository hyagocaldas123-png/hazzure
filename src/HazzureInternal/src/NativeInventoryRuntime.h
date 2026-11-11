// Coalesced UI requests. Only the existing engine frame callback consumes them.
#pragma once
#include "NativeInventoryEngine.h"
#include <atomic>
namespace HazzureNativeInventory {
struct EquippedEntry {
    uint64_t localItemID{},nativeItemID{};
    int definition{},paint{};
    bool equippedT{},equippedCT{};
    int slotT{-1},slotCT{-1};
};
struct EquipRequest {
    uint64_t localItemID{};
    int team{};
    int slot{-1};
    bool unequip{};
};
struct RuntimeSnapshot {
    Diagnostic diagnostic{};
    uint64_t requested{},processed{};
    bool enabled{},available{};
    std::array<EquippedEntry,HazzureInventory::Capacity> items{};
    size_t itemCount{};
    uint64_t equipRequested{},equipProcessed{};
    uint64_t lastEquipFailedID{};
    bool lastEquipSucceeded{true};
    uint64_t reasserted{},reassertRejected{},reassertBlocked{};
};
class Runtime;extern Runtime runtime;extern Engine nativeEngine;extern SRWLOCK frameLock;extern std::atomic<bool> backendAvailable;
class Runtime {
    SRWLOCK lock=SRWLOCK_INIT;HazzureInventory::Collection submitted{},desired{};uint64_t requested{},processing{};
    bool enabled{},desiredEnabled{},accepting=true,initialized{},reconcilePending{};std::atomic<DWORD> owner{};Controller controller;RuntimeSnapshot published{};
    std::array<EquipRequest,16> pendingEquips{};size_t pendingEquipCount{};uint64_t equipTicket{};
    uint64_t reassertBlockUntil{};
    std::atomic_flag busy=ATOMIC_FLAG_INIT;
public:
    bool Submit(const HazzureInventory::Collection& collection,bool active){
        if(active&&!HazzureInventory::Valid(collection))return false;
        AcquireSRWLockExclusive(&lock);
        bool ok=accepting&&requested!=(std::numeric_limits<uint64_t>::max)();
        if(ok){submitted=active?collection:HazzureInventory::Collection{};enabled=active;++requested;
            if(!active)pendingEquipCount=0;
            published.enabled=active;published.requested=requested;published.diagnostic.status=Status::Queued;}
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    bool SubmitEquip(uint64_t localItemID,int team,int slot=-1,bool unequip=false){
        if(!localItemID||(team!=2&&team!=3))return false;
        AcquireSRWLockExclusive(&lock);
        bool ok=accepting&&enabled&&pendingEquipCount<pendingEquips.size();
        if(ok){
            pendingEquips[pendingEquipCount++]={localItemID,team,slot,unequip};
            ++equipTicket;
            published.equipRequested=equipTicket;
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    bool SubmitEquipBoth(uint64_t localItemID,int slotT=-1,int slotCT=-1){
        if(!localItemID)return false;
        AcquireSRWLockExclusive(&lock);
        bool ok=accepting&&enabled&&(pendingEquipCount+2)<=pendingEquips.size();
        if(ok){
            pendingEquips[pendingEquipCount++]={localItemID,2,slotT,false};
            pendingEquips[pendingEquipCount++]={localItemID,3,slotCT,false};
            ++equipTicket;
            published.equipRequested=equipTicket;
        }
        ReleaseSRWLockExclusive(&lock);return ok;
    }
    RuntimeSnapshot Read(){AcquireSRWLockShared(&lock);auto out=published;ReleaseSRWLockShared(&lock);return out;}
    // Equip flags live in the persisted collection, but a new session starts with
    // an untouched loadout. Re-apply them once per accepted snapshot so an item
    // equipped in the UI survives restarts; -1 failure, 1 budget exhausted, 0 done.
    int Reconcile(const HazzureInventory::Collection& wanted,Backend& backend,DWORD thread,unsigned budget){
        bool failed=false;
        for(size_t i=0;i<controller.Count();++i){
            const auto* rec=controller.RecordAt(i);
            if(!rec||rec->phase!=Phase::Live)continue;
            const int index=HazzureInventory::Find(wanted,rec->localItemID);
            if(index<0)continue;
            const auto& item=wanted.items[static_cast<size_t>(index)];
            const int definition=NativeDefinition(item);
            for(int team=2;team<=3;++team){
                const bool want=team==2?item.equippedT:item.equippedCT;
                const bool have=team==2?rec->equippedT:rec->equippedCT;
                if(want==have||!TeamAllowedForDefinition(definition,team))continue;
                if(!budget)return 1;
                --budget;
                bool ok=false;
                if(!want)ok=controller.UnequipFromLoadout(item.id,team,thread);
                else{
                    int slot=-1;Session s{};
                    if(backend.session&&backend.session(backend.context,s)&&s.Valid()&&backend.context==&nativeEngine)
                        slot=nativeEngine.ResolveLoadoutSlot(s,rec->nativeItemID,definition,team);
                    else slot=DefaultSlotForDefinition(definition,team);
                    ok=slot>=0&&controller.EquipInLoadout(item.id,team,slot,thread);
                }
                if(!ok)failed=true;
            }
        }
        return failed?-1:0;
    }
    void Tick(Backend backend,DWORD thread,bool suspended){
        if(busy.test_and_set(std::memory_order_acquire))return;
        struct Guard{std::atomic_flag& value;~Guard(){value.clear(std::memory_order_release);}} guard{busy};
        if(!thread||(owner&&owner!=thread))return;
        if(!initialized){if(!controller.Initialize(backend,thread))return;owner=thread;initialized=true;}
        std::array<EquipRequest,16> equipsToRun{};size_t equipCount{};uint64_t currentEquipTicket{};
        AcquireSRWLockExclusive(&lock);accepting=!suspended;
        if(suspended&&enabled){enabled=false;submitted={};pendingEquipCount=0;if(requested!=(std::numeric_limits<uint64_t>::max)())++requested;}
        if(processing!=requested){desired=submitted;desiredEnabled=enabled;processing=requested;reconcilePending=desiredEnabled;}
        auto ticket=processing;
        if(desiredEnabled&&pendingEquipCount){
            equipCount=pendingEquipCount;equipsToRun=pendingEquips;pendingEquipCount=0;currentEquipTicket=equipTicket;
        }
        ReleaseSRWLockExclusive(&lock);
        bool complete=desiredEnabled?controller.Synchronize(desired,thread,2):controller.Teardown(thread,2);
        bool equipOk=published.lastEquipSucceeded;
        uint64_t failedID=published.lastEquipFailedID;
        if(complete&&desiredEnabled&&equipCount){
            equipOk=true;failedID=0;
            for(size_t i=0;i<equipCount;++i){
                const auto& req=equipsToRun[i];
                bool stepOk=false;
                if(req.unequip){
                    stepOk=controller.UnequipFromLoadout(req.localItemID,req.team,thread);
                }else{
                    const auto* rec=controller.Lookup(req.localItemID);
                    int slot=req.slot;
                    if(slot<0&&rec){
                        int def=NativeDefinition(rec->item);
                        Session s{};
                        if(backend.session&&backend.session(backend.context,s)&&s.Valid()&&backend.context==&nativeEngine)
                            slot=nativeEngine.ResolveLoadoutSlot(s,rec->nativeItemID,def,req.team);
                        else slot=DefaultSlotForDefinition(def,req.team);
                    }
                    stepOk=(slot>=0&&controller.EquipInLoadout(req.localItemID,req.team,slot,thread));
                }
                if(!stepOk){equipOk=false;failedID=req.localItemID;}
            }
        }else if(equipCount&&!complete){
            // Synchronization not yet complete; re-queue pending equip requests for next frame.
            AcquireSRWLockExclusive(&lock);
            for(size_t i=0;i<equipCount&&pendingEquipCount<pendingEquips.size();++i)
                pendingEquips[pendingEquipCount++]=equipsToRun[i];
            ReleaseSRWLockExclusive(&lock);
        }
        if(complete&&desiredEnabled&&reconcilePending){
            const int state=Reconcile(desired,backend,thread,4);
            // 1 = budget exhausted, finish on the next frame; failures are already
            // reported through the controller status, so do not retry in a loop.
            if(state!=1)reconcilePending=false;
        }
        // Steady state after the reconcile burst: the engine silently rebuilds a team
        // loadout on respawn, team swap and map change, and nothing re-submits a snapshot
        // when that happens, so the flags can say "equipped" while the slot holds default.
        unsigned reasserted{},reassertRejected{};bool reassertRan=false,reassertBlocked{};
        uint64_t clock=GetTickCount64();
        if(complete&&desiredEnabled&&!reconcilePending){
            if(clock>=reassertBlockUntil){
                reassertRan=true;
                if(controller.ReassertEquipped(thread,2,reasserted,reassertRejected)){
                    if(reassertRejected)reassertBlockUntil=clock+3000;
                }else{reassertBlocked=true;reassertBlockUntil=clock+3000;}
            }else reassertBlocked=true;
        }else reassertBlockUntil=0;
        // Prova da luva no T: o motor resolve a luva pelo loadout do time atual (a
        // referencia que funciona le GetItemInLoadout(m_iTeamNum, 41) e nunca escreve o
        // modelo das maos), entao o que o slot 41 segura em cada time e o fato que separa
        // "loadout T sem luva" de "luva no loadout que nao foi aplicada". Leitura pura na
        // thread dona, logada so quando muda.
        static uint64_t gloveSlotSeen[2]{};static uint64_t gloveIdSeen{};static bool gloveSlotLogged{};
        if(complete&&desiredEnabled){
            Session s{};
            if(backend.session&&backend.session(backend.context,s)&&s.Valid()&&backend.context==&nativeEngine){
                uint64_t heldT{},heldCT{};uint16_t defT{},defCT{};
                bool okT=nativeEngine.ReadLoadoutEntry(s.inventory,2,41,heldT,&defT);
                bool okCT=nativeEngine.ReadLoadoutEntry(s.inventory,3,41,heldCT,&defCT);
                // A colecao guarda mais de uma luva como registro Live (a desequipada
                // continua aqui). A primeira que a sonda pegava era a Slick 5031 do
                // catalogo, entao flagT/slotT/matchT eram mentira; agora escolhe-se a
                // que o loadout realmente segura, e em falta a marcada como equipada.
                uint64_t gloveId{};int slotT{-1},slotCT{-1};bool flagsT{},flagsCT{};
                int liveGloves=0,bestScore=-1;
                for(size_t i=0;i<controller.Count();++i){
                    const auto* rec=controller.RecordAt(i);
                    if(!rec||rec->phase!=Phase::Live||!KnownGlove(NativeDefinition(rec->item)))continue;
                    ++liveGloves;
                    int score=(rec->nativeItemID&&rec->nativeItemID==heldT)||rec->nativeItemID==heldCT?2:((rec->equippedT||rec->equippedCT)?1:0);
                    if(score>bestScore){
                        bestScore=score;gloveId=rec->nativeItemID;slotT=rec->slotT;slotCT=rec->slotCT;
                        flagsT=rec->equippedT;flagsCT=rec->equippedCT;
                    }
                }
                if(!gloveSlotLogged||heldT!=gloveSlotSeen[0]||heldCT!=gloveSlotSeen[1]||gloveId!=gloveIdSeen){
                    gloveSlotLogged=true;gloveSlotSeen[0]=heldT;gloveSlotSeen[1]=heldCT;gloveIdSeen=gloveId;
                    LogDebug("[GLOVESLOT] gloveId=0x%llX flagT=%d flagCT=%d slotT=%d slotCT=%d heldT=0x%llX defT=%u heldCT=0x%llX defCT=%u readOk=%d/%d matchT=%d matchCT=%d liveGloves=%d",
                        static_cast<unsigned long long>(gloveId),static_cast<int>(flagsT),static_cast<int>(flagsCT),slotT,slotCT,
                        static_cast<unsigned long long>(heldT),static_cast<unsigned>(defT),
                        static_cast<unsigned long long>(heldCT),static_cast<unsigned>(defCT),
                        static_cast<int>(okT),static_cast<int>(okCT),
                        static_cast<int>(gloveId&&heldT==gloveId),static_cast<int>(gloveId&&heldCT==gloveId),liveGloves);
                }
            }
        }
        auto diagnostic=controller.Snapshot();
        AcquireSRWLockExclusive(&lock);published.diagnostic=diagnostic;
        if(requested!=ticket)published.diagnostic.status=Status::Queued;
        if(complete&&requested==ticket)published.processed=ticket;
        if(complete&&equipCount){published.equipProcessed=currentEquipTicket;published.lastEquipSucceeded=equipOk;published.lastEquipFailedID=failedID;}
        if(reassertRan){published.reasserted+=reasserted;published.reassertRejected+=reassertRejected;}
        if(reassertBlocked)published.reassertBlocked+=1;
        published.itemCount=controller.Count();
        for(size_t i=0;i<published.itemCount&&i<published.items.size();++i){
            const auto* r=controller.RecordAt(i);
            if(r&&r->phase==Phase::Live){
                published.items[i]={r->localItemID,r->nativeItemID,NativeDefinition(r->item),r->item.skin.paint,r->equippedT,r->equippedCT,r->slotT,r->slotCT};
            }else published.items[i]={};
        }
        published.requested=requested;published.enabled=enabled;ReleaseSRWLockExclusive(&lock);
    }
    DWORD Owner()const{return owner.load(std::memory_order_acquire);}
};
extern Runtime runtime;extern Engine nativeEngine;extern SRWLOCK frameLock;extern std::atomic<bool> backendAvailable;
inline bool Request(const HazzureInventory::Collection& collection){return backendAvailable.load()&&runtime.Submit(collection,true);}
inline bool RequestStop(){return backendAvailable.load()&&runtime.Submit({},false);}
inline bool RequestEquip(uint64_t localItemID,int team,int slot=-1){return backendAvailable.load()&&runtime.SubmitEquip(localItemID,team,slot,false);}
inline bool RequestEquipBoth(uint64_t localItemID,int slotT=-1,int slotCT=-1){return backendAvailable.load()&&runtime.SubmitEquipBoth(localItemID,slotT,slotCT);}
inline bool RequestUnequip(uint64_t localItemID,int team){return backendAvailable.load()&&runtime.SubmitEquip(localItemID,team,-1,true);}
inline RuntimeSnapshot Snapshot(){auto out=runtime.Read();out.available=backendAvailable.load();
    if(!out.available){out.enabled=false;out.diagnostic.status=Status::Unsupported;}return out;}
inline void Frame(uintptr_t client,bool suspended){
    if(!TryAcquireSRWLockExclusive(&frameLock))return;
    struct Guard{~Guard(){ReleaseSRWLockExclusive(&frameLock);}} guard;
    auto thread=GetCurrentThreadId();if(runtime.Owner()&&runtime.Owner()!=thread)return;
    nativeEngine.client=client;runtime.Tick(nativeEngine.Bind(),thread,suspended);
}
}
