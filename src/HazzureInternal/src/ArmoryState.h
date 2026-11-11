#pragma once
#include "GameData.h"
#include "ArmoryCatalog.h"
#include <array>
#include <climits>
#include <cstdio>
namespace HazzureArmory {
struct Sticker {int id{};float x{},y{},rotation{},wear{},scale=1.f;};
struct Profile {
    int definition{};bool stickers{},charm{},stat{},increment=true,nametag{};
    std::array<Sticker,5> slots{};int charmId{},charmSeed{},counter{};
    float charmX{},charmY{},charmZ{};char name[161]{};
};
template<size_t N>inline const char* Lookup(const CatalogEntry(&entries)[N],int id){
    auto it=std::lower_bound(entries,entries+N,id,[](const CatalogEntry& e,int v){return e.id<v;});
    return it!=entries+N&&it->id==id?it->name:nullptr;
}
inline bool Range(float v,float low,float high){return std::isfinite(v)&&v>=low&&v<=high;}
inline bool Valid(const Profile& p){
    if(p.definition<0||p.definition>65535||p.counter<0||p.charmSeed<0||p.charmSeed>100000||
       (p.charmId!=0&&!Lookup(charmsCatalog,p.charmId))||!Range(p.charmX,-20,20)||!Range(p.charmY,-20,20)||!Range(p.charmZ,-20,20)||
       !memchr(p.name,0,sizeof(p.name)))return false;
    for(const unsigned char* c=reinterpret_cast<const unsigned char*>(p.name);*c;++c)if(*c<32||*c==127)return false;
    for(const auto& s:p.slots)if((s.id!=0&&!Lookup(stickersCatalog,s.id))||!Range(s.x,-10,10)||!Range(s.y,-10,10)||
        !Range(s.rotation,-360,360)||!Range(s.wear,0,1)||!Range(s.scale,.1f,5))return false;
    return true;
}
inline bool Active(const Profile& p){return p.definition&&(p.stickers||p.charm||p.stat||p.nametag);}
inline bool SameProfile(const Profile& a,const Profile& b){
    if(a.definition!=b.definition||a.stickers!=b.stickers||a.charm!=b.charm||a.stat!=b.stat||a.increment!=b.increment||
       a.nametag!=b.nametag||a.charmId!=b.charmId||a.charmSeed!=b.charmSeed||a.counter!=b.counter||
       a.charmX!=b.charmX||a.charmY!=b.charmY||a.charmZ!=b.charmZ||strncmp(a.name,b.name,sizeof(a.name)))return false;
    for(size_t i=0;i<a.slots.size();++i){const auto& x=a.slots[i];const auto& y=b.slots[i];
        if(x.id!=y.id||x.x!=y.x||x.y!=y.y||x.rotation!=y.rotation||x.wear!=y.wear||x.scale!=y.scale)return false;}
    return true;
}
inline bool KnifeDefinition(int id){return id==42||id==59||(id>=500&&id<=526);}
inline int KillWeapon(const char* weapon,int current){
    if(!weapon)return 0;if(!strncmp(weapon,"weapon_",7))weapon+=7;
    struct Entry{const char* name;int id;};
    constexpr Entry entries[]={{"deagle",1},{"elite",2},{"fiveseven",3},{"glock",4},{"ak47",7},{"aug",8},{"awp",9},
        {"famas",10},{"g3sg1",11},{"galilar",13},{"m249",14},{"m4a1",16},{"mac10",17},{"p90",19},{"mp5sd",23},{"ump45",24},
        {"xm1014",25},{"bizon",26},{"mag7",27},{"negev",28},{"sawedoff",29},{"tec9",30},{"taser",31},{"hkp2000",32},
        {"mp7",33},{"mp9",34},{"nova",35},{"p250",36},{"scar20",38},{"sg556",39},{"ssg08",40},{"m4a1_silencer",60},
        {"usp_silencer",61},{"cz75a",63},{"revolver",64}};
    for(auto entry:entries)if(!strcmp(weapon,entry.name))return entry.id;
    if(!strncmp(weapon,"knife",5)&&(current==42||current==59||(current>=500&&current<=526)))return current;
    return 0;
}
struct Attribute {uint16_t id{};uint32_t bits{};char name[48]{};};
struct Plan {std::array<Attribute,48> entries{};size_t count{};};
inline uint32_t FloatBits(float v){uint32_t b{};memcpy(&b,&v,4);return b;}
inline Plan Attributes(const Profile& p){
    Plan plan{};if(!Valid(p)||!Active(p))return plan;
    auto add=[&](int id,const char* name,uint32_t bits){auto& a=plan.entries[plan.count++];a.id=static_cast<uint16_t>(id);a.bits=bits;strcpy_s(a.name,name);};
    if(p.stickers)for(int i=0;i<5;++i){const auto& s=p.slots[i];char name[48];
        const char* fields[]={"id","wear","scale","rotation","offset x","offset y","schema"};
        const int ids[]={113+i*4,114+i*4,115+i*4,116+i*4,278+i*2,279+i*2,290+i};
        const uint32_t bits[]={static_cast<uint32_t>(s.id),FloatBits(s.wear),FloatBits(s.scale),FloatBits(s.rotation),FloatBits(s.x),FloatBits(s.y),1u};
        for(int f=0;f<7;++f){sprintf_s(name,"sticker slot %d %s",i,fields[f]);add(ids[f],name,bits[f]);}
    }
    if(p.charm){add(299,"keychain slot 0 id",static_cast<uint32_t>(p.charmId));
        add(300,"keychain slot 0 offset x",FloatBits(p.charmX));add(301,"keychain slot 0 offset y",FloatBits(p.charmY));
        add(302,"keychain slot 0 offset z",FloatBits(p.charmZ));add(306,"keychain slot 0 seed",static_cast<uint32_t>(p.charmSeed));}
    if(p.stat){add(80,"kill eater",static_cast<uint32_t>(p.counter));add(81,"kill eater score type",0);}
    return plan;
}
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::array<Profile,64> profiles{};
inline std::array<int,64> kills{};
inline uint64_t revision{};
inline void Publish(const Profile* input){
    AcquireSRWLockExclusive(&lock);bool changed=false;
    for(size_t i=0;i<profiles.size();++i)if(!SameProfile(profiles[i],input[i])){
        if(profiles[i].definition!=input[i].definition||profiles[i].counter!=input[i].counter||profiles[i].stat!=input[i].stat)kills[i]=0;
        profiles[i]=input[i];changed=true;}
    if(changed)++revision;ReleaseSRWLockExclusive(&lock);
}
inline Profile Snapshot(size_t index,int definition,uint64_t& version){
    AcquireSRWLockShared(&lock);Profile p{};version=revision;
    if(index<profiles.size()&&profiles[index].definition==definition){p=profiles[index];
        if(p.stat&&p.increment)p.counter=static_cast<int>(std::min<int64_t>(INT_MAX,static_cast<int64_t>(p.counter)+kills[index]));}
    ReleaseSRWLockShared(&lock);return p;
}
inline void RecordKill(int definition){
    AcquireSRWLockExclusive(&lock);
    for(size_t i=0;i<profiles.size();++i)if((profiles[i].definition==definition||
        (KnifeDefinition(profiles[i].definition)&&KnifeDefinition(definition)))&&profiles[i].stat&&profiles[i].increment){
        if(kills[i]<INT_MAX-profiles[i].counter){++kills[i];++revision;}break;}
    ReleaseSRWLockExclusive(&lock);
}
inline Profile ForPersistence(size_t index,const Profile& input){
    auto value=input;AcquireSRWLockShared(&lock);
    if(index<profiles.size()&&SameProfile(profiles[index],input)&&value.stat&&value.increment)
        value.counter=static_cast<int>(std::min<int64_t>(INT_MAX,static_cast<int64_t>(value.counter)+kills[index]));
    ReleaseSRWLockShared(&lock);return value;
}
} // namespace HazzureArmory
