// internal.cpp — DLL x64 injetada
#include <windows.h>
#include <winternl.h>
#include <cstring>
#include "ipc.h"

using LdrLoadDll_t = NTSTATUS(NTAPI*)(PWSTR, PULONG, PUNICODE_STRING, PHANDLE);

static Shared* g_sh = nullptr;
static LdrLoadDll_t oLdrLoadDll = nullptr;
static bool        g_blockVac = true;

// ---- hook de 14 bytes --------------------------------------------------
struct JumpHook { void* target; uint8_t stub[32]; };
static JumpHook g_hook{};

static void* InstallHook(void* target, void* hook) {
    DWORD old;
    VirtualProtect(g_hook.stub, sizeof(g_hook.stub), PAGE_EXECUTE_READWRITE, &old);
    memcpy(g_hook.stub, target, 14);
    uint8_t* p = g_hook.stub + 14;
    p[0] = 0xFF; p[1] = 0x25; *(uint32_t*)(p + 2) = 0;
    *(uint64_t*)(p + 6) = (uint64_t)target + 14;
    VirtualProtect(g_hook.stub, sizeof(g_hook.stub), old, &old);

    VirtualProtect(target, 14, PAGE_EXECUTE_READWRITE, &old);
    p = (uint8_t*)target;
    p[0] = 0xFF; p[1] = 0x25; *(uint32_t*)(p + 2) = 0;
    *(uint64_t*)(p + 6) = (uint64_t)hook;
    VirtualProtect(target, 14, old, &old);
    return g_hook.stub;
}

// ---- MODO A: nao deixa o modulo do anti-cheat carregar ------------------
static NTSTATUS NTAPI hkLdrLoadDll(PWSTR path, PULONG flags, PUNICODE_STRING name, PHANDLE out) {
    if (g_blockVac && name && name->Buffer && wcsstr(name->Buffer, L"vac")) {
        if (out) *out = nullptr;          // finge sucesso sem carregar
        if (g_sh) g_sh->vacBlocked = 1;
        return 0;                          // STATUS_SUCCESS
    }
    return oLdrLoadDll(path, flags, name, out);
}

// ---- observador: pra MODO B (deixar carregar e neutralizar depois) ------
static VOID NTAPI OnModule(void*, ULONG reason, const LDR_DLL_NOTIFICATION_DATA* d) {
    if (reason != LDR_DLL_NOTIFICATION_REASON_LOADED || !d || !d->Loaded.BaseDllName) return;
    const wchar_t* n = d->Loaded.BaseDllName->Buffer;
    if (n && (wcsstr(n, L"vac") || wcsstr(n, L"steamclient")))
        OutputDebugStringW(L"[internal] anti-cheat observado\n");
}

static DWORD WINAPI Main(void*) {
    g_sh = static_cast<Shared*>(MapViewOfFile(
        OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, IPC_MAP),
        FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared)));

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    oLdrLoadDll = (LdrLoadDll_t)GetProcAddress(ntdll, "LdrLoadDll");
    if (oLdrLoadDll) InstallHook(oLdrLoadDll, hkLdrLoadDll);

    using RegFn = NTSTATUS(NTAPI*)(ULONG, PVOID, PVOID, PVOID*);
    auto reg = (RegFn)GetProcAddress(ntdll, "LdrRegisterDllNotification");
    PVOID cookie = nullptr;
    if (reg) reg(0, OnModule, nullptr, &cookie);

    for (;;) {
        // jogo -> painel (exemplo; aqui entram teus pointers/schema)
        if (g_sh) {
            g_sh->stateSeq.fetch_add(1, std::memory_order_acq_rel);   // impar
            g_sh->inGame = 1;
            g_sh->stateSeq.fetch_add(1, std::memory_order_acq_rel);   // par
        }
        // painel -> jogo
        if (g_sh && g_sh->cmd != CMD_NONE) {
            if (g_sh->cmd == CMD_QUIT) break;
            g_sh->cmd = CMD_NONE;
        }
        Sleep(1);
    }
    return 0;
}

BOOL WINAPI DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(mod);
        CreateThread(nullptr, 0, Main, nullptr, 0, nullptr);
    }
    return TRUE;
}