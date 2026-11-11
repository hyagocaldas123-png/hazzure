// C++ / panel-render-tests.cpp / MSVC x64 C++17 / isolated D3D11 WARP UI rendering
#define NOMINMAX
#ifndef INTERNAL_SELF_TEST
#define INTERNAL_SELF_TEST
#endif
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

static unsigned checks=0;
static void Check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
static void NavigationInteractions(){
    using namespace HazzureUI;
    auto& io=ImGui::GetIO();io.DisplaySize={640,480};bool open=true;Shared settings{};
    GSLayout::gs_currentTab=0;
    auto frame=[&](){ImGui_ImplDX11_NewFrame();ImGui::NewFrame();GSLayout::DrawGSPanel(&settings,&open,640,480);ImGui::Render();};
    frame();frame();
    ImGuiWindow* sidebar=nullptr;
    for(auto* w:ImGui::GetCurrentContext()->Windows)if(strstr(w->Name,"##gs_sidebar"))sidebar=w;
    Check(sidebar&&sidebar->ScrollMax.y>0,"compact navigation has no scroll access");
    ImVec2 target(sidebar->Pos.x+40,sidebar->DC.CursorStartPos.y+ImGui::GetFontSize()+8+40+16);
    io.AddMousePosEvent(target.x,target.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();
    Check(GSLayout::gs_currentTab==1,"sidebar route click failed");
    io.AddMousePosEvent(sidebar->Pos.x+40,sidebar->Pos.y+100);io.AddMouseWheelEvent(0,-30);frame();frame();
    Check(sidebar->Scroll.y>0,"sidebar wheel does not reach hidden navigation");
    int pending='K';g_bindingTarget=&pending;g_bindingRenderedFrame=-1;frame();Check(!g_bindingTarget,"hidden widget retained key capture");
    g_bindingTarget=&pending;open=false;frame();Check(!g_bindingTarget,"closed menu retained key capture");
    puts("PASS: actual sidebar click, compact wheel access and closed-panel capture cleanup");
}

static void SearchNavigationTests(){
    using namespace HazzureUI;
    auto& io=ImGui::GetIO();io.DisplaySize={800,600};bool open=true;Shared settings{};
    auto frame=[&](){ImGui_ImplDX11_NewFrame();ImGui::NewFrame();GSLayout::DrawGSPanel(&settings,&open,800,600);ImGui::Render();};
    frame();frame();

    // Test 1: Alias "clima" -> Visuals/World (tab 6, section 1)
    strcpy_s(GSLayout::gs_searchQuery, "clima");
    // Simulate input text change by calling the search processor directly
    bool navigated = GSLayout::GS_ProcessSearchQuery("clima");
    Check(navigated, "search 'clima' should navigate");
    Check(GSLayout::gs_currentTab == 6, "search 'clima' should go to tab 6 (Visuals)");
    Check(GSLayout::gs_currentSection[6] == 1, "search 'clima' should go to section 1 (World)");

    // Test 2: Alias "tempo" (sem acento) -> same destination
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("tempo");
    Check(navigated, "search 'tempo' should navigate");
    Check(GSLayout::gs_currentTab == 6, "search 'tempo' should go to tab 6");
    Check(GSLayout::gs_currentSection[6] == 1, "search 'tempo' should go to section 1");

    // Test 3: Alias "mira" -> Aim Assist (tab 1, section 0)
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("mira");
    Check(navigated, "search 'mira' should navigate");
    Check(GSLayout::gs_currentTab == 1, "search 'mira' should go to tab 1 (Aim Assist)");
    Check(GSLayout::gs_currentSection[1] == 0, "search 'mira' should go to section 0 (General)");

    // Test 4: Alias "crosshair" (EN) -> same destination
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("crosshair");
    Check(navigated, "search 'crosshair' should navigate");
    Check(GSLayout::gs_currentTab == 1, "search 'crosshair' should go to tab 1");

    // Test 5: Alias "inventário" (com acento) -> Inventory (tab 11)
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("inventário");
    Check(navigated, "search 'inventário' should navigate");
    Check(GSLayout::gs_currentTab == 11, "search 'inventário' should go to tab 11 (Inventory)");

    // Test 6: Alias "inventario" (sem acento) -> same destination (Fold handles accents)
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("inventario");
    Check(navigated, "search 'inventario' should navigate");
    Check(GSLayout::gs_currentTab == 11, "search 'inventario' should go to tab 11");

    // Test 7: Alias "inventory" (EN) -> same destination
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("inventory");
    Check(navigated, "search 'inventory' should navigate");
    Check(GSLayout::gs_currentTab == 11, "search 'inventory' should go to tab 11");

    // Test 8: Substring match on tab name "visual" -> Visuals (tab 6)
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("visual");
    Check(navigated, "search 'visual' should navigate via substring");
    Check(GSLayout::gs_currentTab == 6, "search 'visual' should go to tab 6");

    // Test 9: Substring match on section name "skybox" -> World tab 7, section 0
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("skybox");
    Check(navigated, "search 'skybox' should navigate via section substring");
    Check(GSLayout::gs_currentTab == 7, "search 'skybox' should go to tab 7 (World)");
    Check(GSLayout::gs_currentSection[7] == 0, "search 'skybox' should go to section 0 (Skybox)");

    // Test 10: Empty query should not navigate
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("");
    Check(!navigated, "empty search should not navigate");
    Check(GSLayout::gs_currentTab == 0, "empty search should not change tab");

    // Test 11: Unknown query should not navigate
    GSLayout::gs_currentTab = 0; GSLayout::gs_currentSection[0] = 0;
    navigated = GSLayout::GS_ProcessSearchQuery("xyzunknown123");
    Check(!navigated, "unknown search should not navigate");
    Check(GSLayout::gs_currentTab == 0, "unknown search should not change tab");

    puts("PASS: search navigation aliases, accents, substring matches, and negative cases");
}

static void HiddenTabsValidationTests(){
    using namespace HazzureUI;
    auto& io=ImGui::GetIO();io.DisplaySize={800,600};bool open=true;Shared settings{};
    auto frame=[&](){ImGui_ImplDX11_NewFrame();ImGui::NewFrame();GSLayout::DrawGSPanel(&settings,&open,800,600);ImGui::Render();};

    // Test 1: gs_currentTab = 5 (Items, hidden) should be remapped to 11 (Inventory) on DrawGSPanel entry
    GSLayout::gs_currentTab = 5;
    GSLayout::gs_currentSection[5] = 0;
    frame(); // DrawGSPanel should remap
    Check(GSLayout::gs_currentTab == 11, "hidden tab 5 (Items) should be remapped to 11 (Inventory) on panel draw");

    // Test 2: gs_currentTab = -1 (invalid) should be reset to 0
    GSLayout::gs_currentTab = -1;
    frame();
    Check(GSLayout::gs_currentTab == 0, "invalid tab -1 should be reset to 0");

    // Test 3: gs_currentTab = 14 (out of bounds) should be reset to 0
    GSLayout::gs_currentTab = 14;
    frame();
    Check(GSLayout::gs_currentTab == 0, "out of bounds tab 14 should be reset to 0");

    // Test 4: gs_currentSection out of bounds should be clamped
    GSLayout::gs_currentTab = 0; // Legitbot has 6 sections (0-5)
    GSLayout::gs_currentSection[0] = 10;
    frame();
    Check(GSLayout::gs_currentSection[0] == 0, "section index 10 for tab 0 (max 5) should be clamped to 0");

    // Test 5: gs_currentSection negative should be clamped
    GSLayout::gs_currentTab = 0;
    GSLayout::gs_currentSection[0] = -5;
    frame();
    Check(GSLayout::gs_currentSection[0] == 0, "negative section index should be clamped to 0");

    puts("PASS: hidden tabs remapping, index validation and clamping");
}

static void PersistenceNavigationValidationTests(){
    using namespace HazzureUI;
    using namespace HazzureUI::Persistence;

    // Test 1: ValidateNavigationIndices corrects hidden tab 5 -> 11
    GSLayout::gs_currentTab = 5;
    GSLayout::gs_currentSection[5] = 0;
    g_inventorySubTab = 2; // Containers (hidden)
    ValidateNavigationIndices();
    Check(GSLayout::gs_currentTab == 11, "ValidateNavigationIndices should remap tab 5 to 11");
    Check(g_inventorySubTab == 0, "ValidateNavigationIndices should remap inventory sub-tab 2 (Containers) to 0");

    // Test 2: ValidateNavigationIndices corrects inventory sub-tab 3 (Tools) -> 0
    GSLayout::gs_currentTab = 11;
    g_inventorySubTab = 3;
    ValidateNavigationIndices();
    Check(g_inventorySubTab == 0, "ValidateNavigationIndices should remap inventory sub-tab 3 (Tools) to 0");

    // Test 3: ValidateNavigationIndices corrects inventory sub-tab -1 -> 0
    g_inventorySubTab = -1;
    ValidateNavigationIndices();
    Check(g_inventorySubTab == 0, "ValidateNavigationIndices should remap inventory sub-tab -1 to 0");

    // Test 4: ValidateNavigationIndices corrects inventory sub-tab 5 -> 0
    g_inventorySubTab = 5;
    ValidateNavigationIndices();
    Check(g_inventorySubTab == 0, "ValidateNavigationIndices should remap inventory sub-tab 5 to 0");

    // Test 5: ValidateNavigationIndices clamps section indices
    GSLayout::gs_currentTab = 0;
    GSLayout::gs_currentSection[0] = 100;
    ValidateNavigationIndices();
    Check(GSLayout::gs_currentSection[0] == 0, "ValidateNavigationIndices should clamp section index 100 to 0");

    puts("PASS: persistence navigation validation for hidden tabs and sections");
}
static void WidgetInteractions(){
    using namespace HazzureUI;
    auto& io=ImGui::GetIO();io.DisplaySize={800,700};
    bool toggle=false,disabled=false,selected=false;float value=.25f;int integer=5,key='K';
    ImRect toggleRect,disabledRect,sliderRect,intRect,keyRect,colorRect,slotRect;
    bool keyChanged=false;
    const char* longLabel="A long descriptive option that wraps safely inside the card##private-identity";
    float startY=0;
    auto frame=[&](){
        ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
        ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({360,660});
        ImGui::Begin("Synthetic widgets",nullptr,ImGuiWindowFlags_NoSavedSettings);
        GSWidgets::GS_Toggle(longLabel,&toggle);toggleRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
        Check(toggleRect.Max.x<=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x,"long switch exceeds card");
        ImGui::BeginDisabled();GSWidgets::GS_Toggle("Disabled switch",&disabled);disabledRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};ImGui::EndDisabled();
        GSWidgets::GS_SliderFloat("Value##float",&value,0,1,"%.2f");sliderRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
        GSWidgets::GS_SliderInt("Integer",&integer,0,100,"%d");intRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
        keyChanged=GSWidgets::GS_Keybind("Key",&key);keyRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
        float color[]={.4f,.2f,.8f,1};GSWidgets::GS_ColorRow("Tint",color);colorRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
        GSWidgets::GS_InventorySlot("fixture",longLabel,"Local paint",&selected,(ImTextureID)0,.05f,23);slotRect={ImGui::GetItemRectMin(),ImGui::GetItemRectMax()};
        startY=ImGui::GetCursorScreenPos().y;
        ImGui::End();ImGui::Render();
    };
    auto click=[&](ImVec2 p){io.AddMousePosEvent(p.x,p.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();};
    frame();frame();click(toggleRect.GetCenter());Check(toggle,"switch click lost");
    click(disabledRect.GetCenter());Check(!disabled,"disabled switch mutated");
    click(slotRect.GetCenter());Check(selected,"local collection selection failed");click(slotRect.GetCenter());Check(!selected,"local collection deselection failed");
    click({sliderRect.Min.x+sliderRect.GetWidth()*.75f,sliderRect.GetCenter().y});Check(value>.65f&&value<.85f,"float slider mapping");
    click({intRect.Min.x+intRect.GetWidth()*.5f,intRect.GetCenter().y});Check(integer>=45&&integer<=55,"integer format/value mapping");
    // Keybind's final group dummy covers the whole row. Its real button is on the right.
    click({keyRect.Max.x-40,keyRect.Min.y-12});
    Check(g_bindingTarget==&key&&key=='K',"arming click captured as mouse key");
    frame();io.AddKeyEvent(ImGuiKey_Escape,true);frame();Check(!g_bindingTarget&&key=='K',"Escape failed to cancel");io.AddKeyEvent(ImGuiKey_Escape,false);frame();
    g_bindingTarget=&key;g_bindingWaitRelease=false;io.AddKeyEvent(ImGuiKey_A,true);frame();Check(key=='A'&&keyChanged&&!g_bindingTarget,"letter capture");io.AddKeyEvent(ImGuiKey_A,false);frame();
    g_bindingTarget=&key;g_bindingWaitRelease=false;io.AddKeyEvent(ImGuiKey_Delete,true);frame();Check(key==0&&keyChanged&&!g_bindingTarget,"Delete clear");io.AddKeyEvent(ImGuiKey_Delete,false);frame();
    for(int mouse=0;mouse<5;++mouse){g_bindingTarget=&key;g_bindingWaitRelease=false;io.AddMousePosEvent(700,650);io.AddMouseButtonEvent(mouse,true);frame();
        const int expected[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};Check(key==expected[mouse]&&!g_bindingTarget,"mouse key mapping");io.AddMouseButtonEvent(mouse,false);frame();}
    for(auto pair:{std::pair<ImGuiKey,int>{ImGuiKey_LeftCtrl,VK_CONTROL},{ImGuiKey_RightShift,VK_SHIFT},{ImGuiKey_LeftAlt,VK_MENU},{ImGuiKey_F12,VK_F12}}){
        g_bindingTarget=&key;g_bindingWaitRelease=false;io.AddKeyEvent(pair.first,true);frame();Check(key==pair.second&&!g_bindingTarget,"modifier/function key mapping");io.AddKeyEvent(pair.first,false);frame();}
    g_bindingTarget=&key;g_bindingWaitRelease=false;io.AddFocusEvent(false);frame();Check(!g_bindingTarget,"focus loss retained capture");io.AddFocusEvent(true);frame();
    click(colorRect.GetCenter());Check(ImGui::GetCurrentContext()->OpenPopupStack.Size>0,"color popup did not open");
    io.AddMousePosEvent(740,650);click({740,650});frame();Check(ImGui::GetCurrentContext()->OpenPopupStack.Size==0,"color popup did not dismiss");
    Check(startY<650,"test widgets unexpectedly overflow");
    puts("PASS: synthetic switch, disabled state, float/int slider, key release/Escape/Delete/M1-M5/modifiers/focus and color popup lifecycle");
}
#ifdef RUN_T003_TESTS
#include "inventory-ui-tests.inl"
#endif
int wmain(int argc,wchar_t** argv){
    if(argc<2)return 2;
    try{
        std::filesystem::path output=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(output);
        const bool baseline=argc>2&&!wcscmp(argv[2],L"--baseline");
        const bool noCaptures=argc>2&&!wcscmp(argv[2],L"--no-captures");
        Require(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"COM");
        Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;
        Require(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,device.Address(),nullptr,context.Address()),"WARP");
        ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DeltaTime=1.f/60.f;
        ImGui::StyleColorsDark();
        if(!baseline) Check(HazzureTypography::Build({0,16,13}),"real Segoe UI typography");
        Check(ImGui_ImplDX11_Init(device.p,context.p),"DX11 init");
        HazzureUI::InitializeContextDefaults();
        // Deliberately never call RenderFrame, PlayPcmBuffer, native synchronization or SaveConfigFile.
        Shared settings{};bool open=true;
        auto render=[&](int w,int h,int tab,const wchar_t* file){
            io.DisplaySize=ImVec2(float(w),float(h));HazzureUI::GSLayout::gs_currentTab=tab;
            for(int frame=0;frame<3;++frame){ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
                HazzureUI::GSLayout::DrawGSPanel(&settings,&open,w,h);
                if(!baseline){auto* panel=ImGui::FindWindowByName("##HazzureGameSense");Check(panel!=nullptr,"active panel missing");
                    Check(panel->Pos.x>=0&&panel->Pos.y>=0&&panel->Pos.x+panel->Size.x<=w+.5f&&panel->Pos.y+panel->Size.y<=h+.5f,"retained position outside viewport");}
                ImGui::Render();}
            if(!file||noCaptures)return;
            D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=desc.ArraySize=1;
            desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
            Com<ID3D11Texture2D> texture;Require(device->CreateTexture2D(&desc,nullptr,texture.Address()),"target");
            Com<ID3D11RenderTargetView> target;Require(device->CreateRenderTargetView(texture.p,nullptr,target.Address()),"view");
            const float bg[]={.025f,.03f,.045f,1};context->ClearRenderTargetView(target.p,bg);context->OMSetRenderTargets(1,&target.p,nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());SavePng(device.p,context.p,texture.p,output/file);
        };
        render(1920,1080,1,L"aim-assist-1920.png");render(1280,720,11,L"inventory-1280.png");
        if(!baseline){
            for(auto size:{ImVec2(2560,1440),ImVec2(1920,1080),ImVec2(1280,720),ImVec2(640,480),ImVec2(320,240)}){
                auto l=HazzureUI::GSLayout::ResolveGSLayout(int(size.x),int(size.y));
                Check(l.width<=size.x&&l.height<=size.y,"panel exceeds viewport");
                Check(l.bodyW>0&&l.bodyH>0&&l.columnW>0,"nonpositive body");
                for(int tab=0;tab<14;++tab)for(int section=0;section<HazzureUI::GSLayout::gs_mainTabSectionCounts[tab];++section){
                    HazzureUI::GSLayout::gs_currentSection[tab]=section;render(int(size.x),int(size.y),tab,nullptr);}
            }
            HazzureUI::GSLayout::gs_currentSection[0]=0;HazzureUI::GSLayout::gs_currentSection[1]=0;
            render(2560,1440,0,L"general-2560.png");render(640,480,1,L"compact-640.png");
            // Valid local-only fixtures: no asset loading, 3D preview or native requests.
            for(int n=0;n<4;++n){HazzureInventory::Item item{};item.skin.definition=7;item.skin.paint=180;item.skin.seed=17+n;item.skin.wear=.03f+n*.01f;
                sprintf_s(item.label,n==0?"Minha variante com nome longo para validar os limites do card %d":"Variante de estudo %d",n+1);
                Check(HazzureInventory::Add(HazzureUI::g_cfg.inventory,item),"valid local inventory fixture");}
            render(1280,720,11,L"inventory-populated-1280.png");
            ImGuiWindow* grid=nullptr;for(auto* window:ImGui::GetCurrentContext()->Windows)if(strstr(window->Name,"##profileGrid"))grid=window;
            Check(grid&&grid->InnerClipRect.GetHeight()>=152,"populated cards clipped by grid viewport at 1280x720");
            WidgetInteractions();
            NavigationInteractions();
            SearchNavigationTests();
            HiddenTabsValidationTests();
            PersistenceNavigationValidationTests();
#ifdef RUN_T003_TESTS
            InventoryUITests(device.p,context.p,output);
#endif
        }
        ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();CoUninitialize();
        printf("PASS: %u assertions; active GSLayout 240 section/viewport routes, ImGui stacks; no OS input/audio/game\n",checks);return 0;
    }catch(const std::exception& e){printf("FAIL: %s\n",e.what());return 1;}
}
