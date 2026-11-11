// Local collection/loadout only. Does not manufacture Steam/GC shared objects.
#pragma once
#include "SkinProtocol.h"
#include "ArmoryJSON.h"
#include <array>
#include <limits>
namespace HazzureInventory {
inline constexpr size_t Capacity=96;
struct Item {uint64_t id{};char label[81]{};SkinConfig skin{};HazzureArmory::Profile attributes{};bool equippedCT{};bool equippedT{};};
struct Collection {std::array<Item,Capacity> items{};size_t count{};uint64_t next=1;};
inline bool Glove(int d){return d==4725||(d>=5027&&d<=5035);}
inline int Category(const SkinConfig& s){return HazzureArmory::KnifeDefinition(s.definition)?0:Glove(s.definition)?1:2;}
inline bool Valid(const Item& i){
    const auto& s=i.skin;
    if(!i.id||!memchr(i.label,0,sizeof(i.label))||!i.label[0]||s.definition<1||s.definition>65535||s.paint<0||s.paint>20000||
       s.seed<0||s.seed>1000||s.model<0||s.model>65535||!std::isfinite(s.wear)||s.wear<0||s.wear>1||
       s.enabled!=1||(s.legacy!=0&&s.legacy!=1)||!HazzureArmory::Valid(i.attributes)||i.attributes.definition!=s.definition)return false;
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(i.label);*p;++p)if(*p<32||*p==127)return false;
    return true;
}
inline bool Valid(const Collection& c){
    if(c.count>Capacity||!c.next)return false;
    for(size_t n=0;n<c.count;++n){if(!Valid(c.items[n])||c.items[n].id>=c.next)return false;
        for(size_t j=0;j<n;++j)if(c.items[j].id==c.items[n].id)return false;}
    return true;
}
inline bool Add(Collection& c,Item item,uint64_t* created=nullptr){
    if(!Valid(c)||c.count==Capacity||c.next==(std::numeric_limits<uint64_t>::max)())return false;
    item.id=c.next;item.skin.enabled=1;item.attributes.definition=item.skin.definition;
    if(!Valid(item))return false;
    c.items[c.count++]=item;++c.next;if(created)*created=item.id;return true;
}
inline int Find(const Collection& c,uint64_t id){for(size_t n=0;n<c.count;++n)if(c.items[n].id==id)return static_cast<int>(n);return -1;}
inline bool Erase(Collection& c,uint64_t id){int n=Find(c,id);if(n<0)return false;
    for(size_t j=static_cast<size_t>(n)+1;j<c.count;++j)c.items[j-1]=c.items[j];c.items[--c.count]={};return true;}
inline bool Update(Collection& c,const Item& item){
    if(!Valid(c)||!Valid(item))return false;
    int index=Find(c,item.id);if(index<0)return false;
    c.items[index]=item;return true;
}
inline void EquipItem(Collection& c,uint64_t id,bool ct,bool tr){
    int idx=Find(c,id);if(idx<0)return;
    int def=c.items[idx].skin.definition;
    bool isKnife=HazzureArmory::KnifeDefinition(def);
    bool isGlove=Glove(def);
    for(size_t n=0;n<c.count;++n){
        auto& item=c.items[n];
        int itemDef=item.skin.definition;
        bool match=(isKnife&&HazzureArmory::KnifeDefinition(itemDef))||
                   (isGlove&&Glove(itemDef))||
                   (!isKnife&&!isGlove&&itemDef==def);
        if(match){
            if(ct)item.equippedCT=(item.id==id);
            if(tr)item.equippedT=(item.id==id);
        }
    }
}

inline bool ReadJSON(const HazzureConfig::Value& root,Collection& output,std::string& error){
    using namespace HazzureConfig;auto value=HazzureConfig::Find(root,"localInventory");if(!value)return true;
    auto bad=[&](){error="Colecao local invalida";return false;};
    int version{};if(!value->IsObject()||!GetInt(*value,"version",version,1,1))return bad();
    auto array=HazzureConfig::Find(*value,"items"),next=HazzureConfig::Find(*value,"nextId");if(!array||!array->IsArray()||array->Size()>Capacity||!next||!next->IsUint64())return bad();
    Collection candidate{};candidate.next=next->GetUint64();
    for(const auto& entry:array->GetArray()){
        Item item{};auto id=HazzureConfig::Find(entry,"id");std::string label;
        if(!entry.IsObject()||!id||!id->IsUint64()||!GetString(entry,"label",label,80)||label.find('\0')!=std::string::npos)return bad();
        item.id=id->GetUint64();strcpy_s(item.label,label.c_str());auto& s=item.skin;
        if(!GetInt(entry,"definition",s.definition,1,65535)||!GetInt(entry,"paint",s.paint,0,20000)||!GetInt(entry,"seed",s.seed,0,1000)||
           !GetInt(entry,"model",s.model,0,65535)||!GetInt(entry,"legacy",s.legacy,0,1)||!GetFloat(entry,"wear",s.wear,0,1))return bad();s.enabled=1;
        // Reuse the canonical attribute validator; the wrapper holds exactly one profile.
        auto attributes=HazzureConfig::Find(entry,"attributes");if(!attributes||!attributes->IsObject())return bad();
        rapidjson::Document wrapper;wrapper.SetObject();Value profiles(rapidjson::kArrayType),profile;profile.CopyFrom(*attributes,wrapper.GetAllocator());
        profiles.PushBack(profile,wrapper.GetAllocator());SetValue(wrapper,"armory",std::move(profiles),wrapper.GetAllocator());
        std::array<HazzureArmory::Profile,64> parsed{};
        if(!HazzureArmory::ReadJSON(wrapper,parsed.data(),error))return false;item.attributes=parsed[0];
        GetBool(entry, "equippedCT", item.equippedCT, true);
        GetBool(entry, "equippedT", item.equippedT, true);
        candidate.items[candidate.count++]=item;
    }
    if(!Valid(candidate))return bad();output=candidate;return true;
}
inline bool WriteJSON(HazzureConfig::Value& root,HazzureConfig::AllocatorType& a,const Collection& c,std::string& error){
    using namespace HazzureConfig;if(!Valid(c)){error="Colecao local invalida";return false;}
    Value result(rapidjson::kObjectType),array(rapidjson::kArrayType);Set(result,"version",1,a);Value next;next.SetUint64(c.next);SetValue(result,"nextId",std::move(next),a);
    for(size_t n=0;n<c.count;++n){const auto& i=c.items[n];const auto& s=i.skin;Value v(rapidjson::kObjectType),id;id.SetUint64(i.id);SetValue(v,"id",std::move(id),a);
        Set(v,"label",std::string(i.label),a);Set(v,"definition",s.definition,a);Set(v,"paint",s.paint,a);Set(v,"seed",s.seed,a);
        Set(v,"model",s.model,a);Set(v,"legacy",s.legacy,a);Set(v,"wear",static_cast<double>(s.wear),a);
        Set(v,"equippedCT",i.equippedCT,a);Set(v,"equippedT",i.equippedT,a);
        Value attrs(rapidjson::kObjectType);if(!HazzureArmory::WriteProfile(attrs,a,i.attributes,nullptr,error))return false;
        SetValue(v,"attributes",std::move(attrs),a);array.PushBack(v,a);
    }
    SetValue(result,"items",std::move(array),a);SetValue(root,"localInventory",std::move(result),a);return true;
}
}
