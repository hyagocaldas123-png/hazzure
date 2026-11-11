// Hazzure In-Game Overlay UI & Visuals Engine - DirectX 11 + Dear ImGui
// Signature Clean Obsidian Monochrome Theme + Real-Time Memory Overlay
#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#pragma comment(lib, "winmm.lib")

#include "../../Compartilhado/ipc.h"
#include "../../Compartilhado/offsets_generated.h"
#include "../../HazzurePanel/third_party/imgui/imgui.h"

namespace HazzureUI {

// ---------- Paleta Monocromática Oficial Hazzure ----------
static const ImVec4 kAccent(0.961f, 0.961f, 0.976f, 1.0f);       // Branco puro #F5F5F9
static const ImVec4 kMuted(0.510f, 0.510f, 0.550f, 1.0f);        // Cinza neutro #82828C
static const ImVec4 kBorder(0.165f, 0.165f, 0.192f, 1.00f);      // Borda #2A2A31
static const ImVec4 kDarkBg(0.043f, 0.043f, 0.051f, 0.96f);      // Fundo Obsidian #0B0B0D

// Estado de aba global (garante alternância imediata de abas)
static int g_currentTab = 0;

// ---------- Estruturas Matemáticas & Geometria ----------
struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };
struct Matrix4 { float m[4][4]; };

static bool WorldToScreen(const Vec3& p, const Matrix4& matrix, int width, int height, Vec2& result) {
    if (width <= 0 || height <= 0) return false;
    const auto& m = matrix.m;
    float w = m[3][0] * p.x + m[3][1] * p.y + m[3][2] * p.z + m[3][3];
    if (!std::isfinite(w) || w < 0.001f) return false;
    float x = (m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3]) / w;
    float y = (m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3]) / w;
    result.x = (width * 0.5f) + (x * width * 0.5f);
    result.y = (height * 0.5f) - (y * height * 0.5f);
    return std::isfinite(result.x) && std::isfinite(result.y);
}

