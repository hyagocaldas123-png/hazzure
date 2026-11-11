// User-recorded, map-labelled grenade positions. Native CS:2 (Source 2 / Sub-tick / Ballistics).
#pragma once
#include "GameData.h"
#include "ConfigStorage.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace HazzureLineups {
using HazzureGame::Vec3;

inline Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3 operator*(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }

enum class ThrowType : int {
    Stand = 0,
    Jumpthrow = 1,
    Runthrow = 2,
    Crouch = 3,
    Underhand = 4,
    Walk = 5
};

enum class Stance : int {
    Standing = 0,
    Crouched = 1
};

enum class ArcKind : int {
    Direct = 0,
    Bounce = 1
};

struct Entry {
    char map[64]{};
    char name[64]{};
    Vec3 position{};       // Origin dos pes
    Vec3 eye_origin{};     // Olhos: position + view_offset
    Vec3 angles{};         // Pitch, Yaw gravados
    Vec3 impact{};         // Ponto de detonacao/impacto (obrigatorio para solver balistico)
    Vec3 velocity{};       // Velocidade no frame do arremesso (RUNTHROW)
    float when{ 0.0f };    // Fracao sub-tick do input [0, 1)
    int definition{ 45 };  // 43=Flash, 44=HE, 45=Smoke, 46/48=Molotov, 47=Decoy
    int throwMode{ 1 };    // ThrowType (0=Stand, 1=Jumpthrow, etc.)
    int stance{ 0 };       // 0=Standing, 1=Crouched
    int arcKind{ 0 };      // 0=Direct, 1=Bounce
    int runMs{ 0 };
    bool validated{ true };
};

struct Library {
    char map[64]{};
    std::array<Entry, 64> entries{};
    int count{};
};

inline bool Text(const char* text, size_t capacity) {
    size_t n = strnlen_s(text, capacity);
    return n > 0 && n < capacity;
}

inline bool Valid(const Entry& e) {
    return Text(e.map, sizeof(e.map)) && Text(e.name, sizeof(e.name)) &&
        HazzureGame::Finite(e.position) && HazzureGame::Finite(e.angles, 36000) &&
        std::fabs(e.angles.x) <= 89.0f && e.definition >= 43 && e.definition <= 48 &&
        e.throwMode >= 0 && e.throwMode <= 5 && e.runMs >= 0 && e.runMs <= 1000;
}

inline float Distance(Vec3 a, Vec3 b) {
    return std::hypot(std::hypot(a.x - b.x, a.y - b.y), a.z - b.z);
}

