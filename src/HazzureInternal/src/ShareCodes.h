// C++ / ShareCodes.h / MSVC x64 C++17 / bounded, versioned configuration transport
#pragma once
#include <windows.h>
#include <wincrypt.h>
#include <string>
#include <cstdint>
#include <cstdio>
#pragma comment(lib,"crypt32.lib")
namespace HazzureShare {
inline constexpr size_t MaxJson=1024*1024,MaxCode=1400000;
inline uint32_t Crc32(const std::string& bytes){uint32_t crc=0xffffffffu;for(unsigned char b:bytes){crc^=b;for(int i=0;i<8;i++)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}return ~crc;}
inline bool Encode(const std::string& json,std::string& code,std::string& error){
    if(json.empty()||json.size()>MaxJson){error="Perfil vazio ou grande demais";return false;}
    DWORD length{};constexpr DWORD flags=CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF;
    if(!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(json.data()),static_cast<DWORD>(json.size()),flags,nullptr,&length)){error="Falha ao codificar perfil";return false;}
    std::string value(length,'\0');
    if(!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(json.data()),static_cast<DWORD>(json.size()),flags,value.data(),&length)){error="Falha ao codificar perfil";return false;}
    value.resize(length);while(!value.empty()&&value.back()==0)value.pop_back();
    char header[24];sprintf_s(header,"HAZ3:%08X:",Crc32(json));code=header+value;return true;
}
inline bool Decode(const std::string& code,std::string& json,std::string& error){
    if(code.size()<18||code.size()>MaxCode||code.compare(0,5,"HAZ3:")||code[13]!=':'){error="Share code invalido ou versao incompativel";return false;}
    uint32_t expected=0;
    for(size_t i=5;i<13;i++){char c=code[i];unsigned v=c>='0'&&c<='9'?c-'0':(c>='A'&&c<='F'?c-'A'+10:99);if(v>15){error="Checksum invalido";return false;}expected=(expected<<4)|v;}
    size_t n=code.size()-14;if(n%4){error="Base64 incompleto";return false;}
    unsigned padding=0;
    for(size_t i=14;i<code.size();++i){unsigned char c=code[i];if(c=='='){if(++padding>2||i<code.size()-2){error="Padding Base64 invalido";return false;}}
        else if(padding||!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='+'||c=='/')){error="Caractere Base64 invalido";return false;}}
    DWORD bytes{};if(!CryptStringToBinaryA(code.data()+14,static_cast<DWORD>(n),CRYPT_STRING_BASE64,nullptr,&bytes,nullptr,nullptr)||!bytes||bytes>MaxJson){error="Perfil codificado invalido";return false;}
    std::string decoded(bytes,'\0');
    if(!CryptStringToBinaryA(code.data()+14,static_cast<DWORD>(n),CRYPT_STRING_BASE64,reinterpret_cast<BYTE*>(decoded.data()),&bytes,nullptr,nullptr)){error="Falha ao decodificar perfil";return false;}
    decoded.resize(bytes);if(Crc32(decoded)!=expected){error="Share code corrompido";return false;}
    json=std::move(decoded);return true;
}
} // namespace HazzureShare
