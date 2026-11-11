#include <mutex>
// Present and ResizeBuffers keep their process-resident MinHook trampolines.
// Never replace a COM vtable or remove an executing callback's trampoline.
typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);

static Present_t oPresent = nullptr;
using ResizeBuffers_t=HRESULT(__stdcall*)(IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT);
static ResizeBuffers_t oResizeBuffers;
static std::recursive_mutex g_presentationLock;

static IDXGISwapChain* g_activeSwapChain = nullptr; // owned until shutdown/switch



static ID3D11Device* g_pd3dDevice = nullptr;

static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static ID3D11RasterizerState* g_pWireframeRS = nullptr;

static HWND g_hWindow = nullptr;

static WNDPROC oWndProc = nullptr;
// The HWND owns this property until window destruction. Retain it after
// unbinding: an already-entered handler still needs its exact native fallback.
static constexpr wchar_t kOriginalWndProcProperty[]=L"Hazzure.Internal.Presentation.OriginalWndProc";

static bool g_bImGuiInitialized = false;

static bool g_bShowMenu = true;



static Shared* shared;

static HANDLE mapping;

static HMODULE self;

static std::atomic<bool> failed{false};

static std::atomic<unsigned> preparing{0};

static uintptr_t client;

static void* visibleMaterial;

static void* hiddenMaterial;

static void* armsMaterial;

static void* gradientMaterial;

static void* glowVisibleMaterial;

static void* glowHiddenMaterial;

static void* wireframeVisibleMaterial;

static void* wireframeHiddenMaterial;

static void* metallicVisibleMaterial;

static void* metallicHiddenMaterial;

static void* crystalVisibleMaterial;

static void* crystalHiddenMaterial;

static void* propMaterial;

static void* fallbackSkyMaterial;

static char skyResourceName[160]{};



