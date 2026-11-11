static void Message(const char* text) {

    if(shared)strncpy_s(shared->message,text,_TRUNCATE);

}

static bool Read(const void* ptr,void* out,size_t bytes) {

    if(!ptr)return false;

    __try { memcpy(out,ptr,bytes);return true; }

    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }

}

template<class T> static T Get(uintptr_t ptr) { T value{};Read(reinterpret_cast<void*>(ptr),&value,sizeof(value));return value; }

static bool MatchModule(HMODULE module,unsigned stamp,unsigned size) {

    if(!module)return false;

    auto base=reinterpret_cast<uintptr_t>(module);

    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);

    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;

    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);

    return nt->Signature==IMAGE_NT_SIGNATURE&&nt->FileHeader.TimeDateStamp==stamp&&nt->OptionalHeader.SizeOfImage==size;

}

static void* MakeSkyMaterial(int index = 0);

static bool PrecacheModelResource(const char* name);
static void ResetMaterialSession();

static uintptr_t Entity(uint32_t handle) {

    if(handle==0xffffffff)return 0;

    uintptr_t list=Get<uintptr_t>(client+HazzureOffsets::Globals::dwEntityList);

    uintptr_t chunk=Get<uintptr_t>(list+0x10+8*((handle&0x7fff)>>9));

    uintptr_t slot=chunk+0x70*(handle&0x1ff);

    if(HazzureGame::CanonicalHandle(Get<uint32_t>(slot+0x10),Get<uint32_t>(slot+0x30))!=handle)return 0;

    return Get<uintptr_t>(slot);

}

static bool ClassName(uintptr_t entity,char* out,size_t capacity) {

    // MSVC x64 RTTI: CompleteObjectLocator precedes the primary vtable.

    // No obsolete schema pointer offsets and no calls through guessed vtable slots.

    uintptr_t table=Get<uintptr_t>(entity);

    uintptr_t col=Get<uintptr_t>(table-8);

    if(Get<uint32_t>(col)!=1)return false;

    uint32_t selfRva=Get<uint32_t>(col+20),typeRva=Get<uint32_t>(col+12);

    if(!selfRva||!typeRva||selfRva>0x4000000||typeRva>0x4000000)return false;

    uintptr_t name=col-selfRva+typeRva+16;

    if(!name)return false;

    for(size_t i=0;i<capacity;i++) {

        char ch{};if(!Read(reinterpret_cast<void*>(name+i),&ch,1))return false;

        out[i]=ch;if(ch==0)return i>0;

    }

    out[capacity-1]=0;return false;

}

#include "Climate.h"

#include "SkinEngine.h"

#include "AudioEvents.h"

using CameraGetter=float(__fastcall*)(void*);
static CameraGetter originalCamera;
static float __fastcall CameraFovHook(void* camera){
    if(!originalCamera||!camera)return 90.f;
    __try {
        // Preserve every native side effect, weapon modifier and scoped result first.
        float native=originalCamera(camera);
        return HazzureCamera::Resolve(client,reinterpret_cast<uintptr_t>(camera),native,
            !HazzureState::g_unloading.load()&&HazzureWorld::ready.load());
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return 90.f;
    }
}


