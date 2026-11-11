// C++ / visibility-recoil-tests.cpp / MSVC x64 C++17 / own synthetic memory only
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <vector>
#include <cstdio>
#include <limits>
#include "../src/Visibility.h"
#include "../src/VisualRecoil.h"
static unsigned passed{},failed{},queries{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
static HazzureVisibility::Verdict MockQuery(uintptr_t,const HazzureVisibility::Request& q){++queries;return q.target==2?HazzureVisibility::Verdict::Visible:HazzureVisibility::Verdict::Blocked;}
int main(){
 using namespace HazzureVisibility;
 Trace trace{};trace.fraction=1;
 CHECK(Classify(trace,2)==Verdict::Visible,"unobstructed ray is visible");
 trace.fraction=.9999f;CHECK(Classify(trace,2)==Verdict::Blocked,"a wall close to the target is still an obstruction");
 trace.fraction=.4f;trace.entity=2;CHECK(Classify(trace,2)==Verdict::Visible,"first collision with the selected target is accepted");
 trace.entity=3;CHECK(Classify(trace,2)==Verdict::Blocked,"another entity blocks acquisition");
 trace.entity=2;trace.solid=true;CHECK(Classify(trace,2)==Verdict::Blocked,"starting in solid geometry does not lock on");
 trace.solid=false;trace.fraction=std::numeric_limits<float>::quiet_NaN();CHECK(Classify(trace,2)==Verdict::Unavailable,"invalid trace output fails closed");
 Request request{1,2,{0,0,64},{100,0,64}};uint64_t now=GetTickCount64();
 CHECK(Submit(request,now)==Verdict::Unknown&&queries==0,"render-side submission never executes native physics");
 ProcessPending(0,MockQuery);now=GetTickCount64();
 CHECK(queries==1&&Submit(request,now)==Verdict::Visible,"frame callback publishes the matching visibility result");
 CHECK(Submit(request,now+33)==Verdict::Unknown,"old visibility cannot keep aim attached through a new obstruction");
 request.end.x+=3;CHECK(Submit(request,now)==Verdict::Unknown,"target displacement invalidates the cached ray");
 request.target=3;Submit(request,now);ProcessPending(0,MockQuery);CHECK(Submit(request,GetTickCount64())==Verdict::Blocked,"occluded result remains blocked on the render side");
 CHECK(NativeQuery(0,request)==Verdict::Unavailable,"missing build or physics backend is never treated as visible");

 using namespace HazzureOffsets::Schema;
 std::vector<unsigned char> pawnBytes(0x4000),cameraBytes(0x400),punchBytes(0x400);
 uintptr_t pawn=reinterpret_cast<uintptr_t>(pawnBytes.data()),cam=reinterpret_cast<uintptr_t>(cameraBytes.data()),punch=reinterpret_cast<uintptr_t>(punchBytes.data());
 auto put=[](uintptr_t at,const auto& value){memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));};
 put(pawn+m_iHealth,int(100));put(pawn+m_iTeamNum,uint8_t(2));put(pawn+m_pCameraServices,cam);put(pawn+m_pAimPunchServices,punch);
 HazzureGame::Vec3 a{1,2,3},b{4,5,6},c{7,8,9},d{10,11,12};
 put(cam+0x48,a);put(punch+m_predictableBaseAngle,b);put(punch+0x5c,c);put(punch+m_unpredictableBaseAngle,d);
 HazzureVisual::enabled=false;CHECK(!HazzureVisual::Begin(pawn),"disabled visual option does not touch punch");
 HazzureVisual::enabled=true;CHECK(HazzureVisual::Begin(pawn),"render-stage transaction captures valid camera and punch fields");
 float x{},y{};CHECK(HazzureVisual::OriginalPunch(pawn,x,y)&&x==4&&y==5,"RCS still reads the original punch while visuals are suppressed");
 CHECK(HazzureGame::Read(punch+m_predictableBaseAngle,b)&&b.x==0&&b.y==0&&b.z==0,"render calculation sees zero visual punch");
 CHECK(!HazzureVisual::Begin(pawn),"nested render call cannot replace an active snapshot");
 HazzureVisual::End();HazzureGame::Vec3 restored{};HazzureGame::Read(punch+m_predictableBaseAngle,restored);
 CHECK(restored.x==4&&restored.y==5&&restored.z==6&&!HazzureVisual::active,"prediction punch is restored after render calculation");
 CHECK(HazzureVisual::Begin(pawn),"next render can start a new transaction");
 put(cam+0x48,HazzureGame::Vec3{20,21,22});HazzureVisual::End();HazzureGame::Read(cam+0x48,restored);
 CHECK(restored.x==20&&restored.y==21,"fresh native camera update is not overwritten by restoration");
 put(punch+m_predictableBaseAngle,HazzureGame::Vec3{std::numeric_limits<float>::quiet_NaN(),0,0});
 CHECK(!HazzureVisual::Begin(pawn)&&!HazzureVisual::active,"invalid punch declines the transaction without partial suppression");
 printf("RESULT: %u passed, %u failed; no game attachment, native engine calls or OS input.\n",passed,failed);return failed?1:0;
}
