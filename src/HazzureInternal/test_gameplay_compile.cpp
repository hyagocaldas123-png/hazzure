// Standalone compilation test for aimbot/triggerbot changes
// This only includes the core gameplay headers needed to verify the changes compile

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define USE_MINIMAL_BUILD

#include <windows.h>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <atomic>

// Minimal forward declarations to avoid including full headers
namespace HazzureOffsets {
    namespace Globals {
        extern const uintptr_t dwCCSGOInput;
        extern const uintptr_t dwEntityList;
    }
    namespace Schema {
        extern const uintptr_t m_iIDEntIndex;
        extern const uintptr_t m_pWeaponServices;
        extern const uintptr_t m_hActiveWeapon;
        extern const uintptr_t m_hPlayerPawn;
    }
}

namespace HazzureGame {
    struct Vec3 { float x, y, z; };
    struct Vec2 { float x, y; };
    struct Pawn { 
        uintptr_t address;
        Vec3 eye;
        Vec3 origin;
        int team;
        int health;
        int life;
        bool anglesValid;
        bool velocityValid;
        Vec3 velocity;
    };
    
    static float NormalizeAngle(float angle) {
        while (angle > 180.f) angle -= 360.f;
        while (angle < -180.f) angle += 360.f;
        return angle;
    }
    
    static float SmoothAlpha(float smooth, float dt) { return 1.f - std::exp(-smooth * dt); }
    
    static bool AimAngles(const Vec3& from, const Vec3& to, const Vec3& view, Vec2& out) {
        Vec3 delta = { to.x - from.x, to.y - from.y, to.z - from.z };
        float hyp = std::hypot(delta.x, delta.y);
        out.x = std::atan2(-delta.z, hyp) * 180.f / 3.14159265f - view.x;
        out.y = std::atan2(delta.y, delta.x) * 180.f / 3.14159265f - view.y;
        out.y = NormalizeAngle(out.y);
        return true;
    }
    
    static bool ReadBytes(uintptr_t, void*, size_t) { return true; }
    static Pawn LocalPawn(uintptr_t) { return {}; }
    static uintptr_t Entity(uintptr_t, int) { return 0; }
    static bool ReadPawn(uintptr_t, Pawn&) { return true; }
}

// Minimal SafeRead (from UIState.inl)
template<typename T>
static bool SafeRead(uintptr_t addr, T* out) {
    return out && HazzureGame::ReadBytes(addr, out, sizeof(T));
}

// Minimal config struct
struct Config {
    bool aimActive = true;
    bool rcsActive = true;
    float aimSmooth = 5.f;
    float rcsScaleX = 1.f;
    float rcsScaleY = 1.f;
    float rcsStrength = 1.f;
    float rcsSmooth = 5.f;
    int aimKey = 0;
    float aimReactionMinMs = 0.f;
    float aimReactionMaxMs = 0.f;
    int aimKillDelayMs = 0;
    int aimSwitchDelayMs = 0;
    bool aimBezier = false;
    bool aimEase = false;
    float aimEaseBias = 0.5f;
    float aimCurveJitter = 0.f;
    bool shotStop = false;
    bool shotFullStop = false;
    float shotStopSpeed = 0.f;
    bool shotStopEdgeGuard = false;
    bool triggerActive = true;
    int triggerKey = 0;
    bool humanizedTrigger = false;
    float triggerDelayMs = 0.f;
    bool triggerDistanceDelay = false;
    bool autoScope = false;
    bool strictSmoke = false;
    float mouseYawScale = 0.022f;
    float mousePitchScale = 0.022f;
    bool fastLadder = false;
} g_cfg;

// Frame diagnostics
struct FrameDiagnostics {
    bool haveTarget = false;
    bool aimHeld = false;
    bool sensitivityValid = false;
    bool visibilityUnavailable = false;
    bool visibilityPending = false;
    bool occluded = false;
    const char* aimStatus = "ready";
    const char* triggerStatus = "ready";
} g_frameDiagnostics;

// Minimal HazzureOffsets definitions
namespace HazzureOffsets {
    namespace Globals {
        const uintptr_t dwCCSGOInput = 0x257F980;
        const uintptr_t dwEntityList = 0x1A733C0;
        const uintptr_t dwViewAngles = 0x2580008;
    }
    namespace Schema {
        const uintptr_t m_iIDEntIndex = 0x36DC;
        const uintptr_t m_pWeaponServices = 0x2A8;
        const uintptr_t m_hActiveWeapon = 0x2EF8;
        const uintptr_t m_hPlayerPawn = 0x7D4;
    }
}

// GameplayRuntime.h functions under test
namespace HazzureGameplay {

// Direct view angles write for internal DLL injection (bypasses Raw Input / SendInput)
static bool WriteViewAngles(uintptr_t client, const HazzureGame::Vec3& angles) {
    // CCSGOInput is at client.dll + dwCCSGOInput (0x257F980 for Build 14194)
    // View angles are at CCSGOInput + 0x688 (current) or history buffer at +0x430
    uintptr_t input = client + HazzureOffsets::Globals::dwCCSGOInput;
    if (!input) return false;
    
    // Read history count to determine which buffer to write
    int count = 0;
    if (!SafeRead(input + 0xB50, &count) || count < 0 || count > 4096) {
        // Fallback: write directly to the no-history offset
        return SafeRead(input + 0x688, const_cast<HazzureGame::Vec3*>(&angles));
    }
    
    if (count > 0) {
        uintptr_t records = 0;
        if (!SafeRead(input + 0xB58, &records) || records < 0x10000) return false;
        // Write to the latest history record (current frame)
        uintptr_t address = records + static_cast<size_t>(count - 1) * 0x440 + 0x430;
        return SafeRead(address, const_cast<HazzureGame::Vec3*>(&angles));
    }
    
    // No history: write to fallback offset
    return SafeRead(input + 0x688, const_cast<HazzureGame::Vec3*>(&angles));
}

// Resolve entity by raw entity index (from m_iIDEntIndex), not by handle
// Source 2 entity list: chunks of 512 entries, each entry 0x78 bytes
static uintptr_t EntityByIndex(uintptr_t list, int32_t index) {
    if (!list || index <= 0 || index > 0x7FFF) return 0;
    uintptr_t chunk = 0;
    // chunk = list + 0x10 + (index >> 9) * 8
    if (!SafeRead(list + 0x10 + (index >> 9) * 8, &chunk) || !chunk) return 0;
    // entity = chunk + (index & 0x1FF) * 0x78
    uintptr_t entity = 0;
    if (!SafeRead(chunk + (index & 0x1FF) * 0x78, &entity) || entity < 0x10000) return 0;
    return entity;
}

} // namespace HazzureGameplay

int main() {
    // Test compilation only
    HazzureGame::Vec3 testAngles{ 10.f, 90.f, 0.f };
    uintptr_t client = 0x7FF600000000;
    
    // These will fail at runtime (no real process) but should compile
    bool result1 = HazzureGameplay::WriteViewAngles(client, testAngles);
    uintptr_t result2 = HazzureGameplay::EntityByIndex(0x1A733C0, 5);
    
    (void)result1;
    (void)result2;
    
    return 0;
}