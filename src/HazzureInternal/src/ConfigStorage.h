// C++ / ConfigStorage.h / MSVC x64 C++17 / typed JSON and atomic configuration storage
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "../references/Andromeda-CS2-Base/Andromeda-CS2-Base/Andromeda-CS2-Base/Common/Include/rapidjson/document.h"
#include "../references/Andromeda-CS2-Base/Andromeda-CS2-Base/Andromeda-CS2-Base/Common/Include/rapidjson/prettywriter.h"
#include "../references/Andromeda-CS2-Base/Andromeda-CS2-Base/Andromeda-CS2-Base/Common/Include/rapidjson/stringbuffer.h"
#include "../references/Andromeda-CS2-Base/Andromeda-CS2-Base/Andromeda-CS2-Base/Common/Include/rapidjson/error/en.h"

namespace HazzureConfig {

using Value = rapidjson::Value;
using AllocatorType = rapidjson::Document::AllocatorType;
inline constexpr size_t MaxFileBytes = 1024 * 1024;
inline constexpr unsigned MaxDepth = 64;

inline const Value* Find(const Value& object, const char* key) {
    if (!object.IsObject() || !key) return nullptr;
    auto it = object.FindMember(key);
    return it == object.MemberEnd() ? nullptr : &it->value;
}
inline Value* Find(Value& object, const char* key) {
    return const_cast<Value*>(Find(static_cast<const Value&>(object), key));
}
inline bool GetBool(const Value& object, const char* key, bool& out, bool legacyNumeric = false) {
    auto value = Find(object, key);
    if (!value) return false;
    if (value->IsBool()) { out = value->GetBool(); return true; }
    if (legacyNumeric && value->IsInt() && (value->GetInt() == 0 || value->GetInt() == 1)) {
        out = value->GetInt() != 0; return true;
    }
    return false;
}
inline bool GetInt(const Value& object, const char* key, int& out,
                   int minimum = (std::numeric_limits<int>::min)(),
                   int maximum = (std::numeric_limits<int>::max)()) {
    auto value = Find(object, key);
    if (!value || !value->IsInt64()) return false;
    auto number = value->GetInt64();
    if (number < minimum || number > maximum) return false;
    out = static_cast<int>(number); return true;
}
inline bool GetFloat(const Value& object, const char* key, float& out,
                     float minimum = -(std::numeric_limits<float>::max)(),
                     float maximum = (std::numeric_limits<float>::max)()) {
    auto value = Find(object, key);
    if (!value || !value->IsNumber()) return false;
    double number = value->GetDouble();
    if (!std::isfinite(number) || number < minimum || number > maximum) return false;
    out = static_cast<float>(number); return std::isfinite(out);
}
inline bool GetString(const Value& object, const char* key, std::string& out, size_t maximum = 4096) {
    auto value = Find(object, key);
    if (!value || !value->IsString() || value->GetStringLength() > maximum) return false;
    out.assign(value->GetString(), value->GetStringLength()); return true;
}
inline bool GetFloatArray(const Value& object, const char* key, float* out, size_t count,
                          float minimum = 0.f, float maximum = 1.f) {
    auto value = Find(object, key);
    if (!value || !value->IsArray() || value->Size() != count || (!out && count)) return false;
    std::vector<float> values; values.reserve(count);
    for (const auto& element : value->GetArray()) {
        if (!element.IsNumber()) return false;
        double number = element.GetDouble();
        if (!std::isfinite(number) || number < minimum || number > maximum) return false;
        values.push_back(static_cast<float>(number));
    }
    for (size_t i = 0; i < count; ++i) out[i] = values[i];
    return true;
}

inline bool Validate(const Value& value, std::string& error, unsigned depth = 0) {
    if (depth > MaxDepth) { error = "JSON nesting exceeds limit"; return false; }
    if (value.IsNumber() && !std::isfinite(value.GetDouble())) {
        error = "Non-finite JSON number"; return false;
    }
    if (value.IsObject()) {
        std::set<std::string> keys;
        for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
            if (!keys.emplace(it->name.GetString(), it->name.GetStringLength()).second) {
                error = "Duplicate JSON member"; return false;
            }
            if (!Validate(it->value, error, depth + 1)) return false;
        }
    } else if (value.IsArray()) {
        for (const auto& element : value.GetArray()) if (!Validate(element, error, depth + 1)) return false;
    }
    return true;
}

