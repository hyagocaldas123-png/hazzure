// Isolated geometry/events/config/DX11-WARP regressions; never touches a game.
#define INTERNAL_SELF_TEST
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <wincodec.h>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
#include "../src/HazzureUI.h"
#include "../../HazzurePanel/third_party/imgui/imgui_internal.h"
#include "../../HazzurePanel/third_party/imgui/imgui_impl_dx11.h"
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"dwmapi.lib")
static int checks{};
static void Check(bool value,const char* text){++checks;if(!value)throw std::runtime_error(text);}
static void Hr(HRESULT value,const char* text){Check(SUCCEEDED(value),text);}
template<class T>struct Com {T* p{};~Com(){HazzureSceneColor::Release(p);}T** Out(){return &p;}};
static float Area(const HazzureVisualGeometry::Mesh& mesh){float area=0;for(const auto& r:mesh.fill)area+=(r.right-r.left)*(r.top-r.bottom);return area;}
static bool Contains(const HazzureVisualGeometry::Mesh& mesh,float x,float y){for(const auto& r:mesh.fill)if(x>r.left&&x<r.right&&y>r.bottom&&y<r.top)return true;return false;}
static void Geometry(){
    using namespace HazzureVisualGeometry;Mesh mesh;
    Rect overlap[]={{0,0,2,2,0},{1,0,3,2,0}};
    Check(Build(overlap,2,mesh)&&Area(mesh)==6&&mesh.outline.size()==4,"overlap union has exact area and external outline only");
    Rect disjoint[]={{0,0,1,1,0},{3,0,4,1,0}};
    Check(Build(disjoint,2,mesh)&&Area(mesh)==2&&mesh.outline.size()==8&&!Contains(mesh,2,.5f),"disconnected components preserve empty gap");
    Rect ring[]={{0,0,3,1,0},{0,2,3,3,0},{0,1,1,2,0},{2,1,3,2,0}};
    Check(Build(ring,4,mesh)&&Area(mesh)==8&&!Contains(mesh,1.5f,1.5f),"ring preserves interior hole");
    Check(mesh.outline.size()>=8,"hole boundaries retained");
    for(size_t i=0;i<mesh.fill.size();++i)for(size_t j=i+1;j<mesh.fill.size();++j){auto a=mesh.fill[i],b=mesh.fill[j];
        Check(std::min(a.right,b.right)<=std::max(a.left,b.left)||std::min(a.top,b.top)<=std::max(a.bottom,b.bottom),"union fill rectangles never overlap");}
    Rect stairs[]={{0,0,2,2,0},{0,0,2,2,40}};
    Check(Build(stairs,2,mesh)&&Area(mesh)==8&&mesh.outline.size()==8,"separate elevations never bridged");
    Check(!Build(overlap,65,mesh)&&!mesh.valid,"excess cell count rejected");
    overlap[0].left=NAN;Check(!Build(overlap,2,mesh),"NaN cell rejected");
    Rect cells[64];for(int i=0;i<64;++i)cells[i]={float(i%8),float(i/8),float(i%8+2),float(i/8+2),0};
    Check(Build(cells,64,mesh)&&Area(mesh)==81&&mesh.outline.size()==4,"maximum overlapping fixture remains bounded");
    LARGE_INTEGER frequency{},start{},finish{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
    for(int run=0;run<100;++run){for(int i=0;i<64;++i){float x=i*1.17f,y=(i*17%64)*1.23f;cells[i]={x,y,x+25,y+25,0};}
        Check(Build(cells,64,mesh)&&mesh.fill.size()<127*127&&mesh.outline.size()<4*127*127,"64-cell recomputation bounded");}
    QueryPerformanceCounter(&finish);printf("MEASURE: 64-cell union average %.3f ms over 100 synthetic rebuilds\n",(finish.QuadPart-start.QuadPart)*1000./frequency.QuadPart/100);
    HazzureGame::Matrix4 matrix{};matrix.m[0][1]=1;matrix.m[1][2]=1;matrix.m[3][0]=1;
    HazzureGame::Vec3 quad[]={{-.5f,-1,-1},{2,-1,-1},{2,1,1},{-.5f,1,1}};ImVec2 points[16]{};
    int count=HazzureUI::VisualPolygon(quad,4,matrix,640,480,points);Check(count>=3,"quad crossing near plane is clipped");
    for(int i=0;i<count;++i)Check(std::isfinite(points[i].x)&&std::isfinite(points[i].y)&&points[i].x>=0&&points[i].x<=640&&points[i].y>=0&&points[i].y<=480,"clipped polygon stays on viewport");
    for(auto& p:quad)p.x=-1;Check(HazzureUI::VisualPolygon(quad,4,matrix,640,480,points)==0,"behind-camera polygon discarded");
    HazzureGame::Vec3 triangle[]={{-.5f,-.2f,0},{1,.2f,0},{1,-.2f,0}};
    count=HazzureUI::VisualPolygon(triangle,3,matrix,640,480,points);
    Check(count>=3,"floor triangle crossing near plane is clipped");
    for(int i=0;i<count;++i)Check(std::isfinite(points[i].x)&&std::isfinite(points[i].y)&&
        points[i].x>=0&&points[i].x<=640&&points[i].y>=0&&points[i].y<=480,"floor clipping stays inside viewport");
    KillGate gate;Check(gate.Accept(2,1,1,true,1),"confirmed local enemy kill accepted");
    Check(!gate.Accept(2,1,1,true,1),"duplicate death suppressed in epoch");
    Check(!gate.Accept(3,1,1,false,1)&&!gate.Accept(1,1,1,true,1)&&!gate.Accept(3,4,1,true,1),"teamkill suicide foreign attacker rejected");
    Check(gate.Accept(2,1,1,true,2),"new epoch permits new kill");gate.Clear();Check(gate.Accept(2,1,1,true,2),"reset clears dedupe");
    HazzureCombat::roundEpoch=9;HazzureCombat::Effects effects;
    auto event=HazzureCombat::Event{HazzureCombat::Kind::Kill,100,1,2,{1,1,1},{},0,true,9};
    effects.Add(event);effects.Add(event);Check(effects.count==1,"effect queue also deduplicates kills");
    event.target=3;event.epoch=8;effects.Add(event);Check(effects.count==1,"old-epoch kill rejected");
    effects.Add({HazzureCombat::Kind::Reset});Check(effects.count==0,"round reset removes effects");
    event.epoch=9;effects.Add(event);effects.Prune(3100);Check(effects.count==0,"maximum effect lifetime prunes deterministically");
    puts("PASS: exact fire union, holes, elevation, clipping, bounded geometry and kill deduplication");
}
static void Config(){
    using namespace HazzureUI;OverlayConfig value{};
    Check(!value.killEffects&&!value.sceneColorEnabled&&!value.grenadeAreaFill,"new effects default opt-in");
    value.grenadeAreaFill=value.grenadeAreaGlow=value.trajectoryGlow=value.trajectoryShimmer=true;
    value.impactFill=value.impactGlow=value.impactSparks=value.killEffects=value.sceneColorEnabled=true;
    value.tracerWidth=3;value.tracerDuration=.8f;value.impactDuration=2.7f;value.impactSize=7;
    value.sceneSaturation=.7f;value.sceneContrast=1.2f;value.sceneTemperature=-.4f;value.sceneTint=.1f;value.sceneColorCurve=1;
    value.colTracer[0]=.77f;value.killEffectIntensity=.5f;
    HazzureConfig::Document document;std::string error;Check(Persistence::WriteConfig(document,value,{},{},{},-1,error),"visual config serialize");
    OverlayConfig restored;Check(Persistence::ReadConfig(document.Root(),restored,error),"visual config deserialize");
    Check(restored.grenadeAreaFill&&restored.grenadeAreaGlow&&restored.trajectoryGlow&&restored.trajectoryShimmer&&restored.impactSparks&&
        restored.impactFill&&restored.impactGlow&&restored.killEffects&&restored.sceneColorEnabled&&restored.tracerWidth==3&&restored.impactSize==7&&
        restored.tracerDuration==.8f&&restored.impactDuration==2.7f&&restored.sceneSaturation==.7f&&restored.sceneTemperature==-.4f&&
        restored.sceneColorCurve==1&&restored.colTracer[0]==.77f&&restored.killEffectIntensity==.5f,"all new visual controls roundtrip");
    for(auto name:{"sceneSaturation","sceneContrast","impactSize","killEffectDuration","trajectoryWidth"}){
        HazzureConfig::Document invalid;Check(invalid.Parse(std::string("{\"")+name+"\":9999}",error),"invalid setting JSON parse");
        Check(!Persistence::ReadConfig(invalid.Root(),restored,error),"out-of-range visual setting rejected");
    }
    HazzureConfig::Document old;Check(old.Parse("{\"tracerDuration\":1.5,\"espWeaponImages\":false}",error),"legacy config parse");
    OverlayConfig legacy;Check(Persistence::ReadConfig(old.Root(),legacy,error)&&legacy.tracerDuration==1.5f&&legacy.impactDuration==1.5f&&!legacy.espWeaponImages&&!legacy.killEffects&&!legacy.sceneColorEnabled,"old config preserves duration semantics, defaults and existing fields");
    puts("PASS: visual controls persist independently and old/invalid configurations are handled");
}
struct Surface {
    Com<ID3D11Texture2D> texture;Com<ID3D11RenderTargetView> target;UINT width{},height{};DXGI_FORMAT format{};UINT samples{};
    Surface(ID3D11Device* d,UINT w,UINT h,DXGI_FORMAT f=DXGI_FORMAT_R8G8B8A8_UNORM,UINT count=1,UINT bindings=D3D11_BIND_RENDER_TARGET):width(w),height(h),format(f),samples(count){
        D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=desc.ArraySize=1;desc.SampleDesc.Count=count;
        desc.Format=f;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=bindings;
        Hr(d->CreateTexture2D(&desc,nullptr,texture.Out()),"WARP surface");Hr(d->CreateRenderTargetView(texture.p,nullptr,target.Out()),"WARP target");
    }
};
static std::vector<unsigned char> Pixels(ID3D11Device* d,ID3D11DeviceContext* c,const Surface& surface){
    D3D11_TEXTURE2D_DESC desc{};surface.texture.p->GetDesc(&desc);Com<ID3D11Texture2D> resolved;
    ID3D11Texture2D* source=surface.texture.p;
    if(desc.SampleDesc.Count>1){desc.SampleDesc={1,0};desc.BindFlags=0;Hr(d->CreateTexture2D(&desc,nullptr,resolved.Out()),"resolve target");
        c->ResolveSubresource(resolved.p,0,source,0,desc.Format);source=resolved.p;}
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Com<ID3D11Texture2D> staging;Hr(d->CreateTexture2D(&desc,nullptr,staging.Out()),"readback target");c->CopyResource(staging.p,source);
    D3D11_MAPPED_SUBRESOURCE mapped{};Hr(c->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"readback map");
    std::vector<unsigned char> result(surface.width*surface.height*4);
    for(UINT row=0;row<surface.height;++row)memcpy(result.data()+row*surface.width*4,static_cast<unsigned char*>(mapped.pData)+row*mapped.RowPitch,surface.width*4);
    c->Unmap(staging.p,0);return result;
}
static void Save(ID3D11Device* d,ID3D11DeviceContext* c,const Surface& surface,const std::filesystem::path& path){
    auto pixels=Pixels(d,c,surface);Com<IWICImagingFactory> factory;Hr(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.Out())),"PNG factory");
    Com<IWICStream> stream;Hr(factory.p->CreateStream(stream.Out()),"PNG stream");Hr(stream.p->InitializeFromFilename(path.c_str(),GENERIC_WRITE),"PNG path");
    Com<IWICBitmapEncoder> encoder;Hr(factory.p->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.Out()),"PNG encoder");Hr(encoder.p->Initialize(stream.p,WICBitmapEncoderNoCache),"PNG init");
    Com<IWICBitmapFrameEncode> frame;Hr(encoder.p->CreateNewFrame(frame.Out(),nullptr),"PNG frame");Hr(frame.p->Initialize(nullptr),"PNG frame init");Hr(frame.p->SetSize(surface.width,surface.height),"PNG size");
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;Hr(frame.p->SetPixelFormat(&format),"PNG format");
    Check(format==GUID_WICPixelFormat32bppBGRA,"BGRA PNG accepted");
    if(surface.format==DXGI_FORMAT_R8G8B8A8_UNORM)for(size_t i=0;i<pixels.size();i+=4)std::swap(pixels[i],pixels[i+2]);
    Hr(frame.p->WritePixels(surface.height,surface.width*4,static_cast<UINT>(pixels.size()),pixels.data()),"PNG write");
    Hr(frame.p->Commit(),"PNG frame commit");Hr(encoder.p->Commit(),"PNG commit");
}
static void Grading(ID3D11Device* d,ID3D11DeviceContext* c,const std::filesystem::path& folder){
    using namespace HazzureSceneColor;Pass pass;Settings neutral{true,1,1,0,0,0};
    Check(!Active(neutral),"neutral settings bypass GPU pass");Settings invalid=neutral;invalid.temperature=NAN;Check(!Valid(invalid)&&!Active(invalid),"NaN grading rejected");
    Surface image(d,32,24),other(d,32,24);float color[]={.2f,.4f,.7f,.4f};c->ClearRenderTargetView(image.target.p,color);
    auto original=Pixels(d,c,image);Check(pass.Apply(d,c,image.texture.p,image.target.p,neutral)&&Pixels(d,c,image)==original,"neutral pass bitwise identity");
    Settings adjusted{true,0,1,0,0,0};
    Com<ID3D11DeviceContext1> context1;Com<ID3D11Buffer> rangeBuffer;UINT first=16,length=16;
    if(SUCCEEDED(c->QueryInterface(__uuidof(ID3D11DeviceContext1),reinterpret_cast<void**>(context1.Out())))){
        D3D11_BUFFER_DESC desc{};desc.ByteWidth=4096;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        Hr(d->CreateBuffer(&desc,nullptr,rangeBuffer.Out()),"ranged constant buffer");context1.p->PSSetConstantBuffers1(0,1,&rangeBuffer.p,&first,&length);
    }
    ID3D11RenderTargetView* targets[]={image.target.p,other.target.p};c->OMSetRenderTargets(2,targets,nullptr);
    D3D11_VIEWPORT viewport{3,4,12,10,.1f,.8f};c->RSSetViewports(1,&viewport);
    D3D11_RECT scissor{1,2,9,10};c->RSSetScissorRects(1,&scissor);
    D3D11_BLEND_DESC bd{};bd.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_RED;
    Com<ID3D11BlendState> blend;Hr(d->CreateBlendState(&bd,blend.Out()),"test blend");float factors[]={.1f,.2f,.3f,.4f};c->OMSetBlendState(blend.p,factors,123);
    D3D11_QUERY_DESC qd{D3D11_QUERY_OCCLUSION_PREDICATE,0};Com<ID3D11Predicate> predicate;Hr(d->CreatePredicate(&qd,predicate.Out()),"predicate");
    c->Begin(predicate.p);c->End(predicate.p);c->Flush();BOOL predicateResult{};
    while(c->GetData(predicate.p,&predicateResult,sizeof(predicateResult),0)==S_FALSE)Sleep(1);
    c->SetPredication(predicate.p,TRUE);
    Check(pass.Apply(d,c,image.texture.p,image.target.p,adjusted),"grayscale shader compiled and ran");
    if(context1.p){ID3D11Buffer* actualBuffer{};UINT actualFirst{},actualLength{};
        context1.p->PSGetConstantBuffers1(0,1,&actualBuffer,&actualFirst,&actualLength);
        Check(actualBuffer==rangeBuffer.p&&actualFirst==first&&actualLength==length,"DX11.1 constant-buffer ranges restored");Release(actualBuffer);}
    ID3D11Predicate* restoredPredicate{};BOOL pv{};c->GetPredication(&restoredPredicate,&pv);
    Check(restoredPredicate==predicate.p&&pv==TRUE,"predication restored after unconditional grading draw");Release(restoredPredicate);c->SetPredication(nullptr,FALSE);
    ID3D11RenderTargetView* restored[8]{};c->OMGetRenderTargets(8,restored,nullptr);
    Check(restored[0]==image.target.p&&restored[1]==other.target.p,"all MRT targets restored");for(auto*& t:restored)Release(t);
    UINT n=1;D3D11_VIEWPORT actual{};c->RSGetViewports(&n,&actual);Check(n==1&&memcmp(&actual,&viewport,sizeof(viewport))==0,"viewport restored");
    ID3D11BlendState* restoredBlend{};FLOAT restoredFactors[4]{};UINT mask{};c->OMGetBlendState(&restoredBlend,restoredFactors,&mask);
    Check(restoredBlend==blend.p&&mask==123&&memcmp(factors,restoredFactors,sizeof(factors))==0,"blend state restored");Release(restoredBlend);
    auto grey=Pixels(d,c,image);Check(abs(int(grey[0])-grey[1])<=1&&abs(int(grey[1])-grey[2])<=1,"grayscale shader transforms all channels consistently");
    Check(grey[3]==original[3]&&grey!=original,"shader preserves alpha and changes scene pixels despite false predicate");
    c->OMSetBlendState(nullptr,nullptr,0xffffffff);c->OMSetRenderTargets(0,nullptr,nullptr);
    for(auto format:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB}){
        Surface scene(d,41,17,format);c->ClearRenderTargetView(scene.target.p,color);auto before=Pixels(d,c,scene);
        Settings warm{true,1.2f,1.1f,.5f,-.2f,1};Check(pass.Apply(d,c,scene.texture.p,scene.target.p,warm),"format-specific grading");
        auto after=Pixels(d,c,scene);Check(after!=before&&after[3]==before[3],"grading changes scene but preserves alpha per format");
    }
    UINT qualities{};Hr(d->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,4,&qualities),"MSAA support query");
    if(qualities){Surface msaa(d,31,19,DXGI_FORMAT_R8G8B8A8_UNORM,4);c->ClearRenderTargetView(msaa.target.p,color);
        Check(pass.Apply(d,c,msaa.texture.p,msaa.target.p,adjusted),"MSAA resolves before grading");auto px=Pixels(d,c,msaa);Check(abs(int(px[0])-px[2])<=1,"MSAA grayscale pixels");}
    for(int i=0;i<12;++i){pass.ReleaseTargets();Surface resized(d,23+i*3,19+i);
        c->ClearRenderTargetView(resized.target.p,color);Check(pass.Apply(d,c,resized.texture.p,resized.target.p,adjusted),"resize resources recreate");}
    pass.Reset();Check(pass.Apply(d,c,image.texture.p,image.target.p,adjusted),"device-resource shutdown and recreation");
    Com<ID3D11Device> oldDevice;Com<ID3D11DeviceContext> oldContext;D3D_FEATURE_LEVEL requested=D3D_FEATURE_LEVEL_10_0;
    Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,&requested,1,D3D11_SDK_VERSION,oldDevice.Out(),nullptr,oldContext.Out()),"legacy device");
    Surface legacy(oldDevice.p,8,8);Pass legacyPass;
    Check(!legacyPass.Apply(oldDevice.p,oldContext.p,legacy.texture.p,legacy.target.p,adjusted),"unsupported feature level declines unchanged");
    Check(!legacyPass.Apply(oldDevice.p,oldContext.p,legacy.texture.p,legacy.target.p,adjusted),"unsupported device failure stays latched");
    legacyPass.Reset();Check(!legacyPass.Apply(oldDevice.p,oldContext.p,legacy.texture.p,legacy.target.p,adjusted),"explicit reset permits a new device assessment");
    Surface alias(d,16,16,DXGI_FORMAT_R8G8B8A8_UNORM,1,D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE);
    Com<ID3D11ShaderResourceView> aliasView;Hr(d->CreateShaderResourceView(alias.texture.p,nullptr,aliasView.Out()),"alias view");
    c->OMSetRenderTargets(0,nullptr,nullptr);c->PSSetShaderResources(7,1,&aliasView.p);
    Check(!pass.Apply(d,c,alias.texture.p,alias.target.p,adjusted),"source bound in another SRV slot is declined");
    ID3D11ShaderResourceView* retainedView{};c->PSGetShaderResources(7,1,&retainedView);
    Check(retainedView==aliasView.p,"declined pass preserves aliasing SRV slot");Release(retainedView);ID3D11ShaderResourceView* empty=nullptr;c->PSSetShaderResources(7,1,&empty);
    c->OMSetRenderTargets(0,nullptr,nullptr);pass.Reset();
    puts("PASS: WARP shader pixels, neutral bypass, alpha, MRT/viewport/blend/predication, formats, MSAA and resource resize");
    (void)folder;
}
static void SwapchainResize(){
    const wchar_t* name=L"HazzureIsolatedColorResize";WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=name;
    Check(RegisterClassW(&wc)!=0,"hidden resize test class");
    HWND window=CreateWindowExW(0,name,L"",WS_OVERLAPPED,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);Check(window!=nullptr,"hidden resize test window");
    // No WS_VISIBLE, ShowWindow, focus changes, or synthetic input.
    Com<ID3D11Device> d;Com<ID3D11DeviceContext> c;Com<IDXGISwapChain> chain;
    DXGI_SWAP_CHAIN_DESC desc{};desc.BufferDesc.Width=32;desc.BufferDesc.Height=32;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;desc.OutputWindow=window;desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    Hr(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,chain.Out(),d.Out(),nullptr,c.Out()),"hidden WARP swapchain");
    HazzureSceneColor::Pass pass;
    for(UINT i=0;i<6;++i){
        {Com<ID3D11Texture2D> source;Com<ID3D11RenderTargetView> target;
            Hr(chain.p->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(source.Out())),"swapchain backbuffer");
            Hr(d.p->CreateRenderTargetView(source.p,nullptr,target.Out()),"swapchain RTV");float color[]={.2f,.4f,.7f,1};c.p->ClearRenderTargetView(target.p,color);
            Check(pass.Apply(d.p,c.p,source.p,target.p,{true,1.1f,1.2f,.3f,0,0}),"grading swapchain before resize");
        }
        pass.ReleaseTargets();c.p->OMSetRenderTargets(0,nullptr,nullptr);
        Hr(chain.p->ResizeBuffers(2,48+i*11,40+i*7,DXGI_FORMAT_UNKNOWN,0),"real ResizeBuffers releases all backbuffer references");
    }
    pass.Reset();c.p->ClearState();chain.p->SetFullscreenState(FALSE,nullptr);HazzureSceneColor::Release(chain.p);
    DestroyWindow(window);UnregisterClassW(name,wc.hInstance);
    puts("PASS: six real hidden WARP swapchain ResizeBuffers calls with grading resources released/recreated");
}
static void Captures(ID3D11Device* d,ID3D11DeviceContext* c,const std::filesystem::path& folder){
    using namespace HazzureUI;ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DeltaTime=1.f/60;
    Check(ImGui_ImplDX11_Init(d,c),"ImGui WARP init");ImGui::StyleColorsDark();Shared shared{};shared.magic=0x485a4931;shared.version=2;
    g_settingsShared=&shared;InitializeContextDefaults();
    auto configure=[](){g_cfg.grenadeZones=g_cfg.grenadeAreaFill=g_cfg.grenadeAreaGlow=true;g_cfg.trajectoryGlow=g_cfg.trajectoryShimmer=true;
        g_cfg.bulletTracers=g_cfg.impactBoxes=g_cfg.impactFill=g_cfg.impactGlow=g_cfg.impactSparks=g_cfg.killEffects=true;g_cfg.sceneColorEnabled=true;};
    HazzureGame::Matrix4 matrix{};matrix.m[0][1]=-1.3f;matrix.m[1][2]=1.732f;matrix.m[1][3]=-64.f*1.732f;matrix.m[3][0]=1;
    for(auto size:{std::pair<int,int>{640,480},{1024,720},{1920,1080}}){Surface image(d,size.first,size.second);
        io.DisplaySize={float(size.first),float(size.second)};
        for(int scene=0;scene<5;++scene){g_cfg=OverlayConfig{};configure();
            for(int frame=0;frame<3;++frame){float bg[]={.025f,.04f,.065f,1};c->ClearRenderTargetView(image.target.p,bg);
                ImGui_ImplDX11_NewFrame();ImGui::NewFrame();auto* draw=ImGui::GetBackgroundDrawList();
                if(scene==0){
                    HazzureZones::enabled=true;HazzureZones::Clear();HazzureZones::Push({HazzureZones::Kind::Fire,6,{450,0,0},1000});
                    HazzureInferno::Cells cells{};cells.id=6;cells.entity=1;cells.handle=1;cells.time=2000;cells.halfWidth=26;cells.count=8;
                    for(int i=0;i<8;++i){int x=i<4?i%2:(i-4)%2+3,y=i<4?i/2:(i-4)/2;
                        cells.positions[i]={425.f+x*45.f,-70.f+y*45.f,0};}
                    AcquireSRWLockExclusive(&HazzureInferno::lock);HazzureInferno::snapshots[0]=cells;ReleaseSRWLockExclusive(&HazzureInferno::lock);
                    DrawGrenadeZones(draw,matrix,size.first,size.second,2000);
                    draw->AddText({20,20},IM_COL32_WHITE,"Areas: componentes separados, preenchimento e borda externa");
                }else if(scene==1){
                    HazzureCombat::effects.Clear();HazzureCombat::roundEpoch=7;
                    HazzureCombat::effects.Add({HazzureCombat::Kind::Impact,1800,1,0,{450,-80,70},{120,70,40}});
                    HazzureCombat::effects.Add({HazzureCombat::Kind::Kill,1700,1,2,{450,100,40},{},0,true,7});
                    DrawCombatEffects(draw,matrix,{},size.first,size.second,2000);
                    draw->AddText({20,20},IM_COL32_WHITE,"Tracers, impactos e efeito de eliminacao (eventos sinteticos)");
                }else if(scene==2){
                    HazzureGrenade::Result path{};path.count=4;path.bounceCount=1;path.complete=true;path.finish=HazzureGrenade::Finish::RestEstimate;
                    path.points[0]={100,-40,45};path.points[1]={300,-40,120};path.points[2]={500,-30,30};path.points[3]={550,120,0};path.bounces[0]=path.points[2];
                    DrawGrenadePreview(draw,path,matrix,size.first,size.second);
                }else{
                    ImGui::SetNextWindowPos({16,16});ImGui::SetNextWindowSize({float(size.first)-32,float(size.second)-32});
                    ImGui::Begin("Visual Package",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
                    g_cleanCardContentW=float(size.first)-100;
                    if(scene==3){ImGui::SetNextItemOpen(true,ImGuiCond_Always);DrawSettingsCard1(&shared);}
                    else {HazzureSceneColor::scenePass.Apply(nullptr,nullptr,nullptr,nullptr,{true,1,1,0,0,0});DrawSettingsCard15(&shared);}ImGui::End();
                }
                ImGui::Render();c->OMSetRenderTargets(1,&image.target.p,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            }
            wchar_t name[100];swprintf_s(name,L"visual-package-%d-%dx%d.png",scene,size.first,size.second);Save(d,c,image,folder/name);
        }
    }
    c->OMSetRenderTargets(0,nullptr,nullptr);HazzureInferno::Clear();HazzureZones::Clear();HazzureCombat::Reset();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();
    puts("PASS: 15 isolated WARP captures of new effects and controls at 640x480, 1024x720 and 1920x1080");
}
int wmain(int argc,wchar_t** argv){
    try{Check(argc==2,"output folder required");auto folder=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(folder);
        Hr(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"COM init");Geometry();Config();
        Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;
        Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,device.Out(),nullptr,context.Out()),"WARP device");
        Grading(device.p,context.p,folder);SwapchainResize();Captures(device.p,context.p,folder);HazzureSceneColor::scenePass.Reset();
        printf("PASS: %d visual package checks\n",checks);CoUninitialize();return 0;
    }catch(const std::exception& error){printf("FAIL: %s\n",error.what());return 1;}
}