// Leitura de memória interna ultra-rápida protegida por SEH
template<typename T>
static bool SafeRead(uintptr_t addr, T* out) {
    if (!addr || !out) return false;
    __try {
        *out = *reinterpret_cast<const T*>(addr);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Conexões esqueléticas padrão CS2
static const int kBoneLinks[][2] = {
    {7,6},{6,5},{5,4},{4,3},{3,2},{2,1},
    {6,8},{8,9},{9,10},{10,11},
    {6,12},{12,13},{13,14},{14,15},
    {1,17},{17,18},{18,19},
    {1,20},{20,21},{21,22}
};

// ---------- Configurações Gerais do Overlay ----------
struct OverlayConfig {
    // ESP 2D / 3D
    bool espEnabled = true;
    bool espBoxes = true;
    int  espBoxType = 1; // 0 = Caixa Completa, 1 = Cantos (Corner)
    bool espSkeleton = true;
    bool espHealth = true;
    bool espName = true;
    bool espWeapon = true;
    bool espDistance = true;
    bool espSnaplines = false;
    bool espFovCircle = true;
    float aimFov = 4.5f;

    // Balística & Indicadores (Parte 1)
    bool offScreenArrows = true;
    bool showGrenadeTrajectory = true;
    bool showSpreadCircle = true;

    // HUD Flutuante & Estética (Parte 2)
    bool showSpectators = true;
    bool showBombTimer = true;
    bool showHitmarker = true;
    bool backgroundVignette = true;

    // Aimbot & Assist (Funcional)
    bool aimActive = true;
    float aimSmooth = 4.5f;
    int aimBone = 0; // 0 = Cabeça, 1 = Peito
    bool rcsActive = true;

    // Movimentação (Funcional)
    bool bhopActive = true;
    bool autoStrafe = false;
    bool showSpeedometer = true;
    bool showMovementGraph = true;
    bool jumpthrowActive = true;
    int jumpthrowKey = 'X';
    bool autoPistol = false;

    // Áudio Procedural DSP (Parte 3)
    bool hitSound = true;
    bool killSound = true;
    bool proceduralAudio = true;
    bool bombBeepAudio = true;
    float soundVolume = 0.8f;

    // Teclas
    int menuKey = VK_INSERT;
    int aimKey = VK_LBUTTON;

    // Cores (RGBA float)
    float colEnemy[4] = { 1.0f, 0.25f, 0.25f, 1.0f };
    float colTeam[4] = { 0.25f, 0.65f, 1.0f, 1.0f };
    float colSkeleton[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
    float colFov[4] = { 1.0f, 1.0f, 1.0f, 0.22f };
    float colSnaplines[4] = { 0.85f, 0.85f, 0.95f, 0.45f };
};

static OverlayConfig g_cfg;

// ---------- Partículas de Neve Fluida ----------
struct SnowFlake {
    float x = 0.f, y = 0.f, speedY = 0.f, swayAmp = 0.f, swayFreq = 0.f, phase = 0.f, radius = 2.f;
    int alpha = 160;
    bool isBlack = false;
};

static std::vector<SnowFlake> g_snow;
static float g_clock = 0.f;

static void InitSnow() {
    if (!g_snow.empty()) return;
    const int kCount = 85;
    g_snow.resize(kCount);
    unsigned int seed = 0x485A5234u;
    auto nextRand = [&]() -> float {
        seed = seed * 1664525u + 1013904223u;
        return (float)((seed >> 8) & 0xFFFFFF) / (float)0x1000000;
    };
    for (int i = 0; i < kCount; ++i) {
        SnowFlake& s = g_snow[i];
        s.x = nextRand();
        s.y = nextRand();
        s.isBlack = (i % 4 == 0);
        float layer = nextRand();
        if (layer > 0.65f) {
            s.radius = s.isBlack ? (2.4f + nextRand() * 1.8f) : (2.0f + nextRand() * 1.8f);
            s.speedY = 65.f + nextRand() * 45.f;
            s.swayAmp = 12.f + nextRand() * 10.f;
            s.swayFreq = 1.0f + nextRand() * 1.2f;
            s.alpha = s.isBlack ? 210 : (140 + (int)(nextRand() * 80.f));
        } else {
            s.radius = s.isBlack ? (1.5f + nextRand() * 1.0f) : (1.2f + nextRand() * 1.0f);
            s.speedY = 35.f + nextRand() * 25.f;
            s.swayAmp = 6.f + nextRand() * 8.f;
            s.swayFreq = 0.7f + nextRand() * 0.9f;
            s.alpha = s.isBlack ? 180 : (100 + (int)(nextRand() * 60.f));
        }
        s.phase = nextRand() * 6.2831853f;
    }
}

static void RenderSnow(ImDrawList* draw, ImVec2 pos, ImVec2 size) {
    InitSnow();
    float dt = ImGui::GetIO().DeltaTime;
    if (dt <= 0.f || dt > 0.1f) dt = 0.016f;
    g_clock += dt;

    draw->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    for (auto& s : g_snow) {
        s.y += (s.speedY * dt) / size.y;
        if (s.y > 1.05f) {
            s.y = -0.05f;
            s.x = std::fmod(s.x + 0.31f, 1.0f);
        }
        float sway = std::sin(g_clock * s.swayFreq + s.phase) * s.swayAmp;
        float px = pos.x + (s.x * size.x) + sway;
        float py = pos.y + (s.y * size.y);

        ImU32 col = s.isBlack ? IM_COL32(18, 18, 22, s.alpha) : IM_COL32(235, 235, 245, s.alpha);
        draw->AddCircleFilled(ImVec2(px, py), s.radius, col, 12);
    }
    draw->PopClipRect();
}

// ---------- Conversão de Cores IPC (BGRA / RGBA) ----------
static uint32_t Float4ToIpcCol(const float c[4]) {
    uint32_t r = static_cast<uint32_t>(c[0] * 255.f) & 0xFF;
    uint32_t g = static_cast<uint32_t>(c[1] * 255.f) & 0xFF;
    uint32_t b = static_cast<uint32_t>(c[2] * 255.f) & 0xFF;
    uint32_t a = static_cast<uint32_t>(c[3] * 255.f) & 0xFF;
    return b | (g << 8) | (r << 16) | (a << 24);
}

static void IpcColToFloat4(uint32_t col, float c[4]) {
    c[2] = ((col) & 0xFF) / 255.f;         // B
    c[1] = ((col >> 8) & 0xFF) / 255.f;    // G
    c[0] = ((col >> 16) & 0xFF) / 255.f;   // R
    c[3] = ((col >> 24) & 0xFF) / 255.f;   // A
}

static ImU32 Float4ToImU32(const float c[4]) {
    return IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f), (int)(c[2] * 255.f), (int)(c[3] * 255.f));
}

// ==========================================================
// PARTE 3: ÁUDIO & DSP SINTETIZADO EM MEMÓRIA (ZERO ARQUIVOS)
// ==========================================================
#pragma pack(push, 1)
struct WavPcmHeader {
    char riff[4] = { 'R', 'I', 'F', 'F' };
    uint32_t chunkSize = 0;
    char wave[4] = { 'W', 'A', 'V', 'E' };
    char fmt[4] = { 'f', 'm', 't', ' ' };
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1; // Mono
    uint32_t sampleRate = 44100;
    uint32_t byteRate = 44100 * 2;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char data[4] = { 'd', 'a', 't', 'a' };
    uint32_t subchunk2Size = 0;
};
#pragma pack(pop)

static std::vector<char> g_hsWav;
static std::vector<char> g_bodyWav;
static std::vector<char> g_c4BeepWav;
static bool g_audioInitDone = false;

static std::vector<char> SynthesizePcmTone(float startFreq, float endFreq, float durationSec, float decayRate) {
    const uint32_t sampleRate = 44100;
    uint32_t totalSamples = static_cast<uint32_t>(sampleRate * durationSec);
    uint32_t pcmBytes = totalSamples * sizeof(int16_t);

    std::vector<char> buffer(sizeof(WavPcmHeader) + pcmBytes);
    WavPcmHeader* hdr = reinterpret_cast<WavPcmHeader*>(buffer.data());
    hdr->chunkSize = static_cast<uint32_t>(sizeof(WavPcmHeader) - 8 + pcmBytes);
    hdr->subchunk2Size = pcmBytes;

    int16_t* samples = reinterpret_cast<int16_t*>(buffer.data() + sizeof(WavPcmHeader));
    double phase = 0.0;
    const double twoPi = 6.283185307179586;

    for (uint32_t i = 0; i < totalSamples; ++i) {
        float t = (float)i / (float)sampleRate;
        float progress = (float)i / (float)totalSamples;
        float currentFreq = startFreq + (endFreq - startFreq) * progress;
        phase += twoPi * currentFreq / sampleRate;
        if (phase > twoPi) phase -= twoPi;

        float envelope = std::exp(-t * decayRate);
        float wave = static_cast<float>(std::sin(phase)) * envelope;
        samples[i] = static_cast<int16_t>(std::clamp(wave * 28000.f, -32767.f, 32767.f));
    }
    return buffer;
}

static void EnsureProceduralAudio() {
    if (g_audioInitDone) return;
    g_hsWav = SynthesizePcmTone(1950.f, 1350.f, 0.085f, 22.f);
    g_bodyWav = SynthesizePcmTone(480.f, 220.f, 0.065f, 28.f);
    g_c4BeepWav = SynthesizePcmTone(1150.f, 1150.f, 0.038f, 12.f);
    g_audioInitDone = true;
}

static void PlayProceduralHit(bool headshot, float) {
    if (!g_cfg.proceduralAudio) return;
    EnsureProceduralAudio();
    const auto& wav = headshot ? g_hsWav : g_bodyWav;
    if (!wav.empty()) {
        PlaySoundA(reinterpret_cast<LPCSTR>(wav.data()), NULL, SND_MEMORY | SND_ASYNC);
    }
}

static void PlayProceduralC4Beep() {
    if (!g_cfg.bombBeepAudio) return;
    EnsureProceduralAudio();
    if (!g_c4BeepWav.empty()) {
        PlaySoundA(reinterpret_cast<LPCSTR>(g_c4BeepWav.data()), NULL, SND_MEMORY | SND_ASYNC);
    }
}

// ---------- Hitmarker Visual State ----------
static ULONGLONG g_hitmarkerExpiry = 0;
static bool g_hitmarkerHeadshot = false;

static void TriggerHitmarker(bool headshot) {
    g_hitmarkerHeadshot = headshot;
    g_hitmarkerExpiry = GetTickCount64() + 320;
    if (g_cfg.hitSound) {
        PlayProceduralHit(headshot, g_cfg.soundVolume);
    }
}

static void RenderHitmarker(ImDrawList* draw, int screenW, int screenH) {
    ULONGLONG now = GetTickCount64();
    if (now >= g_hitmarkerExpiry) return;

    float remain = (float)(g_hitmarkerExpiry - now) / 320.0f;
    int alpha = std::clamp((int)(remain * 255.f), 0, 255);

    ImVec2 c((float)screenW * 0.5f, (float)screenH * 0.5f);
    ImU32 col = g_hitmarkerHeadshot ? IM_COL32(255, 55, 55, alpha) : IM_COL32(245, 245, 250, alpha);

    const float l1 = 4.f;
    const float l2 = 11.f;

    draw->AddLine(ImVec2(c.x - l2, c.y - l2), ImVec2(c.x - l1, c.y - l1), col, 1.8f);
    draw->AddLine(ImVec2(c.x + l2, c.y + l2), ImVec2(c.x + l1, c.y + l1), col, 1.8f);
    draw->AddLine(ImVec2(c.x - l2, c.y + l2), ImVec2(c.x - l1, c.y + l1), col, 1.8f);
    draw->AddLine(ImVec2(c.x + l2, c.y - l2), ImVec2(c.x + l1, c.y - l1), col, 1.8f);
}

// ==========================================================
// PARTE 1: BALÍSTICA & FÍSICA PREDITIVA
// ==========================================================
static void RenderGrenadeTrajectory(ImDrawList* draw, uintptr_t localPawn, const Matrix4& matrix, int screenW, int screenH) {
    if (!g_cfg.showGrenadeTrajectory || !localPawn) return;

    Vec3 eyePos{};
    if (!SafeRead(localPawn + HazzureOffsets::Schema::m_vecAbsOrigin, &eyePos)) return;
    eyePos.z += 64.f;

    Vec3 forward{ matrix.m[0][2], matrix.m[1][2], matrix.m[2][2] };
    float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (len < 0.001f) return;
    forward.x /= len; forward.y /= len; forward.z /= len;

    Vec3 vel = { forward.x * 1050.f, forward.y * 1050.f, forward.z * 1050.f };

    const float dt = 0.04f;
    const float gravity = 800.f;
    Vec3 curr = eyePos;
    Vec2 prevScreen{};
    bool hasPrev = WorldToScreen(curr, matrix, screenW, screenH, prevScreen);

    for (int step = 0; step < 35; ++step) {
        curr.x += vel.x * dt;
        curr.y += vel.y * dt;
        curr.z += vel.z * dt;
        vel.z -= gravity * dt;

        Vec2 currScreen{};
        if (WorldToScreen(curr, matrix, screenW, screenH, currScreen)) {
            if (hasPrev) {
                float progress = (float)step / 35.f;
                ImU32 col = IM_COL32((int)(255 * (1.f - progress)), (int)(220 + 35 * progress), 255, 210);
                draw->AddLine(ImVec2(prevScreen.x, prevScreen.y), ImVec2(currScreen.x, currScreen.y), col, 1.8f);
            }
            prevScreen = currScreen;
            hasPrev = true;
        } else {
            hasPrev = false;
        }
    }

    if (hasPrev) {
        draw->AddCircleFilled(ImVec2(prevScreen.x, prevScreen.y), 4.5f, IM_COL32(255, 60, 60, 230), 16);
        draw->AddCircle(ImVec2(prevScreen.x, prevScreen.y), 6.5f, IM_COL32(255, 255, 255, 200), 16, 1.2f);
    }
}

static void RenderSpreadCircle(ImDrawList* draw, int screenW, int screenH) {
    if (!g_cfg.showSpreadCircle) return;
    ImVec2 center((float)screenW * 0.5f, (float)screenH * 0.5f);
    float radius = 8.5f;
    draw->AddCircle(center, radius, IM_COL32(245, 245, 250, 160), 32, 1.2f);
    draw->AddCircle(center, 1.5f, IM_COL32(255, 55, 55, 220), 12, 1.0f);
}

// ==========================================================
// PARTE 4: MOVIMENTAÇÃO & VELOCÍMETRO
// ==========================================================
static std::vector<float> g_speedHistory;

static void RenderSpeedometer(ImDrawList* draw, uintptr_t localPawn, int screenW, int screenH) {
    if (!g_cfg.showSpeedometer || !localPawn) return;

    Vec3 vel{};
    SafeRead(localPawn + 0x3E8, &vel);
    float speed = std::hypot(vel.x, vel.y);
    if (!std::isfinite(speed)) speed = 0.f;

    if (g_speedHistory.size() < 60) g_speedHistory.resize(60, 0.f);
    g_speedHistory.erase(g_speedHistory.begin());
    g_speedHistory.push_back(speed);

    float boxW = 160.f;
    float boxH = 46.f;
    float posX = screenW * 0.5f - boxW * 0.5f;
    float posY = screenH - 110.f;

    draw->AddRectFilled(ImVec2(posX, posY), ImVec2(posX + boxW, posY + boxH), IM_COL32(11, 11, 13, 210), 6.f);
    draw->AddRect(ImVec2(posX, posY), ImVec2(posX + boxW, posY + boxH), IM_COL32(42, 42, 49, 230), 6.f, 0, 1.2f);

    char buf[32];
    sprintf_s(buf, "SPEED  %d u/s", (int)speed);
    draw->AddText(ImVec2(posX + 14.f, posY + 6.f), IM_COL32(245, 245, 250, 240), buf);

    if (g_cfg.showMovementGraph) {
        for (int i = 1; i < 60; ++i) {
            float s1 = g_speedHistory[i - 1];
            float s2 = g_speedHistory[i];
            float h1 = std::clamp(s1 / 380.f, 0.f, 1.f) * 18.f;
            float h2 = std::clamp(s2 / 380.f, 0.f, 1.f) * 18.f;

            float x1 = posX + 14.f + (i - 1) * 2.2f;
            float y1 = (posY + boxH - 6.f) - h1;
            float x2 = posX + 14.f + i * 2.2f;
            float y2 = (posY + boxH - 6.f) - h2;

            ImU32 lineCol = (s2 >= s1) ? IM_COL32(60, 240, 120, 210) : IM_COL32(240, 70, 70, 210);
            draw->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), lineCol, 1.4f);
        }
    }
}

