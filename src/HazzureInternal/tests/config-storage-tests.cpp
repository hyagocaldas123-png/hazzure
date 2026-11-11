// C++ / config-storage-tests.cpp / MSVC x64 C++17 / isolated JSON persistence regressions
#include "../src/ConfigStorage.h"
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace HazzureConfig;
static int checks = 0, failures = 0;
#define CHECK(condition, text) do { ++checks; if (!(condition)) { ++failures; printf("FAIL: %s\n", text); } } while (0)
static std::string ReadBytes(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary); std::ostringstream contents; contents << stream.rdbuf(); return contents.str();
}
static void WriteBytes(const std::filesystem::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary); stream << text;
}

int main() {
    wchar_t directory[MAX_PATH]{}; GetTempPathW(MAX_PATH, directory);
    auto folder = std::filesystem::path(directory) / (L"hazzure-config-tests-" + Detail::UniqueSuffix());
    std::filesystem::create_directories(folder);
    std::string error;
    Document document;
    CHECK(document.Parse(R"({"schemaVersion":2,"enabled":1,"scale":4.5,"name":"quotes \" and \\ paths","colour":[0,0.25,0.5,1],"unknown":{"keep":[1,true,null,{"x":"y"}]}})", error), "parse nested document");
    bool enabled = false; int version = 0; float scale = 0; std::string name;
    CHECK(GetBool(document.Root(), "enabled", enabled, true) && enabled, "legacy numeric bool accepted explicitly");
    CHECK(!GetBool(document.Root(), "enabled", enabled), "strict bool rejects numeric flag");
    CHECK(GetInt(document.Root(), "schemaVersion", version, 1, 3) && version == 2, "integer in range");
    CHECK(!GetInt(document.Root(), "schemaVersion", version, 3, 5), "integer outside range rejected");
    CHECK(GetFloat(document.Root(), "scale", scale, 1, 25) && scale == 4.5f, "float in range");
    CHECK(!GetFloat(document.Root(), "name", scale), "number rejects string type");
    CHECK(GetString(document.Root(), "name", name) && name == "quotes \" and \\ paths", "string escapes decode");
    float colour[4]{};
    CHECK(GetFloatArray(document.Root(), "colour", colour, 4) && colour[3] == 1, "RGBA array validates");
    CHECK(!GetFloatArray(document.Root(), "colour", colour, 3), "RGBA wrong length rejected");
    std::string serialized;
    CHECK(document.Serialize(serialized, error), "serialize valid object");
    const auto before = serialized;
    for (const auto& bad : {"{\"x\":1,\"x\":2}", "{\"x\":NaN}", "{\"x\":1}junk", "[]", "{\"x\":01}", "{\"x\":true,}", "{\"x\":1e400}", "{\"x\":{\"same\":1,\"same\":2}}"}) {
        CHECK(!document.Parse(bad, error), "invalid JSON rejected transactionally");
        document.Serialize(serialized, error); CHECK(serialized == before, "failed parse preserves prior document");
    }
    std::string nested = "{}";
    for (int i = 0; i < 70; ++i) nested = "{\"x\":" + nested + "}";
    CHECK(!document.Parse(nested, error), "excessive nesting rejected");
    CHECK(!document.Parse(std::string("{\"x\":1}\0junk", 12), error), "embedded NUL rejected");
    CHECK(!document.Parse(std::string("{\"x\":\"") + static_cast<char>(-1) + "\"}", error), "invalid UTF-8 rejected");
    CHECK(document.Parse("\xef\xbb\xbf{}", error), "UTF-8 BOM accepted");

    auto path = folder / L"config with spaces.json";
    const std::string original = R"({"schemaVersion":2,"unknown":{"keep":[1,true,null,{"x":"y"}]},"userValue":17})";
    WriteBytes(path, original);
    CHECK(document.Load(path.u8string(), error), "load existing configuration");
    Set(document.Root(), "schemaVersion", 3, document.Allocator());
    Set(document.Root(), "name", "slot \"quoted\" \\ path", document.Allocator());
    Set(document.Root(), "enabled", true, document.Allocator());
    CHECK(SetRaw(document.Root(), "contexts", R"({"enemy":{"colour":[1,0,0,1]},"local":{"enabled":false}})", document.Allocator(), error), "set nested JSON");
    std::string backup;
    CHECK(document.SaveAtomic(path.u8string(), error, 2, &backup), "migration publishes atomically");
    CHECK(!backup.empty() && ReadBytes(std::filesystem::u8path(backup)) == original, "migration backup equals original bytes");
    Document reloaded;
    CHECK(reloaded.Load(path.u8string(), error), "reload migrated JSON");
    CHECK(Find(reloaded.Root(), "unknown") && Find(reloaded.Root(), "unknown")->IsObject(), "unknown object preserved");
    int user = 0; CHECK(GetInt(reloaded.Root(), "userValue", user) && user == 17, "unknown scalar preserved");
    CHECK(GetBool(reloaded.Root(), "enabled", enabled) && enabled, "true bool round trips");
    CHECK(GetString(reloaded.Root(), "name", name) && name == "slot \"quoted\" \\ path", "escaped strings round trip");

    Document stale;
    CHECK(stale.Load(path.u8string(), error), "second editor loads");
    Set(reloaded.Root(), "userValue", 18, reloaded.Allocator());
    CHECK(reloaded.SaveAtomic(path.u8string(), error), "first editor saves");
    auto newer = ReadBytes(path);
    CHECK(!stale.SaveAtomic(path.u8string(), error) && ReadBytes(path) == newer, "stale editor cannot overwrite");
    Document fresh; CHECK(!fresh.SaveAtomic(path.u8string(), error), "untracked document cannot replace existing file");

    auto absent = folder / L"new" / L"config.json";
    CHECK(fresh.Load(absent.u8string(), error, true), "missing file loads defaults without creation");
    CHECK(!std::filesystem::exists(absent), "allowMissing is read-only");
    Set(fresh.Root(), "value", 42, fresh.Allocator());
    CHECK(fresh.SaveAtomic(absent.u8string(), error), "new file created atomically");
    Document copy = fresh;
    Set(copy.Root(), "value", 43, copy.Allocator());
    CHECK(GetInt(fresh.Root(), "value", user) && user == 42, "document copy owns independent DOM");

    auto heldPath = folder / L"held.json";
    WriteBytes(heldPath, "{\"value\":1}");
    Document held; CHECK(held.Load(heldPath.u8string(), error), "load locked-file scenario");
    Set(held.Root(), "value", 2, held.Allocator());
    {
        Detail::Handle lock(CreateFileW(heldPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
        CHECK(lock.Valid(), "hold existing file against replacement");
        CHECK(!held.SaveAtomic(heldPath.u8string(), error), "locked target causes save failure");
        CHECK(ReadBytes(heldPath) == "{\"value\":1}", "publication failure preserves original bytes");
    }
    CHECK(held.SaveAtomic(heldPath.u8string(), error), "save recovers after lock released");
    std::string failedMigrationBackup;
    auto preMigration = ReadBytes(heldPath);
    {
        Detail::Handle lock(CreateFileW(heldPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
        Set(held.Root(), "value", 3, held.Allocator());
        CHECK(!held.SaveAtomic(heldPath.u8string(), error, 2, &failedMigrationBackup), "locked target rejects migration publication");
        CHECK(!failedMigrationBackup.empty() && ReadBytes(std::filesystem::u8path(failedMigrationBackup)) == preMigration,
              "failed migration retains a byte-exact pre-migration backup");
        CHECK(ReadBytes(heldPath) == preMigration, "failed migration leaves current configuration intact");
    }
    SetValue(held.Root(), "bad", Value(std::numeric_limits<double>::infinity()), held.Allocator());
    auto safe = ReadBytes(heldPath);
    CHECK(!held.SaveAtomic(heldPath.u8string(), error) && ReadBytes(heldPath) == safe, "non-finite number rejected before touching file");
    auto removedPath = folder / L"removed.json";
    WriteBytes(removedPath, "{}");
    Document removed; CHECK(removed.Load(removedPath.u8string(), error), "load before external deletion");
    std::filesystem::remove(removedPath);
    CHECK(!removed.SaveAtomic(removedPath.u8string(), error) && !std::filesystem::exists(removedPath),
          "external deletion requires reload and cannot be silently undone");
    size_t temporaries = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder))
        if (entry.path().extension() == ".tmp") ++temporaries;
    CHECK(temporaries == 0, "no temporary files remain after failures");

    // Remove only the explicitly created files/directories, without recursive deletion.
    std::filesystem::remove(absent);
    std::filesystem::remove(absent.parent_path());
    for (const auto& entry : std::filesystem::directory_iterator(folder))
        if (entry.is_regular_file()) std::filesystem::remove(entry.path());
    std::filesystem::remove(folder);
    printf("CONFIG STORAGE: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