inline bool ParseValue(const std::string& text, rapidjson::Document& out, std::string& error) {
    error.clear();
    if (text.empty() || text.size() > MaxFileBytes) { error = "JSON file empty or too large"; return false; }
    const char* data = text.data(); size_t size = text.size();
    if (size >= 3 && static_cast<unsigned char>(data[0]) == 0xef &&
        static_cast<unsigned char>(data[1]) == 0xbb && static_cast<unsigned char>(data[2]) == 0xbf) {
        data += 3; size -= 3;
    }
    // Explicit length and encoding validation also reject embedded NUL/trailing content.
    out.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseIterativeFlag>(data, size);
    if (out.HasParseError()) {
        error = std::string(rapidjson::GetParseError_En(out.GetParseError())) +
                " at byte " + std::to_string(out.GetErrorOffset());
        return false;
    }
    if (text.find('\0') != std::string::npos) { error = "Embedded NUL in JSON input"; return false; }
    return Validate(out, error);
}

inline Value& SetValue(Value& object, const char* key, Value value, AllocatorType& allocator) {
    if (!object.IsObject()) object.SetObject();
    if (auto existing = Find(object, key)) { *existing = std::move(value); return *existing; }
    Value name; name.SetString(key, static_cast<rapidjson::SizeType>(strlen(key)), allocator);
    object.AddMember(name, value, allocator);
    return object.MemberEnd()[-1].value;
}
inline void Set(Value& object, const char* key, bool value, AllocatorType& allocator) {
    SetValue(object, key, Value(value), allocator);
}
inline void Set(Value& object, const char* key, int value, AllocatorType& allocator) {
    SetValue(object, key, Value(value), allocator);
}
inline bool Set(Value& object, const char* key, double value, AllocatorType& allocator) {
    if (!std::isfinite(value)) return false;
    SetValue(object, key, Value(value), allocator); return true;
}
inline void Set(Value& object, const char* key, const std::string& value, AllocatorType& allocator) {
    Value text; text.SetString(value.data(), static_cast<rapidjson::SizeType>(value.size()), allocator);
    SetValue(object, key, std::move(text), allocator);
}
inline void Set(Value& object, const char* key, const char* value, AllocatorType& allocator) {
    Set(object, key, std::string(value ? value : ""), allocator);
}
inline bool SetFloatArray(Value& object, const char* key, const float* values, size_t count,
                          AllocatorType& allocator) {
    if (!values && count) return false;
    Value array(rapidjson::kArrayType);
    for (size_t i = 0; i < count; ++i) {
        if (!std::isfinite(values[i])) return false;
        array.PushBack(Value(static_cast<double>(values[i])), allocator);
    }
    SetValue(object, key, std::move(array), allocator); return true;
}
inline bool SetRaw(Value& object, const char* key, const std::string& text,
                   AllocatorType& allocator, std::string& error) {
    rapidjson::Document parsed;
    if (!ParseValue(text, parsed, error)) return false;
    Value copy; copy.CopyFrom(parsed, allocator);
    SetValue(object, key, std::move(copy), allocator); return true;
}

