// C++ / DynamicSchema.h / MSVC x64 C++17
// Resolucao dinamica de offsets via SchemaSystem001 + Cache com Fingerprint PE64 + Degradacao Graciosa
#pragma once
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace HazzureSchema {

// Offset definitions with status tracking
struct SchemaOffset {
    uintptr_t offset = 0;
    bool resolved = false;
    bool fromCache = false;
    bool fromFallback = false;
    const char* className = "";
    const char* fieldName = "";
};

inline SchemaOffset g_mGlow{ 0xDE8, false, false, true, "C_BaseModelEntity", "m_Glow" };
inline SchemaOffset g_mMaxShadowDist{ 0x10D0, false, false, true, "C_CascadeLight", "m_flMaxShadowDistance" };
inline SchemaOffset g_mPing{ 0x838, false, false, true, "CCSPlayerController", "m_iPing" };
inline SchemaOffset g_mIsScoped{ 0x1EA0, false, false, true, "C_CSPlayerPawnBase", "m_bIsScoped" };
inline SchemaOffset g_mArmorValue{ 0x287C, false, false, true, "C_CSPlayerPawn", "m_ArmorValue" };
inline SchemaOffset g_mHasDefuser{ 0x81C, false, false, true, "CCSPlayerController", "m_bPawnHasDefuser" };
inline SchemaOffset g_mGunGameImmunity{ 0x14EC, false, false, true, "C_CSPlayerPawn", "m_bGunGameImmunity" };
inline SchemaOffset g_mInReload{ 0x196C, false, false, true, "C_CSWeaponBase", "m_bInReload" };
inline SchemaOffset g_mPinPulled{ 0x1DF0, false, false, true, "C_BaseCSGrenade", "m_bPinPulled" };
inline SchemaOffset g_mFlashAlpha{ 0x160C, false, false, true, "C_CSPlayerPawnBase", "m_flFlashOverlayAlpha" };
inline SchemaOffset g_mClip1{ 0x1680, false, false, true, "C_BasePlayerWeapon", "m_iClip1" };
inline SchemaOffset g_mIsDefusing{ 0x2864, false, false, true, "C_CSPlayerPawn", "m_bIsDefusing" };

// Module Fingerprint Helper
inline bool GetModuleFingerprint(const char* moduleName, uint32_t& outTimestamp, uint32_t& outSizeOfImage) {
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod) {
        if (strcmp(moduleName, "client.dll") == 0) hMod = GetModuleHandleW(L"client.dll");
        if (!hMod) return false;
    }
    __try {
        auto base = reinterpret_cast<uintptr_t>(hMod);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
        outTimestamp = nt->FileHeader.TimeDateStamp;
        outSizeOfImage = nt->OptionalHeader.SizeOfImage;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Source 2 Schema System Minimal Structures
class CSchemaField {
public:
    const char* m_name;
    void* m_type;
    int32_t m_offset;
    int32_t m_metadataSize;
    void* m_metadata;
};

class CSchemaClass {
public:
    void* m_vtable;
    const char* m_name;
    const char* m_module;
    int32_t m_size;
    int16_t m_fieldsCount;
    int16_t m_staticCount;
    int16_t m_metadataCount;
    int16_t m_iAlign;
    int16_t m_nAlignment;
    int16_t m_flags;
    CSchemaField* m_fields;
};

class CSchemaTypeScope {
public:
    CSchemaClass* FindDeclaredClass(const char* name) {
        if (!this) return nullptr;
        __try {
            void** vtbl = *reinterpret_cast<void***>(this);
            if (!vtbl || !vtbl[2]) return nullptr;
            using Fn = CSchemaClass*(__thiscall*)(void*, const char*);
            return reinterpret_cast<Fn>(vtbl[2])(this, name);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }
};

class ISchemaSystem {
public:
    CSchemaTypeScope* FindTypeScopeForModule(const char* module) {
        if (!this) return nullptr;
        __try {
            void** vtbl = *reinterpret_cast<void***>(this);
            if (!vtbl || !vtbl[13]) return nullptr;
            using Fn = CSchemaTypeScope*(__thiscall*)(void*, const char*);
            return reinterpret_cast<Fn>(vtbl[13])(this, module);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }
};

// Logging and Diagnostics
inline void LogResolution(const char* fmt, ...) {
#ifdef INTERNAL_SELF_TEST
    (void)fmt;return;
#else
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buf, fmt, args);
    va_end(args);

    FILE* f = nullptr;
    if (fopen_s(&f, "C:\\HazzureKernel\\schema_resolver.log", "a") == 0 && f) {
        fprintf(f, "%s\n", buf);
        fclose(f);
    }
#endif
}

// Disk Cache Serialization com Fingerprint PE64
inline void SaveSchemaCache(const char* path = "C:\\HazzureKernel\\schema_cache.json") {
#ifdef INTERNAL_SELF_TEST
    if(!strcmp(path,"C:\\HazzureKernel\\schema_cache.json"))return;
#endif
    uint32_t timestamp = 0, sizeOfImage = 0;
    GetModuleFingerprint("client.dll", timestamp, sizeOfImage);

    FILE* f = nullptr;
    if (fopen_s(&f, path, "w") != 0 || !f) return;

    fprintf(f, "{\n");
    fprintf(f, "  \"version\": 1,\n");
    fprintf(f, "  \"timestamp\": %u,\n", timestamp);
    fprintf(f, "  \"sizeOfImage\": %u,\n", sizeOfImage);
    fprintf(f, "  \"m_Glow\": %llu,\n", (unsigned long long)g_mGlow.offset);
    fprintf(f, "  \"m_flMaxShadowDistance\": %llu,\n", (unsigned long long)g_mMaxShadowDist.offset);
    fprintf(f, "  \"m_iPing\": %llu,\n", (unsigned long long)g_mPing.offset);
    fprintf(f, "  \"m_bIsScoped\": %llu\n", (unsigned long long)g_mIsScoped.offset);
    fprintf(f, "}\n");
    fclose(f);
}

inline bool LoadSchemaCache(const char* path = "C:\\HazzureKernel\\schema_cache.json", uint32_t expectedTs = 0, uint32_t expectedSize = 0) {
#ifdef INTERNAL_SELF_TEST
    if(!strcmp(path,"C:\\HazzureKernel\\schema_cache.json"))return false;
#endif
    FILE* f = nullptr;
    if (fopen_s(&f, path, "r") != 0 || !f) return false;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0 || len > 16384) { fclose(f); return false; }

    std::vector<char> buf(len + 1, 0);
    fread(buf.data(), 1, len, f);
    fclose(f);

    uint32_t currentTs = 0, currentSize = 0;
    if (expectedTs > 0 && expectedSize > 0) {
        currentTs = expectedTs;
        currentSize = expectedSize;
    } else {
        GetModuleFingerprint("client.dll", currentTs, currentSize);
    }

    uint32_t cachedTs = 0, cachedSize = 0;
    auto readInt = [&](const char* key, uint32_t& outVal) {
        char pattern[64];
        sprintf_s(pattern, "\"%s\":", key);
        const char* p = strstr(buf.data(), pattern);
        if (p) {
            p += strlen(pattern);
            outVal = (uint32_t)strtoul(p, nullptr, 10);
        }
    };

    readInt("timestamp", cachedTs);
    readInt("sizeOfImage", cachedSize);

    // Verificacao de Fingerprint Estrito: se o modulo foi remontado pela Valve, o cache eh descartado
    if (currentTs != 0 && (cachedTs != currentTs || cachedSize != currentSize)) {
        LogResolution("[CACHE_INVALIDADO] Fingerprint de client.dll divergiu (cache: ts=0x%X size=0x%X vs atual: ts=0x%X size=0x%X). Descartando cache e forcando re-resolucao.",
            cachedTs, cachedSize, currentTs, currentSize);
        return false;
    }

    auto readOffset = [&](const char* key, SchemaOffset& target) {
        char pattern[64];
        sprintf_s(pattern, "\"%s\":", key);
        const char* p = strstr(buf.data(), pattern);
        if (p) {
            p += strlen(pattern);
            uintptr_t val = (uintptr_t)strtoull(p, nullptr, 10);
            if (val >= 0x100 && val < 0x20000) {
                target.offset = val;
                target.resolved = true;
                target.fromCache = true;
                target.fromFallback = false;
            }
        }
    };

    readOffset("m_Glow", g_mGlow);
    readOffset("m_flMaxShadowDistance", g_mMaxShadowDist);
    readOffset("m_iPing", g_mPing);
    readOffset("m_bIsScoped", g_mIsScoped);
    return (g_mGlow.resolved && g_mMaxShadowDist.resolved && g_mPing.resolved);
}

// Runtime Resolution Engine
inline int32_t FindFieldOffset(CSchemaClass* pClass, const char* fieldName) {
    if (!pClass || !fieldName) return -1;
    __try {
        int count = pClass->m_fieldsCount;
        CSchemaField* fields = pClass->m_fields;
        if (!fields || count <= 0 || count > 512) return -1;

        for (int i = 0; i < count; ++i) {
            const char* name = fields[i].m_name;
            if (name && strcmp(name, fieldName) == 0) {
                return fields[i].m_offset;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
    return -1;
}

inline void InitializeSchemaResolver() {
    LogResolution("=== Hazzure Dynamic Schema Resolver ===");

    // 1. Tenta carregar cache validando fingerprint do client.dll
    bool cacheOk = LoadSchemaCache();

    // 2. Tenta resolucao nativa via schemasystem.dll (se cache falhou ou foi descartado por CACHE_INVALIDADO)
    if (!cacheOk) {
        HMODULE hSchema = GetModuleHandleW(L"schemasystem.dll");
        if (!hSchema) hSchema = LoadLibraryW(L"schemasystem.dll");

        if (hSchema) {
            using CreateInterfaceFn = void* (*)(const char*, int*);
            auto pCreate = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hSchema, "CreateInterface"));
            if (pCreate) {
                ISchemaSystem* schemaSys = reinterpret_cast<ISchemaSystem*>(pCreate("SchemaSystem_001", nullptr));
                if (schemaSys) {
                    CSchemaTypeScope* clientScope = schemaSys->FindTypeScopeForModule("client.dll");
                    if (clientScope) {
                        auto resolveOne = [&](SchemaOffset& target) {
                            CSchemaClass* pClass = clientScope->FindDeclaredClass(target.className);
                            if (pClass) {
                                int32_t off = FindFieldOffset(pClass, target.fieldName);
                                if (off >= 0x100 && off < 0x20000) {
                                    target.offset = static_cast<uintptr_t>(off);
                                    target.resolved = true;
                                    target.fromCache = false;
                                    target.fromFallback = false;
                                    LogResolution("[NATIVO] %s::%s resolvido para 0x%X", target.className, target.fieldName, (unsigned)target.offset);
                                    return true;
                                }
                            }
                            return false;
                        };

                        bool g1 = resolveOne(g_mGlow);
                        bool g2 = resolveOne(g_mMaxShadowDist);
                        bool g3 = resolveOne(g_mPing);
                        bool g4 = resolveOne(g_mIsScoped);
                        if (g1 || g2 || g3 || g4) {
                            SaveSchemaCache(); // Atualiza cache de disco com fingerprint e valores atuais
                        }
                    }
                }
            }
        }
    }

    // 3. Fallbacks e logs para quem nao resolveu nativo nem por cache
    auto finalizeOffset = [](SchemaOffset& target, uintptr_t hardcodedFallback) {
        if (!target.resolved) {
            target.offset = hardcodedFallback;
            target.resolved = true;
            target.fromFallback = true;
            LogResolution("[FALLBACK] %s::%s usando fallback padrao 0x%X", target.className, target.fieldName, (unsigned)target.offset);
        } else if (target.fromCache) {
            LogResolution("[CACHE] %s::%s lido do cache com fingerprint validado (0x%X)", target.className, target.fieldName, (unsigned)target.offset);
        }
    };

    finalizeOffset(g_mGlow, 0xDE8);
    finalizeOffset(g_mMaxShadowDist, 0x10D0);
    finalizeOffset(g_mPing, 0x838);
    finalizeOffset(g_mIsScoped, 0x1EA0);
    finalizeOffset(g_mArmorValue, 0x287C);
    finalizeOffset(g_mHasDefuser, 0x81C);
    finalizeOffset(g_mGunGameImmunity, 0x14EC);
    finalizeOffset(g_mInReload, 0x196C);
    finalizeOffset(g_mPinPulled, 0x1DF0);
    finalizeOffset(g_mFlashAlpha, 0x160C);
    finalizeOffset(g_mClip1, 0x1680);
    finalizeOffset(g_mIsDefusing, 0x2864);

    LogResolution("Inicializacao concluida. Glow=0x%X, Shadow=0x%X, Ping=0x%X\n",
        (unsigned)g_mGlow.offset, (unsigned)g_mMaxShadowDist.offset, (unsigned)g_mPing.offset);
}

// Degradacao graciosa de features
inline bool IsGlowValid() { return g_mGlow.resolved && g_mGlow.offset >= 0x100 && g_mGlow.offset <= 0x10000; }
inline bool IsShadowValid() { return g_mMaxShadowDist.resolved && g_mMaxShadowDist.offset >= 0x100 && g_mMaxShadowDist.offset <= 0x10000; }
inline bool IsPingValid() { return g_mPing.resolved && g_mPing.offset >= 0x100 && g_mPing.offset <= 0x10000; }

} // namespace HazzureSchema
