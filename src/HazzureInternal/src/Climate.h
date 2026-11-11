// C++ / Climate.h / MSVC x64 C++17 / build 14192 world effects on the frame thread

#pragma once

#include "GameData.h"

#include "WorldAppearance.h"
#include "WorldSession.h"

static bool ClimateClassIs(uintptr_t entity,const char* expected){

 char name[96]{};return entity&&ClassName(entity,name,sizeof(name))&&!strcmp(name,expected);

}

static bool ClimateWrite(uintptr_t address,const void* value,size_t bytes){

 if(!address||!value||!bytes)return false;

 __try{memcpy(reinterpret_cast<void*>(address),value,bytes);return true;}

 __except(EXCEPTION_EXECUTE_HANDLER){return false;}

}

template<class T>static bool ClimatePut(uintptr_t address,const T& value){return ClimateWrite(address,&value,sizeof(value));}

static bool ClimateDataSchemaMatches(){

 uint32_t stamp=0,size=0;

 if(!HazzureSchema::GetModuleFingerprint("client.dll",stamp,size))return false;

 // Offsets below belong to this reviewed PE only. A config cannot authorize a new ABI.
 constexpr uint32_t kExpectedStamp=0x6AEBC4DAu;
 constexpr uint32_t kExpectedSize=0x29A3000u;
 if(stamp!=kExpectedStamp||size!=kExpectedSize){
  static bool logged=false;
  if(!logged){
   LogDebug("[CLIMA] Schema fingerprint mismatch: stamp=0x%08X (expected 0x%08X), size=0x%08X (expected 0x%08X)",
            stamp,kExpectedStamp,size,kExpectedSize);
   logged=true;
  }
  // Numeric offsets cannot be made compatible by a UI override.
  return false;
 }
 return true;

}

struct SkySaved { uint32_t handle; uintptr_t entity,material; uint32_t tint; float brightness; bool updatePending{}; };

static SkySaved climateSkies[128]{};

static bool climateFault;

struct ExposureSaved {uint32_t handle;uintptr_t entity;float values[4];};

static ExposureSaved exposureSaved{};

static bool ExposureValues(const float* source,float amount,float* values){

 if(!(amount>=.05f&&amount<=1.f))return false;

 for(unsigned i=0;i<4;i++)if(!std::isfinite(source[i]))return false;

 if(source[0]>source[1]||!(source[2]>0&&source[3]>=source[2]&&source[3]<65536.f))return false;

 float adjustment=log2f(amount);

 values[0]=source[0]+adjustment;values[1]=source[1]+adjustment;

 values[2]=source[2]*amount;values[3]=source[3]*amount;return true;

}

static bool RestoreExposure(){

 auto& s=exposureSaved;if(!s.entity)return true;

 if(Entity(s.handle)==s.entity&&ClimateClassIs(s.entity,".?AVC_PostProcessingVolume@@")&&

    !ClimateWrite(s.entity+HazzureOffsets::Schema::m_flMinLogExposure,s.values,sizeof(s.values)))return false;

 s={};return true;

}

static void ExposureFrame(){

 if(!shared)return;
 if(!HazzureWorld::ready.load()){RestoreExposure();return;}

 auto beat=InterlockedCompareExchange64(&shared->heartbeat,0,0);

 float amount=shared->worldBrightness;

 if(!(shared->climateFlags&2)||!HazzureWorldAppearance::HeartbeatAlive(beat,GetTickCount64())||!(amount>=.05f&&amount<=1.f)||

    !ClimateDataSchemaMatches()){RestoreExposure();return;}

 auto pawn=Get<uintptr_t>(client+HazzureOffsets::Globals::dwLocalPlayerPawn),camera=Get<uintptr_t>(pawn+HazzureOffsets::Schema::m_pCameraServices);

 auto handle=Get<uint32_t>(camera+HazzureOffsets::Schema::m_hActivePostProcessingVolume);auto entity=Entity(handle);

 if(exposureSaved.entity&&(exposureSaved.entity!=entity||exposureSaved.handle!=handle)&&!RestoreExposure())return;

 if(!entity)return;

 char name[96]{};if(!ClassName(entity,name,sizeof(name))||strcmp(name,".?AVC_PostProcessingVolume@@"))return;

 if(!exposureSaved.entity){

  ExposureSaved candidate{};candidate.handle=handle;candidate.entity=entity;float proposed[4]{};

  if(!Read(reinterpret_cast<void*>(entity+HazzureOffsets::Schema::m_flMinLogExposure),candidate.values,16)||!ExposureValues(candidate.values,amount,proposed))return;

  exposureSaved=candidate;

 }

 auto& s=exposureSaved;

 float values[4];if(!ExposureValues(s.values,amount,values))return;

 if(ClimateWrite(entity+HazzureOffsets::Schema::m_flMinLogExposure,values,sizeof(values)))InterlockedIncrement64(&shared->worldDraws);

}



