// C++ / StateRegistry.h / MSVC x64 C++17
// Resident feature suspension and exact restoration of tracked glow snapshots
#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include "../../Compartilhado/ipc.h"
#include "../../Compartilhado/offsets_generated.h"
#include "DynamicSchema.h"

namespace HazzureState {

struct HookEntry {
    const char* name;
    void* target;
    void* detour;
    void* original;
    bool active;
};

inline std::vector<HookEntry> g_hooks;
// Gerenciador armazena uint32_t identificador (low 15 bits = indice do slot)
inline std::vector<uint32_t> g_glowingPawns;
struct GlowSnapshot {
    uint32_t handle;
    uintptr_t pawn,offset;
    uint8_t eligible,glowing;
    int32_t type,range;
    uint32_t colour;
};
inline std::vector<GlowSnapshot> g_glowSnapshots;
inline std::atomic<bool> g_unloading{ false };
inline std::atomic<bool> g_unloadRequested{ false };

// Resolucao customizada para testes unitarios ou mocking
using ResolveFn = bool (*)(uint32_t id, uintptr_t& outPtr, uint32_t& outCurId, int32_t& outHealth);
inline ResolveFn g_customResolver = nullptr;

inline void RegisterHook(const char* name, void* target, void* detour, void* original) {
    g_hooks.push_back({ name, target, detour, original, true });
}

// Registro por identificador uint32_t (low 15 bits = indice do slot)
inline void TrackGlow(uint32_t id) {
    if (!id) return;
    for (uint32_t p : g_glowingPawns) {
        if (p == id) return;
    }
    if (g_glowingPawns.size() < 128) {
        g_glowingPawns.push_back(id);
    }
}

// A chunk slot is the CEntityIdentity record: its first pointer is the entity,
// and the serial-bearing handle is inline at slot+0x10. Do not dereference the
// entity pointer again; doing so interprets its vtable as a pawn.
inline bool ResolveObjectById(uint32_t id, uintptr_t client, uintptr_t& outPawn, uint32_t& outCurId, int32_t& outHealth) {
    outPawn = 0;
    outCurId = 0;
    outHealth = 0;
    if (g_customResolver) {
        return g_customResolver(id, outPawn, outCurId, outHealth);
    }
    if (!client || id==0xffffffffu) return false;

    uint32_t slot = id & 0x7FFF;
    int chunkIdx = static_cast<int>(slot >> 9);
    int slotIdx = static_cast<int>(slot & 0x1FF);
    if (chunkIdx < 0 || chunkIdx >= 64) return false;

    __try {
        uintptr_t entityList = *reinterpret_cast<uintptr_t*>(client + HazzureOffsets::Globals::dwEntityList);
        if (!entityList) return false;

        uintptr_t chunk = *reinterpret_cast<uintptr_t*>(entityList + 0x10 + (chunkIdx * sizeof(uintptr_t)));
        if (!chunk) return false;

        uintptr_t identity = chunk + (slotIdx * 0x70);
        outPawn = *reinterpret_cast<uintptr_t*>(identity);
        if (!outPawn) return false;

        outCurId = *reinterpret_cast<uint32_t*>(identity + 0x10);
        outHealth = *reinterpret_cast<int32_t*>(outPawn + HazzureOffsets::Schema::m_iHealth);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

inline bool ReadGlowSnapshot(GlowSnapshot& value) {
    __try {
        uintptr_t property=value.pawn+value.offset;
        value.eligible=*reinterpret_cast<const uint8_t*>(property+0x50);
        value.glowing=*reinterpret_cast<const uint8_t*>(property+0x51);
        value.type=*reinterpret_cast<const int32_t*>(property+0x30);
        value.range=*reinterpret_cast<const int32_t*>(property+0x38);
        value.colour=*reinterpret_cast<const uint32_t*>(property+0x40);
        return value.eligible<=1&&value.glowing<=1;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool WriteGlowSnapshot(const GlowSnapshot& value) {
    __try {
        uintptr_t property=value.pawn+value.offset;
        *reinterpret_cast<int32_t*>(property+0x30)=value.type;
        *reinterpret_cast<int32_t*>(property+0x38)=value.range;
        *reinterpret_cast<uint32_t*>(property+0x40)=value.colour;
        *reinterpret_cast<uint8_t*>(property+0x50)=value.eligible;
        *reinterpret_cast<uint8_t*>(property+0x51)=value.glowing;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool CaptureGlow(uint32_t handle,uintptr_t pawn,uintptr_t glowOffset) {
    if(!handle||handle==0xffffffffu||!pawn||glowOffset<0x100||glowOffset>0x10000||
       pawn>(std::numeric_limits<uintptr_t>::max)()-glowOffset-0x52)return false;
    uintptr_t resolved=0;uint32_t current=0;int32_t health=0;
    auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if(!ResolveObjectById(handle,base,resolved,current,health)||current!=handle||resolved!=pawn||health<=0)return false;
    for(const auto& saved:g_glowSnapshots)if(saved.handle==handle){
        // Preserve the first snapshot. Recapturing altered values would lose the original.
        return saved.pawn==pawn&&saved.offset==glowOffset;
    }
    if(g_glowSnapshots.size()>=128)return false;
    GlowSnapshot snapshot{};snapshot.handle=handle;snapshot.pawn=pawn;snapshot.offset=glowOffset;
    if(!ReadGlowSnapshot(snapshot))return false;
    g_glowSnapshots.push_back(snapshot);TrackGlow(handle);return true;
}

// Only confirmed identity changes discard a captured original. A temporary read
// failure is retained for a later retry; no object is written during pruning.
inline void Prune(uintptr_t client = 0) {
    for (auto it = g_glowingPawns.begin(); it != g_glowingPawns.end(); ) {
        uint32_t id = *it;
        uintptr_t pawn = 0;
        uint32_t curId = 0;
        int32_t health = 0;

        auto saved=std::find_if(g_glowSnapshots.begin(),g_glowSnapshots.end(),[id](const GlowSnapshot& s){return s.handle==id;});
        if(saved==g_glowSnapshots.end()){
            // Legacy registration has no original values and cannot authorize a write.
            it = g_glowingPawns.erase(it);
            continue;
        }
        bool resolved = ResolveObjectById(id, client, pawn, curId, health);
        if (resolved && (curId != id || pawn != saved->pawn)) {
            g_glowSnapshots.erase(saved);it=g_glowingPawns.erase(it);
            continue;
        }
        ++it;
    }
}

// ResetAll(): chama Prune() no mesmo ciclo antes de tocar em qualquer flag do objeto
inline void ResetAll(uintptr_t client = 0) {
    // 1. Chama Prune() no mesmo ciclo antes de tocar em qualquer flag do objeto
    Prune(client);

    // 2. Restore only captured fields on the exact same serial-bearing identity.
    for (auto it=g_glowSnapshots.begin();it!=g_glowSnapshots.end();) {
        const auto& saved=*it;
        uintptr_t pawn = 0;
        uint32_t curId = 0;
        int32_t health = 0;
        if(!ResolveObjectById(saved.handle,client,pawn,curId,health)){++it;continue;}
        bool recycled=curId!=saved.handle||pawn!=saved.pawn;
        if(!recycled&&!WriteGlowSnapshot(saved)){++it;continue;}
        auto id=saved.handle;
        g_glowingPawns.erase(std::remove(g_glowingPawns.begin(),g_glowingPawns.end(),id),g_glowingPawns.end());
        it=g_glowSnapshots.erase(it);
    }
}

// Alias de compatibilidade com chamadas anteriores
inline void ResetAllGlow(uintptr_t client = 0) {
    ResetAll(client);
}

// Suspend feature processing while the module and installed hooks remain resident.
inline void PerformProductUnload(Shared* shared, uintptr_t client, HWND hWnd = nullptr, WNDPROC origWndProc = nullptr) {
    bool first=!g_unloading.exchange(true);

    FILE* f = nullptr;
#ifndef INTERNAL_SELF_TEST
    fopen_s(&f, "C:\\HazzureKernel\\unload.log", "w");
#endif
    if (f) {
        fprintf(f, "[PAUSE] Suspendendo recursos; modulo e hooks continuam residentes.\n");
    }

    // 1. Reseta o contorno brilhante chamando ResetAll() que executa Prune() primeiro
    ResetAll(client);
    if (f) fprintf(f, "[PAUSE] Snapshots de glow ainda pendentes: %zu.\n",g_glowSnapshots.size());

    // 2. Notifica shared memory
    if (shared) {
        InterlockedExchange(&shared->flags, 0);
        InterlockedExchange(&shared->skinEnabled, 0);
        InterlockedExchange(&shared->status, 0);
        InterlockedExchange(&shared->agentEnabled, 0);
        InterlockedExchange(&shared->climateFlags, 0);
        InterlockedExchange(&shared->removeWeather, 0);
        InterlockedExchange(&shared->cleanMap, 0);
        shared->fogDensity=0.f;
        InterlockedExchange(&shared->snowEnabled,0);
        InterlockedExchange(&shared->outlineGlow,0);
        InterlockedAdd(&shared->revision,2);
        InterlockedExchange64(&shared->heartbeat, 0);
    }
    if (f) fprintf(f, "[PAUSE] Sinalizadores de recursos e heartbeat zerados.\n");

    // 3. Restaura WndProc nativo se fornecido
    if (first && hWnd && origWndProc) {
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(origWndProc));
        if (f) fprintf(f, "[PAUSE] Solicitada restauracao do WndProc fornecido.\n");
    }

    if (f) {
        fprintf(f, "[PAUSE] Suspensao solicitada; nao houve remocao de hooks nem descarregamento da DLL.\n");
        fclose(f);
    }
}

} // namespace HazzureState