// SDL3 ABI: official release-3.2.0 SDL_init/video/mouse/properties/version.h.
// https://wiki.libsdl.org/SDL3/SDL_RunOnMainThread (async, any thread)
// https://wiki.libsdl.org/SDL3/SDL_SetWindowRelativeMouseMode (main thread only)
namespace MenuMouseCapture {
struct Window;
using MainCallback=void(__cdecl*)(void*);
struct Api {
    int(__cdecl* version)();bool(__cdecl* isMain)();
    bool(__cdecl* runMain)(MainCallback,void*,bool);
    Window*(__cdecl* focus)();uint32_t(__cdecl* properties)(Window*);
    void*(__cdecl* pointer)(uint32_t,const char*,void*);
    uint32_t(__cdecl* id)(Window*);Window*(__cdecl* fromId)(uint32_t);
    bool(__cdecl* relative)(Window*);bool(__cdecl* setRelative)(Window*,bool);
    bool(__cdecl* grabbed)(Window*);bool(__cdecl* setGrabbed)(Window*,bool);
};
static Api api{};static std::atomic<bool> ready{false},queued{false};
static std::atomic<HWND> desired{nullptr};static std::atomic<int> menuKey{VK_INSERT};
static std::atomic<uint64_t> releaseEpoch{0};static uint64_t savedEpoch{};
// Only the SDL main-thread callback owns this snapshot; never retain SDL pointers.
struct Snapshot {uint32_t id{};HWND hwnd{};bool relative{},grabbed{};};
static Snapshot saved{};
static bool Bind(){
    if(ready.load())return true;
#ifdef INTERNAL_SELF_TEST
    return false; // fixtures supply the exact ABI without loading/input from SDL
#else
    static bool attempted=false;if(attempted)return false;attempted=true;
    HMODULE module{};if(!GetModuleHandleExW(0,L"SDL3.dll",&module)){LogDebug("[CURSOR] SDL3 unavailable");return false;}
    Api value{};
#define SDL_EXPORT(field,name) value.field=reinterpret_cast<decltype(value.field)>(GetProcAddress(module,name))
    SDL_EXPORT(version,"SDL_GetVersion");SDL_EXPORT(isMain,"SDL_IsMainThread");SDL_EXPORT(runMain,"SDL_RunOnMainThread");
    SDL_EXPORT(focus,"SDL_GetKeyboardFocus");SDL_EXPORT(properties,"SDL_GetWindowProperties");SDL_EXPORT(pointer,"SDL_GetPointerProperty");
    SDL_EXPORT(id,"SDL_GetWindowID");SDL_EXPORT(fromId,"SDL_GetWindowFromID");
    SDL_EXPORT(relative,"SDL_GetWindowRelativeMouseMode");SDL_EXPORT(setRelative,"SDL_SetWindowRelativeMouseMode");
    SDL_EXPORT(grabbed,"SDL_GetWindowMouseGrab");SDL_EXPORT(setGrabbed,"SDL_SetWindowMouseGrab");
#undef SDL_EXPORT
    if(!value.version||!value.isMain||!value.runMain||!value.focus||!value.properties||!value.pointer||
       !value.id||!value.fromId||!value.relative||!value.setRelative||!value.grabbed||!value.setGrabbed){
        LogDebug("[CURSOR] SDL3 required export missing");FreeLibrary(module);return false;
    }
    int version=value.version();
    if(version<3002000||version>=4000000){LogDebug("[CURSOR] Unsupported SDL ABI version=%d",version);FreeLibrary(module);return false;}
    // Keep our module reference for queued callbacks until process exit.
    api=value;ready.store(true);LogDebug("[CURSOR] SDL3 API ready version=%d",version);return true;
#endif
}
static HWND NativeWindow(Window* window){
    auto props=api.properties(window);
    return props?static_cast<HWND>(api.pointer(props,"SDL.window.win32.hwnd",nullptr)):nullptr;
}
static bool OwnUIWindow(HWND hwnd){
    DWORD process{};DWORD thread=GetWindowThreadProcessId(hwnd,&process);
    return IsWindow(hwnd)&&process==GetCurrentProcessId()&&thread==GetCurrentThreadId();
}
static bool Restore(){
    if(!saved.id)return true;
    Window* window=api.fromId(saved.id);
    if(!window||NativeWindow(window)!=saved.hwnd){saved={};return true;}
    if(!OwnUIWindow(saved.hwnd))return false;
    if(api.relative(window)!=saved.relative)api.setRelative(window,saved.relative);
    if(api.grabbed(window)!=saved.grabbed)api.setGrabbed(window,saved.grabbed);
    if(api.relative(window)!=saved.relative||api.grabbed(window)!=saved.grabbed)return false;
    LogDebug("[CURSOR] Restored window=%p relative=%d grabbed=%d thread=%lu",saved.hwnd,saved.relative,saved.grabbed,GetCurrentThreadId());
    saved={};return true;
}
static void Dispatch();
static void __cdecl Apply(void*){
    HWND target=desired.load();auto epoch=releaseEpoch.load();
    if(api.isMain()){
        Window* window=target?api.focus():nullptr;
        if(!window||NativeWindow(window)!=target||!OwnUIWindow(target))window=nullptr;
        uint32_t id=window?api.id(window):0;
        if(saved.id&&(!id||saved.id!=id||saved.hwnd!=target||savedEpoch!=epoch)){
            if(!Restore())window=nullptr;
        }
        if(window&&id){
            bool acquired=!saved.id;
            if(acquired){
                saved={id,target,api.relative(window),api.grabbed(window)};savedEpoch=epoch;
                LogDebug("[CURSOR] Captured window=%p relative=%d grabbed=%d thread=%lu",target,saved.relative,saved.grabbed,GetCurrentThreadId());
            }
            if(api.relative(window))api.setRelative(window,false);
            if(api.grabbed(window))api.setGrabbed(window,false);
            static bool previouslyReleased=false;
            bool released=!api.relative(window)&&!api.grabbed(window);
            if(acquired||released!=previouslyReleased)LogDebug("[CURSOR] Menu mouse released=%d window=%p thread=%lu",released,target,GetCurrentThreadId());
            previouslyReleased=released;
        }
    }
    queued.store(false);
    // A close/suspension published while the callback ran must not get lost.
    if(desired.load()!=target||releaseEpoch.load()!=epoch)Dispatch();
}
static void Publish(HWND hwnd){
    if(hwnd)desired.store(hwnd);
    else if(desired.exchange(nullptr))releaseEpoch.fetch_add(1);
}
static void Dispatch(){
    if(!ready.load())return;
    if(queued.exchange(true))return;
    // Never call with the presentation mutex held: SDL executes inline on main.
    static std::atomic<bool> rejected{false};
    if(!api.runMain(Apply,nullptr,false)){
        queued.store(false);if(!rejected.exchange(true))LogDebug("[CURSOR] SDL main-thread dispatch rejected");
    }else rejected.store(false);
}
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    auto original=reinterpret_cast<WNDPROC>(GetPropW(hWnd,kOriginalWndProcProperty));
    const bool capture=MenuMouseCapture::desired.load()==hWnd&&!HazzureState::g_unloading.load();
    std::unique_lock<std::recursive_mutex> guard(g_presentationLock,std::try_to_lock);
    // Window messages must never wait for render/resize, which can synchronously
    // send a message to this thread. Only touch ImGui with exclusive access.
    if (guard.owns_lock()&&!HazzureState::g_unloading.load()&&g_bImGuiInitialized&&hWnd==g_hWindow) {

        if (g_bShowMenu) {

            // 1. Bloqueia Raw Input para o CS2 nao girar a visao nem mover o cursor em background

            if (uMsg == WM_INPUT) {

                return DefWindowProc(hWnd,uMsg,wParam,lParam);

            }

        }



        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))

            return true;



        if (g_bShowMenu) {

            switch (uMsg) {

                case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:

                case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:

                case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:

                case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL: case WM_MOUSEMOVE:

                    return true;

                case WM_KEYDOWN: case WM_KEYUP:

                case WM_SYSKEYDOWN: case WM_SYSKEYUP:

                case WM_CHAR:

                    if (wParam != (HazzureUI::g_cfg.menuKey > 0 ? HazzureUI::g_cfg.menuKey : VK_INSERT)) {

                        return true;

                    }

                    break;

            }

        }

    }

    if(guard.owns_lock())guard.unlock();
    // Contention may defer ImGui input, but must not leak menu input into the
    // native game. WM_INPUT still needs default cleanup for foreground packets.
    if(capture){
        if(uMsg==WM_INPUT)return DefWindowProc(hWnd,uMsg,wParam,lParam);
        switch(uMsg){
        case WM_LBUTTONDOWN:case WM_LBUTTONUP:case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:case WM_RBUTTONUP:case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:case WM_MBUTTONUP:case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:case WM_XBUTTONUP:case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:case WM_MOUSEHWHEEL:case WM_MOUSEMOVE:return 0;
        case WM_KEYDOWN:case WM_KEYUP:case WM_SYSKEYDOWN:case WM_SYSKEYUP:case WM_CHAR:
            if(wParam!=MenuMouseCapture::menuKey.load())return 0;
        }
    }
    return original?CallWindowProc(original,hWnd,uMsg,wParam,lParam):DefWindowProc(hWnd,uMsg,wParam,lParam);

}



