// C++ / gunplay-tests.cpp / MSVC x64 C++17 / deterministic math and queue regressions
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cstdio>
#include <limits>
#include "../src/Gunplay.h"
#include "../src/Visibility.h"
#include "../src/ShareCodes.h"
static unsigned passed{},failed{},queries{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
static HazzureVisibility::Verdict MockBoneQuery(uintptr_t,const HazzureVisibility::Request& q){++queries;return q.sample==6?HazzureVisibility::Verdict::Blocked:HazzureVisibility::Verdict::Visible;}
int main(){
 using namespace HazzureGunplay;
 CHECK(DistanceFov(4.5f,5,20,.25f)==4.5f,"near targets retain the configured FOV");
 CHECK(DistanceFov(4.5f,80,20,.25f)==1.125f,"distant targets receive a smaller angular cone");
 CHECK(std::fabs(DistanceFov(4.5f,1000,20,.25f)-1.125f)<.0001f,"distance scaling respects its minimum");
 CHECK(DistanceFov(4.5f,std::numeric_limits<float>::quiet_NaN(),20,.25f)==0,"invalid distances do not expand acquisition");
 Acquisition timer;uint32_t seed=123;
 CHECK(!timer.Ready(1,100,80,180,seed)&&timer.delay>=80&&timer.delay<=180,"reaction delay is chosen once within configured bounds");
 auto chosen=timer.delay,afterSeed=seed;
 CHECK(!timer.Ready(1,101,80,180,seed)&&timer.delay==chosen&&seed==afterSeed,"tracking the same target does not reroll the timer each frame");
 CHECK(timer.Ready(1,100+chosen,80,180,seed),"continuous acquisition completes at the chosen deadline");
 CHECK(!timer.Ready(2,100+chosen,80,180,seed),"a different enemy starts a fresh reaction interval");
 CHECK(!timer.Ready(0,500,80,180,seed)&&!timer.target,"loss of the target cancels acquisition");
 CHECK(timer.Ready(3,600,0,0,seed),"zero delay remains an explicit supported setting");
 auto first=Point({5,0},0,.1f),last=Point({5,0},1,.1f),middle=Point({5,0},.5f,.1f);
 CHECK(first.x==0&&first.y==0&&last.x==5&&last.y==0,"Bezier curve preserves exact start and end points");
 CHECK(middle.y>0&&middle.y<=.1f,"Bezier control points create a bounded arc");
 auto shortArc=Point({.001f,0},.5f,.25f);
 CHECK(std::fabs(shortArc.y)<=.00015f,"micro corrections cannot turn into a large random excursion");
 Curve a,b;Vec2 va{},vb{};uint32_t sa=42,sb=42;
 for(int i=0;i<6;++i){Vec2 error{5-va.x,2-va.y};auto v=a.Step(11,va,error,8,.04f,1.f/60,sa);va.x+=v.x;va.y+=v.y;}
 for(int i=0;i<24;++i){Vec2 error{5-vb.x,2-vb.y};auto v=b.Step(11,vb,error,8,.04f,1.f/240,sb);vb.x+=v.x;vb.y+=v.y;}
 CHECK(std::fabs(va.x-vb.x)<.0001f&&std::fabs(va.y-vb.y)<.0001f,"the same curve reaches the same point at equal elapsed time across frame rates");
 for(int i=0;i<30;++i){Vec2 error{5-va.x,2-va.y};auto v=a.Step(11,va,error,8,.04f,1.f/60,sa);va.x+=v.x;va.y+=v.y;}
 CHECK(std::fabs(va.x-5)<.0001f&&std::fabs(va.y-2)<.0001f,"a completed acquisition converges to the endpoint");
 a.Step(12,va,{1,1},8,.04f,1.f/60,sa);CHECK(a.target==12&&a.elapsed<.02f,"target changes reset the curve without reusing the old arc");
 a.Step(12,va,{1,1},8,.04f,0,sa);CHECK(!a.target,"invalid frame timing clears curve state");
 Recoil recoil;float totalX=0,totalY=0;
 for(int i=0;i<180;i++){auto v=recoil.Step({-2,1},3,true,1,1,8,1.f/180);totalX+=v.x;totalY+=v.y;}
 CHECK(std::fabs(totalX+recoil.pending.x-4)<.0001f&&std::fabs(totalY+recoil.pending.y+2)<.0001f,"RCS smoothing preserves the complete compensation instead of dropping the unsent remainder");
 recoil.Step({},0,false,1,1,8,1.f/60);CHECK(!recoil.valid&&recoil.pending.x==0,"release clears standalone recoil state");
 auto immediate=recoil.Step({-1,.5f},2,true,1,1,1,1.f/60);CHECK(immediate.x==2&&immediate.y==-1,"smooth one preserves direct standalone compensation");
 CHECK(EaseTime(0,.35f)==0&&EaseTime(1,.35f)==1,"asymmetric time easing preserves both exact endpoints");
 float eps=.0001f;
 CHECK(EaseTime(eps,.35f)/eps<.001f&&(1-EaseTime(1-eps,.35f))/eps<.001f,"acquisition starts and finishes with a damped velocity envelope");
 float earlySpeed=(EaseTime(.35f+eps,.35f)-EaseTime(.35f-eps,.35f))/(2*eps);
 float lateSpeed=(EaseTime(.8f+eps,.35f)-EaseTime(.8f-eps,.35f))/(2*eps);
 CHECK(earlySpeed>lateSpeed&&earlySpeed>1.9f,"asymmetric acquisition peaks before its long deceleration phase");
 Curve eased60,eased240;Vec2 e60{},e240{};uint32_t es60=3,es240=3;
 for(int i=0;i<6;++i){auto s=eased60.Step(3,e60,{5-e60.x,2-e60.y},8,0,1.f/60,es60,true);e60.x+=s.x;e60.y+=s.y;}
 for(int i=0;i<24;++i){auto s=eased240.Step(3,e240,{5-e240.x,2-e240.y},8,0,1.f/240,es240,true);e240.x+=s.x;e240.y+=s.y;}
 CHECK(std::fabs(e60.x-e240.x)<.0001f&&std::fabs(e60.y-e240.y)<.0001f,"eased acquisition is consistent at 60 and 240 frames per second");
 bool approaches=true;float lastDistance=std::hypot(5-e60.x,2-e60.y);
 for(int i=0;i<60;++i){auto s=eased60.Step(3,e60,{5-e60.x,2-e60.y},8,.04f,1.f/60,es60,true);e60.x+=s.x;e60.y+=s.y;
     float distance=std::hypot(5-e60.x,2-e60.y);approaches=approaches&&distance<=lastDistance+.0001f;lastDistance=distance;}
 CHECK(approaches&&lastDistance<.0001f,"eased tracking converges without passing or moving away from the target");
 auto bounded=BoundApproach({8,0},{1,0}),opposed=BoundApproach({-1,0},{1,0}),atTarget=BoundApproach({.1f,.1f},{});
 CHECK(bounded.x==1&&opposed.x==0&&atTarget.x==0&&atTarget.y==0,"oversized, reverse and already-settled corrections are bounded");
 RecoilSettings option;RecoilControl human;
 auto bullet1=human.Step({-1,.5f},1,true,10,option,1.f/60);
 auto bullet2=human.Step({-2,1},2,true,10,option,1.f/60);
 CHECK(bullet1.x==0&&bullet2.x==0&&human.offset.x==0,"configured bullet delay leaves the first two bullets uncompensated");
 auto bullet3=human.Step({-3,1.5f},3,true,10,option,1.f/60);
 CHECK(std::fabs(bullet3.x-2*RecoilGain(3,option))<.0001f&&bullet3.x<1,"first compensated bullet uses only new punch and the ramp gain");
 auto settled=human.Step({-3,1.5f},3,true,10,option,1.f/60);
 CHECK(settled.x==0&&settled.y==0,"ignored bullet punch is not paid back after the delay");
 CHECK(RecoilGain(3,option)<RecoilGain(4,option)&&std::fabs(RecoilGain(5,option)-.94f)<.0001f,"ramp gradually reaches the configured partial compensation");
 RecoilSettings partial;partial.bulletDelay=0;partial.rampBullets=0;RecoilControl direct;
 direct.Step({-1,0},1,true,20,partial,1.f/60);direct.Step({-2,0},2,true,20,partial,1.f/60);
 CHECK(std::fabs(direct.offset.x-3.76f)<.0001f,"full ramp retains a six percent recoil margin at strength 0.94");
 auto changed=human.Step({-8,4},6,true,11,option,1.f/60);
 CHECK(changed.x==0&&human.offset.x==0&&human.weapon==11,"weapon change drops pending recoil without applying historical spray");
 human.Step({},0,false,11,option,1.f/60);
 CHECK(!human.valid&&human.pending.x==0&&human.offset.x==0,"firing release clears both integrated and standalone compensation");
 human.Step({-1,0},1,true,11,option,1.f/60);human.Step({-2,0},2,true,11,option,1.f/60);human.Step({-3,0},3,true,11,option,1.f/60);
 auto restarted=human.Step({-.5f,0},1,true,11,option,1.f/60);
 CHECK(restarted.x==0&&human.offset.x==0&&human.lastShots==1,"a decreasing bullet counter starts a new delayed burst");
 option.smooth=8;RecoilControl filtered;filtered.Step({},1,true,15,option,1.f/180);filtered.Step({-2,1},3,true,15,option,1.f/180);
 for(int i=0;i<180;++i)filtered.Step({-2,1},3,true,15,option,1.f/180);
 CHECK(std::fabs(filtered.offset.x+filtered.pending.x-4*RecoilGain(3,option))<.0001f,"filtered recoil preserves its unsent remainder at partial strength");
 auto bad=filtered.Step({-2,1},3,true,15,option,std::numeric_limits<float>::quiet_NaN());
 CHECK(bad.x==0&&!filtered.valid&&filtered.offset.x==0,"invalid timing clears recoil state before any input decision");
 using namespace HazzureVisibility;
 entries={};nextEntry=0;Request head{1,2,{0,0,64},{100,0,64},6},chest=head;chest.sample=4;chest.end.z=48;
 uint64_t now=GetTickCount64();Submit(head,now);Submit(chest,now);ProcessPending(0,MockBoneQuery);now=GetTickCount64();
 CHECK(Submit(head,now)==Verdict::Blocked&&Submit(chest,now)==Verdict::Visible,"head and chest traces for the same pawn cannot overwrite each other's visibility");
 entries={};nextEntry=0;queries=0;
 for(unsigned i=0;i<40;i++){Request q{1,100+i,{0,0,64},{100,0,64},4};Submit(q,GetTickCount64());}
 ProcessPending(0,MockBoneQuery);CHECK(queries==32,"physics work is bounded to 32 requests in one frame callback");
 ProcessPending(0,MockBoneQuery);CHECK(queries==40,"round robin processing does not starve requests beyond the first batch");
 std::string code,json,error,payload="{\"name\":\"a\xc3\xa7\xc3\xa3o\",\"profiles\":[1,2,3]}";
 CHECK(HazzureShare::Crc32("123456789")==0xcbf43926u,"share-code checksum matches the CRC32 standard vector");
 CHECK(HazzureShare::Encode(payload,code,error)&&HazzureShare::Decode(code,json,error)&&json==payload,"versioned Base64 share code preserves UTF-8 and the entire JSON payload");
 auto invalid=code;invalid[14]=invalid[14]=='A'?'B':'A';
 CHECK(!HazzureShare::Decode(invalid,json,error),"corrupted share code is rejected before profile application");
 invalid=code;invalid[3]='4';CHECK(!HazzureShare::Decode(invalid,json,error),"unsupported share-code version is rejected");
 invalid=code;invalid.back()='!';CHECK(!HazzureShare::Decode(invalid,json,error),"non-Base64 payload characters are rejected");
 CHECK(!HazzureShare::Decode(code.substr(0,code.size()-1),json,error),"truncated Base64 is rejected");
 CHECK(!HazzureShare::Encode(std::string(HazzureShare::MaxJson+1,'x'),code,error),"oversized profile export is bounded before allocation of transport data");
 printf("RESULT: %u passed, %u failed; own process, no OS input or engine calls.\n",passed,failed);return failed?1:0;
}
