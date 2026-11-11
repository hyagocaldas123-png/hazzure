// Own process, synthetic planes only. No game process, native engine calls or OS input.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include <limits>
#include "../src/GrenadePhysics.h"
using namespace HazzureGrenade;
static unsigned passed{},failed{};
#define CHECK(condition,label) do{if(condition){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
static bool Near(float a,float b,float tolerance=.02f){return std::fabs(a-b)<=tolerance;}
struct Plane {Vec3 normal{};float distance{};};
struct Scene {std::array<Plane,8> planes{};unsigned size{},queries{};bool fail{},solid{},badNormal{},zero{};};
static bool QueryScene(void* context,Vec3 start,Vec3 end,Hit& hit){
    auto& scene=*static_cast<Scene*>(context);++scene.queries;hit={};
    if(scene.fail)return false;if(scene.solid){hit.solid=true;return true;}
    if(scene.zero){hit.fraction=0;hit.normal={-1,0,0};return true;}
    for(unsigned i=0;i<scene.size;++i){const auto& plane=scene.planes[i];
        float a=Dot(plane.normal,start)-plane.distance,b=Dot(plane.normal,end)-plane.distance;
        if(a<-.01f){hit.solid=true;return true;}
        if(a>=0&&b<0){float fraction=a/(a-b);if(fraction<hit.fraction){hit.fraction=fraction;hit.normal=plane.normal;}}
    }
    if(scene.badNormal&&hit.fraction<1)hit.normal={};return true;
}
static Request Base(uint16_t definition=44){Request q{};q.pawn=1;q.weapon=2;q.list=3;q.weaponHandle=7;
    q.definition=definition;q.eye={0,0,64};q.forward={1,0,0};q.simulation=100;return q;}