static std::atomic<uintptr_t> climateCloudMaterial{0};

static bool climateLoadAttempted;

static Draw originalSkyDraw;

using SkyUpdate=uintptr_t(*)(uintptr_t);

static SkyUpdate originalSkyUpdate;

// client+0x2824D0 owns the scene object's material refresh with refcounts.
inline constexpr uintptr_t kSkyUpdateRva=0x2824D0u;
inline constexpr unsigned char kSkyUpdatePrefix[]={0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x55,0x41,0x57};

static bool SkyUpdateSupported(uintptr_t base){
 unsigned char actual[sizeof(kSkyUpdatePrefix)]{};
 return base&&HazzureGame::ReadBytes(base+kSkyUpdateRva,actual,sizeof(actual))&&
     !memcmp(actual,kSkyUpdatePrefix,sizeof(actual));
}

static bool ClimateAlive(LONG flag);

static ULONGLONG climateNext;

static bool SkyEntity(uintptr_t e){return ClimateClassIs(e,".?AVC_EnvSky@@");}

static void ClimateMessage(LONG status,const char* text){shared->climateStatus=status;strncpy_s(shared->climateMessage,text,_TRUNCATE);}

static bool UpdateSky(uintptr_t e){

 // client+0x282473 updates the existing scene object's material with refcounts.

 // Clearing its cache orphans the old registered object instead of updating it.

 if(!originalSkyUpdate)return false;

 __try{originalSkyUpdate(e);return true;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}

}

static uintptr_t SkyUpdateHook(uintptr_t e){

 // AOB-validated hook installed in InternalStartup.inl (client.dll + 0x282370).
 // Only increments telemetry counter; material/tint/brightness writes are
 // exclusively done in ClimateFrameWork to avoid racing the engine's scene object.

 InterlockedIncrement64(&shared->sixFaceDraws);

 if(!originalSkyUpdate)return 0;
 __try{return originalSkyUpdate(e);}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}

}

static bool PutSkyFields(uintptr_t entity,uintptr_t material,uint32_t tint,float brightness){

 // The selected material is retained by MakeSkyMaterial. The native update
 // callback propagates the strong handle to the existing scene object.
 bool written=ClimatePut(entity+HazzureOffsets::Schema::m_hSkyMaterial,material);
 written&=ClimatePut(entity+HazzureOffsets::Schema::m_vTintColor,tint);
 written&=ClimatePut(entity+HazzureOffsets::Schema::m_flBrightnessScale,brightness);return written;

}

static bool RestoreClimate(){

 bool complete=true;

 for(auto& s:climateSkies)if(s.entity){

  if(Entity(s.handle)==s.entity&&SkyEntity(s.entity)){

   if(!PutSkyFields(s.entity,s.material,s.tint,s.brightness)||!UpdateSky(s.entity)){complete=false;continue;}

  }s={};

 }

 return complete;

}

