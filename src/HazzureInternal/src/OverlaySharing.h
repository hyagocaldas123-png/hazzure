// C++ / OverlaySharing.h / MSVC x64 C++17 / include after OverlayPersistence inside HazzureUI
#pragma once
static bool ExportShareCode(std::string& code){
    if(!CaptureSharedConfig(g_settingsShared)){Persistence::Status("IPC ocupado; exportacao cancelada");return false;}
    HazzureConfig::Document candidate=Persistence::storage;std::string json,error;
    if(!Persistence::WriteConfig(candidate,g_cfg,g_contexts,g_weaponClasses,g_profiles,g_activeProfileIndex,error)||
       !candidate.Serialize(json,error)||!HazzureShare::Encode(json,code,error)){Persistence::Status(error);return false;}
    return true;
}
static bool ImportShareCode(const std::string& code){
    std::string json,error;HazzureConfig::Document imported;
    if(!HazzureShare::Decode(code,json,error)||!imported.Parse(json,error)){Persistence::Status(error);return false;}
    OverlayConfig config{};Persistence::ContextMap contexts,classes;std::vector<ConfigProfile> profiles;int active{};
    int version=1;
    if(HazzureConfig::Find(imported.Root(),"schemaVersion")&&
       !HazzureConfig::GetInt(imported.Root(),"schemaVersion",version,1,3)){Persistence::Status("Versao de perfil incompativel");return false;}
    if(!Persistence::ReadConfig(imported.Root(),config,error)||
       !Persistence::ReadProfiles(imported.Root(),contexts,classes,profiles,active,error)){Persistence::Status(error);return false;}
    std::string path=Persistence::storagePath.empty()?"C:\\HazzureKernel\\hazzure_config.json":Persistence::storagePath;
    HazzureConfig::Document stored;
    if(Persistence::storagePath==path)stored=Persistence::storage;
    else if(!stored.Load(path,error,true)){Persistence::Status(error);return false;}
    stored.Root().CopyFrom(imported.Root(),stored.Allocator());
    if(!Persistence::Publish(g_settingsShared,config)){Persistence::Status("IPC ocupado; importacao cancelada");return false;}
    g_cfg=config;g_contexts.swap(contexts);g_weaponClasses.swap(classes);g_profiles.swap(profiles);g_activeProfileIndex=active;
    RefreshCustomAudio();
    Persistence::storage=stored;Persistence::storagePath=path;
    Persistence::Status("Perfil importado; use Salvar Config para persistir");
    return true;
}

static bool ExportConfigByHash(std::string& outHash) {
    std::string code;
    if (!ExportShareCode(code) || code.empty()) return false;
    uint64_t h1 = 14695981039346656037ull, h2 = 1099511628211ull;
    for (unsigned char c : code) {
        h1 = (h1 ^ c) * 1099511628211ull;
        h2 = (h2 + (c * 1315423911ull)) ^ (h1 >> 17);
    }
    char hashBuf[40]{};
    sprintf_s(hashBuf, "HZ-%08llX%08llX",
        static_cast<unsigned long long>(h1 & 0xFFFFFFFFull),
        static_cast<unsigned long long>(h2 & 0xFFFFFFFFull));
    CreateDirectoryA("C:\\HazzureKernel\\cloud_hashes", nullptr);
    char filePath[260]{};
    sprintf_s(filePath, "C:\\HazzureKernel\\cloud_hashes\\%s.hzc", hashBuf);
    FILE* f = nullptr;
    if (fopen_s(&f, filePath, "wb") == 0 && f) {
        fwrite(code.data(), 1, code.size(), f);
        fclose(f);
    }
    outHash = hashBuf;
    Persistence::Status(std::string("Hash gerado e copiado: ") + hashBuf);
    return true;
}

static bool ImportConfigByHash(const char* rawHash) {
    if (!rawHash || !*rawHash) { Persistence::Status("Hash vazio na area de transferencia"); return false; }
    std::string cleaned;
    for (const char* p = rawHash; *p && cleaned.size() < 32; ++p) {
        char c = *p;
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || c == '-' || c == 'H' || c == 'Z' || c == 'h' || c == 'z') {
            cleaned.push_back(static_cast<char>(toupper(static_cast<unsigned char>(c))));
        }
    }
    if (cleaned.rfind("HZ-", 0) != 0 && cleaned.size() == 16) cleaned = "HZ-" + cleaned;
    if (cleaned.size() != 19 || cleaned.rfind("HZ-", 0) != 0) {
        Persistence::Status("Formato de hash invalido (esperado HZ-XXXXXXXXXXXXXXXX)");
        return false;
    }
    char filePath[260]{};
    sprintf_s(filePath, "C:\\HazzureKernel\\cloud_hashes\\%s.hzc", cleaned.c_str());
    FILE* f = nullptr;
    if (fopen_s(&f, filePath, "rb") != 0 || !f) {
        Persistence::Status("Hash nao encontrado no cache local: " + cleaned);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > static_cast<long>(HazzureShare::MaxCode)) { fclose(f); Persistence::Status("Arquivo de hash invalido"); return false; }
    std::string code(static_cast<size_t>(sz), '\0');
    fread(code.data(), 1, static_cast<size_t>(sz), f);
    fclose(f);
    if (ImportShareCode(code)) {
        Persistence::Status("Config importada via Hash: " + cleaned);
        return true;
    }
    return false;
// end OverlaySharing.h
}
