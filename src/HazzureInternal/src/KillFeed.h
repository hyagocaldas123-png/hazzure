// Text and identities are captured at the event, never reconstructed from a dead pawn.
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <atomic>
namespace HazzureKillFeed {
struct Entry {uint64_t time{},epoch{};uint32_t attacker{},victim{},assistant{};bool local{},headshot{};
    char attackerName[96]{},victimName[96]{},assistantName[96]{},weapon[48]{};};
struct Settings {bool enabled{},localOnly{},roundOnly=true;int lines=6;float duration=15;};
inline bool Valid(const Settings& s){return s.lines>=1&&s.lines<=12&&std::isfinite(s.duration)&&s.duration>=0&&s.duration<=120;}
inline std::atomic<bool> enabled{};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::array<Entry,64> inbox{};inline size_t queued{};inline uint64_t queueEpoch{};
inline void Text(char* out,size_t size,const char* in){if(!out||!size)return;size_t at=0;
    if(in)for(;at+1<size&&in[at];++at){auto c=static_cast<unsigned char>(in[at]);out[at]=(c<32||c==127)?' ':static_cast<char>(c);}out[at]=0;}
inline void Push(const Entry& e){if(!enabled.load()||!e.victim||!e.time||!e.victimName[0])return;
    AcquireSRWLockExclusive(&lock);if(queued==inbox.size()){std::move(inbox.begin()+1,inbox.end(),inbox.begin());--queued;}inbox[queued++]=e;ReleaseSRWLockExclusive(&lock);}
inline void Clear(){AcquireSRWLockExclusive(&lock);queued=0;++queueEpoch;ReleaseSRWLockExclusive(&lock);}
struct Store {
    std::array<Entry,64> entries{};size_t count{};uint64_t epoch{},session{};
    void Clear(){count=0;}
    bool Add(const Entry& e){if(!e.victim||!e.time)return false;
        for(size_t i=count;i>0;--i){const auto& old=entries[i-1];if(old.epoch==e.epoch&&old.victim==e.victim&&old.attacker==e.attacker&&
            e.time>=old.time&&e.time-old.time<500)return false;}
        if(count==entries.size()){std::move(entries.begin()+1,entries.end(),entries.begin());--count;}entries[count++]=e;return true;}
    void Frame(const Settings& settings,uint64_t now,uint64_t round,uint64_t world){
        if(!settings.enabled||!Valid(settings)||world!=session||(settings.roundOnly&&round!=epoch))Clear();
        session=world;epoch=round;
        std::array<Entry,64> pending{};size_t n{};
        AcquireSRWLockExclusive(&lock);n=queued;std::copy_n(inbox.begin(),n,pending.begin());queued=0;ReleaseSRWLockExclusive(&lock);
        if(!settings.enabled||!Valid(settings))return;
        for(size_t i=0;i<n;++i)if(!settings.roundOnly||pending[i].epoch==round)Add(pending[i]);
        size_t kept=0;for(size_t i=0;i<count;++i){auto& e=entries[i];if(e.time>now)continue;
            if(settings.duration>0&&now-e.time>static_cast<uint64_t>(settings.duration*1000))continue;
            entries[kept++]=e;}count=kept;
    }
};
inline Store store;
}
