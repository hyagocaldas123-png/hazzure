// Production FrameHook integration with an in-memory SO-cache backend. No game,
// Present, rendering, input, audio or native engine addresses are exercised.
#define INTERNAL_SELF_TEST
#define main ExistingInternalSelfTestMain
#include "../src/Internal.cpp"
#undef main
#define main ExistingNativeInventorySelfTestMain
#include "native-inventory-tests.cpp"
#undef main
#undef CHECK
static unsigned frameChecks{},originalCalls{},sessionCalls{};static DWORD mutationThread{},ownerThread{};
static bool orderValid=true;static int currentStage=-1;
static bool (*fixtureSession)(void*,HazzureNativeInventory::Session&);
static void OriginalFrameFixture(void*,int stage){++originalCalls;currentStage=stage;}
static bool OrderedSession(void* context,HazzureNativeInventory::Session& session){
    ++sessionCalls;mutationThread=GetCurrentThreadId();
    // World::Observe has already cleared this synthetic ready bit by stage 6.
    orderValid=orderValid&&currentStage==6&&!HazzureWorld::ready.load()&&mutationThread==ownerThread;
    return fixtureSession(context,session);
}
#define FRAME_CHECK(condition) do{++frameChecks;if(!(condition)){printf("FAIL frame line %d: %s\n",__LINE__,#condition);return 1;}}while(0)
int main(){
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    Shared ipc{};shared=&ipc;client=0;originalInventoryFrame=OriginalFrameFixture;ownerThread=GetCurrentThreadId();
    Fixture fixture;
    // Install the same public Runtime backend contract used by controller tests.
    // After initialization Runtime retains its backend; production Frame still
    // supplies the real current thread and enforces its owner/lock/budget.
    fixtureSession=fixture.backend.session;fixture.backend.session=OrderedSession;
    HazzureNativeInventory::runtime.Tick(fixture.backend,ownerThread,false);
    HazzureInferno::snapshots[0].id=123;HazzureWeather::published[0].active=true;
    HazzureSmoke::entries[0].pending=true;
    auto desired=Collection(5);
    FRAME_CHECK(!HazzureNativeInventory::Request(desired));
    FRAME_CHECK(!HazzureNativeInventory::RequestStop());
    auto unsupported=HazzureNativeInventory::Snapshot();
    FRAME_CHECK(!unsupported.available&&!unsupported.enabled&&unsupported.requested==0&&unsupported.diagnostic.status==Status::Unsupported);
    HazzureNativeInventory::backendAvailable.store(true); // synthetic callback/backend contract
    FRAME_CHECK(HazzureNativeInventory::Request(desired));
    FRAME_CHECK(HazzureNativeInventory::Snapshot().diagnostic.status==Status::Queued&&fixture.created==0);
    const int observedStages[]={0,1,2,3,5,6,7,8,10,11,12};
    for(int stage:observedStages){
        // This fails deterministically if the tick moves before World::Observe.
        if(stage==6)HazzureWorld::ready.store(true);
        auto before=fixture.created;InventoryFrameHook(nullptr,stage);
        FRAME_CHECK(currentStage==stage&&ipc.frames==originalCalls);
        FRAME_CHECK(fixture.created-before==(stage==6?2u:0u));
        if(stage==6)FRAME_CHECK(!HazzureWorld::ready.load());
    }
    FRAME_CHECK(HazzureInferno::snapshots[0].id==123&&HazzureWeather::published[0].active&&HazzureSmoke::entries[0].pending);
    auto first=HazzureNativeInventory::Snapshot();
    FRAME_CHECK(first.diagnostic.records==2&&first.processed==0&&first.requested==1);
    FRAME_CHECK(!(ipc.stageMask&(1L<<9))&&(ipc.stageMask&(1L<<6)));
    auto beforeLegacyProbe=fixture.created;InventoryFrameHook(nullptr,9);
    FRAME_CHECK(fixture.created==beforeLegacyProbe&&HazzureInferno::snapshots[0].id==123&&HazzureWeather::published[0].active&&HazzureSmoke::entries[0].pending);
    // Tick a full sequence on a non-owner thread: zero cache mutations/consumption.
    auto created=fixture.created;
    std::thread foreign([&]{for(int stage:observedStages)InventoryFrameHook(nullptr,stage);});foreign.join();
    FRAME_CHECK(fixture.created==created&&HazzureNativeInventory::Snapshot().diagnostic.records==2);
    for(unsigned frame=0;frame<2;++frame){
        HazzureWorld::ready.store(true);
        for(int stage:observedStages)InventoryFrameHook(nullptr,stage);
    }
    auto complete=HazzureNativeInventory::Snapshot();
    FRAME_CHECK(complete.processed==complete.requested&&complete.processed==1&&complete.diagnostic.status==Status::Ready);
    FRAME_CHECK(fixture.created==5&&complete.diagnostic.records==5&&HazzureNativeInventory::runtime.Owner()==ownerThread);
    FRAME_CHECK(orderValid&&sessionCalls>0&&mutationThread==ownerThread);
    // Observed non-stage-6 callbacks cannot consume this queue. The minimal
    // current-profile callback does not dispatch legacy stage-9 features.
    FRAME_CHECK(HazzureNativeInventory::RequestStop());
    FRAME_CHECK(HazzureNativeInventory::Snapshot().diagnostic.status==Status::Queued);
    for(int stage:{0,1,2,3,5,7,8,10,11,12})InventoryFrameHook(nullptr,stage);
    FRAME_CHECK(fixture.destroyed==0);
    skinFailed.store(true); // a failed visual skin callback cannot block inventory
    for(unsigned frame=0;frame<3;++frame){auto before=fixture.destroyed;InventoryFrameHook(nullptr,6);FRAME_CHECK(fixture.destroyed-before<=2);}
    FRAME_CHECK(fixture.destroyed==5&&HazzureNativeInventory::Snapshot().diagnostic.records==0);
    FRAME_CHECK(HazzureNativeInventory::Request(Collection(3)));
    InventoryFrameHook(nullptr,6);FRAME_CHECK(HazzureNativeInventory::Snapshot().diagnostic.records==2);
    HazzureState::g_unloading.store(true);InventoryFrameHook(nullptr,6);
    FRAME_CHECK(HazzureNativeInventory::Snapshot().diagnostic.records==0&&fixture.destroyed==7);
    FRAME_CHECK(!HazzureNativeInventory::Request(Collection()));
    FRAME_CHECK(HazzureNativeInventory::runtime.Owner()==ownerThread&&ipc.faults==0);
    printf("PASS: %u production inventory-only callback checks; actual sequence [0,1,2,3,5,6,7,8,10,11,12] without 9 consumes public requests after original stage 6/World observation, owner enforcement, two-item budget, stop and suspension; no Present/game/native ABI.\n",frameChecks);
    return 0;
}
