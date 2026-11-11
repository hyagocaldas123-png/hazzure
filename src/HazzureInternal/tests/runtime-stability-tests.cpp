#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include <limits>
#include "../src/RuntimeStability.h"
#include "../src/WorldSession.h"
#include "../src/InputView.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    using namespace HazzureStable;
    AimTracker a,b;Vec2 p{},q{};
    for(int i=0;i<12;++i){auto step=a.Step(1,{5-p.x,2-p.y},4.5f,.35f,1.f/60);p.x+=step.x;p.y+=step.y;}
    for(int i=0;i<48;++i){auto step=b.Step(1,{5-q.x,2-q.y},4.5f,.35f,1.f/240);q.x+=step.x;q.y+=step.y;}
    CHECK(std::fabs(p.x-q.x)<.0001f&&std::fabs(p.y-q.y)<.0001f,"critical tracking has the same progress at 60 and 240 FPS");
    float omega=120.f/4.5f*(1-.35f*.5f),expected=5*(1-(1+omega*.2f)*std::exp(-omega*.2f));
    CHECK(std::fabs(p.x-expected)<.0001f,"fixed target tracking matches the analytic critically damped solution");
    auto reverse=a.Step(1,{-1,0},4.5f,.35f,1.f/60);
    CHECK(reverse.x<0&&reverse.x>=-1,"a reversing target does not pull along the old acquisition arc");
    bool bounded=true;Vec2 position{};AimTracker follow;
    for(int i=0;i<150;++i){Vec2 error{1-position.x,-2-position.y};auto step=follow.Step(7,error,4.5f,.35f,1.f/120);
        bounded&=step.x>=0&&step.x<=error.x+.000001f&&step.y<=0&&step.y>=error.y-.000001f;
        position.x+=step.x;position.y+=step.y;}
    CHECK(bounded&&std::fabs(position.x-1)<.0001f,"tracking converges without oscillation or component overshoot");
    follow.Step(0,{1,1},4.5f,.35f,1.f/60);CHECK(!follow.target,"loss of target clears tracking velocity");
    auto invalid=follow.Step(1,{1,1},4.5f,.35f,std::numeric_limits<float>::quiet_NaN());
    CHECK(invalid.x==0&&!follow.target,"invalid timing cannot emit stale tracking motion");
    CHECK(TargetScore(1,true)<TargetScore(.9f,false)&&TargetScore(1,true)>TargetScore(.5f,false),"target hysteresis ignores tiny rank changes but permits a clearly better enemy");
    ShotPause pause;pause.Fired(500,100,350);
    CHECK(!pause.Ready(600,100,true)&&!pause.Ready(849,200,true),"post-shot pause survives target or weapon changes");
    CHECK(!pause.Ready(850,200,false)&&pause.Ready(850,200,true),"shot requires both elapsed recovery and settled recoil");
    pause.Fired(1000,100,0);CHECK(!pause.Ready(1079,100,true)&&pause.Ready(1080,100,true),"even zero reaction delay cannot create a frame-rate firing loop");
    CHECK(!pause.Ready(900,100,true)&&pause.until==0,"clock rollback resets a stale firing deadline");
    using HazzureWorld::Gate;Gate world;
    CHECK(world.Observe(1,2,true,100,1000)&&!world.ready,"new world waits before exposing native writers");
    CHECK(!world.Observe(1,2,true,100.1f,1250)&&world.ready,"stable world becomes ready after the settling interval");
    CHECK(world.Observe(0,0,false,0,1300)&&!world.ready,"leaving training immediately closes the world gate");
    CHECK(world.Observe(1,3,true,1,1400)&&!world.ready,"next map cannot reuse the previous resource session");
    world.Observe(1,3,true,1.1f,1650);
    CHECK(world.Observe(1,3,true,0,1700)&&!world.ready,"simulation rollback invalidates caches even when pointers are reused");
    CHECK(HazzureWorld::MaterialNameMatches("materials/skybox/a.vmat_c","materials/skybox/a.vmat")&&
          !HazzureWorld::MaterialNameMatches("materials/dev/error.vmat","materials/skybox/a.vmat"),"error material cannot substitute a requested sky resource");
    CHECK(HazzureGame::CanonicalHandle(0x18001,1)==0x10001&&HazzureGame::CanonicalHandle(0xffffffff,1)==0xffffffff,"entity lookup normalizes serial flags while preserving invalid handles");
    alignas(8) unsigned char input[0xb60]{},history[0x880]{};
    auto address=reinterpret_cast<uintptr_t>(input);auto records=reinterpret_cast<uintptr_t>(history);
    HazzureGame::Vec3 fallback{1,2,0},latest{10,370,0},view{};
    memcpy(input+0x688,&fallback,sizeof(fallback));
    CHECK(HazzureInputView::ReadSnapshot(address,view)&&view.x==1&&view.y==2,"empty input history uses the camera fallback");
    int count=2;memcpy(input+0xb50,&count,4);memcpy(input+0xb58,&records,8);memcpy(history+0x440+0x430,&latest,sizeof(latest));
    CHECK(HazzureInputView::ReadSnapshot(address,view)&&view.x==10&&view.y==10,"active input reads the newest camera history rather than stale fallback or networked pawn angles");
    count=5000;memcpy(input+0xb50,&count,4);
    CHECK(!HazzureInputView::ReadSnapshot(address,view),"malformed history bounds cannot produce a camera correction");
    printf("RESULT: %u passed, %u failed; own process, no engine calls or OS input.\n",passed,failed);return failed?1:0;
}