static Simulation Run(const Request& q,Scene& scene,unsigned budget=24,Parameters p={}){
    Simulation simulation;simulation.Begin(q,p);for(unsigned frame=0;frame<1000&&!simulation.result.complete;++frame)simulation.Advance(QueryScene,&scene,budget);return simulation;
}
int main(){
    CHECK(Type(45)==Kind::Smoke&&Type(48)==Kind::Fire&&Type(7)==Kind::None,"only supported grenade definitions start predictions");
    auto q=Base();CHECK(Valid(q),"finite active grenade request is valid");q.definition=7;CHECK(!Valid(q),"ordinary weapon cannot display a grenade preview");q=Base();
    Vec3 reflected=Reflect({100,50,-20},{-1,0,0},.45f);
    CHECK(Near(reflected.x,-45)&&Near(reflected.y,22.5f)&&Near(reflected.z,-9),"wall reflection reverses normal component and preserves tangent direction");
    reflected=Reflect({10,20,-100},{0,0,1},.45f);
    CHECK(Near(reflected.z,45)&&Near(reflected.x,4.5f),"floor reflection uses measured plane rather than a fixed world height");
    reflected=Reflect({0,0,100},{0,0,-1},.45f);CHECK(Near(reflected.z,-45),"ceiling reflection sends an ascending grenade down");
    Vec3 diagonal{-.70710678f,-.70710678f,0};reflected=Reflect({100,0,0},diagonal,.45f);
    CHECK(Near(reflected.x,0)&&Near(reflected.y,-45),"angled wall ricochet follows its collision normal");
    reflected=Reflect({-100,0,0},{-1,0,0},.45f);CHECK(Near(reflected.x,-100),"velocity already leaving a wall is not reflected back into it");
    Scene empty;auto free=Run(q,empty);
    CHECK(free.result.complete&&free.result.finish==Finish::DetonationEstimate&&Near(free.result.time,1.5f,.0001f),"HE flight stops at its estimated fuse rather than a fixed number of render frames");
    auto endpoint=free.result.points[free.result.count-1];
    CHECK(Near(endpoint.x,16.f+675.f*1.5f,.1f)&&Near(endpoint.z,64.f-.5f*320.f*1.5f*1.5f,.1f),"free flight matches analytic gravity integration without artificial horizontal drag");
    Scene chunkedScene;auto chunked=Run(q,chunkedScene,1);
    CHECK(chunked.result.count==free.result.count&&Separation(chunked.result.points[chunked.result.count-1],endpoint)<.001f,"incremental query budgets do not change the predicted endpoint");
    Simulation budgeted;budgeted.Begin(q);Scene budgetScene;auto queries=budgeted.Advance(QueryScene,&budgetScene,3);
    CHECK(queries==3&&budgetScene.queries==3&&!budgeted.result.complete,"each callback honors its physics query budget including release clearance");
    Scene wall;wall.planes[wall.size++]={{-1,0,0},-120};auto wallFlight=Run(q,wall,1);
    CHECK(wallFlight.result.bounceCount==1&&Near(wallFlight.result.bounces[0].x,120),"wall intersection is placed at the physical hit fraction");
    CHECK(wallFlight.result.points[wallFlight.result.count-1].x<120,"residual sweep after a wall collision continues away from the wall");
    Scene corner;corner.planes[corner.size++]={{-1,0,0},-100};corner.planes[corner.size++]={{0,-1,0},-100};
    auto diagonalQ=q;diagonalQ.forward={.70710678f,.70710678f,0};auto cornerFlight=Run(diagonalQ,corner,1);
    CHECK(cornerFlight.result.bounceCount>=2&&cornerFlight.result.finish!=Finish::Unavailable,"corner contacts can consume separate callback budgets without tunneling");
    Scene ceiling;ceiling.planes[ceiling.size++]={{0,0,-1},-100};auto up=q;up.forward={.8f,0,.6f};auto ceilingFlight=Run(up,ceiling);
    CHECK(ceilingFlight.result.bounceCount>0&&Near(ceilingFlight.result.bounces[0].z,100),"ceiling collision is traced above the local origin");
    Scene floor;floor.planes[floor.size++]={{0,0,1},0};auto fire=Run(Base(46),floor);
    CHECK(fire.result.finish==Finish::DetonationEstimate&&fire.result.time<1.5f&&Near(fire.result.points[fire.result.count-1].z,0),"fire grenade estimates detonation at a sufficiently level ground impact");
    Scene fireWall;fireWall.planes[fireWall.size++]={{-1,0,0},-120};auto fireSide=Run(Base(46),fireWall);
    CHECK(fireSide.result.bounceCount>0&&Near(fireSide.result.time,3.5f,.001f),"vertical wall contact does not detonate a fire grenade as though it were floor");
    Scene smokeFloor;smokeFloor.planes[smokeFloor.size++]={{0,0,1},0};auto smoke=Run(Base(45),smokeFloor);
    CHECK(smoke.result.complete&&smoke.result.finish==Finish::RestEstimate&&smoke.result.time<6,"smoke reports a rest estimate after physical bounces and low-speed ground contact");
    Scene clearSmoke;auto horizon=Run(Base(45),clearSmoke);CHECK(horizon.result.finish==Finish::Horizon,"a still airborne smoke endpoint is marked as horizon rather than bloom");
    Scene solid;solid.solid=true;auto invalid=Run(q,solid);CHECK(invalid.result.finish==Finish::Unavailable,"starting inside geometry never invents a floor or endpoint");
    Scene unavailable;unavailable.fail=true;invalid=Run(q,unavailable);CHECK(invalid.result.finish==Finish::Unavailable&&invalid.result.bounceCount==0,"missing physics backend has no flat-plane fallback");
    Scene normal;normal.badNormal=true;normal.planes[normal.size++]={{-1,0,0},-120};invalid=Run(q,normal);
    CHECK(invalid.result.finish==Finish::Unavailable,"invalid native collision normal invalidates the path");
    Scene stuck;stuck.zero=true;invalid=Run(q,stuck);CHECK(invalid.result.finish==Finish::Stuck&&stuck.queries<=5,"repeated zero-progress contacts terminate instead of spinning in a corner");
    Parameters invalidParameters;invalidParameters.gravity=std::numeric_limits<float>::quiet_NaN();Simulation bad;
    CHECK(!bad.Begin(q,invalidParameters)&&bad.result.finish==Finish::Unavailable,"non-finite physics parameters are rejected");
    auto moved=q;moved.eye.x+=3;CHECK(!Same(q,moved),"player displacement invalidates prior trajectory geometry");
    moved=q;moved.epoch++;CHECK(!Same(q,moved),"round transition invalidates a cached path even if pointers are reused");
    moved=q;moved.simulation=1;CHECK(!Same(q,moved),"simulation clock rollback invalidates the prior map path");
    moved=q;moved.weaponHandle++;CHECK(!Same(q,moved),"weapon identity change invalidates the cached trajectory");
    CHECK((collisionMask&(1ull<<3))==0&&(collisionMask&(1ull<<4))==0&&(collisionMask&(1ull<<33))!=0,"grenade mask excludes sky and player clip and includes grenade clip");
    Channel local;Result output{};auto now=GetTickCount64();Scene channelScene;
    CHECK(!local.Submit(q,now,output)&&channelScene.queries==0,"render-side submission cannot call the physics engine");
    local.evaluated=q;local.output=free.result;local.completed=now;
    CHECK(local.Submit(q,now,output)&&output.complete,"matching fresh trajectory snapshot can be displayed");
    CHECK(!local.Submit(q,now+201,output),"expired result is not displayed through a changed world");
    local.Clear();CHECK(!local.active&&local.output.count==0,"disable clears the shared output immediately");
    channel.Clear();work={};workingGeneration=0;channel.Submit(q,GetTickCount64(),output);Scene queued;
    ProcessPending(0,QueryScene,&queued,2);
    CHECK(queued.queries==2&&channel.Submit(q,GetTickCount64(),output)&&output.count>1&&!output.complete,"frame consumer publishes bounded partial paths without holding the render lock during queries");
    channel.Clear();ProcessPending(0,QueryScene,&queued,2);CHECK(!channel.active&&work.result.count==0,"disable prevents work from resurrecting an older path");
    NativeContext native{};CHECK(!PrepareNative(0,q,native),"isolated tests cannot execute native engine query code");
    printf("%u passed, %u failed; synthetic own process, no native engine calls or OS input.\n",passed,failed);return failed?1:0;
}
