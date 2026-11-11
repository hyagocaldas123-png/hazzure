import re

path = r'c:\Codex2\gpt\HazzureInternal\src\Internal.cpp'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Fix HookedWndProc
old_wndproc = '''static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_bImGuiInitialized) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        if (g_bShowMenu) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse || io.WantCaptureKeyboard)
                return true;
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}'''

new_wndproc = '''static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_bImGuiInitialized) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        if (g_bShowMenu) {
            switch (uMsg) {
                case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
                case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
                case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL: case WM_MOUSEMOVE:
                case WM_KEYDOWN:     case WM_KEYUP:     case WM_SYSKEYDOWN: case WM_SYSKEYUP:
                case WM_CHAR:
                    return true;
            }
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}'''

# 2. Fix MakeMaterial and Prepare
old_makemat = '''static void* MakeMaterial(const char* name,bool ignoreDepth,bool translucent) {
    // KV3 owns allocations created by the engine. These few objects are retained for
    // the process lifetime, never freed using an incompatible allocator.
    void* arena=VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!arena)return nullptr;
    Message("Constructing KV3 arena");
    arenaCtor(arena,0);'''

new_makemat = '''static void* MakeMaterial(const char* name,bool ignoreDepth,bool translucent) {
    if(!arenaCtor || !loadKV || !createMaterial || !materialSystem) return nullptr;
    void* arena=VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!arena)return nullptr;
    Message("Constructing KV3 arena");
    __try {
        arenaCtor(arena,0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        VirtualFree(arena, 0, MEM_RELEASE);
        return nullptr;
    }'''

old_prepare = '''static bool Prepare() {
    bool ok=false;'''

new_prepare = '''static bool Prepare() {
    if(!arenaCtor || !loadKV || !createMaterial || !materialSystem) {
        Message("Chams passthrough (material subsystem unvalidated)");
        return false;
    }
    bool ok=false;'''

# 3. Fix Start()
old_start_tail = '''    client=reinterpret_cast<uintptr_t>(cl);
    if(client) arenaCtor=reinterpret_cast<ArenaCtor>(client+0x19B4860);

    MH_Initialize();

    if(scene) {
        auto draw=reinterpret_cast<unsigned char*>(scene)+0x59FC0;
        const unsigned char expected[]={0x48,0x8b,0xc4,0x48,0x89,0x50,0x10,0x48,0x89,0x48,0x08};
        if(memcmp(draw,expected,sizeof(expected))==0) {
            MH_CreateHook(draw,Hook,reinterpret_cast<void**>(&original));
        } else {
            Message("Aviso: Draw prologue mudou, pulando hook de draw");
        }
    }

    if(SkinAddressesValid()) {
        MH_CreateHook(reinterpret_cast<void*>(client+SKIN_FRAME.rva),FrameHook,reinterpret_cast<void**>(&originalFrame));
    } else {
        Message("Aviso: Skin function identity mudou, pulando hook de skins");
    }

    if(shared) {
        shared->audioProtocol=0;shared->kills=0;shared->headshots=0;shared->skyBrightness=0.7f;shared->skyColour=0xffb8b8b8;
    }

    if(client) {
        const unsigned char skyPrologue[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x55,0x41,0x57,0x48,0x8b,0xec,0x48,0x83,0xec};
        if(memcmp(reinterpret_cast<void*>(client+0x282390),skyPrologue,sizeof(skyPrologue))!=0)climateFault=true;
        if(!climateFault) {
            MH_CreateHook(reinterpret_cast<void*>(client+0x282390),SkyUpdateHook,reinterpret_cast<void**>(&originalSkyUpdate));
        }
        if(AudioAddressesValid()) {
            if(MH_CreateHook(reinterpret_cast<void*>(client+0x9e2960),AudioEventHook,reinterpret_cast<void**>(&originalEvent))==MH_OK && shared) {
                shared->audioProtocol=1;
            }
        }
    }

    if(scene) {
        auto skyDraw=reinterpret_cast<unsigned char*>(scene)+0x154fb0;
        const unsigned char skyDrawExpected[]={0x45,0x85,0xc9,0x0f,0x8e,0x1f,0x07,0x00,0x00,0x4c,0x8b,0xdc,0x55,0x41,0x56,0x49,0x8d,0xab,0x58,0xfc,0xff,0xff,0x48,0x81};
        if(memcmp(skyDraw,skyDrawExpected,sizeof(skyDrawExpected))==0){
            MH_CreateHook(skyDraw,SkyDrawHook,reinterpret_cast<void**>(&originalSkyDraw));
        } else {
            climateFault=true;ClimateMessage(-3,"Aviso: Renderizador de ceu mudou");
        }
    }'''

