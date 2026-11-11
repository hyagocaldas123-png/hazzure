// C++ / game-data-tests.cpp / MSVC x64 C++17 / own-memory snapshots and pure math
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <cstdio>
#include <limits>
#include "../src/GameData.h"

static unsigned passed,failed;
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
template<class T> static void Put(uintptr_t base,size_t offset,const T& value){memcpy(reinterpret_cast<void*>(base+offset),&value,sizeof(value));}
int main(){
    using namespace HazzureGame;using namespace HazzureOffsets;
    std::vector<unsigned char> storage(4096),listData(1024),chunkData(512*0x70),secondChunk(512*0x70),pawnData(0x4000),sceneData(0x400),controllerData(0x1000);
    uintptr_t client=reinterpret_cast<uintptr_t>(storage.data()),list=reinterpret_cast<uintptr_t>(listData.data());
    uintptr_t chunk=reinterpret_cast<uintptr_t>(chunkData.data()),chunk2=reinterpret_cast<uintptr_t>(secondChunk.data());
    uintptr_t pawn=reinterpret_cast<uintptr_t>(pawnData.data()),scene=reinterpret_cast<uintptr_t>(sceneData.data()),controller=reinterpret_cast<uintptr_t>(controllerData.data());
    Globals::dwEntityList=0x20;Globals::dwLocalPlayerPawn=0x28;Globals::dwLocalPlayerController=0x30;
    Put(client,0x20,list);Put(client,0x28,pawn);Put(client,0x30,controller);Put(list,0x10,chunk);Put(list,0x20,chunk2);
    uint32_t handle=(12u<<15)|1260u;Put(chunk2,(1260u&511)*0x70,pawn);Put(chunk2,(1260u&511)*0x70+0x10,handle);
    Put(controller,Schema::m_hPlayerPawn,handle);
    Put(pawn,Schema::m_iHealth,int32_t(255));Put(pawn,Schema::m_iTeamNum,uint8_t(2));Put(pawn,Schema::m_lifeState,uint8_t(0));
    Put(pawn,Schema::m_pGameSceneNode,scene);Put(scene,Schema::m_vecAbsOrigin,Vec3{11636,-4128,8416});
    Put(pawn,Schema::m_vecVelocity,Vec3{120,-30,0});Put(pawn,Schema::m_vecViewOffset,Vec3{0,0,64});Put(pawn,Schema::m_angEyeAngles,Vec3{0,179,0});
    Pawn snapshot{};
    CHECK(ReadPawn(pawn,snapshot)&&snapshot.health==255&&snapshot.eye.z==8480,"health above 100 remains valid; eye position uses the scene");
    CHECK(Entity(list,handle)==pawn&&Entity(list,0)==0&&Entity(list,0xffffffffu)==0,"slots above 64 resolve directly; zero handles are rejected");
    CHECK(LocalPawn(client)==pawn,"valid published pointer is used");
    Put(client,0x28,controller);CHECK(LocalPawn(client)==pawn,"invalid candidate is rejected and controller handle recovers the pawn");
    Put(pawn,Schema::m_vecVelocity,Vec3{0,-4.99e30f,0});
    CHECK(ReadPawn(pawn,snapshot)&&!snapshot.velocityValid&&snapshot.velocity.y==0,"absurd velocity is rejected before integer conversion");
    Put(pawn,Schema::m_iHealth,int32_t(0));
    CHECK(!ReadPawn(pawn,snapshot)&&ReadPawn(pawn,snapshot,false),"dead state is distinguished from an unreadable object");
    Matrix4 matrix{};for(int i=0;i<4;++i)matrix.m[i][i]=1;
    Vec2 screen{};CHECK(Project({0,0,0},matrix,1920,1080,screen)&&screen.x==960&&screen.y==540,"projection maps the origin to the viewport center");
    matrix.m[3][3]=-1;CHECK(!Project({0,0,0},matrix,1920,1080,screen),"behind-camera projection is rejected");matrix.m[3][3]=1;
    Vec2 angular{};CHECK(AimAngles({0,0,0},{1,0,1},{0,0,0},angular)&&std::fabs(angular.x+45)<.001f&&std::fabs(angular.y)<.001f,"3D direction converts to pitch and yaw");
    CHECK(std::fabs(NormalizeAngle(-358)-2)<.001f&&std::fabs(NormalizeAngle(358)+2)<.001f,"angle wrap follows the short rotation");
    float sixty=1,high=1;for(int i=0;i<60;++i)sixty*=1-SmoothAlpha(8,1.f/60);for(int i=0;i<240;++i)high*=1-SmoothAlpha(8,1.f/240);
    CHECK(std::fabs(sixty-high)<.00001f,"smoothing has the same result across frame rates");
    MouseRemainder remainder;LONG dx{},dy{};remainder.Quantize(.4f,-.4f,dx,dy);remainder.Quantize(.4f,-.4f,dx,dy);
    CHECK(remainder.Quantize(.4f,-.4f,dx,dy)&&dx==1&&dy==-1,"fractional relative motion accumulates with the correct sign");
    CHECK(!remainder.Quantize(std::numeric_limits<float>::infinity(),0,dx,dy)&&remainder.x==0,"nonfinite motion is rejected and state reset");
    TriggerClock clock;CHECK(!clock.Ready(123,100,200)&&clock.Ready(123,300,200),"reaction timer waits for uninterrupted acquisition");
    clock.Fired(300,0);CHECK(!clock.Ready(123,301,0)&&clock.Ready(123,345,0),"cooldown cannot underflow when the next delay is zero");
    clock.Reset();CHECK(!clock.Ready(123,400,200)&&!clock.Ready(0,700,0)&&!clock.Ready(123,701,200),"losing the target cancels the previous reaction interval");
    CHECK(FovRadius(5,matrix,1920)>80&&FovRadius(5,matrix,1920)<90,"angular FOV uses the projection focal length");
    JumpPulse jump;
    CHECK(jump.Update(false,false,true,0)==-1,"disabled bhop never writes an unowned jump button");
    CHECK(jump.Update(true,true,true,100)==65537,"stationary grounded activation produces a press");
    CHECK(jump.Update(true,true,true,105)==256,"press has a separate release on the next update");
    CHECK(jump.Update(true,true,true,110)==256,"jump cooldown prevents an early second press");
    CHECK(jump.Update(true,true,true,116)==65537,"grounded retry is possible after cooldown");
    CHECK(jump.Update(true,true,false,120)==256,"airborne update releases without OS keyboard feedback");
    CHECK(jump.Update(false,true,true,125)==256&&jump.Update(false,true,true,126)==-1,"focus loss releases ownership exactly once");
    CHECK(jump.Update(true,true,true,130)==65537&&jump.Update(true,false,true,135)==256,"physical activation release cancels the jump");
    printf("%u passed; %u failed. No process attachment or OS input.\n",passed,failed);return failed?1:0;
}
