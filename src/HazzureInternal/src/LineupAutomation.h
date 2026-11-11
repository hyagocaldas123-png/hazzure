// Deterministic alignment/run/throw controller. Native CS:2 (Source 2 / Sub-tick / Ballistics).
#pragma once
#include "LineupLibrary.h"
#include <atomic>

namespace HazzureLineupAssist {
using HazzureGame::Vec3;
enum class Phase { Idle, Align, Run, Jump, Release, Done, Cancelled };

struct PendingExecution {
    std::atomic<bool> armed{ false };
    int throwMode{ 1 };
    float when{ 0.0f };
    uint64_t timestamp{ 0 };
    char name[64]{};
    char map[64]{};
    float dxy{ 0.0f };
};
inline PendingExecution g_pendingLineup;

struct Input {
    bool active{}, key{}, context{}, pin{}, ground{}, ducked{};
    uintptr_t pawn{}, weapon{};
    int selected = -1, definition{};
    uint64_t now{};
    Vec3 position{}, eye_origin{}, angles{};
    float speed{}, strength{}, when{};
    const char* map = "";
};

struct Output {
    bool aim{}, forward{}, jump{}, releaseAttack{}, restore{};
    Vec3 targetAngles{};
    int throwMode{ 1 };
    float when{ 0.0f };
};

struct Sequence {
    Phase phase = Phase::Idle;
    uint64_t began{}, stage{}, lastExecution{};
    uintptr_t pawn{}, weapon{};
    int selected = -1;
    bool previousKey{};
    HazzureLineups::Entry entry{};
    Vec3 targetAngles{};
    bool solved{};

    void Reset() { *this = {}; }
    Output Cancel() {
        bool was = phase != Phase::Idle && phase != Phase::Done && phase != Phase::Cancelled;
        phase = Phase::Cancelled;
        return { false, false, false, false, was, {}, 1, 0.0f };
    }

    Output Step(const Input& input, const HazzureLineups::Entry* value) {
        bool pressed = input.key && !previousKey;
        previousKey = input.key;
        bool alreadyActive = phase == Phase::Align || phase == Phase::Run || phase == Phase::Jump || phase == Phase::Release;

        // 250ms cooldown pos-execucao (Secao 7 do briefing)
        if (input.now - lastExecution < 250) return {};

        if (pressed && alreadyActive) return Cancel();
        if (!input.active || !input.context || !value || !value->validated || !HazzureLineups::Valid(*value) ||
            !std::isfinite(input.speed) || !std::isfinite(input.strength) || input.strength < 0.99f ||
            !HazzureGame::Finite(input.position) || !HazzureGame::Finite(input.angles, 36000)) return Cancel();

        if (phase == Phase::Idle || phase == Phase::Done || phase == Phase::Cancelled) {
            if (!pressed) return {};
            if (!input.ground || !input.pin) return Cancel();
            if (value->stance == 1 && !input.ducked) return Cancel(); // exige crouch se o lineup gravou crouch

            float dxy = HazzureLineups::HorizontalDistance(input.position, value->position);
            float dz = HazzureLineups::VerticalDistance(input.position, value->position);
            if (dxy > 12.0f || dz > 16.0f) return Cancel(); // Tolerancia estrita: dxy <= 12, dz <= 16
            if (strcmp(input.map, value->map) || input.definition != value->definition) return Cancel();

            entry = *value;
            pawn = input.pawn;
            weapon = input.weapon;
            selected = input.selected;
            began = stage = input.now;
            phase = Phase::Align;
        }

        if (input.pawn != pawn || input.weapon != weapon || input.selected != selected ||
            strcmp(input.map, entry.map) || input.definition != entry.definition ||
            input.now < began || input.now - began > 3000) return Cancel();

        // Resolver balistico para obter targetAngles dinamico
        targetAngles = entry.angles;
        solved = false;
        if (entry.arcKind == 0 && (std::fabs(entry.impact.x) > 0.1f || std::fabs(entry.impact.y) > 0.1f)) {
            float s = (entry.throwMode == 4) ? (750.0f * 0.66f) : 750.0f;
            float solPitch = 0.0f, solYaw = 0.0f;
            if (HazzureLineups::SolveBallistics(input.eye_origin, entry.impact, s, 320.0f, solPitch, solYaw)) {
                targetAngles.x = solPitch;
                targetAngles.y = solYaw;
                solved = true;
            }
        }

        float pitch = std::fabs(input.angles.x - targetAngles.x);
        float yaw = std::fabs(HazzureGame::NormalizeAngle(input.angles.y - targetAngles.y));

        if (phase == Phase::Align) {
            if (!input.pin || !input.ground) return Cancel();
            // Convergencia estrita <= 0.5 graus
            if (pitch > 0.5f || yaw > 0.5f) {
                return { true, false, false, false, false, targetAngles, entry.throwMode, entry.when };
            }
            stage = input.now;
            phase = (entry.runMs > 0) ? Phase::Run : (entry.throwMode == 1 ? Phase::Jump : Phase::Release);
        }

        if (phase == Phase::Run) {
            if (!input.pin || !input.ground || pitch > 1.0f || yaw > 1.0f) return Cancel();
            if (input.now - stage < static_cast<uint64_t>(entry.runMs)) {
                return { true, true, false, false, false, targetAngles, entry.throwMode, entry.when };
            }
            stage = input.now;
            phase = (entry.throwMode == 1 ? Phase::Jump : Phase::Release);
        }

        if (phase == Phase::Jump) {
            if (!input.pin) return Cancel();
            phase = Phase::Release;
            return { false, false, true, true, false, targetAngles, entry.throwMode, entry.when };
        }

        if (phase == Phase::Release) {
            if (!input.pin) return Cancel();
            phase = Phase::Done;
            lastExecution = input.now;
            return { false, false, (entry.throwMode == 1), true, true, targetAngles, entry.throwMode, entry.when };
        }

        return {};
    }
};
} // namespace HazzureLineupAssist
