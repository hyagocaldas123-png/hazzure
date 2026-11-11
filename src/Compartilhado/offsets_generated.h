// Generated automatically by tools/offset_dumper.py
// Timestamp: 2026-11-05 03:42:07 UTC
// CS2 Target Build: 14194 | Mode: PE64_STATIC_SIGSCAN (TimeDateStamp=0x6AEBC4DA, SizeOfImage=0x29A3000)
#pragma once
#include <cstdint>

namespace HazzureOffsets {
    inline constexpr uint32_t kBuildNumber = 14194;

    namespace Globals {
        inline uintptr_t dwCSGOInput = 0x2240C80;
        inline uintptr_t dwEntityList = 0x271FBB8;
        inline uintptr_t dwGameRules = 0x2567188;
        inline uintptr_t dwGlobalVars = 0x22320E8;
        inline uintptr_t dwGlowManager = 0x2563EB0;
        inline uintptr_t dwLocalPlayerController = 0x2542548;
        inline uintptr_t dwLocalPlayerPawn = 0x256A948;
        inline uintptr_t dwPlantedC4 = 0x24D0B70;
        inline uintptr_t dwPrediction = 0x25675E0;
        inline uintptr_t dwSensitivity = 0x2567C48;
        // No live rip-relative reference exists in 0x6AEBC4DA; angles are read/written through CCSGOInput.
        inline uintptr_t dwViewAngles = 0;
        inline uintptr_t dwViewMatrix = 0x2570250;
        inline uintptr_t dwViewRender = 0x256D9A8;
        inline uintptr_t dwWeaponC4 = 0x24CBEE8;
    } // namespace Globals

    namespace Schema {
        inline constexpr uintptr_t dwSensitivity_sensitivity = 0x58;
        inline constexpr uintptr_t m_AttributeList = 0x1560;
        inline constexpr uintptr_t m_AttributeManager = 0x1290;
        inline constexpr uintptr_t m_Item = 0x50;
        inline constexpr uintptr_t m_angEyeAngles = 0x3600;
        inline constexpr uintptr_t m_fFlags = 0x3F4;
        inline constexpr uintptr_t m_flBrightnessScale = 0x10B4;
        inline constexpr uintptr_t m_flFallbackWear = 0x18B0;
        inline constexpr uintptr_t m_flMinLogExposure = 0x119C;
        inline constexpr uintptr_t m_hActivePostProcessingVolume = 0x200;
        inline constexpr uintptr_t m_hActiveWeapon = 0x60;
        inline constexpr uintptr_t m_hMyWearables = 0x1268;
        inline constexpr uintptr_t m_hObserverPawn = 0x930;
        inline constexpr uintptr_t m_hOwnerEntity = 0x520;
        inline constexpr uintptr_t m_hPlayerPawn = 0x92C;
        inline constexpr uintptr_t m_hSkyMaterial = 0x1098;
        inline constexpr uintptr_t m_iHealth = 0x34C;
        inline constexpr uintptr_t m_iIDEntIndex = 0x36DC;
        inline constexpr uintptr_t m_iItemDefinitionIndex = 0x149A;
        inline constexpr uintptr_t m_iItemIDHigh = 0x14B0;
        inline constexpr uintptr_t m_iItemIDLow = 0x14B4;
        inline constexpr uintptr_t m_iShotsFired = 0x1EB8;
        inline constexpr uintptr_t m_iTeamNum = 0x3E7;
        inline constexpr uintptr_t m_iszPlayerName = 0x6FC;
        inline constexpr uintptr_t m_lifeState = 0x354;
        inline constexpr uintptr_t m_modelState = 0x140;
        inline constexpr uintptr_t m_nFallbackPaintKit = 0x18A8;
        inline constexpr uintptr_t m_nFallbackSeed = 0x18AC;
        inline constexpr uintptr_t m_nFallbackStatTrak = 0x18B4;
        inline constexpr uintptr_t m_nSubclassID = 0x380;
        inline constexpr uintptr_t m_pAimPunchServices = 0x1598;
        inline constexpr uintptr_t m_pCameraServices = 0x1328;
        inline constexpr uintptr_t m_pGameSceneNode = 0x330;
        inline constexpr uintptr_t m_pSceneObjectCache = 0x10E8;
        inline constexpr uintptr_t m_pWeaponServices = 0x12F0;
        inline constexpr uintptr_t m_predictableBaseAngle = 0x50;
        inline constexpr uintptr_t m_unpredictableBaseAngle = 0xA4;
        inline constexpr uintptr_t m_vOldOrigin = 0x14A4;
        inline constexpr uintptr_t m_vTintColor = 0x10AC;
        inline constexpr uintptr_t m_vecAbsOrigin = 0xC8;
        inline constexpr uintptr_t m_vecVelocity = 0x430;
        inline constexpr uintptr_t m_vecViewOffset = 0xF60;
    } // namespace Schema

    namespace Buttons {
        inline uintptr_t attack = 0x2727F50;
        inline uintptr_t attack2 = 0x2727FE0;
        inline uintptr_t back = 0x2728220;
        inline uintptr_t duck = 0x27284F0;
        inline uintptr_t forward = 0x2728190;
        inline uintptr_t jump = 0x2728460;
        inline uintptr_t left = 0x27282B0;
        inline uintptr_t lookatweapon = 0x2240BA0;
        inline uintptr_t reload = 0x2727EC0;
        inline uintptr_t right = 0x2728340;
        inline uintptr_t showscores = 0x2240A80;
        inline uintptr_t sprint = 0x2727E30;
        inline uintptr_t turnleft = 0x2728070;
        inline uintptr_t turnright = 0x2728100;
        inline uintptr_t use = 0x27283D0;
        inline uintptr_t zoom = 0x2240B10;
    } // namespace Buttons
} // namespace HazzureOffsets
