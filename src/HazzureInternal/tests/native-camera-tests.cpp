// Pure/synthetic camera regressions: no system input, window or game attachment.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <windows.h>
#include <cstdio>
#include <array>
#include "../src/NativeCamera.h"
static unsigned passed{},failed{};
#define CHECK(v,text) do{if(v){++passed;}else{++failed;printf("FAIL: %s\n",text);}}while(0)
using namespace HazzureNativeCamera;
struct TraceState {float fraction=1;bool solid{},available=true;unsigned calls{};};
static bool Trace(void* c,const Vec3&,const Vec3&,HazzureWorldTrace::Result& r){auto* t=static_cast<TraceState*>(c);++t->calls;r={t->fraction,t->solid};return t->available;}
int main(){
    View native{};native.width=1920;native.height=1080;native.fov=90;native.handFov=68;native.aspect=4.f/3.f;native.origin={10,20,64};native.originA=native.originB=native.origin;
    Settings s;s.allowed=true;s.thirdperson=true;s.distance=120;View out{};float distance{};TraceState trace;
    CHECK(Compose(native,s,Trace,&trace,out,distance)&&distance==120&&std::fabs(out.origin.x+110)<.001f,"open hull uses configured distance without altering angles");
    CHECK(out.origin.y==20&&out.origin.z==64&&out.angles.x==native.angles.x,"third-person offset follows Source forward convention");
    CHECK(out.originA.x==out.origin.x&&out.originB.x==out.origin.x,"native origin copies stay coherent");
    trace.fraction=.5f;CHECK(Compose(native,s,Trace,&trace,out,distance)&&distance==56&&out.origin.x==-46,"wall contact shortens camera with safety margin");
    trace.solid=true;CHECK(Compose(native,s,Trace,&trace,out,distance)&&distance==0&&out.origin.x==native.origin.x,"start-solid retains the native eye");
    trace.solid=false;trace.available=false;CHECK(!Compose(native,s,Trace,&trace,out,distance),"unknown collision never authorizes a third-person offset");
    s.aspectOverride=true;s.ratio=16.f/9.f;
    CHECK(Compose(native,s,Trace,&trace,out,distance)&&distance<0&&out.aspect==s.ratio&&out.origin.x==native.origin.x,"unknown third-person collision does not disable an independent native aspect override");
    s.thirdperson=false;s.aspectOverride=true;s.ratio=16.f/9.f;
    CHECK(Compose(native,s,nullptr,nullptr,out,distance)&&std::fabs(out.fov-106.2602f)<.001f&&out.aspect==s.ratio,"ratio rebases native projection FOV rather than overlay pixels");
    CHECK(std::fabs(RebaseFov(out.fov,s.ratio,native.aspect)-native.fov)<.001f,"projection ratio conversion is reversible");
    CHECK(out.origin.x==native.origin.x&&out.origin.y==native.origin.y,"ratio-only preserves camera origin");
    s.ratio=std::nanf("");CHECK(!Compose(native,s,nullptr,nullptr,out,distance),"non-finite options preserve native view");
    s.ratio=1;s.distance=301;CHECK(!Compose(native,s,nullptr,nullptr,out,distance),"distance outside configured bounds is rejected");
    s.distance=120;native.fov=180;CHECK(!Compose(native,s,nullptr,nullptr,out,distance),"invalid native view cannot reach trig functions");native.fov=90;
    Activation key;
    CHECK(key.Update(true,0,false,true),"unbound camera follows its enable toggle");
    CHECK(!key.Update(true,86,true,true),"changing binding while held cannot activate camera");
    CHECK(!key.Update(true,86,false,true)&&key.Update(true,86,true,true),"configured key toggles on its rising edge");
    CHECK(key.Update(true,86,true,true),"holding toggle key does not oscillate");
    CHECK(key.Update(true,86,false,true)&&!key.Update(true,86,true,true),"second press returns to native first-person");
    CHECK(!key.Update(true,86,true,false),"loss of gameplay context cancels bound camera");
    CHECK(!key.Update(false,0,false,true),"disabled option cannot activate camera");
    std::array<unsigned char,0x700> bytes{};auto at=reinterpret_cast<uintptr_t>(bytes.data());
    Put(at+0x434,native.width);Put(at+0x43c,native.height);Put(at+0x498,native.fov);Put(at+0x49c,native.handFov);Put(at+0x4d4,native.aspect);
    Put(at+0x4a0,native.origin);Put(at+0x4b8,native.angles);Put(at+0x554,native.originA);Put(at+0x56c,native.originB);
    View captured;CHECK(ReadView(at,captured)&&captured.origin.x==10,"view offsets read the reviewed native layout");
    s={};s.aspectOverride=true;s.ratio=16.f/9.f;Compose(native,s,nullptr,nullptr,out,distance);
    CHECK(Commit(at,native,out)&&ReadView(at,captured)&&captured.aspect==s.ratio&&captured.fov>106,"commit writes FOV and aspect in the native setup");
    CHECK(bytes[0x497]==0&&bytes[0x4d8]==0&&bytes[0x578]==0,"camera commit leaves adjacent fields untouched");
    CHECK(!DrawLocal(0,0,true,1,1000),"body draw bypass is closed without a delivered third-person view");
    bodyAt.store(1000);bodyEpoch.store(2);bodyPawn.store(0x1234);
    CHECK(!DrawLocal(0,0x1234,true,1,1000)&&!DrawLocal(0,0x1234,false,2,1000)&&!DrawLocal(0,0x1234,true,2,1101),"body bypass rejects stale epoch, unavailable world and stale camera");ClearBody();
    printf("RESULT: %u passed, %u failed; pure/synthetic, no input or game process.\n",passed,failed);return failed?1:0;
}
