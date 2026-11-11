static volatile LONG testCalls;

__declspec(noinline) static void SyntheticDraw(void* a,void* b,void* c,int d,void* e,void* f,void* g,void* h) {

    if(a!=reinterpret_cast<void*>(1)||b!=reinterpret_cast<void*>(2)||c!=reinterpret_cast<void*>(3)||d!=4||e!=reinterpret_cast<void*>(5)||f!=reinterpret_cast<void*>(6)||g!=reinterpret_cast<void*>(7)||h!=reinterpret_cast<void*>(8))testCalls=-100;

    else InterlockedIncrement(&testCalls);

}

static Draw testOriginal;
static volatile LONG cameraTestCalls;
__declspec(noinline) static float SyntheticCamera(void* services){
    InterlockedIncrement(&cameraTestCalls);
    return services==reinterpret_cast<void*>(0x1234)?91.75f:-1.f;
}

static volatile LONG nativeViewTestCalls,nativeDrawTestCalls;
__declspec(noinline) static void SyntheticNativeView(void* mode,void* view){
    if(mode==reinterpret_cast<void*>(0x1234)&&view)InterlockedIncrement(&nativeViewTestCalls);
    *static_cast<int*>(view)=319;
}
__declspec(noinline) static bool SyntheticNativeBody(void* mode,void* entity){
    if(mode==reinterpret_cast<void*>(0x1234))InterlockedIncrement(&nativeDrawTestCalls);
    return entity==reinterpret_cast<void*>(0x5678);
}

static void SyntheticHook(void* a,void* b,void* c,int d,void* e,void* f,void* g,void* h) {

    InterlockedIncrement(&testCalls);testOriginal(a,b,c,d,e,f,g,h);

}

