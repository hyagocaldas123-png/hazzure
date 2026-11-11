// C++ / skin-engine-tests.cpp / MSVC x64 C++17 / synthetic memory only
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include "../../Compartilhado/ipc.h"
#include "../src/StateRegistry.h"

static Shared* shared;
static uintptr_t client;
struct TestEntity {uint32_t handle;uintptr_t pointer;bool hud;};
static TestEntity entities[8]{};
static unsigned zeroEntityCalls;
static bool Read(const void* source,void* output,size_t count) {
    if(!source||!output)return false;
    __try{memcpy(output,source,count);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T> static T Get(uintptr_t address){T value{};Read(reinterpret_cast<void*>(address),&value,sizeof(value));return value;}
static uintptr_t Entity(uint32_t handle) {
    if(!handle)++zeroEntityCalls;
    for(const auto& entity:entities)if(entity.handle==handle&&entity.pointer)return entity.pointer;
    return 0;
}
static bool ClassName(uintptr_t pointer,char* output,size_t length) {
    for(const auto& entity:entities)if(entity.pointer==pointer&&entity.hud) {
        strcpy_s(output,length,".?AVC_CS2HudModelWeapon@@");return true;
    }
    return false;
}
static bool PrecacheModelResource(const char*){return true;}
// Climate is outside this synthetic skin fixture; production callback coverage lives in native-inventory-frame-tests.
static void DiscardFogSession(){}
static bool RestoreFog(){return true;}
static void FogWeatherFrame(bool){}
static void ClimateFrame(){}
static void ResetClimateSession(){}
static void ResetMaterialSession(){}
static void LogDebug(const char*,...){}
static bool RestoreWorldClimate(){return true;}
#include "../src/SkinEngine.h"

static unsigned passed,failed;
#define CHECK(expression,label) do{if(expression){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)

struct AttachmentFixture {
    bool available=true,valid=true,invalidateAfterFirst{};unsigned calls{},checks{},failCall{};
    uintptr_t expectedItem=200,targets[24]{},items[24]{};
    HazzureArmoryVisual::Module modules[24]{};
};
static bool FixtureAvailable(void* p){return static_cast<AttachmentFixture*>(p)->available;}
static bool FixtureBinding(void* p,const HazzureArmoryVisual::Token& token,uintptr_t target){
    auto& f=*static_cast<AttachmentFixture*>(p);++f.checks;
    return f.valid&&token.item==f.expectedItem&&(target==token.weapon||target==token.hud);
}
static bool FixtureInvoke(void* p,uintptr_t target,uintptr_t item,HazzureArmoryVisual::Module module){
    auto& f=*static_cast<AttachmentFixture*>(p);auto i=f.calls++;
    if(i<24){f.targets[i]=target;f.items[i]=item;f.modules[i]=module;}
    if(f.invalidateAfterFirst)f.valid=false;
    return !f.failCall||f.calls!=f.failCall;
}

int main() {
    Shared ipc{};shared=&ipc;
    ipc.revision=2;ipc.skinEnabled=1;ipc.skins[0]={7,180,123,0,.15f,1,1,1};
    CHECK(SnapshotSettings()&&skinOn&&skinRevision==2,"configuration revision snapshot accepted");
    ipc.skinEnabled=0;CHECK(SnapshotSettings()&&!skinOn,"enable toggle works without unrelated revision change");
    ipc.revision=3;CHECK(!SnapshotSettings()&&skinRevision==2,"odd revision rejected without committing");
    ipc.revision=4;ipc.skins[0].model=507;
    CHECK(!SnapshotSettings()&&skinRevision==2,"knife model on a rifle rejected");
    ipc.skins[0]={5030,0,0,5030,.1f,1,1,1};
    CHECK(SnapshotSettings(),"inline glove configuration accepted");
    CHECK(!Glove(5028)&&!Glove(5029)&&Glove(5027)&&Glove(5035),"unsupported glove definitions rejected");
    CHECK(strcmp(GlovePath(5030),"agents/models/shared/arms/glove_sporty/glove_sporty.vmdl")==0,"glove resource matches installed VPK path");
    CHECK(!SkinSameModel("agents/models/ctm_fbi/ctm_fbi_variantb.vmdl","characters/models/ctm_fbi/ctm_fbi_variantb.vmdl"),"dummy characters model is not equivalent to a full agents model");
    CHECK(SkinSameModel(AgentPathCT(1),AgentPathCT(1)),"identical complete model does not force a reset");
    CHECK(kAgentResourcesCT[1].meshMask==5&&kAgentResourcesCT[2].meshMask==17&&kAgentResourcesT[1].meshMask==1,"agent masks preserve model-authored body variants instead of enabling every variant");
    CHECK(AgentPathCT(11)==nullptr&&AgentPathCT(-1)==nullptr&&AgentPathT(16)==nullptr,"unavailable and out of range agent selections do not apply a placeholder");
    CHECK(SkinEntity(0)==0&&zeroEntityCalls==0,"zero handle cannot resolve and modify world entity");

    alignas(8) unsigned char weapon[0x3000]{},scene[0x500]{},attributes[5*0x48]{};
    alignas(8) unsigned char hud[0x3000]{},hudScene[0x500]{};
    char weaponModel[]="weapons/models/m4a4/weapon_rif_m4a4.vmdl";
    char hudModel[]="weapons/models/m4a4/weapon_rif_m4a4.vmdl";
    uintptr_t w=reinterpret_cast<uintptr_t>(weapon),s=reinterpret_cast<uintptr_t>(scene);
    uintptr_t h=reinterpret_cast<uintptr_t>(hud),hs=reinterpret_cast<uintptr_t>(hudScene);
    entities[0]={0x10001,w,false};entities[1]={0x20002,h,true};
    SkinWrite(w+HazzureOffsets::Schema::m_pGameSceneNode,s);
    SkinWrite(s+HazzureOffsets::Schema::m_modelState+0xA8,reinterpret_cast<uintptr_t>(weaponModel));
    SkinWrite(s+HazzureOffsets::Schema::m_modelState+0x208,uint64_t(7));
    SkinWrite(w+HazzureOffsets::Schema::m_iItemDefinitionIndex,static_cast<unsigned short>(16));
    SkinWrite(w+HazzureOffsets::Schema::m_flFallbackWear,.13f);
    auto originalItem=ArmoryItem(w);SkinWrite(originalItem+0x1bc,17);SkinWrite(w+HazzureOffsets::Schema::m_nFallbackStatTrak,321);
    char originalNames[322]{};strcpy_s(originalNames,161,"Nome original");strcpy_s(originalNames+161,161,"Override original");
    SkinWrite(originalItem+0x2f8,originalNames,sizeof(originalNames));
    SkinWrite(SkinDynamicList(w)+8,2);SkinWrite(SkinDynamicList(w)+16,reinterpret_cast<uintptr_t>(attributes));
    SkinWrite(w+kSkinHudWeaponHandle,uint32_t(0x20002));
    SkinWrite(h+HazzureOffsets::Schema::m_pGameSceneNode,hs);
    SkinWrite(hs+HazzureOffsets::Schema::m_modelState+0xA8,reinterpret_cast<uintptr_t>(hudModel));
    SkinWrite(hs+HazzureOffsets::Schema::m_modelState+0x208,uint64_t(9));
    attributes[0]=0xAB;attributes[0x48]=0xCD;
    SkinSaved saved{};
    CHECK(SkinDynamicList(w)-w==0x1560,"network dynamic list composition stays distinct from ordinary list");
    CHECK(SaveSkin(&saved,w,0x10001)&&saved.attributeCount==2&&saved.attributes[0]==0xAB&&saved.definition==16,"complete original weapon snapshot preserved");
    CHECK(saved.hudEntity==h&&saved.hudMask==9&&saved.mask==7,"HUD model and mesh mask have independent restoration snapshot");
    CHECK(saved.quality==17&&saved.stat==321&&!strcmp(saved.names,"Nome original"),"Armory snapshot preserves the original quality, counter and name buffers");
    HazzureArmory::Profile armory{};armory.definition=16;armory.stickers=armory.charm=armory.stat=true;armory.charmId=1;
    CHECK(ArmoryFits(&saved,armory),"all five sticker slots, a charm and StatTrak fit a normal attribute list");
    auto crowded=saved;crowded.attributeCount=128;
    CHECK(!ArmoryFits(&crowded,armory),"capacity preflight rejects growth before touching models or engine vectors");
    SkinWrite(originalItem+0x1bc,9);SkinWrite(w+HazzureOffsets::Schema::m_nFallbackStatTrak,9999);
    char replacement[322]{};strcpy_s(replacement,161,"Alterado");SkinWrite(originalItem+0x2f8,replacement,sizeof(replacement));
    CHECK(ApplyArmory(&saved,HazzureArmory::Profile{})&&Get<int>(originalItem+0x1bc)==17&&Get<int>(w+HazzureOffsets::Schema::m_nFallbackStatTrak)==321&&
        !strcmp(reinterpret_cast<const char*>(originalItem+0x2f8),"Nome original"),"disabling Armory restores quality, counter and both original names without native calls");
    SkinWrite(SkinDynamicList(w)+8,129);SkinSaved invalid{};invalid.entity=123;
    CHECK(!SaveSkin(&invalid,w,0x10001)&&invalid.entity==123,"invalid attribute vector cannot fabricate or commit original state");
    SkinWrite(SkinDynamicList(w)+8,3);attributes[0]=1;attributes[0x48]=2;
    CHECK(SkinRestoreAttributes(SkinDynamicList(w),saved.attributeCount,saved.attributes)&&attributes[0]==0xAB&&Get<int>(SkinDynamicList(w)+8)==2,"attribute restoration uses validated engine-owned buffer");
    SkinWrite(SkinDynamicList(w)+8,1);
    CHECK(!SkinRestoreAttributes(SkinDynamicList(w),2,saved.attributes),"restoration refuses a smaller destination vector");
    SkinWrite(w+kSkinHudWeaponHandle,uint32_t(0));
    CHECK(SkinHudWeapon(w)==0&&zeroEntityCalls==0,"missing HUD handle cannot resolve world object");
    CHECK(!SkinFunctionValid(SKIN_SET_MODEL),"missing client module rejects native function calls");

    alignas(8) unsigned char pawn[0x4000]{},gloveAttributes[3*0x48]{};
    uintptr_t p=reinterpret_cast<uintptr_t>(pawn),item=p+kSkinGloves;
    entities[2]={0x30003,p,false};
    skinGloves.pawn=p;skinGloves.handle=0x30003;skinGloves.definition=5027;
    skinGloves.high=17;skinGloves.low=23;skinGloves.initialized=true;skinGloves.attributeCount=1;skinGloves.attributes[0]=0xE7;
    SkinWrite(item+kSkinItemDefinition,static_cast<unsigned short>(5030));
    SkinWrite(item+kSkinDynamicAttributes+8,3);SkinWrite(item+kSkinDynamicAttributes+16,reinterpret_cast<uintptr_t>(gloveAttributes));
    CHECK(RestoreGloves()&&Get<unsigned short>(item+kSkinItemDefinition)==5027&&Get<int>(item+kSkinItemHigh)==17&&Get<int>(item+kSkinItemLow)==23,"inline glove identity restored without an invented wearable entity");
    CHECK(gloveAttributes[0]==0xE7&&Get<int>(item+kSkinDynamicAttributes+8)==1&&Get<bool>(p+kSkinGlovesReapply)&&Get<int>(p+kSkinGlovesChanged)==1,"glove attributes restored and engine reapply notification raised");
    skinGloves.pawn=p;skinGloves.handle=0x40004;
    CHECK(RestoreGloves()&&!skinGloves.pawn,"destroyed glove pawn snapshot discarded without stale writes");

    using namespace HazzureArmoryVisual;
    alignas(8) unsigned char identity[0x40]{};
    SkinWrite(w+0x10,reinterpret_cast<uintptr_t>(identity));
    SkinWrite(reinterpret_cast<uintptr_t>(identity)+0x10,uint32_t(0x18001));
    SkinWrite(reinterpret_cast<uintptr_t>(identity)+0x30,uint32_t(1));
    CHECK(ArmoryVisualIdentity(w,0x10001)&&!ArmoryVisualIdentity(w,0x18001),
        "attachment identity compares the engine-normalized serial when identity flag one is set");
    SkinWrite(reinterpret_cast<uintptr_t>(identity)+0x10,uint32_t(0x20001));
    CHECK(!ArmoryVisualIdentity(w,0x10001),"a recycled identity cannot reuse an old attachment refresh token");
    CHECK(OwnershipAllowed(true,false,false,false)&&!OwnershipAllowed(false,false,false,false),
        "world attachment restoration remains permitted after drop or death but new application does not");
    CHECK(!OwnershipAllowed(true,true,false,false)&&!OwnershipAllowed(true,true,true,false)&&OwnershipAllowed(true,true,true,true),
        "restoration never targets a HUD rebound to a foreign or different held weapon");
    AttachmentFixture fixture;Calls calls{&fixture,FixtureAvailable,FixtureBinding,FixtureInvoke};
    Token token{100,200,300,0x10001,0x20002,2,5,false};State visual;
    CHECK(Refresh(visual,token,100,calls)==Status::Dispatched&&fixture.calls==6,
        "native modules are dispatched for the world weapon and its HUD model");
    bool correctItem=true;for(unsigned i=0;i<fixture.calls;++i)correctItem=correctItem&&fixture.items[i]==200;
    CHECK(correctItem&&fixture.targets[0]==100&&fixture.targets[3]==300,
        "HUD attachment refresh uses the owning weapon item instead of HUD weapon offsets");
    CHECK(fixture.modules[0]==Module::StatTrak&&fixture.modules[1]==Module::NameTag&&fixture.modules[2]==Module::Keychain,
        "attachment dispatcher preserves the measured native module order");
    Refresh(visual,token,1000,calls);CHECK(fixture.calls==6,"unchanged profiles do not rebuild attached entities every frame");
    ++token.armoryRevision;Refresh(visual,token,1001,calls);
    CHECK(fixture.calls==12,"a live StatTrak revision refreshes attachments without changing paint");
    token.restoring=true;Refresh(visual,token,1002,calls);
    CHECK(fixture.calls==18,"restoring original attributes invalidates the attachment refresh cache");
    fixture={};visual={};token.restoring=false;token.hud=0;token.hudHandle=0;
    CHECK(Refresh(visual,token,100,calls)==Status::WaitingHud&&visual.worldDone&&fixture.calls==3,
        "a missing HUD remains pending after the world modules are updated");
    token.hud=400;token.hudHandle=0x30003;Refresh(visual,token,101,calls);
    CHECK(visual.hudDone&&fixture.calls==9&&fixture.targets[6]==400,
        "late deployment or a replaced HUD receives its own attachment refresh");
    fixture={};visual={};fixture.available=false;
    CHECK(Refresh(visual,token,100,calls)==Status::Unavailable&&fixture.calls==0,
        "unsupported module builds cannot invoke any attachment callback");
    fixture.available=true;Refresh(visual,token,101,calls);
    CHECK(fixture.calls==0,"backend retries are bounded instead of repeating every render callback");
    Refresh(visual,token,300,calls);CHECK(fixture.calls==6,"a recovered backend can complete a pending refresh");
    fixture={};visual={};fixture.valid=false;
    CHECK(Refresh(visual,token,100,calls)==Status::Stale&&fixture.calls==0,
        "a stale handle or ownership rejection prevents all attachment calls");
    fixture={};visual={};fixture.invalidateAfterFirst=true;
    CHECK(Refresh(visual,token,100,calls)==Status::Stale&&fixture.calls==1&&fixture.checks==2,
        "identity is rechecked between modules if deployment changes during refresh");
    fixture={};visual={};fixture.failCall=2;
    CHECK(Refresh(visual,token,100,calls)==Status::Fault&&fixture.calls==2&&!visual.worldDone,
        "a native attachment failure remains pending and does not claim complete appearance");
    fixture.failCall=0;Refresh(visual,token,300,calls);
    CHECK(visual.status==Status::Dispatched&&fixture.calls==8,
        "a failed module sequence is retried after the bounded interval");
    CHECK(!ArmoryVisualAvailable(nullptr),"native attachment prologues require the reviewed loaded client image");
    SkinConfig maintained{16,309,777,0,.15f,1,1,0};
    SkinWrite(w+HazzureOffsets::Schema::m_iItemDefinitionIndex,static_cast<unsigned short>(16));
    SkinWrite(w+HazzureOffsets::Schema::m_iItemIDHigh,-1);SkinWrite(w+HazzureOffsets::Schema::m_iItemIDLow,-1);
    SkinWrite(w+HazzureOffsets::Schema::m_nFallbackPaintKit,309);SkinWrite(w+HazzureOffsets::Schema::m_nFallbackSeed,777);
    SkinWrite(w+HazzureOffsets::Schema::m_flFallbackWear,.15f);
    SkinWrite(SkinDynamicList(w)+8,3);
    for(int i=0;i<3;++i){SkinWrite(reinterpret_cast<uintptr_t>(attributes)+i*0x48+0x30,static_cast<uint16_t>(6+i));
        SkinWrite(reinterpret_cast<uintptr_t>(attributes)+i*0x48+0x34,i==0?309.f:i==1?777.f:.15f);}
    CHECK(SkinCurrentMatches(&saved,maintained,{}),"matching live skin skips unnecessary rebuilds");
    SkinWrite(w+HazzureOffsets::Schema::m_nFallbackPaintKit,0);
    CHECK(!SkinCurrentMatches(&saved,maintained,{}),"server fallback paint reset requires reapplication despite unchanged revision");
    SkinWrite(w+HazzureOffsets::Schema::m_nFallbackPaintKit,309);SkinWrite(reinterpret_cast<uintptr_t>(attributes)+0x34,0.f);
    CHECK(!SkinCurrentMatches(&saved,maintained,{}),"dynamic attribute overwrite invalidates a previously applied skin");
    SkinWrite(reinterpret_cast<uintptr_t>(attributes)+0x34,309.f);SkinWrite(SkinDynamicList(w)+8,0);
    CHECK(!SkinCurrentMatches(&saved,maintained,{}),"network replacement of the attribute list cannot pass the revision cache");
    uint8_t neighbours[3]={0xC3,0xA5,0x5A};SkinWrite(p+kSkinGlovesChanged,static_cast<uint8_t>(255));
    SkinWrite(p+kSkinGlovesChanged+1,neighbours,3);
    CHECK(NotifyGloves(p)&&Get<uint8_t>(p+kSkinGlovesChanged)==0,"glove change counter wraps as a single byte");
    CHECK(!memcmp(reinterpret_cast<void*>(p+kSkinGlovesChanged+1),neighbours,3),"glove notification preserves ragdoll sync and the next two bytes");
    CHECK(HazzureGloves::DefaultPaint(5034)==10033&&HazzureGloves::DefaultPaint(5031)==10013&&
          HazzureGloves::DefaultPaint(5035)==10057&&HazzureGloves::DefaultPaint(0)==0,"glove families have valid nonzero defaults instead of an unsupported zero kit");
    SkinWrite(w+0x10,reinterpret_cast<uintptr_t>(identity));SkinWrite(reinterpret_cast<uintptr_t>(identity)+0x10,uint32_t(0x18001));
    SkinWrite(reinterpret_cast<uintptr_t>(identity)+0x30,uint32_t(1));uint32_t normalized{};
    CHECK(SkinIdentity(w,normalized)&&normalized==0x10001,"skin and glove capture use the engine-normalized handle serial");
    using namespace HazzureGloveLifecycle;
    HazzureGloveLifecycle::Refresh lifecycle;Appearance appearance{};
    appearance.pawn=p;appearance.handle=0x30003;appearance.scene=s;appearance.model=ModelHash(AgentPathCT(1));appearance.team=3;appearance.spawn=100;
    CHECK(lifecycle.Observe(appearance)&&lifecycle.remaining==4,"first valid glove appearance schedules a bounded material refresh burst");
    CHECK(!lifecycle.Observe(appearance)&&lifecycle.Due(1000),"unchanged appearance does not restart the refresh burst");
    lifecycle.Acknowledge(1000);
    CHECK(lifecycle.remaining==3&&!lifecycle.Due(1000)&&lifecycle.Due(1001),"one callback cannot consume multiple refresh frames at the same timestamp");
    for(uint64_t time=1001;time<=1003;++time)lifecycle.Acknowledge(time);
    CHECK(!lifecycle.Due(2000)&&!lifecycle.Observe(appearance),"stable gloves stop issuing per-frame material updates after the burst");
    appearance.model=ModelHash(AgentPathCT(2));
    CHECK(lifecycle.Observe(appearance)&&lifecycle.remaining==4,"native agent model changes invalidate gloves without any configuration revision change");
    ++appearance.agentEpoch;CHECK(lifecycle.Observe(appearance),"our agentchanger epoch forces refresh even if the model path is unchanged");
    ++appearance.scene;CHECK(lifecycle.Observe(appearance),"scene replacement is detected on the same pawn and model");
    ++appearance.meshMask;CHECK(lifecycle.Observe(appearance),"a native bodygroup reset cannot pass the econ-item cache");
    ++appearance.spawn;CHECK(lifecycle.Observe(appearance),"same-pawn respawn identity invalidates first-person glove materials");
    appearance.hud=h;appearance.hudHandle=0x20002;appearance.hudScene=hs;
    CHECK(lifecycle.Observe(appearance),"late or replaced first-person weapon HUD invalidates glove presentation");
    auto stale=appearance;stale.handle=0xffffffffu;
    CHECK(!lifecycle.Observe(stale)&&!lifecycle.Due(2000),"invalid appearance suspends all refresh work");
    CHECK(lifecycle.Observe(appearance),"appearance recovery retries even when the recovered model matches the prior snapshot");
    CHECK(ModelHash(nullptr)==0&&ModelHash(AgentPathCT(1))!=ModelHash(AgentPathCT(2)),"model hashes distinguish actual model resources and reject missing names");
    alignas(8) unsigned char pawnIdentity[0x40]{};
    SkinWrite(p+0x10,reinterpret_cast<uintptr_t>(pawnIdentity));SkinWrite(reinterpret_cast<uintptr_t>(pawnIdentity)+0x10,uint32_t(0x30003));
    SkinWrite(p+HazzureOffsets::Schema::m_pGameSceneNode,s);SkinWrite(p+HazzureOffsets::Schema::m_iTeamNum,static_cast<uint8_t>(3));SkinWrite(p+0x14ec,3.5f);
    SkinWrite(s+HazzureOffsets::Schema::m_modelState+0xA8,reinterpret_cast<uintptr_t>(AgentPathCT(1)));
    Appearance observed{};
    CHECK(SkinGloveAppearance(p,observed)&&observed.pawn==p&&observed.handle==0x30003&&observed.model==ModelHash(AgentPathCT(1)),"live appearance reader uses exact pawn serial and current model resource");
    SkinWrite(s+HazzureOffsets::Schema::m_modelState+0xA8,reinterpret_cast<uintptr_t>(AgentPathCT(2)));
    Appearance replaced{};
    CHECK(SkinGloveAppearance(p,replaced)&&replaced.model!=observed.model&&replaced.pawn==observed.pawn,"in-place agent swaps are detected by the actual model even without agentchanger involvement");
    SkinWrite(p+0x14ec,NAN);
    CHECK(!SkinGloveAppearance(p,replaced),"unreadable or invalid spawn state cannot become a fabricated glove identity");
    client=0;ipc.agentEnabled=0;ipc.skinEnabled=1;skinOn=true;
    SkinFrameWork();
    CHECK(!skinOn&&!ipc.skinEnabled,"retired cosmetic frame ignores legacy activation");
    printf("RESULT: %u passed, %u failed; synthetic memory only.\n",passed,failed);
    return failed?1:0;
}
