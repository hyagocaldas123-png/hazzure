// C++ / panel-render-tests.cpp / MSVC x64 C++17 / isolated D3D11 WARP UI rendering
#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <windows.h>
#include <d3d11.h>
#include <wincodec.h>
#include <filesystem>
#include <stdexcept>
#include <cstdio>
#include "../src/HazzureUI.h"
#include "../../HazzurePanel/third_party/imgui/imgui_internal.h"
#include "../../HazzurePanel/third_party/imgui/imgui_impl_dx11.h"
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dwmapi.lib")

static void Require(HRESULT result,const char* operation){if(FAILED(result)){printf("FAIL: %s (0x%08lX)\n",operation,static_cast<unsigned long>(result));throw std::runtime_error(operation);}}
template<class T> struct Com {
    T* p{};~Com(){if(p)p->Release();}T** Address(){return &p;}T* operator->(){return p;}
};
static void SavePng(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Texture2D* source,const std::filesystem::path& file){
    D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Com<ID3D11Texture2D> staging;Require(device->CreateTexture2D(&desc,nullptr,staging.Address()),"staging texture");
    context->CopyResource(staging.p,source);
    D3D11_MAPPED_SUBRESOURCE pixels{};Require(context->Map(staging.p,0,D3D11_MAP_READ,0,&pixels),"readback");
    try{
        Com<IWICImagingFactory> factory;Require(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.Address())),"WIC factory");
        Com<IWICStream> stream;Require(factory->CreateStream(stream.Address()),"WIC stream");
        Require(stream->InitializeFromFilename(file.c_str(),GENERIC_WRITE),"PNG path");
        Com<IWICBitmapEncoder> encoder;Require(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.Address()),"PNG encoder");
        Require(encoder->Initialize(stream.p,WICBitmapEncoderNoCache),"PNG initialize");
        Com<IWICBitmapFrameEncode> frame;Com<IPropertyBag2> properties;
        Require(encoder->CreateNewFrame(frame.Address(),properties.Address()),"PNG frame");
        Require(frame->Initialize(properties.p),"frame initialize");Require(frame->SetSize(desc.Width,desc.Height),"frame dimensions");
        WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;Require(frame->SetPixelFormat(&format),"pixel format");
        if(format!=GUID_WICPixelFormat32bppBGRA)throw std::runtime_error("unexpected PNG format");
        Require(frame->WritePixels(desc.Height,pixels.RowPitch,pixels.RowPitch*desc.Height,static_cast<BYTE*>(pixels.pData)),"PNG pixels");
        Require(frame->Commit(),"PNG frame commit");Require(encoder->Commit(),"PNG commit");
    }catch(...){context->Unmap(staging.p,0);throw;}
    context->Unmap(staging.p,0);
}
int wmain(int argc,wchar_t** argv){
    if(argc!=2){puts("Usage: panel-render-tests.exe OUTPUT_DIRECTORY");return 2;}
    try{
        std::filesystem::path output=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(output);
        Require(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"COM initialize");
        Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;
        Require(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,device.Address(),nullptr,context.Address()),"WARP device");
        HazzurePreviewAssets::SetDevice(device.p);
        HazzurePreviewAssets::SetRoot(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"Hazzure/assets");
        auto skinTexture=HazzurePreviewAssets::Get("skins/16_309.png");
        auto agentTexture=HazzurePreviewAssets::Get("agents/ct_1.png");
        if(!skinTexture.view||!agentTexture.view||skinTexture.view==agentTexture.view||!skinTexture.width||!agentTexture.height)
            throw std::runtime_error("selected weapon and agent catalogue images did not decode into separate textures");
        if(!HazzurePreviewAssets::Find("../invalid.png").empty())throw std::runtime_error("preview path traversal accepted");
        HazzurePreviewAssets::SetDevice(nullptr);
        HazzurePreviewAssets::SetDevice(device.p);
        if(!HazzurePreviewAssets::Get("agents/ct_1.png").view)throw std::runtime_error("preview texture did not recover after device recreation");
        puts("PASS: real weapon and model-matched agent PNGs decode, reject foreign paths and recover after device recreation");
        ImGui::CreateContext();ImGuiIO& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DeltaTime=1.f/60.f;
        ImGui::StyleColorsDark();if(!ImGui_ImplDX11_Init(device.p,context.p))throw std::runtime_error("ImGui DX11 init");
        Shared settings{};settings.magic=0x485a4931;settings.version=2;settings.uiConfigOwner=1;
        HazzureUI::g_settingsShared=&settings;HazzureUI::InitializeContextDefaults();
        if(!HazzureUI::g_cfg.humanizedTrigger||HazzureUI::g_cfg.triggerDistanceDelay||HazzureUI::g_cfg.triggerDelayMs!=65)
            throw std::runtime_error("balanced trigger is not the default mode");
        if(!HazzureUI::ApplySharedConfig(&settings))throw std::runtime_error("initial UI config publication");
        auto wave=HazzureUI::SynthesizePcmTone(1950.f,1350.f,.085f,22.f);
        HazzureUI::WavPcmHeader header{};memcpy(&header,wave.data(),sizeof(header));
        if(memcmp(header.riff,"RIFF",4)||memcmp(header.wave,"WAVE",4)||memcmp(header.fmt,"fmt ",4)||
           header.sampleRate!=44100||header.bitsPerSample!=16||header.subchunk2Size+sizeof(header)!=wave.size())
            throw std::runtime_error("invalid PCM WAV header");
        HazzureUI::PlayPcmBuffer(wave,.25f);
        for(size_t offset=sizeof(header);offset+2<=wave.size();offset+=2){
            int16_t original{},scaled{};memcpy(&original,wave.data()+offset,2);memcpy(&scaled,HazzureUI::g_playbackWav.data()+offset,2);
            if(scaled!=static_cast<int16_t>(std::lround(original*.25f)))throw std::runtime_error("PCM volume mismatch");
        }
        HazzureUI::EnsureProceduralAudio();
        for(int preset=0;preset<5;++preset)for(int head=0;head<2;++head)
            if(!HazzureCustomAudio::Canonical(HazzureUI::g_hitPresets[preset][head]))throw std::runtime_error("invalid preset PCM");
        for(int preset=1;preset<5;++preset)
            if(HazzureUI::g_hitPresets[preset][0]==HazzureUI::g_hitPresets[preset-1][0])throw std::runtime_error("identical hit presets");
        puts("PASS: initialized WAV header, quarter-volume gain and ten original preset variants");
        HazzureUI::g_cfg.skinsEnabled=true;
        settings.skinEnabled=1;
        if(!HazzureUI::ApplySharedConfig(&settings)||settings.skinEnabled)
            throw std::runtime_error("retired skinchanger publication reactivated");
        puts("PASS: retired skinchanger stays disabled on publication");
        HazzureUI::g_cfg.triggerPauseMs=475;
        HazzureUI::Persistence::storagePath="isolated-share-test.json";
        std::string shareError,shareCode;
        if(!HazzureUI::Persistence::storage.Parse("{\"extension\":\"kept\"}",shareError))throw std::runtime_error("share fixture parse");
        HazzureUI::g_cfg.aimBezier=true;HazzureUI::g_cfg.aimReactionDelay=true;
        HazzureUI::g_cfg.aimEase=true;HazzureUI::g_cfg.aimEaseBias=.27f;
        HazzureUI::g_cfg.cameraFovOverride=true;HazzureUI::g_cfg.cameraFov=117.5f;
        HazzureUI::g_cfg.rcsBulletDelay=3;HazzureUI::g_cfg.rcsRampBullets=4;HazzureUI::g_cfg.rcsStrength=.93f;
        HazzureUI::g_cfg.espWeaponIcons=true;HazzureUI::g_cfg.triggerDistanceDelay=true;
        HazzureUI::g_cfg.armory[2].definition=7;HazzureUI::g_cfg.armory[2].stickers=true;
        HazzureUI::g_cfg.armory[2].slots[4].id=2;HazzureUI::g_cfg.armory[2].nametag=true;
        strcpy_s(HazzureUI::g_cfg.armory[2].name,"Perfil Armory");
        HazzureUI::g_cfg.soundEsp=HazzureUI::g_cfg.dormantFade=HazzureUI::g_cfg.edgeJump=true;
        if(!HazzureUI::ExportShareCode(shareCode))throw std::runtime_error("share export");
        HazzureUI::g_cfg.aimBezier=false;
        HazzureUI::g_cfg.aimEase=false;HazzureUI::g_cfg.rcsBulletDelay=0;HazzureUI::g_cfg.rcsStrength=1.f;
        HazzureUI::g_cfg.cameraFovOverride=false;HazzureUI::g_cfg.cameraFov=90.f;
        HazzureUI::g_cfg.espWeaponIcons=false;HazzureUI::g_cfg.triggerDistanceDelay=false;
        if(!HazzureUI::ImportShareCode(shareCode)||!HazzureUI::g_cfg.aimBezier||!HazzureUI::g_cfg.aimReactionDelay)throw std::runtime_error("share profile roundtrip");
        if(!HazzureUI::g_cfg.cameraFovOverride||HazzureUI::g_cfg.cameraFov!=117.5f)throw std::runtime_error("camera FOV enabled/value share roundtrip");
        if(HazzureUI::g_cfg.armory[2].slots[4].id!=2||strcmp(HazzureUI::g_cfg.armory[2].name,"Perfil Armory")||
           !HazzureUI::g_cfg.soundEsp||!HazzureUI::g_cfg.dormantFade||!HazzureUI::g_cfg.edgeJump)throw std::runtime_error("combat/Armory share roundtrip");
        if(HazzureUI::g_cfg.triggerPauseMs!=475||!HazzureUI::g_cfg.aimEase||std::fabs(HazzureUI::g_cfg.aimEaseBias-.27f)>.0001f||
           HazzureUI::g_cfg.rcsBulletDelay!=3||HazzureUI::g_cfg.rcsRampBullets!=4||std::fabs(HazzureUI::g_cfg.rcsStrength-.93f)>.0001f||
           !HazzureUI::g_cfg.espWeaponIcons||!HazzureUI::g_cfg.triggerDistanceDelay)throw std::runtime_error("refinements share roundtrip");
        auto extension=HazzureConfig::Find(HazzureUI::Persistence::storage.Root(),"extension");
        if(!extension||!extension->IsString()||strcmp(extension->GetString(),"kept"))throw std::runtime_error("share unknown keys");
        settings.revision|=1;HazzureUI::g_cfg.aimBezier=false;
        if(HazzureUI::ImportShareCode(shareCode)||HazzureUI::g_cfg.aimBezier)throw std::runtime_error("busy share import changed live state");
        settings.revision=(settings.revision+1)&~1;
        std::string invalidCode;
        HazzureShare::Encode("{\"aimBezier\":123}",invalidCode,shareError);
        if(HazzureUI::ImportShareCode(invalidCode)||HazzureUI::g_cfg.aimBezier)throw std::runtime_error("invalid share schema changed state");
        HazzureShare::Encode("{\"rcsBulletDelay\":-1,\"aimEaseBias\":1.5}",invalidCode,shareError);
        if(HazzureUI::ImportShareCode(invalidCode)||HazzureUI::g_cfg.rcsBulletDelay!=3)throw std::runtime_error("invalid refinement schema changed state");
        HazzureUI::g_cfg.humanizedTrigger=true;HazzureUI::g_cfg.triggerDistanceDelay=false;
        if(!HazzureUI::ExportShareCode(shareCode))throw std::runtime_error("balanced trigger export");
        HazzureUI::g_cfg.humanizedTrigger=false;HazzureUI::g_cfg.triggerDistanceDelay=true;
        if(!HazzureUI::ImportShareCode(shareCode)||!HazzureUI::g_cfg.humanizedTrigger||HazzureUI::g_cfg.triggerDistanceDelay||
           HazzureUI::g_cfg.triggerPauseMs!=475)throw std::runtime_error("balanced trigger share lost mode or altered recovery pause");
        HazzureUI::OverlayConfig legacy{};HazzureConfig::Document legacyDocument;
        if(!legacyDocument.Parse("{\"humanizedTrigger\":false,\"triggerDistanceDelay\":true,\"triggerDelayMs\":20}",shareError)||
           !HazzureUI::Persistence::ReadConfig(legacyDocument.Root(),legacy,shareError)||legacy.humanizedTrigger||legacy.triggerDelayMs!=20)
            throw std::runtime_error("legacy manual trigger profile no longer loads");
        puts("PASS: balanced trigger default, persistence, share roundtrip, recovery separation and legacy profile compatibility");
        puts("PASS: complete share-code roundtrip, unknown keys, busy IPC and invalid schema rollback; no clipboard or files written");
        HazzureUI::Persistence::storage=HazzureConfig::Document{};HazzureUI::Persistence::storagePath.clear();HazzureUI::Persistence::Status("Pronto");
        HazzureUI::g_cfg=HazzureUI::OverlayConfig{};
        {
            auto& config=HazzureUI::g_cfg;config.hitchanceEnabled=config.aimMultipoint=config.aimBodyFallback=config.shotStop=config.soundRadar=config.lineupHelper=config.grenadeZones=true;
            config.hitchancePercent=73;config.hitchanceSamples=256;config.multipointScale=.7f;config.bodyFallbackBelow=35;config.shotStopSpeed=68;
            auto& library=config.lineups;strcpy_s(library.map,"de_mirage");library.count=1;
            auto& entry=library.entries[0];strcpy_s(entry.map,"de_mirage");strcpy_s(entry.name,"Treino");entry.position={-123,250,80};entry.angles={-30,170,0};
            HazzureConfig::Document document;std::string error;
            if(!HazzureUI::Persistence::WriteConfig(document,config,{}, {}, {},-1,error))throw std::runtime_error("new controls serialization");
            HazzureUI::OverlayConfig restored{};
            if(!HazzureUI::Persistence::ReadConfig(document.Root(),restored,error)||!restored.hitchanceEnabled||!restored.aimMultipoint||
               !restored.aimBodyFallback||!restored.shotStop||!restored.soundRadar||!restored.lineupHelper||!restored.grenadeZones||restored.hitchancePercent!=73||
               restored.hitchanceSamples!=256||restored.multipointScale!=.7f||restored.bodyFallbackBelow!=35||restored.shotStopSpeed!=68||
               restored.lineups.count!=1||restored.lineups.entries[0].position.x!=-123)throw std::runtime_error("new controls roundtrip");
            auto previous=config;HazzureConfig::Set(document.Root(),"hitchancePercent",101,document.Allocator());
            if(HazzureUI::Persistence::ReadConfig(document.Root(),restored,error))throw std::runtime_error("invalid chance accepted");
            config=previous;
            alignas(16) unsigned char boneData[65*32]{};
            HazzureGame::Vec3 head{1000,0,64},neck{1000,0,59},chest{1000,0,51},spine{1000,0,39},pelvis{1000,0,30};
            memcpy(boneData+6*32,&head,sizeof(head));memcpy(boneData+5*32,&neck,sizeof(neck));memcpy(boneData+4*32,&chest,sizeof(chest));
            memcpy(boneData+2*32,&spine,sizeof(spine));memcpy(boneData,&pelvis,sizeof(pelvis));
            HazzureGame::Pawn local{};local.address=1;local.eye={0,0,64};local.anglesValid=true;
            HazzureWeaponAccuracy::Snapshot weapon{1,2,3,100,.02f,.005f,250,8192,0,1,true};
            HazzureUI::Vec3 chosen{};int bone{};float angle{};
            config.aimMultipoint=false;config.aimBodyFallback=true;config.bodyFallbackBelow=90;
            HazzureVisibility::Clear();
            if(HazzureUI::SelectAccuracyPoint(1,4,local,reinterpret_cast<uintptr_t>(boneData),weapon,"rifles",25,chosen,bone,angle))
                throw std::runtime_error("unconfirmed visibility selected an accuracy point");
            for(auto& entry:HazzureVisibility::entries)if(entry.requested.target){entry.evaluated=entry.requested;entry.completed=GetTickCount64();entry.verdict=HazzureVisibility::Verdict::Visible;entry.pending=false;}
            if(!HazzureUI::SelectAccuracyPoint(1,4,local,reinterpret_cast<uintptr_t>(boneData),weapon,"rifles",25,chosen,bone,angle)||
               (bone!=HazzureUI::BONE_SPINE_2&&bone!=HazzureUI::BONE_PELVIS))throw std::runtime_error("body fallback integration");
            weapon.valid=false;
            if(HazzureUI::SelectAccuracyPoint(1,4,local,reinterpret_cast<uintptr_t>(boneData),weapon,"rifles",25,chosen,bone,angle))
                throw std::runtime_error("unavailable weapon data selected an accuracy point");
            HazzureVisibility::Clear();
            puts("PASS: accuracy/stop/radial/lineup controls persist; body fallback requires confirmed visibility and valid native data");
        }
        HazzureUI::g_cfg=HazzureUI::OverlayConfig{};
        if(!HazzureUI::ApplySharedConfig(&settings))throw std::runtime_error("preview reset");
        {
            auto config=HazzureUI::OverlayConfig{};config.autoStrafePhysics=config.jumpBug=config.edgeBug=config.pixelSurf=true;
            config.shotFullStop=config.lineupAutoAlign=config.poseHistory=true;config.poseHistoryMs=180;
            HazzureLineupCatalogue::Merge(config.lineups);
            HazzureConfig::Document document;std::string error;
            if(!HazzureUI::Persistence::WriteConfig(document,config,{}, {}, {},-1,error))throw std::runtime_error("advanced controls write");
            auto restored=HazzureUI::OverlayConfig{};
            if(!HazzureUI::Persistence::ReadConfig(document.Root(),restored,error)||!restored.autoStrafePhysics||!restored.jumpBug||!restored.edgeBug||
               !restored.pixelSurf||!restored.shotFullStop||!restored.lineupAutoAlign||!restored.poseHistory||restored.poseHistoryMs!=180||
               restored.lineups.count!=4||restored.lineups.entries[0].validated)throw std::runtime_error("advanced controls roundtrip");
            puts("PASS: advanced movement, full-stop, map/lineup metadata and pose-history settings survive typed JSON roundtrip");
        }
        const wchar_t* names[]={L"legitbot",L"visuals",L"misc",L"skins",L"configs",L"skins-detail",L"misc-audio"};
        constexpr int panelPages[]={0,1,2,3,4,3,2};
        HazzureUI::g_cfg.weaponIndex=5;HazzureUI::g_cfg.skins[2]={16,309,777,0,.123f,1,1,0};
        auto preview=HazzureUI::SelectedEquipmentPreview();
        if(preview.definition!=16||preview.paint!=309||preview.seed!=777||std::fabs(preview.wear-.123f)>.00001f)
            throw std::runtime_error("preview does not follow selected weapon and finish");
        HazzureUI::g_cfg.weaponIndex=2;auto changedPreview=HazzureUI::SelectedEquipmentPreview();
        if(changedPreview.definition!=9||!strcmp(preview.name,changedPreview.name))throw std::runtime_error("weapon preview remained on previous selection");
        HazzureUI::g_cfg.weaponIndex=5;settings.skins[2]={16,309,33,0,.25f,1,1,0};
        auto sharedPreview=HazzureUI::SelectedEquipmentPreview(0,&settings);
        if(sharedPreview.paint!=309||sharedPreview.seed!=33)throw std::runtime_error("preview diverges from the paint displayed by shared selectors");
        if(strcmp(HazzureUI::SelectedAgentPreview(true,1).model,"agents/models/ctm_fbi/ctm_fbi_variantb.vmdl")||
           strcmp(HazzureUI::SelectedAgentPreview(false,1).model,"agents/models/tm_professional/tm_professional_varf.vmdl")||
           strcmp(HazzureUI::SelectedAgentPreview(true,5).name,"Sous-Lieutenant Medic | Gendarmerie Nationale"))
            throw std::runtime_error("agent preview is not mapped to the actual selected model");
        puts("PASS: selected weapon/finish/seed and CT/T agent model identity drive the previews");
        for(auto size:{std::pair<int,int>{1024,720},std::pair<int,int>{1920,1080},std::pair<int,int>{320,240}}){
            D3D11_TEXTURE2D_DESC desc{};desc.Width=size.first;desc.Height=size.second;desc.MipLevels=1;desc.ArraySize=1;
            desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
            Com<ID3D11Texture2D> texture;Require(device->CreateTexture2D(&desc,nullptr,texture.Address()),"render texture");
            Com<ID3D11RenderTargetView> target;Require(device->CreateRenderTargetView(texture.p,nullptr,target.Address()),"render target");
            io.DisplaySize=ImVec2(static_cast<float>(size.first),static_cast<float>(size.second));
            for(int tab=size.first<800?9:0;tab<(size.first<800?13:35);++tab){
                HazzureUI::g_currentTab=tab<7?panelPages[tab]:0;bool open=true;
                if(tab==5)HazzureUI::g_panelSections[3]=1;
                if(tab==6)HazzureUI::g_panelSections[2]=3;
                HazzureUI::g_cfg.aimBezier=HazzureUI::g_cfg.aimDistanceFov=HazzureUI::g_cfg.aimReactionDelay=tab==7;
                HazzureUI::g_cfg.showKeybindsOverlay=tab==8;
                if(tab==5){HazzureUI::g_cfg.weaponIndex=5;
                    if(!HazzureUI::ApplySharedConfig(&settings)||settings.skinEnabled)throw std::runtime_error("retired skin activation in fixture");}
                for(int frame=0;frame<3;++frame){
                    const float background[]={.07f,.08f,.10f,1.f};context->ClearRenderTargetView(target.p,background);
                    ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
                    if(tab<8)HazzureUI::DrawPanel(&settings,&open,size.first,size.second);
                    else if(tab==8)HazzureUI::RenderKeybindsOverlay(size.first);
                    else if(tab>=9&&tab<=12){
                        HazzureUI::PlantedC4Data bomb{};bomb.isTicking=true;bomb.site=1;
                        bomb.remaining=tab==9?7.3f:18.7f;bomb.beingDefused=tab==10;
                        bomb.defuseLength=5; bomb.defuseLeft=3.2f;bomb.isDefused=tab==12;
                        HazzureUI::DrawBombTimerHUD(bomb,{tab!=11,static_cast<uint8_t>(tab==9?104:27)},100,size.first);
                        HazzureTactical::Spectators specs{};specs.count=tab==11?64:2;
                        for(size_t i=0;i<specs.count;++i){sprintf_s(specs.entries[i].name,"Nome longo de espectador #%zu com caracteres extras",i+1);specs.entries[i].mode=i%2?5:4;}
                        HazzureUI::DrawSpectatorsHUD(specs,size.first,size.second);
                    }else if(tab>=13&&tab<=15){
                        HazzureUI::g_cfg.soundEsp=HazzureUI::g_cfg.worldDamage=HazzureUI::g_cfg.bulletTracers=HazzureUI::g_cfg.impactBoxes=true;
                        HazzureUI::g_cfg.espMaxDistance=20000;
                        HazzureUI::Matrix4 matrix{};matrix.m[0][1]=-1.3f;matrix.m[1][2]=1.732f;matrix.m[1][3]=-64.f*1.732f;matrix.m[3][0]=1;
                        auto* draw=ImGui::GetBackgroundDrawList();
                        for(int x=100;x<=1000;x+=100)HazzureUI::CombatLine(draw,{static_cast<float>(x),-400,0},{static_cast<float>(x),400,0},matrix,size.first,size.second,IM_COL32(45,50,60,255),1);
                        for(int y=-400;y<=400;y+=100)HazzureUI::CombatLine(draw,{100,static_cast<float>(y),0},{1000,static_cast<float>(y),0},matrix,size.first,size.second,IM_COL32(45,50,60,255),1);
                        if(tab==13){HazzureCombat::effects.Clear();
                            HazzureCombat::effects.Add({HazzureCombat::Kind::Footstep,700,1,0,{300,-100,0}});
                            HazzureCombat::effects.Add({HazzureCombat::Kind::Reload,650,2,0,{450,100,0}});
                            HazzureCombat::effects.Add({HazzureCombat::Kind::Damage,650,1,2,{350,0,90},{},27,false});
                            HazzureCombat::effects.Add({HazzureCombat::Kind::Damage,750,1,2,{500,90,110},{},104,true});
                            HazzureCombat::effects.Add({HazzureCombat::Kind::Impact,700,1,0,{550,-190,70},{150,-30,40}});
                            HazzureUI::DrawCombatEffects(draw,matrix,{},size.first,size.second,1000);
                        }else{
                            HazzureGame::Pawn local{};local.health=100;local.team=2;local.eye={0,0,64};local.anglesValid=true;
                            for(int i=0;i<3;++i){HazzureCombat::Ghost ghost{};ghost.handle=i+1;ghost.team=3;ghost.health=73;
                                ghost.origin=tab==14?HazzureGame::Vec3{400,static_cast<float>((i-1)*160),0}:HazzureGame::Vec3{-100,static_cast<float>((i-1)*200),0};
                                ghost.head={ghost.origin.x,ghost.origin.y,72};strcpy_s(ghost.name,"Ultimo local conhecido");strcpy_s(ghost.weapon,"AK-47");
                                ghost.bones[0]={{ghost.origin.x,ghost.origin.y,64},{ghost.origin.x,ghost.origin.y,30}};ghost.boneCount=1;
                                HazzureUI::DrawCombatGhost(draw,ghost,local,matrix,size.first,size.second,tab==14?1.f-i*.35f:1.f,"rifles");}
                        }
                    }else if(tab>=16&&tab<=19){
                        HazzureUI::g_cfg.skins[2]={7,180,1,0,.05f,1,1,0};auto& item=HazzureUI::g_cfg.armory[2];
                        item.definition=7;item.stickers=item.charm=item.stat=item.nametag=true;item.charmId=1;item.charmSeed=777;item.counter=tab+100;
                        item.slots[0].id=1;item.slots[0].rotation=72;item.slots[0].wear=.25f;strcpy_s(item.name,"Minha AK-47");
                        HazzureUI::g_armoryPage=tab-16;HazzureArmory::Publish(HazzureUI::g_cfg.armory);
                        ImGui::SetNextWindowPos({(size.first-360.f)*.5f,30});ImGui::SetNextWindowSize({360.f,size.second-60.f});
                        ImGui::Begin("Armory - AK-47",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
                        HazzureUI::BeginCleanCard("Detalhes do Item","LOCAL");HazzureUI::DrawInventoryAttributes(item);HazzureUI::EndCleanCard();ImGui::End();
                    }else if(tab==22){
                        auto* draw=ImGui::GetBackgroundDrawList();
                        draw->AddRectFilled({0,0},{static_cast<float>(size.first),static_cast<float>(size.second)},IM_COL32(215,207,181,255));
                        HazzureUI::Matrix4 matrix{};matrix.m[0][1]=-1.3f;matrix.m[1][2]=1.732f;matrix.m[1][3]=-64.f*1.732f;matrix.m[3][0]=1;
                        HazzureGame::Pawn local{};local.health=100;local.team=2;local.eye={0,0,64};local.anglesValid=true;
                        for(int i=0;i<3;++i){HazzureCombat::Ghost ghost{};ghost.handle=i+1;ghost.team=3;ghost.health=27+i*30;ghost.healthShown=static_cast<float>(ghost.health+10);
                            ghost.weaponDefinition=i==0?7:i==1?9:16;ghost.ammo=i==1?5:30;ghost.reserve=i==1?30:90;
                            ghost.origin={400,static_cast<float>((i-1)*170),0};ghost.head={ghost.origin.x,ghost.origin.y,72};
                            strcpy_s(ghost.name,i==0?"AK-47":i==1?"AWP":"M4A4");strcpy_s(ghost.weapon,ghost.name);
                            ghost.bones[0]={{ghost.origin.x,ghost.origin.y,64},{ghost.origin.x,ghost.origin.y,30}};ghost.boneCount=1;
                            HazzureUI::DrawCombatGhost(draw,ghost,local,matrix,size.first,size.second,1.f,"rifles");}
                    }else if(tab==23){
                        HazzureGrenade::Result result{};result.count=5;result.bounceCount=2;result.complete=true;
                        result.finish=HazzureGrenade::Finish::RestEstimate;
                        result.points[0]={100,-60,60};result.points[1]={300,-60,110};result.points[2]={500,-60,60};
                        result.points[3]={400,90,20};result.points[4]={450,150,0};result.bounces[0]=result.points[2];result.bounces[1]=result.points[3];
                        HazzureUI::Matrix4 matrix{};matrix.m[0][1]=-1.3f;matrix.m[1][2]=1.732f;matrix.m[1][3]=-64.f*1.732f;matrix.m[3][0]=1;
                        HazzureUI::DrawGrenadePreview(ImGui::GetBackgroundDrawList(),result,matrix,size.first,size.second);
                    }else if(tab==24){
                        HazzureUI::g_cfg.humanizedTrigger=true;HazzureUI::g_cfg.triggerDistanceDelay=false;
                        ImGui::SetNextWindowPos({(size.first-360.f)*.5f,30});ImGui::SetNextWindowSize({360.f,430.f});
                        ImGui::Begin("Triggerbot",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
                        HazzureUI::DrawTriggerControls();ImGui::End();
                    }else if(tab==31||tab==32){
                        ImGui::SetNextWindowPos({(size.first-390.f)*.5f,30});ImGui::SetNextWindowSize({390.f,size.second-60.f});
                        ImGui::Begin("Recursos Avancados",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
                        if(tab==31){HazzureUI::g_cfg.autoStrafePhysics=HazzureUI::g_cfg.jumpBug=HazzureUI::g_cfg.edgeBug=HazzureUI::g_cfg.pixelSurf=true;
                            HazzureUI::DrawAdvancedMovementControls();}
                        else{HazzureLineupCatalogue::Merge(HazzureUI::g_cfg.lineups);HazzureUI::g_cfg.lineupAutoAlign=true;HazzureUI::DrawLineupControls(0);}
                        ImGui::End();
                    }else if(tab==33||tab==34){
                        HazzureUI::Matrix4 matrix{};matrix.m[0][1]=-1.3f;matrix.m[1][2]=1.732f;matrix.m[1][3]=-64.f*1.732f;matrix.m[3][0]=1;
                        if(tab==33){HazzureUI::g_cfg.grenadeZones=true;HazzureZones::enabled=true;HazzureZones::Clear();
                            HazzureZones::Push({HazzureZones::Kind::Fire,6,{450,0,0},1000});
                            HazzureInferno::Cells cells{};cells.id=6;cells.entity=1;cells.handle=1;cells.time=2000;cells.halfWidth=26;cells.count=4;
                            cells.positions[0]={425,-25,0};cells.positions[1]={475,-25,0};cells.positions[2]={425,25,0};cells.positions[3]={475,25,0};
                            AcquireSRWLockExclusive(&HazzureInferno::lock);HazzureInferno::snapshots[0]=cells;ReleaseSRWLockExclusive(&HazzureInferno::lock);
                            HazzureUI::DrawGrenadeZones(ImGui::GetBackgroundDrawList(),matrix,size.first,size.second,2000);
                        }else{
                            HazzurePoses::Clear();
                            for(int i=0;i<10;++i){HazzureCombat::Ghost ghost{};ghost.pawn=1;ghost.handle=1;ghost.seen=1000+i*16;ghost.boneCount=2;
                                float y=-150.f+i*25;ghost.bones[0]={{400,y,65},{400,y,30}};ghost.bones[1]={{400,y,30},{400,y+10,0}};
                                HazzurePoses::Store(1,ghost);}
                            size_t poseCount=0;
                            HazzurePoses::Visit(1150,160,[&](const auto& ghost,float alpha){++poseCount;for(size_t i=0;i<ghost.boneCount;++i)
                                HazzureUI::CombatLine(ImGui::GetBackgroundDrawList(),ghost.bones[i].a,ghost.bones[i].b,matrix,size.first,size.second,
                                    HazzureUI::CombatAlpha(IM_COL32(90,165,255,200),alpha),2.f);});
                            if(!poseCount)throw std::runtime_error("empty pose-history fixture");
                            ImGui::GetBackgroundDrawList()->AddText({30,30},IM_COL32(180,210,255,255),"Historico visual: poses recebidas nos ultimos 160 ms");
                        }
                    }else if(tab==30){
                        HazzureUI::g_cfg.grenadeZones=HazzureUI::g_cfg.soundRadar=true;HazzureZones::enabled=true;
                        HazzureZones::Clear();HazzureZones::Push({HazzureZones::Kind::Smoke,1,{600,-170,0},1000});
                        HazzureZones::Push({HazzureZones::Kind::Fire,2,{600,170,0},1000});
                        HazzureZones::Push({HazzureZones::Kind::HE,3,{1000,0,0},2000});
                        HazzureUI::Matrix4 matrix{};matrix.m[0][1]=-1.3f;matrix.m[1][2]=1.732f;matrix.m[1][3]=-64.f*1.732f;matrix.m[3][0]=1;
                        HazzureUI::DrawGrenadeZones(ImGui::GetBackgroundDrawList(),matrix,size.first,size.second,2300);
                        HazzureCombat::effects.Clear();HazzureCombat::effects.Add({HazzureCombat::Kind::Footstep,2000,1,0,{300,-100,0}});
                        HazzureCombat::effects.Add({HazzureCombat::Kind::Reload,2000,2,0,{450,100,0}});
                        HazzureUI::DrawCombatEffects(ImGui::GetBackgroundDrawList(),matrix,{},size.first,size.second,2300);
                    }else if(tab>=27&&tab<=29){
                        HazzureUI::g_cfg.hitchanceEnabled=HazzureUI::g_cfg.aimMultipoint=HazzureUI::g_cfg.aimBodyFallback=true;
                        HazzureUI::g_cfg.shotStop=true;
                        HazzureUI::g_cfg.lineupHelper=true;strcpy_s(HazzureUI::g_cfg.lineups.map,"de_mirage");
                        ImGui::SetNextWindowPos({(size.first-390.f)*.5f,30});ImGui::SetNextWindowSize({390.f,size.second-60.f});
                        ImGui::Begin("Assistencias",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
                        if(tab==27)HazzureUI::DrawAccuracyControls();
                        else if(tab==28)HazzureUI::DrawShotStopControls();else HazzureUI::DrawLineupControls(0);
                        ImGui::End();
                    }else if(tab==25||tab==26){
                        HazzureUI::g_cfg.cameraFovOverride=true;HazzureUI::g_cfg.cameraFov=117.5f;
                        HazzureCamera::installed=true;HazzureCamera::lastDelivery=GetTickCount64();
                        HazzureCamera::status=tab==25?HazzureCamera::Status::Applied:HazzureCamera::Status::Scoped;
                        HazzureCamera::delivered=tab==25?117.5f:30.f;
                        ImGui::SetNextWindowPos({(size.first-360.f)*.5f,30});ImGui::SetNextWindowSize({360.f,430.f});
                        ImGui::Begin("Camera",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
                        HazzureUI::DrawCameraControls();ImGui::End();
                    }else{
                        HazzureUI::g_currentTab=tab==20?3:0;
                        HazzureUI::g_cfg.edgeJump=HazzureUI::g_cfg.fastLadder=HazzureUI::g_cfg.dormantFade=true;
                        HazzureUI::DrawPanel(&settings,&open,size.first,size.second);
                    }
                    ImGui::Render();
                    if(ImGui::GetDrawData()->TotalVtxCount<(tab==15?9:100))throw std::runtime_error("empty UI draw data");
                    context->OMSetRenderTargets(1,&target.p,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                }
                const wchar_t* hudNames[]={L"hud-lethal",L"hud-defuse",L"hud-unavailable-dense",L"hud-defused"};
                const wchar_t* combatNames[]={L"combat-world",L"combat-fade",L"combat-arrows",L"armory-stickers",L"armory-charm",L"armory-stattrak",L"armory-nametag",L"movement-tech",L"visual-expanded",L"esp-refined",L"grenade-3d"};
                const wchar_t* name=nullptr;
                const wchar_t* advancedNames[]={L"advanced-movement",L"lineup-catalogue",L"native-fire-cells",L"pose-history"};
                const wchar_t* assistNames[]={L"accuracy-controls",L"shot-stop-controls",L"lineup-controls",L"grenade-zones-radial"};
                if(tab>=31)name=advancedNames[tab-31];
                else if(tab>=27)name=assistNames[tab-27];
                else if(tab>=25)name=tab==25?L"camera-fov-applied":L"camera-fov-scoped";
                else if(tab==24)name=L"trigger-balanced";
                else if(tab<7)name=names[tab];
                else if(tab==7)name=L"gunplay-expanded";
                else if(tab==8)name=L"keybinds";
                else if(tab<13)name=hudNames[tab-9];
                else name=combatNames[tab-13];
                std::wstring filename=std::wstring(name)+L"-"+std::to_wstring(size.first)+L"x"+std::to_wstring(size.second)+L".png";
                SavePng(device.p,context.p,texture.p,output/filename);
                printf("PASS: UI tab %d at %dx%d\n",tab,size.first,size.second);
            }
        }
        HazzureUI::g_cfg.inventory={};
        for(int n=0;n<3;++n){HazzureInventory::Item item{};sprintf_s(item.label,"%s / variante %d",n==2?"AWP":"AK-47",n+1);
            item.skin={n==2?9:7,n==2?344:180,n==1?387:661,0,n==1?.18f:.012f,1,1,0};
            if(!HazzureInventory::Add(HazzureUI::g_cfg.inventory,item))throw std::runtime_error("inventory render fixture invalid");}
        HazzureUI::g_inventorySelected=HazzureUI::g_cfg.inventory.items[0].id;
        if(!HazzureUI::ApplySharedConfig(&settings)||settings.skinEnabled)
            throw std::runtime_error("inventory fixture reactivated retired cosmetics");
        strcpy_s(HazzureUI::g_configStatusMsg,"Colecao local configurada");
        unsigned extraCaptures=0;
        for(auto size:{std::pair<int,int>{640,480},std::pair<int,int>{1024,720},std::pair<int,int>{1920,1080}}){
            auto layout=HazzureUI::ResolvePanelLayout(size.first,size.second);
            if(layout.width>size.first||layout.height>size.second||(size.first==640&&layout.twoColumns))throw std::runtime_error("responsive panel escapes the viewport");
            D3D11_TEXTURE2D_DESC desc{};desc.Width=size.first;desc.Height=size.second;desc.MipLevels=1;desc.ArraySize=1;
            desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
            Com<ID3D11Texture2D> texture;Require(device->CreateTexture2D(&desc,nullptr,texture.Address()),"page render texture");
            Com<ID3D11RenderTargetView> target;Require(device->CreateRenderTargetView(texture.p,nullptr,target.Address()),"page render target");
            io.DisplaySize={static_cast<float>(size.first),static_cast<float>(size.second)};
            settings.agentCT=1;settings.agentT=1;HazzureUI::g_cfg.weaponIndex=5;
            for(int tab=0;tab<static_cast<int>(std::size(HazzureUI::kPanelSectionCounts));++tab)for(int section=0;section<HazzureUI::kPanelSectionCounts[tab];++section)for(int bottom=0;bottom<2;++bottom){
                HazzureUI::g_currentTab=tab;HazzureUI::g_panelSections[tab]=section;bool open=true;
                HazzureUI::g_previewAgentTeam=bottom;
                for(int frame=0;frame<3;++frame){
                    // Drive only ImGui's own offscreen scroll state, never desktop input.
                    for(ImGuiWindow* window:ImGui::GetCurrentContext()->Windows)
                        if(strstr(window->Name,"##settingsColumn")||strstr(window->Name,"##settingsPage"))ImGui::SetScrollY(window,bottom?window->ScrollMax.y:0);
                    const float background[]={.025f,.03f,.045f,1};context->ClearRenderTargetView(target.p,background);
                    ImGui_ImplDX11_NewFrame();ImGui::NewFrame();HazzureUI::DrawPanel(&settings,&open,size.first,size.second);ImGui::Render();
                    auto* panel=ImGui::FindWindowByName("##HazzureEnigmaStudio");
                    if(!panel||panel->Pos.x<0||panel->Pos.y<0||panel->Pos.x+panel->Size.x>size.first+.5f||panel->Pos.y+panel->Size.y>size.second+.5f)
                        throw std::runtime_error("panel bounds overlap viewport edges");
                    context->OMSetRenderTargets(1,&target.p,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                }
                std::wstring filename=L"page-"+std::to_wstring(tab)+L"-"+std::to_wstring(section)+(bottom?L"-bottom-":L"-top-")+std::to_wstring(size.first)+L"x"+std::to_wstring(size.second)+L".png";
                SavePng(device.p,context.p,texture.p,output/filename);++extraCaptures;
            }
        }
        unsigned expectedCaptures=0;for(int sections:HazzureUI::kPanelSectionCounts)expectedCaptures+=sections*2*3;
        if(extraCaptures!=expectedCaptures)throw std::runtime_error("not every subsection was rendered at both scroll positions");
        HazzurePreviewAssets::SetDevice(nullptr);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();CoUninitialize();
        printf("%u UI captures rendered. No game process, hooks or OS input used.\n",74+extraCaptures);return 0;
    }catch(const std::exception& error){printf("FAIL: %s\n",error.what());return 1;}
}
