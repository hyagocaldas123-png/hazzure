// Uses installed Windows typefaces; never changes system fonts or downloads fonts.
#pragma once
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <string>
#include "../../HazzurePanel/third_party/imgui/imgui.h"
namespace HazzureTypography {
struct Settings {int family{};float menu=15,esp=13;};
inline bool Valid(Settings s){return s.family>=0&&s.family<=2&&std::isfinite(s.menu)&&s.menu>=12&&s.menu<=18&&std::isfinite(s.esp)&&s.esp>=11&&s.esp<=18;}
inline Settings current{};inline bool configured{};inline ImFont *body{},*heading{},*overlay{},*brand{};
inline const char* families[]={"Segoe UI","Tahoma","Arial"};
inline bool Changed(Settings s){return !configured||current.family!=s.family||current.menu!=s.menu||current.esp!=s.esp;}
inline ImFont* Load(const std::string& file,float size){
    ImFontConfig config{};config.OversampleH=3;config.OversampleV=2;config.PixelSnapH=true;
    auto attributes=GetFileAttributesA(file.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY))
        return ImGui::GetIO().Fonts->AddFontFromFileTTF(file.c_str(),size,&config,ImGui::GetIO().Fonts->GetGlyphRangesDefault());
    config.SizePixels=size;return ImGui::GetIO().Fonts->AddFontDefault(&config);
}
inline bool Build(Settings s){
    if(!ImGui::GetCurrentContext()||!Valid(s))return false;
    auto& io=ImGui::GetIO();io.Fonts->Clear();char windows[MAX_PATH]{};GetWindowsDirectoryA(windows,MAX_PATH);
    std::string root=std::string(windows)+"/Fonts/";
    const char* semibold[]={"seguisb.ttf","tahomabd.ttf","arialbd.ttf"};
    const char* bold[]={"segoeuib.ttf","tahomabd.ttf","arialbd.ttf"};
    const char* normal[]={"segoeui.ttf","tahoma.ttf","arial.ttf"};
    body=Load(root+semibold[s.family],s.menu);
    if(!body)body=Load(root+normal[s.family],s.menu);
    heading=Load(root+bold[s.family],s.menu+1.5f);
    brand=Load(root+bold[s.family],s.menu+7.0f);
    overlay=Load(root+semibold[s.family],s.esp);
    if(!overlay)overlay=Load(root+normal[s.family],s.esp);
    io.FontDefault=body;current=s;configured=body&&heading&&overlay&&brand;
    return configured&&io.Fonts->Build();
}
inline void Reset(){configured=false;body=heading=overlay=brand=nullptr;}
struct Scope {bool pushed{};explicit Scope(ImFont* font){if(font){ImGui::PushFont(font);pushed=true;}}~Scope(){if(pushed)ImGui::PopFont();}};
struct Palette {ImVec4 window,sidebar,card,border,text,muted,accent,hover,active;};
inline Palette Colors(int theme,const float* accent){
    (void)theme;
    Palette p{{.067f,.067f,.078f,1},{.047f,.047f,.055f,1},{.090f,.090f,.106f,1},{.165f,.165f,.196f,1},{1.f,1.f,1.f,1},{.80f,.81f,.84f,1},{1.f,1.f,1.f,1},{.22f,.22f,.26f,1},{.30f,.30f,.35f,1}};
    if(accent&&std::isfinite(accent[0])&&std::isfinite(accent[1])&&std::isfinite(accent[2]))p.accent={std::clamp(accent[0],.12f,1.f),std::clamp(accent[1],.12f,1.f),std::clamp(accent[2],.12f,1.f),1};
    p.active={p.accent.x*.32f+.05f,p.accent.y*.32f+.05f,p.accent.z*.32f+.05f,1};p.hover={p.active.x*.8f,p.active.y*.8f,p.active.z*.8f,1};return p;
}
}