static void ReleaseRenderTarget(){
    if(g_pd3dDeviceContext)g_pd3dDeviceContext->OMSetRenderTargets(0,nullptr,nullptr);
    if(g_mainRenderTargetView){g_mainRenderTargetView->Release();g_mainRenderTargetView=nullptr;}
}
static bool CreateRenderTarget(IDXGISwapChain* chain){
    if(!chain||!g_pd3dDevice)return false;ID3D11Texture2D* buffer=nullptr;
    if(FAILED(chain->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&buffer))))return false;
    HRESULT result=g_pd3dDevice->CreateRenderTargetView(buffer,nullptr,&g_mainRenderTargetView);buffer->Release();return SUCCEEDED(result);
}
static void ShutdownPresentation(){
    MenuMouseCapture::Publish(nullptr);
    HazzureSceneColor::scenePass.Reset();
    HazzureBlur::pass.Reset();
    HazzureModelPreview::SetDevice(nullptr);
    HazzureTsukuyomi::SetDevice(nullptr);
    HazzureTypography::Reset();
    HazzurePreviewAssets::SetDevice(nullptr);ReleaseRenderTarget();
    if(g_bImGuiInitialized){g_bImGuiInitialized=false;ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();}
    if(g_hWindow&&oWndProc&&IsWindow(g_hWindow)&&reinterpret_cast<WNDPROC>(GetWindowLongPtr(g_hWindow,GWLP_WNDPROC))==HookedWndProc)
        SetWindowLongPtr(g_hWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(oWndProc));
    oWndProc=nullptr;g_hWindow=nullptr;
    if(g_activeSwapChain){g_activeSwapChain->Release();g_activeSwapChain=nullptr;}
    if(g_pWireframeRS){g_pWireframeRS->Release();g_pWireframeRS=nullptr;}
    if(g_pd3dDeviceContext){g_pd3dDeviceContext->Release();g_pd3dDeviceContext=nullptr;}
    if(g_pd3dDevice){g_pd3dDevice->Release();g_pd3dDevice=nullptr;}
}
// Um abort de InitializePresentation deixa g_activeSwapChain/g_hWindow nulos, entao o
// `changed` do SelectPresentation fica verdadeiro em todo Present: sem portao, cada frame
// refaz create/destroy de ImGui e loga o mesmo conflito (36 mil linhas na sessao medida).
static ULONGLONG g_nextPresentationAttempt{};static HWND g_conflictLoggedWindow{};
static bool InitializePresentation(IDXGISwapChain* chain){
    DXGI_SWAP_CHAIN_DESC description{};
    if(!chain||FAILED(chain->GetDesc(&description))||!description.OutputWindow||
       FAILED(chain->GetDevice(__uuidof(ID3D11Device),reinterpret_cast<void**>(&g_pd3dDevice))))return false;
    g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);g_hWindow=description.OutputWindow;
    if(!CreateRenderTarget(chain)){ShutdownPresentation();return false;}
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NoMouseCursorChange;ImGui::StyleColorsDark();
    if(!ImGui_ImplWin32_Init(g_hWindow)){ImGui::DestroyContext();ShutdownPresentation();return false;}
    if(!ImGui_ImplDX11_Init(g_pd3dDevice,g_pd3dDeviceContext)){ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();ShutdownPresentation();return false;}
    auto current=reinterpret_cast<WNDPROC>(GetWindowLongPtr(g_hWindow,GWLP_WNDPROC));
    auto saved=reinterpret_cast<WNDPROC>(GetPropW(g_hWindow,kOriginalWndProcProperty));
    // A later subclass may still forward to us. Do not replace its chain or
    // save that subclass as our original (that would create a callback cycle).
    if(!current||(saved&&current!=saved&&current!=HookedWndProc)||(!saved&&current==HookedWndProc)){
        if(g_conflictLoggedWindow!=g_hWindow){
            g_conflictLoggedWindow=g_hWindow;
            LogDebug("[PRESENT] WndProc ownership conflict window=%p current=%p saved=%p hooked=%p",g_hWindow,reinterpret_cast<void*>(current),reinterpret_cast<void*>(saved),reinterpret_cast<void*>(HookedWndProc));
        }
        ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();ShutdownPresentation();return false;
    }
    if(!saved){
        saved=current;
        if(!SetPropW(g_hWindow,kOriginalWndProcProperty,reinterpret_cast<HANDLE>(saved))){
            ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();ShutdownPresentation();return false;
        }
    }
    oWndProc=saved;
    if(current!=HookedWndProc&&!SetWindowLongPtr(g_hWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(HookedWndProc))){
        ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();ShutdownPresentation();return false;
    }
    g_bImGuiInitialized=true;g_conflictLoggedWindow=nullptr;g_nextPresentationAttempt=0;
    MenuMouseCapture::Bind();MenuMouseCapture::Publish(g_bShowMenu?g_hWindow:nullptr);
    chain->AddRef();g_activeSwapChain=chain;
    LogDebug("[PRESENT] Active chain=%p window=%p device=%p renderThread=%lu windowThread=%lu",chain,g_hWindow,g_pd3dDevice,GetCurrentThreadId(),GetWindowThreadProcessId(g_hWindow,nullptr));
    HazzurePreviewAssets::SetDevice(g_pd3dDevice);
    HazzureModelPreview::SetDevice(g_pd3dDevice);
    HazzureTsukuyomi::SetDevice(g_pd3dDevice);
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_WIREFRAME;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;
    g_pd3dDevice->CreateRasterizerState(&raster,&g_pWireframeRS);return true;
}
// Called under g_presentationLock. Ignore unrelated process windows; a new chain
// for the same window replaces the old one, even when the device is unchanged.
static bool SelectPresentation(IDXGISwapChain* chain){
    DXGI_SWAP_CHAIN_DESC description{};DWORD process{};
    if(!chain||FAILED(chain->GetDesc(&description))||!IsWindow(description.OutputWindow))return false;
    GetWindowThreadProcessId(description.OutputWindow,&process);
    if(process!=GetCurrentProcessId())return false;
    if(g_hWindow&&description.OutputWindow!=g_hWindow&&IsWindow(g_hWindow)&&
       GetAncestor(description.OutputWindow,GA_ROOT)!=GetAncestor(GetForegroundWindow(),GA_ROOT))return false;
    ID3D11Device* device=nullptr;
    if(FAILED(chain->GetDevice(__uuidof(ID3D11Device),reinterpret_cast<void**>(&device))))return false;
    bool changed=chain!=g_activeSwapChain||description.OutputWindow!=g_hWindow||device!=g_pd3dDevice;
    device->Release();
    if(changed){
        auto now=GetTickCount64();
        if(now<g_nextPresentationAttempt)return false;
        ShutdownPresentation();
        if(InitializePresentation(chain))return true;
        g_nextPresentationAttempt=GetTickCount64()+500;
        return false;
    }
    return g_bImGuiInitialized&&(g_mainRenderTargetView||CreateRenderTarget(chain));
}
static HRESULT __stdcall ResizeBuffersHook(IDXGISwapChain* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags){
    std::unique_lock<std::recursive_mutex> guard(g_presentationLock);
    bool active=chain==g_activeSwapChain;
    if(active){
        HazzureSceneColor::scenePass.ReleaseTargets();HazzureBlur::pass.Reset();
        HazzureModelPreview::ReleaseTargets();ReleaseRenderTarget();
    }
    HRESULT result=oResizeBuffers?oResizeBuffers(chain,count,width,height,format,flags):DXGI_ERROR_INVALID_CALL;
    if(active){
        if(result==DXGI_ERROR_DEVICE_REMOVED||result==DXGI_ERROR_DEVICE_RESET)ShutdownPresentation();
        // A rejected resize retains the old buffer. Reacquire it on either result.
        else if(g_bImGuiInitialized&&!HazzureState::g_unloading.load())CreateRenderTarget(chain);
        LogDebug("[PRESENT] Resize chain=%p %ux%u result=0x%08X",chain,width,height,static_cast<unsigned>(result));
    }
    guard.unlock();MenuMouseCapture::Dispatch();
    return result;
}

