#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include <limits>
#include "../src/CombatConvenience.h"
#include "../src/RuntimeStability.h"
#include "../src/SmokeDensity.h"
#include "../src/RenderOptions.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
static HazzureSmoke::Verdict ClearSmoke(uintptr_t,const HazzureSmoke::Request&){return HazzureSmoke::Verdict::Clear;}
static HazzureSmoke::Verdict ResetSmoke(uintptr_t,const HazzureSmoke::Request& q){HazzureSmoke::Clear();HazzureSmoke::Submit(q,GetTickCount64());return HazzureSmoke::Verdict::Clear;}
static HazzureVisibility::Verdict ResetTrace(uintptr_t,const HazzureVisibility::Request& q){HazzureVisibility::Clear();HazzureVisibility::Submit(q,GetTickCount64());return HazzureVisibility::Verdict::Visible;}
int main(){
    using namespace HazzureConvenience;
    CHECK(CombatAllowed(true,false)&&!CombatAllowed(true,true)&&!CombatAllowed(false,false),"lineup input ownership excludes combat even with otherwise valid context");
    ScopeGate gate;ScopeInput q{true,true,true,false,false,false,1,2,3,9,100};
    CHECK(gate.Step(q),"eligible AWP acquisition requests exactly one zoom");
    CHECK(!gate.Step(q)&&gate.BlocksShot(1,2),"pending zoom never cycles zoom levels or releases fire gate");
    q.now=850;CHECK(!gate.Step(q)&&!gate.BlocksShot(1,2)&&gate.latched,"missing zoom confirmation expires without retry spam");
    q.target=4;q.now=851;CHECK(gate.Step(q),"new target acquisition is independent of old pending zoom");
    q.scoped=true;CHECK(!gate.Step(q)&&!gate.pending,"native scoped observation confirms the zoom");
    q.scoped=false;CHECK(!gate.Step(q),"manual unzoom does not cause an immediate automatic toggle loop");
    q.context=false;gate.Step(q);CHECK(!gate.pawn&&!gate.pending,"focus/context loss clears owned zoom intent");
    q.context=true;q.manualZoom=true;CHECK(!gate.Step(q),"physical zoom input takes precedence over automation");
    q.manualZoom=false;q.manualAttack=true;CHECK(!gate.Step(q),"physical fire input takes precedence over zoom");
    q.manualAttack=false;q.definition=7;CHECK(!gate.Step(q),"automatic scope excludes non-sniper weapons");
    ZoomSettle settle;
    CHECK(!settle.Ready(1,2,true,100)&&!settle.Ready(1,2,true,149)&&settle.Ready(1,2,true,150),"shot waits fifty milliseconds after confirmed zoom");
    CHECK(!settle.Ready(1,3,true,151),"weapon change cannot inherit another sniper zoom deadline");
    CHECK(!settle.Ready(1,3,false,152)&&!settle.observed,"unscoping clears readiness");
    HazzureStable::StopGate stop;
    CHECK(!stop.Step(74,75)&&stop.Step(74,75),"brake readiness requires two measured low-speed observations");
    CHECK(stop.Step(76,75)&&!stop.Step(80,75),"small speed noise does not reapply opposite directions but larger motion resets stop");
    CHECK(!stop.Step(.4f,0)&&stop.Step(.4f,0),"practical Full Stop accepts sub-unit residual speed after confirmation");
    CHECK(!stop.Step(1,0),"Full Stop rejects meaningful remaining movement");
    CHECK(!stop.Step(std::numeric_limits<float>::quiet_NaN(),0),"invalid speed never authorizes a shot");
    using S=HazzureSmoke::Verdict;
    HazzureZones::Clear();HazzureZones::enabled.store(false);HazzureSmoke::enabled.store(true);HazzureSmoke::Clear();
    HazzureSmoke::Request smoke{1,2,{0,0,64},{1000,0,64},0};auto now=GetTickCount64();
    CHECK(!HazzureSmoke::Allows(HazzureSmoke::FallbackQuery(now,smoke)),"empty event cache cannot prove a smoke-free line");
    HazzureZones::enabled.store(true);HazzureZones::Push({HazzureZones::Kind::Smoke,9,{500,0,64},now});
    CHECK(HazzureSmoke::FallbackQuery(now,smoke)==S::Blocked,"estimated intersecting smoke can conservatively block");
    smoke.end={0,1000,64};CHECK(!HazzureSmoke::Allows(HazzureSmoke::FallbackQuery(now,smoke)),"nonintersecting event sphere still cannot replace unavailable live voxels");
    smoke.end={1000,0,64};
    CHECK(HazzureSmoke::Submit(smoke,now)==S::Unknown,"first density request is pending, not clear");
    HazzureSmoke::ProcessPending(0,ClearSmoke);now=GetTickCount64();
    CHECK(HazzureSmoke::Submit(smoke,now)==S::Clear,"fresh confirmed native-query result can permit the line");
    CHECK(HazzureSmoke::Submit(smoke,now+17)==S::Unknown,"density results expire beyond sixteen milliseconds");
    HazzureSmoke::Clear();HazzureSmoke::Submit(smoke,GetTickCount64());HazzureSmoke::ProcessPending(0,ResetSmoke);
    CHECK(HazzureSmoke::Submit(smoke,GetTickCount64())==S::Unknown,"world reset rejects an old in-flight smoke result even when pointers are reused");
    CHECK(!HazzureSmoke::Allows(HazzureSmoke::Classify(-1))&&!HazzureSmoke::Allows(HazzureSmoke::Classify(INFINITY)),"invalid native density fails closed");
    CHECK(HazzureSmoke::Classify(0)==S::Clear&&HazzureSmoke::Classify(.00001f)==S::Blocked,"strict density distinguishes zero from any positive smoke density");
    HazzureVisibility::Clear();HazzureVisibility::Request trace{1,2,{0,0,0},{10,0,0},1};
    HazzureVisibility::Submit(trace,GetTickCount64());HazzureVisibility::ProcessPending(0,ResetTrace);
    CHECK(HazzureVisibility::Submit(trace,GetTickCount64())==HazzureVisibility::Verdict::Unknown,"world reset also rejects in-flight visibility from old generations");
    using namespace HazzurePerformance;Reset();
    for(int i=1;i<=100;++i)Record(Stage::Aim,static_cast<float>(i));auto stats=Read(Stage::Aim);
    CHECK(stats.samples==100&&stats.average==50.5f&&stats.p95==95&&stats.p99==99&&stats.maximum==100,"timing summary reports measured average and percentile ranks");
    for(int i=101;i<=200;++i)Record(Stage::Aim,static_cast<float>(i));stats=Read(Stage::Aim);
    CHECK(stats.samples==128&&stats.total==200&&stats.average==136.5f,"timing history keeps a bounded rolling window");
    Record(Stage::Aim,NAN);Record(Stage::Aim,-1);CHECK(Read(Stage::Aim).total==200,"invalid performance samples are ignored");
    HazzureRenderOptions::Publish(true,false);auto options=HazzureRenderOptions::Read();
    CHECK(options.weapon&&!options.dualLayer,"render callback reads a coherent published option snapshot");
    HazzureRenderOptions::Publish(false,true);options=HazzureRenderOptions::Read();
    CHECK(!options.weapon&&options.dualLayer,"render option updates do not retain obsolete feature bits");
    printf("RESULT: %u passed, %u failed; isolated own-process decisions, no game or OS input.\n",passed,failed);return failed?1:0;
}
