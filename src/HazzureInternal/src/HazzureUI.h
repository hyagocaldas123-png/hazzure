// Hazzure In-Game Overlay UI & Visuals Engine - DirectX 11 + Dear ImGui
// Clean Studio Minimalist Monochrome UI + Real-Time Memory Overlay (Build 14192)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <mmsystem.h>
#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <map>
#pragma comment(lib, "winmm.lib")

#include "../../Compartilhado/ipc.h"
#include "../../Compartilhado/offsets_generated.h"
#include "DebugLogger.h"
#include "../../HazzurePanel/third_party/imgui/imgui.h"
#include "DynamicSchema.h"
#include "StateRegistry.h"
#include "FeatureTelemetry.h"
#include "PerformanceMetrics.h"
#include "PreviewAssets.h"
#include "Typography.h"
#include "CustomAudio.h"
#include "WorldAppearance.h"
#include "SceneColorGrading.h"
#include "VisualGeometry.h"
#include "RenderOptions.h"
#include "GameData.h"
#include "Visibility.h"
#include "GrenadePhysics.h"
#include "VisualRecoil.h"
#include "Gunplay.h"
#include "RuntimeStability.h"
#include "InputView.h"
#include "WorldSession.h"
#ifndef USE_MINIMAL_BUILD
#include "GlovePresets.h"
#endif
#include "ShareCodes.h"
#include "BasicTuning.h"
#include "CameraFov.h"
#include "NativeCamera.h"
#include "InputContext.h"
#include "TacticalHUD.h"
#include "ScopeOverlay.h"
#include "CombatVisuals.h"
#include "PoseHistory.h"
#include "GrenadeZones.h"
#include "InfernoCells.h"
#include "CombatRefinements.h"
#include "ArmoryJSON.h"
#ifndef USE_MINIMAL_BUILD
#include "LocalInventory.h"
#endif
#include "DX11Blur.h"
#include "WeaponChoices.h"
#include "MovementTech.h"
#include "AdvancedMovement.h"
#include "CombatAccuracy.h"
#include "WeaponAccuracy.h"
#include "ShotBrake.h"
#include "CombatConvenience.h"
#include "SmokeDensity.h"
#include "Autowall.h"
#include "LineupLibrary.h"
#include "LineupAutomation.h"
#include "LineupCatalogue.h"
#include "MapContext.h"
#include "ConfigStorage.h"
#ifndef USE_MINIMAL_BUILD
#include "../../HazzurePanel/src/skin_catalog.generated.h"
#endif
#include "WorldTrace.h"
#include "WorldWeather.h"
#include "KillFeed.h"

#include "ModelPreview.h"
#include "TsukuyomiOrb.h"
#ifndef USE_MINIMAL_BUILD
#include "NativeInventoryRuntime.h"
#endif



namespace HazzureUI {

#include "UIState.inl"
#include "UIAudio.inl"
#include "VisualDrawing.inl"
#include "UIWorldDrawing.inl"
#include "UIMovement.inl"
#include "UIPlayerGeometry.inl"
#include "UITacticalHud.inl"
#include "UIWidgets.inl"
#include "UIProfiles.inl"
#ifndef USE_MINIMAL_BUILD
#include "GSWidgets.inl"
#endif
#ifndef USE_MINIMAL_BUILD
#include "UIPanel.inl"
#endif
#include "GSLayout.inl"
#include "UIFrame.inl"
} // namespace HazzureUI