namespace Detail {
class Handle {
    HANDLE value_;
public:
    explicit Handle(HANDLE value = INVALID_HANDLE_VALUE) : value_(value) {}
    ~Handle() { if (Valid()) CloseHandle(value_); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    bool Valid() const { return value_ && value_ != INVALID_HANDLE_VALUE; }
    HANDLE Get() const { return value_; }
    void Close() { if (Valid()) CloseHandle(value_); value_ = INVALID_HANDLE_VALUE; }
};
inline std::string WinError(const char* action) {
    return std::string(action) + " (Win32 " + std::to_string(GetLastError()) + ")";
}
inline std::filesystem::path Normalize(const std::string& path) {
    return std::filesystem::absolute(std::filesystem::u8path(path)).lexically_normal();
}
inline bool SamePath(const std::filesystem::path& a, const std::filesystem::path& b) {
    return !a.empty() && !b.empty() && _wcsicmp(a.c_str(), b.c_str()) == 0;
}
inline bool ReadHandle(HANDLE file, std::string& text, std::string& error) {
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size)) { error = WinError("Cannot read file size"); return false; }
    if (size.QuadPart < 0 || size.QuadPart > MaxFileBytes) { error = "Configuration file too large"; return false; }
    text.resize(static_cast<size_t>(size.QuadPart));
    DWORD count = 0;
    if (!text.empty() && (!ReadFile(file, &text[0], static_cast<DWORD>(text.size()), &count, nullptr) ||
                          count != text.size())) { error = WinError("Cannot read configuration"); return false; }
    return true;
}
inline std::wstring MutexName(const std::filesystem::path& path) {
    uint64_t hash = 14695981039346656037ull;
    for (wchar_t c : path.native()) { hash ^= static_cast<uint16_t>(towlower(c)); hash *= 1099511628211ull; }
    return L"Local\\HazzureConfigSave-" + std::to_wstring(hash);
}
class Lock {
    Handle handle_;
    bool held_ = false;
public:
    explicit Lock(const std::wstring& name) : handle_(CreateMutexW(nullptr, FALSE, name.c_str())) {
        if (handle_.Valid()) { auto wait = WaitForSingleObject(handle_.Get(), 5000); held_ = wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED; }
    }
    ~Lock() { if (held_) ReleaseMutex(handle_.Get()); }
    bool Held() const { return held_; }
};
inline std::wstring UniqueSuffix() {
    static std::atomic<unsigned long> serial{0};
    SYSTEMTIME now{}; GetSystemTime(&now);
    wchar_t value[128];
    swprintf_s(value, L"%04u%02u%02u_%02u%02u%02u_%03u.%lu.%lu", now.wYear, now.wMonth,
               now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
               GetCurrentProcessId(), serial.fetch_add(1));
    return value;
}
} // namespace Detail

