// C++ / FeatureTelemetry.h / MSVC x64 C++17
// Telemetria Interna por Feature: Monitoramento de Taxas, Delays e Estatisticas
#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <numeric>
#include <algorithm>

namespace HazzureTelemetry {

struct TelemetryData {
    // Triggerbot
    uint64_t triggerClicksTotal = 0;
    float triggerClicksPerMinute = 0.0f;
    int triggerLastDelayMs = 0;
    float triggerAvgDelayMs = 0.0f;
    std::vector<int> triggerDelayHistory;
    std::vector<ULONGLONG> triggerClickTimestamps;

    // Chams & Materiais
    uint64_t chamsMeshBatches = 0;
    uint64_t chamsMaterialOverrides = 0;
    float chamsOverridesPerSec = 0.0f;
    std::vector<ULONGLONG> chamsOverrideTimestamps;

    // Clima & Atmosfera
    uint64_t skyboxSwaps = 0;
    uint64_t exposureModulations = 0;

    // Performance
    float renderFrameTimeMs = 0.0f;
};

inline TelemetryData g_metrics;

inline void RecordTriggerClick(int delayApplied) {
    ULONGLONG now = GetTickCount64();
    g_metrics.triggerClicksTotal++;
    g_metrics.triggerLastDelayMs = delayApplied;

    g_metrics.triggerDelayHistory.push_back(delayApplied);
    if (g_metrics.triggerDelayHistory.size() > 30) {
        g_metrics.triggerDelayHistory.erase(g_metrics.triggerDelayHistory.begin());
    }

    if (!g_metrics.triggerDelayHistory.empty()) {
        float sum = (float)std::accumulate(g_metrics.triggerDelayHistory.begin(), g_metrics.triggerDelayHistory.end(), 0);
        g_metrics.triggerAvgDelayMs = sum / (float)g_metrics.triggerDelayHistory.size();
    }

    g_metrics.triggerClickTimestamps.push_back(now);
}

inline void RecordChamsOverride() {
    ULONGLONG now = GetTickCount64();
    g_metrics.chamsMaterialOverrides++;
    g_metrics.chamsOverrideTimestamps.push_back(now);
}

inline void UpdateTelemetryRates() {
    ULONGLONG now = GetTickCount64();

    // 1. Calcula cliques por minuto (janela de 60 segundos)
    while (!g_metrics.triggerClickTimestamps.empty() && now - g_metrics.triggerClickTimestamps.front() > 60000) {
        g_metrics.triggerClickTimestamps.erase(g_metrics.triggerClickTimestamps.begin());
    }
    g_metrics.triggerClicksPerMinute = (float)g_metrics.triggerClickTimestamps.size();

    // 2. Calcula overrides por segundo (janela de 1 segundo)
    while (!g_metrics.chamsOverrideTimestamps.empty() && now - g_metrics.chamsOverrideTimestamps.front() > 1000) {
        g_metrics.chamsOverrideTimestamps.erase(g_metrics.chamsOverrideTimestamps.begin());
    }
    g_metrics.chamsOverridesPerSec = (float)g_metrics.chamsOverrideTimestamps.size();
}

} // namespace HazzureTelemetry
