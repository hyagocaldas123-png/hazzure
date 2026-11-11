// ---------- Calculo de Caixa Delimitadora Dinamica via Ossos Validados ----------
static bool ComputeDynamicPlayerBox(
    uintptr_t currentPawn,
    uintptr_t boneArray,
    const Vec3& origin,
    const Matrix4& matrix,
    int screenW,
    int screenH,
    float* outLeft,
    float* outTop,
    float* outRight,
    float* outBottom,
    Vec2* outHeadScreen,
    Vec2* outFootScreen
) {
    Vec3 topWorld = { origin.x, origin.y, origin.z + 72.f };
    Vec3 botWorld = { origin.x, origin.y, origin.z - 2.f };

    if (boneArray) {
        Vec3 headBone{};
        if (ReadBonePos(boneArray, BONE_HEAD, &headBone)) {
            topWorld = { headBone.x, headBone.y, headBone.z + 8.5f };
        }
        float minFootZ = origin.z;
        Vec3 ankleL{}, ankleR{};
        if (ReadBonePos(boneArray, BONE_ANKLE_L, &ankleL)) minFootZ = std::min(minFootZ, ankleL.z);
        if (ReadBonePos(boneArray, BONE_ANKLE_R, &ankleR)) minFootZ = std::min(minFootZ, ankleR.z);
        botWorld = { origin.x, origin.y, minFootZ - 3.0f };
    } else {
        Vec3 viewOff{};
        if (SafeRead(currentPawn + HazzureOffsets::Schema::m_vecViewOffset, &viewOff) &&
            std::isfinite(viewOff.z) && viewOff.z >= 28.f && viewOff.z <= 76.f) {
            topWorld.z = origin.z + viewOff.z + 8.5f;
        }
    }

    if (!WorldToScreen(topWorld, matrix, screenW, screenH, *outHeadScreen) ||
        !WorldToScreen(botWorld, matrix, screenW, screenH, *outFootScreen)) {
        return false;
    }

    float topY = std::min(outHeadScreen->y, outFootScreen->y);
    float botY = std::max(outHeadScreen->y, outFootScreen->y);
    float height = botY - topY;
    if (height < 4.f) return false;

    float centerX = (outHeadScreen->x + outFootScreen->x) * 0.5f;
    float minX = centerX - height * 0.22f;
    float maxX = centerX + height * 0.22f;

    if (boneArray) {
        static const int kHullBones[] = { 0, 4, 6, 8, 9, 10, 13, 14, 15, 18, 19, 21, 22 };
        for (int bIdx : kHullBones) {
            Vec3 bp{};
            Vec2 sp{};
            if (ReadBonePos(boneArray, bIdx, &bp) && WorldToScreen(bp, matrix, screenW, screenH, sp)) {
                minX = std::min(minX, sp.x - 4.f);
                maxX = std::max(maxX, sp.x + 4.f);
                topY = std::min(topY, sp.y - 3.f);
                botY = std::max(botY, sp.y + 3.f);
            }
        }
        height = botY - topY;
        float spanX = maxX - minX;
        float maxAllowedW = height * 0.82f;
        if (spanX > maxAllowedW) {
            float mid = (minX + maxX) * 0.5f;
            minX = mid - maxAllowedW * 0.5f;
            maxX = mid + maxAllowedW * 0.5f;
        }
    } else {
        minX = centerX - height * 0.25f;
        maxX = centerX + height * 0.25f;
    }

    *outLeft = minX;
    *outRight = maxX;
    *outTop = topY;
    *outBottom = botY;
    return true;
}

// ==========================================================
// RENDERIZACAO ESP, AIMBOT POR OSSO REAL, RCS & TRIGGERBOT
// ==========================================================
#include "GameplayRuntime.h"

