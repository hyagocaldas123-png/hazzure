#include "../src/CustomAudio.h"
#include <cstdio>
#include <limits>
using namespace HazzureCustomAudio;
static unsigned passed{},failed{},loads{};
#define CHECK(value,label) do{if(value){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
static void Chunk(std::vector<char>& bytes,const char* tag,const std::vector<char>& data){
    size_t at=bytes.size();bytes.resize(at+8+data.size()+(data.size()&1));memcpy(bytes.data()+at,tag,4);Put32(bytes.data()+at+4,static_cast<uint32_t>(data.size()));memcpy(bytes.data()+at+8,data.data(),data.size());
}
static std::vector<char> Format(unsigned channels=1,unsigned bits=16,unsigned rate=8000){
    std::vector<char> f(16);Put16(f.data(),1);Put16(f.data()+2,static_cast<uint16_t>(channels));Put32(f.data()+4,rate);
    Put32(f.data()+8,rate*channels*bits/8);Put16(f.data()+12,static_cast<uint16_t>(channels*bits/8));Put16(f.data()+14,static_cast<uint16_t>(bits));return f;
}
static std::vector<char> Wav(std::vector<char> pcm={0,0,1,0},std::vector<char> format=Format(),bool metadata=false,bool dataFirst=false){
    std::vector<char> bytes(12);memcpy(bytes.data(),"RIFF",4);memcpy(bytes.data()+8,"WAVE",4);
    if(metadata)Chunk(bytes,"JUNK",{'a','b','c'});
    if(dataFirst)Chunk(bytes,"data",pcm);Chunk(bytes,"fmt ",format);if(!dataFirst)Chunk(bytes,"data",pcm);
    Put32(bytes.data()+4,static_cast<uint32_t>(bytes.size()-8));return bytes;
}
static bool MockLoad(const std::string& path,Sound& sound,Error& error){++loads;if(path=="bad"){error=Error::OpenFailed;return false;}return Decode(Wav(),sound,error);}
int main(){
    Error error{};Sound sound;auto plain=Wav();
    CHECK(Decode(plain,sound,error)&&sound.Valid()&&Canonical(sound.wav)&&sound.frames==2&&sound.rate==8000,"ordinary PCM16 is normalized to a complete canonical WAV");
    auto prior=sound.wav;auto invalid=plain;invalid[0]='X';
    CHECK(!Decode(invalid,sound,error)&&error==Error::Container&&sound.wav==prior,"invalid containers cannot overwrite an accepted sound");
    CHECK(!Decode(nullptr,0,sound,error)&&error==Error::Truncated,"null and empty inputs are rejected");
    CHECK(Decode(Wav({0,static_cast<char>(128),static_cast<char>(255)},Format(1,8)),sound,error)&&
        static_cast<int16_t>(U16(reinterpret_cast<const unsigned char*>(sound.wav.data()+44)))==-32768&&
        U16(reinterpret_cast<const unsigned char*>(sound.wav.data()+46))==0&&
        U16(reinterpret_cast<const unsigned char*>(sound.wav.data()+48))==32512,"unsigned PCM8 converts silence and both extremes into signed PCM16");
    std::vector<char> stereo(8);Put16(stereo.data(),0x8000);Put16(stereo.data()+2,0xffff);Put16(stereo.data()+4,1);Put16(stereo.data()+6,0x7fff);
    CHECK(Decode(Wav(stereo,Format(2,16,48000)),sound,error)&&sound.channels==2&&sound.frames==2&&!memcmp(sound.wav.data()+44,stereo.data(),8),"stereo PCM16 preserves channel ordering and signed extrema");
    CHECK(Decode(Wav({0,0,1,0},Format(),true),sound,error)&&Canonical(sound.wav),"unknown odd-length RIFF chunks are skipped with their padding");
    CHECK(Decode(Wav({0,0,1,0},Format(),false,true),sound,error),"data before fmt is supported without assuming a 44-byte source header");
    auto format=Format();Put16(format.data(),3);
    CHECK(!Decode(Wav({0,0},format),sound,error)&&error==Error::Format,"compressed and IEEE float formats are rejected");
    CHECK(!Decode(Wav({0,0,0,0},Format(3)),sound,error)&&error==Error::Channels,"more than two channels are rejected");
    CHECK(!Decode(Wav({0,0,0},Format(1,24)),sound,error)&&error==Error::Bits,"unsupported PCM24 is rejected explicitly");
    CHECK(!Decode(Wav({0,0},Format(1,16,12345)),sound,error)&&error==Error::Rate,"unsupported sample rates are rejected");
    CHECK(Decode(Wav({0,0},Format(1,16,96000)),sound,error)&&sound.rate==96000,"the upper supported PCM sample rate is accepted");
    format=Format();Put32(format.data()+8,1);
    CHECK(!Decode(Wav({0,0},format),sound,error)&&error==Error::Alignment,"byte-rate mismatch is rejected");
    format=Format();Put16(format.data()+12,1);
    CHECK(!Decode(Wav({0,0},format),sound,error)&&error==Error::Alignment,"block alignment mismatch is rejected");
    CHECK(!Decode(Wav({0},Format()),sound,error)&&error==Error::Alignment,"partial PCM frames cannot be played");
    CHECK(!Decode(Wav({}),sound,error)&&error==Error::EmptyData,"empty data chunks are rejected");
    CHECK(Decode(Wav(std::vector<char>(40000,128),Format(1,8)),sound,error)&&sound.frames==40000,"exactly five seconds is accepted");
    CHECK(!Decode(Wav(std::vector<char>(40001,128),Format(1,8)),sound,error)&&error==Error::Duration,"sounds exceeding five seconds are rejected before output allocation");
    auto huge=std::vector<char>(MaximumFileBytes+1);
    CHECK(!Decode(huge,sound,error)&&error==Error::FileSize,"the 8 MB input limit is enforced");
    invalid=plain;Put32(invalid.data()+4,0xffffffff);
    CHECK(!Decode(invalid,sound,error)&&error==Error::Truncated,"oversized RIFF length is rejected without integer overflow");
    invalid=plain;Put32(invalid.data()+40,0xffffffff);
    CHECK(!Decode(invalid,sound,error)&&error==Error::Truncated,"oversized chunk length is rejected without pointer overflow");
    invalid=Wav({1,2,3},Format(1,8));invalid.pop_back();Put32(invalid.data()+4,static_cast<uint32_t>(invalid.size()-8));
    CHECK(!Decode(invalid,sound,error)&&error==Error::Truncated,"missing odd-chunk padding is rejected");
    invalid=plain;Chunk(invalid,"data",{0,0});Put32(invalid.data()+4,static_cast<uint32_t>(invalid.size()-8));
    CHECK(!Decode(invalid,sound,error)&&error==Error::Chunks,"duplicate data chunks are rejected");
    invalid=plain;Chunk(invalid,"fmt ",Format());Put32(invalid.data()+4,static_cast<uint32_t>(invalid.size()-8));
    CHECK(!Decode(invalid,sound,error)&&error==Error::Chunks,"duplicate format chunks are rejected");
    invalid=plain;invalid.push_back(0);
    CHECK(!Decode(invalid,sound,error)&&error==Error::Truncated,"bytes outside the declared RIFF container are rejected");
    std::vector<char> gain;Decode(Wav(stereo,Format(2,16,48000)),sound,error);
    CHECK(Gain(sound.wav,.5f,gain)&&static_cast<int16_t>(U16(reinterpret_cast<const unsigned char*>(gain.data()+44)))==-16384&&
        U16(reinterpret_cast<const unsigned char*>(gain.data()+50))==16384,"gain scales signed PCM samples without changing the format");
    CHECK(Gain(sound.wav,2,gain)&&gain==sound.wav,"gain above unity clamps without clipping");
    CHECK(Gain(sound.wav,std::numeric_limits<float>::quiet_NaN(),gain)&&U16(reinterpret_cast<const unsigned char*>(gain.data()+44))==0,"nonfinite gain becomes silence");
    CHECK(!Gain(invalid,.5f,gain),"playback gain refuses noncanonical or malformed buffers");
    CHECK(SelectEvent(true,true,true,true,true)==Kind::Kill,"kill audio wins when hit and headshot arrive in the same frame");
    CHECK(SelectEvent(true,true,true,true,false)==Kind::Headshot,"disabling kill audio still permits the headshot mapping");
    CHECK(SelectEvent(false,true,false,true,true)==Kind::Hit&&SelectEvent(true,true,false,false,true)==Kind::None,"ordinary hit mapping respects the existing hit-sound master toggle");
    Cache cache;loads=0;
    CHECK(cache.Prepare(Kind::Hit,true,"hit",false,MockLoad)&&loads==1,"custom selection is decoded at preparation time");
    CHECK(cache.Prepare(Kind::Hit,true,"hit",false,MockLoad)&&loads==1,"repeated frames do not reopen or decode an unchanged file");
    CHECK(cache.Prepare(Kind::Headshot,true,"head",false,MockLoad)&&cache.Prepare(Kind::Kill,true,"kill",false,MockLoad)&&loads==3,"hit headshot and kill retain independent cached mappings");
    CHECK(cache.Prepare(Kind::Hit,true,"hit",true,MockLoad)&&loads==4,"explicit test or reload refreshes a file with the same path");
    CHECK(!cache.Prepare(Kind::Hit,true,"bad",false,MockLoad)&&cache.Get(Kind::Hit).error==Error::OpenFailed&&!cache.Get(Kind::Hit).sound.Valid(),"a failed replacement does not play the stale previous custom sound");
    cache.Prepare(Kind::Hit,true,"bad",false,MockLoad);
    CHECK(loads==5,"invalid paths are cached instead of retried for every hit or frame");
    CHECK(!cache.Prepare(Kind::Kill,false,"kill",false,MockLoad)&&cache.Get(Kind::Kill).error==Error::Disabled&&!cache.Get(Kind::Kill).sound.Valid()&&loads==5,"disabling a mapping releases its buffer without file IO");
    CHECK(!cache.Prepare(Kind::None,true,"invalid",false,MockLoad)&&loads==5,"non-event kinds cannot index outside the cache");
    CHECK(!Load("",sound,error)&&error==Error::EmptyPath&&!Load(std::string(512,'x'),sound,error)&&error==Error::PathTooLong,"empty and unterminated-capacity paths are rejected before opening");
    uint32_t seed=1;bool bounded=true;
    for(unsigned i=0;i<5000;++i){auto bytes=plain;seed=seed*1664525+1013904223;size_t at=seed%bytes.size();bytes[at]=static_cast<char>(seed>>24);Sound decoded;Error failure{};
        if(Decode(bytes,decoded,failure)&&(!Canonical(decoded.wav)||decoded.wav.size()>MaximumFileBytes))bounded=false;}
    CHECK(bounded,"mutated headers and samples either fail or produce bounded canonical audio");
    cache.Clear();CHECK(!cache.Get(Kind::Headshot).sound.Valid(),"cache reset releases all decoded sounds");
    printf("%u passed, %u failed\n",passed,failed);return failed?1:0;
}