static void ClimateFrameWork(){

 if(climateFault||!shared)return;

 LONG flags=InterlockedCompareExchange(&shared->climateFlags,0,0);

 LONG64 beat=InterlockedCompareExchange64(&shared->heartbeat,0,0);

 bool enabled=(flags&1)&&HazzureWorldAppearance::HeartbeatAlive(beat,GetTickCount64());

 if(!enabled){bool restored=RestoreClimate();ClimateMessage(restored?0:-3,restored?"Clima desligado; ceu original restaurado":"Restauracao do ceu pendente; snapshot preservado");return;}

 uintptr_t local{};HazzureGame::Pawn pawn{};

 if(!HazzureGame::Read(client+HazzureOffsets::Globals::dwLocalPlayerPawn,local)||

    !HazzureGame::ReadPawn(local,pawn,false)){

  RestoreClimate();

  ClimateMessage(1,"Aguardando jogador local; ceu original preservado");return;

 }

 if(GetTickCount64()<climateNext)return;climateNext=GetTickCount64()+250;

 float brightness=shared->skyBrightness;

 if(!(brightness>=0.05f&&brightness<=2.f)){RestoreClimate();ClimateMessage(-2,"Brilho de ceu invalido; original restaurado");return;}

 if(!ClimateDataSchemaMatches()){RestoreClimate();ClimateMessage(-5,"Clima indisponivel: schema do modulo nao validado");return;}

 if(!originalSkyUpdate){RestoreClimate();ClimateMessage(-5,"Ceu indisponivel: callback de atualizacao ausente");return;}

 static int s_activeSkyIdx = -1;

 int curSkyIdx = shared ? shared->skyboxIndex : 0;

 if(!climateLoadAttempted || curSkyIdx != s_activeSkyIdx){

  if(curSkyIdx<0||curSkyIdx>9){RestoreClimate();ClimateMessage(-2,"Indice de skybox invalido");return;}

  fallbackSkyMaterial = MakeSkyMaterial(curSkyIdx);

  s_activeSkyIdx = curSkyIdx;

  climateLoadAttempted = fallbackSkyMaterial!=nullptr;

  climateCloudMaterial = reinterpret_cast<uintptr_t>(fallbackSkyMaterial);

 }

 auto list=Get<uintptr_t>(client+HazzureOffsets::Globals::dwEntityList);int active=0;

 if(!fallbackSkyMaterial){RestoreClimate();ClimateMessage(-4,"Skybox selecionada indisponivel; original preservado");return;}

 // The selected index owns the material. An overcast resource in the map must

 // not silently replace another skybox chosen in the panel.

 climateCloudMaterial=reinterpret_cast<uintptr_t>(fallbackSkyMaterial);

 for(auto& s:climateSkies)if(s.entity&&(Entity(s.handle)!=s.entity||!SkyEntity(s.entity)))s={};

 for(unsigned block=0;block<64;block++){

  auto chunk=Get<uintptr_t>(list+0x10+block*8);if(!chunk)continue;

  for(unsigned i=0;i<512;i++){

   auto slot=chunk+i*0x70,e=Get<uintptr_t>(slot);if(!SkyEntity(e))continue;

   auto handle=HazzureGame::CanonicalHandle(Get<uint32_t>(slot+0x10),Get<uint32_t>(slot+0x30));if(Entity(handle)!=e)continue;

   SkySaved* saved=nullptr;

   for(auto& s:climateSkies)if(s.entity==e&&s.handle==handle){saved=&s;break;}

   if(!saved)for(auto& s:climateSkies)if(!s.entity){s={handle,e,Get<uintptr_t>(e+HazzureOffsets::Schema::m_hSkyMaterial),Get<uint32_t>(e+HazzureOffsets::Schema::m_vTintColor),Get<float>(e+HazzureOffsets::Schema::m_flBrightnessScale)};saved=&s;break;}

   if(!saved)continue;

   uintptr_t material=reinterpret_cast<uintptr_t>(fallbackSkyMaterial);uint32_t tint=shared->skyColour & 0x00FFFFFFu;

   if(saved->updatePending||Get<uintptr_t>(e+HazzureOffsets::Schema::m_hSkyMaterial)!=material||Get<uint32_t>(e+HazzureOffsets::Schema::m_vTintColor)!=tint||Get<float>(e+HazzureOffsets::Schema::m_flBrightnessScale)!=brightness){

    saved->updatePending=true;
    if(!PutSkyFields(e,material,tint,brightness)||!UpdateSky(e))continue;
    saved->updatePending=false;

    InterlockedIncrement64(&shared->climateChanges);

   }active++;

  }

 }

 shared->skyCount=active;

 ClimateMessage(active?2:0,active?"Skybox selecionada aplicada; validar resultado visual":"Aguardando ceu do mapa");

}



// Fog fields are reviewed against client 0x6AE217A2/0x29A2000; precipitation remains legacy-only.

// They are used only after module fingerprint, RTTI class and serial validation.

namespace ClimateFields {

 static constexpr uintptr_t fog=0x600;
 static constexpr uintptr_t fogDensity=0x30;
 static constexpr uintptr_t fogPrimary=0x14;
 static constexpr uintptr_t fogSecondary=0x18;
 static constexpr uintptr_t fogEnabled=0x64,fogBlend=0x65;

