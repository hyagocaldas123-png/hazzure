// C++ / DebugLogger.h / MSVC x64 C++17
#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "../../Compartilhado/offsets_generated.h"

inline void LogDebug(const char* fmt, ...) {
#ifdef INTERNAL_SELF_TEST
    (void)fmt;
    return;
#else
    static HANDLE hMutex = CreateMutexA(nullptr, FALSE, "Local\\HazzureDebugMutex");
    if (hMutex) WaitForSingleObject(hMutex, 200);

    FILE* f = nullptr;
    if (fopen_s(&f, "C:\\HazzureKernel\\debug_log.txt", "a") == 0 && f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fprintf(f, "\n");
        fclose(f);
    }

    if (hMutex) ReleaseMutex(hMutex);
#endif
}

namespace HazzureScanner {

inline uintptr_t FindPattern(uintptr_t base, size_t size, const char* patternStr) {
    if (!base || size == 0 || !patternStr) return 0;

    std::vector<uint8_t> patternBytes;
    std::vector<bool> patternMask;

    const char* cur = patternStr;
    while (*cur) {
        while (*cur == ' ') cur++;
        if (!*cur) break;

        if (*cur == '?') {
            patternBytes.push_back(0);
            patternMask.push_back(false);
            if (*(cur + 1) == '?') cur++;
            cur++;
        } else {
            char byteStr[3] = { cur[0], cur[1], 0 };
            patternBytes.push_back(static_cast<uint8_t>(strtoul(byteStr, nullptr, 16)));
            patternMask.push_back(true);
            cur += 2;
        }
    }

    size_t pLen = patternBytes.size();
    if (pLen == 0 || size < pLen) return 0;

    const uint8_t* pBase = reinterpret_cast<const uint8_t*>(base);
    for (size_t i = 0; i <= size - pLen; ++i) {
        bool match = true;
        for (size_t j = 0; j < pLen; ++j) {
            if (patternMask[j] && pBase[i + j] != patternBytes[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return base + i;
        }
    }
    return 0;
}

inline uintptr_t ResolveRip(uintptr_t instructionAddr, int32_t dispOffset, int32_t nextInstructionOffset, uintptr_t moduleBase) {
    if (!instructionAddr) return 0;
    int32_t disp = *reinterpret_cast<const int32_t*>(instructionAddr + dispOffset);
    uintptr_t target = (instructionAddr + nextInstructionOffset) + disp;
    return target - moduleBase;
}

inline void RunPatternScan(uintptr_t client, size_t clientSize) {
    LogDebug("==================================================");
    LogDebug("[PATTERN SCANNER] Iniciando escaneamento de memoria no client.dll (Base=0x%llX, Size=0x%llX)...", client, (uint64_t)clientSize);

    // 1. dwEntityList
    // Sig 1: 48 8B 05 ? ? ? ? 48 8B B0 10 02 00 00
    uintptr_t entHit = FindPattern(client, clientSize, "48 8B 05 ? ? ? ? 48 8B B0 10 02 00 00");
    if (entHit) {
        uintptr_t rva = ResolveRip(entHit, 3, 7, client);
        HazzureOffsets::Globals::dwEntityList = rva;
        LogDebug("[PATTERN SCANNER] [+] dwEntityList encontrado via Sig1! RVA=0x%llX (Absoluto=0x%llX, Hit=0x%llX)", rva, client + rva, entHit);
    } else {
        // Sig 2: 48 8B 0D ? ? ? ? 48 89 7C 24 ? 8B FA C1 EA
        uintptr_t entHit2 = FindPattern(client, clientSize, "48 8B 0D ? ? ? ? 48 89 7C 24 ? 8B FA C1 EA");
        if (entHit2) {
            uintptr_t rva = ResolveRip(entHit2, 3, 7, client);
            HazzureOffsets::Globals::dwEntityList = rva;
            LogDebug("[PATTERN SCANNER] [+] dwEntityList encontrado via Sig2! RVA=0x%llX (Absoluto=0x%llX, Hit=0x%llX)", rva, client + rva, entHit2);
        } else {
            LogDebug("[PATTERN SCANNER] [-] dwEntityList NAO encontrado via signature scan! Usando fallback: 0x%llX", HazzureOffsets::Globals::dwEntityList);
        }
    }

    // 2. dwViewMatrix
    // Sig 1: 48 8D 0D ? ? ? ? 48 C1 E0 06 48 03 C1 C3
    uintptr_t vmHit = FindPattern(client, clientSize, "48 8D 0D ? ? ? ? 48 C1 E0 06 48 03 C1 C3");
    if (vmHit) {
        uintptr_t rva = ResolveRip(vmHit, 3, 7, client);
        HazzureOffsets::Globals::dwViewMatrix = rva;
        LogDebug("[PATTERN SCANNER] [+] dwViewMatrix encontrado via Sig1! RVA=0x%llX (Absoluto=0x%llX, Hit=0x%llX)", rva, client + rva, vmHit);
    } else {
        uintptr_t vmHit2 = FindPattern(client, clientSize, "48 8D 0D ? ? ? ? 48 C1 E0 06");
        if (vmHit2) {
            uintptr_t rva = ResolveRip(vmHit2, 3, 7, client);
            HazzureOffsets::Globals::dwViewMatrix = rva;
            LogDebug("[PATTERN SCANNER] [+] dwViewMatrix encontrado via Sig2! RVA=0x%llX (Absoluto=0x%llX, Hit=0x%llX)", rva, client + rva, vmHit2);
        } else {
            LogDebug("[PATTERN SCANNER] [-] dwViewMatrix NAO encontrado via signature scan! Usando fallback: 0x%llX", HazzureOffsets::Globals::dwViewMatrix);
        }
    }

    // 3. The old generic Prediction LEA/RET matched 2517 functions in build
    // 14192. Its first result pointed into code, not a pawn. Preserve the
    // class-qualified PE result; GameData validates it per frame and can
    // resolve the player from the controller handle without this global.
    LogDebug("[PATTERN SCANNER] dwLocalPlayerPawn do PE: RVA=0x%llX; validacao semantica por quadro",
        HazzureOffsets::Globals::dwLocalPlayerPawn);

    // 4. dwLocalPlayerController
    uintptr_t lpcHit = FindPattern(client, clientSize, "48 8D 0D ? ? ? ? 33 F6 4C 89 34 F9");
    if (lpcHit) {
        uintptr_t rva = ResolveRip(lpcHit, 3, 7, client);
        HazzureOffsets::Globals::dwLocalPlayerController = rva;
        LogDebug("[PATTERN SCANNER] [+] dwLocalPlayerController encontrado! RVA=0x%llX (Absoluto=0x%llX)", rva, client + rva);
    } else {
        LogDebug("[PATTERN SCANNER] [-] dwLocalPlayerController NAO encontrado via scan! Usando fallback: 0x%llX", HazzureOffsets::Globals::dwLocalPlayerController);
    }

    LogDebug("[PATTERN SCANNER] Escaneamento finalizado.");
    LogDebug("==================================================");
}

} // namespace HazzureScanner