static HRESULT CallOriginalPresent(IDXGISwapChain* chain,UINT interval,UINT flags,std::unique_lock<std::recursive_mutex>& guard){
    // In particular, vsync and cross-thread window dispatch run without our
    // mutex. Window input remains available throughout the native Present.
    guard.unlock();MenuMouseCapture::Dispatch();
    HRESULT result=oPresent?oPresent(chain,interval,flags):DXGI_ERROR_INVALID_CALL;
    if(result==DXGI_ERROR_DEVICE_REMOVED||result==DXGI_ERROR_DEVICE_RESET){
        guard.lock();if(chain==g_activeSwapChain)ShutdownPresentation();
        guard.unlock();MenuMouseCapture::Dispatch();
    }
    return result;
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    std::unique_lock<std::recursive_mutex> guard(g_presentationLock);
    if(HazzureState::g_unloading.load()){
        g_bShowMenu=false;
        if(g_bImGuiInitialized)ShutdownPresentation();
        return CallOriginalPresent(pSwapChain,SyncInterval,Flags,guard);
    }
    // DXGI_PRESENT_TEST probes must not create resources, consume hotkeys or render.
    if((Flags&DXGI_PRESENT_TEST)||!SelectPresentation(pSwapChain))
        return CallOriginalPresent(pSwapChain,SyncInterval,Flags,guard);
    auto overlayStarted=HazzurePerformance::Clock();
    static ULONGLONG lastHeartbeat{};
    auto now=GetTickCount64();
    if(now-lastHeartbeat>=5000){
        LogDebug("[PRESENT] Frame chain=%p window=%p device=%p menu=%d",pSwapChain,g_hWindow,g_pd3dDevice,g_bShowMenu);
        lastHeartbeat=now;
    }

    int menuKey = HazzureUI::g_cfg.menuKey > 0 ? HazzureUI::g_cfg.menuKey : VK_INSERT;

    if(HazzureState::g_unloading.load())g_bShowMenu=false;

    else if (!HazzureUI::g_bindingTarget && (GetAsyncKeyState(menuKey) & 1)) {

        g_bShowMenu = !g_bShowMenu;

    }



    MenuMouseCapture::menuKey.store(menuKey);
    MenuMouseCapture::Publish(g_bShowMenu?g_hWindow:nullptr);
    if (g_bImGuiInitialized && g_mainRenderTargetView) {

        ImGuiIO& io = ImGui::GetIO();



        if (g_bShowMenu) {

            io.MouseDrawCursor = true;

            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

            // SDL main-thread capture owns relative mode and mouse confinement.

        } else {

            io.MouseDrawCursor = false;

        }



        HazzureTypography::Settings typography{HazzureUI::g_cfg.uiFontFamily,HazzureUI::g_cfg.uiFontSize,HazzureUI::g_cfg.espFontSize};
        if(HazzureTypography::Changed(typography)&&HazzureTypography::Valid(typography)){
            ImGui_ImplDX11_InvalidateDeviceObjects();HazzureTypography::Build(typography);
        }
        ImGui_ImplDX11_NewFrame();

        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();



        if(!HazzureState::g_unloading.load()){
            const auto& cfg=HazzureUI::g_cfg;
            HazzureSceneColor::Settings colors{cfg.sceneColorEnabled,cfg.sceneSaturation,cfg.sceneContrast,cfg.sceneTemperature,cfg.sceneTint,cfg.sceneColorCurve};
            if(HazzureSceneColor::Active(colors)){
                ID3D11Texture2D* buffer=nullptr;
                if(SUCCEEDED(pSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&buffer)))){
                    HazzureSceneColor::scenePass.Apply(g_pd3dDevice,g_pd3dDeviceContext,buffer,g_mainRenderTargetView,colors);buffer->Release();
                }
            }else HazzureSceneColor::scenePass.Apply(nullptr,nullptr,nullptr,nullptr,colors);
        }else HazzureSceneColor::scenePass.Reset();
        static bool blurWasEnabled=false;
        bool blurEnabled=HazzureUI::g_cfg.glassBlur&&!HazzureState::g_unloading.load();
        if(blurWasEnabled&&!blurEnabled)HazzureBlur::pass.Reset();
        blurWasEnabled=blurEnabled;
        if(g_bShowMenu&&blurEnabled){
            ID3D11Texture2D* buffer=nullptr;
            if(SUCCEEDED(pSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&buffer)))){
                HazzureBlur::pass.Capture(g_pd3dDevice,g_pd3dDeviceContext,buffer);buffer->Release();
            }else HazzureBlur::pass.ReleaseTargets();
        }else HazzureBlur::pass.ReleaseTargets();

        HazzureUI::RenderFrame(shared, client, &g_bShowMenu, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));



        ImGui::Render();

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    }



    MenuMouseCapture::Publish(g_bShowMenu&&!HazzureState::g_unloading.load()?g_hWindow:nullptr);
    HazzurePerformance::End(HazzurePerformance::Stage::Overlay,overlayStarted);
    HazzureTelemetry::g_metrics.renderFrameTimeMs=HazzurePerformance::Read(HazzurePerformance::Stage::Overlay).average;
    return CallOriginalPresent(pSwapChain,SyncInterval,Flags,guard);

}

