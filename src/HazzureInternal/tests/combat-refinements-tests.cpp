#define NOMINMAX
#include <cstdio>
#include <limits>
#include "../src/CombatRefinements.h"
static unsigned passed{},failed{};
#define CHECK(value,label) do{if(value){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    using namespace HazzureCombatRefine;
    AimDelay aim;KillNotice kill{};
    CHECK(aim.Ready(true,1,10,2,3,5,100,kill,200,150),"first acquisition does not become a target switch delay");
    CHECK(!aim.Ready(true,1,10,2,4,5,110,kill,200,150)&&aim.wait==AimWait::Switch,"switching identity starts its own configured delay");
    CHECK(!aim.Ready(true,1,10,2,4,5,259,kill,200,150),"switch delay retains one timestamp across frames");
    CHECK(aim.Ready(true,1,10,2,4,5,260,kill,200,150),"switch delay expires at its exact boundary");
    kill={1,10,5,270};
    CHECK(!aim.Ready(true,1,10,2,4,5,300,kill,200,150)&&aim.wait==AimWait::Kill,"confirmed local death event starts kill delay even without changing target");
    CHECK(aim.Ready(true,1,10,2,4,5,470,kill,200,150),"kill delay expires using monotonic elapsed time");
    kill={8,10,5,480};
    CHECK(aim.Ready(true,1,10,2,4,5,490,kill,200,150),"another player's kill cannot pause this owner");
    kill={1,99,5,480};
    CHECK(aim.Ready(true,1,10,2,4,5,490,kill,200,150),"recycled pawn pointer cannot consume an old owner serial");
    kill={1,10,4,480};
    CHECK(aim.Ready(true,1,10,2,4,5,490,kill,200,150),"a prior round kill cannot carry into the new epoch");
    kill={};
    CHECK(aim.Ready(true,1,10,2,9,5,500,kill,0,0),"disabled delay knobs preserve immediate acquisition");
    CHECK(!aim.Ready(false,1,10,2,9,5,501,kill,200,150)&&!aim.target,"releasing aim resets target delay ownership");
    aim.Ready(true,1,10,2,3,5,600,kill,200,150);
    aim.Ready(true,1,10,2,4,5,610,kill,200,150);
    CHECK(aim.Ready(true,1,10,2,4,5,609,kill,200,150),"clock rollback clears the previous switch deadline");
    aim.Ready(true,1,10,2,5,5,620,kill,200,150);
    CHECK(aim.Ready(true,1,11,2,5,5,621,kill,200,150),"owner replacement clears a pending switch deadline");
    ConfirmKill({1,10,5,700,22});ConfirmKill({1,10,5,701,22});
    CHECK(ReadKill().time==700,"duplicate death notification cannot extend kill delay");
    ConfirmKill({1,10,5,702,23});
    CHECK(ReadKill().time==702,"a different victim starts a distinct confirmed kill window");
    ConfirmKill({});CHECK(!ReadKill().time,"round reset clears confirmed kill timing");
    kill={1,11,5,900};
    CHECK(aim.Ready(true,1,11,2,5,5,800,kill,200,150),"a notice from a future clock cannot pin acquisition indefinitely");
    auto balanced=BalancedWindow();
    CHECK(balanced.minimum==40&&balanced.maximum==90,"balanced random mode uses the requested inclusive 40-90 ms window");
    CHECK(TriggerDelayMode(true,false)==TriggerMode::Balanced&&TriggerDelayMode(true,true)==TriggerMode::Distance&&
          TriggerDelayMode(false,true)==TriggerMode::Manual,"persisted flags select balanced, distance and manual modes consistently");
    std::mt19937 random(12345);TriggerTiming timing;
    CHECK(!timing.Ready(1,10,TriggerMode::Balanced,100,balanced,random),"acquisition cannot fire on its first frame");
    auto chosen=timing.delay;auto afterSample=random;
    CHECK(!timing.Ready(1,10,TriggerMode::Balanced,100+chosen-1,balanced,random)&&timing.delay==chosen&&random==afterSample,
          "pending target retains one sample without consuming random values every frame");
    CHECK(timing.Ready(1,10,TriggerMode::Balanced,100+chosen,balanced,random),"the selected delay opens at its exact deadline");
    CHECK(!timing.Ready(1,11,TriggerMode::Balanced,200,balanced,random)&&timing.since==200,
          "switching weapon cannot reuse an almost-complete reaction from the previous gun");
    CHECK(!timing.Ready(1,11,TriggerMode::Distance,300,{150,220},random)&&timing.since==300,
          "switching delay mode rearms the acquisition rather than retaining the previous mode sample");
    chosen=timing.delay;afterSample=random;
    timing.Ready(1,11,TriggerMode::Distance,301,{149,219},random);
    CHECK(timing.delay==chosen&&timing.since==300&&random==afterSample,
          "distance changes while tracking do not restart or reroll a pending reaction");
    CHECK(!timing.Ready(2,11,TriggerMode::Balanced,400,balanced,random)&&timing.target==2&&timing.since==400,
          "a new enemy starts a fresh balanced delay");
    CHECK(!timing.Ready(0,11,TriggerMode::Balanced,500,balanced,random)&&!timing.target,
          "target loss clears the pending shot");
    timing.Ready(1,10,TriggerMode::Balanced,600,balanced,random);
    CHECK(!timing.Ready(1,10,TriggerMode::Balanced,550,balanced,random)&&timing.since==550,
          "a clock rollback rearms instead of leaving a stale future deadline");
    CHECK(!timing.Ready(1,0,TriggerMode::Balanced,650,balanced,random)&&!timing.target&&
          !timing.Ready(1,10,TriggerMode::Balanced,650,{90,40},random),"invalid weapon or reversed delay window cannot arm a shot");
    CHECK(!timing.Ready(1,10,TriggerMode::Manual,700,{65,65},random)&&timing.delay==65&&
          !timing.Ready(1,10,TriggerMode::Manual,710,{75,75},random)&&timing.since==710&&timing.delay==75,
          "manual delay edits replace the pending manual interval");
    uint32_t minimum=1000,maximum=0;uint64_t total=0;bool inRange=true;
    for(uintptr_t i=1;i<=10000;++i){timing.Reset();timing.Ready(i,10,TriggerMode::Balanced,1000,balanced,random);
        minimum=std::min(minimum,timing.delay);maximum=std::max(maximum,timing.delay);total+=timing.delay;
        inRange&=timing.delay>=40&&timing.delay<=90;}
    CHECK(inRange&&minimum==40&&maximum==90,"ten thousand seeded acquisitions remain in range and reach both endpoints");
    CHECK(std::fabs(static_cast<double>(total)/10000-65)<1,"balanced samples center around 65 ms without an artificial fixed delay");
    auto close=ReactionWindow(0),distant=ReactionWindow(30),beyond=ReactionWindow(100),middle=ReactionWindow(15);
    CHECK(close.minimum==60&&close.maximum==90,"close reaction interval uses the chosen 60-90 ms bounds");
    CHECK(distant.minimum==150&&distant.maximum==220&&beyond.maximum==220,"distance interval saturates after 30 metres");
    CHECK(middle.minimum==105&&middle.maximum==155,"intermediate distances interpolate both delay bounds");
    CHECK(!ReactionWindow(-1).maximum&&!ReactionWindow(std::numeric_limits<float>::quiet_NaN()).maximum,"invalid distances cannot select an immediate shot");
    bool monotonic=true;auto previous=close;
    for(int distance=1;distance<100;++distance){auto next=ReactionWindow(static_cast<float>(distance));
        monotonic&=next.minimum>=previous.minimum&&next.maximum>=previous.maximum&&next.minimum<=next.maximum;previous=next;}
    CHECK(monotonic,"delay windows stay ordered and monotonic over the range");
    float sixty=100,fast=100;
    for(int frame=0;frame<6;++frame)sixty=Health(sixty,27,1.f/60);
    for(int frame=0;frame<24;++frame)fast=Health(fast,27,1.f/240);
    CHECK(std::fabs(sixty-fast)<.001f&&sixty>27&&sixty<100,"health animation has equal progress at 60 and 240 FPS");
    CHECK(Health(30,80,.016f)==80,"healing does not display a stale lower health value");
    CHECK(Health(-1,73,.016f)==73&&Health(80,27,2.f)==27,"first snapshot and stale timing reset health animation");
    CHECK(Health(100,1000,.016f)==100&&Health(100,-1,.016f)>=0,"untrusted health values stay bounded");
    auto red=HealthColour(0),yellow=HealthColour(50),green=HealthColour(100);
    CHECK(red.red>red.green&&yellow.red==yellow.green&&green.green>green.red,"health colors pass from red through yellow to green");
    printf("RESULT: %u passed, %u failed; own process, no OS input or engine calls.\n",passed,failed);return failed?1:0;
}
