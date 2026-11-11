// C++ / Internal.cpp / MSVC x64 C++17 / CS2 local material module

// GPL-3.0-or-later; material approach derived from Aspasia (see references/source.json).

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <cstdint>

#include <cstdio>

#include <cstring>

#include <atomic>

#include <cmath>

#include <d3d11.h>

#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")

#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "user32.lib")



#include "../Compartilhado/offsets_generated.h"

#include "../references/Aspasia/Includes/kiero/kiero.h"

#include "../../HazzurePanel/third_party/imgui/imgui.h"

#include "../../HazzurePanel/third_party/imgui/imgui_impl_win32.h"

#include "../../HazzurePanel/third_party/imgui/imgui_impl_dx11.h"

#include "DebugLogger.h"

#include "HazzureUI.h"

#include "../references/Aspasia/Includes/kiero/minhook/include/MinHook.h"

#include "BuildIdentity.h"


#ifndef USE_MINIMAL_BUILD
#include "InternalPresentation.inl"
#include "InternalResources.inl"
#include "InternalDrawing.inl"
#endif
#include "InternalStartup.inl"
#ifndef INTERNAL_SELF_TEST

BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {

    if(reason==DLL_PROCESS_ATTACH) {

        self=instance;DisableThreadLibraryCalls(instance);

        HANDLE thread=CreateThread(nullptr,0,Start,nullptr,0,nullptr);

        if(thread)CloseHandle(thread);

    }

    // Resident until process exit. Turning effects off retains the passthrough hook;
    // no unsafe unloading while another rendering thread could still execute it.

    return TRUE;

}

#else

#include "InternalTests.inl"

#endif