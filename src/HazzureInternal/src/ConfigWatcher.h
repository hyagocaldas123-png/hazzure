// C++17 / ConfigWatcher.h / MSVC x64 / Hazzure Config Hot-Reloading & File Watcher
#pragma once

#include <windows.h>
#include <string>
#include <cstdint>
#include <filesystem>

namespace Persistence {

struct FileTimestampWatcher {
    std::wstring path;
    FILETIME lastWriteTime{};
    uint64_t lastCheckTick = 0;

    bool HasFileChanged(uint64_t currentTick, uint32_t throttleIntervalMs = 500) {
        if (path.empty()) return false;
        if (currentTick >= lastCheckTick && currentTick - lastCheckTick < throttleIntervalMs) return false;
        lastCheckTick = currentTick;

        WIN32_FILE_ATTRIBUTE_DATA fileData{};
        if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileData)) return false;

        if (lastWriteTime.dwLowDateTime == 0 && lastWriteTime.dwHighDateTime == 0) {
            lastWriteTime = fileData.ftLastWriteTime;
            return false;
        }

        if (CompareFileTime(&lastWriteTime, &fileData.ftLastWriteTime) != 0) {
            lastWriteTime = fileData.ftLastWriteTime;
            return true;
        }
        return false;
    }
};

inline FileTimestampWatcher g_configWatcher{ L"C:\\HazzureKernel\\hazzure_config.json" };

} // namespace Persistence
