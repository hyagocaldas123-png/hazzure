// Scope filter & Native Scope Overlay Hook
// client.dll build 14194:
// 1. ConVar cl_ironsight_filter_alpha (RVA 0x24dbe50 + 0x58) for ironsight quad
// 2. DrawScopeOverlay (RVA 0x8e2e90, signature: 48 8B C4 53 57 48 83 EC ? 48 8B FA)
#pragma once
#include <intrin.h>
#include "Visibility.h"
#include "DebugLogger.h"
#include <atomic>

namespace HazzureScopeOverlay {
// Build 14194: DrawScopeOverlay RVA and signature
inline constexpr uintptr_t drawScopeOverlayRva = 0x8e2e90;
inline constexpr unsigned char drawScopeOverlayPrefix[] = { 0x48, 0x8B, 0xC4, 0x53, 0x57, 0x48, 0x83, 0xEC };

typedef void(__fastcall* DrawScopeOverlayFn)(void* pPawn, void* pScopeData);
inline DrawScopeOverlayFn originalDrawScopeOverlay = nullptr;
inline std::atomic<bool> hookInstalled{ false };
inline std::atomic<uint64_t> suppressedFrames{ 0 };

inline uintptr_t FindDrawScopeOverlay(uintptr_t client) {
    if (!client) return 0;
    if (HazzureVisibility::Prefix(client + drawScopeOverlayRva, drawScopeOverlayPrefix, sizeof(drawScopeOverlayPrefix))) {
        return client + drawScopeOverlayRva;
    }
    // Dynamic scan fallback across .text section if build offset drifts
    PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(client);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(client + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        if (memcmp(sec->Name, ".text", 5) == 0) {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(client + sec->VirtualAddress);
            size_t size = sec->Misc.VirtualSize;
            for (size_t j = 0; j + 11 < size; ++j) {
                if (p[j] == 0x48 && p[j+1] == 0x8B && p[j+2] == 0xC4 && p[j+3] == 0x53 &&
                    p[j+4] == 0x57 && p[j+5] == 0x48 && p[j+6] == 0x83 && p[j+7] == 0xEC &&
                    p[j+9] == 0x48 && p[j+10] == 0x8B && p[j+11] == 0xFA) {
                    return reinterpret_cast<uintptr_t>(p + j);
                }
            }
            break;
        }
    }
    return 0;
}

// client.dll 0x6AEBC4DA: ConVarF32 registered in ctor 0xA9830..0xA98DE (lea rcx at +0xA98B2 -> 0x24DBE50,
// name +0x1C03890 "cl_ironsight_filter_alpha", help "Ironsight filter alpha", default 1.0 via +0x76BD90).
// Float slot at [global+8]+0x58 is sampled directly by the renderer at +0x833D23..0x833D2A.
inline constexpr uintptr_t filterAlphaRva = 0x24dbe50;
inline constexpr uintptr_t valueOffset = 0x58;
enum class Status { Disabled, Unverified, Applied, Restored };
inline std::atomic<Status> status{ Status::Disabled };
inline std::atomic<float> observed{}, delivered{}, saved{};
inline std::atomic<bool> owned{};
inline std::atomic<uint32_t> writes{};
template<class T> inline bool Put(uintptr_t at, const T& value) {
    __try { memcpy(reinterpret_cast<void*>(at), &value, sizeof(value)); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
// The float must read back as a plausible alpha before anything is written to it: a slot
// that does not answer with a finite 0..1 value is not this convar on this build.
inline bool Slot(uintptr_t client, uintptr_t& address, float& value) {
    uintptr_t data{}; float read{};
    if (!HazzureGame::Read(client + filterAlphaRva + 8, data) || !data) return false;
    address = data + valueOffset;
    if (!HazzureGame::Read(address, read) || !std::isfinite(read) || read < 0.f || read > 1.f) return false;
    value = read; return true;
}
// Runs on the UI pump; logs one transition line when disabled so session logs distinguish off vs unverified.
inline void Update(uintptr_t client, bool enabled, float target, uint64_t now) {
    static Status lastStatus{ Status::Unverified }; static uint64_t loggedAt{};
    if (!enabled && !owned.load()) {
        status.store(Status::Disabled);
        if (lastStatus != Status::Disabled && client && HazzureVisibility::BuildMatches(client)) {
            uintptr_t address{}; float current{};
            if (Slot(client, address, current)) observed.store(current);
            lastStatus = Status::Disabled; loggedAt = now;
            LogDebug("[SCOPE] filtro: medido=%.3f entregue=%.3f alvo=%.3f ligado=0 writes=%u slot=0x%llX",
                current, delivered.load(), target, writes.load(), static_cast<unsigned long long>(address));
        }
        return;
    }
    if (!client || !HazzureVisibility::BuildMatches(client)) { status.store(Status::Unverified); return; }
    uintptr_t address{}; float current{};
    if (!Slot(client, address, current)) { status.store(Status::Unverified); return; }
    observed.store(current);
    float want{};
    if (enabled) {
        want = target < 0.f ? 0.f : (target > 1.f ? 1.f : target);
        if (!owned.exchange(true)) saved.store(current);
    } else {
        want = saved.load(); owned.store(false);
    }
    if (current != want) {
        if (!Put(address, want)) { status.store(Status::Unverified); return; }
        delivered.store(want); writes.fetch_add(1);
    }
    Status next = enabled ? Status::Applied : Status::Restored;
    status.store(next);
    if (next != lastStatus || now - loggedAt >= 2000) {
        lastStatus = next; loggedAt = now;
        LogDebug("[SCOPE] filtro: medido=%.3f entregue=%.3f alvo=%.3f ligado=%d writes=%u slot=0x%llX",
            current, delivered.load(), want, enabled ? 1 : 0, writes.load(),
            static_cast<unsigned long long>(address));
    }
}
inline const char* Label() {
    switch (status.load()) {
    case Status::Applied: return "Filtro do scope aplicado";
    case Status::Restored: return "Filtro do scope restaurado para o valor nativo";
    case Status::Unverified: return "Convar do filtro nao confirmado neste build; nada foi escrito";
    default: return "Desativado";
    }
}
}
