#pragma once

// HazzureInternal/src/UISettingsCards.inl — Frente 2.4 Agregador Modular de Cards Enigma CS2
// Divide o antigo arquivo monolitico de 601 linhas em 6 modulos coesos em src/cards/

#include "UIEquipmentPreviews.inl"

#include "cards/CardCombat.inl"
#include "cards/CardVisuals.inl"
#include "cards/CardMovement.inl"
#include "cards/CardCosmetics.inl"
#include "cards/CardEnvironment.inl"
#include "cards/CardProfiles.inl"

// ============================================================================
// Forwarders Legados para Compatibilidade Total com UIPanel.inl e Testes
// ============================================================================
static void DrawSettingsCard0(Shared* shared)  { DrawVisualsPlayerEspCard(shared); }
static void DrawSettingsCard1(Shared* shared)  { DrawVisualsWorldTacticCard(); DrawVisualsKillfeedCard(); }
static void DrawSettingsCard2(Shared* shared)  { DrawVisualsRadarFlashCard(); }
static void DrawSettingsCard3(Shared* shared)  { DrawVisualsChamsCard(shared); }
static void DrawSettingsCard4(Shared* shared)  { DrawVisualsChamsGradientCard(shared); }
static void DrawSettingsCard5(Shared* shared)  { DrawVisualsChamsMaterialCard(shared); }
static void DrawSettingsCard6(Shared* shared)  { DrawCombatAimbotCard(); }
static void DrawSettingsCard7(Shared* shared)  { DrawCombatTrajectoryCard(); }
static void DrawSettingsCard8(Shared* shared)  { DrawCombatRecoilCard(); }
static void DrawSettingsCard9(Shared* shared)  { DrawCombatReactionCard(); }
static void DrawSettingsCard10(Shared* shared) { DrawMovementBasicCard(); }
static void DrawSettingsCard11(Shared* shared) { DrawMovementAutomationCard(); }
static void DrawSettingsCard12(Shared* shared) { DrawEnvironmentStabilityCard(); }
static void DrawSettingsCard13(Shared* shared) { DrawEnvironmentAudioCard(); }
static void DrawSettingsCard14(Shared* shared) { DrawEnvironmentParticlesCard(shared); DrawEnvironmentWeather3DCard(); }
static void DrawSettingsCard15(Shared* shared) { DrawEnvironmentSkyboxAtmosphereCard(shared); DrawEnvironmentClimatePresetCard(); DrawEnvironmentColorGradingCard(); }
static void DrawSettingsCard16(Shared* shared) { DrawCosmeticsAgentsCard(shared); }
static void DrawSettingsCard17(Shared* shared) { DrawCosmeticsSkinsCard(shared); }
static void DrawSettingsCard18(Shared* shared) { DrawProfilesPresetsCard(shared); }
static void DrawSettingsCard19(Shared* shared) { DrawProfilesOverridesCard(); }
static void DrawSettingsCard20(Shared* shared) { DrawProfilesDiagnosticsCard(); DrawProfilesTelemetryCard(shared); }
