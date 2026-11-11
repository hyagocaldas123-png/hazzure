<!-- NAO EDITAR A MAO (offsets): gerado por tools/vault_sync.py
     Fonte da verdade = codigo-fonte + C:/HazzureKernel. Rodar: python tools/vault_sync.py -->
# Offsets e RVAs (gerado)

_Gerado por `python tools/vault_sync.py` a partir de `Compartilhado/offsets_generated.h`, que por sua vez é produzido por `python tools/offset_dumper.py` contra o `client.dll` instalado. Não edite à mão._

> Generated automatically by tools/offset_dumper.py

> Timestamp: 2026-11-05 03:42:07 UTC

> CS2 Target Build: 14194 \| Mode: PE64_STATIC_SIGSCAN (TimeDateStamp=0x6AEBC4DA, SizeOfImage=0x29A3000)

Todos os valores são **RVAs relativos** ao handle do módulo, nunca endereços absolutos: a base muda a cada boot.

### `HazzureOffsets`

| símbolo | valor |
|---|---|
| `kBuildNumber` | `14194` |

### `Globals`

| símbolo | valor |
|---|---|
| `dwCSGOInput` | `0x2240C80` |
| `dwEntityList` | `0x271FBB8` |
| `dwGameRules` | `0x2567188` |
| `dwGlobalVars` | `0x22320E8` |
| `dwGlowManager` | `0x2563EB0` |
| `dwLocalPlayerController` | `0x2542548` |
| `dwLocalPlayerPawn` | `0x256A948` |
| `dwPlantedC4` | `0x24D0B70` |
| `dwPrediction` | `0x25675E0` |
| `dwSensitivity` | `0x2567C48` |
| `dwViewAngles` | `0` |
| `dwViewMatrix` | `0x2570250` |
| `dwViewRender` | `0x256D9A8` |
| `dwWeaponC4` | `0x24CBEE8` |

### `Schema`

| símbolo | valor |
|---|---|
| `dwSensitivity_sensitivity` | `0x58` |
| `m_AttributeList` | `0x1560` |
| `m_AttributeManager` | `0x1290` |
| `m_Item` | `0x50` |
| `m_angEyeAngles` | `0x3600` |
| `m_fFlags` | `0x3F4` |
| `m_flBrightnessScale` | `0x10B4` |
| `m_flFallbackWear` | `0x18B0` |
| `m_flMinLogExposure` | `0x119C` |
| `m_hActivePostProcessingVolume` | `0x200` |
| `m_hActiveWeapon` | `0x60` |
| `m_hMyWearables` | `0x1268` |
| `m_hObserverPawn` | `0x930` |
| `m_hOwnerEntity` | `0x520` |
| `m_hPlayerPawn` | `0x92C` |
| `m_hSkyMaterial` | `0x1098` |
| `m_iHealth` | `0x34C` |
| `m_iIDEntIndex` | `0x36DC` |
| `m_iItemDefinitionIndex` | `0x149A` |
| `m_iItemIDHigh` | `0x14B0` |
| `m_iItemIDLow` | `0x14B4` |
| `m_iShotsFired` | `0x1EB8` |
| `m_iTeamNum` | `0x3E7` |
| `m_iszPlayerName` | `0x6FC` |
| `m_lifeState` | `0x354` |
| `m_modelState` | `0x140` |
| `m_nFallbackPaintKit` | `0x18A8` |
| `m_nFallbackSeed` | `0x18AC` |
| `m_nFallbackStatTrak` | `0x18B4` |
| `m_nSubclassID` | `0x380` |
| `m_pAimPunchServices` | `0x1598` |
| `m_pCameraServices` | `0x1328` |
| `m_pGameSceneNode` | `0x330` |
| `m_pSceneObjectCache` | `0x10E8` |
| `m_pWeaponServices` | `0x12F0` |
| `m_predictableBaseAngle` | `0x50` |
| `m_unpredictableBaseAngle` | `0xA4` |
| `m_vOldOrigin` | `0x14A4` |
| `m_vTintColor` | `0x10AC` |
| `m_vecAbsOrigin` | `0xC8` |
| `m_vecVelocity` | `0x430` |
| `m_vecViewOffset` | `0xF60` |

### `Buttons`

| símbolo | valor |
|---|---|
| `attack` | `0x2727F50` |
| `attack2` | `0x2727FE0` |
| `back` | `0x2728220` |
| `duck` | `0x27284F0` |
| `forward` | `0x2728190` |
| `jump` | `0x2728460` |
| `left` | `0x27282B0` |
| `lookatweapon` | `0x2240BA0` |
| `reload` | `0x2727EC0` |
| `right` | `0x2728340` |
| `showscores` | `0x2240A80` |
| `sprint` | `0x2727E30` |
| `turnleft` | `0x2728070` |
| `turnright` | `0x2728100` |
| `use` | `0x27283D0` |
| `zoom` | `0x2240B10` |

## Funções assinadas (`src/SkinAddresses.h`)

Cada RVA só é usado depois de conferir os 16 bytes da assinatura; se o build do CS2 mudar, a checagem falha e o caminho é re-dumpar a função, não remapear no chute.

| símbolo | RVA (`client+`) | assinatura (16 bytes) |
|---|---|---|
| `SKIN_FRAME` | `0x0B6EFE0` | `48 89 5C 24 18 48 89 6C 24 20 57 48 83 EC 40 48` |
| `SKIN_SET_ATTR` | `0x11A5820` | `40 53 48 83 EC 20 48 8B D9 48 81 C1 08 02 00 00` |
| `SKIN_UPDATE_SKIN` | `0x08214F0` | `48 89 5C 24 08 57 48 83 EC 20 8B DA 48 8B F9 E8` |
| `SKIN_CLEAR_MATERIAL` | `0x08214F0` | `48 89 5C 24 08 57 48 83 EC 20 8B DA 48 8B F9 E8` |
| `SKIN_SET_MODEL` | `0x0965C90` | `40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 0D 3D` |
| `SKIN_SUBCLASS` | `0x0B07DB0` | `40 53 48 83 EC 20 48 8B D9 E8 42 12 70 FF 48 83` |
| `SKIN_MESH_MASK` | `0x0AC5A90` | `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48` |
| `SKIN_SET_BODY_GROUP` | `0x0964BB0` | `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 41` |
| `SKIN_UPDATE_BODY_GROUPS` | `0x0955FF0` | `48 8B C4 55 48 8B EC 48 83 EC 70 48 89 58 10 48` |

---

**73 offsets** no header gerado + RVAs assinados acima.

<!-- gerado por tools/vault_sync.py — corpo dea9d6cd39f3 -->
