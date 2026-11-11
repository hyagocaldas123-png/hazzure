// Bounded RIFF/WAVE PCM decoding. Every successful decode produces one canonical PCM16 WAV.
#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
namespace HazzureCustomAudio {
inline constexpr size_t MaximumFileBytes=8*1024*1024;
inline constexpr unsigned MaximumSeconds=5;
enum class Kind {Hit,Headshot,Kill,None};
inline Kind SelectEvent(bool headshot,bool hit,bool kill,bool hitEnabled,bool killEnabled){
    if(kill&&killEnabled)return Kind::Kill;
    if(headshot&&hitEnabled)return Kind::Headshot;
    return hit&&hitEnabled?Kind::Hit:Kind::None;
}
enum class Error {None,Disabled,EmptyPath,PathTooLong,OpenFailed,FileSize,Truncated,Container,Format,Channels,Rate,Bits,Alignment,EmptyData,Duration,Chunks};
inline const char* Label(Error error){switch(error){
    case Error::None:return "WAV personalizado pronto";
    case Error::Disabled:return "Som personalizado desativado";
    case Error::EmptyPath:return "Informe um arquivo WAV local";
    case Error::PathTooLong:return "Caminho muito longo";
    case Error::OpenFailed:return "Nao foi possivel abrir o arquivo";
    case Error::FileSize:return "Arquivo vazio ou acima de 8 MB";
    case Error::Truncated:return "Arquivo WAV incompleto";
    case Error::Container:return "Use um arquivo RIFF/WAVE";
    case Error::Format:return "Use WAV PCM sem compressao";
    case Error::Channels:return "Use audio mono ou stereo";
    case Error::Rate:return "Taxa de amostragem nao suportada";
    case Error::Bits:return "Use PCM de 8 ou 16 bits";
    case Error::Alignment:return "Dados PCM ou cabecalho inconsistentes";
    case Error::EmptyData:return "O arquivo nao contem amostras";
    case Error::Duration:return "O som deve ter no maximo 5 segundos";
    default:return "Estrutura de chunks WAV invalida";
}}
inline uint16_t U16(const unsigned char* p){return static_cast<uint16_t>(p[0]|(static_cast<unsigned>(p[1])<<8));}
inline uint32_t U32(const unsigned char* p){return static_cast<uint32_t>(p[0])|(static_cast<uint32_t>(p[1])<<8)|(static_cast<uint32_t>(p[2])<<16)|(static_cast<uint32_t>(p[3])<<24);}
inline void Put16(char* p,uint16_t v){p[0]=static_cast<char>(v);p[1]=static_cast<char>(v>>8);}
inline void Put32(char* p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=static_cast<char>(v>>(i*8));}
inline bool Rate(unsigned rate){for(unsigned value:{8000u,11025u,16000u,22050u,32000u,44100u,48000u,88200u,96000u})if(rate==value)return true;return false;}
struct Sound {std::vector<char> wav;unsigned rate{},channels{},frames{};bool Valid()const{return wav.size()>44&&rate&&channels&&frames;}};
inline bool Decode(const unsigned char* bytes,size_t size,Sound& output,Error& error){
    auto fail=[&](Error e){error=e;return false;};
    if(!bytes||size<12)return fail(Error::Truncated);
    if(size>MaximumFileBytes)return fail(Error::FileSize);
    if(memcmp(bytes,"RIFF",4)||memcmp(bytes+8,"WAVE",4))return fail(Error::Container);
    uint32_t declared=U32(bytes+4);if(declared<4||static_cast<uint64_t>(declared)+8!=size)return fail(Error::Truncated);
    const unsigned char* format=nullptr;const unsigned char* pcm=nullptr;size_t formatSize{},pcmSize{};
    for(size_t at=12;at<size;){
        if(size-at<8)return fail(Error::Chunks);
        uint32_t length=U32(bytes+at+4);size_t payload=at+8;
        if(length>size-payload)return fail(Error::Truncated);
        if(!memcmp(bytes+at,"fmt ",4)){if(format)return fail(Error::Chunks);format=bytes+payload;formatSize=length;}
        if(!memcmp(bytes+at,"data",4)){if(pcm)return fail(Error::Chunks);pcm=bytes+payload;pcmSize=length;}
        size_t next=payload+length;if(length&1){if(next==size)return fail(Error::Truncated);++next;}at=next;
    }
    if(!format||formatSize<16||formatSize==17)return fail(Error::Format);
    if(U16(format)!=1)return fail(Error::Format);
    unsigned channels=U16(format+2),rate=U32(format+4),byteRate=U32(format+8),align=U16(format+12),bits=U16(format+14);
    if(channels!=1&&channels!=2)return fail(Error::Channels);
    if(!Rate(rate))return fail(Error::Rate);
    if(bits!=8&&bits!=16)return fail(Error::Bits);
    if(align!=channels*(bits/8)||byteRate!=rate*align)return fail(Error::Alignment);
    if(!pcm||!pcmSize)return fail(Error::EmptyData);
    if(pcmSize%align)return fail(Error::Alignment);
    size_t frames=pcmSize/align;if(frames>static_cast<size_t>(rate)*MaximumSeconds)return fail(Error::Duration);
    Sound candidate;candidate.rate=rate;candidate.channels=channels;candidate.frames=static_cast<unsigned>(frames);
    size_t sampleCount=frames*channels;uint32_t normalized=static_cast<uint32_t>(sampleCount*2);
    candidate.wav.resize(44+normalized);char* wav=candidate.wav.data();
    memcpy(wav,"RIFF",4);Put32(wav+4,36+normalized);memcpy(wav+8,"WAVEfmt ",8);Put32(wav+16,16);
    Put16(wav+20,1);Put16(wav+22,static_cast<uint16_t>(channels));Put32(wav+24,rate);Put32(wav+28,rate*channels*2);
    Put16(wav+32,static_cast<uint16_t>(channels*2));Put16(wav+34,16);memcpy(wav+36,"data",4);Put32(wav+40,normalized);
    for(size_t i=0;i<sampleCount;++i){int sample=bits==8?(static_cast<int>(pcm[i])-128)*256:static_cast<int16_t>(U16(pcm+i*2));Put16(wav+44+i*2,static_cast<uint16_t>(sample));}
    output=std::move(candidate);error=Error::None;return true;
}
inline bool Decode(const std::vector<char>& bytes,Sound& out,Error& error){return Decode(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),out,error);}
inline bool Load(const std::string& path,Sound& output,Error& error){
    if(path.empty()){error=Error::EmptyPath;return false;}if(path.size()>=512||path.find('\0')!=std::string::npos){error=Error::PathTooLong;return false;}
    try{
        std::ifstream stream(std::filesystem::u8path(path),std::ios::binary|std::ios::ate);if(!stream){error=Error::OpenFailed;return false;}
        auto length=stream.tellg();if(length<12||length>static_cast<std::streamoff>(MaximumFileBytes)){error=Error::FileSize;return false;}
        std::vector<char> bytes(static_cast<size_t>(length));stream.seekg(0);
        if(!stream.read(bytes.data(),static_cast<std::streamsize>(bytes.size()))){error=Error::Truncated;return false;}
        return Decode(bytes,output,error);
    }catch(...){error=Error::OpenFailed;return false;}
}
struct Cached {std::string path;Sound sound;Error error=Error::Disabled;bool enabled{},initialized{};};
class Cache {
    std::array<Cached,3> entries{};
public:
    using Loader=bool(*)(const std::string&,Sound&,Error&);
    bool Prepare(Kind kind,bool enabled,const std::string& path,bool force=false,Loader loader=Load){
        auto index=static_cast<size_t>(kind);if(index>=entries.size())return false;auto& entry=entries[index];
        if(!force&&entry.initialized&&entry.path==path&&entry.enabled==enabled)return entry.sound.Valid();
        Cached candidate;candidate.path=path;candidate.enabled=enabled;candidate.initialized=true;
        if(enabled)loader(path,candidate.sound,candidate.error);
        entry=std::move(candidate);return entry.sound.Valid();
    }
    const Cached& Get(Kind kind)const{static const Cached invalid{};auto i=static_cast<size_t>(kind);return i<entries.size()?entries[i]:invalid;}
    void Clear(){for(auto& entry:entries)entry={};}
};
inline bool Canonical(const std::vector<char>& wav){
    if(wav.size()<=44||wav.size()>MaximumFileBytes)return false;auto p=reinterpret_cast<const unsigned char*>(wav.data());
    return !memcmp(p,"RIFF",4)&&!memcmp(p+8,"WAVEfmt ",8)&&U32(p+4)==wav.size()-8&&U32(p+16)==16&&U16(p+20)==1&&
        (U16(p+22)==1||U16(p+22)==2)&&Rate(U32(p+24))&&U16(p+34)==16&&U16(p+32)==U16(p+22)*2&&
        U32(p+28)==U32(p+24)*U16(p+32)&&!memcmp(p+36,"data",4)&&U32(p+40)==wav.size()-44&&
        U32(p+40)%U16(p+32)==0&&U32(p+40)/U16(p+32)<=U32(p+24)*MaximumSeconds;
}
inline bool Gain(const std::vector<char>& source,float volume,std::vector<char>& output){
    if(!Canonical(source))return false;
    volume=std::isfinite(volume)?std::clamp(volume,0.f,1.f):0.f;
    output=source;for(size_t offset=44;offset+2<=output.size();offset+=2){int16_t sample=static_cast<int16_t>(U16(reinterpret_cast<const unsigned char*>(source.data()+offset)));
        sample=static_cast<int16_t>(std::lround(sample*volume));Put16(output.data()+offset,static_cast<uint16_t>(sample));}return true;
}
}