using NativeViewSetup=void(__fastcall*)(void*,void*);
using NativeDrawEntity=bool(__fastcall*)(void*,void*);
static NativeViewSetup originalViewSetup;
static NativeDrawEntity originalDrawEntity;
static void __fastcall NativeViewHook(void* mode,void* view){
    if(!originalViewSetup||!mode||!view)return;
    __try{
        originalViewSetup(mode,view);
        HazzureNativeCamera::Apply(client,reinterpret_cast<uintptr_t>(view),
            !HazzureState::g_unloading.load()&&HazzureWorld::ready.load(),HazzureWorld::epoch.load(),GetTickCount64());
    }__except(EXCEPTION_EXECUTE_HANDLER){HazzureNativeCamera::ClearBody();}
}
static bool __fastcall NativeDrawEntityHook(void* mode,void* entity){
    if(!originalDrawEntity||!mode||!entity)return false;
    __try{
        if(HazzureNativeCamera::ViewModelSuppress()){
            const uintptr_t target=reinterpret_cast<uintptr_t>(entity);
            char name[96]{};
            if(ClassName(target,name,sizeof(name))&&
               (!strcmp(name,".?AVC_CS2HudModelArms@@")||!strcmp(name,".?AVC_CS2HudModelWeapon@@"))){
                HazzureNativeCamera::viewSuppressions.fetch_add(1,std::memory_order_relaxed);
                return false;
            }
            if(target==HazzureGame::LocalPawn(client))
                HazzureNativeCamera::bodyCalls.fetch_add(1,std::memory_order_relaxed);
        }
        const bool native=originalDrawEntity(mode,entity);
        return native||HazzureNativeCamera::DrawLocal(client,reinterpret_cast<uintptr_t>(entity),
            !HazzureState::g_unloading.load()&&HazzureWorld::ready.load(),HazzureWorld::epoch.load(),GetTickCount64());
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}

static constexpr const char* kvPrefix="<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->";

struct KVId { const char* name;uint64_t a,b; };

static void* MakeMaterial(const char* name,bool ignoreDepth,bool translucent,int style = 0) {

    if(!arenaCtor || !loadKV || !createMaterial || !materialSystem) return nullptr;

    void* arena=VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);

    if(!arena)return nullptr;

    Message("Constructing KV3 arena");

    __try {

        arenaCtor(arena,8192,true);

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        VirtualFree(arena, 0, MEM_RELEASE);

        return nullptr;

    }

    void* kv=static_cast<unsigned char*>(arena)+0x78;

    char text[2048];

    int trans = (style == 1 || style == 3 || style == 4) ? 1 : (translucent ? 1 : 0);

    int blend = (style == 1 || style == 3 || style == 4) ? 1 : (translucent ? 1 : 0);

    int wire = (style == 2) ? 1 : 0;

    if (style == 1) { // Metallic / Chrome / Complex Self-Illum
        sprintf_s(text, "%s\n{\n shader = \"csgo_complex.vfx\"\n F_SELF_ILLUM = 1\n F_PAINT_VERTEX_COLORS = 1\n F_TRANSLUCENT = %d\n F_DISABLE_Z_BUFFERING = %d\n F_DISABLE_Z_WRITE = %d\n g_vColorTint = [1.0, 1.0, 1.0, 1.0]\n g_flSelfIllumScale = [3.0, 3.0, 3.0, 3.0]\n g_flSelfIllumBrightness = [3.0, 3.0, 3.0, 3.0]\n g_vSelfIllumTint = [1.0, 1.0, 1.0, 1.0]\n g_tColor = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n g_tNormal = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n g_tSelfIllumMask = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n}",
            kvPrefix, trans, ignoreDepth?1:0, (ignoreDepth||trans)?1:0);
    } else if (style == 3) { // Fresnel Rim-Glow (Neverlose signature)
        sprintf_s(text, "%s\n{\n shader = \"csgo_effects.vfx\"\n g_flFresnelExponent = 7.0\n g_flFresnelFalloff = 10.0\n g_flFresnelMax = 0.1\n g_flFresnelMin = 1.0\n g_vColorTint = [1.0, 1.0, 1.0, 1.0]\n F_IGNOREZ = %d\n F_TRANSLUCENT = 1\n F_DISABLE_Z_WRITE = %d\n F_DISABLE_Z_BUFFERING = %d\n F_RENDER_BACKFACES = 0\n g_tColor = resource:\"materials/dev/primary_white_color_tga_21186c76.vtex\"\n g_tMask1 = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n g_tMask2 = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n g_tMask3 = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n g_tSceneDepth = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n}",
            kvPrefix, ignoreDepth?1:0, (ignoreDepth||trans)?1:0, ignoreDepth?1:0);
    } else if (style == 4) { // Crystal / Glass
        sprintf_s(text, "%s\n{\n shader = \"csgo_complex.vfx\"\n F_PAINT_VERTEX_COLORS = 1\n F_TRANSLUCENT = 1\n F_BLEND_MODE = 1\n F_DISABLE_Z_WRITE = 1\n F_DISABLE_Z_BUFFERING = %d\n g_vColorTint = [1.0, 1.0, 1.0, 0.45]\n g_tColor = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n g_tNormal = resource:\"materials/default/default_normal_tga_7652cb.vtex\"\n}",
            kvPrefix, ignoreDepth?1:0);
    } else { // 0 = Flat, 2 = Wireframe
        const char* tint = (style == 1) ? "2.5" : (translucent ? "0.35" : "1.0");
        sprintf_s(text,"%s\n{ shader = \"csgo_unlitgeneric.vfx\"\n F_PAINT_VERTEX_COLORS = 1\n F_TRANSLUCENT = %d\n F_BLEND_MODE = %d\n F_DISABLE_Z_BUFFERING = %d\n F_DISABLE_Z_WRITE = %d\n F_WIREFRAME = %d\n g_vColorTint = [1.0,1.0,1.0,%s]\n g_tColor = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n}",
            kvPrefix, trans, blend, ignoreDepth?1:0, (ignoreDepth||trans)?1:0, wire, tint);
    }

    KVId id{"generic",0x41B818518343427E,0xB5F447C23C0CDF8C};

    Message("Loading KV3 text into arena root");

    if(!loadKV(kv,nullptr,text,&id,"",0)) {
        sprintf_s(text,"%s\n{ shader = \"csgo_unlitgeneric.vfx\"\n F_PAINT_VERTEX_COLORS = 1\n F_TRANSLUCENT = %d\n F_BLEND_MODE = %d\n F_DISABLE_Z_BUFFERING = %d\n F_DISABLE_Z_WRITE = %d\n F_WIREFRAME = %d\n g_vColorTint = [1.0,1.0,1.0,1.0]\n g_tColor = resource:\"materials/default/default_mask_tga_fde710a5.vtex\"\n}",
            kvPrefix, trans, blend, ignoreDepth?1:0, (ignoreDepth||trans)?1:0, wire);
        if(!loadKV(kv,nullptr,text,&id,"",0)) return nullptr;
    }

    void* strong=nullptr;

    Message("Creating material resource");

    createMaterial(materialSystem,&strong,name,kv,0,1,0,0,0,1);

    return strong?Get<void*>(reinterpret_cast<uintptr_t>(strong)):nullptr;

}

