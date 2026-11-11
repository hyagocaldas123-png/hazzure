#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include <limits>
#include "../src/CombatAccuracy.h"
#include "../src/WeaponAccuracy.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    using namespace HazzureAccuracy;
    Capsule target{{100,0,0},{100,0,0},5,6};Vec3 eye{};
    auto hit=Evaluate(eye,{},0,0,1000,&target,1);
    CHECK(hit.valid&&hit.percent==100,"zero spread aimed at a target hits its volume");
    CHECK(Evaluate(eye,{0,90,0},0,0,1000,&target,1).percent==0,"aim direction matters even when dispersion is zero");
    Capsule behind{{-100,0,0},{-100,0,0},5};
    CHECK(Evaluate(eye,{},0,0,1000,&behind,1).percent==0,"targets behind the eye cannot satisfy the shot gate");
    CHECK(Evaluate(eye,{},0,0,50,&target,1).percent==0,"weapon range limits the ray");
    Capsule tall{{100,0,-40},{100,0,40},3};
    CHECK(Intersects(eye,{1,0,0},1000,tall),"world-space ray intersects a non-spherical capsule");
    CHECK(!Intersects({0,8,0},{1,0,0},1000,tall),"parallel offset ray misses the same capsule");
    auto deterministic=Evaluate(eye,{},.1f,.03f,1000,&target,1,256);
    CHECK(deterministic.hits==Evaluate(eye,{},.1f,.03f,1000,&target,1,256).hits,"identical snapshots do not reroll the decision every frame");
    CHECK(deterministic.percent>0&&deterministic.percent<100,"wide spread produces a bounded partial result");
    Capsule large=target;large.radius=15;
    CHECK(Evaluate(eye,{},.1f,.03f,1000,&large,1,256).hits>=deterministic.hits,"larger collision volume cannot lose the same sampled rays");
    Capsule distant=target;distant.a.x=distant.b.x=1000;
    CHECK(Evaluate(eye,{},.1f,.03f,2000,&distant,1,256).hits<=deterministic.hits,"apparent size decreases with distance");
    CHECK(!Evaluate(eye,{},-1,0,1000,&target,1).valid,"negative dispersion is unavailable rather than a perfect shot");
    float nan=std::numeric_limits<float>::quiet_NaN();
    CHECK(!Evaluate(eye,{},nan,0,1000,&target,1).valid&&!Evaluate(eye,{nan,0,0},0,0,1000,&target,1).valid,"nonfinite spread and angles are rejected");
    CHECK(!Evaluate(eye,{},0,0,1000,nullptr,1).valid&&!Evaluate(eye,{},0,0,1000,&target,0).valid,"missing geometry never permits a shot");
    CHECK(!Evaluate(eye,{},0,0,1000,&target,1,0).valid&&!Evaluate(eye,{},0,0,1000,&target,1,257).valid,"sample counts are bounded without division by zero");
    CHECK(!Pass({},0)&&!Pass(hit,nan)&&!Pass(hit,101)&&Pass(hit,100),"invalid estimates and thresholds fail closed, inclusive 100 percent works");
    Capsule invalid=target;invalid.radius=0;
    CHECK(!Evaluate(eye,{},0,0,1000,&invalid,1).valid,"empty collision volumes are rejected");
    auto points=Multipoints(target,eye,.8f);
    bool inside=points.count==7;for(size_t i=0;i<points.count;++i)inside&=DistanceSquared(points.values[i],points.values[i],target.a,target.b)<=25.001f;
    CHECK(inside,"all generated multipoints remain inside the capsule");
    CHECK(Multipoints(target,eye,0).count==1&&!Multipoints(target,eye,nan).count,"zero scale yields one centre and invalid scale yields no point");
    auto w=CounterStrafe({250,0,0},0,75);
    CHECK(w.valid&&w.brake&&w.forward<-.99f&&std::fabs(w.side)<.001f,"world X speed maps to backward at yaw zero");
    auto n=CounterStrafe({250,0,0},90,75);
    CHECK(n.valid&&n.brake&&n.side<-.99f&&std::fabs(n.forward)<.001f,"counter-strafe uses Source's right vector in view coordinates");
    CHECK(!CounterStrafe({75,0,0},0,75).brake&&!CounterStrafe({0,0,500},0,75).brake,"speed gate uses measured horizontal speed and inclusive limit");
    CHECK(!CounterStrafe({nan,0,0},0,75).valid&&!CounterStrafe({},0,nan).valid,"bad movement data cannot authorize braking");
    HazzureWeaponAccuracy::Snapshot snapshot{1,2,3,100,.01f,.005f,250,8192,0,1,true};
    CHECK(HazzureWeaponAccuracy::Values(snapshot),"native single-projectile snapshot has bounded accuracy inputs");
    CHECK(HazzureWeaponAccuracy::Fresh(snapshot,1,2,3,132)&&!HazzureWeaponAccuracy::Fresh(snapshot,1,2,3,133),"snapshot lifetime is bounded to 32 ms");
    CHECK(!HazzureWeaponAccuracy::Fresh(snapshot,1,2,4,101)&&!HazzureWeaponAccuracy::Fresh(snapshot,9,2,3,101),"pawn and weapon handle changes invalidate a snapshot");
    CHECK(!HazzureWeaponAccuracy::Fresh(snapshot,1,2,3,99),"clock rollback rejects the cached result");
    snapshot.bullets=8;CHECK(!HazzureWeaponAccuracy::Values(snapshot),"shotgun patterns are not silently treated as one independent bullet");
    HazzureWeaponAccuracy::Publish(snapshot);HazzureWeaponAccuracy::Clear();
    CHECK(!HazzureWeaponAccuracy::Read(1,2,3,100).valid,"session clearing removes old native data");
    printf("RESULT: %u passed, %u failed; own process, no OS input or engine calls.\n",passed,failed);return failed?1:0;
}
