// C++ / SkinEngine.h / MSVC x64 / local CS2 weapon finishes and knife models
#pragma once
#include "TacticalHUD.h"
#include "ArmoryState.h"
#include "ArmoryVisual.h"
#include "WorldSession.h"
#include "WorldWeather.h"
#include "CombatVisuals.h"
#include "GlovePresets.h"
#include "GloveLifecycle.h"
#include "MovementTech.h"
#include "AdvancedMovement.h"
#include "WeaponAccuracy.h"
#include "ShotBrake.h"
#include "SkinAddresses.h"
#include "GameData.h"
#include "AgentCatalog.h"
#include "Visibility.h"
#include "TsukuyomiOrb.h"
#include "GrenadePhysics.h"
#include "GrenadeZones.h"
#include "InfernoCells.h"
#include "SmokeDensity.h"
#include "VisualRecoil.h"
#include "BasicTuning.h"
#include "NativeInventoryRuntime.h"
using FrameNotify=void(*)(void*,int);
static FrameNotify originalFrame;
static std::atomic<bool> skinFailed{false};
static SkinConfig skinSettings[64]{};
static LONG skinRevision=-1;
static bool skinOn;
struct SkinSaved {
    uintptr_t entity;
    uint32_t handle;
    int revision,attributeCount;
    unsigned short definition;
    int high,low,paint,seed,stat,quality;
    char names[322];uint64_t armoryRevision;
    bool attachmentRefreshNeeded;
    HazzureArmoryVisual::State attachments;
    uint64_t nextRetry{},nextAudit{};
    float wear;
    uint64_t mask;
    char model[240];
    uintptr_t hudEntity;
    uint32_t hudHandle;
    uint64_t hudMask;
    char hudModel[240];
    unsigned char attributes[128*0x48];
};
static SkinSaved skinSaved[64]{};
static void SkinMessage(int status,const char* text) {
    shared->skinStatus=status;strncpy_s(shared->skinMessage,text,_TRUNCATE);
}
static bool Knife(int def) { return def==42||def==59||(def>=500&&def<=526); }
static const char* KnifePath(int def) {
    switch(def) {
    case 500:return "weapons/models/knife/knife_bayonet/weapon_knife_bayonet.vmdl";
    case 505:return "weapons/models/knife/knife_flip/weapon_knife_flip.vmdl";
    case 506:return "weapons/models/knife/knife_gut/weapon_knife_gut.vmdl";
    case 507:return "weapons/models/knife/knife_karambit/weapon_knife_karambit.vmdl";
    case 508:return "weapons/models/knife/knife_m9/weapon_knife_m9.vmdl";
    case 509:return "weapons/models/knife/knife_tactical/weapon_knife_tactical.vmdl";
    case 512:return "weapons/models/knife/knife_falchion/weapon_knife_falchion.vmdl";
    case 514:return "weapons/models/knife/knife_bowie/weapon_knife_bowie.vmdl";
    case 515:return "weapons/models/knife/knife_butterfly/weapon_knife_butterfly.vmdl";
    case 516:return "weapons/models/knife/knife_push/weapon_knife_push.vmdl";
    case 517:return "weapons/models/knife/knife_cord/weapon_knife_cord.vmdl";
    case 518:return "weapons/models/knife/knife_canis/weapon_knife_canis.vmdl";
    case 519:return "weapons/models/knife/knife_ursus/weapon_knife_ursus.vmdl";
    case 520:return "weapons/models/knife/knife_navaja/weapon_knife_navaja.vmdl";
    case 521:return "weapons/models/knife/knife_outdoor/weapon_knife_outdoor.vmdl";
    case 522:return "weapons/models/knife/knife_stiletto/weapon_knife_stiletto.vmdl";
    case 523:return "weapons/models/knife/knife_talon/weapon_knife_talon.vmdl";
    case 525:return "weapons/models/knife/knife_skeleton/weapon_knife_skeleton.vmdl";
    case 526:return "weapons/models/knife/knife_kukri/weapon_knife_kukri.vmdl";
    default:return nullptr;
    }
}
static bool Glove(int def) { return def==4725||def==5027||(def>=5030&&def<=5035); }
static const char* GlovePath(int def) {
    switch(def) {
    case 5027:return "agents/models/shared/arms/glove_bloodhound/glove_bloodhound.vmdl";
    case 5030:return "agents/models/shared/arms/glove_sporty/glove_sporty.vmdl";
    case 5031:return "agents/models/shared/arms/glove_slick/glove_slick.vmdl";
    case 5032:return "agents/models/shared/arms/glove_handwrap/glove_handwrap.vmdl";
    case 5033:return "agents/models/shared/arms/glove_motorcycle/glove_motorcycle.vmdl";
    case 5034:return "agents/models/shared/arms/glove_specialist/glove_specialist.vmdl";
    case 5035:return "agents/models/shared/arms/glove_hydra/glove_hydra.vmdl";
    case 4725:return "agents/models/shared/arms/glove_brokenfang/glove_brokenfang.vmdl";
    default:return nullptr;
    }
}
static const char* AgentPathCT(int idx) {
    return idx>0&&idx<static_cast<int>(sizeof(kAgentResourcesCT)/sizeof(kAgentResourcesCT[0]))?kAgentResourcesCT[idx].path:nullptr;
}
static const char* AgentPathT(int idx) {
    return idx>0&&idx<static_cast<int>(sizeof(kAgentResourcesT)/sizeof(kAgentResourcesT[0]))?kAgentResourcesT[idx].path:nullptr;
}
static uintptr_t g_savedAgentEntity = 0;
static char g_savedAgentModel[240]{};
static uint32_t g_savedAgentHandle;
static uint64_t g_savedAgentMask;
static uint64_t skinPawnModelEpoch;
static bool agentMaskPending;
// [AGENT] so escreve em transicao de flag/alvo/pawn e em transicao de resultado do
// restore; por frame o log viria ilegivel e era exatamente por isso que nao tinhamos
// como dizer se o agente desligado devolveu o modelo ou nao.
static bool g_agentLogEnabled;
static const char* g_agentLogTarget;
static uintptr_t g_agentLogPawn;
static uintptr_t g_agentRestoreLogPawn;
static int g_agentRestoreLogState;
static constexpr uintptr_t kSkinItemDefinition=0x1BA;
static constexpr uintptr_t kSkinItemHigh=0x1D0,kSkinItemLow=0x1D4,kSkinItemInitialized=0x1E8;
static constexpr uintptr_t kSkinGloves=0x1770,kSkinGlovesReapply=0x176D,kSkinGlovesChanged=0x1D20;
static constexpr uintptr_t kSkinGlovesDisallowSOC=0x1E9; // m_bDisallowSOC, no mesmo item view
static constexpr uintptr_t kSkinBodyGroupChoiceWords=0xF40; // lido por UpdateBodyGroupChoice
static constexpr uintptr_t kSkinHudWeaponHandle=0x1990; // Validated live as C_CS2HudModelWeapon.
// Installed PE (0x6AEBC4DA): sub_181143E00 hands the engine's attribute setter the
// entity's CEconItemView, which sits at m_AttributeManager(0x1290)+m_Item(0x50) =
// +0x12E0 ("a1 + 604" where a1 is a __int64*). SetFloatAttribute then reaches the
// CAttributeList at view+520 (0x208) -> entity+0x14E8, matching the schema dump's
// C_EconItemView::m_AttributeList = 0x208. The list is {count at +8, data at +16,
// 72-byte entries, u16 attribute id at +0x30, value at +0x34}. The previous 0x12C0
// base read 0x20 before the view, and 0x280 was m_NetworkedDynamicAttributes, so
// every setter call and verification read the wrong memory.
static constexpr uintptr_t kSkinItemView=0x12E0,kSkinAttrListInItemView=0x208;
static uintptr_t SkinWeaponView(uintptr_t weapon) { return weapon+kSkinItemView; }
static uintptr_t SkinAttrList(uintptr_t item) { return item+kSkinAttrListInItemView; }
static uintptr_t SkinEntity(uint32_t handle) {
    return handle&&handle!=0xFFFFFFFFu?Entity(handle):0;
}
static bool SkinIdentity(uintptr_t entity,uint32_t& handle) {
    uintptr_t identity{};uint32_t raw{},flags{};
    return Read(reinterpret_cast<void*>(entity+0x10),&identity,sizeof(identity))&&identity&&
        Read(reinterpret_cast<void*>(identity+0x10),&raw,sizeof(raw))&&
        Read(reinterpret_cast<void*>(identity+0x30),&flags,sizeof(flags))&&
        (handle=HazzureGame::CanonicalHandle(raw,flags))!=0xffffffffu&&SkinEntity(handle)==entity;
}
static bool SkinWrite(uintptr_t address,const void* value,size_t length) {
    if(address<0x10000||!value||!length)return false;
    __try {memcpy(reinterpret_cast<void*>(address),value,length);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T> static bool SkinWrite(uintptr_t address,const T& value) {return SkinWrite(address,&value,sizeof(value));}
static bool SkinFunctionValid(const CheckedAddress& target) {
    unsigned char bytes[16]{};
    return client&&Read(reinterpret_cast<void*>(client+target.rva),bytes,sizeof(bytes))&&memcmp(bytes,target.bytes,sizeof(bytes))==0;
}
static bool SkinModelState(uintptr_t entity,char* model,size_t size,uint64_t& mask) {
    uintptr_t scene{},name{};
    if(!entity||!model||size<2||!Read(reinterpret_cast<void*>(entity+HazzureOffsets::Schema::m_pGameSceneNode),&scene,sizeof(scene))||!scene||
       !Read(reinterpret_cast<void*>(scene+HazzureOffsets::Schema::m_modelState+0xA8),&name,sizeof(name))||!name||
       !Read(reinterpret_cast<void*>(scene+HazzureOffsets::Schema::m_modelState+0x208),&mask,sizeof(mask)))return false;
    for(size_t i=0;i<size;i++) {
        char value{};if(!Read(reinterpret_cast<void*>(name+i),&value,1))return false;
        model[i]=value;
        if(!value)return i>5&&strstr(model,".vmdl")!=nullptr;
        if(static_cast<unsigned char>(value)<32)return false;
    }
    model[size-1]=0;return false;
}
static bool SkinSetSceneMask(uintptr_t entity,uint64_t mask) {
    uintptr_t scene=Get<uintptr_t>(entity+HazzureOffsets::Schema::m_pGameSceneNode);
    if(!scene||!SkinFunctionValid(SKIN_MESH_MASK))return false;
    reinterpret_cast<void(*)(void*,uint64_t)>(client+SKIN_MESH_MASK.rva)(reinterpret_cast<void*>(scene),mask);return true;
}
static uintptr_t SkinHudWeapon(uintptr_t weapon,uint32_t* outHandle=nullptr) {
    uint32_t handle{};char name[96]{};
    if(!Read(reinterpret_cast<void*>(weapon+kSkinHudWeaponHandle),&handle,sizeof(handle)))return 0;
    uintptr_t hud=SkinEntity(handle);
    if(!hud||!ClassName(hud,name,sizeof(name))||strcmp(name,".?AVC_CS2HudModelWeapon@@"))return 0;
    if(outHandle)*outHandle=handle;
    return hud;
}
// m_hHudModelArms is the first-person hands entity in the installed schema. The glove
// model is drawn by those arms, so the entity — not only the pawn's econ view — decides
// what the local player sees. The class name check keeps a stale handle out.
static constexpr uintptr_t kSkinHudArmsHandle=0x1DA8;
static uintptr_t SkinHudArms(uintptr_t pawn,uint32_t* outHandle=nullptr) {
    uint32_t handle{};char name[96]{};
    if(!Read(reinterpret_cast<void*>(pawn+kSkinHudArmsHandle),&handle,sizeof(handle)))return 0;
    uintptr_t arms=SkinEntity(handle);
    if(!arms||!ClassName(arms,name,sizeof(name))||strcmp(name,".?AVC_CS2HudModelArms@@"))return 0;
    if(outHandle)*outHandle=handle;
    return arms;
}
static bool SkinSetVisualModel(uintptr_t entity,const char* path) {
    if(!entity||!path||!path[0]||!SkinFunctionValid(SKIN_SET_MODEL))return false;
    if(!PrecacheModelResource(path))return false;
    reinterpret_cast<void(*)(void*,const char*)>(client+SKIN_SET_MODEL.rva)(reinterpret_cast<void*>(entity),path);return true;
}
static bool SkinSameModel(const char* first,const char* second) {
    // characters/ contains dummy tool models; it is not an alias for agents/.
    if(!first||!second)return false;
    return strcmp(first,second)==0;
}
static bool RestoreAgent() {
    if(!g_savedAgentEntity)return true;
    if(SkinEntity(g_savedAgentHandle)!=g_savedAgentEntity){
        if(g_agentRestoreLogPawn!=g_savedAgentEntity||g_agentRestoreLogState!=1){
            g_agentRestoreLogPawn=g_savedAgentEntity;g_agentRestoreLogState=1;
            LogDebug("[AGENT] restore no-op: pawn %p nao resolve",reinterpret_cast<void*>(g_savedAgentEntity));
        }
        g_savedAgentEntity=0;g_savedAgentHandle=0;g_savedAgentModel[0]=0;g_savedAgentMask=0;return true;
    }
    bool ok=SkinSetVisualModel(g_savedAgentEntity,g_savedAgentModel)&&SkinSetSceneMask(g_savedAgentEntity,g_savedAgentMask);
    int state=ok?2:3;
    if(g_agentRestoreLogPawn!=g_savedAgentEntity||g_agentRestoreLogState!=state){
        g_agentRestoreLogPawn=g_savedAgentEntity;g_agentRestoreLogState=state;
        LogDebug("[AGENT] restore ok=%d pawn=%p model=%s mask=0x%llX",ok?1:0,reinterpret_cast<void*>(g_savedAgentEntity),g_savedAgentModel,(unsigned long long)g_savedAgentMask);
    }
    if(!ok)return false;
    g_savedAgentEntity=0;g_savedAgentModel[0]=0;agentMaskPending=false;++skinPawnModelEpoch;return true;
}
static void ApplyAgentChanger(uintptr_t local) {
    if(!shared)return;
    if(g_savedAgentEntity&&(g_savedAgentEntity!=local||SkinEntity(g_savedAgentHandle)!=g_savedAgentEntity)&&!RestoreAgent())return;
    HazzureGame::Pawn pawn{};
    if(!local||!HazzureGame::ReadPawn(local,pawn)) {RestoreAgent();return;}
    bool enabled=InterlockedCompareExchange(&shared->agentEnabled,0,0)!=0;
    unsigned char team=Get<unsigned char>(local+HazzureOffsets::Schema::m_iTeamNum);
    int idx=(team==3)?InterlockedCompareExchange(&shared->agentCT,0,0):((team==2)?InterlockedCompareExchange(&shared->agentT,0,0):0);
    const char* targetPath=enabled?((team==3)?AgentPathCT(idx):((team==2)?AgentPathT(idx):nullptr)):nullptr;
    if(enabled!=g_agentLogEnabled||targetPath!=g_agentLogTarget||local!=g_agentLogPawn) {
        g_agentLogEnabled=enabled;g_agentLogTarget=targetPath;g_agentLogPawn=local;
        LogDebug("[AGENT] enabled=%d team=%u idx=%d target=%s pawn=%p",enabled?1:0,static_cast<unsigned>(team),idx,targetPath?targetPath:"-",reinterpret_cast<void*>(local));
    }
    if(!targetPath) {
        RestoreAgent();return;
    }
    char currentModel[240]{};uint64_t currentMask{};uint32_t handle{};
    if(!SkinIdentity(local,handle)||!SkinModelState(local,currentModel,sizeof(currentModel),currentMask))return;
    if(g_savedAgentEntity!=local) {
        g_savedAgentEntity=local;g_savedAgentHandle=handle;g_savedAgentMask=currentMask;
        strncpy_s(g_savedAgentModel,currentModel,_TRUNCATE);
    }
    const auto& resource=team==3?kAgentResourcesCT[idx]:kAgentResourcesT[idx];
    bool same=SkinSameModel(currentModel,targetPath);
    if(!same){if(!SkinSetVisualModel(local,targetPath))return;agentMaskPending=true;}
    // Native glove/bodygroup updates legitimately change the mask. Reasserting
    // the agent's default every frame would undo those updates indefinitely.
    if(agentMaskPending&&SkinSetSceneMask(local,resource.meshMask)){agentMaskPending=false;++skinPawnModelEpoch;}
}
static uint32_t SubclassToken(int definition) {
    char text[16];sprintf_s(text,"%d",definition);
    const auto* data=reinterpret_cast<const unsigned char*>(text);
    unsigned len=static_cast<unsigned>(strlen(text)),remaining=len;
    uint32_t hash=0x31415926u^len;
    while(remaining>=4) { uint32_t k;memcpy(&k,data,4);k*=0x5bd1e995;k^=k>>24;k*=0x5bd1e995;hash=(hash*0x5bd1e995)^k;data+=4;remaining-=4; }
    for(unsigned i=0;i<remaining;i++)hash^=uint32_t(data[i])<<(8*i);
    if(remaining)hash*=0x5bd1e995;
    hash^=hash>>13;hash*=0x5bd1e995;return hash^(hash>>15);
}
static bool SkinAddressesValid() {
    const CheckedAddress* entries[]={&SKIN_FRAME,&SKIN_SET_ATTR,&SKIN_UPDATE_SKIN,&SKIN_CLEAR_MATERIAL,&SKIN_SET_MODEL,&SKIN_SUBCLASS,&SKIN_MESH_MASK};
    for(auto entry:entries)if(memcmp(reinterpret_cast<void*>(client+entry->rva),entry->bytes,16)!=0)return false;
    return Get<uintptr_t>(client+0x1C67320)==client+SKIN_FRAME.rva;
}
static bool SnapshotSettings() {
    if(!shared)return false;
    LONG before=InterlockedCompareExchange(&shared->revision,0,0);
    if(before&1)return false;
    if(before==skinRevision){skinOn=InterlockedCompareExchange(&shared->skinEnabled,0,0)!=0;return true;}
    SkinConfig snapshot[64];memcpy(snapshot,shared->skins,sizeof(snapshot));
    bool enabled=shared->skinEnabled!=0;
    MemoryBarrier();
    if(before!=InterlockedCompareExchange(&shared->revision,0,0))return false;
    for(auto& c:snapshot)if(c.enabled&&(c.definition<1||(c.definition>526&&!Glove(c.definition))||c.paint<0||c.paint>20000||c.seed<0||c.seed>1000||!(c.wear>=0&&c.wear<=1)||(c.model&&!KnifePath(c.model)&&!GlovePath(c.model))||(c.model&&KnifePath(c.model)&&!Knife(c.definition))||(c.model&&GlovePath(c.model)&&!Glove(c.definition))||(c.legacy!=0&&c.legacy!=1))) {
        SkinMessage(-2,"Invalid skin configuration");return false;
    }
    memcpy(skinSettings,snapshot,sizeof(snapshot));skinOn=enabled;skinRevision=before;
    InterlockedExchange(&shared->appliedRevision,before);return true;
}
static bool SetSkinModel(uintptr_t weapon,int definition,const char* path) {
    if(!weapon||!path||!path[0]||!SkinFunctionValid(SKIN_SET_MODEL)||!SkinFunctionValid(SKIN_SUBCLASS)||!PrecacheModelResource(path))return false;
    if(!SkinWrite(weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex,static_cast<unsigned short>(definition))||
       !SkinWrite(weapon+HazzureOffsets::Schema::m_nSubclassID,SubclassToken(definition)))return false;
    reinterpret_cast<void(*)(void*,const char*)>(client+SKIN_SET_MODEL.rva)(reinterpret_cast<void*>(weapon),path);
    reinterpret_cast<void(*)(void*)>(client+SKIN_SUBCLASS.rva)(reinterpret_cast<void*>(weapon));return true;
}
static void SkinMask(uintptr_t weapon,uint64_t mask) {
    SkinSetSceneMask(weapon,mask);
    auto hud=SkinHudWeapon(weapon);if(hud)SkinSetSceneMask(hud,mask);
}
static bool RebuildSkin(uintptr_t weapon) {
    if(!weapon||!SkinFunctionValid(SKIN_UPDATE_SKIN))return false;

    reinterpret_cast<void(*)(void*,bool)>(client+SKIN_UPDATE_SKIN.rva)(reinterpret_cast<void*>(weapon),true);return true;
}
static bool SkinAttributeView(uintptr_t list,int& count,uintptr_t& attributes) {
    return Read(reinterpret_cast<void*>(list+8),&count,sizeof(count))&&count>=0&&count<=128&&
        Read(reinterpret_cast<void*>(list+16),&attributes,sizeof(attributes))&&(!count||attributes>=0x10000);
}
static bool SkinRestoreAttributes(uintptr_t list,int count,const unsigned char* attributes) {
    int currentCount{};uintptr_t currentAttributes{};
    if(count<0||count>128||!SkinAttributeView(list,currentCount,currentAttributes)||currentCount<count)return false;
    if(count&&!SkinWrite(currentAttributes,attributes,static_cast<size_t>(count)*0x48))return false;
    return SkinWrite(list+8,count);
}
// The engine keys the fallback attributes by schema name: sub_1811A5450 resolves
// sub_1811330F0(system+8, name) and reads the attribute id from the definition's
// vtable[0] ("mov rax,[def]; call [rax]" == (**def)(def); 0xFFFF invalid). Reading
// only *(definition) yields the vtable itself, so the getter must dereference twice:
// vtable = *(definition), then getter = *(vtable).
static bool SkinAttributeId(const char* name,uint16_t& id,int* fail=nullptr) {
    uintptr_t system{},inner{},definition{},vtable{},getter{};
    auto stop=[&](int code){id=0xffffu;if(fail)*fail=code;return false;};
    if(!client||!name)return stop(1);
    if(!Read(reinterpret_cast<void*>(client+0x25F9470),&system,sizeof(system))||system<0x10000)return stop(2);
    if(!Read(reinterpret_cast<void*>(system+8),&inner,sizeof(inner))||inner<0x10000)return stop(3);
    __try{definition=reinterpret_cast<uintptr_t(*)(uintptr_t,const char*)>(client+0x11330F0)(inner,name);}
    __except(EXCEPTION_EXECUTE_HANDLER){return stop(4);}
    if(definition<0x10000)return stop(5);
    if(!Read(reinterpret_cast<void*>(definition),&vtable,sizeof(vtable))||vtable<client||vtable>=client+0x29A3000)return stop(6);
    if(!Read(reinterpret_cast<void*>(vtable),&getter,sizeof(getter))||getter<client||getter>=client+0x29A3000)return stop(7);
    unsigned raw{};
    __try{raw=reinterpret_cast<unsigned(__fastcall*)(void*)>(getter)(reinterpret_cast<void*>(definition));}
    __except(EXCEPTION_EXECUTE_HANDLER){return stop(8);}
    id=static_cast<uint16_t>(raw);return id!=0xffffu;
}
// A wrong view base would let the setter grow a vector that is not an attribute list.
// The header must be a sane {count, data} pair; an empty list (count 0) is a valid
// state the engine's setter appends to, so only validate an entry when one exists.
static bool SkinListSane(uintptr_t list) {
    int count{};uintptr_t values{};
    if(!SkinAttributeView(list,count,values))return false;
    if(count<1)return true;
    uint16_t id{};float value{};
    return Read(reinterpret_cast<void*>(values+0x30),&id,sizeof(id))&&id<4096&&
        Read(reinterpret_cast<void*>(values+0x34),&value,4)&&std::isfinite(value);
}
static bool SkinListHas(uintptr_t list,uint16_t id,float value) {
    int count{};uintptr_t values{};
    if(!SkinAttributeView(list,count,values))return false;
    for(int i=0;i<count;++i){auto at=values+static_cast<size_t>(i)*0x48;uint16_t actual{};float stored{};
        if(!Read(reinterpret_cast<void*>(at+0x30),&actual,sizeof(actual))||
           !Read(reinterpret_cast<void*>(at+0x34),&stored,4))return false;
        if(actual==id&&stored==value)return true;}
    return false;
}
static bool SkinPaintMatches(uintptr_t item,const SkinConfig& config) {
    uint16_t kit{},seed{},wear{};
    if(!item||!SkinAttributeId("set item texture prefab",kit)||!SkinAttributeId("set item texture seed",seed)||
       !SkinAttributeId("set item texture wear",wear))return false;
    const auto list=SkinAttrList(item);
    if(config.paint>=1&&!SkinListHas(list,kit,static_cast<float>(config.paint)))return false;
    if(config.seed>=1&&!SkinListHas(list,seed,static_cast<float>(config.seed)))return false;
    if(config.wear>0.f&&!SkinListHas(list,wear,config.wear))return false;
    return true;
}
// Pinha a cadeia que falhou em "skin default": base da view, lista real de atributos,
// ids resolvidos pelo motor (com código de etapa de falha) e a leitura final de
// kit/seed/wear. Throttle por tag (2 s) para a rota de luvas não esfomear a da arma.
static bool SkinChainDue(const char* tag,ULONGLONG now) {
    struct Gate { const char* tag; ULONGLONG next; };
    static Gate gates[8]{};
    for(auto& g:gates) {
        if(g.tag==tag) { if(now<g.next)return false; g.next=now+2000; return true; }
        if(!g.tag) { g.tag=tag; g.next=now+2000; return true; }
    }
    return true;
}
static void SkinChain(const char* tag,uintptr_t item,const SkinConfig& config,bool ok) {
    if(!item)return;
    auto now=GetTickCount64();
    if(!SkinChainDue(tag,now))return;
    uint16_t kit{},seed{},wear{};int fk{},fs{},fw{};int count{};uintptr_t values{};
    bool resolved=SkinAttributeId("set item texture prefab",kit,&fk)&&SkinAttributeId("set item texture seed",seed,&fs)&&
        SkinAttributeId("set item texture wear",wear,&fw);
    if(!SkinAttributeView(SkinAttrList(item),count,values))values=0;
    LogDebug("[SKINCHAIN] %s item=0x%llX data=0x%llX count=%d ids=%d/%d/%d fails=%d/%d/%d kit=%d seed=%d wear=%.3f stored=%d/%d/%d ok=%d",
        tag,static_cast<unsigned long long>(item),static_cast<unsigned long long>(values),count,
        resolved?kit:-1,resolved?seed:-1,resolved?wear:-1,fk,fs,fw,config.paint,config.seed,config.wear,
        SkinListHas(SkinAttrList(item),kit,static_cast<float>(config.paint))?1:0,
        SkinListHas(SkinAttrList(item),seed,static_cast<float>(config.seed))?1:0,
        SkinListHas(SkinAttrList(item),wear,config.wear)?1:0,ok?1:0);
}
// sub_1811A5820(view,name,float) is CEconItemView::SetFloatAttribute: it resolves the
// schema definition by name, writes the float into the CAttributeList at view+0x208
// (allocating through the engine when the attribute is absent) and recomputes the view.
// The previous call passed the list instead of the view, so the engine added 0x208 a
// second time and every write landed outside the item. Paint is only staged for the
// values the engine itself accepts (kit/seed >= 1, wear > 0) and each write is read
// back from the list by its resolved attribute id.
static bool SkinSetAttributes(uintptr_t item,const SkinConfig& config) {
    if(!item||!SkinFunctionValid(SKIN_SET_ATTR)||!SkinListSane(SkinAttrList(item)))return false;
    auto setter=reinterpret_cast<void(__fastcall*)(void*,const char*,float)>(client+SKIN_SET_ATTR.rva);
    __try{
        if(config.paint>=1)setter(reinterpret_cast<void*>(item),"set item texture prefab",static_cast<float>(config.paint));
        if(config.seed>=1)setter(reinterpret_cast<void*>(item),"set item texture seed",static_cast<float>(config.seed));
        if(config.wear>0.f)setter(reinterpret_cast<void*>(item),"set item texture wear",config.wear);
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    return SkinPaintMatches(item,config);
}
#include "ArmoryBackend.h"
static bool SkinCurrentMatches(const SkinSaved* saved,const SkinConfig& config,const HazzureArmory::Profile& profile){
    auto weapon=saved->entity;
    if(!weapon||SkinEntity(saved->handle)!=weapon||
       Get<unsigned short>(weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex)!=(config.model?config.model:saved->definition)||
       Get<int>(weapon+HazzureOffsets::Schema::m_iItemIDHigh)!=-1||Get<int>(weapon+HazzureOffsets::Schema::m_iItemIDLow)!=-1||
       Get<int>(weapon+HazzureOffsets::Schema::m_nFallbackPaintKit)!=config.paint||
       Get<int>(weapon+HazzureOffsets::Schema::m_nFallbackSeed)!=config.seed||
       Get<float>(weapon+HazzureOffsets::Schema::m_flFallbackWear)!=config.wear||!SkinPaintMatches(SkinWeaponView(weapon),config))return false;
    auto plan=HazzureArmory::Attributes(profile);
    for(size_t i=0;i<plan.count;++i){auto at=ArmoryAttribute(SkinAttrList(SkinWeaponView(weapon)),plan.entries[i].id);
        if(!at||Get<uint32_t>(at+0x34)!=plan.entries[i].bits)return false;}
    if(profile.stat&&Get<int>(weapon+HazzureOffsets::Schema::m_nFallbackStatTrak)!=profile.counter)return false;
    return true;
}
// The attachment routines take a generic model entity and an explicit item.
// The HUD model is NOT an econ weapon: never call SKIN_UPDATE_SKIN on it or
// infer an item/container at the weapon's offsets in the HUD object.
static bool ArmoryVisualAvailable(void*) {
    static constexpr CheckedAddress stat={0x8035A0,{0x48,0x89,0x5c,0x24,0x10,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57}};
    static constexpr CheckedAddress name={0x802B00,{0x40,0x55,0x53,0x56,0x48,0x8d,0xac,0x24,0x90,0xfc,0xff,0xff,0x48,0x81,0xec,0x70}};
    static constexpr CheckedAddress charm={0x801A40,{0x44,0x88,0x44,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x48,0x89,0x4c,0x24,0x08,0x55}};
    return HazzureVisibility::BuildMatches(client)&&SkinFunctionValid(stat)&&SkinFunctionValid(name)&&SkinFunctionValid(charm);
}
static bool ArmoryVisualIdentity(uintptr_t entity,uint32_t expected) {
    uintptr_t identity{};uint32_t raw{},flags{};
    return entity&&expected&&expected!=0xffffffffu&&SkinEntity(expected)==entity&&
        Read(reinterpret_cast<void*>(entity+0x10),&identity,sizeof(identity))&&identity&&
        Read(reinterpret_cast<void*>(identity+0x10),&raw,sizeof(raw))&&
        Read(reinterpret_cast<void*>(identity+0x30),&flags,sizeof(flags))&&
        HazzureArmoryVisual::CanonicalHandle(raw,flags)==expected;
}
static bool ArmoryVisualBinding(void*,const HazzureArmoryVisual::Token& token,uintptr_t target) {
    if(!ArmoryVisualAvailable(nullptr)||token.item!=ArmoryItem(token.weapon)||
       !ArmoryVisualIdentity(token.weapon,token.weaponHandle))return false;
    auto local=HazzureGame::LocalPawn(client);HazzureGame::Pawn state{};
    bool localValid=local&&HazzureGame::ReadPawn(local,state,!token.restoring);
    bool owner=localValid&&SkinEntity(Get<uint32_t>(token.weapon+HazzureOffsets::Schema::m_hOwnerEntity))==local;
    auto services=localValid?Get<uintptr_t>(local+HazzureOffsets::Schema::m_pWeaponServices):0;
    bool held=services&&Get<uint32_t>(services+HazzureOffsets::Schema::m_hActiveWeapon)==token.weaponHandle;
    if(!HazzureArmoryVisual::OwnershipAllowed(token.restoring,target!=token.weapon,owner,held))return false;
    // The provider getter and HUD vtable do not re-anchor in this build; the
    // identity/ownership checks above already gate every attachment call.
    auto table=Get<uintptr_t>(token.item),nameGetter=table?Get<uintptr_t>(table+0xD0):0;
    if(table<client||table>=client+0x29A3000||nameGetter<client||nameGetter>=client+0x29A3000)return false;
    if(target==token.weapon)return true;
    uint32_t bound{};
    if(!token.hud||target!=token.hud||SkinHudWeapon(token.weapon,&bound)!=token.hud||bound!=token.hudHandle||
       !ArmoryVisualIdentity(token.hud,token.hudHandle))return false;
    char model[240]{};uint64_t mask{};
    return SkinModelState(token.hud,model,sizeof(model),mask);
}
static bool ArmoryVisualInvoke(void*,uintptr_t target,uintptr_t item,HazzureArmoryVisual::Module module) {
    __try {
        if(module==HazzureArmoryVisual::Module::StatTrak)
            reinterpret_cast<void(*)(void*,void*)>(client+0x8035A0)(reinterpret_cast<void*>(target),reinterpret_cast<void*>(item));
        else if(module==HazzureArmoryVisual::Module::NameTag)
            reinterpret_cast<void(*)(void*,void*)>(client+0x802B00)(reinterpret_cast<void*>(target),reinterpret_cast<void*>(item));
        else reinterpret_cast<void(*)(void*,void*,bool)>(client+0x801A40)(reinterpret_cast<void*>(target),reinterpret_cast<void*>(item),false);
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static HazzureArmoryVisual::Status RefreshArmoryVisual(SkinSaved* saved,bool restoring=false) {
    uint32_t hudHandle{};auto hud=SkinHudWeapon(saved->entity,&hudHandle);
    if(restoring){auto local=HazzureGame::LocalPawn(client);
        auto services=local?Get<uintptr_t>(local+HazzureOffsets::Schema::m_pWeaponServices):0;
        bool localBinding=local&&SkinEntity(Get<uint32_t>(saved->entity+HazzureOffsets::Schema::m_hOwnerEntity))==local&&
            services&&Get<uint32_t>(services+HazzureOffsets::Schema::m_hActiveWeapon)==saved->handle;
        if(!localBinding){hud=0;hudHandle=0;}
    }
    HazzureArmoryVisual::Token token{saved->entity,ArmoryItem(saved->entity),hud,saved->handle,hudHandle,
        skinRevision,saved->armoryRevision,restoring};
    return HazzureArmoryVisual::Refresh(saved->attachments,token,GetTickCount64(),
        {nullptr,ArmoryVisualAvailable,ArmoryVisualBinding,ArmoryVisualInvoke});
}
static bool SkinRefreshHud(SkinSaved* saved) {
    uint32_t handle{};uintptr_t hud=SkinHudWeapon(saved->entity,&handle);
    if(hud==saved->hudEntity&&handle==saved->hudHandle)return true;
    if(saved->hudEntity&&SkinEntity(saved->hudHandle)==saved->hudEntity) {
        if(!SkinSetVisualModel(saved->hudEntity,saved->hudModel)||!SkinSetSceneMask(saved->hudEntity,saved->hudMask))return false;
    }
    char model[240]{};uint64_t mask{};
    if(hud&&!SkinModelState(hud,model,sizeof(model),mask))return false;
    saved->hudEntity=hud;saved->hudHandle=handle;saved->hudMask=mask;
    strncpy_s(saved->hudModel,model,_TRUNCATE);return true;
}
static bool SaveSkin(SkinSaved* saved,uintptr_t weapon,uint32_t handle) {
    if(!saved||!weapon||SkinEntity(handle)!=weapon)return false;
    SkinSaved candidate{};uintptr_t attributes{};
    if(!SkinAttributeView(SkinAttrList(SkinWeaponView(weapon)),candidate.attributeCount,attributes)||
       (candidate.attributeCount&&!Read(reinterpret_cast<void*>(attributes),candidate.attributes,static_cast<size_t>(candidate.attributeCount)*0x48)))return false;
    using namespace HazzureOffsets::Schema;
    if(!Read(reinterpret_cast<void*>(weapon+m_iItemDefinitionIndex),&candidate.definition,sizeof(candidate.definition))||
       !Read(reinterpret_cast<void*>(weapon+m_iItemIDHigh),&candidate.high,sizeof(candidate.high))||
       !Read(reinterpret_cast<void*>(weapon+m_iItemIDLow),&candidate.low,sizeof(candidate.low))||
       !Read(reinterpret_cast<void*>(weapon+m_nFallbackPaintKit),&candidate.paint,sizeof(candidate.paint))||
       !Read(reinterpret_cast<void*>(weapon+m_nFallbackSeed),&candidate.seed,sizeof(candidate.seed))||
       !Read(reinterpret_cast<void*>(weapon+m_flFallbackWear),&candidate.wear,sizeof(candidate.wear))||
       !Read(reinterpret_cast<void*>(weapon+m_nFallbackStatTrak),&candidate.stat,sizeof(candidate.stat))||
       !SkinModelState(weapon,candidate.model,sizeof(candidate.model),candidate.mask))return false;
    const auto item=weapon+HazzureOffsets::Schema::m_AttributeManager+HazzureOffsets::Schema::m_Item;
    if(!Read(reinterpret_cast<void*>(item+0x1bc),&candidate.quality,sizeof(candidate.quality))||
       !Read(reinterpret_cast<void*>(item+0x2f8),candidate.names,sizeof(candidate.names)))return false;
    if(!std::isfinite(candidate.wear)||candidate.definition<1||candidate.definition>526)return false;
    candidate.entity=weapon;candidate.handle=handle;candidate.revision=-1;
    if(!SkinRefreshHud(&candidate))return false;
    *saved=candidate;return true;
}
static bool RestoreSkin(SkinSaved* saved) {
    auto weapon=saved->entity;
    if(!weapon||SkinEntity(saved->handle)!=weapon) { saved->entity=0;return true; }
    if(!SkinFunctionValid(SKIN_CLEAR_MATERIAL)||!SkinFunctionValid(SKIN_UPDATE_SKIN)||!SkinFunctionValid(SKIN_MESH_MASK))return false;
    if(!SkinRestoreAttributes(SkinAttrList(SkinWeaponView(weapon)),saved->attributeCount,saved->attributes))return false;
    const auto item=ArmoryItem(weapon);
    if(!SkinWrite(item+0x1bc,saved->quality)||!SkinWrite(item+0x2f8,saved->names,sizeof(saved->names)))return false;
    if(!SkinWrite(weapon+HazzureOffsets::Schema::m_iItemIDHigh,saved->high)||!SkinWrite(weapon+HazzureOffsets::Schema::m_iItemIDLow,saved->low)||
       !SkinWrite(weapon+HazzureOffsets::Schema::m_nFallbackPaintKit,saved->paint)||!SkinWrite(weapon+HazzureOffsets::Schema::m_nFallbackSeed,saved->seed)||
       !SkinWrite(weapon+HazzureOffsets::Schema::m_flFallbackWear,saved->wear)||!SkinWrite(weapon+HazzureOffsets::Schema::m_nFallbackStatTrak,saved->stat))return false;
    if(Get<unsigned short>(weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex)!=saved->definition) {
        if(!SetSkinModel(weapon,saved->definition,saved->model))return false;
    }
    if(!SkinSetSceneMask(weapon,saved->mask))return false;
    if(saved->hudEntity&&SkinEntity(saved->hudHandle)==saved->hudEntity) {
        if(!SkinSetVisualModel(saved->hudEntity,saved->hudModel)||!SkinSetSceneMask(saved->hudEntity,saved->hudMask))return false;
    }
    if(!RebuildSkin(weapon))return false;
    if(saved->attachmentRefreshNeeded)RefreshArmoryVisual(saved,true);
    saved->entity=0;return true;
}
static bool ApplySkin(SkinSaved* saved,const SkinConfig& config) {
    auto weapon=saved->entity;
    if(!weapon||SkinEntity(saved->handle)!=weapon||!SkinFunctionValid(SKIN_SET_ATTR)||
       !SkinFunctionValid(SKIN_UPDATE_SKIN)||!SkinFunctionValid(SKIN_MESH_MASK))return false;
    const auto address=reinterpret_cast<uintptr_t>(&config),begin=reinterpret_cast<uintptr_t>(skinSettings);
    size_t slot=address>=begin&&address<begin+sizeof(skinSettings)?(address-begin)/sizeof(SkinConfig):64;
    uint64_t armoryVersion{};auto armory=HazzureArmory::Snapshot(slot,config.definition,armoryVersion);
    if(!ArmoryFits(saved,armory))return false;
    if(!SkinRefreshHud(saved))return false;
    if(config.model&&Get<unsigned short>(weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex)!=config.model) {
        const char* kPath = KnifePath(config.model);
        if(kPath) {
            if(!SetSkinModel(weapon,config.model,kPath))return false;
        }
    }
    if(config.model&&saved->hudEntity) {
        char currentModel[240]{};uint64_t currentMask{};const char* path=KnifePath(config.model);
        if(!path||!SkinModelState(saved->hudEntity,currentModel,sizeof(currentModel),currentMask))return false;
        if(strcmp(currentModel,path)&&!SkinSetVisualModel(saved->hudEntity,path))return false;
    }
    if(!config.model&&Get<unsigned short>(weapon+HazzureOffsets::Schema::m_iItemDefinitionIndex)!=saved->definition) {
        if(!SetSkinModel(weapon,saved->definition,saved->model))return false;
        if(saved->hudEntity&&SkinEntity(saved->hudHandle)==saved->hudEntity&&!SkinSetVisualModel(saved->hudEntity,saved->hudModel))return false;
    }
    // Stage the entity fields the engine's own fallback route reads (sub_181143E00).
    // The id must stay non-zero and unresolvable, otherwise the engine paints from the
    // resolved store item and ignores the fallback values entirely.
    if(!SkinWrite(weapon+HazzureOffsets::Schema::m_iItemIDHigh,-1)||!SkinWrite(weapon+HazzureOffsets::Schema::m_iItemIDLow,-1)||
       !SkinWrite(weapon+HazzureOffsets::Schema::m_nFallbackPaintKit,config.paint)||!SkinWrite(weapon+HazzureOffsets::Schema::m_nFallbackSeed,config.seed)||
       !SkinWrite(weapon+HazzureOffsets::Schema::m_flFallbackWear,config.wear))return false;
    // Restore the pristine list, then let the engine append the paint attributes to the
    // list at view+0x208 and recompute the view. Allocation/growth stays with the engine.
    if(!SkinRestoreAttributes(SkinAttrList(SkinWeaponView(weapon)),saved->attributeCount,saved->attributes)||
       !SkinSetAttributes(SkinWeaponView(weapon),config)) {
        SkinChain("weapon-attrs",weapon,config,false);
        return false;
    }
    if(!ApplyArmory(saved,armory))return false;
    saved->armoryRevision=armoryVersion;
    SkinMask(weapon,config.legacy?2:1);
    bool rebuilt=RebuildSkin(weapon);
    SkinChain("weapon",weapon,config,rebuilt);
    if(!rebuilt)return false;
    saved->attachmentRefreshNeeded=saved->attachmentRefreshNeeded||armory.charm||armory.stat||armory.nametag;
    if(saved->attachmentRefreshNeeded)RefreshArmoryVisual(saved);
    saved->revision=skinRevision;InterlockedIncrement64(&shared->skinChanges);return true;
}
struct SkinGloveSaved {
    uintptr_t pawn{};uint32_t handle{};LONG revision{-1};
    unsigned short definition{};int high{},low{},attributeCount{};bool initialized{};
    bool restoreMaterials{};bool disallowSOC{};unsigned reapplyFrames{};uint64_t nextAudit{},modelEpoch{};
    HazzureGloveLifecycle::Refresh appearance{};
    unsigned char attributes[128*0x48]{};
};
static SkinGloveSaved skinGloves;
static bool SkinGloveAppearance(uintptr_t pawn,HazzureGloveLifecycle::Appearance& out){
    HazzureGloveLifecycle::Appearance value{};char model[240]{};float spawn{};
    value.pawn=pawn;value.agentEpoch=skinPawnModelEpoch;
    if(!SkinIdentity(pawn,value.handle)||!SkinModelState(pawn,model,sizeof(model),value.meshMask)||
       !Read(reinterpret_cast<void*>(pawn+HazzureOffsets::Schema::m_pGameSceneNode),&value.scene,sizeof(value.scene))||!value.scene||
       !Read(reinterpret_cast<void*>(pawn+HazzureOffsets::Schema::m_iTeamNum),&value.team,sizeof(value.team))||
       !Read(reinterpret_cast<void*>(pawn+0x14ec),&spawn,sizeof(spawn))||!std::isfinite(spawn)||spawn<0)return false;
    // m_flLastSpawnTimeIndex was confirmed in the installed schema. A respawn
    // can rebuild first-person arms without changing the pawn pointer or skin revision.
    memcpy(&value.spawn,&spawn,sizeof(spawn));value.model=HazzureGloveLifecycle::ModelHash(model);
    // The first-person hands carry the glove model, so an engine-side rebuild of the
    // arms (respawn, team swap, agent reload) reverts what the player sees even when
    // the staged econ fields survived. Observing it is what makes the revert visible.
    if(auto arms=SkinHudArms(pawn,&value.armsHandle)){
        char armsPath[240]{};uint64_t armsMask{};
        value.arms=arms;
        Read(reinterpret_cast<void*>(arms+HazzureOffsets::Schema::m_pGameSceneNode),&value.armsScene,sizeof(value.armsScene));
        if(SkinModelState(arms,armsPath,sizeof(armsPath),armsMask))value.armsModel=HazzureGloveLifecycle::ModelHash(armsPath);
    }
    uintptr_t services{};uint32_t weaponHandle{};
    if(Read(reinterpret_cast<void*>(pawn+HazzureOffsets::Schema::m_pWeaponServices),&services,sizeof(services))&&services&&
       Read(reinterpret_cast<void*>(services+HazzureOffsets::Schema::m_hActiveWeapon),&weaponHandle,sizeof(weaponHandle))){
        auto weapon=SkinEntity(weaponHandle);value.hud=weapon?SkinHudWeapon(weapon,&value.hudHandle):0;
        if(value.hud&&!Read(reinterpret_cast<void*>(value.hud+HazzureOffsets::Schema::m_pGameSceneNode),&value.hudScene,sizeof(value.hudScene)))return false;
    }
    if(!HazzureGloveLifecycle::Valid(value))return false;out=value;return true;
}
static bool NotifyGloves(uintptr_t pawn){
    // m_nEconGlovesChanged is uint8 at 0x1D20 in the fingerprinted build.
    // A readable address elsewhere in the pawn is never a field validation.
    uint8_t changed{};
    return pawn&&Read(reinterpret_cast<void*>(pawn+kSkinGlovesChanged),&changed,1)&&
        SkinWrite(pawn+kSkinGlovesChanged,static_cast<uint8_t>(changed+1))&&
        SkinWrite(pawn+kSkinGlovesReapply,true);
}
static bool SkinDriveBodyGroups(uintptr_t pawn) {
    // A luva so vira geometria pelo caminho do proprio motor: o nome do bodygroup e
    // internado em runtime (nao existe como string no client.dll) e a escolha cai no
    // mapa do modelo do agente. Sem este par de chamadas o m_EconGloves escrito permanece
    // sem efeito, que e o estado em que a luva esteve desde a t039 nos dois times.
    if(!pawn||!SkinFunctionValid(SKIN_SET_BODY_GROUP)||!SkinFunctionValid(SKIN_UPDATE_BODY_GROUPS))return false;
    __try{
        reinterpret_cast<void(*)(void*,const char*,int)>(client+SKIN_SET_BODY_GROUP.rva)(
            reinterpret_cast<void*>(pawn),"first_or_third_person",1);
        reinterpret_cast<void(*)(void*)>(client+SKIN_UPDATE_BODY_GROUPS.rva)(reinterpret_cast<void*>(pawn));
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    return true;
}
static void GloveBodyGroupBurst(uintptr_t local) {
    if(!skinGloves.pawn||!skinGloves.reapplyFrames)return;
    if(!local||SkinEntity(skinGloves.handle)!=local){skinGloves.reapplyFrames=0;return;}
    auto words=Get<unsigned>(local+kSkinBodyGroupChoiceWords);
    auto tail=Get<unsigned>(local+kSkinBodyGroupChoiceWords+4);
    bool ok=SkinDriveBodyGroups(local);
    --skinGloves.reapplyFrames;
    LogDebug("[GLOVEBG] frame=%u ok=%d choices=0x%X/0x%X soc=%d reapply=%d changed=%u",
        skinGloves.reapplyFrames+1,ok?1:0,words,tail,
        static_cast<int>(Get<bool>(local+kSkinGloves+kSkinGlovesDisallowSOC)),
        static_cast<int>(Get<bool>(local+kSkinGlovesReapply)),
        static_cast<unsigned>(Get<unsigned char>(local+kSkinGlovesChanged)));
}
static bool RestoreGloves() {
    if(!skinGloves.pawn)return true;
    if(SkinEntity(skinGloves.handle)!=skinGloves.pawn){skinGloves={};return true;}
    uintptr_t item=skinGloves.pawn+kSkinGloves;
    if(!SkinRestoreAttributes(SkinAttrList(item),skinGloves.attributeCount,skinGloves.attributes)||
       !SkinWrite(item+kSkinItemDefinition,skinGloves.definition)||!SkinWrite(item+kSkinItemHigh,skinGloves.high)||
       !SkinWrite(item+kSkinItemLow,skinGloves.low)||!SkinWrite(item+kSkinItemInitialized,skinGloves.initialized)||
       !SkinWrite(item+0x1b8,skinGloves.restoreMaterials)||
       !SkinWrite(item+kSkinGlovesDisallowSOC,skinGloves.disallowSOC)||!NotifyGloves(skinGloves.pawn))return false;
    // Devolver o estado pedido tambem precisa de re-apply, senao o burst da aplicacao
    // deixaria a escolha de bodygroup apontando para a luva depois do desligamento.
    SkinDriveBodyGroups(skinGloves.pawn);
    skinGloves={};return true;
}
static bool ApplyGloves(uintptr_t local, const SkinConfig* customConfig = nullptr) {
    const SkinConfig* config = customConfig;
    if(!config && skinOn) {
        for(const auto& candidate : skinSettings) if(candidate.enabled && (Glove(candidate.definition) || Glove(candidate.model))) { config = &candidate; break; }
    }
    if(skinGloves.pawn && (skinGloves.pawn != local || SkinEntity(skinGloves.handle) != skinGloves.pawn) && !RestoreGloves()) return false;
    if(!local || !config) return RestoreGloves();
    SkinConfig effective=*config;
    if(!effective.paint)effective.paint=HazzureGloves::DefaultPaint(effective.model?effective.model:effective.definition);
    config=&effective;
    const int definition=config->model?config->model:config->definition;
    const char* path=GlovePath(definition);if(!path)return false;
    const uintptr_t item=local+kSkinGloves;
    if(!skinGloves.pawn) {
        SkinGloveSaved candidate{};uintptr_t attributes{};
        if(!SkinIdentity(local,candidate.handle))return false;
        if(!Read(reinterpret_cast<void*>(item+kSkinItemDefinition),&candidate.definition,sizeof(candidate.definition))||
           !Read(reinterpret_cast<void*>(item+kSkinItemHigh),&candidate.high,sizeof(candidate.high))||
           !Read(reinterpret_cast<void*>(item+kSkinItemLow),&candidate.low,sizeof(candidate.low))||
           !Read(reinterpret_cast<void*>(item+kSkinItemInitialized),&candidate.initialized,sizeof(candidate.initialized))||
           !Read(reinterpret_cast<void*>(item+0x1b8),&candidate.restoreMaterials,sizeof(candidate.restoreMaterials))||
           !Read(reinterpret_cast<void*>(item+kSkinGlovesDisallowSOC),&candidate.disallowSOC,sizeof(candidate.disallowSOC))||
           !SkinAttributeView(SkinAttrList(item),candidate.attributeCount,attributes)||
           (candidate.attributeCount&&!Read(reinterpret_cast<void*>(attributes),candidate.attributes,static_cast<size_t>(candidate.attributeCount)*0x48)))return false;
        candidate.pawn=local;skinGloves=candidate;
    }
    auto now=GetTickCount64();
    HazzureGloveLifecycle::Appearance appearance{};
    if(!SkinGloveAppearance(local,appearance)){skinGloves.appearance.Observe({});return false;}
    bool modelChanged=skinGloves.appearance.Observe(appearance);
    if(modelChanged){skinGloves.modelEpoch=skinPawnModelEpoch;skinGloves.nextAudit=0;}
    bool matching=Get<unsigned short>(item+kSkinItemDefinition)==definition&&Get<int>(item+kSkinItemHigh)==-1&&
        Get<int>(item+kSkinItemLow)==-1&&Get<bool>(item+kSkinItemInitialized)&&Get<bool>(item+0x1b8)&&
        !Get<bool>(item+kSkinGlovesDisallowSOC);
    bool refresh=skinGloves.appearance.Due(now);
    if(!modelChanged&&!refresh&&skinGloves.revision==skinRevision&&matching&&now<skinGloves.nextAudit)return true;
    skinGloves.nextAudit=now+100;
    bool textureMatches=SkinPaintMatches(item,*config);
    if(!modelChanged&&!refresh&&skinGloves.revision==skinRevision&&matching&&textureMatches)return true;
    // The old cache returned above on a new agent as long as the inline item
    // still matched. Native model/scene/bodygroup/spawn/HUD changes now force a
    // material refresh even when those econ fields have survived the rebuild.
    if(modelChanged||skinGloves.revision!=skinRevision||!matching||!textureMatches)skinGloves.appearance.Require();
    if(path)PrecacheModelResource(path);
    // Invalidate the store id before the engine recomputes: with an unresolvable id the
    // view paints from the values just staged in its attribute list.
    if(!SkinWrite(item+kSkinItemDefinition,static_cast<unsigned short>(definition))||!SkinWrite(item+kSkinItemHigh,-1)||
       !SkinWrite(item+kSkinItemLow,-1)||!SkinWrite(item+kSkinItemInitialized,true)||!SkinWrite(item+0x1b8,true)||
       !SkinWrite(item+kSkinGlovesDisallowSOC,false)) {
        SkinChain("gloves-fields",item,*config,false);return false;
    }
    if(!SkinSetAttributes(item,*config)) {SkinChain("gloves",item,*config,false);return false;}
    if(!NotifyGloves(local)) {SkinChain("gloves-notify",item,*config,false);return false;}
    // Escrever o modelo da luva na entidade de maos foi medido e rejeitado: o motor
    // dono do mesh group mask devolve 0xFFFFFFFFFFFFFFFE no frame seguinte e o modelo
    // de luva no lugar do modelo do agente remove a geometria das mãos (faca flutuando,
    // sessao t042). O visual da luva so pode vir do re-apply do proprio motor.
    SkinChain("gloves",item,*config,true);
    skinGloves.appearance.Acknowledge(now);
    // A referencia reaplica por tres frames; o contador que existia aqui era escrito e
    // nunca lido, entao agora ele e o proprio burst de bodygroup.
    skinGloves.reapplyFrames=3;
    skinGloves.revision=skinRevision;InterlockedIncrement64(&shared->skinChanges);return true;
}
static void MaterializeInventoryCosmetics(uintptr_t client);
// Verification the T-side revert needs: what the hands entity carries frame by frame,
// for a bounded window after the team or the wanted glove changes. If the definition
// index and the model string alternate between the virtual value and the default, the
// overwriter is real and this is where it shows.
static void GloveViewModelProbe(uintptr_t local,unsigned char team,int wanted,const char* path){
    static uint64_t windowEnd{},nextLine{};static unsigned char lastTeam{};static int lastWanted{};
    static uintptr_t lastPawn{};
    auto now=GetTickCount64();
    // Re-armar tambem em pawn novo: respawn e map load trocam a entidade sem trocar o
    // luva pedida, e era justamente ai que a maquina de provas ficava muda.
    if(team!=lastTeam||wanted!=lastWanted||local!=lastPawn){
        lastTeam=team;lastWanted=wanted;lastPawn=local;windowEnd=now+6000;nextLine=0;
    }
    if(now>=windowEnd||now<nextLine)return;
    nextLine=now+250;
    auto staged=Get<unsigned short>(local+kSkinGloves+kSkinItemDefinition);
    uint32_t armsHandle{};auto arms=SkinHudArms(local,&armsHandle);
    char armsPath[240]{};uint64_t armsMask{};
    int armsDef=-1;int armsAttributes=-1;
    if(arms){
        SkinModelState(arms,armsPath,sizeof(armsPath),armsMask);
        int count{};uintptr_t values{};
        auto view=arms+kSkinItemView;
        if(SkinAttributeView(SkinAttrList(view),count,values)){
            armsAttributes=count;auto def=Get<unsigned short>(view+kSkinItemDefinition);
            if(def&&def<60000)armsDef=def;
        }
    }
    LogDebug("[GLOVEVIEW] team=%d pawn=%p stagedDef=%d wantDef=%d wantPath=%s arms=%p/%u armsDef=%d attrs=%d armsModel=%s mask=%llX reapply=%d changed=%u ok=%d",
        team,reinterpret_cast<void*>(local),static_cast<int>(staged),wanted,path?path:"-",
        reinterpret_cast<void*>(arms),armsHandle,armsDef,armsAttributes,armsPath[0]?armsPath:"-",
        static_cast<unsigned long long>(armsMask),static_cast<int>(Get<bool>(local+kSkinGlovesReapply)),
        static_cast<unsigned>(Get<unsigned char>(local+kSkinGlovesChanged)),
        arms&&path&&SkinSameModel(armsPath,path)?1:0);
}
static void SkinFrameWork() {
    if(!shared)return;
    auto local=HazzureGame::LocalPawn(client);
    HazzureGame::Pawn localState{};
    if(!HazzureGame::ReadPawn(local,localState))local=0;
    ApplyAgentChanger(local);
    MaterializeInventoryCosmetics(client);
    GloveBodyGroupBurst(local);
}
// Consume the UI queue on the observed post-network callback, independently of
// local-pawn readiness or visual skin processing. Runtime::Frame enforces owner
// thread, serialization and the existing two-item budget.
static void NativeInventoryTick(int stage,bool suspended){
    HazzureNativeInventory::Frame(client,suspended);
    auto snapshot=HazzureNativeInventory::Snapshot();
    static SRWLOCK diagnosticLock=SRWLOCK_INIT;
    if(!TryAcquireSRWLockExclusive(&diagnosticLock))return;
    static HazzureNativeInventory::RuntimeSnapshot previous{};
    static DWORD previousThread{};static LONG previousMask{};static bool previousSuspended{},logged{};
    DWORD thread=GetCurrentThreadId();LONG mask=shared?InterlockedCompareExchange(&shared->stageMask,0,0):0;
    if(!logged||snapshot.requested!=previous.requested||snapshot.processed!=previous.processed||
       snapshot.diagnostic.status!=previous.diagnostic.status||snapshot.diagnostic.records!=previous.diagnostic.records||
       snapshot.equipRequested!=previous.equipRequested||snapshot.equipProcessed!=previous.equipProcessed||
       snapshot.lastEquipSucceeded!=previous.lastEquipSucceeded||snapshot.lastEquipFailedID!=previous.lastEquipFailedID||
       snapshot.reasserted!=previous.reasserted||snapshot.reassertRejected!=previous.reassertRejected||
       snapshot.reassertBlocked!=previous.reassertBlocked||
       snapshot.enabled!=previous.enabled||suspended!=previousSuspended||thread!=previousThread||mask!=previousMask){
        LogDebug("[INVENTORY] stage=%d stages=0x%lX thread=%lu suspended=%d requested=%llu processed=%llu status=%d records=%zu equipReq=%llu equipProc=%llu equipOk=%d equipFail=%llu reassert=%llu reject=%llu rblock=%llu",
                 stage,mask,thread,suspended,static_cast<unsigned long long>(snapshot.requested),
                 static_cast<unsigned long long>(snapshot.processed),static_cast<int>(snapshot.diagnostic.status),snapshot.diagnostic.records,
                 static_cast<unsigned long long>(snapshot.equipRequested),static_cast<unsigned long long>(snapshot.equipProcessed),
                 snapshot.lastEquipSucceeded?1:0,static_cast<unsigned long long>(snapshot.lastEquipFailedID),
                 static_cast<unsigned long long>(snapshot.reasserted),static_cast<unsigned long long>(snapshot.reassertRejected),
                 static_cast<unsigned long long>(snapshot.reassertBlocked));
        previous=snapshot;previousThread=thread;previousMask=mask;previousSuspended=suspended;logged=true;
    }
    ReleaseSRWLockExclusive(&diagnosticLock);
}
static void WorldEffectsFrame(bool ready){
    HazzureWeather::Frame(client,ready,HazzureWorld::epoch.load());
    if(ready)HazzureSmoke::ProcessPending(client);else HazzureSmoke::Clear();
    if(ready&&HazzureZones::enabled.load())HazzureInferno::Frame(client,HazzureZones::Snapshot(GetTickCount64()));
    else HazzureInferno::Clear();
}
// Current PE route: inventory and the independently reviewed fog fields only.
// Legacy skin/combat/trace writers below do not belong to this callback.
static void MaterializeInventoryCosmetics(uintptr_t client) {
    auto local = HazzureGame::LocalPawn(client);
    HazzureGame::Pawn localState{};
    if(!local || !HazzureGame::ReadPawn(local, localState)) return;
    // SkinFrameWork already applied the agent changer this frame; a second call
    // here would re-read and re-drive the same model every frame.
    // Native SOCache owns which item is equipped; the entity writer below owns how the
    // held weapon paints. They are complementary: the writer only stages the engine's
    // own fallback attributes (sub_181143E00) on the view, which is what the HUD and the
    // first-person model read. It was disabled in T033 because its setter addressed the
    // attribute list as if it were the view, so every write landed outside the item.
    unsigned char team = Get<unsigned char>(local + HazzureOffsets::Schema::m_iTeamNum);
    if(team != 2 && team != 3) return;
    auto nativeSnap = HazzureNativeInventory::Snapshot();
    const auto& collection = HazzureUI::g_cfg.inventory;

    auto findEquipped = [&](int weaponDef, bool isKnife, bool isGlove) -> const HazzureInventory::Item* {
        // Priority 1: native snap equipped
        for(size_t i = 0; i < collection.count; ++i) {
            const auto& item = collection.items[i];
            for(size_t k = 0; k < nativeSnap.itemCount && k < nativeSnap.items.size(); ++k) {
                if(nativeSnap.items[k].localItemID == item.id) {
                    bool eq = (team == 2) ? nativeSnap.items[k].equippedT : nativeSnap.items[k].equippedCT;
                    if(eq) {
                        if(isGlove && (HazzureInventory::Glove(item.skin.definition) || HazzureInventory::Glove(item.skin.model))) return &item;
                        if(isKnife && (HazzureArmory::KnifeDefinition(item.skin.definition) || HazzureArmory::KnifeDefinition(item.skin.model))) return &item;
                        if(!isGlove && !isKnife && HazzureNativeInventory::NativeDefinition(item) == weaponDef) return &item;
                    }
                    break;
                }
            }
        }
        // Priority 2: direct collection match (standalone internal mode)
        for(size_t i = 0; i < collection.count; ++i) {
            const auto& item = collection.items[i];
            if(isGlove) {
                if(HazzureInventory::Glove(item.skin.definition) || HazzureInventory::Glove(item.skin.model)) return &item;
            } else if(isKnife) {
                if(HazzureArmory::KnifeDefinition(item.skin.definition) || HazzureArmory::KnifeDefinition(item.skin.model)) return &item;
            } else {
                if(HazzureNativeInventory::NativeDefinition(item) == weaponDef) return &item;
            }
        }
        return nullptr;
    };

    // 1. Gloves
    const auto* gloveItem = findEquipped(0, false, true);
    if(gloveItem) {
        SkinConfig gc{}; gc.enabled = true;
        gc.definition = gloveItem->skin.definition;
        gc.model = gloveItem->skin.model ? gloveItem->skin.model : gloveItem->skin.definition;
        gc.paint = gloveItem->skin.paint;
        gc.seed = gloveItem->skin.seed;
        gc.wear = gloveItem->skin.wear;
        ApplyGloves(local, &gc);
        GloveViewModelProbe(local, team, gc.model ? gc.model : gc.definition, GlovePath(gc.model ? gc.model : gc.definition));
    } else {
        RestoreGloves();
        GloveViewModelProbe(local, team, 0, nullptr);
    }

    // 2. Weapons & Knives
    auto services = Get<uintptr_t>(local + HazzureOffsets::Schema::m_pWeaponServices);
    if(!services) return;
    uint32_t handle = Get<uint32_t>(services + HazzureOffsets::Schema::m_hActiveWeapon);
    auto weapon = SkinEntity(handle);
    if(!weapon || SkinEntity(Get<uint32_t>(weapon + HazzureOffsets::Schema::m_hOwnerEntity)) != local) return;

    for(auto& saved : skinSaved) if(saved.entity) {
        if(SkinEntity(saved.handle) != saved.entity || SkinEntity(Get<uint32_t>(saved.entity + HazzureOffsets::Schema::m_hOwnerEntity)) != local) {
            RestoreSkin(&saved);
        }
    }

    auto definition = Get<unsigned short>(weapon + HazzureOffsets::Schema::m_iItemDefinitionIndex);
    SkinSaved* saved = nullptr;
    for(auto& s : skinSaved) if(s.entity == weapon && s.handle == handle) {
        saved = &s; definition = s.definition; break;
    }

    const bool isKnife = Knife(definition);
    const auto* equippedItem = findEquipped(definition, isKnife, false);
    if(!equippedItem) {
        static ULONGLONG noItemNext{};
        if(GetTickCount64()>=noItemNext) {noItemNext=GetTickCount64()+2000;
            LogDebug("[SKINCHAIN] no-equipped-item def=%d knife=%d collected=%u nativeReady=%d status=%d",
                definition,isKnife?1:0,static_cast<unsigned>(collection.count),
                nativeSnap.available?1:0,static_cast<int>(nativeSnap.diagnostic.status));}
        if(saved) RestoreSkin(saved);
        return;
    }

    if(!saved) {
        for(auto& s : skinSaved) if(!s.entity) { saved = &s; break; }
        if(!saved) return;
        if(!SaveSkin(saved, weapon, handle)) {
            SkinChain("save-failed",SkinWeaponView(weapon),SkinConfig{},false);
            return;
        }
    }

    SkinConfig config{}; config.enabled = true;
    config.definition = equippedItem->skin.definition;
    config.paint = equippedItem->skin.paint;
    config.seed = equippedItem->skin.seed;
    config.wear = equippedItem->skin.wear;
    config.legacy = equippedItem->skin.legacy;
    if(isKnife) {
        config.model = equippedItem->skin.model ? equippedItem->skin.model : equippedItem->skin.definition;
    }

    uint32_t currentHudHandle{};
    uintptr_t currentHud = SkinHudWeapon(weapon, &currentHudHandle);
    auto now = GetTickCount64();
    if(now < saved->nextRetry) return;

    HazzureArmory::Profile armoryProfile = equippedItem->attributes;
    if(saved->hudEntity == currentHud && saved->hudHandle == currentHudHandle) {
        if(now < saved->nextAudit) return;
        saved->nextAudit = now + 100;
        if(SkinCurrentMatches(saved, config, armoryProfile)) return;
    }

    if(!ApplySkin(saved, config)) {
        saved->revision = -1;
        saved->nextRetry = now + 250;
        return;
    }
    saved->nextRetry = 0;
    saved->nextAudit = now + 100;
}

static void FrameHook(void* object,int stage) {
    // This PE has no live stage 9: case 9 of 0xB6EFE0 is a stub and the mask
    // never carries bit 9. Its per-frame stage is 6 (FramePostDataUpdate), so
    // aim traces, movement and recoil work must run there instead.
    // The emitted render pair is 10/11; keep the punch override inside 10 only.
    if(stage!=10||HazzureState::g_unloading.load())HazzureVisual::End();
    bool visual=stage==10&&!HazzureState::g_unloading.load()&&HazzureWorld::ready.load()&&
        HazzureVisibility::BuildMatches(client)&&HazzureVisual::Begin(HazzureGame::LocalPawn(client));
    bool completed=false;
    __try{originalFrame(object,stage);completed=true;}
    __finally{if(visual&&!completed)HazzureVisual::End();}
    InterlockedIncrement64(&shared->frames);
    if(stage>=0&&stage<31)InterlockedOr(&shared->stageMask,1L<<stage);
    if(stage!=6)return;
    if(HazzureState::g_unloading.load()){
        WorldEffectsFrame(false);
        NativeInventoryTick(stage,true);
        __try { HazzureGrenade::Clear();HazzureWeaponAccuracy::Clear();HazzureBrake::Stop();HazzureMovementTech::Stop();
            HazzureTactical::damageQueue.Reset();HazzureMove::Stop(client);HazzureTuning::Restore(client);RestoreWorldClimate();SkinFrameWork(); }
        __except(EXCEPTION_EXECUTE_HANDLER){InterlockedIncrement64(&shared->faults);}
        InterlockedExchange64(&shared->heartbeat,0);
        return;
    }
    if(HazzureWorld::Observe(client)){
        for(auto& state:skinSaved)state={};skinGloves={};skinRevision=-1;skinFailed=false;
        g_savedAgentEntity=0;g_savedAgentModel[0]=0;agentMaskPending=false;++skinPawnModelEpoch;
        ResetClimateSession();ResetMaterialSession();HazzureVisibility::Clear();HazzureAutowall::Clear();HazzureGrenade::Clear();
        HazzureTactical::damageQueue.Reset();HazzureCombat::roundEpoch.fetch_add(1);
        LogDebug("[SESSION] World transition; old snapshots and resource caches discarded");
    }
    // Installed PE dispatch labels stage 6 FramePostDataUpdate; consume after
    // original and Observe so no previous-world snapshots reach native queries.
    WorldEffectsFrame(HazzureWorld::ready.load());
    NativeInventoryTick(stage,false);
    if(!HazzureWorld::ready.load())return;
    ClimateFrame();
    // Formerly the dead stage-9 block. Stage 6 runs every frame on this PE, so
    // pending visibility traces complete and CreateMove consumers get fresh
    // commands instead of tracing forever.
    __try {
        HazzureMove::Frame(client);
        HazzureMovementTech::Frame(client);
        HazzureBrake::Frame(client);
        HazzureWeaponAccuracy::Frame(client);
        HazzureTuning::Frame(client);
        HazzureVisibility::ProcessPending(client);
        HazzureAutowall::ProcessPending(client);
        HazzureTsukuyomi::ProcessPending(client);
        HazzureTactical::damageQueue.Process(client);
        HazzureGrenade::ProcessPending(client,nullptr,nullptr,96);
    } __except(EXCEPTION_EXECUTE_HANDLER) { InterlockedIncrement64(&shared->faults); }
    if(skinFailed)return;
    // Watchdog legado desativado: menu interno no DX11
    InterlockedExchange64(&shared->heartbeat, static_cast<LONG64>(GetTickCount64()));
    LONG tid=static_cast<LONG>(GetCurrentThreadId());
    LONG observed=InterlockedCompareExchange(&shared->frameThread,tid,0);
    if(observed&&observed!=tid) { InterlockedExchange(&shared->frameThread,tid); }
    __try { SkinFrameWork(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { skinFailed=true;InterlockedIncrement64(&shared->faults);SkinMessage(-6,"Skin callback fault; processing stopped"); }
}