static uint64_t MurmurHash64B(const void* key,int len,uint64_t seed){

    const auto* data=static_cast<const unsigned char*>(key);

    const uint32_t m=0x5bd1e995;

    uint32_t h1=static_cast<uint32_t>(seed)^len,h2=static_cast<uint32_t>(seed>>32);

    while(len>=8){

        uint32_t k1,k2;memcpy(&k1,data,4);memcpy(&k2,data+4,4);

        k1*=m;k1^=k1>>24;k1*=m;h1*=m;h1^=k1;

        k2*=m;k2^=k2>>24;k2*=m;h2*=m;h2^=k2;

        data+=8;len-=8;

    }

    if(len>=4){uint32_t k1;memcpy(&k1,data,4);k1*=m;k1^=k1>>24;k1*=m;h1*=m;h1^=k1;data+=4;len-=4;}

    switch(len){

        case 3:h2^=static_cast<uint32_t>(data[2])<<16;[[fallthrough]];

        case 2:h2^=static_cast<uint32_t>(data[1])<<8;[[fallthrough]];

        case 1:h2^=data[0];h2*=m;

    }

    h1^=h2>>18;h1*=m;h2^=h1>>22;h2*=m;h1^=h2>>17;h1*=m;h2^=h1>>19;h2*=m;

    return (static_cast<uint64_t>(h1)<<32)|h2;

}

