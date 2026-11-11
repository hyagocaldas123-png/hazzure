#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <vector>
#include <cstdio>
#include <limits>
#include "../src/CameraFov.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
template<class T>static void Put(uintptr_t at,const T& value){memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));}
static bool Accept(uintptr_t){return true;}static bool Reject(uintptr_t){return false;}
int main(){
 using namespace HazzureOffsets;using namespace HazzureCamera;
 std::vector<unsigned char> module(0x1000),listBytes(0x210),chunkBytes(512*0x70),pawnBytes(0x4000),cameraBytes(0x400);
 auto base=reinterpret_cast<uintptr_t>(module.data()),list=reinterpret_cast<uintptr_t>(listBytes.data()),chunk=reinterpret_cast<uintptr_t>(chunkBytes.data());
 auto pawn=reinterpret_cast<uintptr_t>(pawnBytes.data()),camera=reinterpret_cast<uintptr_t>(cameraBytes.data());
 IMAGE_DOS_HEADER dos{};dos.e_magic=IMAGE_DOS_SIGNATURE;dos.e_lfanew=0x100;Put(base,dos);
 IMAGE_NT_HEADERS64 nt{};nt.Signature=IMAGE_NT_SIGNATURE;nt.FileHeader.TimeDateStamp=0x6ada7340;nt.OptionalHeader.SizeOfImage=0x29a0000;Put(base+0x100,nt);
 Globals::dwEntityList=0x20;Globals::dwLocalPlayerPawn=0x28;Put(base+0x20,list);Put(base+0x28,pawn);Put(list+0x10,chunk);
 auto identity=chunk+65*0x70;Put(identity,pawn);Put(identity+0x10,uint32_t(0x8041));Put(pawn+0x10,identity);
 Put(pawn+Schema::m_iHealth,int(100));Put(pawn+Schema::m_iTeamNum,uint8_t(2));Put(pawn+Schema::m_pCameraServices,camera);
 Put(camera+0x298,uint32_t(90));Put(camera+0x29c,uint32_t(90));
 HazzureTuning::Settings option;option.camera=true;option.cameraFov=124.75f;HazzureTuning::Configure(option);
 auto resolve=[&](float native=90.f,bool allowed=true){return Resolve(base,camera,native,allowed,Accept);};
 CHECK(resolve()==124.75f&&status.load()==Status::Applied,"effective getter result replaces the final native FOV without rounding");
 uint32_t endpoint{};HazzureGame::Read(camera+0x298,endpoint);
 CHECK(endpoint==90,"effective override leaves network FOV and zoom transition fields untouched");
 CHECK(resolve(74.f)==124.75f,"a late weapon FOV calculation cannot overwrite the selected final return");
 CHECK(resolve(90.f,false)==90.f,"loading or suspended feature processing preserves the native result");
 CHECK(Resolve(base,camera+8,90,true,Accept)==90,"foreign camera services do not receive the local player's override");
 CHECK(Resolve(base,camera,90,true,Reject)==90,"a non-player class cannot receive a camera override");
 Put(pawn+0x1ea0,uint8_t(1));
 CHECK(resolve(30)==30&&status.load()==Status::Scoped&&delivered.load()==30,"scoped sniper zoom keeps its native FOV");
 Put(pawn+0x1ea0,uint8_t(0));
 CHECK(resolve()==124.75f,"leaving scope restores the selected world FOV on the next getter call");
 option.camera=false;HazzureTuning::Configure(option);
 CHECK(resolve(90)==90&&status.load()==Status::Disabled,"disabling the option returns the original result immediately without stale field restoration");
 option.camera=true;option.cameraFov=60;HazzureTuning::Configure(option);CHECK(resolve()==60,"camera range accepts the 60 degree endpoint");
 option.cameraFov=140;HazzureTuning::Configure(option);CHECK(resolve()==140,"camera range accepts the 140 degree endpoint");
 option.cameraFov=std::numeric_limits<float>::quiet_NaN();HazzureTuning::Configure(option);CHECK(resolve()==90,"nonfinite requested FOV cannot reach the renderer");
 option.cameraFov=141;HazzureTuning::Configure(option);CHECK(resolve()==90,"out of range requested FOV preserves the native result");
 option.cameraFov=105;HazzureTuning::Configure(option);
 Put(pawn+Schema::m_iHealth,int(0));CHECK(resolve()==90,"dead local players preserve native observer camera behavior");Put(pawn+Schema::m_iHealth,int(100));
 Put(identity+0x10,uint32_t(0x10041));Put(identity+0x30,uint32_t(1));CHECK(resolve()==105,"normalized serial flags allow a valid live camera");
 Put(identity,reinterpret_cast<uintptr_t>(cameraBytes.data()));CHECK(resolve()==90,"a reused entity slot cannot receive a stale local camera override");Put(identity,pawn);
 auto changedCamera=camera+0x100;Put(pawn+Schema::m_pCameraServices,changedCamera);
 CHECK(resolve()==90&&Resolve(base,changedCamera,90,true,Accept)==105,"replaced camera services invalidate the old binding and accept the new one");Put(pawn+Schema::m_pCameraServices,camera);
 nt.FileHeader.TimeDateStamp=0;Put(base+0x100,nt);CHECK(resolve()==90&&!Supported(base),"unknown builds preserve native camera results");
 nt.FileHeader.TimeDateStamp=0x6ada7340;Put(base+0x100,nt);
 CHECK(resolve(std::numeric_limits<float>::quiet_NaN())!=resolve(std::numeric_limits<float>::quiet_NaN()),"invalid native results are not converted into an apparently valid override");
 installed=false;CHECK(strstr(Label(GetTickCount64()),"Indisponivel")!=nullptr,"an uninstalled getter is reported as unavailable instead of applied");
 installed=true;status=Status::Applied;lastDelivery=100;CHECK(strstr(Label(1101),"Aguardando")!=nullptr,"stale camera telemetry cannot claim a current application");
 CHECK(strstr(Label(99),"Aguardando")!=nullptr,"clock rollback cannot expose stale application status");
 installed=false;
 printf("RESULT: %u passed, %u failed; own process, no engine calls or OS input.\n",passed,failed);return failed?1:0;
}