int main() {
    HazzureWorld::ready.store(true);

    float exposureSource[4]={0,0,1,1},exposureResult[4]{};

    if(!ExposureValues(exposureSource,.25f,exposureResult)||exposureResult[0]!=-2.f||exposureResult[1]!=-2.f||exposureResult[2]!=.25f||exposureResult[3]!=.25f||exposureSource[2]!=1.f)return 40;

    if(ExposureValues(exposureSource,0.f,exposureResult)||ExposureValues(exposureSource,2.f,exposureResult))return 41;

    exposureSource[0]=NAN;if(ExposureValues(exposureSource,.25f,exposureResult))return 42;

    puts("PASS: exposure scales linear and log bounds consistently, preserves source and rejects invalid inputs.");

    Shared ipc{};shared=&ipc;

    ipc.revision=2;ipc.skinEnabled=1;ipc.skins[0]={7,180,200,0,0.15f,1,1,1};

    if(!SnapshotSettings()||skinRevision!=2||!skinOn||skinSettings[0].paint!=180)return 20;

    ipc.revision=3;ipc.skins[0].paint=302;

    if(SnapshotSettings()||skinSettings[0].paint!=180)return 21;

    ipc.revision=4;ipc.skins[0].wear=-1;

    if(SnapshotSettings()||skinRevision!=2)return 22;

    if(SubclassToken(7)!=0x85AD0A57||SubclassToken(42)!=0x1193F9E4||SubclassToken(515)!=0x50AFF643)return 23;

    if(!Glove(5027)||!Glove(5030)||!Glove(4725)||Glove(7)||Glove(500))return 24;

    if(GlovePath(5027)==nullptr||GlovePath(4725)==nullptr||GlovePath(7)!=nullptr)return 25;

    if(AgentPathCT(1)==nullptr||AgentPathCT(10)==nullptr||AgentPathCT(11)!=nullptr||AgentPathCT(0)!=nullptr)return 26;

    if(AgentPathT(1)==nullptr||AgentPathT(15)==nullptr||AgentPathT(0)!=nullptr)return 27;

    puts("PASS: skin configuration rejects torn/invalid snapshots; measured subclass hashes; IPC offsets.");

    // Gradient colour mixing is pure arithmetic, so it is checked exactly: the endpoints

    // must reproduce the inputs and the midpoint must sit between them per channel.

    if(MixColour(0xFFFFFFFFu,0xFF000000u,0.f)!=0xFF000000u)return 30;

    if(MixColour(0xFFFFFFFFu,0xFF000000u,1.f)!=0xFFFFFFFFu)return 31;

    if(MixColour(0xFFFFFFFFu,0xFF000000u,0.5f)!=0xFF7F7F7Fu)return 32;

    // Out-of-range factors clamp instead of wrapping a channel.

    if(MixColour(0xFFFFFFFFu,0xFF000000u,-4.f)!=0xFF000000u)return 33;

    if(MixColour(0xFFFFFFFFu,0xFF000000u,4.f)!=0xFFFFFFFFu)return 34;

    // A mesh without a readable scene must decline the gradient rather than paint a

    // guessed colour over the whole element.

    unsigned char unreadable[0x70]{};

    if(NormalisedHeight(unreadable)>=0.f)return 35;

    puts("PASS: gradient colour mixing clamps and reproduces the endpoints; unreadable bounds decline the pass.");

    unsigned char mesh[0x70]{};

    *reinterpret_cast<void**>(mesh+0x20)=reinterpret_cast<void*>(0x1234);

    *reinterpret_cast<uint32_t*>(mesh+0x50)=0x12345678;

    Backup saved{};

    if(!Override(mesh,reinterpret_cast<void*>(0x5678),0xabcdefab,&saved))return 1;

    if(*reinterpret_cast<void**>(mesh+0x20)!=reinterpret_cast<void*>(0x5678)||*reinterpret_cast<uint32_t*>(mesh+0x50)!=0xabcdefab)return 2;

    Restore(mesh,&saved);

    if(*reinterpret_cast<void**>(mesh+0x20)!=reinterpret_cast<void*>(0x1234)||*reinterpret_cast<uint32_t*>(mesh+0x50)!=0x12345678)return 3;

    if(Override(nullptr,nullptr,0,&saved))return 4;

    if(MH_Initialize()!=MH_OK)return 5;

    if(MH_CreateHook(SyntheticDraw,SyntheticHook,reinterpret_cast<void**>(&testOriginal))!=MH_OK)return 6;

    if(MH_EnableHook(SyntheticDraw)!=MH_OK)return 7;

    volatile Draw call=SyntheticDraw;

    call(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),4,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6),reinterpret_cast<void*>(7),reinterpret_cast<void*>(8));

    if(testCalls!=2)return 8;

    if(MH_DisableHook(SyntheticDraw)!=MH_OK||MH_RemoveHook(SyntheticDraw)!=MH_OK||MH_Uninitialize()!=MH_OK)return 9;

    call(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),4,reinterpret_cast<void*>(5),reinterpret_cast<void*>(6),reinterpret_cast<void*>(7),reinterpret_cast<void*>(8));

    if(testCalls!=3)return 10;

    if(MH_Initialize()!=MH_OK)return 80;
    if(MH_CreateHook(SyntheticCamera,CameraFovHook,reinterpret_cast<void**>(&originalCamera))!=MH_OK)return 81;
    if(MH_EnableHook(SyntheticCamera)!=MH_OK)return 82;
    volatile CameraGetter cameraCall=SyntheticCamera;
    if(cameraCall(reinterpret_cast<void*>(0x1234))!=91.75f||cameraTestCalls!=1)return 83;
    if(MH_DisableHook(SyntheticCamera)!=MH_OK||MH_RemoveHook(SyntheticCamera)!=MH_OK||MH_Uninitialize()!=MH_OK)return 84;
    if(cameraCall(reinterpret_cast<void*>(0x1234))!=91.75f||cameraTestCalls!=2)return 85;
    originalCamera=nullptr;
    puts("PASS: typed camera getter hook preserves the pointer argument, native call and float return; removed in own process.");

    if(MH_Initialize()!=MH_OK)return 140;
    if(MH_CreateHook(SyntheticNativeView,NativeViewHook,reinterpret_cast<void**>(&originalViewSetup))!=MH_OK||
       MH_CreateHook(SyntheticNativeBody,NativeDrawEntityHook,reinterpret_cast<void**>(&originalDrawEntity))!=MH_OK)return 141;
    if(MH_EnableHook(SyntheticNativeView)!=MH_OK||MH_EnableHook(SyntheticNativeBody)!=MH_OK)return 142;
    volatile NativeViewSetup viewCall=SyntheticNativeView;volatile NativeDrawEntity bodyCall=SyntheticNativeBody;
    int viewResult{};viewCall(reinterpret_cast<void*>(0x1234),&viewResult);
    if(viewResult!=319||nativeViewTestCalls!=1||!bodyCall(reinterpret_cast<void*>(0x1234),reinterpret_cast<void*>(0x5678))||
       bodyCall(reinterpret_cast<void*>(0x1234),reinterpret_cast<void*>(0x1111))||nativeDrawTestCalls!=2)return 143;
    if(MH_DisableHook(SyntheticNativeView)!=MH_OK||MH_DisableHook(SyntheticNativeBody)!=MH_OK||
       MH_RemoveHook(SyntheticNativeView)!=MH_OK||MH_RemoveHook(SyntheticNativeBody)!=MH_OK||MH_Uninitialize()!=MH_OK)return 144;
    originalViewSetup=nullptr;originalDrawEntity=nullptr;
    puts("PASS: native view void(this,view) and body bool(this,entity) hooks preserve both arguments and native results in own process.");

    if(!IsLocalHeadshot("player_hurt",1,1,123,456,123))return 30;

    if(!IsLocalKill("player_death",123,456,123))return 32;

    if(IsLocalKill("player_hurt",123,456,123)||IsLocalKill("player_death",789,456,123)||IsLocalKill("player_death",123,123,123))return 33;

    if(IsLocalHeadshot("player_hurt",2,20,123,456,123)||IsLocalHeadshot("player_hurt",1,20,789,456,123)||IsLocalHeadshot("player_death",1,20,123,456,123)||IsLocalHeadshot("player_hurt",1,0,123,456,123)||IsLocalHeadshot("player_hurt",1,20,123,123,123))return 31;

    puts("PASS: local non-lethal headshot accepted; body, other attacker, death, zero damage and self damage rejected.");

    // REGRESSION TEST 1: Schema resolver & degradation

    HazzureSchema::InitializeSchemaResolver();

    if (!HazzureSchema::IsGlowValid() || HazzureSchema::g_mGlow.offset < 0x100) return 50;

    if (!HazzureSchema::IsShadowValid() || HazzureSchema::g_mMaxShadowDist.offset < 0x100) return 51;

    if (!HazzureSchema::IsPingValid() || HazzureSchema::g_mPing.offset < 0x100) return 52;

    puts("PASS: schema fallback offsets remain bounded in the isolated test process.");



    // REGRESSION TEST 2: StateRegistry & product unload tracking

    HazzureState::TrackGlow(0xABCDEF);

    if (HazzureState::g_glowingPawns.empty()) return 60;

    HazzureState::ResetAllGlow(0);

    if (!HazzureState::g_glowingPawns.empty()) return 61;

    HazzureState::PerformProductUnload(shared, 0);

    if (!HazzureState::g_unloading.load()) return 62;

    puts("PASS: state registry clears synthetic tracking and suspends resident feature state.");



    // REGRESSION TEST 3: Telemetry & humanized trigger bounds

    HazzureTelemetry::RecordTriggerClick(350);

    if (HazzureTelemetry::g_metrics.triggerClicksTotal < 1 || HazzureTelemetry::g_metrics.triggerLastDelayMs != 350) return 70;

    std::mt19937 rng(1337);

    HazzureCombatRefine::TriggerTiming triggerTiming;

    for (int it = 0; it < 1000; ++it) {

        triggerTiming.Ready(static_cast<uintptr_t>(it+1),1,HazzureCombatRefine::TriggerMode::Balanced,
                            1000,HazzureCombatRefine::BalancedWindow(),rng);
        int d = static_cast<int>(triggerTiming.delay);

        if (d < 40 || d > 90) return 71;

    }

    puts("PASS: feature telemetry tracks real events; integrated trigger acquisition samples stay within [40, 90] ms.");



    puts("PASS: material and RGBA restoration; invalid mesh rejection; x64 eight-argument trampoline, original call and hook removal in own process.");return 0;

}

