// Exact exported base-weapon assets. Paint/wear/seed remain catalogue/game data.
#pragma once
namespace HazzureWeaponPreview {
struct Entry {int definition;const char* asset;const char* key;};
inline constexpr Entry Catalogue[]={
    {7,"models/weapons/7.glb","weapon:7"},
    {9,"models/weapons/9.glb","weapon:9"},
    {60,"models/weapons/60.glb","weapon:60"},
    {1,"models/weapons/1.glb","weapon:1"},
    {16,"models/weapons/16.glb","weapon:16"},
    {2,"models/weapons/2.glb","weapon:2"},
    {3,"models/weapons/3.glb","weapon:3"},
    {4,"models/weapons/4.glb","weapon:4"},
    {8,"models/weapons/8.glb","weapon:8"},
    {10,"models/weapons/10.glb","weapon:10"},
    {11,"models/weapons/11.glb","weapon:11"},
    {13,"models/weapons/13.glb","weapon:13"},
    {14,"models/weapons/14.glb","weapon:14"},
    {17,"models/weapons/17.glb","weapon:17"},
    {19,"models/weapons/19.glb","weapon:19"},
    {23,"models/weapons/23.glb","weapon:23"},
    {24,"models/weapons/24.glb","weapon:24"},
    {25,"models/weapons/25.glb","weapon:25"},
    {26,"models/weapons/26.glb","weapon:26"},
    {27,"models/weapons/27.glb","weapon:27"},
    {28,"models/weapons/28.glb","weapon:28"},
    {29,"models/weapons/29.glb","weapon:29"},
    {30,"models/weapons/30.glb","weapon:30"},
    {31,"models/weapons/31.glb","weapon:31"},
    {32,"models/weapons/32.glb","weapon:32"},
    {33,"models/weapons/33.glb","weapon:33"},
    {34,"models/weapons/34.glb","weapon:34"},
    {35,"models/weapons/35.glb","weapon:35"},
    {36,"models/weapons/36.glb","weapon:36"},
    {38,"models/weapons/38.glb","weapon:38"},
    {39,"models/weapons/39.glb","weapon:39"},
    {40,"models/weapons/40.glb","weapon:40"},
    {61,"models/weapons/61.glb","weapon:61"},
    {63,"models/weapons/63.glb","weapon:63"},
    {64,"models/weapons/64.glb","weapon:64"},
};
inline const Entry* Find(int definition){for(const auto& entry:Catalogue)if(entry.definition==definition)return &entry;return nullptr;}
}
