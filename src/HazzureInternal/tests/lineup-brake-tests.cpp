#define NOMINMAX
#define INTERNAL_SELF_TEST
#include <cstdio>
#include "../src/LineupLibrary.h"
#include "../src/ShotBrake.h"
#include "../src/GrenadeZones.h"
static unsigned passed{},failed{};
#define CHECK(x,label) do{if(x){++passed;printf("PASS: %s\n",label);}else{++failed;printf("FAIL: %s\n",label);}}while(0)
int main(){
    using namespace HazzureLineups;
    Entry entry{};strcpy_s(entry.map,"de_mirage");strcpy_s(entry.name,"Treino medido");entry.position={-123,250,80};entry.angles={-30,179.9f,0};
    CHECK(Valid(entry)&&Ready(entry,"de_mirage",45,entry.position,entry.angles),"a recorded lineup matches its map, grenade, position and view");
    CHECK(Ready(entry,"de_mirage",45,entry.position,{-30,-179.9f,0}),"alignment correctly wraps yaw across 180 degrees");
    CHECK(!Ready(entry,"de_dust2",45,entry.position,entry.angles)&&!Ready(entry,"de_mirage",44,entry.position,entry.angles),"map and grenade mismatches cannot authorize a throw");
    CHECK(!Ready(entry,"de_mirage",45,{-120,250,80},entry.angles)&&!Ready(entry,"de_mirage",45,entry.position,{-29,179.9f,0}),"position and angular errors block the throw");
    Library library{};strcpy_s(library.map,"de_mirage");library.entries[0]=entry;library.count=1;
    CHECK(Nearest(library,45,entry.position)==0&&Nearest(library,44,entry.position)==-1,"nearest marker is restricted to the held grenade");
    CHECK(Nearest(library,45,{1000,1000,1000})==-1,"far away recorded positions do not populate the current helper");
    HazzureConfig::Document document;std::string error;document.Parse("{\"future\":17}",error);
    CHECK(WriteJSON(document.Root(),document.Allocator(),library,error),"lineup library serializes without using a file or clipboard");
    Library restored{};
    CHECK(ReadJSON(document.Root(),restored,error)&&restored.count==1&&restored.entries[0].position.x==-123&&restored.entries[0].angles.x==-30,"negative positions and angles survive the JSON round trip");
    CHECK(HazzureConfig::Find(document.Root(),"future")!=nullptr,"lineup serialization preserves unrelated configuration keys");
    HazzureConfig::Document bad;bad.Parse("{\"lineups\":{\"map\":\"de_mirage\",\"entries\":[{\"name\":\"broken\"}]}}",error);
    CHECK(!ReadJSON(bad.Root(),restored,error)&&restored.count==1,"invalid imports do not partially replace the library");
    library.count=65;CHECK(!WriteJSON(document.Root(),document.Allocator(),library,error),"library capacity is validated before serialization");
    using namespace HazzureBrake;
    Request wanted{1,2,3,100,75,true,true};Configure(wanted);auto generation=revision;
    CHECK(!Ready(wanted,100),"braking must be confirmed by the frame callback before firing");
    Result(Status::Braking,generation,100);CHECK(!Ready(wanted,100),"an active braking command does not itself prove safe speed");
    Result(Status::Ready,generation,100);CHECK(Ready(wanted,132)&&!Ready(wanted,133),"completed speed decisions expire in 32 ms");
    wanted.target=4;wanted.time=101;Configure(wanted);
    Result(Status::Ready,generation,101);CHECK(!Ready(wanted,101),"a result from an old target generation cannot authorize the new one");
    generation=revision;Result(Status::Ready,generation,101);CHECK(Ready(wanted,101),"new acquisition can proceed after its own frame result");
    Cancel();CHECK(!Ready(wanted,101),"cancellation immediately invalidates the shot gate");
    Configure(wanted);Result(Status::Ready,generation,102);CHECK(!Ready(wanted,102),"cancelling and reacquiring the same target does not revive an old result");
    Result(Status::Edge,revision,102);CHECK(!Ready(wanted,102),"unconfirmed ledge support never passes the braking gate");
    Result(Status::TimedOut,revision,102);CHECK(!Ready(wanted,102),"a braking timeout cannot turn into permission to fire");
    Cancel();
    HazzureZones::enabled=true;
    HazzureZones::Clear();HazzureZones::Push({HazzureZones::Kind::Smoke,5,{100,20,0},1000});
    auto zones=HazzureZones::Snapshot(1000);
    CHECK(zones[0].id==5&&HazzureZones::Alive(zones[0],1000),"smoke events create bounded visible guides");
    CHECK(HazzureZones::Alive(zones[0],18999)&&!HazzureZones::Alive(zones[0],19000),"estimated smoke duration has a finite exact expiry");
    HazzureZones::Push({HazzureZones::Kind::SmokeEnd,5,{},2000});
    CHECK(!HazzureZones::Snapshot(2000)[0].start,"the matching end event removes the guide before its estimated duration");
    for(int i=0;i<100;++i)HazzureZones::Push({HazzureZones::Kind::Fire,i+1,{100,20,0},3000});
    size_t count=0;for(auto& zone:HazzureZones::Snapshot(3000))if(zone.start)++count;
    CHECK(count==32,"an event burst cannot exceed the fixed zone capacity");
    HazzureZones::Push({HazzureZones::Kind::Reset,0,{},0});
    count=0;for(auto& zone:HazzureZones::Snapshot(3000))if(zone.start)++count;
    CHECK(count==0,"round reset clears all grenade guides");
    HazzureZones::enabled=false;HazzureZones::Push({HazzureZones::Kind::HE,1,{},4000});
    CHECK(!HazzureZones::Snapshot(4000)[0].start,"disabled guides ignore late events");
    printf("RESULT: %u passed, %u failed; own process, no engine calls or OS input.\n",passed,failed);return failed?1:0;
}
