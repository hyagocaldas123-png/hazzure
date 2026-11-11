// Synthetic snapshots and queue tests; no game process or native engine calls.
#define NOMINMAX
#include <vector>
#include <limits>
#include "../src/TacticalHUD.h"

static unsigned passed{},failed{},queries{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
template<class T>static void Put(uintptr_t at,const T& value){memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));}
static HazzureTactical::DamageQueue* resetDuringQuery{};
static HazzureTactical::Damage Query(uintptr_t,const HazzureTactical::DamageRequest&){
    ++queries;if(resetDuringQuery)resetDuringQuery->Reset();return {true,73};
}
static HazzureTactical::Damage Unavailable(uintptr_t,const HazzureTactical::DamageRequest&){return {};}
static HazzureTactical::Damage Zero(uintptr_t,const HazzureTactical::DamageRequest&){return {true,0};}
int main(){
    using namespace HazzureTactical;using namespace HazzureOffsets;
    std::vector<unsigned char> moduleBytes(0x1000),listBytes(0x210),chunkBytes(512*0x70),
        localBytes(0x4000),bombBytes(0x1a00),sceneBytes(0x400),bombSceneBytes(0x400),globalsBytes(0x100),
        controllerBytes(0x1000),specControllerBytes(0x1000),otherControllerBytes(0x1000),
        observerBytes(0x2000),otherObserverBytes(0x2000),localObserverBytes(0x2000),
        observedBytes(0x4000),servicesBytes(0x100),otherServicesBytes(0x100),localServicesBytes(0x100);
    const auto ptr=[](auto& b){return reinterpret_cast<uintptr_t>(b.data());};
    const auto client=ptr(moduleBytes),list=ptr(listBytes),chunk=ptr(chunkBytes),local=ptr(localBytes),bomb=ptr(bombBytes),
        controller=ptr(controllerBytes),specController=ptr(specControllerBytes),otherController=ptr(otherControllerBytes),
        observer=ptr(observerBytes),otherObserver=ptr(otherObserverBytes),localObserver=ptr(localObserverBytes),observed=ptr(observedBytes),
        services=ptr(servicesBytes),otherServices=ptr(otherServicesBytes),localServices=ptr(localServicesBytes);
    Globals::dwEntityList=0x20;Globals::dwLocalPlayerPawn=0x28;Globals::dwLocalPlayerController=0x30;
    Globals::dwPlantedC4=0x38;Globals::dwGlobalVars=0x40;
    Put(client+0x20,list);Put(client+0x28,local);Put(client+0x30,controller);Put(client+0x38,bomb);Put(client+0x40,ptr(globalsBytes));
    Put(list+0x10,chunk);Put(ptr(globalsBytes)+0x30,100.f);
    const auto add=[&](unsigned slot,uintptr_t entity,const char* name){auto id=chunk+slot*0x70;auto handle=0x18000u|slot;
        Put(id,entity);Put(id+0x10,handle);Put(id+0x20,reinterpret_cast<uintptr_t>(name));Put(entity+0x10,id);return handle;};
    const auto localId=add(65,local,"player"),bombId=add(100,bomb,"planted_c4"),observedId=add(66,observed,"player");
    const auto observerId=add(71,observer,"cs_observer_pawn"),otherObserverId=add(72,otherObserver,"cs_observer_pawn"),
        localObserverId=add(73,localObserver,"cs_observer_pawn");
    add(1,controller,"cs_player_controller");add(2,specController,"cs_player_controller");add(3,otherController,"cs_player_controller");
    Put(controller+Schema::m_hPlayerPawn,localId);Put(controller+Schema::m_hObserverPawn,localObserverId);
    Put(specController+Schema::m_hObserverPawn,observerId);Put(otherController+Schema::m_hObserverPawn,otherObserverId);
    Put(local+Schema::m_iHealth,100);Put(local+Schema::m_iTeamNum,uint8_t(2));Put(local+Schema::m_lifeState,uint8_t(0));
    Put(local+Schema::m_pGameSceneNode,ptr(sceneBytes));Put(ptr(sceneBytes)+Schema::m_vecAbsOrigin,Vec3{100,200,30});
    Put(local+Schema::m_angEyeAngles,Vec3{0,179,0});Put(local+0x3b8,100.f);
    Put(bomb+Schema::m_pGameSceneNode,ptr(bombSceneBytes));Put(ptr(bombSceneBytes)+Schema::m_vecAbsOrigin,Vec3{0,0,0});
    Put(bomb+0x1288,uint8_t(1));Put(bomb+0x128c,1);Put(bomb+0x12b8,140.f);Put(bomb+0x12c0,40.f);

    CHECK(ExactEntity(list,localId)==local&&!ExactEntity(list,0)&&!ExactEntity(list,0xffffffffu),"serialized identities resolve; invalid handles do not");
    CHECK(!ExactEntity(list,localId+0x8000)&&!ExactEntity(list,65),"a recycled slot or index-only handle cannot impersonate a pawn");
    CHECK(Handle(list,bomb)==bombId,"bomb identity is tied to the current entity slot");
    Bomb snapshot{};
    CHECK(ReadBombEntity(bomb,list,100,snapshot)&&snapshot.remaining==40&&snapshot.site==1,"C4 reads the actual timer and site");
    Put(bomb+0x12c0,35.f);Put(bomb+0x12b8,130.f);
    CHECK(ReadBombEntity(bomb,list,100,snapshot)&&snapshot.remaining==30&&snapshot.timerLength==35,"custom server timer is not forced to 40 seconds");
    Put(bomb+0x12c4,uint8_t(1));Put(bomb+0x12d4,5.f);Put(bomb+0x12d8,104.f);
    CHECK(ReadBombEntity(bomb,list,100,snapshot)&&snapshot.beingDefused&&snapshot.defuseLength==5&&snapshot.defuseLeft==4,"kit defuse uses the engine countdown");
    Put(bomb+0x12d4,10.f);Put(bomb+0x12d8,109.f);
    CHECK(ReadBombEntity(bomb,list,100,snapshot)&&snapshot.defuseLength==10&&snapshot.defuseLeft==9,"non-kit defuse retains its 10-second duration");
    CHECK(!DefuseHasMargin(5,5)&&!DefuseHasMargin(4.9f,5),"ties and late finishes are never labeled a successful defuse");
    CHECK(DefuseHasMargin(5.1f,5)&&!DefuseHasMargin(5.01f,5),"a defuse needs at least a server tick of timing margin");
    CHECK(!DefuseHasMargin(std::numeric_limits<float>::quiet_NaN(),5),"invalid timing cannot produce a favorable defuse prediction");
    Put(bomb+0x12d8,std::numeric_limits<float>::quiet_NaN());
    CHECK(!ReadBombEntity(bomb,list,100,snapshot)&&!snapshot.address,"invalid defuse data clears the snapshot instead of claiming success");
    Put(bomb+0x12c4,uint8_t(0));Put(bomb+0x12bd,uint8_t(1));
    CHECK(!ReadBombEntity(bomb,list,100,snapshot),"an exploded C4 stops the HUD");Put(bomb+0x12bd,uint8_t(0));
    CHECK(!ReadBombEntity(bomb,list,std::numeric_limits<float>::infinity(),snapshot),"nonfinite game clocks are rejected");
    Put(bomb+0x12b8,300.f);CHECK(!ReadBombEntity(bomb,list,100,snapshot),"a clock from another map cannot produce a fabricated timer");Put(bomb+0x12b8,130.f);
    Put(bomb+0x128c,9);CHECK(!ReadBombEntity(bomb,list,100,snapshot),"invalid site does not silently become site A");Put(bomb+0x128c,1);
    BombLocator locator;
    CHECK(locator.Read(client,snapshot,1000)&&snapshot.address==bomb,"locator accepts an exact direct C4 pointer");
    Put(client+0x38,client+0x48);Put(client+0x48,bomb);
    CHECK(locator.Read(client,snapshot,1010),"locator accepts the pointer-to-pointer global layout");
    Put(local+0x3b8,80.f);CHECK(!locator.Read(client,snapshot,1020),"global time must agree with the local simulation clock");Put(local+0x3b8,100.f);
    CHECK(locator.Read(client,snapshot,1030),"clock recovery restores the current snapshot");
    DamageRequest request{};
    CHECK(MakeDamageRequest(snapshot,local,request)&&request.pawnHandle==localId,"damage request captures identity and full native pose");
    CHECK(!NativeDamage(client,request).available,"an unrecognized module never calls the native damage routine");
    DamageQueue queue;
    CHECK(!queue.Submit(request,100).available&&queries==0,"render-side submission never invokes engine code");
    queue.Process(client,Query,101);
    CHECK(queue.Submit(request,110).available&&queue.Submit(request,110).amount==73&&queries==1,"frame-side result reaches the HUD without inventing a formula");
    CHECK(!queue.Submit(request,134).available,"damage results expire after 32 milliseconds");
    queue.Process(client,Zero,135);
    CHECK(queue.Submit(request,136).available&&queue.Submit(request,136).amount==0,"a genuine zero damage result is distinct from unavailable data");
    auto moved=request;moved.origin.x+=3;
    CHECK(!queue.Submit(moved,137).available,"movement invalidates cached blast damage");
    queue.Process(client,Query,138);moved.angles.y-=3;
    CHECK(!queue.Submit(moved,139).available,"turning invalidates direction-dependent cached damage");
    auto wrapped=request;wrapped.angles.y=-180.f;
    CHECK(SamePose(request,wrapped),"direction comparisons take the short path around 180 degrees");
    queue.Process(client,Unavailable,140);CHECK(!queue.Submit(moved,141).available,"missing map data is not reported as safe");
    queue.Reset();CHECK(!queue.Submit(request,142).available,"reset removes damage from the previous round");
    resetDuringQuery=&queue;queue.Process(client,Query,143);resetDuringQuery=nullptr;
    CHECK(!queue.Submit(request,144).available,"a late query cannot republish after disable or unload");
    Put(bomb+0x12dc,uint8_t(1));ReadBombEntity(bomb,list,100,snapshot);
    CHECK(!MakeDamageRequest(snapshot,local,moved),"defused bombs never queue explosion predictions");Put(bomb+0x12dc,uint8_t(0));

    Put(observer+0x1308,services);Put(otherObserver+0x1308,otherServices);Put(localObserver+0x1308,localServices);
    Put(services+0x48,uint8_t(4));Put(services+0x4c,localId);
    Put(otherServices+0x48,uint8_t(5));Put(otherServices+0x4c,localId);
    Put(localServices+0x48,uint8_t(4));Put(localServices+0x4c,observedId);
    char cleanName[128]="Nome sanitizado";Put(specController+0x878,reinterpret_cast<uintptr_t>(cleanName));
    char rawName[128]="Outro\nNome";memcpy(reinterpret_cast<void*>(otherController+Schema::m_iszPlayerName),rawName,sizeof(rawName));
    auto specs=CollectSpectators(list,local,controller);
    CHECK(specs.count==2&&specs.entries[0].mode==4&&specs.entries[1].mode==5,"only actual first/third-person observers enter the list");
    CHECK(!strcmp(specs.entries[0].name,"Nome sanitizado")&&!strcmp(specs.entries[1].name,"Outro Nome"),"sanitized names are preferred and control characters cannot break rows");
    Put(services+0x48,uint8_t(6));CHECK(CollectSpectators(list,local,controller).count==1,"free-camera mode is not mislabeled third person");Put(services+0x48,uint8_t(4));
    Put(services+0x4c,localId+0x8000);CHECK(CollectSpectators(list,local,controller).count==1,"a stale target handle does not match a recycled local slot");Put(services+0x4c,localId);
    Put(specController+Schema::m_hObserverPawn,observerId+0x8000);
    CHECK(CollectSpectators(list,local,controller).count==1,"a stale observer pawn does not create a spectator");Put(specController+Schema::m_hObserverPawn,observerId);
    Put(local+Schema::m_iHealth,0);Put(local+Schema::m_lifeState,uint8_t(1));
    Put(services+0x4c,observedId);Put(otherServices+0x4c,observedId);
    CHECK(CollectSpectators(list,local,controller).count==2,"while dead, spectators sharing the actual observed camera are listed");
    CHECK(CollectSpectators(list,0,controller).count==2,"an observer camera still works after the dead player pawn is removed");
    Put(localServices+0x48,uint8_t(6));CHECK(CollectSpectators(list,local,controller).count==0,"free camera cannot reuse the previous observed target");
    Put(localServices+0x48,uint8_t(4));Put(localServices+0x4c,observedId+0x8000);
    CHECK(CollectSpectators(list,local,controller).count==0,"recycled camera targets clear the spectator list");
    Put(local+Schema::m_iHealth,100);Put(local+Schema::m_lifeState,uint8_t(0));Put(services+0x4c,localId);
    Put(specController+0x878,uintptr_t(0));memset(reinterpret_cast<void*>(specController+Schema::m_iszPlayerName),'X',127);
    specs=CollectSpectators(list,local,controller);
    CHECK(specs.count==1&&specs.entries[0].name[127]==0,"maximum-length player names are always terminated");
    CHECK(CollectSpectators(0,local,controller).count==0,"disconnect yields an empty list");
    printf("RESULT: %u passed, %u failed; synthetic memory only, no native engine calls.\n",passed,failed);
    return failed?1:0;
}
