#define INTERNAL_SELF_TEST
#define NOMINMAX
#include "../src/HazzureUI.h"
#include <cstdio>
static unsigned checks;
#define CHECK(v) do{++checks;if(!(v)){printf("FAIL %d: %s\n",__LINE__,#v);return 1;}}while(0)
int main(){
 using namespace HazzureUI;
 Shared ipc{};g_cfg={};g_cfg.skinsEnabled=true;ipc.skinEnabled=1;
 HazzureInventory::Item item{};strcpy_s(item.label,"inventory independent");item.skin={7,180,661,0,.12f,1,1,0};
 CHECK(HazzureInventory::Add(g_cfg.inventory,item));
 CHECK(ApplySharedConfig(&ipc));CHECK(!ipc.skinEnabled);CHECK(g_cfg.inventory.count==1);CHECK(!ipc.skins[2].enabled);
 ipc.skinEnabled=1;CHECK(CaptureSharedConfig(&ipc));CHECK(!g_cfg.skinsEnabled&&g_cfg.inventory.count==1);
 HazzureConfig::Document document;std::string error;CHECK(document.Parse("{\"skinsEnabled\":true}",error));
 OverlayConfig loaded{};loaded.skinsEnabled=true;CHECK(Persistence::ReadConfig(document.Root(),loaded,error));CHECK(!loaded.skinsEnabled);
 loaded.inventory=g_cfg.inventory;loaded.agentEnabled=true;
 CHECK(Persistence::WriteConfig(document,loaded,{}, {}, {},-1,error));CHECK(!document.Root()["skinsEnabled"].GetBool());
 OverlayConfig roundtrip{};CHECK(Persistence::ReadConfig(document.Root(),roundtrip,error));
 CHECK(roundtrip.inventory.count==1&&roundtrip.inventory.items[0].skin.paint==180&&roundtrip.agentEnabled);
 ipc.revision=3;CHECK(!ApplySharedConfig(&ipc));CHECK(g_cfg.inventory.count==1);
 auto native=HazzureNativeInventory::Snapshot();CHECK(!native.enabled&&!native.requested);
 printf("PASS %u inventory-only persistence/IPC checks; no window, input, audio or game.\n",checks);
}
