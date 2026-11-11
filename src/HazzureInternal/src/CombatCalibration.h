// C++17 / CombatCalibration.h / MSVC x64 / Hazzure Suite de Combate Biologica
// Frente 2.3: Histerese de Alvo, Matriz de Recoil Calibrada, Triggerbot Log-Normal e Trava de Deriva Angular
#pragma once

#include "GameData.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace HazzureCombatCalib {

struct WeaponRecoilCalibration {
    float pitchMultiplier = 1.0f;
    float yawMultiplier = 1.0f;
    int bulletDelay = 1;         // Disparos antes de iniciar compensacao ativa
    int rampBullets = 3;         // Disparos necessarios para atingir forca total
    float smoothFactor = 1.8f;   // Amortecimento da transicao de retorno
    float punchDecayWindow = 0.25f; // Segundos para zerar compensacao residual
};

// Retorna parametros calibrados por indice de definicao de arma do CS2
inline WeaponRecoilCalibration GetCalibratedRecoil(uint16_t itemDefinitionIndex) {
    WeaponRecoilCalibration c{};
    switch (itemDefinitionIndex) {
        // Rifles (AK-47, M4A4, M4A1-S, Galil, Famas, SG553, AUG)
        case 7:  // AK-47
        case 8:  // AUG
        case 10: // FAMAS
        case 13: // Galil AR
        case 16: // M4A4
        case 60: // M4A1-S
        case 39: // SG 553
            c.bulletDelay = 1;
            c.rampBullets = 3;
            c.pitchMultiplier = 1.0f;
            c.yawMultiplier = 1.0f;
            c.smoothFactor = 1.8f;
            break;

        // Pistolas (Deagle, Glock, USP-S, P250, Tec-9, Five-SeveN, Dual Berettas, CZ75, R8)
        case 1:  // Desert Eagle
        case 2:  // Dual Berettas
        case 3:  // Five-SeveN
        case 4:  // Glock-18
        case 30: // Tec-9
        case 32: // P2000
        case 36: // P250
        case 61: // USP-S
        case 63: // CZ75-Auto
        case 64: // R8 Revolver
            c.bulletDelay = 1;  // Taps isolados nao sofrem puxao de RCS
            c.rampBullets = 1;
            c.pitchMultiplier = 0.5f;
            c.yawMultiplier = 0.5f;
            c.smoothFactor = 2.5f;
            break;

        // Snipers (AWP, SSG 08, SCAR-20, G3SG1)
        case 9:  // AWP
        case 11: // G3SG1
        case 38: // SCAR-20
        case 40: // SSG 08
            c.bulletDelay = 1;
            c.rampBullets = 1;
            c.pitchMultiplier = 0.0f; // Snipers nao utilizam compensacao de spray continuo
            c.yawMultiplier = 0.0f;
            c.smoothFactor = 1.0f;
            break;

        // Submetralhadoras / SMGs (Mac-10, MP9, MP7, MP5-SD, P90, PP-Bizon, UMP-45)
        case 17: // MAC-10
        case 19: // P90
        case 23: // MP5-SD
        case 24: // UMP-45
        case 26: // PP-Bizon
        case 33: // MP7
        case 34: // MP9
            c.bulletDelay = 0;
            c.rampBullets = 2;
            c.pitchMultiplier = 0.85f;
            c.yawMultiplier = 0.85f;
            c.smoothFactor = 1.5f;
            break;

        // Padrao para outras armas (Shotguns, MGs)
        default:
            c.bulletDelay = 1;
            c.rampBullets = 2;
            c.pitchMultiplier = 0.9f;
            c.yawMultiplier = 0.9f;
            c.smoothFactor = 2.0f;
            break;
    }
    return c;
}

// Histerese de Alvo (Target Retention Lock): previne troca espasmodica de alvos proximos
struct TargetRetentionState {
    uintptr_t lockedPawn = 0;
    uint32_t lockedHandle = 0;
    uint64_t lastLockedTick = 0;
    int targetBone = 0;

    bool IsLocked(uintptr_t candidatePawn, uint64_t now, uint32_t retentionWindowMs = 250) const {
        if (!lockedPawn || lockedPawn != candidatePawn) return false;
        return (now >= lastLockedTick && now - lastLockedTick <= retentionWindowMs);
    }

    void Lock(uintptr_t pawn, uint32_t handle, int bone, uint64_t now) {
        lockedPawn = pawn;
        lockedHandle = handle;
        targetBone = bone;
        lastLockedTick = now;
    }

    void Unlock() {
        lockedPawn = 0;
        lockedHandle = 0;
        lastLockedTick = 0;
    }
};

// Gerador de Reacao Humana Biologica (Distribuicao Log-Normal / Box-Muller)
struct HumanReactionGenerator {
    static uint32_t GenerateDelayMs(float meanMs, float stdDevMs, uint32_t minFloorMs, uint32_t maxCeilMs, uint32_t& seed) {
        auto NextFloat = [&seed]() {
            if (!seed) seed = 0x9e3779b9;
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            return (static_cast<float>(seed & 0xFFFFFF) + 1.0f) / 16777217.0f;
        };

        float u1 = std::clamp(NextFloat(), 1e-6f, 0.999999f);
        float u2 = NextFloat();
        // Transformada Box-Muller para amostragem gaussiana
        float z0 = std::sqrt(-2.0f * std::log(u1)) * std::cos(6.2831853f * u2);
        float sample = meanMs + z0 * stdDevMs;

        return static_cast<uint32_t>(std::clamp(sample, static_cast<float>(minFloorMs), static_cast<float>(maxCeilMs)));
    }
};

// Trava de Velocidade Angular da Camera (Crosshair Drift):
// Impede disparos acidentais quando o operador estiver fazendo flick rapido de mouse
inline bool IsCrosshairMotionStable(const HazzureGame::Vec3& currentAngles, const HazzureGame::Vec3& previousAngles,
                                   float maxDegreesPerSecond = 180.f, float dt = 0.016f) {
    if (dt <= 0.0001f || dt > 0.5f) return true;
    float dPitch = std::fabs(currentAngles.x - previousAngles.x);
    float dYaw   = std::fabs(HazzureGame::NormalizeAngle(currentAngles.y - previousAngles.y));
    float totalAngleDelta = std::hypot(dPitch, dYaw);
    float angularVelocity = totalAngleDelta / dt;
    return (angularVelocity <= maxDegreesPerSecond);
}

inline TargetRetentionState g_aimRetention;

} // namespace HazzureCombatCalib
