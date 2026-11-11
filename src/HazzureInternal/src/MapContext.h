#pragma once
#include "Visibility.h"
namespace HazzureMap {
inline SRWLOCK lock=SRWLOCK_INIT;inline char current[64]{};inline uint64_t epoch{};
inline bool Normalize(const char* source,char (&out)[64]){
    if(!source)return false;size_t n=strnlen_s(source,256);if(!n||n>=256)return false;
    const char* leaf=source;for(size_t i=0;i<n;++i)if(source[i]=='/'||source[i]=='\\')leaf=source+i+1;
    n=strlen(leaf);if(n>=5&&!strcmp(leaf+n-5,".vmap"))n-=5;
    if(!n||n>=64)return false;
    for(size_t i=0;i<n;++i){unsigned char c=static_cast<unsigned char>(leaf[i]);
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;out[i]=static_cast<char>(c);}
    out[n]=0;return true;
}
inline bool Publish(const char* name){char normalized[64]{};if(!Normalize(name,normalized))return false;
    AcquireSRWLockExclusive(&lock);if(strcmp(current,normalized)){strcpy_s(current,normalized);++epoch;}ReleaseSRWLockExclusive(&lock);return true;}
inline bool Read(char (&out)[64]){AcquireSRWLockShared(&lock);strcpy_s(out,current);ReleaseSRWLockShared(&lock);return *out!=0;}
inline void Clear(){AcquireSRWLockExclusive(&lock);current[0]=0;++epoch;ReleaseSRWLockExclusive(&lock);}
inline bool ObserveEngine(){
    auto engine=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"engine2.dll"));if(!engine)return false;
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    constexpr unsigned char code[]={0x48,0x8b,0x89,0x18,0x02,0,0};
    if(!HazzureGame::Read(engine,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<=0||dos.e_lfanew>0x10000||
       !HazzureGame::Read(engine+dos.e_lfanew,nt)||nt.Signature!=IMAGE_NT_SIGNATURE||
       nt.FileHeader.TimeDateStamp!=0x6ada3418||nt.OptionalHeader.SizeOfImage!=0xa15000||
       !HazzureVisibility::Prefix(engine+0x764de,code,sizeof(code)))return false;
    uintptr_t network{},name{};int state{};
    if(!HazzureGame::Read(engine+0x9b88c0,network)||!network||!HazzureGame::Read(network+0x230,state)||state<2||
       !HazzureGame::Read(network+0x218,name)||!name)return false;
    char value[64]{};for(size_t i=0;i<sizeof(value);++i){if(!HazzureGame::Read(name+i,value[i]))return false;
        if(!value[i])return Publish(value);}
    return false;
}
}
