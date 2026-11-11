#pragma once
namespace HazzureWeapons {
// Preserve the original six UI indices so existing profiles keep their weapon.
inline constexpr int definitions[]={0,7,9,60,1,16,2,3,4,8,10,11,13,14,17,19,23,24,25,26,27,28,29,30,31,32,33,34,35,36,38,39,40,61,63,64};
inline constexpr const char* names[]={"Nenhum","AK-47","AWP","M4A1-S","Desert Eagle","M4A4","Dual Berettas","Five-SeveN","Glock","AUG","FAMAS","G3SG1","Galil","M249","MAC-10","P90","MP5-SD","UMP-45","XM1014","PP-Bizon","MAG-7","Negev","Sawed-Off","Tec-9","Zeus","P2000","MP7","MP9","Nova","P250","SCAR-20","SG 553","SSG 08","USP-S","CZ75","R8"};
inline constexpr int count=sizeof(definitions)/sizeof(definitions[0]);
static_assert(sizeof(names)/sizeof(names[0])==count);
}
