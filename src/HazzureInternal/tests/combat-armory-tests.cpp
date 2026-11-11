// Decision, projection, serialization and concurrency tests; no game or OS input.
#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <limits>
#include <thread>
#include "../src/CombatVisuals.h"
#include "../src/ArmoryJSON.h"
#include "../src/MovementTech.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    using namespace HazzureCombat;using namespace HazzureArmory;
    CHECK(Height(49,0)==SoundHeight::Above&&Height(-49,0)==SoundHeight::Below,"sonar distinguishes sound sources on floors above and below the listener");
    CHECK(Height(48,0)==SoundHeight::Level&&Height(-48,0)==SoundHeight::Level,"normal step height remains the same floor at inclusive limits");
    CHECK(Height(std::numeric_limits<float>::quiet_NaN(),0)==SoundHeight::Unknown,"invalid source height cannot produce a misleading altitude color");
    CHECK(!strcmp(SoundLabel(HazzureCombat::Kind::Fire),"DISPARO")&&!strcmp(HeightLabel(SoundHeight::Above),"ACIMA"),"sound type remains explicit when altitude controls color");
    Effects shots;Event fired{};fired.kind=HazzureCombat::Kind::Fire;fired.actor=1;fired.time=100;
    shots.Add(fired);CHECK(shots.count==1&&shots.entries[0].kind==HazzureCombat::Kind::Fire,"confirmed weapon fire is retained for sonar presentation");
    CHECK(Fade(0,2.5f)==1&&Fade(1.25f,2.5f)==.5f&&Fade(2.5f,2.5f)==0,"dormant opacity fades for the complete configured interval");
    CHECK(Fade(-1,2.5f)==0&&Fade(1,0)==0&&Fade(std::numeric_limits<float>::quiet_NaN(),2)==0,"invalid effect timing cannot create persistent geometry");
    CHECK(Age(99,100)>3,"events from a future clock are discarded");
    Inbox queue;std::array<Event,256> drained;
    for(int i=0;i<260;++i)queue.Push({Kind::Damage,100,1,2,{}, {},i});
    auto n=queue.Drain(drained);
    CHECK(n==256&&queue.dropped==4&&drained[0].damage==4&&drained[255].damage==259,"bounded transport keeps the newest 256 events without overflow");
    CHECK(queue.Drain(drained)==0,"event batches are consumed once");
    Event invalid{};invalid.point.x=std::numeric_limits<float>::infinity();queue.Push(invalid);
    CHECK(queue.Drain(drained)==0,"nonfinite world positions never reach the renderer");
    Effects fx;fx.Add({Kind::Footstep,100,7});fx.Add({Kind::Footstep,150,7});fx.Add({Kind::Footstep,150,8});
    CHECK(fx.count==2,"duplicate footsteps are coalesced per actor without merging two players");
    fx.Add({Kind::Damage,200,1,2,{}, {},104,true});CHECK(fx.entries[2].damage==104&&fx.entries[2].lethal,"damage and lethal state survive transport independently");
    fx.Add({Kind::Reset,210});CHECK(fx.count==0,"round transitions clear combat effects");
    fx.Add({Kind::Impact,100});fx.Prune(3200);CHECK(fx.count==0,"expired impact geometry is removed");
    History cache;cache.Begin(123,10);Ghost ghost{};ghost.pawn=1000;ghost.handle=0x18041;ghost.origin={1,2,3};ghost.seen=100;
    cache.Store(ghost);cache.Begin(123,10.1f);
    CHECK(cache.players[0].origin.x==1&&!cache.players[0].visited,"dormant history retains world coordinates and stops refreshing last-seen time");
    cache.Begin(123,1);CHECK(!cache.players[0].handle,"a clock rollback clears the previous map or round history");
    cache.Store(ghost);cache.Begin(456,2);CHECK(!cache.players[0].handle,"an entity-list replacement clears cached player positions");
    cache.Store(ghost);cache.Forget(ghost.handle);CHECK(!cache.players[0].handle,"a dead or recycled pawn can be removed without waiting for fade");
    HazzureGame::Vec2 point{};float angle{};
    CHECK(EdgeArrow({100,0,0},{},0,1920,1080,point,angle)&&point.y<40&&std::fabs(point.x-960)<.01f,"a forward off-screen target points to the top edge");
    CHECK(EdgeArrow({0,-100,0},{},0,1920,1080,point,angle)&&point.x>1850,"a target on the right points to the right edge");
    CHECK(EdgeArrow({-100,0,0},{},0,1920,1080,point,angle)&&point.y>1040,"behind-camera targets point down instead of flipping to the front");
    CHECK(EdgeArrow({100,0,0},{},90,320,240,point,angle)&&point.x>280&&point.x<310,"arrow placement respects camera yaw and narrow viewport margins");
    CHECK(!EdgeArrow({},{},0,1920,1080,point,angle),"coincident positions do not produce an arbitrary arrow");
    HazzureGame::Matrix4 matrix{};for(int i=0;i<4;++i)matrix.m[i][i]=1;
    HazzureGame::Vec2 a{},b{};
    CHECK(ProjectSegment({-2,0,0},{2,0,0},matrix,1000,800,a,b)&&std::fabs(a.x)<.001f&&std::fabs(b.x-1000)<.001f,"world lines clip to the viewport before drawing");
    CHECK(!ProjectSegment({2,0,0},{3,0,0},matrix,1000,800,a,b),"fully off-screen segments do not create huge draw primitives");
    matrix={};matrix.m[0][1]=1;matrix.m[1][2]=1;matrix.m[3][0]=1;
    CHECK(ProjectSegment({-10,-1,0},{100,1,0},matrix,1000,800,a,b),"tracers crossing the near plane remain renderable");
    CHECK(!ProjectSegment({-20,0,0},{-10,0,0},matrix,1000,800,a,b),"fully behind-camera segments are rejected");

    Profile profile{};profile.definition=7;profile.stickers=profile.charm=profile.stat=profile.nametag=true;
    profile.slots[0].id=1;profile.slots[0].rotation=72;profile.slots[0].x=.25f;profile.slots[4].id=2;
    profile.charmId=1;profile.charmSeed=99999;profile.counter=104;strcpy_s(profile.name,"Nome personalizado");
    CHECK(Valid(profile),"five-slot stickers, charm, StatTrak and name form a valid profile");
    CHECK(KillWeapon("ak47",9)==7&&KillWeapon("weapon_m4a1_silencer",7)==60,"the kill event identifies the weapon independently of a later switch");
    CHECK(KillWeapon("inferno",7)==0&&KillWeapon("hegrenade",9)==0,"delayed grenade damage cannot increment the currently held gun");
    auto equivalent=profile;equivalent.name[100]='x';CHECK(SameProfile(profile,equivalent),"unused name-buffer bytes cannot reset counters or dirty a profile");
    auto plan=Attributes(profile);
    CHECK(plan.count==42&&plan.entries[0].id==113&&plan.entries[0].bits==1,"integer sticker IDs retain their exact uint32 encoding");
    CHECK(plan.entries[1].id==114&&plan.entries[2].bits==FloatBits(1.f)&&plan.entries[3].bits==FloatBits(72.f),"wear, scale and rotation remain IEEE float attributes");
    CHECK(plan.entries[4].id==278&&plan.entries[4].bits==FloatBits(.25f)&&plan.entries[6].id==290,"free-placement offsets and schema are emitted for the correct slot");
    CHECK(plan.entries[28].id==129&&plan.entries[28].bits==2,"the fifth sticker has its own attribute indices");
    CHECK(plan.entries[35].id==299&&plan.entries[39].id==306&&plan.entries[39].bits==99999,"charm definition, position and seed use their schema fields");
    CHECK(plan.entries[40].id==80&&plan.entries[40].bits==104&&plan.entries[41].id==81,"StatTrak emits a counter and kill score type");
    auto bad=profile;bad.slots[0].id=999999;CHECK(!Valid(bad)&&Attributes(bad).count==0,"unknown sticker IDs cannot produce an apply plan");
    bad=profile;bad.charmId=999999;CHECK(!Valid(bad),"unknown charms are rejected against the installed catalog");
    bad=profile;bad.slots[4].wear=1.01f;CHECK(!Valid(bad),"sticker scraping cannot exceed its valid range");
    bad=profile;bad.charmZ=std::numeric_limits<float>::quiet_NaN();CHECK(!Valid(bad),"nonfinite charm placement is rejected");
    bad=profile;memset(bad.name,'x',sizeof(bad.name));CHECK(!Valid(bad),"unterminated names cannot overrun the native name buffer");
    bad=profile;strcpy_s(bad.name,"Nome\nquebrado");CHECK(!Valid(bad),"control characters cannot break native labels");
    std::array<Profile,64> configured{};configured[2]=profile;Publish(configured.data());uint64_t version{};
    auto current=Snapshot(2,7,version);auto initialVersion=version;
    CHECK(current.counter==104&&Snapshot(2,9,version).definition==0,"a slot cannot apply a profile to another weapon definition");
    RecordKill(9);CHECK(Snapshot(2,7,version).counter==104,"a kill with another weapon does not increment this StatTrak");
    RecordKill(7);CHECK(Snapshot(2,7,version).counter==105&&version>initialVersion,"a local kill marks the active counter dirty for native reapplication");
    CHECK(ForPersistence(2,configured[2]).counter==105&&Snapshot(2,7,version).counter==105,"saving includes live kills without resetting the running counter");
    configured[2].slots[0].rotation=12;Publish(configured.data());CHECK(Snapshot(2,7,version).counter==105,"editing a sticker preserves accumulated kills");
    configured[2].counter=400;Publish(configured.data());CHECK(Snapshot(2,7,version).counter==400,"setting a new counter replaces the old baseline exactly");
    std::thread first([](){for(int i=0;i<500;++i)RecordKill(7);}),second([](){for(int i=0;i<500;++i)RecordKill(7);});first.join();second.join();
    CHECK(Snapshot(2,7,version).counter==1400,"concurrent event callbacks cannot lose StatTrak increments");
    configured[2].counter=INT_MAX;Publish(configured.data());RecordKill(7);CHECK(Snapshot(2,7,version).counter==INT_MAX,"the kill counter saturates instead of overflowing");
    configured[0].definition=42;configured[0].stat=true;Publish(configured.data());RecordKill(507);
    CHECK(Snapshot(0,42,version).counter==1,"a replacement knife model increments its configured base knife counter");
    HazzureConfig::Document document;std::string error;
    CHECK(document.Parse("{\"extension\":\"keep\",\"armory\":[{\"future\":17}]}",error)&&WriteJSON(document.Root(),document.Allocator(),configured.data(),error),"Armory serializes with forward-compatible fields");
    std::array<Profile,64> restored{};
    CHECK(ReadJSON(document.Root(),restored.data(),error)&&restored[2].counter==INT_MAX&&restored[2].slots[4].id==2&&!strcmp(restored[2].name,"Nome personalizado"),"complete inventory details survive a JSON round trip");
    auto root=HazzureConfig::Find(document.Root(),"extension");auto list=HazzureConfig::Find(document.Root(),"armory");
    CHECK(root&&root->IsString()&&HazzureConfig::Find((*list)[0],"future"),"unknown root and per-item fields are preserved");
    HazzureConfig::Document malformed;malformed.Parse("{\"armory\":[{\"slots\":[]}]}",error);CHECK(!ReadJSON(malformed.Root(),restored.data(),error),"malformed sticker arrays are rejected");
    malformed.Parse("{\"armory\":[{\"counter\":-1}]}",error);CHECK(!ReadJSON(malformed.Root(),restored.data(),error),"negative imported counters are rejected");
    malformed.Parse("{\"armory\":[{\"charmId\":99999}]}",error);CHECK(!ReadJSON(malformed.Root(),restored.data(),error),"imports validate IDs against the installed schema");
    configured={};Publish(configured.data());

    HazzureMove::EdgeGate gate;
    CHECK(!gate.Update(100,true,true,true,100,HazzureMove::Support::Ground),"solid ground does not trigger an edge jump");
    CHECK(!gate.Update(100,true,true,true,100,HazzureMove::Support::Unavailable),"failed physical traces never authorize a jump");
    CHECK(gate.Update(100,true,true,true,100,HazzureMove::Support::Edge),"a supported walking state can jump before the predicted ledge");
    CHECK(!gate.Update(100,true,true,true,100,HazzureMove::Support::Edge),"the same ledge produces one jump request");
    gate.Update(100,true,false,true,100,HazzureMove::Support::Edge);
    CHECK(gate.Update(100,true,true,true,100,HazzureMove::Support::Edge),"landing rearms the next ledge decision");
    CHECK(!gate.Update(100,false,true,true,100,HazzureMove::Support::Edge),"disabling movement cancels pending ledge state");
    CHECK(!gate.Update(200,true,true,false,100,HazzureMove::Support::Edge),"ladders and noclip cannot be treated as walking edges");
    CHECK(!gate.Update(200,true,true,true,0,HazzureMove::Support::Edge),"standing still cannot trigger edge jumping");
    HazzureGame::Vec3 target{};
    CHECK(HazzureMove::LadderTarget({1,0,0},target)&&target.x==-89&&std::fabs(target.y-135)<.01f,"ladder steering follows the actual surface normal");
    CHECK(!HazzureMove::LadderTarget({},target),"an invalid ladder normal cannot steer the camera");
    printf("RESULT: %u passed, %u failed; own process, no engine calls or OS input.\n",passed,failed);
    return failed?1:0;
}