class Document {
    rapidjson::Document value_;
    std::filesystem::path loadedPath_;
    std::string original_;
    bool tracked_ = false, existed_ = false;
public:
    Document() { value_.SetObject(); }
    Document(const Document& other) : loadedPath_(other.loadedPath_), original_(other.original_),
        tracked_(other.tracked_), existed_(other.existed_) { value_.CopyFrom(other.value_, value_.GetAllocator()); }
    Document& operator=(const Document& other) {
        if (this != &other) {
            rapidjson::Document copy; copy.CopyFrom(other.value_, copy.GetAllocator()); value_.Swap(copy);
            loadedPath_ = other.loadedPath_; original_ = other.original_;
            tracked_ = other.tracked_; existed_ = other.existed_;
        }
        return *this;
    }
    Value& Root() { return value_; }
    const Value& Root() const { return value_; }
    AllocatorType& Allocator() { return value_.GetAllocator(); }
    bool Parse(const std::string& text, std::string& error) {
        rapidjson::Document candidate;
        if (!ParseValue(text, candidate, error)) return false;
        if (!candidate.IsObject()) { error = "Configuration root must be an object"; return false; }
        value_.Swap(candidate); loadedPath_.clear(); original_.clear(); tracked_ = existed_ = false;
        return true;
    }
    bool Load(const std::string& path, std::string& error, bool allowMissing = false) {
        error.clear();
        try {
            auto normalized = Detail::Normalize(path);
            Detail::Handle file(CreateFileW(normalized.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
            if (!file.Valid()) {
                DWORD code = GetLastError();
                if (allowMissing && (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND)) {
                    value_.SetObject(); loadedPath_ = normalized; original_.clear(); tracked_ = true; existed_ = false;
                    return true;
                }
                error = Detail::WinError("Cannot open configuration"); return false;
            }
            std::string text;
            if (!Detail::ReadHandle(file.Get(), text, error)) return false;
            rapidjson::Document candidate;
            if (!ParseValue(text, candidate, error)) return false;
            if (!candidate.IsObject()) { error = "Configuration root must be an object"; return false; }
            value_.Swap(candidate); loadedPath_ = normalized; original_ = std::move(text);
            tracked_ = existed_ = true; return true;
        } catch (const std::exception& exception) { error = exception.what(); return false; }
    }
    bool Serialize(std::string& text, std::string& error) const {
        error.clear();
        if (!value_.IsObject() || !Validate(value_, error)) { if (error.empty()) error = "Invalid configuration root"; return false; }
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>,
            rapidjson::CrtAllocator, rapidjson::kWriteValidateEncodingFlag> writer(buffer);
        writer.SetIndent(' ', 2);
        if (!value_.Accept(writer)) { error = "Invalid UTF-8 or value in configuration"; return false; }
        text.assign(buffer.GetString(), buffer.GetSize()); text += '\n';
        if (text.size() > MaxFileBytes) { error = "Serialized configuration too large"; return false; }
        return true;
    }
    // Existing files must have been loaded by this Document. Unknown keys therefore survive
    // edits, and a second writer cannot silently replace changes made after the load.
    bool SaveAtomic(const std::string& path, std::string& error, int migrationFromVersion = 0,
                    std::string* backupPath = nullptr) {
        error.clear(); if (backupPath) backupPath->clear();
        std::filesystem::path temporary;
        try {
            std::string text;
            if (!Serialize(text, error)) return false;
            auto target = Detail::Normalize(path);
            Detail::Lock lock(Detail::MutexName(target));
            if (!lock.Held()) { error = "Configuration save lock unavailable"; return false; }
            Detail::Handle source(CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
            bool exists = source.Valid(); DWORD openError = exists ? ERROR_SUCCESS : GetLastError();
            if (!exists && openError != ERROR_FILE_NOT_FOUND && openError != ERROR_PATH_NOT_FOUND) {
                error = Detail::WinError("Cannot lock current configuration"); return false;
            }
            if (exists) {
                std::string current;
                if (!Detail::ReadHandle(source.Get(), current, error)) return false;
                source.Close();
                if (!tracked_ || !existed_ || !Detail::SamePath(loadedPath_, target) || current != original_) {
                    error = "Configuration changed on disk; reload before saving"; return false;
                }
            } else if (tracked_ && existed_ && Detail::SamePath(loadedPath_, target)) {
                error = "Configuration was removed on disk; reload before saving"; return false;
            }
            if (migrationFromVersion > 0 && !exists) { error = "Cannot migrate a missing configuration"; return false; }
            if (!target.parent_path().empty()) std::filesystem::create_directories(target.parent_path());
            temporary = target; temporary += L"." + Detail::UniqueSuffix() + L".tmp";
            Detail::Handle output(CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr));
            if (!output.Valid()) { error = Detail::WinError("Cannot create temporary configuration"); return false; }
            DWORD written = 0;
            if (!WriteFile(output.Get(), text.data(), static_cast<DWORD>(text.size()), &written, nullptr) ||
                written != text.size() || !FlushFileBuffers(output.Get())) {
                error = Detail::WinError("Cannot flush temporary configuration"); output.Close();
                DeleteFileW(temporary.c_str()); return false;
            }
            output.Close();
            if (migrationFromVersion > 0) {
                auto backup = target;
                backup += L".v" + std::to_wstring(migrationFromVersion) + L"." + Detail::UniqueSuffix() + L".bak";
                if (!CopyFileW(target.c_str(), backup.c_str(), TRUE)) {
                    error = Detail::WinError("Cannot create migration backup"); DeleteFileW(temporary.c_str()); return false;
                }
                if (backupPath) *backupPath = backup.u8string();
            }
            BOOL committed = exists ? ReplaceFileW(target.c_str(), temporary.c_str(), nullptr, 0, nullptr, nullptr) :
                                      MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH);
            if (!committed) { error = Detail::WinError("Cannot publish configuration"); DeleteFileW(temporary.c_str()); return false; }
            temporary.clear(); loadedPath_ = target; original_ = std::move(text); tracked_ = existed_ = true;
            return true;
        } catch (const std::exception& exception) {
            error = exception.what(); if (!temporary.empty()) DeleteFileW(temporary.c_str()); return false;
        }
    }
};

} // namespace HazzureConfig
