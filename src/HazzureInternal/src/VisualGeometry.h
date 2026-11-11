// Exact union of bounded axis-aligned fire-cell footprints; no convex hull.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace HazzureVisualGeometry {
struct Rect {float left{},bottom{},right{},top{},z{};};
struct Edge {float x0{},y0{},x1{},y1{},z{};};
struct Mesh {std::vector<Rect> fill;std::vector<Edge> outline;bool valid{};};
inline bool Valid(const Rect& r){return std::isfinite(r.left)&&std::isfinite(r.bottom)&&std::isfinite(r.right)&&
    std::isfinite(r.top)&&std::isfinite(r.z)&&r.left<r.right&&r.bottom<r.top;}
inline bool Build(const Rect* rectangles,size_t count,Mesh& output){
    output.fill.clear();output.outline.clear();output.valid=false;
    if(!rectangles||!count||count>64)return false;
    for(size_t i=0;i<count;++i)if(!Valid(rectangles[i]))return false;
    std::array<bool,64> used{};
    for(size_t plane=0;plane<count;++plane){
        if(used[plane])continue;
        // Different elevations remain separate rather than connecting across stairs.
        const float z=rectangles[plane].z;std::vector<float> xs,ys;std::vector<Rect> group;
        for(size_t i=plane;i<count;++i)if(!used[i]&&rectangles[i].z==z){
            used[i]=true;group.push_back(rectangles[i]);xs.push_back(rectangles[i].left);xs.push_back(rectangles[i].right);
            ys.push_back(rectangles[i].bottom);ys.push_back(rectangles[i].top);
        }
        auto unique=[](auto& v){std::sort(v.begin(),v.end());v.erase(std::unique(v.begin(),v.end()),v.end());};
        unique(xs);unique(ys);const size_t w=xs.size()-1,h=ys.size()-1;
        std::array<uint8_t,127*127> cells{};
        for(const auto& r:group){
            auto x0=std::lower_bound(xs.begin(),xs.end(),r.left)-xs.begin(),x1=std::lower_bound(xs.begin(),xs.end(),r.right)-xs.begin();
            auto y0=std::lower_bound(ys.begin(),ys.end(),r.bottom)-ys.begin(),y1=std::lower_bound(ys.begin(),ys.end(),r.top)-ys.begin();
            for(auto y=y0;y<y1;++y)for(auto x=x0;x<x1;++x)cells[y*w+x]=1;
        }
        auto occupied=[&](int x,int y){return x>=0&&y>=0&&x<static_cast<int>(w)&&y<static_cast<int>(h)&&cells[y*w+x]!=0;};
        for(size_t y=0;y<h;++y)for(size_t x=0;x<w;){
            if(!cells[y*w+x]){++x;continue;}size_t first=x;while(x<w&&cells[y*w+x])++x;
            bool merged=false;
            for(auto it=output.fill.rbegin();it!=output.fill.rend();++it)if(it->z==z&&it->left==xs[first]&&it->right==xs[x]&&it->top==ys[y]){
                it->top=ys[y+1];merged=true;break;
            }
            if(!merged)output.fill.push_back({xs[first],ys[y],xs[x],ys[y+1],z});
        }
        // Only occupied/empty transitions form an edge. Internal grid lines vanish.
        for(size_t y=0;y<=h;++y)for(size_t x=0;x<w;){
            int sign=occupied(static_cast<int>(x),static_cast<int>(y)-1)-occupied(static_cast<int>(x),static_cast<int>(y));
            if(!sign){++x;continue;}size_t first=x++;
            while(x<w&&occupied(static_cast<int>(x),static_cast<int>(y)-1)-occupied(static_cast<int>(x),static_cast<int>(y))==sign)++x;
            output.outline.push_back({xs[first],ys[y],xs[x],ys[y],z});
        }
        for(size_t x=0;x<=w;++x)for(size_t y=0;y<h;){
            int sign=occupied(static_cast<int>(x)-1,static_cast<int>(y))-occupied(static_cast<int>(x),static_cast<int>(y));
            if(!sign){++y;continue;}size_t first=y++;
            while(y<h&&occupied(static_cast<int>(x)-1,static_cast<int>(y))-occupied(static_cast<int>(x),static_cast<int>(y))==sign)++y;
            output.outline.push_back({xs[x],ys[first],xs[x],ys[y],z});
        }
    }
    output.valid=true;return true;
}
struct KillGate {
    struct Entry {uint32_t victim{};uint64_t epoch{};};std::array<Entry,128> entries{};size_t cursor{};
    void Clear(){entries={};cursor=0;}
    bool Accept(uint32_t victim,uint32_t attacker,uint32_t local,bool enemy,uint64_t epoch){
        if(!victim||!attacker||attacker!=local||victim==attacker||!enemy)return false;
        for(const auto& e:entries)if(e.victim==victim&&e.epoch==epoch)return false;
        entries[cursor++%entries.size()]={victim,epoch};return true;
    }
};
}