static bool PrecacheSkyMaterial(const char* name){

    if(!resourceSystem||!insertPath||!purgePath||!precacheMaterial)return false;

    // CBufferString at 0..15; ResourceSystem's path hash and extension at D0/D8.

    alignas(8) unsigned char path[0xe0]{};

    *reinterpret_cast<uint32_t*>(path+4)=0xc0000008;

    if(!insertPath(path,0,name,-1,false))return false;

    const char* normalized=(*reinterpret_cast<uint32_t*>(path+4)&0x40000000)?reinterpret_cast<const char*>(path+8):*reinterpret_cast<char**>(path+8);

    bool ok=false;

    if(normalized){

        auto length=static_cast<int>(strlen(normalized));

        *reinterpret_cast<uint64_t*>(path+0xd0)=MurmurHash64B(normalized,length,0xEDABCDEF);

        memcpy(path+0xd8,"vmat",5);

        ok=precacheMaterial(resourceSystem,path,"")!=nullptr;

    }

    purgePath(path,0);

    return ok;

}

static bool PrecacheModelResource(const char* name){

    if(!resourceSystem||!insertPath||!purgePath||!precacheMaterial)return false;

    alignas(8) unsigned char path[0xe0]{};

    *reinterpret_cast<uint32_t*>(path+4)=0xc0000008;

    if(!insertPath(path,0,name,-1,false))return false;

    const char* normalized=(*reinterpret_cast<uint32_t*>(path+4)&0x40000000)?reinterpret_cast<const char*>(path+8):*reinterpret_cast<char**>(path+8);

    bool ok=false;

    if(normalized){

        auto length=static_cast<int>(strlen(normalized));

        *reinterpret_cast<uint64_t*>(path+0xd0)=MurmurHash64B(normalized,length,0xEDABCDEF);

        memcpy(path+0xd8,"vmdl",5);

        ok=precacheMaterial(resourceSystem,path,"")!=nullptr;

    }

    purgePath(path,0);

    return ok;

}

static const char* kSkyboxCatalog[] = {

    "materials/skybox/sky_overcast_01.vmat",

    "materials/skybox/sky_black.vmat",

    "materials/skybox/sky_rain_night_01.vmat",

    "materials/skybox/sky_de_dust2.vmat",

    "materials/skybox/sky_de_mirage.vmat",

    "materials/skybox/sky_de_nuke.vmat",

    "materials/skybox/sky_de_vertigo.vmat",

    "materials/skybox/cs_office_45_0.vmat",

    "materials/skybox/cs_italy_s2_skybox_2.vmat",

    "materials/skybox/sky_cs15_daylight01_hdr.vmat"

};

static void* g_skyboxCache[10]{};
static void ResetMaterialSession(){
    // Retiring scene jobs may still own old bindings. New map lookups must start fresh.
    // Called on the session thread with the world gate already closed. Render
    // workers only observe the atomic preparation state; they do not reset it.
    preparing.store(0);failed.store(false);
    memset(g_skyboxCache,0,sizeof(g_skyboxCache));fallbackSkyMaterial=nullptr;skyResourceName[0]=0;
}



static void* MakeSkyMaterial(int index) {

    // The sky shader needs a compiled resource and its HDR texture. Creating a

    // text KV3 material returns a handle but renders without that texture.

    if (index < 0 || index >= 10) index = 0;

    if (g_skyboxCache[index]) return g_skyboxCache[index];

    const char* name = kSkyboxCatalog[index];

    if(!PrecacheSkyMaterial(name))return nullptr;

    void* strong=nullptr;

    void* resource=findMaterialFromResource(materialSystem,&strong,name);

    void* handle=resource?Get<void*>(reinterpret_cast<uintptr_t>(resource)):nullptr;

    if(handle){

        auto object=Get<uintptr_t>(reinterpret_cast<uintptr_t>(handle));

        auto table=Get<uintptr_t>(object),target=Get<uintptr_t>(table);

        auto module=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"materialsystem2.dll"));

        if(target<module||target>=module+MATERIAL_SIZE)return nullptr;

        auto actual=reinterpret_cast<const char*(*)(uintptr_t)>(target)(object);

        for(size_t i=0;i<sizeof(skyResourceName)-1;i++){

            char ch{};if(!Read(actual+i,&ch,1))return nullptr;

            skyResourceName[i]=ch;if(!ch)break;

        }

        if(!HazzureWorld::MaterialNameMatches(skyResourceName,name)){
            LogDebug("[SKY] Rejected unexpected/error material: %s",skyResourceName);return nullptr;
        }
        memcpy(&shared->materialSamples[4],skyResourceName,sizeof(skyResourceName));



    }

    // Retain the strong resource handle for the lifetime of this module. The

    // engine releases its temporary lookup reference before map transitions.

    if(handle) {

        InterlockedIncrement(reinterpret_cast<volatile LONG*>(static_cast<unsigned char*>(handle)+0x20));

        g_skyboxCache[index] = handle;

    }

    return handle;

}

