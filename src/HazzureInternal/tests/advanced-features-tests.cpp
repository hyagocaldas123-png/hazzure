#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include <limits>
#include "../src/AdvancedMovement.h"
#include "../src/LineupAutomation.h"
#include "../src/MapContext.h"
#include "../src/LineupCatalogue.h"
#include "../src/InfernoCells.h"
#include "../src/PoseHistory.h"
#include "../src/CombatAccuracy.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    HazzureVisibility::Ray ray{};
    CHECK(HazzurePhysics::Box(ray,{-16,-16,0},{16,16,72})&&ray.bytes[0x28]==2,"box shape uses the installed AABB discriminant");
    HazzureGame::Vec3 minimum{},maximum{};memcpy(&minimum,ray.bytes,12);memcpy(&maximum,ray.bytes+12,12);
    CHECK(minimum.x==-16&&minimum.z==0&&maximum.z==72,"native shape stores exact minimum and maximum vectors");
    CHECK(!HazzurePhysics::Box(ray,{1,0,0},{-1,0,0}),"reversed hull bounds are rejected");
    using namespace HazzureMovementTech;
    auto strafe=OptimalStrafe({250,0,0},0,1.f/64,12,30,250,1,1);
    CHECK(strafe.valid&&strafe.gain>0&&strafe.side==-1,"air optimizer predicts positive acceleration for a feasible lateral input");
    auto reverse=OptimalStrafe({250,0,0},0,1.f/64,12,30,250,1,-1);
    CHECK(reverse.valid&&reverse.side==1&&std::fabs(strafe.gain-reverse.gain)<.001f,"left and right optimized directions have symmetric gains");
    CHECK(!OptimalStrafe({},0,1.f/64,12,30,250,1,0).valid&&!OptimalStrafe({250,0,0},0,0,12,30,250,1,0).valid,"stationary and invalid timing states do not generate steering");
    float nan=std::numeric_limits<float>::quiet_NaN();
    CHECK(!OptimalStrafe({250,0,0},0,1.f/64,nan,30,250,1,0).valid,"invalid server physics is unavailable rather than guessed");
    LandingGate landingGate;Prediction p{true,true,false,false,.3f,{0,0,1}};
    CHECK(landingGate.Step(1,true,false,true,p,true,false,false).duck,"jumpbug prepares a crouch on confirmed imminent landing");
    CHECK(landingGate.Step(1,true,true,false,{},true,false,false).jump,"jumpbug requests a jump only after measured ground contact");
    CHECK(!landingGate.Step(1,true,true,false,{},true,false,false).jump,"a landing cannot request repeated jumps");
    landingGate.Step(1,true,false,true,p,true,false,false);
    CHECK(!landingGate.Step(2,true,true,false,{},true,false,false).jump,"pawn change invalidates the prepared landing");
    CHECK(!landingGate.Step(2,true,false,true,{},true,false,false).duck,"unknown physics cannot prepare a jumpbug");
    landingGate.Step(2,true,false,true,p,true,false,false);
    for(int i=0;i<3;++i)landingGate.Step(2,true,false,true,{},true,false,false);
    CHECK(!landingGate.Step(2,true,true,false,{},true,false,false).jump,"a stale predicted landing cannot authorize a much later jump");
    p.edge=true;
    CHECK(landingGate.Step(2,true,false,true,p,false,true,false).duck&&!landingGate.Step(2,true,false,true,p,false,true,false).duck,"edgebug emits a bounded crouch/release sequence near a detected ledge");
    p.landing=false;p.edge=false;p.wall=true;p.normal={1,0,0};
    CHECK(landingGate.Step(2,true,false,true,p,false,false,true).surf,"pixel-surf assistance requires a confirmed wall contact");
    CHECK(!landingGate.Step(2,false,false,true,p,false,false,true).surf,"context loss cancels movement techniques");
    Prediction bad{true,true,false,false,.3f,{0,0,0}};
    CHECK(!Confirmed(bad)&&!landingGate.Step(2,true,false,true,bad,true,false,false,100).duck,"zero normals cannot authorize a landing action");
    bad.normal={0,0,1};bad.fraction=nan;
    CHECK(!Confirmed(bad),"nonfinite collision fractions cannot authorize movement");
    bad.fraction=1;
    CHECK(!Confirmed(bad),"a no-contact sweep cannot claim a landing");
    LandingGate timed;Prediction floor{true,true,false,false,.3f,{0,0,1}};
    CHECK(timed.Step(1,true,false,true,floor,true,false,false,100).duck,"timed landing starts a bounded crouch window");
    CHECK(!timed.Step(1,true,true,false,{},true,false,false,151).jump,"a delayed render frame cannot consume an expired landing prediction");
    timed.Step(1,true,false,true,floor,true,false,false,200);
    CHECK(!timed.Step(1,true,true,false,{},true,false,false,199).jump,"clock rollback invalidates prepared movement");
    timed.Step(1,true,false,true,floor,true,false,false,300);
    for(unsigned t=301;t<312;++t)timed.Step(1,true,false,true,floor,true,false,false,t);
    CHECK(timed.Step(1,true,true,false,{},true,false,false,312).jump,"render frequency does not expire a fresh timed landing after three frames");
    timed.Step(1,true,false,true,floor,true,false,false,400);
    timed.Step(1,true,false,true,{},true,false,false,401);
    CHECK(!timed.Step(1,true,true,false,{},true,false,false,402).jump,"unknown airborne collision invalidates a previously prepared landing");
    Prediction ledge{true,true,true,false,.3f,{0,0,1}};
    timed.Step(1,true,false,true,ledge,false,true,false,410);
    Prediction corrupt=ledge;corrupt.normal={};
    timed.Step(1,true,false,true,corrupt,false,true,false,411);
    CHECK(!timed.edgePrepared,"invalid normal clears edge preparation even when the producer claims valid");
    CHECK(timed.Step(1,true,false,true,ledge,false,true,false,412).duck,"a newly confirmed ledge can rearm after the invalid sample");
    auto turnWide=OptimalStrafe({250,0,0},0,1.f/64,12,30,250,1,500);
    CHECK(turnWide.valid&&std::fabs(turnWide.yaw-strafe.yaw)<.001f,"turn input is a direction, not a multiplier for steering angle");
    using HazzurePistol::Snapshot;
    HazzurePistol::Gate pistol;Snapshot gun{};gun.pawn=1;gun.weapon=2;gun.handle=3;gun.definition=61;
    gun.valid=gun.ready=true;gun.clip=12;gun.nextTick=100;
    CHECK(pistol.Step(gun,true,true,100)==65537,"native ready semiautomatic weapon starts one press");
    CHECK(pistol.Step(gun,true,true,104)==65537,"high FPS retains the minimum press edge");
    CHECK(pistol.Step(gun,true,true,116)==256,"owned pistol press has an explicit release");
    CHECK(pistol.Step(gun,true,true,124)==256,"unchanged cooldown and clip cannot repeat a shot");
    gun.nextTick=110;gun.clip=11;gun.ready=false;
    CHECK(pistol.Step(gun,true,true,140)==256,"cooldown not ready cannot authorize the next press");
    gun.ready=true;
    CHECK(pistol.Step(gun,true,true,160)==65537,"a confirmed advanced cooldown allows a new press");
    CHECK(pistol.Step(gun,false,true,164)==65537&&!pistol.owned,"turning assistance off restores physically held attack");
    gun.reload=true;
    CHECK(pistol.Step(gun,true,true,200)==-1,"reloading cannot start automatic pistol input");
    gun.reload=false;gun.clip=0;
    CHECK(pistol.Step(gun,true,true,204)==-1,"empty clip cannot start automatic pistol input");
    gun.clip=12;gun.definition=63;
    CHECK(pistol.Step(gun,true,true,208)==-1,"fully automatic CZ is not treated as a semiautomatic pistol");
    gun.definition=64;
    CHECK(pistol.Step(gun,true,true,212)==-1,"charge-based revolver is not subjected to tap timing");
    gun.definition=61;gun.nextFraction=nan;
    CHECK(!HazzurePistol::Valid(gun),"unknown native cooldown fraction is unavailable");
    gun.nextFraction=0;gun.time=100;HazzurePistol::Publish(gun);
    CHECK(HazzurePistol::Read(1,2,3,175).valid&&!HazzurePistol::Read(1,2,3,176).valid,"native readiness snapshot expires before stale input can be emitted");
    CHECK(!HazzurePistol::Read(1,2,4,120).valid,"reused weapon handles cannot consume a previous cooldown");
    HazzurePistol::Publish({});
    char map[64]{};
    CHECK(HazzureMap::Normalize("maps/de_mirage.vmap",map)&&!strcmp(map,"de_mirage"),"map event names normalize to a catalogue key");
    CHECK(!HazzureMap::Normalize("bad map;command",map),"map names cannot contain console commands");
    CHECK(HazzureMap::Publish("de_nuke")&&HazzureMap::Read(map)&&!strcmp(map,"de_nuke"),"automatic map channel publishes a coherent snapshot");
    HazzureMap::Clear();CHECK(!HazzureMap::Read(map),"map reset does not retain the previous level");
    using namespace HazzureLineupAssist;
    HazzureLineups::Entry entry{};strcpy_s(entry.map,"de_mirage");strcpy_s(entry.name,"Test");entry.position={10,20,30};entry.angles={-20,90,0};
    Input input{};input.active=input.context=input.key=input.pin=input.ground=true;input.pawn=1;input.weapon=2;input.selected=0;input.definition=45;
    input.now=100;input.position=entry.position;input.angles={0,0,0};input.strength=1;input.map="de_mirage";
    Sequence sequence;auto output=sequence.Step(input,&entry);
    CHECK(output.aim&&sequence.phase==Phase::Align,"activation first requests alignment without throwing");
    input.angles=entry.angles;input.now=110;output=sequence.Step(input,&entry);
    CHECK(output.jump&&!output.releaseAttack&&sequence.phase==Phase::Jump,"jumpthrow waits for takeoff instead of releasing immediately");
    input.ground=false;input.now=125;output=sequence.Step(input,&entry);
    CHECK(output.releaseAttack&&sequence.phase==Phase::Done,"measured takeoff releases the prepared grenade once");
    CHECK(!sequence.Step(input,&entry).releaseAttack,"held activation key cannot repeat the throw");
    sequence.Reset();entry.runMs=120;entry.throwMode=0;input.ground=true;input.now=200;
    output=sequence.Step(input,&entry);CHECK(output.forward&&!output.releaseAttack,"run-throw begins its configured run before releasing");
    input.now=319;CHECK(sequence.Step(input,&entry).forward,"run duration is measured from its own start");
    input.now=320;CHECK(sequence.Step(input,&entry).releaseAttack,"run-throw releases at the configured deadline");
    sequence.Reset();input.now=400;sequence.Step(input,&entry);input.weapon=3;input.now=401;
    CHECK(sequence.Step(input,&entry).restore&&sequence.phase==Phase::Cancelled,"weapon change cancels and releases owned movement");
    sequence.Reset();input.weapon=2;input.now=500;sequence.Step(input,&entry);input.context=false;
    CHECK(sequence.Step(input,&entry).restore&&!sequence.Step(input,&entry).releaseAttack,"focus/menu loss cancels without releasing a shot");
    sequence.Reset();input.context=true;input.now=450;input.key=true;sequence.Step(input,&entry);
    input.key=false;input.now=451;sequence.Step(input,&entry);input.key=true;input.now=452;
    CHECK(sequence.Step(input,&entry).restore&&sequence.phase==Phase::Cancelled,"a second activation cancels an in-progress throw sequence");
    HazzureConfig::Document document;HazzureLineups::Library library{},restored{};strcpy_s(library.map,entry.map);library.count=1;library.entries[0]=entry;std::string error;
    CHECK(HazzureLineups::WriteJSON(document.Root(),document.Allocator(),library,error)&&HazzureLineups::ReadJSON(document.Root(),restored,error)&&
        restored.entries[0].runMs==120&&restored.entries[0].throwMode==0,"run time and throw mode survive configuration serialization");
    library={};CHECK(HazzureLineupCatalogue::Merge(library)==4&&library.count==4,"published Mirage coordinate references can populate the library");
    CHECK(HazzureLineupCatalogue::Merge(library)==0&&library.count==4,"adding the catalogue again preserves existing entries without duplicates");
    bool valid=true;for(int i=0;i<library.count;++i)valid=valid&&HazzureLineups::Valid(library.entries[i])&&!library.entries[i].validated;
    CHECK(valid,"reference coordinates are valid but do not silently authorize an uncalibrated throw");
    sequence.Reset();input.context=input.key=true;input.map="de_mirage";input.position=library.entries[0].position;input.now=600;
    CHECK(!sequence.Step(input,&library.entries[0]).aim,"unvalidated catalogue metadata blocks automation");
    HazzureInferno::Cells cells{};cells.entity=1;cells.handle=2;cells.count=1;cells.positions[0]={100,200,30};cells.halfWidth=32;
    CHECK(HazzureInferno::Validate(cells),"bounded native fire cells form a usable read-only snapshot");
    cells.count=65;CHECK(!HazzureInferno::Validate(cells),"untrusted fire cell count cannot overrun the snapshot");
    cells.count=1;cells.positions[0].x=nan;CHECK(!HazzureInferno::Validate(cells),"nonfinite fire positions are rejected");
    HazzurePoses::Clear();HazzureCombat::Ghost ghost{};ghost.pawn=100;ghost.handle=1;ghost.seen=100;ghost.boneCount=1;
    HazzurePoses::Store(5,ghost);ghost.seen=120;HazzurePoses::Store(5,ghost);size_t visited=0;
    HazzurePoses::Visit(125,160,[&](const auto&,float alpha){++visited;valid=valid&&alpha>0&&alpha<=1;});
    CHECK(visited==1&&valid,"visual history yields a bounded previous pose with temporal fade");
    ghost.handle=2;ghost.seen=140;HazzurePoses::Store(5,ghost);visited=0;
    HazzurePoses::Visit(145,160,[&](const auto&,float){++visited;});CHECK(visited==0,"recycled pawn address invalidates the previous entity handle's poses");
    ghost.seen=160;HazzurePoses::Store(5,ghost);visited=0;HazzurePoses::Visit(500,160,[&](const auto&,float){++visited;});
    CHECK(visited==0,"expired history never becomes an indefinitely visible ghost");
    HazzurePoses::Clear();
    auto lateral=HazzureAccuracy::CounterStrafe({0,250,0},0,0);
    CHECK(lateral.valid&&lateral.brake&&lateral.side>.99f,"positive world Y velocity brakes with Source's right input");
    CHECK(HazzureAccuracy::CounterStrafe({.01f,0,0},0,0).brake&&!HazzureAccuracy::CounterStrafe({},0,0).brake,"full-stop waits for measured zero without replacing velocity");
    printf("RESULT: %u passed, %u failed; own process, no engine calls or OS input.\n",passed,failed);return failed?1:0;
}
