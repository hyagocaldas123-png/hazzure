// C++ / climate_effects_tests.cpp / MSVC x64 C++17 / synthetic world-effect regressions
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>
#include "../../Compartilhado/offsets_generated.h"

struct Shared {
 volatile LONG64 heartbeat{},worldDraws{},sixFaceDraws{},climateChanges{},skyDraws{},skyApplied{},faults{};
 volatile LONG climateFlags{},climateStatus{},skyCount{},skyboxIndex{},cleanMap{},removeWeather{};
 float skyBrightness=.7f,worldBrightness=1.f,fogDensity=0.f;
 uint32_t skyColour=0xffffffffu,fogColour=0xff112233u;
 uintptr_t materialSamples[96]{};
 char climateMessage[160]{};
};
static Shared state;
static Shared* shared=&state;
static uintptr_t client;
static void* fallbackSkyMaterial;
using Draw=void(*)(void*,void*,void*,int,void*,void*,void*,void*);
static std::unordered_map<uintptr_t,const char*> names;
static std::unordered_map<uint32_t,uintptr_t> handles;
static bool matchingSchema=true,materialAvailable=true,currentSchema=true;
static unsigned updates{},logs{};
static bool failNextSkyUpdate{};
static uintptr_t lastUpdateCache{};
static unsigned skyDrawCalls{};static bool skyDrawForwarded{},skyColourUnchanged{};
static void* drawArguments[7]{};static float* observedSkyColour{};
static bool Read(const void* source,void* output,size_t size){
 if(!source)return false;
 __try{memcpy(output,source,size);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T>static T Get(uintptr_t address){T value{};Read(reinterpret_cast<void*>(address),&value,sizeof(value));return value;}
static uintptr_t Entity(uint32_t handle){auto it=handles.find(handle);return it==handles.end()?0:it->second;}
static bool ClassName(uintptr_t entity,char* output,size_t capacity){
 auto it=names.find(entity);if(it==names.end())return false;strncpy_s(output,capacity,it->second,_TRUNCATE);return true;
}
static void* MakeSkyMaterial(int index){return materialAvailable?reinterpret_cast<void*>(uintptr_t(0x100000)+static_cast<uintptr_t>(index)*16):nullptr;}
static void LogDebug(const char*,...){++logs;}
namespace HazzureSchema {
 inline bool GetModuleFingerprint(const char*,uint32_t& stamp,uint32_t& size){
  stamp=matchingSchema?(currentSchema?0x6AE217A2u:0x6ADA7340u):1u;size=currentSchema?0x29A2000u:0x29A0000u;return true;
 }
}
#include "../src/Climate.h"

static uintptr_t MockSkyUpdate(uintptr_t entity){if(failNextSkyUpdate){failNextSkyUpdate=false;RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);}++updates;lastUpdateCache=Get<uintptr_t>(entity+HazzureOffsets::Schema::m_pSceneObjectCache);return entity;}
static void MockSkyDraw(void* a,void* b,void* mesh,int count,void* e,void* f,void* g,void* h){
 ++skyDrawCalls;skyDrawForwarded=count==16&&a==drawArguments[0]&&b==drawArguments[1]&&mesh==drawArguments[2]&&
  e==drawArguments[3]&&f==drawArguments[4]&&g==drawArguments[5]&&h==drawArguments[6];
 skyColourUnchanged=observedSkyColour[0]==.1f&&observedSkyColour[1]==.2f&&observedSkyColour[2]==.3f;
}
static std::vector<unsigned char> clientBytes(512),listBytes(0x210),chunkBytes(512*0x70);
static std::vector<std::vector<unsigned char>> entityBytes;
static uintptr_t Add(unsigned index,const char* name){
 entityBytes.emplace_back(0x2000);
 uintptr_t entity=reinterpret_cast<uintptr_t>(entityBytes.back().data());
 uint32_t handle=0x8000u+index;
 *reinterpret_cast<uintptr_t*>(chunkBytes.data()+index*0x70)=entity;
 *reinterpret_cast<uint32_t*>(chunkBytes.data()+index*0x70+0x10)=handle;
 names[entity]=name;handles[handle]=entity;return entity;
}
static void NextFrame(){
 InterlockedExchange64(&state.heartbeat,GetTickCount64());climateNext=climateEffectsNext=0;
 FogWeatherFrame();
}
#define CHECK(condition) do{if(!(condition)){printf("FAIL line %d: %s\n",__LINE__,#condition);return 1;}}while(0)
int main(){
 HazzureWorld::ready.store(true); // The synthetic scene below is a ready world, without a game process.
 HazzureOffsets::Globals::dwEntityList=0x20;HazzureOffsets::Globals::dwLocalPlayerPawn=0x28;
 client=reinterpret_cast<uintptr_t>(clientBytes.data());
 *reinterpret_cast<uintptr_t*>(clientBytes.data()+0x20)=reinterpret_cast<uintptr_t>(listBytes.data());
 *reinterpret_cast<uintptr_t*>(listBytes.data()+0x10)=reinterpret_cast<uintptr_t>(chunkBytes.data());
 originalSkyUpdate=MockSkyUpdate;
 auto fog=Add(12,".?AVC_FogController@@"),rain=Add(13,".?AVC_Precipitation@@"),sky=Add(14,".?AVC_EnvSky@@");
 ClimatePut(fog+0x630,.6f);ClimatePut(fog+0x614,0xaaaabbccu);ClimatePut(fog+0x618,0x55ddeeffu);
 ClimatePut(fog+0x664,uint8_t(0));ClimatePut(fog+0x665,uint8_t(1));ClimatePut(fog+0x666,uint16_t(0x5aa5));
 ClimatePut(rain+0x1180,.8f);ClimatePut(rain+0x11b4,uint8_t(1));
 state.fogDensity=.25f;state.removeWeather=1;NextFrame();
 CHECK(Get<float>(fog+0x630)==.25f&&Get<uint32_t>(fog+0x614)==0xaa112233u&&Get<uint32_t>(fog+0x618)==0x55112233u);
 CHECK(Get<uint8_t>(fog+0x664)==1&&Get<uint8_t>(fog+0x665)==0&&Get<uint16_t>(fog+0x666)==0x5aa5);
 CHECK(Get<float>(rain+0x1180)==.8f&&Get<uint8_t>(rain+0x11b4)==1&&climateWeatherCount==0);
 CHECK(climateFogCount==1);
 state.fogDensity=0;state.removeWeather=0;NextFrame();
 CHECK(Get<float>(fog+0x630)==.6f&&Get<uint32_t>(fog+0x614)==0xaaaabbccu&&Get<uint32_t>(fog+0x618)==0x55ddeeffu);
 CHECK(Get<uint8_t>(fog+0x664)==0&&Get<uint8_t>(fog+0x665)==1&&Get<uint16_t>(fog+0x666)==0x5aa5);
 puts("PASS: current fog uses separate Color32 fields, preserves original alpha, one-byte enable/blend and padding; legacy precipitation stays untouched.");
 state.fogDensity=.3f;NextFrame();InterlockedExchange64(&state.heartbeat,0);FogWeatherFrame(true);
 CHECK(Get<float>(fog+0x630)==.6f&&Get<uint8_t>(fog+0x665)==1);
 puts("PASS: stale heartbeat restores all owned fog fields.");
 NextFrame();CHECK(Get<float>(fog+0x630)==.3f);HazzureWorld::ready.store(false);FogWeatherFrame(true);
 CHECK(Get<float>(fog+0x630)==.6f&&Get<uint8_t>(fog+0x665)==1);HazzureWorld::ready.store(true);
 matchingSchema=false;NextFrame();CHECK(Get<float>(fog+0x630)==.6f);matchingSchema=true;NextFrame();
 CHECK(Get<float>(fog+0x630)==.3f);
 matchingSchema=false;CHECK(!RestoreFog()&&climateFogs[0].entity==fog);matchingSchema=true;
 handles.erase(0x8000u+12);ClimatePut(fog+0x630,.99f);state.fogDensity=0;NextFrame();CHECK(Get<float>(fog+0x630)==.99f);
 puts("PASS: incompatible fog schema retains snapshots; recycled serials are never overwritten.");
 // Colours/density are on the writable page; flags are on the read-only next page.
 auto allocation=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x3000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(allocation);
 auto protectedFog=allocation+0x9a0;uint32_t protectedHandle=0x8012;
 names[protectedFog]=".?AVC_FogController@@";handles[protectedHandle]=protectedFog;
 auto slot=chunkBytes.data()+18*0x70;memcpy(slot,&protectedFog,sizeof(protectedFog));memcpy(slot+0x10,&protectedHandle,sizeof(protectedHandle));
 ClimatePut(protectedFog+0x630,.7f);ClimatePut(protectedFog+0x614,0x77123456u);ClimatePut(protectedFog+0x618,0x99abcdefu);
 ClimatePut(protectedFog+0x664,uint8_t(0));ClimatePut(protectedFog+0x665,uint8_t(1));ClimatePut(protectedFog+0x666,uint16_t(0xbeef));
 DWORD oldFogProtection{};CHECK(VirtualProtect(reinterpret_cast<void*>(allocation+0x1000),0x1000,PAGE_READONLY,&oldFogProtection));
 state.fogDensity=.4f;NextFrame();
 CHECK(Get<float>(protectedFog+0x630)==.7f&&Get<uint32_t>(protectedFog+0x614)==0x77123456u&&Get<uint32_t>(protectedFog+0x618)==0x99abcdefu);
 CHECK(Get<uint8_t>(protectedFog+0x664)==0&&Get<uint8_t>(protectedFog+0x665)==1&&Get<uint16_t>(protectedFog+0x666)==0xbeef);
 CHECK(!RestoreFog());DWORD ignoredFogProtection{};CHECK(VirtualProtect(reinterpret_cast<void*>(allocation+0x1000),0x1000,oldFogProtection,&ignoredFogProtection));
 CHECK(RestoreFog()&&Get<float>(protectedFog+0x630)==.7f&&Get<uint8_t>(protectedFog+0x665)==1);
 handles.erase(protectedHandle);names.erase(protectedFog);memset(slot,0,0x70);CHECK(VirtualFree(reinterpret_cast<void*>(allocation),0,MEM_RELEASE));
 puts("PASS: partial fog write rolls back writable fields, retains failed restoration, and restores exactly when flags become writable.");
 currentSchema=false; // Remaining legacy sky/exposure tests never authorize the current client.
 state.fogDensity=0;state.climateFlags=1;state.skyboxIndex=2;
 ClimatePut(sky+HazzureOffsets::Schema::m_hSkyMaterial,uintptr_t(0x123456));
 ClimatePut(sky+HazzureOffsets::Schema::m_vTintColor,uint32_t(0xff010203));
 ClimatePut(sky+HazzureOffsets::Schema::m_flBrightnessScale,.9f);
 ClimatePut(sky+HazzureOffsets::Schema::m_pSceneObjectCache,uintptr_t(0x567890));
 NextFrame();ClimateFrameWork();
 CHECK(Get<uintptr_t>(sky+HazzureOffsets::Schema::m_hSkyMaterial)==0x123456&&updates==0);
 puts("PASS: map-loading sky stays untouched before a valid local pawn exists.");
 std::vector<unsigned char> pawnBytes(0x4000);auto pawn=reinterpret_cast<uintptr_t>(pawnBytes.data());
 ClimatePut(pawn+HazzureOffsets::Schema::m_iHealth,int(255));
 ClimatePut(pawn+HazzureOffsets::Schema::m_iTeamNum,uint8_t(2));
 *reinterpret_cast<uintptr_t*>(clientBytes.data()+0x28)=pawn;
 NextFrame();ClimateFrameWork();
 CHECK(Get<uintptr_t>(sky+HazzureOffsets::Schema::m_hSkyMaterial)==0x100020u&&updates>0);
 CHECK(lastUpdateCache==0x567890&&Get<uintptr_t>(sky+HazzureOffsets::Schema::m_pSceneObjectCache)==0x567890);
 puts("PASS: native sky update reuses its registered scene cache instead of orphaning the object.");
 state.skyBrightness=.5f;SkyUpdateHook(sky);
 CHECK(lastUpdateCache==0x567890&&Get<float>(sky+HazzureOffsets::Schema::m_flBrightnessScale)==.7f);
 puts("PASS: native sky callback forwards without a second writer of material, tint or scene cache.");
 std::vector<unsigned char> renderScene(0x200),meshes(16*0x70);
 auto sceneAddress=reinterpret_cast<uintptr_t>(renderScene.data());names[sceneAddress]=".?AVCSceneSkyBoxObject@@";
 ClimatePut(reinterpret_cast<uintptr_t>(meshes.data())+15*0x70+0x18,sceneAddress);
 observedSkyColour=reinterpret_cast<float*>(renderScene.data()+0xe8);
 observedSkyColour[0]=.1f;observedSkyColour[1]=.2f;observedSkyColour[2]=.3f;
 for(unsigned i=0;i<7;i++)drawArguments[i]=reinterpret_cast<void*>(uintptr_t(0x100000+i*0x100));
 drawArguments[2]=meshes.data();originalSkyDraw=MockSkyDraw;
 SkyDrawHook(drawArguments[0],drawArguments[1],drawArguments[2],16,drawArguments[3],drawArguments[4],drawArguments[5],drawArguments[6]);
 CHECK(skyDrawCalls==1&&skyDrawForwarded&&skyColourUnchanged);
 puts("PASS: render worker forwards the full 16-mesh batch and all arguments exactly once without tint writes.");
 state.climateFlags=0;climateNext=0;ClimateFrameWork();
 CHECK(Get<uintptr_t>(sky+HazzureOffsets::Schema::m_hSkyMaterial)==0x123456u);
 CHECK(Get<uint32_t>(sky+HazzureOffsets::Schema::m_vTintColor)==0xff010203u&&Get<float>(sky+HazzureOffsets::Schema::m_flBrightnessScale)==.9f);
 puts("PASS: selected skybox material takes priority and restores the original map material/tint/brightness.");

 state.climateFlags=1;state.skyboxIndex=3;materialAvailable=false;climateNext=0;ClimateFrameWork();
 CHECK(Get<uintptr_t>(sky+HazzureOffsets::Schema::m_hSkyMaterial)==0x123456u&&state.climateStatus==-4);
 state.cleanMap=1;auto before=Get<float>(sky+0x10D0);CleanMapFrame();CHECK(Get<float>(sky+0x10D0)==before);
 CHECK(logs>0);
 puts("PASS: missing sky material and unvalidated shadow offset preserve original memory with diagnostics.");

 DWORD ignoredProtection=0;
 climateSkies[0]={0x1234,0xDEAD,0xBAD,0,1.f};climateLoadAttempted=true;climateCloudMaterial.store(0xBAD);
 auto beforeReset=updates;ResetClimateSession();
 CHECK(!climateSkies[0].entity&&!climateLoadAttempted&&!climateCloudMaterial.load()&&!exposureSaved.entity);
 CHECK(updates==beforeReset);
 puts("PASS: map transition discards retiring snapshots without writing or updating old scene objects.");
 using namespace HazzureWorldAppearance;
 for(size_t i=0;i<Count;++i){Settings preset{};CHECK(Select(static_cast<int>(i),preset)&&Valid(preset)&&Matches(preset,static_cast<int>(i))&&*Presets[i].name);}
 Settings original{};CHECK(Select(0,original)&&!original.skyActive&&!original.darkWorld&&!original.removeOriginalWeather&&!original.snowParticles&&original.fogDensity==0);
 CHECK(!Select(-1,original)&&!Select(static_cast<int>(Count),original)&&Matches(original,0));
 puts("PASS: scene presets use bounded existing channels; Original disables every world override and invalid choices preserve the selection.");
 auto invalid=original;invalid.fog[0]=NAN;CHECK(!Valid(invalid));invalid=original;invalid.worldBrightness=.1f;CHECK(!Valid(invalid));
 invalid=original;invalid.skyboxIndex=10;CHECK(!Valid(invalid));invalid=original;invalid.sky[3]=1.01f;CHECK(!Valid(invalid));
 Settings edited{};CHECK(Select(1,edited));edited.sky[0]+=.01f;CHECK(!Matches(edited,1));
 CHECK(HeartbeatAlive(1000,3999)&&!HeartbeatAlive(1000,4000)&&!HeartbeatAlive(1001,1000)&&!HeartbeatAlive(0,1000)&&!HeartbeatAlive(-1,1000));
 puts("PASS: custom palette changes are distinguishable from presets; NaN, out-of-range options and future/stale heartbeat values are rejected.");
 auto post=Add(16,".?AVC_PostProcessingVolume@@");std::vector<unsigned char> cameraBytes(0x1000);
 auto camera=reinterpret_cast<uintptr_t>(cameraBytes.data());
 ClimatePut(pawn+HazzureOffsets::Schema::m_pCameraServices,camera);
 ClimatePut(camera+HazzureOffsets::Schema::m_hActivePostProcessingVolume,uint32_t(0x8010));
 const float initialExposure[4]={-3.f,1.f,.25f,8.f};
 ClimateWrite(post+HazzureOffsets::Schema::m_flMinLogExposure,initialExposure,sizeof(initialExposure));
 state.climateFlags=2;state.worldBrightness=.5f;InterlockedExchange64(&state.heartbeat,GetTickCount64());ExposureFrame();
 auto exposureAddress=post+HazzureOffsets::Schema::m_flMinLogExposure;
 CHECK(Get<float>(exposureAddress)==-4&&Get<float>(exposureAddress+4)==0&&Get<float>(exposureAddress+8)==.125f&&Get<float>(exposureAddress+12)==4);
 ExposureFrame();CHECK(Get<float>(exposureAddress)==-4&&Get<float>(exposureAddress+8)==.125f);
 state.climateFlags=0;ExposureFrame();CHECK(!memcmp(reinterpret_cast<void*>(exposureAddress),initialExposure,sizeof(initialExposure))&&!exposureSaved.entity);
 puts("PASS: exposure presets scale the original snapshot without cumulative darkening and restore all four exact exposure fields.");
 state.climateFlags=1;state.skyboxIndex=2;materialAvailable=true;climateNext=0;ClimateFrameWork();
 CHECK(Get<uintptr_t>(sky+HazzureOffsets::Schema::m_hSkyMaterial)==0x100020u);
 *reinterpret_cast<uintptr_t*>(clientBytes.data()+0x28)=0;climateNext=0;ClimateFrameWork();
 CHECK(Get<uintptr_t>(sky+HazzureOffsets::Schema::m_hSkyMaterial)==0x123456u);
 *reinterpret_cast<uintptr_t*>(clientBytes.data()+0x28)=pawn;
 puts("PASS: losing the local world context restores the sky instead of leaving the prior scene preset active.");
 state.climateFlags=2;
 matchingSchema=false;state.climateFlags=2;ExposureFrame();CHECK(!exposureSaved.entity&&!memcmp(reinterpret_cast<void*>(exposureAddress),initialExposure,sizeof(initialExposure)));
 matchingSchema=true;ExposureFrame();CHECK(exposureSaved.entity==post);
 HazzureWorld::ready.store(false);ExposureFrame();CHECK(!exposureSaved.entity&&!memcmp(reinterpret_cast<void*>(exposureAddress),initialExposure,sizeof(initialExposure)));
 HazzureWorld::ready.store(true);ExposureFrame();CHECK(exposureSaved.entity==post);
 puts("PASS: losing world context restores exposure before further writes.");
 names[post]=".?AVC_FogController@@";ClimatePut(exposureAddress,123.f);CHECK(RestoreExposure()&&Get<float>(exposureAddress)==123.f&&!exposureSaved.entity);
 names[post]=".?AVC_PostProcessingVolume@@";ClimateWrite(exposureAddress,initialExposure,sizeof(initialExposure));
 puts("PASS: exposure application requires the reviewed schema and restoration never writes into a recycled foreign-class entity.");
 auto protectedPost=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x3000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(protectedPost);
 names[protectedPost]=".?AVC_PostProcessingVolume@@";handles[0x8011]=protectedPost;
 ClimatePut(camera+HazzureOffsets::Schema::m_hActivePostProcessingVolume,uint32_t(0x8011));
 ClimateWrite(protectedPost+HazzureOffsets::Schema::m_flMinLogExposure,initialExposure,sizeof(initialExposure));ExposureFrame();CHECK(exposureSaved.entity==protectedPost);
 DWORD oldPostProtection{};CHECK(VirtualProtect(reinterpret_cast<void*>(protectedPost),0x3000,PAGE_READONLY,&oldPostProtection));
 CHECK(!RestoreExposure()&&exposureSaved.entity==protectedPost);
 CHECK(VirtualProtect(reinterpret_cast<void*>(protectedPost),0x3000,oldPostProtection,&ignoredProtection));
 CHECK(RestoreExposure()&&!memcmp(reinterpret_cast<void*>(protectedPost+HazzureOffsets::Schema::m_flMinLogExposure),initialExposure,sizeof(initialExposure)));
 handles.erase(0x8011);names.erase(protectedPost);CHECK(VirtualFree(reinterpret_cast<void*>(protectedPost),0,MEM_RELEASE));
 puts("PASS: read-only exposure restoration retains its original snapshot and retries successfully after writable memory returns.");
 const float invalidExposure[4]={1,-1,.25f,8};float computedExposure[4]{};
 CHECK(!ExposureValues(invalidExposure,.5f,computedExposure));
 state.climateFlags=2;ClimatePut(camera+HazzureOffsets::Schema::m_hActivePostProcessingVolume,uint32_t(0x8010));
 ClimateWrite(exposureAddress,invalidExposure,sizeof(invalidExposure));ExposureFrame();CHECK(!exposureSaved.entity);
 puts("PASS: invalid ordered exposure ranges cannot be captured and later restored as an apparent valid world setting.");
 auto protectedSky=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x3000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(protectedSky);
 names[protectedSky]=".?AVC_EnvSky@@";handles[0x8012]=protectedSky;
 SkySaved savedSky{0x8012,protectedSky,0x54321,0xff112233u,.8f};climateSkies[0]=savedSky;
 CHECK(PutSkyFields(protectedSky,0x99999,0xffaabbccu,.4f));
 DWORD skyProtection{};CHECK(VirtualProtect(reinterpret_cast<void*>(protectedSky),0x3000,PAGE_READONLY,&skyProtection));
 CHECK(!RestoreClimate()&&climateSkies[0].entity==protectedSky);
 CHECK(VirtualProtect(reinterpret_cast<void*>(protectedSky),0x3000,skyProtection,&ignoredProtection));
 CHECK(RestoreClimate()&&!climateSkies[0].entity&&Get<uintptr_t>(protectedSky+HazzureOffsets::Schema::m_hSkyMaterial)==savedSky.material&&Get<float>(protectedSky+HazzureOffsets::Schema::m_flBrightnessScale)==savedSky.brightness);
 handles.erase(0x8012);names.erase(protectedSky);CHECK(VirtualFree(reinterpret_cast<void*>(protectedSky),0,MEM_RELEASE));
 puts("PASS: sky restoration preserves its snapshot across a failed write and retries material, tint and brightness together.");
 // Keep a scene update pending even after all entity fields already match.
 state.climateFlags=1;state.skyboxIndex=2;state.skyBrightness=.73f;materialAvailable=true;
 failNextSkyUpdate=true;NextFrame();auto beforeRetry=updates;ClimateFrameWork();
 bool dirty=false;for(const auto& saved:climateSkies)if(saved.entity==sky)dirty=saved.updatePending;
 CHECK(dirty&&updates==beforeRetry&&state.skyCount==0);
 NextFrame();ClimateFrameWork();dirty=false;for(const auto& saved:climateSkies)if(saved.entity==sky)dirty=saved.updatePending;
 CHECK(!dirty&&updates==beforeRetry+1&&state.skyCount==1);
 NextFrame();ClimateFrameWork();CHECK(updates==beforeRetry+1);
 state.climateFlags=0;ClimateFrameWork();
 puts("PASS: a transient native sky update fault retries once without requiring a settings change or repeating successful updates.");

 return 0;
}