// Both originals are published before either callback is enabled. On enable
// failure retain allocated trampolines: a callback may already be in flight.
static bool InstallPresentationHooks(void** methods){
    std::lock_guard<std::recursive_mutex> guard(g_presentationLock);
    if(!methods||oPresent||oResizeBuffers)return false;
    void* presentTarget=methods[8];void* resizeTarget=methods[13];
    auto presentStatus=MH_CreateHook(presentTarget,reinterpret_cast<void*>(hkPresent),reinterpret_cast<void**>(&oPresent));
    if(presentStatus!=MH_OK){LogDebug("[PRESENT] Create Present failed: %d",presentStatus);return false;}
    auto resizeStatus=MH_CreateHook(resizeTarget,reinterpret_cast<void*>(ResizeBuffersHook),reinterpret_cast<void**>(&oResizeBuffers));
    if(resizeStatus!=MH_OK){
        MH_RemoveHook(presentTarget);oPresent=nullptr;
        LogDebug("[PRESENT] Create ResizeBuffers failed: %d",resizeStatus);return false;
    }
    resizeStatus=MH_EnableHook(resizeTarget);
    presentStatus=resizeStatus==MH_OK?MH_EnableHook(presentTarget):MH_ERROR_DISABLED;
    LogDebug("[PRESENT] Resident detours Present=%d ResizeBuffers=%d",presentStatus,resizeStatus);
    if(presentStatus!=MH_OK||resizeStatus!=MH_OK)return false;
    HazzureState::RegisterHook("Present",presentTarget,reinterpret_cast<void*>(hkPresent),reinterpret_cast<void*>(oPresent));
    HazzureState::RegisterHook("ResizeBuffers",resizeTarget,reinterpret_cast<void*>(ResizeBuffersHook),reinterpret_cast<void*>(oResizeBuffers));
    return true;
}






using Draw=void(__fastcall*)(void*,void*,void*,int,void*,void*,void*,void*);

static Draw original;

using Factory=void*(*)(const char*,int*);

using LoadKV=bool(*)(void*,void*,const char*,const void*,const char*,unsigned);

using CreateMaterial=void*(*)(void*,void*,const char*,void*,int,int,int,int,int,int);

using FindMaterialFromResource=void*(*)(void*,void**,const char*);

static LoadKV loadKV;

static CreateMaterial createMaterial;

static FindMaterialFromResource findMaterialFromResource;

static void* materialSystem;

static void* resourceSystem;

using InsertPath=const char*(*)(void*,int,const char*,int,bool);

using PurgePath=void(*)(void*,int);

using PrecacheMaterial=void*(*)(void*,void*,const char*);

static InsertPath insertPath;

static PurgePath purgePath;

static PrecacheMaterial precacheMaterial;

using ArenaCtor=void*(*)(void*,unsigned int,bool);

static ArenaCtor arenaCtor;

static thread_local bool nested;



