// Shared native IPC v2. Preserve the ABI used by the C# runtime.
#pragma once
#include <windows.h>
#include <cstdint>
#include <cstddef>

static_assert(sizeof(void*)==8,"IPC requires x64");
struct SkinConfig {
    int definition,paint,seed,model;
    float wear;
    int legacy,enabled,reserved;
};
static_assert(sizeof(SkinConfig)==32,"Skin IPC layout");
struct Shared {
    uint32_t magic,version;
    volatile LONG flags,status;
    volatile LONG64 calls,changed,faults;
    char message[216];
    volatile LONG revision,appliedRevision,skinEnabled,skinStatus;
    volatile LONG64 frames,skinChanges;
    volatile LONG stageMask,frameThread;
    uint32_t visibleColour,hiddenColour,armsColour;
    volatile LONG teamFilter;
    char skinMessage[200];
    SkinConfig skins[64];
    volatile LONG64 heartbeat;
    // Appended after the heartbeat so every offset the client already published stays
    // exactly where it was; the shared mapping is 4096 bytes and had room to spare.
    volatile LONG gradient;
    uint32_t gradientColour;
    volatile LONG audioProtocol,kills;
    volatile LONG64 headshots;
    volatile LONG climateFlags,climateStatus;
    float skyBrightness,worldBrightness;
    uint32_t skyColour;
    volatile LONG skyCount;
    volatile LONG64 climateChanges;
    char climateMessage[160];
    volatile LONG64 worldDraws,skyDraws,sixFaceDraws,skyApplied;
    volatile LONG materialCount;
    LONG materialPadding;
    uintptr_t materialSamples[96];
    volatile LONG agentEnabled, agentCT, agentT;
    volatile LONG materialStyle; // 0 = Flat, 1 = Glow, 2 = Wireframe
    volatile LONG outlineGlow;   // 0 = Off, 1 = On
    volatile LONG skyboxIndex;   // 0..9
    float fogDensity;
    uint32_t fogColour;
    volatile LONG snowEnabled;
    volatile LONG cleanMap;
    volatile LONG removeWeather;
    volatile LONG uiConfigOwner;
    volatile LONG64 bodyHits;
    volatile LONG chamsVisibleStyle; // 0..4 material style for visible layer
    volatile LONG chamsHiddenStyle;  // 0..4 material style for hidden (through-wall) layer
    // stage: 5 | reason: IPC para 4 layers adicionais de chams + glow
    uint32_t historyColour, onShotColour, localColour, teamColour;
    volatile LONG chamsLayerFlags; // bit0=history, bit1=onShot, bit2=local, bit3=team, bit4=glow
    float chamsGlowScale;
    // stage: 6 | reason: climate preset params via IPC para render-thread
    volatile LONG climateActive;
    volatile LONG climatePreset; // 0=pink_violet,1=cold,2=mono,3=custom
    float climateHueShift, climateSatBoost, climateValBoost;
    float climateBloom, climateShadowStr;
    volatile LONG climateTonemap;
};
static_assert(offsetof(Shared,revision)==256,"IPC prefix");
static_assert(offsetof(Shared,skins)==512,"IPC configuration offset");
static_assert(offsetof(Shared,heartbeat)==2560,"IPC heartbeat offset");
static_assert(offsetof(Shared,gradient)==2568,"IPC gradient flags offset");
static_assert(offsetof(Shared,audioProtocol)==2576,"IPC audio capability");
static_assert(offsetof(Shared,headshots)==2584,"IPC headshot sequence");

static_assert(offsetof(Shared,heartbeat)==2560,"IPC heartbeat offset");

static_assert(offsetof(Shared,climateFlags)==2592,"IPC climate flags");
static_assert(offsetof(Shared,climateChanges)==2616,"IPC climate changes");

static_assert(sizeof(Shared)<=4096,"IPC must fit the existing mapping");
