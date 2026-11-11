// Own-process memory and pure context decisions only. No input, audio or window.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <windows.h>
#include <array>
#include <cstdio>
#include "../src/HazzureUI.h"
static unsigned passed{},failed{};
#define CHECK(v,text) do{if(v)++passed;else{++failed;printf("FAIL: %s\n",text);}}while(0)
struct QueryState {bool available=true,roof{},occluded{};unsigned calls{};};
static bool Query(void* p,const HazzureGame::Vec3& a,const HazzureGame::Vec3& b,HazzureWorldTrace::Result& out){
 auto* s=static_cast<QueryState*>(p);++s->calls;
 out={s->roof&&b.z-a.z>1000?0.4f:s->occluded?0.5f:1.f,false};return s->available;
}
int main(){
 using HazzureInputContext::BlockReason;
 CHECK(!strcmp(BlockReason(true,false,true,true,true,false),"menu-open"),"open menu blocks gameplay input");
 CHECK(!strcmp(BlockReason(false,true,true,true,true,false),"imgui-capture"),"text/capture context blocks gameplay input");
 CHECK(!strcmp(BlockReason(false,false,false,true,true,false),"no-foreground"),"unknown foreground blocks gameplay input");
 CHECK(!strcmp(BlockReason(false,false,true,false,true,false),"other-window"),"other process foreground blocks gameplay input");
 CHECK(!strcmp(BlockReason(false,false,true,true,false,false),"cursor-state-unavailable"),"unavailable cursor state cannot authorize gameplay input");
 CHECK(!strcmp(BlockReason(false,false,true,true,true,true),"game-cursor-visible"),"game cursor blocks synthetic gameplay while buy/panorama UI is visible");
 CHECK(!strcmp(BlockReason(false,false,true,true,true,false),"ready"),"active game context without UI may proceed");
 std::array<unsigned char,256> client{};std::array<unsigned char,4096> block{};
 auto old=HazzureOffsets::Globals::dwSensitivity;HazzureOffsets::Globals::dwSensitivity=0x20;
 auto clientAt=reinterpret_cast<uintptr_t>(client.data()),blockAt=reinterpret_cast<uintptr_t>(block.data());
 auto put=[](uintptr_t at,const auto& value){memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));};
 auto field=blockAt+HazzureOffsets::Schema::dwSensitivity_sensitivity;
 put(clientAt+0x20,blockAt);put(field,1.75f);float value{};
 CHECK(HazzureUI::ReadSensitivity(clientAt,value)&&value==1.75f,"read input scale only from valid current memory");
 put(clientAt+0x20,uintptr_t(0));value=99;
 CHECK(!HazzureUI::ReadSensitivity(clientAt,value)&&value==0,"missing input scale cannot reuse a prior valid value");
 put(clientAt+0x20,blockAt);put(field,std::nanf(""));
 CHECK(!HazzureUI::ReadSensitivity(clientAt,value)&&value==0,"non-finite input scale cannot authorize movement");
 put(field,.001f);CHECK(!HazzureUI::ReadSensitivity(clientAt,value),"out-of-range input scale is rejected");
 CHECK(!HazzureUI::ReadSensitivity(0,value)&&value==0,"missing module never fabricates default sensitivity");
 HazzureOffsets::Globals::dwSensitivity=old;
 HazzureWeather::Settings settings;settings.enabled=true;QueryState q;HazzureWeather::System weather;
 HazzureGame::Vec3 eye{0,0,64};uint64_t now=1000;
 for(unsigned i=0;i<80;++i){weather.Step(settings,eye,now,1,Query,&q);CHECK(weather.queries<=12,"weather queries stay inside independent budget");now+=16;}
 unsigned visible=0;for(const auto& p:weather.particles)if(HazzureWeather::Visible(p,now-16))++visible;
 CHECK(visible>0&&visible<=192,"only recently geometry-confirmed particles are visible");
 HazzureWeather::Particle p;p.active=true;p.point={0,0,1};CHECK(!HazzureWeather::Visible(p,1000),"active-but-unknown particle is not visible");
 p.visibleUntil=1001;CHECK(HazzureWeather::Visible(p,1000)&&!HazzureWeather::Visible(p,1002),"visibility TTL expires independently of snapshot publication");
 q.available=false;weather.Clear();for(unsigned i=0;i<60;++i){weather.Step(settings,eye,now,1,Query,&q);now+=16;}
 visible=0;for(const auto& v:weather.particles)visible+=HazzureWeather::Visible(v,now);CHECK(!visible,"unavailable geometry never authorizes weather rendering");
 q.available=true;q.roof=true;weather.Clear();for(unsigned i=0;i<60;++i){weather.Step(settings,eye,now,1,Query,&q);now+=16;}
 visible=0;for(const auto& v:weather.particles)visible+=HazzureWeather::Visible(v,now);CHECK(!visible,"roof occlusion prevents precipitation from spawning through ceilings");
 settings.enabled=false;weather.Step(settings,eye,now,1,Query,&q);
 bool active=false;for(const auto& v:weather.particles)active|=v.active;CHECK(!active,"disabling weather clears all particles");
 printf("RESULT: %u passed, %u failed; synthetic context/memory, no input or audio.\n",passed,failed);return failed?1:0;
}