new_start_tail = '''    client=reinterpret_cast<uintptr_t>(cl);
    if(client && modulesMatched) arenaCtor=reinterpret_cast<ArenaCtor>(client+0x19B4860);
    else arenaCtor=nullptr;

    MH_Initialize();

    if(scene && modulesMatched) {
        auto draw=reinterpret_cast<unsigned char*>(scene)+0x59FC0;
        const unsigned char expected[]={0x48,0x8b,0xc4,0x48,0x89,0x50,0x10,0x48,0x89,0x48,0x08};
        if(memcmp(draw,expected,sizeof(expected))==0) {
            MH_CreateHook(draw,Hook,reinterpret_cast<void**>(&original));
        } else {
            Message("Aviso: Draw prologue mudou, pulando hook de draw");
        }
    }

    if(SkinAddressesValid() && modulesMatched) {
        MH_CreateHook(reinterpret_cast<void*>(client+SKIN_FRAME.rva),FrameHook,reinterpret_cast<void**>(&originalFrame));
    } else {
        Message("Aviso: Skin function identity mudou, pulando hook de skins");
    }

    if(shared) {
        shared->audioProtocol=0;shared->kills=0;shared->headshots=0;shared->skyBrightness=0.7f;shared->skyColour=0xffb8b8b8;
    }

    if(client && modulesMatched) {
        const unsigned char skyPrologue[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x55,0x41,0x57,0x48,0x8b,0xec,0x48,0x83,0xec};
        if(memcmp(reinterpret_cast<void*>(client+0x282390),skyPrologue,sizeof(skyPrologue))!=0)climateFault=true;
        if(!climateFault) {
            MH_CreateHook(reinterpret_cast<void*>(client+0x282390),SkyUpdateHook,reinterpret_cast<void**>(&originalSkyUpdate));
        }
        if(AudioAddressesValid()) {
            if(MH_CreateHook(reinterpret_cast<void*>(client+0x9e2960),AudioEventHook,reinterpret_cast<void**>(&originalEvent))==MH_OK && shared) {
                shared->audioProtocol=1;
            }
        }
    }

    if(scene && modulesMatched) {
        auto skyDraw=reinterpret_cast<unsigned char*>(scene)+0x154fb0;
        const unsigned char skyDrawExpected[]={0x45,0x85,0xc9,0x0f,0x8e,0x1f,0x07,0x00,0x00,0x4c,0x8b,0xdc,0x55,0x41,0x56,0x49,0x8d,0xab,0x58,0xfc,0xff,0xff,0x48,0x81};
        if(memcmp(skyDraw,skyDrawExpected,sizeof(skyDrawExpected))==0){
            MH_CreateHook(skyDraw,SkyDrawHook,reinterpret_cast<void**>(&originalSkyDraw));
        } else {
            climateFault=true;ClimateMessage(-3,"Aviso: Renderizador de ceu mudou");
        }
    }'''

assert old_wndproc in text, "old_wndproc not found"
assert old_makemat in text, "old_makemat not found"
assert old_prepare in text, "old_prepare not found"
assert old_start_tail in text, "old_start_tail not found"

text = text.replace(old_wndproc, new_wndproc)
text = text.replace(old_makemat, new_makemat)
text = text.replace(old_prepare, new_prepare)
text = text.replace(old_start_tail, new_start_tail)

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)
print("Internal.cpp patched successfully!")