 static constexpr uintptr_t precipitationDensity=0x1180,precipitationActive=0x11B4;

}

struct FogSaved{uint32_t handle;uintptr_t entity;float density;uint32_t primary,secondary;uint8_t enabled,blend;};

struct WeatherSaved{uint32_t handle;uintptr_t entity;float density;uint8_t active;};

static FogSaved climateFogs[64]{};

static WeatherSaved climateWeather[128]{};

static ULONGLONG climateEffectsNext;

static unsigned climateFogCount,climateWeatherCount;

static bool ClimateSnapshotValid(uint32_t handle,uintptr_t entity,const char* expected){

 return entity&&handle!=0xffffffffu&&Entity(handle)==entity&&ClimateClassIs(entity,expected);

}

// Widths proven by A78847..A7892C and C05970 in the installed PE:
// RGBA dwords +14/+18, float +30, individual bytes +64/+65. Never touch padding.
static bool ClimateFogSchemaMatches(){
 uint32_t stamp{},size{};
 return HazzureSchema::GetModuleFingerprint("client.dll",stamp,size)&&stamp==0x6AEBC4DAu&&size==0x29A3000u;
}
static bool RestoreFogFields(const FogSaved& s){
 auto fog=s.entity+ClimateFields::fog;
 bool written=ClimatePut(fog+ClimateFields::fogDensity,s.density);
 written&=ClimatePut(fog+ClimateFields::fogPrimary,s.primary);
 written&=ClimatePut(fog+ClimateFields::fogSecondary,s.secondary);
 written&=ClimatePut(fog+ClimateFields::fogEnabled,s.enabled);
 written&=ClimatePut(fog+ClimateFields::fogBlend,s.blend);return written;
}
static bool RestoreFog(){
 if(!ClimateFogSchemaMatches()){for(const auto& s:climateFogs)if(s.entity)return false;return true;}
 bool complete=true;
 for(auto& s:climateFogs)if(s.entity){
  if(ClimateSnapshotValid(s.handle,s.entity,".?AVC_FogController@@")&&!RestoreFogFields(s)){complete=false;continue;}
  s={};
 }
 if(complete)climateFogCount=0;return complete;
}
static void DiscardFogSession(){for(auto& s:climateFogs)s={};climateFogCount=0;climateEffectsNext=0;}

static bool RestoreWeather(){

 bool complete=true;

 for(auto& s:climateWeather)if(s.entity){

  if(ClimateSnapshotValid(s.handle,s.entity,".?AVC_Precipitation@@")){

   bool restored=ClimatePut(s.entity+ClimateFields::precipitationDensity,s.density);

   restored&=ClimatePut(s.entity+ClimateFields::precipitationActive,s.active);

   if(!restored){complete=false;continue;}

  }s={};

 }

 climateWeatherCount=0;

 return complete;

}

static bool RestoreWorldClimate(){bool complete=RestoreFog();return RestoreWeather()&&complete;}

