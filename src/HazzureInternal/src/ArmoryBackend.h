// Included after SkinSetAttributes; uses the engine allocator and measured layouts.
#pragma once
static uintptr_t ArmoryItem(uintptr_t weapon){return weapon+HazzureOffsets::Schema::m_AttributeManager+HazzureOffsets::Schema::m_Item;}
static bool ArmoryFits(const SkinSaved* saved,const HazzureArmory::Profile& profile){
    if(!HazzureArmory::Valid(profile)||saved->attributeCount<0||saved->attributeCount>128)return false;
    auto plan=HazzureArmory::Attributes(profile);size_t missing=0;
    auto found=[&](uint16_t id){for(int i=0;i<saved->attributeCount;++i){uint16_t actual{};memcpy(&actual,saved->attributes+i*0x48+0x30,2);if(actual==id)return true;}return false;};
    for(uint16_t id:{6,7,8})if(!found(id))++missing;
    for(size_t i=0;i<plan.count;++i)if(!found(plan.entries[i].id))++missing;
    return static_cast<size_t>(saved->attributeCount)+missing<=128;
}
static uintptr_t ArmoryAttribute(uintptr_t list,uint16_t id){
    int count{};uintptr_t values{};if(!SkinAttributeView(list,count,values))return 0;
    for(int i=0;i<count;++i){auto at=values+static_cast<size_t>(i)*0x48;uint16_t actual{};
        if(!Read(reinterpret_cast<void*>(at+0x30),&actual,sizeof(actual)))return 0;
        if(actual==id)return at;}
    return 0;
}
static bool ApplyArmory(SkinSaved* saved,const HazzureArmory::Profile& profile){
    if(!HazzureArmory::Valid(profile))return false;
    auto weapon=saved->entity,item=ArmoryItem(weapon),view=SkinWeaponView(weapon),list=SkinAttrList(view);
    const auto plan=HazzureArmory::Attributes(profile);int count{};uintptr_t values{};
    if(!SkinAttributeView(list,count,values))return false;
    size_t missing=0;for(size_t i=0;i<plan.count;++i)if(!ArmoryAttribute(list,plan.entries[i].id))++missing;
    if(static_cast<size_t>(count)+missing>128)return false;
    if(!SkinListSane(list))return false;
    auto setter=reinterpret_cast<void(*)(void*,const char*,float)>(client+SKIN_SET_ATTR.rva);
    for(size_t i=0;i<plan.count;++i){const auto& a=plan.entries[i];auto at=ArmoryAttribute(list,a.id);
        // Native setter grows the vector. Integer attributes retain uint32 bits;
        // a numeric float conversion would corrupt sticker/charm/counter IDs.
        if(!at){setter(reinterpret_cast<void*>(view),a.name,0.f);at=ArmoryAttribute(list,a.id);}
        if(!at||!SkinWrite(at+0x34,a.bits)||!SkinWrite(at+0x38,a.bits)||Get<uint32_t>(at+0x34)!=a.bits)return false;
    }
    const int quality=profile.stat?9:saved->quality;
    const int counter=profile.stat?profile.counter:saved->stat;
    if(!SkinWrite(item+0x1bc,quality)||!SkinWrite(weapon+HazzureOffsets::Schema::m_nFallbackStatTrak,counter))return false;
    if(profile.nametag){char names[322]{};strcpy_s(names,161,profile.name);strcpy_s(names+161,161,profile.name);
        if(!SkinWrite(item+0x2f8,names,sizeof(names)))return false;
    }else if(!SkinWrite(item+0x2f8,saved->names,sizeof(saved->names)))return false;
    return true;
}
