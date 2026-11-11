// Exercises production Controller APIs against injected backend failures, never the game.
#define INTERNAL_SELF_TEST
#define WIN32_LEAN_AND_MEAN
#include "../src/NativeInventoryRuntime.h"
#include <vector>
#include <cstdio>
#include <memory>
#include <thread>
using namespace HazzureNativeInventory;
static unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return false;}}while(0)
struct Fixture {
    Controller controller;Session session{1,2,3,{76561198000000001ULL,1,0}};bool available=true;
    std::vector<CEconItem*> cache,allocated;size_t created{},destroyed{},prepares{},notifications{},adds{},removes{};
    int failPrepare{},failAdd{},failRemove{},failNotify{},failDestroy{};bool falseAddButInserted{},stickyRemove{};
    size_t presenceCalls{},failPresenceAt{};
    int faultStage{};bool faultAfter{},faulted{};size_t nativeEffects{},destroyCalls{};
    uint64_t maxID{},loadout=33;uint32_t maxInventory{};HazzureInventory::Item last{};
    static bool Fail(int& n){if(n){--n;return true;}return false;}
    Backend backend;
    Fixture(){
        backend.context=this;backend.cacheOwnsObjects=true;
        backend.session=[](void* p,Session& s){auto& f=*static_cast<Fixture*>(p);s=f.session;return f.available;};
        backend.preflight=[](void*,const Session&,const HazzureInventory::Item& item){return HazzureInventory::Valid(item)&&KnownDefinition(item);};
        backend.valid=[](void* p,const Session& s){auto& f=*static_cast<Fixture*>(p);return f.available&&Same(s,f.session);};
        backend.limits=[](void* p,const Session&,uint64_t& i,uint32_t& v){auto& f=*static_cast<Fixture*>(p);i=f.maxID;v=f.maxInventory;
            for(auto* o:f.cache){i=(std::max)(i,o->id);v=(std::max)(v,o->inventory);}return true;};
        backend.contains=[](void* p,const Session&,CEconItem* o,uint64_t id,bool& found){auto& f=*static_cast<Fixture*>(p);
            if(++f.presenceCalls==f.failPresenceAt)return false;
            found=std::find(f.cache.begin(),f.cache.end(),o)!=f.cache.end()&&o->id==id;return true;};
        backend.create=[](void* p){auto& f=*static_cast<Fixture*>(p);++f.created;auto* o=new CEconItem;f.allocated.push_back(o);return o;};
        backend.prepare=[](void* p,const Session&,CEconItem* o,const HazzureInventory::Item& item,uint64_t id,uint32_t inventory){
            auto& f=*static_cast<Fixture*>(p);f.faulted=false;++f.prepares;if(f.faultStage==1&&!f.faultAfter){f.faulted=true;return false;}if(Fail(f.failPrepare))return false;
            o->id=id;o->inventory=inventory;o->definition=static_cast<uint16_t>(NativeDefinition(item));f.last=item;
            if(f.faultStage==1){f.faulted=true;return false;}return true;};
        backend.add=[](void* p,const Session&,CEconItem* o){auto& f=*static_cast<Fixture*>(p);++f.adds;
            f.faulted=false;if(f.faultStage==2&&!f.faultAfter){f.faulted=true;return false;}
            if(Fail(f.failAdd)){if(f.falseAddButInserted)f.cache.push_back(o);return false;}f.cache.push_back(o);
            if(f.faultStage==2){f.faulted=true;return false;}return true;};
        backend.remove=[](void* p,const Session&,CEconItem* o){auto& f=*static_cast<Fixture*>(p);++f.removes;f.faulted=false;
            if(f.faultStage==3&&!f.faultAfter){f.faulted=true;return false;}if(Fail(f.failRemove))return false;
            if(!f.stickyRemove)f.cache.erase(std::remove(f.cache.begin(),f.cache.end(),o),f.cache.end());
            if(f.faultStage==3){f.faulted=true;return false;}return true;};
        backend.notify=[](void* p,const Session&,CEconItem*,Change){auto& f=*static_cast<Fixture*>(p);++f.notifications;f.faulted=false;
            if(f.faultStage==4&&!f.faultAfter){f.faulted=true;return Notification::Indeterminate;}
            if(Fail(f.failNotify))return Notification::Rejected;++f.nativeEffects;
            if(f.faultStage==4){f.faulted=true;return Notification::Indeterminate;}return Notification::Done;};
        backend.destroy=[](void* p,CEconItem* o){auto& f=*static_cast<Fixture*>(p);++f.destroyCalls;f.faulted=false;
            if(f.faultStage==5&&!f.faultAfter){f.faulted=true;return false;}if(Fail(f.failDestroy))return false;
            if(std::find(f.cache.begin(),f.cache.end(),o)!=f.cache.end())return false;
            auto i=std::find(f.allocated.begin(),f.allocated.end(),o);if(i==f.allocated.end())return false;
            f.allocated.erase(i);delete o;++f.destroyed;if(f.faultStage==5){f.faulted=true;return false;}return true;};
        backend.faulted=[](void* p){return static_cast<Fixture*>(p)->faulted;};
        backend.loadout=[](void* p,const Session&,int,int,uint64_t& id){id=static_cast<Fixture*>(p)->loadout;return true;};
        backend.equip=[](void* p,const Session&,int,int,uint64_t id){static_cast<Fixture*>(p)->loadout=id;return true;};
        controller.Initialize(backend,10);
    }
    ~Fixture(){for(auto* o:allocated)delete o;}
    void ReplaceCache(){for(auto* o:cache){allocated.erase(std::remove(allocated.begin(),allocated.end(),o),allocated.end());delete o;++destroyed;}cache.clear();++session.cache;++session.typeCache;}
};
static HazzureInventory::Collection Collection(unsigned count=1){
    HazzureInventory::Collection c;
    for(unsigned i=0;i<count;++i){HazzureInventory::Item item{};strcpy_s(item.label,"Variante");item.skin={7,180,123,0,.15f,1,1,0};item.attributes.definition=7;HazzureInventory::Add(c,item);}return c;
}
static bool IdentityAndUpdates(){Fixture f;auto c=Collection(3);
    CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Count()==3);CHECK(f.created==3&&f.adds==3);
    auto id=f.controller.Lookup(1)->nativeItemID;auto* object=f.controller.Lookup(1)->object;
    CHECK(id>0x200000000ULL&&id!=c.items[0].id);CHECK(f.controller.Synchronize(c,10));CHECK(f.created==3&&f.prepares==3&&f.notifications==3);
    c.items[0].skin.wear=.7f;c.items[0].attributes.stat=true;c.items[0].attributes.counter=1337;
    CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Lookup(1)->nativeItemID==id&&f.controller.Lookup(1)->object==object);
    CHECK(f.controller.Snapshot().updated==1&&f.last.attributes.counter==1337);CHECK(f.controller.Teardown(10));CHECK(f.destroyed==3&&f.cache.empty());CHECK(f.controller.Teardown(10));
    auto before=f.created;CHECK(!f.controller.Synchronize(c,11));CHECK(f.created==before);CHECK(f.controller.Snapshot().status==Status::WrongThread);
    c.items[0].skin.wear=NAN;CHECK(!f.controller.Synchronize(c,10));CHECK(f.created==before);return true;
}
static bool CreationFailures(){
    for(int stage=0;stage<4;++stage){Fixture f;auto c=Collection();
        if(stage==0)f.failPrepare=1;if(stage==1)f.failAdd=1;if(stage==2){f.failAdd=1;f.falseAddButInserted=true;}if(stage==3)f.failNotify=1;
        CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Count()==0);CHECK(f.cache.empty());CHECK(f.created==f.destroyed);
        CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Count()==1);CHECK(f.controller.Teardown(10));}
    Fixture f;auto c=Collection();f.failNotify=2;CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Count()==1&&f.destroyed==0&&f.cache.empty());
    CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Count()==1);CHECK(f.created==2&&f.destroyed==1);return true;
}
static bool RemovalFailures(){Fixture f;auto c=Collection();CHECK(f.controller.Synchronize(c,10));f.failRemove=1;
    CHECK(!f.controller.RemoveItem(1,10));CHECK(f.destroyed==0&&f.cache.size()==1&&f.controller.Count()==1);
    f.stickyRemove=true;CHECK(!f.controller.RemoveItem(1,10));CHECK(f.destroyed==0&&f.controller.Count()==1);
    f.stickyRemove=false;f.failNotify=1;CHECK(!f.controller.RemoveItem(1,10));CHECK(f.destroyed==0&&f.cache.empty()&&f.controller.Count()==1);
    CHECK(f.controller.RemoveItem(1,10));CHECK(f.destroyed==1&&f.controller.Count()==0);
    CHECK(f.controller.Synchronize(c,10));f.failDestroy=1;CHECK(!f.controller.Teardown(10));CHECK(f.controller.Count()==1);CHECK(f.controller.Teardown(10));return true;
}
static bool SessionsAndBounds(){Fixture f;auto c=Collection();CHECK(f.controller.Synchronize(c,10));auto id=f.controller.Lookup(1)->nativeItemID;
    f.ReplaceCache();CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Lookup(1)->nativeItemID>id);CHECK(f.controller.Snapshot().sessionChanges==1);
    f.available=false;CHECK(!f.controller.Teardown(10));CHECK(f.controller.Count()==1);f.available=true;CHECK(f.controller.Teardown(10));
    f.maxInventory=0xffffffffu;CHECK(!f.controller.Synchronize(c,10));CHECK(f.created==2);f.maxInventory=0;f.maxID=0xffffffffffffffffULL;
    CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Count()==0);f.maxID=0;CHECK(f.controller.Synchronize(c,10));
    auto* old=f.controller.Lookup(1)->object;auto removed=f.backend.remove(f.backend.context,f.session,old);CHECK(removed);f.backend.destroy(f.backend.context,old);
    CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Count()==1);CHECK(f.controller.Teardown(10));
    auto big=Collection(96);CHECK(HazzureInventory::Valid(big));unsigned frames=0;while(!f.controller.Synchronize(big,10,2)&&++frames<60){}
    CHECK(f.controller.Count()==96&&frames<60);CHECK(f.controller.Synchronize(big,10,2));frames=0;while(!f.controller.Teardown(10,2)&&++frames<60){}
    CHECK(!f.controller.Count()&&frames<60);big.count=97;CHECK(!f.controller.Synchronize(big,10));return true;
}
static bool UpdateAndLoadout(){Fixture f;auto c=Collection();CHECK(f.controller.Synchronize(c,10));auto id=f.controller.Lookup(1)->nativeItemID;
    CHECK(f.controller.EquipInLoadout(1,2,0,10));CHECK(f.loadout==id);CHECK(!f.controller.EquipInLoadout(999,2,0,10));
    CHECK(!f.controller.EquipInLoadout(1,1,0,10));CHECK(!f.controller.EquipInLoadout(1,2,57,10));
    f.failPrepare=1;c.items[0].skin.wear=.8f;CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Lookup(1)->item.skin.wear==.15f);
    CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Lookup(1)->item.skin.wear==.8f);CHECK(f.created==1);
    CHECK(f.controller.RemoveItem(1,10));CHECK(f.loadout==33&&f.destroyed==1);
    CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.EquipInLoadout(1,2,0,10));f.loadout=99;
    CHECK(f.controller.RemoveItem(1,10));CHECK(f.loadout==99);return true;
}
static bool AttributeContracts(){auto c=Collection();auto& item=c.items[0];item.attributes.stat=true;item.attributes.counter=1337;
    item.attributes.charm=true;item.attributes.charmId=1;item.attributes.charmSeed=19;item.attributes.charmX=1;item.attributes.charmY=2;item.attributes.charmZ=3;
    item.attributes.stickers=true;item.attributes.slots[0].id=1;item.attributes.slots[0].x=.5f;item.attributes.slots[0].rotation=45;
    item.attributes.nametag=true;strcpy_s(item.attributes.name,"Nome proprio");CHECK(HazzureInventory::Valid(item));auto p=AttributePlan(item);CHECK(p.count==45);
    auto bits=[&](int id){for(size_t n=0;n<p.count;++n)if(p.entries[n].id==id)return p.entries[n].bits;return 0xffffffffu;};
    CHECK(bits(80)==1337&&bits(299)==1&&bits(306)==19);CHECK(bits(278)==HazzureArmory::FloatBits(.5f));CHECK(bits(301)==HazzureArmory::FloatBits(2));
    CHECK(bits(6)==HazzureArmory::FloatBits(180)&&bits(7)==HazzureArmory::FloatBits(123));
    item.attributes.stickers=item.attributes.charm=item.attributes.stat=false;p=AttributePlan(item);CHECK(bits(80)==0xffffffffu&&bits(299)==0xffffffffu&&bits(113)==0xffffffffu);
    CHECK(bits(115)==0xffffffffu);CHECK(NativeDefinition(item)==7);item.skin.definition=42;item.skin.model=515;CHECK(NativeDefinition(item)==515);
    Engine e;CHECK(!e.Compatible());CHECK(!e.Create());CEconItem obj;CHECK(!e.Destroy(&obj));CHECK(k_eSOCacheEvent_Incremental==4);CHECK(offsetof(CEconItem,flags)==0x40);
    return true;
}
static bool RuntimeRequests(){Fixture f;Runtime r;auto c=Collection(5);auto empty=r.Read();CHECK(!empty.enabled&&empty.requested==0&&empty.diagnostic.records==0);
    CHECK(r.Submit(c,true));CHECK(r.Read().diagnostic.status==Status::Queued);r.Tick(f.backend,10,false);CHECK(f.created==2&&r.Read().diagnostic.records==2);
    r.Tick(f.backend,11,false);CHECK(f.created==2);r.Tick(f.backend,10,false);r.Tick(f.backend,10,false);CHECK(f.created==5&&r.Read().processed==r.Read().requested);
    CHECK(r.Submit({},false));r.Tick(f.backend,10,false);CHECK(r.Read().diagnostic.records==3);r.Tick(f.backend,10,false);r.Tick(f.backend,10,false);CHECK(!r.Read().diagnostic.records&&f.destroyed==5);
    CHECK(r.Submit(Collection(),true));r.Tick(f.backend,10,true);CHECK(!r.Read().enabled&&r.Read().diagnostic.records==0);CHECK(!r.Submit(c,true));
    r.Tick(f.backend,10,false);CHECK(!r.Read().enabled);CHECK(r.Submit(Collection(),true));r.Tick(f.backend,10,false);CHECK(r.Read().diagnostic.records==1);
    std::thread a([&]{for(int n=0;n<20;++n)r.Submit(c,true);});std::thread b([&]{for(int n=0;n<20;++n)r.Submit({},false);});a.join();b.join();
    CHECK(r.Submit(Collection(2),true));r.Tick(f.backend,10,false);CHECK(r.Read().diagnostic.records==2&&r.Read().processed==r.Read().requested);
    CHECK(r.Submit({},false));r.Tick(f.backend,10,false);CHECK(!r.Read().diagnostic.records);return true;
}
static bool AdapterReadContracts(){Engine e;e.client=0x1000000;CEconItem object{};object.vtable=e.client+CurrentProfile::itemVtable;object.secondary=e.client+CurrentProfile::itemSecondary;
    alignas(16) unsigned char attributes[4+16*128]{};object.custom=reinterpret_cast<uintptr_t>(attributes);uint16_t count=1,id=111;
    memcpy(attributes+2,&count,2);memcpy(attributes+4,&id,2);uintptr_t entry{};CHECK(e.Entry(&object,111,entry)&&entry==reinterpret_cast<uintptr_t>(attributes+4));
    std::string name="Nome curto";struct Proto{uintptr_t vtable{},value{};} proto;proto.value=reinterpret_cast<uintptr_t>(&name);auto pp=reinterpret_cast<uintptr_t>(&proto);memcpy(attributes+12,&pp,8);
    CHECK(e.NameMatches(&object,"Nome curto"));CHECK(!e.NameMatches(&object,"Errado"));
    name="Nome com comprimento maior que quinze caracteres";CHECK(e.NameMatches(&object,name.c_str()));
    uintptr_t indirect=reinterpret_cast<uintptr_t>(&name);proto.value=reinterpret_cast<uintptr_t>(&indirect)|1;CHECK(e.NameMatches(&object,name.c_str()));
    count=129;memcpy(attributes+2,&count,2);CHECK(!e.Entry(&object,111,entry));count=0;memcpy(attributes+2,&count,2);CHECK(e.NameMatches(&object,""));
    CHECK(!e.SetBits(&object,6,1,123));CHECK(!e.SetName(&object,"Unsafe",123));CHECK(!e.RemoveAttribute(&object,111,123));
    struct Cache{uintptr_t table{};int count{};uint32_t padding{};uintptr_t data{};} cache;int n{};uintptr_t data{};
    CHECK(Engine::ReadCache(reinterpret_cast<uintptr_t>(&cache),n,data));cache.count=-1;CHECK(!Engine::ReadCache(reinterpret_cast<uintptr_t>(&cache),n,data));
    cache.count=4097;CHECK(!Engine::ReadCache(reinterpret_cast<uintptr_t>(&cache),n,data));cache.count=1;CHECK(!Engine::ReadCache(reinterpret_cast<uintptr_t>(&cache),n,data));
    cache.data=reinterpret_cast<uintptr_t>(&object);CHECK(Engine::ReadCache(reinterpret_cast<uintptr_t>(&cache),n,data));return true;
}
static bool ReservedItemLimits(){
    CEconItem virtuals[2]{};virtuals[0].id=0xf000000000000001ULL;virtuals[0].definition=4001;
    virtuals[1].id=0xf000000000000002ULL;virtuals[1].definition=36;
    virtuals[0].inventory=virtuals[1].inventory=0xc0000005u;
    uint64_t id{};uint32_t position{};
    for(const auto& item:virtuals)Engine::AccumulateLimits(item,id,position);
    CHECK(id==0&&position==0);
    Fixture empty;empty.maxID=id;empty.maxInventory=position;CHECK(empty.controller.Synchronize(Collection(),10));
    CHECK(empty.controller.Lookup(1)->nativeItemID==0x200000001ULL&&empty.controller.Lookup(1)->inventoryID==1001);
    CHECK(empty.controller.Teardown(10));
    CEconItem normal{};normal.id=0x234567890ULL;normal.inventory=1042;
    Engine::AccumulateLimits(normal,id,position);
    for(const auto& item:virtuals)Engine::AccumulateLimits(item,id,position);
    CHECK(id==normal.id&&position==1042);
    normal.id=0x234567891ULL;normal.inventory=1007;Engine::AccumulateLimits(normal,id,position);
    CHECK(id==normal.id&&position==1042); // maxima may come from different normal items
    Fixture mixed;mixed.maxID=id;mixed.maxInventory=position;CHECK(mixed.controller.Synchronize(Collection(),10));
    CHECK(mixed.controller.Lookup(1)->nativeItemID==id+1&&mixed.controller.Lookup(1)->inventoryID==1043);
    CHECK(mixed.controller.Teardown(10));
    // A genuinely normal entry keeps every position bit, including the observed
    // high value: do not mask 0xc0000000 to make an invalid real position pass.
    for(uint32_t value:{0x7fffffffu,0xc0000005u,0xffffffffu}){
        id=0;position=0;normal.id=77;normal.inventory=value;Engine::AccumulateLimits(normal,id,position);
        for(const auto& item:virtuals)Engine::AccumulateLimits(item,id,position);
        CHECK(id==77&&position==value);Fixture rejected;rejected.maxID=id;rejected.maxInventory=position;
        CHECK(!rejected.controller.Synchronize(Collection(),10));CHECK(rejected.controller.Snapshot().status==Status::Invalid&&rejected.created==0);
    }
    id=0;position=0;normal.id=0x0fffffffffffffffULL;normal.inventory=12;Engine::AccumulateLimits(normal,id,position);
    CHECK(id==normal.id&&position==12);Fixture overflow;overflow.maxID=id;overflow.maxInventory=position;
    CHECK(!overflow.controller.Synchronize(Collection(),10));CHECK(overflow.created==0&&overflow.controller.Snapshot().status==Status::Invalid);
    id=0;position=0;normal.id=0x234567890ULL;normal.inventory=0x7ffffffeu;Engine::AccumulateLimits(normal,id,position);
    Fixture boundary;boundary.maxID=id;boundary.maxInventory=position;CHECK(boundary.controller.Synchronize(Collection(),10));
    CHECK(boundary.controller.Lookup(1)->nativeItemID==id+1&&boundary.controller.Lookup(1)->inventoryID==0x7fffffffu);
    CHECK(boundary.controller.Teardown(10));return true;
}
static Engine::CacheState CacheShape(const Fixture& fixture,CEconItem* object,uint64_t id){
    Engine::CacheState state{static_cast<int>(fixture.cache.size()),0,0};
    for(auto* entry:fixture.cache){if(entry==object)++state.pointers;if(entry->id==id)++state.ids;}return state;
}
static bool GuardedFixtureRemove(void* context,const Session&,CEconItem* object){
    auto& f=*static_cast<Fixture*>(context);f.faulted=false;uint64_t id=object->id;
    auto before=CacheShape(f,object,id);if(!Engine::UniqueCached(before)){f.faulted=true;return false;}
    ++f.removes;
    if(f.failRemove==1)f.cache.push_back(object); // reproduces the old unchecked-Add RVA
    else if(f.failRemove!=3)f.cache.erase(std::remove(f.cache.begin(),f.cache.end(),object),f.cache.end());
    auto* returned=(f.failRemove==2||f.failRemove==3)?nullptr:object;
    auto after=CacheShape(f,object,id);
    if(f.failRemove==4||!Engine::DetachedExactly(before,after,object,returned)){f.faulted=true;return false;}
    return true;
}
static bool GuardedFixtureAdd(void* context,const Session&,CEconItem* object){
    auto& f=*static_cast<Fixture*>(context);f.faulted=false;auto before=CacheShape(f,object,object->id);
    if(before.pointers||before.ids){f.faulted=true;return false;}++f.adds;
    if(f.failAdd!=3)f.cache.push_back(object);if(f.failAdd==1)f.cache.push_back(object);
    bool returned=f.failAdd!=2&&f.failAdd!=3;auto after=CacheShape(f,object,object->id);
    if(returned&&Engine::AddedExactly(before,after))return true;
    if(!returned&&after.total==before.total&&!after.pointers&&!after.ids)return false;
    f.faulted=true;return false;
}
static bool CacheMutationContracts(){
    auto* requested=reinterpret_cast<CEconItem*>(0x1234);auto* wrong=reinterpret_cast<CEconItem*>(0x5678);
    CHECK(Engine::DetachedExactly({3,1,1},{2,0,0},requested,requested));
    CHECK(!Engine::DetachedExactly({3,1,1},{2,0,0},requested,wrong));
    CHECK(!Engine::DetachedExactly({3,1,1},{3,0,0},requested,requested));
    CHECK(!Engine::DetachedExactly({3,1,1},{2,1,1},requested,requested));
    CHECK(!Engine::UniqueCached({314,312,312})&&!Engine::UniqueCached({3,1,2}));
    CHECK(Engine::AddedExactly({2,0,0},{3,1,1})&&!Engine::AddedExactly({2,0,0},{4,2,2}));
    for(int mode=0;mode<=4;++mode){Fixture f;auto c=Collection();f.backend.remove=GuardedFixtureRemove;Controller guarded;
        CHECK(guarded.Initialize(f.backend,10));CHECK(guarded.Synchronize(c,10));f.failRemove=mode;
        bool removed=guarded.Teardown(10);
        if(mode==0){CHECK(removed&&guarded.Count()==0&&f.cache.empty()&&f.destroyed==1&&f.notifications==2);continue;}
        CHECK(!removed&&guarded.Count()==1&&guarded.Lookup(1)->quarantined&&guarded.Snapshot().status==Status::Indeterminate);
        auto nativeCalls=f.removes,notifies=f.notifications,destructors=f.destroyCalls,size=f.cache.size();
        for(int retry=0;retry<40;++retry){CHECK(!guarded.Teardown(10));CHECK(!guarded.Synchronize(c,10));}
        CHECK(f.removes==nativeCalls&&f.notifications==notifies&&f.destroyCalls==destructors&&f.cache.size()==size);
        CHECK(f.destroyed==0);if(mode==1)CHECK(size==2); // one bad call cannot grow to 314
    }
    // Pre-existing pointer aliases or ID collisions must block detach entirely.
    for(bool samePointer:{true,false}){Fixture f;f.backend.remove=GuardedFixtureRemove;Controller guarded;
        CHECK(guarded.Initialize(f.backend,10));CHECK(guarded.Synchronize(Collection(),10));auto* object=guarded.Lookup(1)->object;
        if(samePointer)f.cache.push_back(object);else{auto* duplicate=new CEconItem(*object);f.allocated.push_back(duplicate);f.cache.push_back(duplicate);}
        CHECK(!guarded.Teardown(10));CHECK(guarded.Lookup(1)->quarantined&&f.removes==0&&f.destroyed==0&&f.notifications==1);
        CHECK(!guarded.Teardown(10));CHECK(f.cache.size()==2&&f.removes==0&&f.destroyed==0);
    }
    for(int mode=0;mode<=3;++mode){Fixture f;f.backend.add=GuardedFixtureAdd;f.failAdd=mode;Controller guarded;
        CHECK(guarded.Initialize(f.backend,10));bool created=guarded.Synchronize(Collection(),10);
        if(mode==0){CHECK(created&&guarded.Count()==1);CHECK(guarded.Teardown(10));continue;}
        if(mode==3){CHECK(!created&&guarded.Count()==0&&f.cache.empty()&&f.destroyed==1);continue;}
        CHECK(!created&&guarded.Lookup(1)->quarantined&&guarded.Snapshot().status==Status::Indeterminate);
        auto size=f.cache.size();for(int retry=0;retry<40;++retry)CHECK(!guarded.Synchronize(Collection(),10));
        CHECK(f.created==1&&f.adds==1&&f.notifications==0&&f.destroyed==0&&f.cache.size()==size);
        CHECK(!guarded.Teardown(10)&&f.removes==0&&f.destroyed==0);
    }
    return true;
}
static bool SelectionAndRemoval(){Fixture f;auto c=Collection();c.items[0].skin.definition=65535;c.items[0].attributes.definition=65535;
    CHECK(!f.controller.Synchronize(c,10));CHECK(f.created==0);c=Collection();c.items[0].skin.paint=19999;CHECK(!f.controller.Synchronize(c,10));CHECK(f.created==0);
    c=Collection();c.items[0].skin.definition=42;c.items[0].skin.model=515;c.items[0].skin.paint=418;c.items[0].attributes.definition=42;CHECK(KnownDefinition(c.items[0]));
    c.items[0].skin.model=501;CHECK(!KnownDefinition(c.items[0]));c=Collection();c.items[0].skin.definition=5030;c.items[0].attributes.definition=5030;c.items[0].skin.model=5030;c.items[0].skin.paint=10018;CHECK(KnownDefinition(c.items[0]));
    c=Collection();c.items[0].attributes.stat=true;c.items[0].attributes.counter=1337;c.items[0].attributes.nametag=true;strcpy_s(c.items[0].attributes.name,"Teste");
    CHECK(f.controller.Synchronize(c,10));CHECK(f.last.attributes.stat&&f.last.attributes.counter==1337&&f.last.attributes.nametag);
    c.items[0].attributes.stat=c.items[0].attributes.nametag=false;CHECK(f.controller.Synchronize(c,10));CHECK(!f.last.attributes.stat&&!f.last.attributes.nametag);
    CHECK(f.controller.Teardown(10));return true;
}
static bool ExternalEraseAndAmbiguousEffects(){
    for(int method=0;method<3;++method){Fixture f;auto c=Collection();CHECK(f.controller.Synchronize(c,10));
        auto* old=f.controller.Lookup(1)->object;CHECK(f.backend.remove(&f,f.session,old));CHECK(f.backend.destroy(&f,old));
        auto notifications=f.notifications,destructors=f.destroyCalls;
        CHECK(method==0?f.controller.RemoveItem(1,10):method==1?f.controller.Teardown(10):f.controller.Synchronize({},10));
        CHECK(!f.controller.Count()&&f.notifications==notifications&&f.destroyCalls==destructors);
    }
    for(int stage=1;stage<=5;++stage)for(bool after:{false,true}){Fixture f;auto c=Collection();
        if(stage>=3)CHECK(f.controller.Synchronize(c,10));f.faultStage=stage;f.faultAfter=after;
        CHECK(stage<=2?!f.controller.Synchronize(c,10):!f.controller.RemoveItem(1,10));
        CHECK(f.controller.Count()==1&&f.controller.Lookup(1)->quarantined);
        CHECK(f.controller.Snapshot().status==Status::Indeterminate);
        auto calls=f.prepares+f.adds+f.removes+f.notifications+f.destroyCalls;auto effects=f.nativeEffects;
        f.faultStage=0;CHECK(!f.controller.Synchronize(c,10));CHECK(!f.controller.Teardown(10));
        CHECK(f.prepares+f.adds+f.removes+f.notifications+f.destroyCalls==calls&&f.nativeEffects==effects);
        f.ReplaceCache();CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Count()==1);
        CHECK(f.controller.Snapshot().status==Status::SessionChanged);
    }
    // Notification failures while cache-owned are safely forgotten when that owner goes away.
    for(bool after:{false,true}){Fixture f;auto c=Collection();f.faultStage=4;f.faultAfter=after;
        CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Lookup(1)->cached&&f.controller.Lookup(1)->quarantined);
        auto calls=f.notifications;f.faultStage=0;CHECK(!f.controller.Synchronize(c,10));CHECK(f.notifications==calls);
        f.ReplaceCache();CHECK(f.controller.Synchronize(c,10));CHECK(f.controller.Count()==1&&f.controller.Snapshot().sessionChanges==1);
        CHECK(f.controller.Teardown(10));
    }
    // A failed Updated callback never triggers a second native callback or an unproved rollback.
    for(bool after:{false,true}){Fixture f;auto c=Collection();CHECK(f.controller.Synchronize(c,10));
        c.items[0].skin.wear=.9f;f.faultStage=4;f.faultAfter=after;CHECK(!f.controller.Synchronize(c,10));
        auto calls=f.notifications;CHECK(f.controller.Lookup(1)->quarantined);CHECK(!f.controller.Synchronize(c,10));CHECK(f.notifications==calls);
    }
    // Unknown factory effects cannot be retried just because no pointer was returned.
    Fixture f;auto c=Collection();f.backend.create=[](void* p)->CEconItem*{auto& value=*static_cast<Fixture*>(p);++value.created;value.faulted=true;return nullptr;};
    CHECK(f.controller.Initialize(f.backend,10));CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Snapshot().status==Status::Indeterminate);
    CHECK(!f.controller.Synchronize(c,10));CHECK(f.created==1);CHECK(!f.controller.Teardown(10));
    CHECK(f.controller.Snapshot().status==Status::Indeterminate);return true;
}
static bool FailedMutationReadOwnership(){
    // Add success, false without insertion, and false after insertion all need reconciliation.
    for(int outcome=0;outcome<3;++outcome)for(bool absent:{false,true}){Fixture f;auto c=Collection();
        if(outcome){f.failAdd=1;f.falseAddButInserted=outcome==2;}f.failPresenceAt=1;
        CHECK(!f.controller.Synchronize(c,10));CHECK(f.controller.Count()==1);
        CHECK(f.controller.Lookup(1)->uncertain&&f.notifications==0&&f.destroyCalls==0);
        bool inserted=outcome!=1;
        if(absent&&inserted){auto* object=f.controller.Lookup(1)->object;
            CHECK(f.backend.remove(&f,f.session,object));CHECK(f.backend.destroy(&f,object));}
        auto notifications=f.notifications,destructors=f.destroyCalls,removes=f.removes;
        if(!absent&&inserted){CHECK(f.controller.Teardown(10));CHECK(f.controller.Count()==0);
            CHECK(f.notifications==notifications+1&&f.destroyCalls==destructors+1&&f.removes==removes+1);
            CHECK(f.controller.Teardown(10));CHECK(f.destroyCalls==destructors+1);
        }else{CHECK(!f.controller.Teardown(10));CHECK(!f.controller.Synchronize(c,10));
            CHECK(f.controller.Count()==1&&f.controller.Lookup(1)->uncertain);
            CHECK(f.notifications==notifications&&f.destroyCalls==destructors&&f.removes==removes);
            f.ReplaceCache();CHECK(!f.controller.Teardown(10));CHECK(f.controller.Snapshot().status==Status::SessionChanged);
            CHECK(f.controller.Count()==1&&f.notifications==notifications&&f.destroyCalls==destructors);}
    }
    // Session replacement before reconciliation cannot discard unknown ownership, even if cache-owned.
    for(int outcome=0;outcome<3;++outcome){Fixture f;auto c=Collection();
        if(outcome){f.failAdd=1;f.falseAddButInserted=outcome==2;}f.failPresenceAt=1;
        CHECK(!f.controller.Synchronize(c,10));f.ReplaceCache();CHECK(!f.controller.Synchronize(c,10));
        CHECK(f.controller.Count()==1&&f.notifications==0&&f.destroyCalls==0);
        CHECK(f.controller.Snapshot().status==Status::SessionChanged);
    }
    // Remove's bool does not prove detach if the following read fails. Retain an absent record.
    for(int outcome=0;outcome<3;++outcome){Fixture f;auto c=Collection();CHECK(f.controller.Synchronize(c,10));
        f.stickyRemove=outcome==2;f.failPresenceAt=f.presenceCalls+2;
        CHECK(!f.controller.RemoveItem(1,10));CHECK(f.controller.Lookup(1)->uncertain&&f.controller.Count()==1);
        CHECK(f.destroyCalls==0&&f.notifications==1);
        if(outcome==1)CHECK(f.backend.destroy(&f,f.controller.Lookup(1)->object));
        auto notifications=f.notifications,destructors=f.destroyCalls,removes=f.removes;
        if(outcome==2){f.stickyRemove=false;CHECK(f.controller.RemoveItem(1,10));
            CHECK(f.controller.Count()==0&&f.notifications==notifications+1&&f.destroyCalls==destructors+1);
            CHECK(f.controller.Teardown(10));CHECK(f.destroyCalls==destructors+1);
        }else{CHECK(!f.controller.RemoveItem(1,10));CHECK(!f.controller.Teardown(10));
            CHECK(f.controller.Count()==1&&f.notifications==notifications&&f.destroyCalls==destructors&&f.removes==removes);
            f.ReplaceCache();CHECK(!f.controller.Teardown(10));CHECK(f.controller.Count()==1);
            CHECK(f.controller.Snapshot().status==Status::SessionChanged);
            CHECK(f.notifications==notifications&&f.destroyCalls==destructors);}
    }
    // Even a successful Add followed immediately by absence cannot establish module ownership.
    Fixture f;auto c=Collection();f.backend.add=[](void* p,const Session&,CEconItem* o){auto& value=*static_cast<Fixture*>(p);
        ++value.adds;value.backend.destroy(p,o);return true;};CHECK(f.controller.Initialize(f.backend,10));
    CHECK(!f.controller.Synchronize(c,10));auto destructors=f.destroyCalls;
    CHECK(!f.controller.Teardown(10));CHECK(f.controller.Count()==1&&f.controller.Lookup(1)->uncertain);
    CHECK(f.notifications==0&&f.destroyCalls==destructors);return true;
}
static bool CurrentProfileContracts(){
    namespace P=HazzureNativeInventory::CurrentProfile;
    CHECK(P::Identity(0x6AE217A2u,0x29A2000u));CHECK(!P::Identity(0x6ADA7340u,0x29A0000u));
    std::vector<unsigned char> image(P::imageSize);auto base=reinterpret_cast<uintptr_t>(image.data());
    IMAGE_DOS_HEADER dos{};dos.e_magic=IMAGE_DOS_SIGNATURE;dos.e_lfanew=0x80;memcpy(image.data(),&dos,sizeof(dos));
    IMAGE_NT_HEADERS64 nt{};nt.Signature=IMAGE_NT_SIGNATURE;nt.FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;
    nt.FileHeader.TimeDateStamp=P::stamp;nt.OptionalHeader.SizeOfImage=P::imageSize;nt.OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    memcpy(image.data()+dos.e_lfanew,&nt,sizeof(nt));
    for(const auto& entry:P::prefixes)memcpy(image.data()+entry.rva,entry.bytes,sizeof(entry.bytes));
    for(const auto& entry:P::slots){uintptr_t target=base+entry.target;memcpy(image.data()+entry.rva,&target,sizeof(target));}
    CHECK(P::Supported(base,true));
    image[P::frame]=0xE9;CHECK(P::Supported(base));CHECK(!P::Supported(base,true)); // installed detour vs startup gate
    image[P::create]^=1;CHECK(!P::Supported(base));image[P::create]^=1;
    uintptr_t wrong=base+P::add;memcpy(image.data()+P::typeVtable+0x18,&wrong,sizeof(wrong));CHECK(!P::Supported(base));
    return true;
}
int main(){if(!CurrentProfileContracts()||!IdentityAndUpdates()||!CreationFailures()||!RemovalFailures()||!SessionsAndBounds()||!UpdateAndLoadout()||!AttributeContracts()||
    !RuntimeRequests()||!AdapterReadContracts()||!ReservedItemLimits()||!CacheMutationContracts()||!SelectionAndRemoval()||!ExternalEraseAndAmbiguousEffects()||!FailedMutationReadOwnership())return 1;
    printf("PASS: %u native inventory controller checks; own process, no game, input or audio.\n",checks);return 0;}