static void FogWeatherFrame(bool fogOnly=false){

 if(!shared)return;

 auto now=GetTickCount64();auto beat=InterlockedCompareExchange64(&shared->heartbeat,0,0);

 float density=shared->fogDensity;

 bool alive=HazzureWorldAppearance::HeartbeatAlive(beat,now);

 bool fogOn=ClimateFogSchemaMatches()&&HazzureWorld::ready.load()&&alive&&std::isfinite(density)&&density>0.f&&density<=1.f;

 bool weatherOff=!fogOnly&&ClimateDataSchemaMatches()&&alive&&InterlockedCompareExchange(&shared->removeWeather,0,0)!=0;

 if(!fogOn)RestoreFog();

 if(!fogOnly&&!weatherOff)RestoreWeather();
 if(!fogOn&&!weatherOff)return;

 if(now<climateEffectsNext)return;climateEffectsNext=now+250;

 auto list=Get<uintptr_t>(client+HazzureOffsets::Globals::dwEntityList);if(!list)return;

 unsigned fogCount=0,weatherCount=0;

 for(auto& s:climateFogs)if(s.entity&&!ClimateSnapshotValid(s.handle,s.entity,".?AVC_FogController@@"))s={};

 for(auto& s:climateWeather)if(s.entity&&!ClimateSnapshotValid(s.handle,s.entity,".?AVC_Precipitation@@"))s={};

 for(unsigned block=0;block<64;++block){

  auto chunk=Get<uintptr_t>(list+0x10+block*8);if(!chunk)continue;

  for(unsigned index=0;index<512;++index){

   auto slot=chunk+index*0x70,e=Get<uintptr_t>(slot);if(!e)continue;

   auto handle=HazzureGame::CanonicalHandle(Get<uint32_t>(slot+0x10),Get<uint32_t>(slot+0x30));if(Entity(handle)!=e)continue;

   if(fogOn&&ClimateClassIs(e,".?AVC_FogController@@")){

    FogSaved* saved=nullptr;

    for(auto& s:climateFogs)if(s.entity==e&&s.handle==handle){saved=&s;break;}

    if(!saved)for(auto& s:climateFogs)if(!s.entity){

     FogSaved candidate{};candidate.entity=e;candidate.handle=handle;

     auto fog=e+ClimateFields::fog;

     if(!Read(reinterpret_cast<void*>(fog+ClimateFields::fogDensity),&candidate.density,sizeof(candidate.density))||

        !std::isfinite(candidate.density)||candidate.density<0.f||candidate.density>1.f||

        !Read(reinterpret_cast<void*>(fog+ClimateFields::fogPrimary),&candidate.primary,sizeof(candidate.primary))||

        !Read(reinterpret_cast<void*>(fog+ClimateFields::fogSecondary),&candidate.secondary,sizeof(candidate.secondary))||

        !Read(reinterpret_cast<void*>(fog+ClimateFields::fogEnabled),&candidate.enabled,sizeof(candidate.enabled))||candidate.enabled>1||
        !Read(reinterpret_cast<void*>(fog+ClimateFields::fogBlend),&candidate.blend,sizeof(candidate.blend))||candidate.blend>1)break;

     s=candidate;saved=&s;break;

    }

    if(saved){

     auto fog=e+ClimateFields::fog;uint8_t enabled=1,blend=0;
     // One IPC colour means uniform fog. Preserve each original alpha byte;
     // disable directional blending instead of inventing a second colour.
     uint32_t rgb=shared->fogColour&0x00ffffffu;
     uint32_t primary=rgb|(saved->primary&0xff000000u),secondary=rgb|(saved->secondary&0xff000000u);
     bool written=ClimatePut(fog+ClimateFields::fogDensity,density);
     written&=ClimatePut(fog+ClimateFields::fogPrimary,primary);
     written&=ClimatePut(fog+ClimateFields::fogSecondary,secondary);
     written&=ClimatePut(fog+ClimateFields::fogEnabled,enabled);
     written&=ClimatePut(fog+ClimateFields::fogBlend,blend);
     if(written)++fogCount;else RestoreFogFields(*saved);

    }

   }else if(weatherOff&&ClimateClassIs(e,".?AVC_Precipitation@@")){

    WeatherSaved* saved=nullptr;

    for(auto& s:climateWeather)if(s.entity==e&&s.handle==handle){saved=&s;break;}

    if(!saved)for(auto& s:climateWeather)if(!s.entity){

     WeatherSaved candidate{};candidate.entity=e;candidate.handle=handle;

     if(!Read(reinterpret_cast<void*>(e+ClimateFields::precipitationDensity),&candidate.density,sizeof(candidate.density))||

        !std::isfinite(candidate.density)||candidate.density<0.f||

        !Read(reinterpret_cast<void*>(e+ClimateFields::precipitationActive),&candidate.active,sizeof(candidate.active))||candidate.active>1)break;

     s=candidate;saved=&s;break;

    }

    if(saved){float none=0.f;bool inactive=false;

     bool written=ClimatePut(e+ClimateFields::precipitationDensity,none);

     written&=ClimatePut(e+ClimateFields::precipitationActive,inactive);if(written)++weatherCount;

    }

   }

  }

 }

 static bool hadReport=false;

 if(!hadReport||climateFogCount!=fogCount||climateWeatherCount!=weatherCount){

  LogDebug("[CLIMA] Fog aplicado em %u controller(s); precipitacao desativada em %u entidade(s)",fogCount,weatherCount);hadReport=true;

 }

 climateFogCount=fogCount;climateWeatherCount=weatherCount;

}

