#pragma once
#include "ArmoryState.h"
#include "ConfigStorage.h"
namespace HazzureArmory {
inline bool ReadJSON(const HazzureConfig::Value& root,Profile* output,std::string& error){
    using namespace HazzureConfig;
    auto array=Find(root,"armory");if(!array)return true;
    auto bad=[&](){error="Armory: perfil/atributo invalido";return false;};
    if(!array->IsArray()||array->Size()>64)return bad();
    for(rapidjson::SizeType i=0;i<array->Size();++i){const auto& item=(*array)[i];auto& p=output[i];if(!item.IsObject())return bad();
        struct B{const char* name;bool Profile::*member;};
        const B bools[]={{"stickers",&Profile::stickers},{"charm",&Profile::charm},{"stat",&Profile::stat},{"increment",&Profile::increment},{"nametag",&Profile::nametag}};
        for(auto f:bools)if(Find(item,f.name)&&!GetBool(item,f.name,p.*f.member,true))return bad();
        struct I{const char* name;int Profile::*member;int low,high;};
        const I ints[]={{"definition",&Profile::definition,0,65535},{"charmId",&Profile::charmId,0,100000},{"charmSeed",&Profile::charmSeed,0,100000},{"counter",&Profile::counter,0,INT_MAX}};
        for(auto f:ints)if(Find(item,f.name)&&!GetInt(item,f.name,p.*f.member,f.low,f.high))return bad();
        struct F{const char* name;float Profile::*member;};const F floats[]={{"charmX",&Profile::charmX},{"charmY",&Profile::charmY},{"charmZ",&Profile::charmZ}};
        for(auto f:floats)if(Find(item,f.name)&&!GetFloat(item,f.name,p.*f.member,-20,20))return bad();
        if(Find(item,"name")){std::string value;if(!GetString(item,"name",value,160)||value.find('\0')!=std::string::npos)return bad();strcpy_s(p.name,value.c_str());}
        if(auto slots=Find(item,"slots")){if(!slots->IsArray()||slots->Size()!=5)return bad();
            for(unsigned j=0;j<5;++j){const auto& value=(*slots)[j];auto& s=p.slots[j];if(!value.IsObject())return bad();
                if(Find(value,"id")&&!GetInt(value,"id",s.id,0,100000))return bad();
                struct S{const char* name;float Sticker::*member;float low,high;};
                const S fields[]={{"x",&Sticker::x,-10,10},{"y",&Sticker::y,-10,10},{"rotation",&Sticker::rotation,-360,360},{"wear",&Sticker::wear,0,1},{"scale",&Sticker::scale,.1f,5}};
                for(auto f:fields)if(Find(value,f.name)&&!GetFloat(value,f.name,s.*f.member,f.low,f.high))return bad();
            }
        }
        if(!Valid(p))return bad();
    }
    return true;
}
inline bool WriteProfile(HazzureConfig::Value& item,HazzureConfig::AllocatorType& allocator,const Profile& p,
                         const HazzureConfig::Value* previous,std::string& error){
    using namespace HazzureConfig;
        if(!Valid(p)){error="Armory: perfil invalido";return false;}
        if(previous&&previous->IsObject())item.CopyFrom(*previous,allocator);
        Set(item,"definition",p.definition,allocator);Set(item,"stickers",p.stickers,allocator);Set(item,"charm",p.charm,allocator);
        Set(item,"stat",p.stat,allocator);Set(item,"increment",p.increment,allocator);Set(item,"nametag",p.nametag,allocator);
        Set(item,"counter",p.counter,allocator);Set(item,"charmId",p.charmId,allocator);Set(item,"charmSeed",p.charmSeed,allocator);
        Set(item,"charmX",static_cast<double>(p.charmX),allocator);Set(item,"charmY",static_cast<double>(p.charmY),allocator);Set(item,"charmZ",static_cast<double>(p.charmZ),allocator);
        Set(item,"name",std::string(p.name),allocator);Value slots(rapidjson::kArrayType);auto oldSlots=Find(item,"slots");
        for(unsigned j=0;j<5;++j){const auto& s=p.slots[j];Value v(rapidjson::kObjectType);
            if(oldSlots&&oldSlots->IsArray()&&j<oldSlots->Size()&&(*oldSlots)[j].IsObject())v.CopyFrom((*oldSlots)[j],allocator);
            Set(v,"id",s.id,allocator);Set(v,"x",static_cast<double>(s.x),allocator);Set(v,"y",static_cast<double>(s.y),allocator);
            Set(v,"rotation",static_cast<double>(s.rotation),allocator);Set(v,"wear",static_cast<double>(s.wear),allocator);Set(v,"scale",static_cast<double>(s.scale),allocator);
            slots.PushBack(v,allocator);
        }
        SetValue(item,"slots",std::move(slots),allocator);return true;
}
inline bool WriteJSON(HazzureConfig::Value& root,HazzureConfig::AllocatorType& allocator,const Profile* inputProfiles,std::string& error){
    using namespace HazzureConfig;Value array(rapidjson::kArrayType);auto previous=Find(root,"armory");
    for(unsigned i=0;i<64;++i){const auto p=ForPersistence(i,inputProfiles[i]);Value item(rapidjson::kObjectType);
        const Value* old=previous&&previous->IsArray()&&i<previous->Size()?&(*previous)[i]:nullptr;
        if(!WriteProfile(item,allocator,p,old,error))return false;array.PushBack(item,allocator);
    }
    SetValue(root,"armory",std::move(array),allocator);return true;
}
}