static void ProcessMovementMechanics(uintptr_t localPawn) {
    // 1. BunnyHop Funcional
    if (g_cfg.bhopActive && (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
        if (localPawn) {
            int32_t flags = 0;
            SafeRead(localPawn + HazzureOffsets::Schema::m_fFlags, &flags);
            if (flags & 1) { // FL_ONGROUND
                keybd_event(VK_SPACE, 0, 0, 0);
                keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
            }
        }
    }

    // 2. Jumpthrow Funcional
    if (g_cfg.jumpthrowActive && g_cfg.jumpthrowKey > 0) {
        if (GetAsyncKeyState(g_cfg.jumpthrowKey) & 1) {
            keybd_event(VK_SPACE, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
        }
    }

    // 3. Auto-Pistol
    if (g_cfg.autoPistol && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        static ULONGLONG lastClick = 0;
        ULONGLONG now = GetTickCount64();
        if (now - lastClick > 45) {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            lastClick = now;
        }
    }
}

// ==========================================================
// RENDERIZAÇÃO ESP & AIMBOT (CHUNK SCANNING VERIFICADO CS2)
// ==========================================================
static void RenderInGameESP(Shared* shared, uintptr_t client, int screenW, int screenH) {
    if (!client) {
        client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
        if (!client) return;
    }

    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    // 1. Círculo de FOV do Aimbot
    if (g_cfg.espFovCircle) {
        ImVec2 center((float)screenW * 0.5f, (float)screenH * 0.5f);
        float radius = g_cfg.aimFov * 14.f;
        draw->AddCircle(center, radius, Float4ToImU32(g_cfg.colFov), 64, 1.2f);
    }

    // 2. Leitura da Matriz de Visão e LocalPawn
    Matrix4 matrix{};
    if (!SafeRead(client + HazzureOffsets::Globals::dwViewMatrix, &matrix)) return;

    uintptr_t localPawn = 0;
    SafeRead(client + HazzureOffsets::Globals::dwLocalPlayerPawn, &localPawn);

    int32_t localTeam = 0;
    Vec3 localOrigin{};
    if (localPawn) {
        SafeRead(localPawn + HazzureOffsets::Schema::m_iTeamNum, &localTeam);
        SafeRead(localPawn + HazzureOffsets::Schema::m_vecAbsOrigin, &localOrigin);
    }

    uintptr_t entityList = 0;
    if (!SafeRead(client + HazzureOffsets::Globals::dwEntityList, &entityList) || !entityList) return;
