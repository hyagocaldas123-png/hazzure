// Transactional local SO-cache inventory. Mutations belong to the engine frame owner.
#pragma once
#include "LocalInventory.h"
#include <cstddef>
#include <limits>
namespace HazzureNativeInventory {
inline constexpr unsigned k_EEconTypeItem=1;
inline constexpr int k_eSOCacheEvent_Incremental=4;
struct SOID_t {uint64_t steamId{};uint32_t type{},padding{};};
struct CEconItem {
    uintptr_t vtable{},secondary{};uint64_t id{},originalId{};uintptr_t custom{};
    uint32_t account{},inventory{};uint16_t definition{},packed{};
    unsigned char opaque34[12]{};uint8_t flags{};unsigned char tail[7]{};
};
static_assert(sizeof(SOID_t)==16&&sizeof(CEconItem)==0x48,"Native economic object ABI");
static_assert(offsetof(CEconItem,id)==0x10&&offsetof(CEconItem,originalId)==0x18&&offsetof(CEconItem,custom)==0x20,"Native identities");
static_assert(offsetof(CEconItem,account)==0x28&&offsetof(CEconItem,inventory)==0x2c&&offsetof(CEconItem,definition)==0x30&&
    offsetof(CEconItem,packed)==0x32&&offsetof(CEconItem,flags)==0x40,"Native fields");
struct Session {
    uintptr_t inventory{},cache{},typeCache{};SOID_t owner{};
    bool Valid()const{return inventory&&cache&&typeCache&&owner.steamId&&owner.type;}
};
inline bool Same(const Session& a,const Session& b){return a.inventory==b.inventory&&a.cache==b.cache&&a.typeCache==b.typeCache&&
    a.owner.steamId==b.owner.steamId&&a.owner.type==b.owner.type;}
enum class Change {Created,Updated,Destroyed};
enum class Notification {Done,Rejected,Indeterminate};
enum class Status {Idle,Queued,Ready,Pending,Unavailable,Invalid,WrongThread,Capacity,Unsupported,SessionChanged,Indeterminate};
inline const char* Label(Status s){switch(s){case Status::Idle:return "Inventario nativo desligado";case Status::Queued:return "Solicitacao aguardando o frame do jogo";
    case Status::Ready:return "Colecao sincronizada no inventario local do jogo";case Status::Pending:return "Operacao pendente; objetos preservados para recuperacao";
    case Status::Invalid:return "Colecao ou parametro invalido";case Status::WrongThread:return "Aguardando a thread proprietaria do jogo";
    case Status::Capacity:return "Limite do inventario local atingido";case Status::Unsupported:return "Esta operacao nao tem contrato nativo confirmado";
    case Status::SessionChanged:return "Sessao substituida; aguardando reconciliacao segura";
    case Status::Indeterminate:return "Operacao nativa interrompida; estado preservado sem repetir chamadas";
    default:return "Inventario nativo indisponivel nesta sessao/build";}}
// True means the operation completed. Native return values must not be guessed.
struct Backend {
    void* context{};bool (*session)(void*,Session&){};bool (*valid)(void*,const Session&){};
    bool (*preflight)(void*,const Session&,const HazzureInventory::Item&){};
    bool (*limits)(void*,const Session&,uint64_t&,uint32_t&){};bool (*contains)(void*,const Session&,CEconItem*,uint64_t,bool&){};
    CEconItem* (*create)(void*){};bool (*prepare)(void*,const Session&,CEconItem*,const HazzureInventory::Item&,uint64_t,uint32_t){};
    bool (*add)(void*,const Session&,CEconItem*){};bool (*remove)(void*,const Session&,CEconItem*){};
    Notification (*notify)(void*,const Session&,CEconItem*,Change){};bool (*destroy)(void*,CEconItem*){};bool (*faulted)(void*){};
    bool (*loadout)(void*,const Session&,int,int,uint64_t&){};bool (*equip)(void*,const Session&,int,int,uint64_t){};
    bool cacheOwnsObjects{};
    bool Complete()const{return session&&valid&&preflight&&limits&&contains&&create&&prepare&&add&&remove&&notify&&destroy;}
};
inline bool SameItem(const HazzureInventory::Item& a,const HazzureInventory::Item& b){return a.id==b.id&&a.skin.definition==b.skin.definition&&
    a.skin.paint==b.skin.paint&&a.skin.seed==b.skin.seed&&a.skin.model==b.skin.model&&a.skin.wear==b.skin.wear&&a.skin.legacy==b.skin.legacy&&HazzureArmory::SameProfile(a.attributes,b.attributes);}
enum class Phase {Owned,Cached,Live,Removing};
struct Record {
    uint64_t localItemID{},nativeItemID{};uint32_t inventoryID{};CEconItem* object{};HazzureInventory::Item item{};
    Phase phase=Phase::Owned;bool repair{},wasAdded{},cached{},uncertain{},quarantined{},equipped{};int team{},slot{};uint64_t previousLoadout{};
    bool equippedT{},equippedCT{};int slotT{-1},slotCT{-1};uint64_t previousLoadoutT{},previousLoadoutCT{};
};
struct Diagnostic {Status status=Status::Idle;size_t records{},created{},updated{},removed{},rollbacks{},sessionChanges{};uint64_t completed{};};
class Controller {
    Backend backend{};Session current{};DWORD owner{};std::array<Record,HazzureInventory::Capacity> records{};size_t count{};Diagnostic diagnostic{};
    uint64_t lastNativeID{};uint32_t lastInventoryID{};bool unknownAllocation{};
    bool Check(DWORD thread){
        if(!thread||owner!=thread){diagnostic.status=Status::WrongThread;return false;}
        if(unknownAllocation){diagnostic.status=Status::Indeterminate;return false;}
        if(!backend.Complete()){diagnostic.status=Status::Unavailable;return false;}
        Session next{};if(!backend.session(backend.context,next)||!next.Valid()||!backend.valid(backend.context,next)){diagnostic.status=Status::Unavailable;return false;}
        if(current.Valid()&&!Same(current,next)){
            if(count&&!backend.cacheOwnsObjects){diagnostic.status=Status::SessionChanged;return false;}
            // Cached objects belong to the previous cache destructor. Never touch them through a new session.
            for(size_t n=0;n<count;++n){const auto& r=records[n];
                if(r.uncertain||(!r.cached&&(r.wasAdded||r.quarantined))){diagnostic.status=Status::SessionChanged;return false;}
            }
            while(count){auto& r=records[count-1];
                if(!r.cached&&r.object&&!backend.destroy(backend.context,r.object)){
                    if(!Fault(r))diagnostic.status=Status::Pending;return false;}
                Erase(count-1);
            }++diagnostic.sessionChanges;
        }current=next;return true;
    }
    int Find(uint64_t id)const{for(size_t n=0;n<count;++n)if(records[n].localItemID==id)return static_cast<int>(n);return -1;}
    void Erase(size_t n){for(size_t j=n+1;j<count;++j)records[j-1]=records[j];records[--count]={};}
    bool Presence(Record& r,bool& present){return backend.valid(backend.context,current)&&backend.contains(backend.context,current,r.object,r.nativeItemID,present);}
    bool Fault(Record& r){if(backend.faulted&&backend.faulted(backend.context)){r.quarantined=true;diagnostic.status=Status::Indeterminate;return true;}return false;}
    bool Notify(Record& r,Change change){auto outcome=backend.notify(backend.context,current,r.object,change);
        if(outcome==Notification::Indeterminate){r.quarantined=true;diagnostic.status=Status::Indeterminate;return false;}
        return outcome==Notification::Done;}
    void SyncPrimaryEquipped(Record& r){
        if(r.equippedCT){r.equipped=true;r.team=3;r.slot=r.slotCT;r.previousLoadout=r.previousLoadoutCT;}
        else if(r.equippedT){r.equipped=true;r.team=2;r.slot=r.slotT;r.previousLoadout=r.previousLoadoutT;}
        else{r.equipped=false;r.team=0;r.slot=0;r.previousLoadout=0;}
    }
    bool RestoreTeamSlot(Record& r,int team,bool& teamFlag,int& teamSlot,uint64_t& teamPrev){
        if(!teamFlag)return true;
        if(!backend.loadout||!backend.equip)return false;
        uint64_t selected{};
        if(!backend.loadout(backend.context,current,team,teamSlot,selected)){if(Fault(r))return false;return false;}
        if(selected==r.nativeItemID){
            if(!backend.equip(backend.context,current,team,teamSlot,teamPrev)){if(Fault(r))return false;return false;}
        }
        teamFlag=false;teamSlot=-1;teamPrev=0;
        SyncPrimaryEquipped(r);
        return true;
    }
    bool RestoreLoadout(Record& r){
        if(!r.equipped&&!r.equippedT&&!r.equippedCT)return true;
        if(!RestoreTeamSlot(r,2,r.equippedT,r.slotT,r.previousLoadoutT))return false;
        if(!RestoreTeamSlot(r,3,r.equippedCT,r.slotCT,r.previousLoadoutCT))return false;
        SyncPrimaryEquipped(r);
        return true;
    }
    bool Cleanup(size_t n){
        auto& r=records[n];if(r.quarantined){diagnostic.status=Status::Indeterminate;return false;}
        r.phase=Phase::Removing;if(!RestoreLoadout(r)){if(!r.quarantined)diagnostic.status=Status::Pending;return false;}
        bool present{};if(!Presence(r,present)){diagnostic.status=Status::Pending;return false;}
        // A failed post-mutation read never proves that an absent pointer is still ours.
        if(r.uncertain&&!present){diagnostic.status=Status::Pending;return false;}
        // The old cache owner may already have deleted an externally removed object.
        if(r.cached&&!present){Erase(n);++diagnostic.removed;return true;}
        if(present){
            r.cached=true;r.uncertain=false;
            bool removed=backend.remove(backend.context,current,r.object);
            if(Fault(r)){r.uncertain=true;return false;}
            bool remains{};
            if(!Presence(r,remains)){r.uncertain=true;diagnostic.status=Status::Pending;return false;}
            if(!removed||remains){diagnostic.status=Status::Pending;return false;}
            r.cached=false;r.uncertain=false;
        }
        // Object was never cached or already verified detached.
        if(r.wasAdded&&!Notify(r,Change::Destroyed)){if(!r.quarantined)diagnostic.status=Status::Pending;return false;}
        r.wasAdded=false;
        if(!backend.destroy(backend.context,r.object)){
            if(!Fault(r))diagnostic.status=Status::Pending;
            return false;
        }
        // Flags cleared ONLY after successful destroy (ITEM 4 fix)
        r.cached=false;r.uncertain=false;
        Erase(n);++diagnostic.removed;return true;
    }
    bool Create(const HazzureInventory::Item& item){
        if(!backend.preflight(backend.context,current,item)){diagnostic.status=Status::Unsupported;return false;}
        if(count==records.size()){diagnostic.status=Status::Capacity;return false;}uint64_t id{};uint32_t inventory{};
        if(!backend.limits(backend.context,current,id,inventory)){diagnostic.status=Status::Unavailable;return false;}
        id=(std::max)((std::max)(id,uint64_t(0x200000000)),lastNativeID);inventory=(std::max)((std::max)(inventory,uint32_t(1000)),lastInventoryID);
        for(size_t n=0;n<count;++n){id=(std::max)(id,records[n].nativeItemID);inventory=(std::max)(inventory,records[n].inventoryID);}
        if(id>=(std::numeric_limits<uint64_t>::max)()-1||inventory>=0x7fffffffu||((id+1)>>60)){diagnostic.status=Status::Invalid;return false;}
        auto* object=backend.create(backend.context);if(!object){unknownAllocation=backend.faulted&&backend.faulted(backend.context);
            diagnostic.status=unknownAllocation?Status::Indeterminate:Status::Pending;return false;}
        size_t index=count++;auto& r=records[index];r={};r.localItemID=item.id;r.nativeItemID=lastNativeID=id+1;r.inventoryID=lastInventoryID=inventory+1;r.item=item;r.object=object;
        if(!backend.prepare(backend.context,current,object,item,r.nativeItemID,r.inventoryID)){
            ++diagnostic.rollbacks;if(Fault(r))return false;Cleanup(index);if(!r.quarantined)diagnostic.status=Status::Pending;return false;}
        bool added=backend.add(backend.context,current,object),present{};
        if(Fault(r)){r.uncertain=true;return false;}
        if(!Presence(r,present)||(added&&!present)){r.wasAdded=r.uncertain=true;r.phase=Phase::Removing;diagnostic.status=Status::Pending;return false;}
        r.cached=r.wasAdded=present;r.phase=present?Phase::Cached:Phase::Owned;
        if(!added||!present){++diagnostic.rollbacks;Cleanup(index);diagnostic.status=Status::Pending;return false;}
        if(!Notify(r,Change::Created)){++diagnostic.rollbacks;if(r.quarantined)return false;r.phase=Phase::Removing;Cleanup(index);if(!r.quarantined)diagnostic.status=Status::Pending;return false;}
        r.phase=Phase::Live;++diagnostic.created;return true;
    }
    bool Update(size_t n,const HazzureInventory::Item& desired){
        if(!backend.preflight(backend.context,current,desired)){diagnostic.status=Status::Unsupported;return false;}
        auto& r=records[n];bool present{};if(!Presence(r,present)||!present){diagnostic.status=Status::Pending;return false;}
        bool prepared=backend.prepare(backend.context,current,r.object,desired,r.nativeItemID,r.inventoryID);if(Fault(r))return false;
        if(!prepared||!Notify(r,Change::Updated)){
            if(r.quarantined)return false;
            ++diagnostic.rollbacks;r.repair=true;
            bool restored=backend.prepare(backend.context,current,r.object,r.item,r.nativeItemID,r.inventoryID);if(Fault(r))return false;
            if(restored&&Notify(r,Change::Updated))r.repair=false;if(r.quarantined)return false;
            diagnostic.status=Status::Pending;return false;
        }r.item=desired;++diagnostic.updated;return true;
    }
public:
    bool Initialize(Backend value,DWORD thread){if(!thread||!value.Complete()||count||unknownAllocation)return false;backend=value;owner=thread;current={};diagnostic={};return true;}
    Diagnostic Snapshot()const{auto value=diagnostic;value.records=count;return value;}
    size_t Count()const{return count;}
    const Record* RecordAt(size_t index)const{return index<count?&records[index]:nullptr;}
    const Record* Lookup(uint64_t local)const{int i=Find(local);return i<0?nullptr:&records[static_cast<size_t>(i)];}
    bool Synchronize(const HazzureInventory::Collection& desired,DWORD thread,unsigned budget=4){
        if(!HazzureInventory::Valid(desired)||!budget||budget>HazzureInventory::Capacity){diagnostic.status=Status::Invalid;return false;}
        if(!Check(thread))return false;diagnostic.status=Status::Pending;unsigned used{};
        for(size_t n=0;n<count;++n)if(records[n].quarantined){diagnostic.status=Status::Indeterminate;return false;}
        for(size_t n=count;n>0;--n){auto index=n-1;auto& r=records[index];
            if(r.phase==Phase::Removing||HazzureInventory::Find(desired,r.localItemID)<0){if(used++==budget)return false;if(!Cleanup(index))return false;}}
        for(size_t n=0;n<desired.count;++n){const auto& item=desired.items[n];int index=Find(item.id);
            if(index>=0){auto& r=records[static_cast<size_t>(index)];
                if(r.phase==Phase::Live){bool present{};if(!Presence(r,present))return false;
                    if(!present){Erase(static_cast<size_t>(index));if(used++==budget)return false;if(!Create(item))return false;continue;}}
                if(r.repair){if(used++==budget)return false;bool restored=backend.prepare(backend.context,current,r.object,r.item,r.nativeItemID,r.inventoryID);
                    if(Fault(r)||!restored||!Notify(r,Change::Updated))return false;r.repair=false;}
                if(!SameItem(r.item,item)){if(used++==budget)return false;if(!Update(static_cast<size_t>(index),item))return false;}
            }else{if(used++==budget)return false;if(!Create(item))return false;}}
        diagnostic.status=Status::Ready;return true;
    }
    bool RemoveItem(uint64_t local,DWORD thread){if(!Check(thread))return false;int n=Find(local);return n<0||Cleanup(static_cast<size_t>(n));}
    bool EquipInLoadout(uint64_t local,int team,int slot,DWORD thread){
        if(team<2||team>3||slot<0||slot>=58){diagnostic.status=Status::Invalid;return false;}if(!Check(thread))return false;int i=Find(local);
        if(i<0||records[i].phase!=Phase::Live){diagnostic.status=Status::Invalid;return false;}
        if(!backend.loadout||!backend.equip){diagnostic.status=Status::Unsupported;return false;}
        auto& r=records[i];bool present{};if(!Presence(r,present)||!present)return false;
        // If this record is already equipped in another slot on the same team, restore that slot first.
        if(team==2&&r.equippedT&&r.slotT!=slot&&!RestoreTeamSlot(r,2,r.equippedT,r.slotT,r.previousLoadoutT))return false;
        if(team==3&&r.equippedCT&&r.slotCT!=slot&&!RestoreTeamSlot(r,3,r.equippedCT,r.slotCT,r.previousLoadoutCT))return false;
        uint64_t previous{};
        if(!backend.loadout(backend.context,current,team,slot,previous)){Fault(r);return false;}
        // Preserve the original baseline if another custom record currently occupies this team+slot.
        for(size_t n=0;n<count;++n){auto& other=records[n];
            if(team==2&&other.equippedT&&other.slotT==slot&&previous==other.nativeItemID){
                previous=other.previousLoadoutT;
                if(n!=static_cast<size_t>(i)){other.equippedT=false;other.slotT=-1;other.previousLoadoutT=0;SyncPrimaryEquipped(other);}
            }
            if(team==3&&other.equippedCT&&other.slotCT==slot&&previous==other.nativeItemID){
                previous=other.previousLoadoutCT;
                if(n!=static_cast<size_t>(i)){other.equippedCT=false;other.slotCT=-1;other.previousLoadoutCT=0;SyncPrimaryEquipped(other);}
            }
        }
        if(!backend.equip(backend.context,current,team,slot,r.nativeItemID)){Fault(r);return false;}
        if(team==2){r.equippedT=true;r.slotT=slot;r.previousLoadoutT=previous;}
        else{r.equippedCT=true;r.slotCT=slot;r.previousLoadoutCT=previous;}
        SyncPrimaryEquipped(r);return true;
    }
    bool UnequipFromLoadout(uint64_t local,int team,DWORD thread){
        if(team<2||team>3){diagnostic.status=Status::Invalid;return false;}if(!Check(thread))return false;int i=Find(local);
        if(i<0){diagnostic.status=Status::Invalid;return false;}
        auto& r=records[i];
        return team==2?RestoreTeamSlot(r,2,r.equippedT,r.slotT,r.previousLoadoutT):RestoreTeamSlot(r,3,r.equippedCT,r.slotCT,r.previousLoadoutCT);
    }
    // The engine rebuilds loadouts on respawn, team change and map load, so a record's
    // flags say what we asked for, not what the slot holds now. Reading the slot back and
    // re-equipping through EquipInLoadout keeps the restore baseline correct, because that
    // path re-reads whatever the slot carries before writing and preserves another custom
    // record's baseline if it still owns the slot.
    bool ReassertEquipped(DWORD thread,unsigned budget,unsigned& reasserted,unsigned& rejected){
        reasserted=0;rejected=0;
        if(!backend.loadout||!backend.equip||!Check(thread))return false;
        unsigned used{};
        for(size_t n=0;n<count;++n){
            auto& r=records[n];
            if(r.phase!=Phase::Live||r.quarantined||r.uncertain)continue;
            for(int team=2;team<=3;++team){
                const bool flag=team==2?r.equippedT:r.equippedCT;
                const int slot=team==2?r.slotT:r.slotCT;
                if(!flag||slot<0||slot>=58)continue;
                uint64_t holding{};
                if(!backend.loadout(backend.context,current,team,slot,holding)){if(Fault(r))return false;continue;}
                if(holding==r.nativeItemID)continue;
                if(used>=budget)return true;
                ++used;
                if(!EquipInLoadout(r.localItemID,team,slot,thread)){++rejected;if(Fault(r))return false;}
                else ++reasserted;
                break;
            }
        }
        return true;
    }
    bool Teardown(DWORD thread,unsigned budget=8){
        if(unknownAllocation){diagnostic.status=Status::Indeterminate;return false;}
        if(!count){diagnostic.status=Status::Idle;return true;}if(!Check(thread)||!budget)return false;
        unsigned used{};
        for(size_t n=count;n>0&&used<budget;--n,++used)if(!Cleanup(n-1))return false;
        diagnostic.status=count?Status::Pending:Status::Idle;return !count;
    }
};
}
