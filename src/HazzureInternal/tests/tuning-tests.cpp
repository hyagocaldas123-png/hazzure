// C++ / tuning-tests.cpp / MSVC x64 C++17 / synthetic entity identities and field restoration
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <vector>
#include <cstdio>
#include "../src/BasicTuning.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
static bool Accept(uintptr_t){return true;}static bool Reject(uintptr_t){return false;}
template<class T>static void Put(uintptr_t at,const T& value){memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));}
template<class T>static T Get(uintptr_t at){T value{};HazzureGame::Read(at,value);return value;}
int main(){
 using namespace HazzureOffsets;using namespace HazzureTuning;
 std::vector<unsigned char> module(0x1000),listBytes(0x210),chunkBytes(512*0x70),localBytes(0x4000),enemyBytes(0x4000),allyBytes(0x4000),controllerBytes(0x1000),allyControllerBytes(0x1000),cameraBytes(0x400);
 auto base=reinterpret_cast<uintptr_t>(module.data()),list=reinterpret_cast<uintptr_t>(listBytes.data()),chunk=reinterpret_cast<uintptr_t>(chunkBytes.data());
 auto local=reinterpret_cast<uintptr_t>(localBytes.data()),enemy=reinterpret_cast<uintptr_t>(enemyBytes.data()),ally=reinterpret_cast<uintptr_t>(allyBytes.data()),camera=reinterpret_cast<uintptr_t>(cameraBytes.data());
 IMAGE_DOS_HEADER dos{};dos.e_magic=IMAGE_DOS_SIGNATURE;dos.e_lfanew=0x100;Put(base,dos);
 IMAGE_NT_HEADERS64 nt{};nt.Signature=IMAGE_NT_SIGNATURE;nt.FileHeader.TimeDateStamp=0x6ada7340;nt.OptionalHeader.SizeOfImage=0x29a0000;Put(base+0x100,nt);
 Globals::dwEntityList=0x20;Globals::dwLocalPlayerPawn=0x28;Put(base+0x20,list);Put(base+0x28,local);Put(list+0x10,chunk);
 auto add=[&](unsigned index,uintptr_t pawn,uint8_t team){uint32_t h=0x8000u+index;Put(chunk+index*0x70,pawn);Put(chunk+index*0x70+0x10,h);Put(pawn+0x10,chunk+index*0x70);Put(pawn+Schema::m_iHealth,int(100));Put(pawn+Schema::m_iTeamNum,team);return h;};
 auto localId=add(65,local,2),enemyId=add(70,enemy,3);add(71,ally,2);
 auto controller=reinterpret_cast<uintptr_t>(controllerBytes.data()),allyController=reinterpret_cast<uintptr_t>(allyControllerBytes.data());
 Put(chunk+0x70,controller);Put(controller+Schema::m_hPlayerPawn,enemyId);Put(chunk+2*0x70,allyController);Put(allyController+Schema::m_hPlayerPawn,uint32_t(0x8047));
 Put(local+Schema::m_pCameraServices,camera);Put(camera+0x298,uint32_t(90));Put(camera+0x29c,uint32_t(90));
 Put(local+0x150c,255.f);Put(local+0x1504,200.f);
 const float original[]={1,2,3,68};for(unsigned i=0;i<4;i++)Put(local+0x1dc0+i*4,original[i]);
 Settings option;option.flash=option.radar=option.camera=option.viewmodel=true;option.x=4;option.y=5;option.z=-2;option.handFov=75;
 Configure(option);Frame(base,Reject);
 CHECK(Get<float>(local+0x150c)==255&&Get<uint8_t>(enemy+0x1e90)==0,"class mismatch prevents all local/radar writes");
 Frame(base,Accept);
 CHECK(Get<float>(local+0x150c)==63.75f&&Get<float>(local+0x1504)==63.75f,"flash maximum and current overlay respect the configured alpha cap");
 CHECK(Get<uint32_t>(camera+0x298)==90&&Get<uint32_t>(camera+0x29c)==90,"frame tuning no longer writes camera network endpoints before the weapon calculation");
 CHECK(Get<float>(local+0x1dc0)==4&&Get<float>(local+0x1dcc)==75,"viewmodel position and hand FOV use their class-qualified fields");
 CHECK(Get<uint8_t>(enemy+0x1e90)==1&&Get<uint8_t>(ally+0x1e90)==0,"native radar marks enemies without modifying allies");
 CHECK(Matches(list,localId,local)&&Matches(list,enemyId,enemy),"identity checks include the complete serialized handle");
 Put(chunk+70*0x70+0x10,uint32_t(enemyId+0x8000));Put(chunk+70*0x70+0x30,uint32_t(1));
 CHECK(Matches(list,enemyId,enemy),"identity flags normalize serialized handles instead of rejecting a live enemy");
 Put(chunk+70*0x70+0x10,enemyId);Put(chunk+70*0x70+0x30,uint32_t(0));
 Put(local+0x150c,128.f);Put(local+0x1dc0,2.f);Frame(base,Accept);
 CHECK(Get<float>(local+0x150c)==63.75f&&Get<float>(local+0x1dc0)==4,"fresh engine settings are captured before the override is reapplied");
 Put(local+0x1ea0,uint8_t(1));Put(camera+0x298,uint32_t(30));Put(camera+0x29c,uint32_t(30));Frame(base,Accept);
 CHECK(Get<uint32_t>(camera+0x298)==30,"camera override preserves a native scoped FOV");
 Configure({});Frame(base,Accept);
 CHECK(Get<float>(local+0x150c)==128.f&&Get<float>(local+0x1504)==200.f,"disabling flash cap restores the latest engine-owned values");
 CHECK(Get<float>(local+0x1dc0)==2&&Get<float>(local+0x1dcc)==68,"viewmodel restoration preserves newer native position settings");
 CHECK(Get<uint8_t>(enemy+0x1e90)==0,"disabling native radar restores the tracked spotted value");
 Put(local+0x1ea0,uint8_t(0));Put(camera+0x298,uint32_t(90));Put(camera+0x29c,uint32_t(90));Configure(option);Frame(base,Accept);Restore(base);
 CHECK(Get<uint32_t>(camera+0x298)==90&&Get<float>(local+0x150c)==128,"resident suspension restores camera and flash overrides");
 Frame(base,Accept);Put(chunk+70*0x70+0x10,uint32_t(0x10046));Put(enemy+0x1e90,uint8_t(0));Restore(base);
 CHECK(Get<uint8_t>(enemy+0x1e90)==0&&!Matches(list,enemyId,enemy),"recycled identity never receives a stale radar restoration");
 auto page=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!page)return 2;Put(page,1.f);Patch<float> patch;CHECK(patch.Apply(page,2.f),"field patch preserves its original value");
 DWORD prior{},ignored{};VirtualProtect(reinterpret_cast<void*>(page),4096,PAGE_READONLY,&prior);
 CHECK(!patch.Restore(true)&&patch.active,"failed restoration retains its snapshot for retry");
 VirtualProtect(reinterpret_cast<void*>(page),4096,prior,&ignored);
 CHECK(patch.Restore(true)&&Get<float>(page)==1.f&&!patch.active,"restoration succeeds when write access returns");
 VirtualFree(reinterpret_cast<void*>(page),0,MEM_RELEASE);
 printf("RESULT: %u passed, %u failed; synthetic memory only.\n",passed,failed);return failed?1:0;
}