inline float HorizontalDistance(Vec3 a, Vec3 b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

inline float VerticalDistance(Vec3 a, Vec3 b) {
    return std::fabs(a.z - b.z);
}

// Solver balistico analitico deterministico (Secao 5 do briefing)
inline bool SolveBallistics(Vec3 p0, Vec3 impact, float throwVelocity, float gravity, float& outPitch, float& outYaw) {
    float dx = impact.x - p0.x;
    float dy = impact.y - p0.y;
    float d = std::hypot(dx, dy);
    float dz = impact.z - p0.z;
    if (d < 1.0f) return false;

    float s = throwVelocity;
    float g = gravity;
    float s2 = s * s;
    float d2 = d * d;

    float A = (g * d2) / (2.0f * s2);
    (void)d;
    float C = dz + A;
    float disc = d2 - (4.0f * A * C);

    if (disc < 0.0f) {
        return false; // Fora de alcance balistico direto
    }

    // Raiz rasa = arco direto
    float tanTheta = (d - std::sqrt(disc)) / (2.0f * A);
    outPitch = -std::atan(tanTheta) * 57.2957795131f; // CS2 pitch negativo = mirar para cima
    outYaw = std::atan2(dy, dx) * 57.2957795131f;
    return true;
}

// Matching em runtime (Secao 4 do briefing: dxy <= 12, dz <= 16, score = 1 - dxy/12)
inline int Nearest(const Library& library, int definition, Vec3 position, int stance = 0, float* outScore = nullptr) {
    if (library.count < 0 || library.count > 64 || !Text(library.map, sizeof(library.map)) || !HazzureGame::Finite(position)) return -1;
    int selected = -1;
    float bestScore = -1.0f;
    for (int i = 0; i < library.count; ++i) {
        const auto& e = library.entries[i];
        if (!Valid(e) || strcmp(e.map, library.map) || e.definition != definition) continue;
        if (e.stance == 1 && stance != 1) continue; // exige crouch se o lineup gravou crouch
        float dxy = HorizontalDistance(position, e.position);
        float dz = VerticalDistance(position, e.position);
        if (dxy > 12.0f || dz > 16.0f) continue;
        float score = 1.0f - (dxy / 12.0f);
        if (score > bestScore) {
            bestScore = score;
            selected = i;
        }
    }
    if (outScore) *outScore = bestScore;
    return selected;
}

// Verificacao de tolerancia e convergencia de mira
inline bool Ready(const Entry& e, const char* map, int definition, Vec3 position, Vec3 angles, Vec3 eyeOrigin, Vec3& targetAngles, bool& solved) {
    if (!Valid(e) || !map || strcmp(e.map, map) || e.definition != definition) return false;
    if (!HazzureGame::Finite(position) || !HazzureGame::Finite(angles, 36000)) return false;

    float dxy = HorizontalDistance(position, e.position);
    float dz = VerticalDistance(position, e.position);
    if (dxy > 12.0f || dz > 16.0f) return false;

    targetAngles = e.angles;
    solved = false;
    if (e.arcKind == 0 && (std::fabs(e.impact.x) > 0.1f || std::fabs(e.impact.y) > 0.1f)) {
        float s = (e.throwMode == 4) ? (750.0f * 0.66f) : 750.0f;
        float solPitch = 0.0f, solYaw = 0.0f;
        if (SolveBallistics(eyeOrigin, e.impact, s, 320.0f, solPitch, solYaw)) {
            targetAngles.x = solPitch;
            targetAngles.y = solYaw;
            solved = true;
        }
    }

    float deltaPitch = std::fabs(angles.x - targetAngles.x);
    float deltaYaw = std::fabs(HazzureGame::NormalizeAngle(angles.y - targetAngles.y));
    return deltaPitch <= 0.5f && deltaYaw <= 0.5f; // convergencia estrita <= 0.5 graus
}

inline bool Ready(const Entry& e, const char* map, int definition, Vec3 position, Vec3 angles) {
    Vec3 dummyTarget{};
    bool dummySolved = false;
    Vec3 eyeOrigin = { position.x, position.y, position.z + 64.0f };
    return Ready(e, map, definition, position, angles, eyeOrigin, dummyTarget, dummySolved);
}

inline bool ReadJSON(const HazzureConfig::Value& root, Library& output, std::string& error) {
    using namespace HazzureConfig;
    const auto* value = Find(root, "lineups");
    if (!value) return true;
    Library candidate{};
    std::string map;
    auto fail = [&]() { error = "Biblioteca de lineups invalida"; return false; };
    if (!value->IsObject() || !GetString(*value, "map", map, 63) || map.find('\0') != std::string::npos) return fail();
    strcpy_s(candidate.map, map.c_str());
    const auto* entries = Find(*value, "entries");
    if (!entries || !entries->IsArray() || entries->Size() > 64) return fail();
    for (const auto& item : entries->GetArray()) {
        Entry e{};
        std::string name, entryMap;
        float position[3]{}, angles[3]{}, impact[3]{}, eye[3]{}, vel[3]{};
        if (!item.IsObject() || !GetString(item, "map", entryMap, 63) || !GetString(item, "name", name, 63) ||
            entryMap.find('\0') != std::string::npos || name.find('\0') != std::string::npos ||
            !GetInt(item, "definition", e.definition, 43, 48) ||
            !GetFloatArray(item, "position", position, 3, -1000000.f, 1000000.f) ||
            !GetFloatArray(item, "angles", angles, 3, -36000.f, 36000.f)) return fail();
        strcpy_s(e.map, entryMap.c_str());
        strcpy_s(e.name, name.c_str());
        e.position = { position[0], position[1], position[2] };
        e.angles = { angles[0], angles[1], angles[2] };
        if (Find(item, "throwMode") && !GetInt(item, "throwMode", e.throwMode, 0, 5)) return fail();
        if (Find(item, "stance") && !GetInt(item, "stance", e.stance, 0, 1)) return fail();
        if (Find(item, "arcKind") && !GetInt(item, "arcKind", e.arcKind, 0, 1)) return fail();
        if (Find(item, "runMs") && !GetInt(item, "runMs", e.runMs, 0, 1000)) return fail();
        if (Find(item, "when") && !GetFloat(item, "when", e.when, 0.0f, 1.0f)) return fail();
        if (Find(item, "validated") && !GetBool(item, "validated", e.validated)) return fail();
        if (Find(item, "impact") && GetFloatArray(item, "impact", impact, 3, -1000000.f, 1000000.f)) {
            e.impact = { impact[0], impact[1], impact[2] };
        }
        if (Find(item, "eye_origin") && GetFloatArray(item, "eye_origin", eye, 3, -1000000.f, 1000000.f)) {
            e.eye_origin = { eye[0], eye[1], eye[2] };
        } else {
            e.eye_origin = { position[0], position[1], position[2] + 64.0f };
        }
        if (Find(item, "velocity") && GetFloatArray(item, "velocity", vel, 3, -1000000.f, 1000000.f)) {
            e.velocity = { vel[0], vel[1], vel[2] };
        }
        if (!Valid(e)) return fail();
        candidate.entries[candidate.count++] = e;
    }
    output = candidate;
    return true;
}

inline bool WriteJSON(HazzureConfig::Value& root, HazzureConfig::AllocatorType& allocator, const Library& library, std::string& error) {
    using namespace HazzureConfig;
    if (library.count < 0 || library.count > 64 || strnlen_s(library.map, sizeof(library.map)) >= sizeof(library.map)) {
        error = "Biblioteca de lineups invalida"; return false;
    }
    Value object(rapidjson::kObjectType), entries(rapidjson::kArrayType);
    Set(object, "map", std::string(library.map), allocator);
    for (int i = 0; i < library.count; ++i) {
        const auto& e = library.entries[i];
        if (!Valid(e)) { error = "Lineup invalido"; return false; }
        Value item(rapidjson::kObjectType);
        Set(item, "map", std::string(e.map), allocator);
        Set(item, "name", std::string(e.name), allocator);
        Set(item, "definition", e.definition, allocator);
        float position[] = { e.position.x, e.position.y, e.position.z };
        float angles[] = { e.angles.x, e.angles.y, e.angles.z };
        float impact[] = { e.impact.x, e.impact.y, e.impact.z };
        float eye[] = { e.eye_origin.x, e.eye_origin.y, e.eye_origin.z };
        float vel[] = { e.velocity.x, e.velocity.y, e.velocity.z };
        Set(item, "throwMode", e.throwMode, allocator);
        Set(item, "stance", e.stance, allocator);
        Set(item, "arcKind", e.arcKind, allocator);
        Set(item, "runMs", e.runMs, allocator);
        Set(item, "when", e.when, allocator);
        Set(item, "validated", e.validated, allocator);
        SetFloatArray(item, "position", position, 3, allocator);
        SetFloatArray(item, "angles", angles, 3, allocator);
        SetFloatArray(item, "impact", impact, 3, allocator);
        SetFloatArray(item, "eye_origin", eye, 3, allocator);
        SetFloatArray(item, "velocity", vel, 3, allocator);
        entries.PushBack(item, allocator);
    }
    SetValue(object, "entries", std::move(entries), allocator);
    SetValue(root, "lineups", std::move(object), allocator);
    return true;
}
} // namespace HazzureLineups
