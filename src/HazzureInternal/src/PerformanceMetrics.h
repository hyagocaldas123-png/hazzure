// Bounded measurements. Recording has no heap allocation; summaries copy under a short lock.
#pragma once
#include <windows.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace HazzurePerformance {
enum class Stage {Overlay,Aim,Trigger,Visibility,Smoke,Count};
struct Samples {std::array<float,128> values{};size_t count{},cursor{};uint64_t total{};};
struct Summary {size_t samples{};uint64_t total{};float average{},p95{},p99{},maximum{};};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::array<Samples,static_cast<size_t>(Stage::Count)> stages{};
inline uint64_t Clock(){LARGE_INTEGER q{};QueryPerformanceCounter(&q);return static_cast<uint64_t>(q.QuadPart);}
inline double Frequency(){static const double f=[](){LARGE_INTEGER q{};QueryPerformanceFrequency(&q);return static_cast<double>(q.QuadPart);}();return f;}
inline float Milliseconds(uint64_t begin,uint64_t end){return end>=begin&&Frequency()>0?static_cast<float>((end-begin)*1000.0/Frequency()):0;}
inline void Record(Stage stage,float ms){
    if(!std::isfinite(ms)||ms<0||ms>60000||stage>=Stage::Count)return;
    AcquireSRWLockExclusive(&lock);auto& s=stages[static_cast<size_t>(stage)];
    s.values[s.cursor]=ms;s.cursor=(s.cursor+1)%s.values.size();s.count=std::min(s.count+1,s.values.size());++s.total;ReleaseSRWLockExclusive(&lock);
}
inline void End(Stage stage,uint64_t begin){Record(stage,Milliseconds(begin,Clock()));}
inline bool Budget(uint64_t begin,float ms=2.f){return Milliseconds(begin,Clock())>=ms;}
inline Summary Read(Stage stage){
    if(stage>=Stage::Count)return {};AcquireSRWLockShared(&lock);auto s=stages[static_cast<size_t>(stage)];ReleaseSRWLockShared(&lock);
    Summary out{};out.samples=s.count;out.total=s.total;if(!s.count)return out;
    std::sort(s.values.begin(),s.values.begin()+s.count);float sum=0;for(size_t i=0;i<s.count;++i)sum+=s.values[i];
    out.average=sum/s.count;out.maximum=s.values[s.count-1];out.p95=s.values[static_cast<size_t>(std::ceil(s.count*.95))-1];
    out.p99=s.values[static_cast<size_t>(std::ceil(s.count*.99))-1];return out;
}
inline void Reset(){AcquireSRWLockExclusive(&lock);stages={};ReleaseSRWLockExclusive(&lock);}
struct Measure {Stage stage;uint64_t begin;explicit Measure(Stage s):stage(s),begin(Clock()){}~Measure(){End(stage,begin);}};
}
