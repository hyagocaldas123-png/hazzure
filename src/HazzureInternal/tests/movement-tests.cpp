// Movement decisions and cache regressions in our process only. No native engine/input calls.
#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include <limits>
#include "../src/MovementTech.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    using namespace HazzureMove;
    EdgeGate gate;
    CHECK(!gate.Update(100,true,true,true,250,Support::Ground),"ground support does not trigger");
    CHECK(gate.Update(100,true,true,true,250,Support::Edge),"predicted ledge requests a jump");
    CHECK(!gate.Update(100,true,true,true,250,Support::Edge),"the same ground cycle does not repeat the edge pulse");
    CHECK(!gate.Update(100,true,false,true,250,Support::Unavailable),"airborne frame updates without requesting another jump");
    CHECK(gate.Update(100,true,true,true,250,Support::Edge),"landing rearms the next ledge");
    CHECK(!gate.Update(100,true,true,true,250,Support::Ground)&&gate.armed,"returning to solid support rearms a missed jump");
    CHECK(!gate.Update(100,true,true,true,250,Support::Unavailable),"a failed physics request cannot authorize an edge jump");
    CHECK(!gate.Update(100,false,true,true,250,Support::Edge)&&gate.armed,"disabling resets the gate");
    CHECK(!gate.Update(200,true,true,false,250,Support::Edge),"ladder and noclip movement do not trigger walking edges");
    CHECK(!gate.Update(200,true,true,true,40,Support::Edge),"standing or slow movement is ignored");
    CHECK(!gate.Update(200,true,true,true,std::numeric_limits<float>::quiet_NaN(),Support::Edge),"invalid speed is ignored");

    Vec3 start{},end{};
    CHECK(ProbeSegment({1000,2000,100},{250,0,0},0,start,end)&&std::fabs(start.x-1006)<.001f&&
        start.y==2000&&start.z==104&&end.z==82,"probe starts at actual pawn world origin, not its velocity");
    CHECK(ProbeSegment({1000,2000,100},{250,0,0},12,start,end)&&start.y==2012,"support probes span the lateral footprint");
    CHECK(ProbeSegment({1000,2000,100},{1000,0,0},0,start,end)&&start.x==1020,"lookahead remains bounded at high speed");
    CHECK(!ProbeSegment({},{0,0,0},0,start,end),"zero speed cannot create an arbitrary probe");
    CHECK(!ProbeSegment({std::numeric_limits<float>::infinity(),0,0},{250,0,0},0,start,end),"nonfinite origin cannot reach physics");
    auto trace=EmptyProbeTrace();
    CHECK(ClassifySupport(trace)==Support::Unavailable,"a backend that leaves the native result untouched is unavailable, not false ground support");
    trace.fraction=std::numeric_limits<float>::quiet_NaN();
    CHECK(ClassifySupport(trace)==Support::Unavailable,"nonfinite native fractions cannot authorize or rearm a ledge");
    trace.fraction=1.01f;
    CHECK(ClassifySupport(trace)==Support::Unavailable,"out of range native fractions are rejected");
    trace.fraction=1;trace.solid=true;
    CHECK(ClassifySupport(trace)==Support::Unavailable,"a start-solid support query is unavailable");
    trace.solid=false;trace.fraction=.5f;
    CHECK(ClassifySupport(trace)==Support::Ground,"a measured downward collision establishes support");
    trace.fraction=1;
    CHECK(ClassifySupport(trace)==Support::Edge,"a fully initialized clear downward trace establishes missing support");

    SupportSample sample{100,200,{1000,2000,100},{250,0,0},1000,Support::Edge};
    CHECK(sample.Read(100,200,{1004,2000,100},{250,0,0},1016)==Support::Edge,"fresh frame result permits bounded normal movement");
    CHECK(sample.Read(100,200,{1000,2000,100},{250,0,0},1025)==Support::Unavailable,"stale physics results expire");
    CHECK(sample.Read(100,200,{1000,2000,100},{250,0,0},999)==Support::Unavailable,"clock rollback rejects old results");
    CHECK(sample.Read(101,200,{1000,2000,100},{250,0,0},1010)==Support::Unavailable,"a different pawn cannot consume cached support");
    CHECK(sample.Read(100,201,{1000,2000,100},{250,0,0},1010)==Support::Unavailable,"scene changes invalidate support even when pawn address is reused");
    CHECK(sample.Read(100,200,{1100,2000,100},{250,0,0},1010)==Support::Unavailable,"teleports reject old support");
    CHECK(sample.Read(100,200,{1000,2000,105},{250,0,0},1010)==Support::Unavailable,"vertical displacement rejects old support");
    CHECK(sample.Read(100,200,{1000,2000,100},{0,250,0},1010)==Support::Unavailable,"a sharp turn cannot use the previous direction probe");
    PublishSupport(sample);
    CHECK(ReadSupport(100,200,{1000,2000,100},{250,0,0},1001)==Support::Edge,"support is transferred once through the protected frame cache");
    PublishSupport({});
    CHECK(ReadSupport(100,200,{1000,2000,100},{250,0,0},1001)==Support::Unavailable,"stopping clears pending frame support");

    Vec3 target{};
    CHECK(LadderTarget({1,0,0},target)&&target.x==-89&&std::fabs(target.y-135)<.01f,"ladder alignment follows the measured surface normal");
    CHECK(LadderTarget({-1,0,0},target)&&std::fabs(target.y+45)<.01f,"opposite ladder normal rotates the target direction");
    CHECK(!LadderTarget({},target)&&!LadderTarget({1,0,std::numeric_limits<float>::quiet_NaN()},target),"invalid ladder normals cannot move the view");
    CHECK(NeedsOwnedPress(false,true,256),"a newly owned ladder direction issues its initial press");
    CHECK(!NeedsOwnedPress(true,true,65537)&&!NeedsOwnedPress(true,true,1),"a still held direction is not pressed again every callback");
    CHECK(NeedsOwnedPress(true,true,256)&&NeedsOwnedPress(true,true,0),"an engine-cleared owned direction is reasserted on the next callback");
    CHECK(NeedsOwnedPress(true,false,65537),"an unreadable state must pass through the validated button path again");
    CHECK(!CanRestoreView(false,false,{-89,135,0},{-89,135,0}),"focus or context loss prohibits restoration input");
    CHECK(!CanRestoreView(true,true,{-89,135,0},{-89,135,0}),"manual movement prohibits restoration input");
    CHECK(!CanRestoreView(true,false,{-30,135,0},{-89,135,0}),"a user view override cancels automatic restoration");
    CHECK(CanRestoreView(true,false,{-89,179,0},{-89,-179,0}),"view comparison handles wrapped yaw without a false override");
    Settings option;option.yawUnit=option.pitchUnit=.044f;
    Vec3 current{-89,135,0},restore{0,0,0};mouse.Reset();
    for(int i=0;i<20;++i){if(!SendAngles(current,restore,option,12)){break;}current=lastSteer;}
    CHECK(std::fabs(current.x)<.08f&&std::fabs(HazzureGame::NormalizeAngle(current.y))<.08f,
        "bounded multi-frame restoration reaches original view without the old 1000-count truncation");
    option.pitchUnit=0;
    CHECK(!SendAngles(current,restore,option,12),"invalid mouse calibration cannot send steering input");
    printf("RESULT: %u passed, %u failed; own process, no engine calls or OS input.\n",passed,failed);
    return failed?1:0;
}