static bool Prepare() {

    if(!arenaCtor || !loadKV || !createMaterial || !materialSystem) {

        Message("Chams passthrough (material subsystem unvalidated)");

        return false;

    }

    bool ok=false;

    __try {

        visibleMaterial=MakeMaterial("materials/hazzure/player_visible.vmat",false,false,0);

        hiddenMaterial=MakeMaterial("materials/hazzure/player_hidden.vmat",true,false,0);

        armsMaterial=MakeMaterial("materials/hazzure/arms_translucent.vmat",true,true,0);

        gradientMaterial=MakeMaterial("materials/hazzure/player_gradient.vmat",false,false,0);

        metallicVisibleMaterial=MakeMaterial("materials/hazzure/player_metal_vis.vmat",false,false,1);

        metallicHiddenMaterial=MakeMaterial("materials/hazzure/player_metal_hid.vmat",true,false,1);

        wireframeVisibleMaterial=MakeMaterial("materials/hazzure/player_wire_vis.vmat",false,false,2);

        wireframeHiddenMaterial=MakeMaterial("materials/hazzure/player_wire_hid.vmat",true,false,2);

        glowVisibleMaterial=MakeMaterial("materials/hazzure/player_glow_vis.vmat",false,true,3);

        glowHiddenMaterial=MakeMaterial("materials/hazzure/player_glow_hid.vmat",true,true,3);

        crystalVisibleMaterial=MakeMaterial("materials/hazzure/player_crystal_vis.vmat",false,true,4);

        crystalHiddenMaterial=MakeMaterial("materials/hazzure/player_crystal_hid.vmat",true,true,4);

        propMaterial=MakeMaterial("materials/hazzure/prop_fosco.vmat",false,true,0);

        ok=visibleMaterial&&hiddenMaterial&&armsMaterial;

    }__except(EXCEPTION_EXECUTE_HANDLER) { InterlockedIncrement64(&shared->faults); }

    Message(ok?"Materials created on draw callback":"Material creation failed; draw remains passthrough");

    return ok;

}

struct Backup { void* material;uint32_t colour; };

static bool Override(void* mesh,void* material,uint32_t colour,Backup* backup) {

    __try {

        auto p=static_cast<unsigned char*>(mesh);

        backup->material=*reinterpret_cast<void**>(p+0x20);

        backup->colour=*reinterpret_cast<uint32_t*>(p+0x50);

        *reinterpret_cast<void**>(p+0x20)=material;

        *reinterpret_cast<uint32_t*>(p+0x50)=colour;

        return true;

    }__except(EXCEPTION_EXECUTE_HANDLER) { return false; }

}

static void Restore(void* mesh,const Backup* backup) {

    __try {

        auto p=static_cast<unsigned char*>(mesh);

        *reinterpret_cast<void**>(p+0x20)=backup->material;

        *reinterpret_cast<uint32_t*>(p+0x50)=backup->colour;

    }__except(EXCEPTION_EXECUTE_HANDLER) { failed=true;InterlockedIncrement64(&shared->faults); }

}

// Gradient chams. The element is drawn twice: once with the top colour and once with the

// bottom colour through the shared translucent material, so the overlap reads as a

// vertical fade. A true per-vertex gradient would need the vertex colour stream, which

// this module does not own. The colour is interpolated by the mesh height, so the top of

// the model is the tint the operator picked and the feet are the second colour.

