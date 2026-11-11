// Exercises the production presentation code in our own process; hidden WARP
// windows only. Never calls an overlay frame, consumes input or touches the game.
#define INTERNAL_SELF_TEST
#define main ExistingInternalSelfTestMain
#include "../src/Internal.cpp"
#undef main
#include <thread>
static unsigned checks{},presentCalls{};
static Present_t realPresent{};
static IDXGISwapChain* lastChain{};
static UINT lastInterval{},lastFlags{};
static HRESULT forcedPresent=S_OK;
static bool probePresentLock{},presentLockAvailable{};
static HANDLE nativeEntered{},lifecycleFinished{};
static constexpr UINT kLifecycleMessage=WM_APP+117;
static LRESULT CALLBACK NativeTestWndProc(HWND window,UINT message,WPARAM w,LPARAM l){
    if(message==kLifecycleMessage){
        SetEvent(nativeEntered);
        return WaitForSingleObject(lifecycleFinished,10000)==WAIT_OBJECT_0?0x1111:-1;
    }
    return DefWindowProcW(window,message,w,l);
}
static LRESULT CALLBACK OtherTestWndProc(HWND window,UINT message,WPARAM w,LPARAM l){
    if(message==kLifecycleMessage)return 0x2222;
    return DefWindowProcW(window,message,w,l);
}
static bool ConcurrentLifecycle(HWND oldWindow,IDXGISwapChain* replacement,bool contended){
    HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    nativeEntered=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    lifecycleFinished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    bool changed=false;
    std::thread render([&]{
        std::unique_lock<std::recursive_mutex> guard(g_presentationLock,std::defer_lock);
        if(contended)guard.lock();SetEvent(ready);
        if(WaitForSingleObject(nativeEntered,10000)==WAIT_OBJECT_0){
            if(!guard.owns_lock())guard.lock();
            ShutdownPresentation();changed=InitializePresentation(replacement);
        }
        SetEvent(lifecycleFinished);
    });
    bool readyOk=WaitForSingleObject(ready,10000)==WAIT_OBJECT_0;
    // Deliberately invoke the installed handler on the window's owner thread.
    // Its exact native fallback waits for a concurrent shutdown/rebind to finish.
    LRESULT result=readyOk?HookedWndProc(oldWindow,kLifecycleMessage,0,0):-1;
    render.join();CloseHandle(ready);CloseHandle(nativeEntered);CloseHandle(lifecycleFinished);
    nativeEntered=lifecycleFinished=nullptr;
    return readyOk&&changed&&result==0x1111;
}
static HRESULT __stdcall CountPresent(IDXGISwapChain* chain,UINT interval,UINT flags){
    ++presentCalls;lastChain=chain;lastInterval=interval;lastFlags=flags;
    if(probePresentLock){
        std::thread probe([]{std::unique_lock<std::recursive_mutex> guard(g_presentationLock,std::try_to_lock);presentLockAvailable=guard.owns_lock();});
        probe.join();
    }
    return forcedPresent!=S_OK?forcedPresent:realPresent(chain,interval,flags);
}
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static bool MakeChain(HWND window,ID3D11Device** device,IDXGISwapChain** chain){
    DXGI_SWAP_CHAIN_DESC desc{};desc.BufferDesc.Width=64;desc.BufferDesc.Height=64;
    desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=1;
    desc.OutputWindow=window;desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    return SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,
        D3D11_SDK_VERSION,&desc,chain,device,nullptr,nullptr));
}
static bool TargetSize(UINT width,UINT height){
    if(!g_mainRenderTargetView)return false;ID3D11Resource* resource{};
    g_mainRenderTargetView->GetResource(&resource);ID3D11Texture2D* texture{};
    HRESULT hr=resource->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture));resource->Release();
    if(FAILED(hr))return false;D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);texture->Release();
    return desc.Width==width&&desc.Height==height;
}
// SDL contract fixture: mutations are memory-only and never invoke SDL/OS input.
namespace MouseFixture {
using namespace MenuMouseCapture;
struct FakeWindow {uint32_t id;HWND hwnd;bool relative,grabbed,alive;};
static FakeWindow windows[2]{};static Window* focused{};
static MainCallback pending{};static void* pendingData{};
static unsigned schedules{},mutations{},wrongThread{};static DWORD mainThread{};
static bool rejectDispatch{},failRestore{},closeDuringDisable{},inlineDispatch{},dispatchUnlocked=true;
static FakeWindow* Cast(Window* w){return reinterpret_cast<FakeWindow*>(w);}
static bool __cdecl IsMain(){return GetCurrentThreadId()==mainThread;}
static void OnMain(){if(!IsMain())++wrongThread;}
static int __cdecl Version(){return 3005000;}
static bool __cdecl Run(MainCallback callback,void* data,bool wait){
    if(wait)++wrongThread;
    std::thread probe([]{std::unique_lock<std::recursive_mutex> guard(g_presentationLock,std::try_to_lock);dispatchUnlocked=dispatchUnlocked&&guard.owns_lock();});probe.join();
    ++schedules;if(rejectDispatch)return false;
    if(inlineDispatch&&IsMain())callback(data);else{pending=callback;pendingData=data;}return true;
}
static Window* __cdecl Focus(){OnMain();return focused;}
static uint32_t __cdecl Properties(Window* w){OnMain();return Cast(w)->id;}
static Window* __cdecl FromId(uint32_t id){OnMain();for(auto& w:windows)if(w.id==id&&w.alive)return reinterpret_cast<Window*>(&w);return nullptr;}
static void* __cdecl Pointer(uint32_t id,const char* name,void* fallback){
    OnMain();if(strcmp(name,"SDL.window.win32.hwnd")!=0)return fallback;
    for(auto& w:windows)if(w.id==id&&w.alive)return w.hwnd;return fallback;
}
static uint32_t __cdecl Id(Window* w){OnMain();return Cast(w)->id;}
static bool __cdecl Relative(Window* w){OnMain();return Cast(w)->relative;}
static bool __cdecl Grabbed(Window* w){OnMain();return Cast(w)->grabbed;}
static bool __cdecl SetRelative(Window* w,bool enabled){
    OnMain();++mutations;if(failRestore&&enabled)return false;Cast(w)->relative=enabled;
    if(closeDuringDisable&&!enabled){closeDuringDisable=false;Publish(nullptr);}return true;
}
static bool __cdecl SetGrabbed(Window* w,bool enabled){OnMain();++mutations;Cast(w)->grabbed=enabled;return true;}
static void Drain(){while(pending){auto callback=pending;auto data=pendingData;pending=nullptr;callback(data);}}
#define MOUSE_CHECK(condition) do{++checks;if(!(condition)){printf("FAIL mouse fixture line %d: %s\n",__LINE__,#condition);return false;}}while(0)
static bool Check(HWND first,HWND second,IDXGISwapChain* productionChain){
    mainThread=GetCurrentThreadId();windows[0]={11,first,true,true,true};windows[1]={22,second,true,true,true};
    auto one=reinterpret_cast<Window*>(&windows[0]);auto two=reinterpret_cast<Window*>(&windows[1]);focused=one;
    api={Version,IsMain,Run,Focus,Properties,Pointer,Id,FromId,Relative,SetRelative,Grabbed,SetGrabbed};ready.store(true);
    saved={};queued.store(false);Publish(nullptr);
    for(unsigned i=0;i<200;++i){Publish(first);Dispatch();}
    MOUSE_CHECK(!(schedules!=1||mutations||!queued.load()));Drain();
    MOUSE_CHECK(!(windows[0].relative||windows[0].grabbed||saved.id!=11||wrongThread));
    // Reassert menu capture if the engine requests relative mode again, retaining
    // the first original snapshot for restoration.
    windows[0].relative=windows[0].grabbed=true;Dispatch();Drain();
    MOUSE_CHECK(!(windows[0].relative||windows[0].grabbed||!saved.relative||!saved.grabbed));
    Publish(nullptr);Dispatch();Drain();MOUSE_CHECK(!(!windows[0].relative||!windows[0].grabbed||saved.id));
    windows[0].relative=windows[0].grabbed=false;auto before=mutations;
    Publish(first);Dispatch();Drain();Publish(nullptr);Dispatch();Drain();
    MOUSE_CHECK(!(windows[0].relative||windows[0].grabbed||mutations!=before||saved.id));
    // An SDL focus window belonging to another HWND cannot authorize mutation.
    Publish(second);Dispatch();Drain();MOUSE_CHECK(!(mutations!=before||saved.id));
    windows[0].relative=windows[0].grabbed=true;Publish(first);Dispatch();Drain();
    focused=two;Publish(second);Dispatch();Drain();
    MOUSE_CHECK(!(!windows[0].relative||!windows[0].grabbed||windows[1].relative||windows[1].grabbed||saved.id!=22));
    Publish(nullptr);Dispatch();Drain();MOUSE_CHECK(!(!windows[1].relative||!windows[1].grabbed||saved.id));
    // Failed restoration retains ownership; do not acquire a second window.
    focused=one;Publish(first);Dispatch();Drain();failRestore=true;
    focused=two;Publish(second);Dispatch();Drain();
    MOUSE_CHECK(!(saved.id!=11||!windows[1].relative));
    failRestore=false;Dispatch();Drain();MOUSE_CHECK(!(saved.id!=22||!windows[0].relative||windows[1].relative));
    Publish(nullptr);Dispatch();Drain();
    // Destroyed/recycled SDL identities must never be dereferenced or restored.
    focused=one;Publish(first);Dispatch();Drain();windows[0].alive=false;before=mutations;
    Publish(nullptr);Dispatch();Drain();MOUSE_CHECK(!(saved.id||mutations!=before));
    windows[0].alive=true;windows[0].relative=windows[0].grabbed=true;
    Publish(first);Dispatch();Drain();windows[0].hwnd=second;before=mutations;
    Publish(nullptr);Dispatch();Drain();MOUSE_CHECK(!(saved.id||mutations!=before));
    windows[0].hwnd=first;windows[0].relative=windows[0].grabbed=true;
    // Dispatch from a rendering thread is queued and cannot mutate until main.
    before=mutations;std::thread render([first]{Publish(first);Dispatch();});render.join();
    MOUSE_CHECK(!(mutations!=before||!pending));Drain();MOUSE_CHECK(!(windows[0].relative||wrongThread));
    Publish(nullptr);Dispatch();Drain();
    rejectDispatch=true;Publish(first);Dispatch();MOUSE_CHECK(!(queued.load()||saved.id));
    rejectDispatch=false;Dispatch();Drain();MOUSE_CHECK(!(saved.id!=11));
    Publish(nullptr);Dispatch();Drain();
    // A close published while the main-thread callback runs must get a follow-up.
    closeDuringDisable=true;Publish(first);Dispatch();Drain();
    MOUSE_CHECK(!(saved.id||!windows[0].relative||!windows[0].grabbed||queued.load()));
    // Close and reopen coalesced to the same HWND must still unwind the old
    // capture session before taking a fresh original-state snapshot.
    Publish(first);Dispatch();Drain();before=mutations;
    Publish(nullptr);Publish(first);Dispatch();Drain();
    MOUSE_CHECK(!(mutations!=before+4||windows[0].relative||windows[0].grabbed||saved.id!=11));
    Publish(nullptr);Dispatch();Drain();
    inlineDispatch=true;Publish(first);Dispatch();MOUSE_CHECK(!(windows[0].relative||saved.id!=11));
    Publish(nullptr);Dispatch();MOUSE_CHECK(!(saved.id||!windows[0].relative||!windows[0].grabbed));
    // Exercise production Dispatch at the actual trampoline boundary as well.
    inlineDispatch=false;Publish(first);
    hkPresent(productionChain,0,DXGI_PRESENT_TEST);Drain();
    MOUSE_CHECK(!(windows[0].relative||!dispatchUnlocked||wrongThread));
    Publish(nullptr);Dispatch();Drain();ready.store(false);api={};saved={};queued.store(false);
    printf("PASS: SDL3 main-thread contract fixture; %u coalesced dispatches, %u memory-only mutations; exact prior state, focus/HWND identity, two-window transfer, failed restore, destroyed/recycled windows, inline/async calls and close-during-callback.\n",schedules,mutations);
    return true;
}
#undef MOUSE_CHECK
}
int main(){
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    WNDCLASSW wc{};wc.lpfnWndProc=NativeTestWndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"HazzurePresentationTest";
    CHECK(RegisterClassW(&wc)!=0);
    HWND window=CreateWindowW(wc.lpszClassName,L"hidden",WS_OVERLAPPEDWINDOW,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);
    HWND other=CreateWindowW(wc.lpszClassName,L"hidden-other",WS_OVERLAPPEDWINDOW,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);
    CHECK(window&&other&&!IsWindowVisible(window)&&!IsWindowVisible(other));
    SetWindowLongPtr(other,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(OtherTestWndProc));
    auto originalWindowProc=reinterpret_cast<WNDPROC>(GetWindowLongPtr(window,GWLP_WNDPROC));
    ID3D11Device *firstDevice{},*secondDevice{};IDXGISwapChain *first{},*second{},*unrelated{},*sameDevice{};
    CHECK(MakeChain(window,&firstDevice,&first));
    CHECK(MakeChain(window,&secondDevice,&second));
    IDXGIFactory* factory{};CHECK(SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory),reinterpret_cast<void**>(&factory))));
    DXGI_SWAP_CHAIN_DESC desc{};CHECK(SUCCEEDED(second->GetDesc(&desc)));
    CHECK(SUCCEEDED(factory->CreateSwapChain(secondDevice,&desc,&sameDevice)));desc.OutputWindow=other;
    CHECK(SUCCEEDED(factory->CreateSwapChain(secondDevice,&desc,&unrelated)));factory->Release();
    CHECK(MH_Initialize()==MH_OK);
    void** methods=*reinterpret_cast<void***>(first);
    CHECK(InstallPresentationHooks(methods));realPresent=oPresent;oPresent=CountPresent;
    CHECK(!InstallPresentationHooks(methods));
    // Actual COM calls pass through the installed process-wide detours on BOTH
    // chains. A TEST present must never initialize resources or select a chain.
    first->Present(0,DXGI_PRESENT_TEST);CHECK(presentCalls==1&&lastChain==first&&lastFlags==DXGI_PRESENT_TEST&&!g_activeSwapChain);
    second->Present(0,DXGI_PRESENT_TEST);CHECK(presentCalls==2&&lastChain==second&&!g_activeSwapChain);
    probePresentLock=true;first->Present(0,DXGI_PRESENT_TEST);probePresentLock=false;
    CHECK(presentLockAvailable); // native Present/vsync must not starve WndProc
    CHECK(SelectPresentation(first));CHECK(g_activeSwapChain==first&&g_pd3dDevice==firstDevice&&TargetSize(64,64));
    CHECK(reinterpret_cast<WNDPROC>(GetWindowLongPtr(window,GWLP_WNDPROC))==HookedWndProc);
    CHECK(!SelectPresentation(unrelated)&&g_activeSwapChain==first);
    auto* target=g_mainRenderTargetView;
    CHECK(SUCCEEDED(unrelated->ResizeBuffers(1,80,48,DXGI_FORMAT_UNKNOWN,0)));
    CHECK(g_mainRenderTargetView==target&&g_activeSwapChain==first);
    CHECK(SUCCEEDED(first->ResizeBuffers(1,96,72,DXGI_FORMAT_UNKNOWN,0)));
    CHECK(g_activeSwapChain==first&&TargetSize(96,72));
    // Holding the old backbuffer makes DXGI reject resize. Our target must be
    // reacquired from that old buffer and the next selection must remain usable.
    ID3D11Texture2D* held{};CHECK(SUCCEEDED(first->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&held))));
    CHECK(FAILED(first->ResizeBuffers(1,100,80,DXGI_FORMAT_UNKNOWN,0)));
    CHECK(TargetSize(96,72));held->Release();
    CHECK(SelectPresentation(second));CHECK(g_activeSwapChain==second&&g_pd3dDevice==secondDevice&&TargetSize(64,64));
    CHECK(SUCCEEDED(second->ResizeBuffers(1,120,88,DXGI_FORMAT_UNKNOWN,0))&&TargetSize(120,88));
    CHECK(SelectPresentation(sameDevice)&&g_activeSwapChain==sameDevice&&g_pd3dDevice==secondDevice&&TargetSize(64,64));
    CHECK(SelectPresentation(first)&&g_activeSwapChain==first&&TargetSize(96,72));
    // A synthetic device-loss return tests cleanup without removing a real GPU.
    forcedPresent=DXGI_ERROR_DEVICE_REMOVED;
    CHECK(first->Present(0,DXGI_PRESENT_TEST)==DXGI_ERROR_DEVICE_REMOVED);
    CHECK(!g_activeSwapChain&&!g_bImGuiInitialized&&!g_pd3dDevice&&!g_mainRenderTargetView);
    CHECK(reinterpret_cast<WNDPROC>(GetWindowLongPtr(window,GWLP_WNDPROC))==originalWindowProc);
    forcedPresent=S_OK;CHECK(SelectPresentation(second));
    CHECK(ConcurrentLifecycle(window,unrelated,true));
    CHECK(g_activeSwapChain==unrelated&&g_hWindow==other);
    CHECK(HookedWndProc(other,kLifecycleMessage,0,0)==0x2222);
    {
        std::lock_guard<std::recursive_mutex> guard(g_presentationLock);
        ShutdownPresentation();CHECK(InitializePresentation(second));
    }
    CHECK(ConcurrentLifecycle(window,unrelated,false));
    CHECK(g_activeSwapChain==unrelated&&g_hWindow==other);
    {
        std::lock_guard<std::recursive_mutex> guard(g_presentationLock);
        ShutdownPresentation();CHECK(InitializePresentation(second));
    }
    CHECK(MouseFixture::Check(window,other,second));
    // Replacing a destroyed output window must permit a new owned window.
    DestroyWindow(window);window=nullptr;
    CHECK(SelectPresentation(unrelated)&&g_activeSwapChain==unrelated&&g_hWindow==other);
    HazzureState::g_unloading.store(true);auto before=presentCalls;
    second->Present(1,0);
    CHECK(presentCalls==before+1&&lastChain==second&&lastInterval==1&&lastFlags==0);
    CHECK(!g_activeSwapChain&&!g_bImGuiInitialized&&!g_pd3dDevice&&!g_mainRenderTargetView);
    first->Present(0,DXGI_PRESENT_TEST);CHECK(presentCalls==before+2&&!g_bImGuiInitialized);
    // Teardown is legal here: this fixture has no competing rendering threads.
    CHECK(MH_DisableHook(MH_ALL_HOOKS)==MH_OK&&MH_Uninitialize()==MH_OK);
    oPresent=nullptr;oResizeBuffers=nullptr;first->Release();second->Release();unrelated->Release();sameDevice->Release();firstDevice->Release();secondDevice->Release();
    DestroyWindow(window);DestroyWindow(other);UnregisterClassW(wc.lpszClassName,wc.hInstance);
    printf("PASS: %u checks; actual resident Present/ResizeBuffers detours across two hidden WARP devices/chains, unrelated-chain isolation, successful/rejected resize, synthetic device loss, resident suspension and deterministic two-thread WndProc/shutdown/rebind with exact per-window fallback and unlocked native Present.\n",checks);
    return 0;
}