static void CleanMapFrame(){

 // Build 14192 exposes no validated C_CascadeLight::m_flMaxShadowDistance.

 // A plausible numeric offset is not a schema resolution and must not be written.

 static bool warned=false;

 if(shared&&shared->cleanMap&&!warned){LogDebug("[CLIMA] Clean Map indisponivel: campo de sombras nao validado nesta build");warned=true;}

 if(!shared||!shared->cleanMap)warned=false;

}

static void ClimateFrame(){

 if(!shared||climateFault)return;

 __try{ExposureFrame();ClimateFrameWork();FogWeatherFrame();CleanMapFrame();}

 __except(EXCEPTION_EXECUTE_HANDLER){climateFault=true;InterlockedIncrement64(&shared->faults);ClimateMessage(-1,"Falha no clima; processamento interrompido");}

}

static void ResetClimateSession(){

 // A level transition retires old identities, but the previous map's scene object
 // keeps whatever material we pushed into m_hSkyMaterial: UpdateSky propagates that
 // strong handle into it, and the engine's own sky refresh later calls the freed
 // object (client+0x230DAF, g_tSkyTexture / g_flBrightnessExposureBias). So restore
 // while the entity still resolves, then discard; restoring into a retiring map is
 // SEH-guarded and a no-op when it can no longer resolve.

 bool restored=false;

 __try{restored=RestoreClimate();}__except(EXCEPTION_EXECUTE_HANDLER){restored=false;}

 LogDebug("[CLIMA] Transicao de mapa: ceu %s antes de descartar snapshots",restored?"restaurado":"nao restauravel");

 for(auto& s:climateSkies)s={};for(auto& s:climateFogs)s={};for(auto& s:climateWeather)s={};

 exposureSaved={};climateFault=false;climateLoadAttempted=false;climateCloudMaterial.store(0);

 climateNext=climateEffectsNext=0;climateFogCount=climateWeatherCount=0;

}



static bool ClimateAlive(LONG flag){

 if(!shared)return false;

 auto beat=InterlockedCompareExchange64(&shared->heartbeat,0,0);

 return (InterlockedCompareExchange(&shared->climateFlags,0,0)&flag)&&HazzureWorldAppearance::HeartbeatAlive(beat,GetTickCount64());

}

static void SkyDrawHook(void* a,void* b,void* mesh,int count,void* e,void* f,void* g,void* h){

 // AOB-validated hook installed in InternalStartup.inl (scenesystem.dll).
 // Render workers only forward the original batch; they never mutate shared
 // scene memory or restore an object that the engine may concurrently replace.

 if(shared&&ClimateAlive(1))InterlockedIncrement64(&shared->skyDraws);

 if(originalSkyDraw)originalSkyDraw(a,b,mesh,count,e,f,g,h);

}

// stage: 0 | reason: preset definitions para climate pipeline
struct ClimatePresetValues { float hue,sat,val; };
static constexpr ClimatePresetValues kClimatePresets[] = {
    {320.f, 40.f,  8.f},  // 0 = pink_violet
    {200.f, 28.f, -2.f},  // 1 = cold
    {0.f,  -95.f,  6.f},  // 2 = mono (hue neutral)
    {0.f,   0.f,   0.f},  // 3 = custom (all neutral, user edits)
};

// stage: 6 | reason: interpolacao 120ms por canal entre presets
static float climateCurrentHue = 320.f, climateCurrentSat = 40.f, climateCurrentVal = 8.f;
static ULONGLONG climateInterpLast = 0;

static void ClimateInterpolatePreset(float targetH, float targetS, float targetV) {
    auto now = GetTickCount64();
    if (!climateInterpLast) { climateInterpLast = now; climateCurrentHue = targetH; climateCurrentSat = targetS; climateCurrentVal = targetV; return; }
    float dt = (float)(now - climateInterpLast);
    climateInterpLast = now;
    constexpr float kRate = 120.f; // ms per channel full transition
    float t = (dt >= kRate) ? 1.f : dt / kRate;
    auto lerp = [](float a, float b, float f) { return a + (b - a) * f; };
    climateCurrentHue = lerp(climateCurrentHue, targetH, t);
    climateCurrentSat = lerp(climateCurrentSat, targetS, t);
    climateCurrentVal = lerp(climateCurrentVal, targetV, t);
}

// stage: 6 | reason: ClimateApplyPreset movido para overlay-side (PushConfig) onde g_cfg é acessível

