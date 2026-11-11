#define WIN32_LEAN_AND_MEAN
#include "../src/LocalInventory.h"
#include <cstdio>
static int checks{};
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    using namespace HazzureInventory;
    Collection c;Item item{};strcpy_s(item.label,"AK variante 1");item.skin={7,180,661,0,.12f,1,1,0};
    CHECK(Add(c,item));CHECK(c.count==1&&c.items[0].id==1&&c.next==2);CHECK(Valid(c));
    auto variant=c.items[0];variant.skin.seed=387;CHECK(Add(c,variant));CHECK(c.items[1].id==2&&c.items[1].skin.seed==387);
    auto edited=c.items[0];edited.skin.paint=302;edited.attributes.nametag=true;strcpy_s(edited.attributes.name,"edited");
    CHECK(Update(c,edited));CHECK(c.items[0].id==1&&c.items[0].skin.paint==302&&c.count==2&&c.next==3);
    auto before=c;edited.skin.wear=2;CHECK(!Update(c,edited));CHECK(c.items[0].skin.wear==before.items[0].skin.wear);
    edited=c.items[0];edited.id=999;CHECK(!Update(c,edited));
    Item knife=item;knife.skin.definition=507;knife.skin.model=507;CHECK(Add(c,knife));CHECK(Category(c.items[2].skin)==0);
    Item glove=item;glove.skin.definition=5030;glove.skin.model=5030;CHECK(Add(c,glove));CHECK(Category(c.items[3].skin)==1);
    item.skin.wear=2;CHECK(!Add(c,item));item.skin.wear=.1f;item.label[0]=1;CHECK(!Add(c,item));strcpy_s(item.label,"nova variante");
    auto bad=c;bad.items[1].id=bad.items[0].id;CHECK(!Valid(bad));bad=c;bad.next=1;CHECK(!Valid(bad));
    CHECK(Erase(c,2));CHECK(Find(c,2)<0&&c.count==3&&c.items[1].id==3);CHECK(!Erase(c,999));
    c.items[0].attributes.nametag=true;strcpy_s(c.items[0].attributes.name,"roundtrip");
    c.items[0].attributes.stickers=true;c.items[0].attributes.slots[0].id=9;c.items[0].attributes.slots[0].rotation=33;
    std::string error;rapidjson::Document document;document.SetObject();
    CHECK(WriteJSON(document,document.GetAllocator(),c,error));Collection restored;
    CHECK(ReadJSON(document,restored,error));CHECK(restored.count==c.count&&restored.next==c.next);
    CHECK(HazzureArmory::SameProfile(c.items[0].attributes,restored.items[0].attributes));
    auto& json=document["localInventory"];json["items"][0]["wear"].SetDouble(-.1);
    auto previous=restored;CHECK(!ReadJSON(document,restored,error));CHECK(restored.count==previous.count&&restored.items[0].skin.wear==previous.items[0].skin.wear);
    json["items"][0]["wear"].SetDouble(.1);json["nextId"].SetUint64(1);CHECK(!ReadJSON(document,restored,error));
    for(size_t n=c.count;n<Capacity;++n)CHECK(Add(c,item));CHECK(c.count==Capacity);CHECK(!Add(c,item));
    rapidjson::Document full;full.SetObject();CHECK(WriteJSON(full,full.GetAllocator(),c,error));
    rapidjson::StringBuffer buffer;rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);full.Accept(writer);
    CHECK(buffer.GetSize()<HazzureConfig::MaxFileBytes);Collection fullCopy;CHECK(ReadJSON(full,fullCopy,error));CHECK(fullCopy.count==Capacity);

    printf("PASS %d local inventory checks\n",checks);return 0;
}
