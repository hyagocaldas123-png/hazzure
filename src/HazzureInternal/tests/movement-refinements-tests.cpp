// Own process only: pure state transitions; no engine calls, windows or OS input.
#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include "../src/MovementTech.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    using namespace HazzureMove;
    JumpPulse pulse;JumpInput q{1,true,true,true,true,false,true,100};
    CHECK(pulse.Step(q)==65537&&pulse.owned&&pulse.down,"held Space starts a named-button press");
    q.now=104;CHECK(pulse.Step(q)==65537,"high render FPS does not truncate the jump pulse below a game tick");
    q.now=115;CHECK(pulse.Step(q)==65537,"pulse remains asserted while its minimum hold has not elapsed");
    q.now=116;CHECK(pulse.Step(q)==256&&!pulse.down,"pulse enters an explicit release phase");
    q.now=120;CHECK(pulse.Step(q)==256,"release has a minimum interval before retry");
    q.now=124;CHECK(pulse.Step(q)==65537,"a missed takeoff can retry while still measured grounded");
    q.now=128;q.ground=false;CHECK(pulse.Step(q)==256&&!pulse.down,"airborne state releases the owned jump");
    q.now=160;q.ground=true;CHECK(pulse.Step(q)==65537,"a new landing starts the next pulse while Space stays held");
    q.now=170;q.context=false;CHECK(pulse.Step(q)==256&&!pulse.owned,"loss of focus or alive context releases ownership");
    CHECK(pulse.Step(q)==-1,"inactive contexts cannot repeatedly write the jump channel");
    q.context=true;q.now=200;q.walking=false;CHECK(pulse.Step(q)==-1,"ladder noclip and unknown movement modes cannot start bhop");
    q.walking=true;CHECK(pulse.Step(q)==65537,"valid walking context can reacquire jump");
    q.pawn=2;q.now=205;CHECK(pulse.Step(q)==256&&!pulse.owned,"pawn replacement releases the previous press before reacquisition");
    q.now=210;CHECK(pulse.Step(q)==65537,"the replacement pawn is accepted only on a later step");
    q.now=190;CHECK(pulse.Step(q)==256&&!pulse.owned,"clock rollback cannot underflow the hold deadline");
    pulse.Reset();q={1,true,true,true,true,false,true,100};pulse.Step(q);
    q.now=200;CHECK(pulse.Step(q)==256,"a low-FPS frame still emits the release edge");
    q.now=234;CHECK(pulse.Step(q)==65537,"skipping the entire airborne interval cannot latch bhop forever");
    q.hold=false;q.now=250;CHECK(pulse.Step(q)==256,"disabling while pressed completes its release phase");
    q.now=251;CHECK(pulse.Step(q)==65537&&!pulse.owned,"disabling restores a physically held Space instead of suppressing manual input");
    q.now=252;CHECK(pulse.Step(q)==-1,"manual Space remains under native ownership after restoration");
    pulse.Reset();q={1,true,true,true,false,true,false,300};
    CHECK(pulse.Step(q)==65537,"a one-shot edge or lineup request starts a complete pulse");
    q.request=false;q.now=304;CHECK(pulse.Step(q)==65537,"one-shot requests are retained after the requesting frame");
    q.now=316;CHECK(pulse.Step(q)==256,"one-shot pulse releases after its hold");
    q.now=317;CHECK(pulse.Step(q)==256&&!pulse.owned,"completed one-shot releases its ownership");
    q.now=340;CHECK(pulse.Step(q)==-1,"a consumed one-shot cannot repeat without a new request");
    pulse.Reset();q={1,true,true,false,false,true,false,400};
    CHECK(pulse.Step(q)==-1&&!pulse.owned,"an airborne one-shot cannot synthesize a ground jump");
    unsigned presses=0,releases=0;pulse.Reset();q={1,true,true,true,true,false,true,500};
    for(unsigned i=0;i<1000;++i){q.now=500+i*4;int state=pulse.Step(q);presses+=state==65537;releases+=state==256;}
    CHECK(presses>1&&releases>1,"long held-Space runs keep making progress without a ground latch");
    q.now=4500;q.pawn=0;CHECK(pulse.Step(q)==256&&!pulse.owned,"a retired pawn clears the last owned press");
    pulse.WriteFailed();CHECK(pulse.restorePending&&pulse.owned,"a failed write retains a pending cleanup obligation");
    CHECK(pulse.Step(q)==256&&!pulse.owned,"failed cleanup is retried even without a valid pawn");
    pulse.WriteFailed();q={1,true,true,true,true,false,true,4600};
    CHECK(pulse.Step(q)==256,"a failed press must clean up before a new press can start");
    q.now=4604;CHECK(pulse.Step(q)==65537,"a successful cleanup allows a later valid retry");
    lineupBusy.store(true);CHECK(lineupBusy.load(),"lineup intent is available to combat and movement command owners");
    lineupBusy.store(false);CHECK(!lineupBusy.load(),"lineup intent can be cleared without touching any input device");
    printf("RESULT: %u passed, %u failed; pure decisions only.\n",passed,failed);
    return failed?1:0;
}
