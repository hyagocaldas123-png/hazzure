// Included after OverlayConfig. All animation uses event age, never frame count.
static ImU32 VisualColour(const float* color,float alpha=1.f){
    return ImGui::ColorConvertFloat4ToU32({color[0],color[1],color[2],std::clamp(color[3]*alpha,0.f,1.f)});
}
static ImU32 VisualAlpha(ImU32 color,float alpha){return (color&0xffffffu)|
    (static_cast<ImU32>(((color>>24)&255)*std::clamp(alpha,0.f,1.f))<<24);}
static void VisualLine(ImDrawList* draw,Vec3 a,Vec3 b,const Matrix4& matrix,int width,int height,
                       ImU32 color,float thickness,bool glow=false){
    HazzureGame::Vec2 sa{},sb{};
    if(!HazzureCombat::ProjectSegment(a,b,matrix,width,height,sa,sb))return;
    if(glow){draw->AddLine({sa.x,sa.y},{sb.x,sb.y},VisualAlpha(color,.10f),thickness+7.f);
        draw->AddLine({sa.x,sa.y},{sb.x,sb.y},VisualAlpha(color,.22f),thickness+3.f);}
    draw->AddLine({sa.x,sa.y},{sb.x,sb.y},color,thickness);
}
// Clip in homogeneous space before perspective divide; preserve near-plane and
// viewport intersections rather than connecting a missing corner to a valid one.
static int VisualPolygon(const Vec3* points,int count,const Matrix4& matrix,int width,int height,ImVec2* result){
    if(!points||count<3||count>4||width<=0||height<=0)return 0;
    struct Clip {float x,y,w;};std::array<Clip,16> input{},output{};
    for(int i=0;i<count;++i){if(!HazzureGame::Finite(points[i]))return 0;const auto& p=points[i];const auto& m=matrix.m;
        input[i]={m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z+m[0][3],
                  m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z+m[1][3],
                  m[3][0]*p.x+m[3][1]*p.y+m[3][2]*p.z+m[3][3]};
        if(!std::isfinite(input[i].x)||!std::isfinite(input[i].y)||!std::isfinite(input[i].w))return 0;
    }
    for(int plane=0;plane<5&&count>=3;++plane){
        auto distance=[&](const Clip& p){switch(plane){case 0:return p.w-.01f;case 1:return p.x+p.w;
            case 2:return p.w-p.x;case 3:return p.y+p.w;default:return p.w-p.y;}};
        int next=0;
        for(int i=0;i<count;++i){auto a=input[i],b=input[(i+1)%count];float da=distance(a),db=distance(b);
            if(da>=0){if(next==16)return 0;output[next++]=a;}
            if((da>=0)!=(db>=0)){float t=da/(da-db);if(next==16)return 0;
                output[next++]={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.w+(b.w-a.w)*t};}
        }
        count=next;input=output;
    }
    if(count<3)return 0;
    for(int i=0;i<count;++i){const auto& p=input[i];
        result[i]={std::clamp((1.f+p.x/p.w)*width*.5f,0.f,static_cast<float>(width)),
                   std::clamp((1.f-p.y/p.w)*height*.5f,0.f,static_cast<float>(height))};}
    return count;
}
static void VisualFill(ImDrawList* draw,const Vec3* points,const Matrix4& matrix,int width,int height,ImU32 color){
    ImVec2 projected[16]{};int count=VisualPolygon(points,4,matrix,width,height,projected);if(count<3)return;
    auto flags=draw->Flags;draw->Flags&=~ImDrawListFlags_AntiAliasedFill;
    draw->AddConvexPolyFilled(projected,count,color);draw->Flags=flags;
}
static void DrawVisualKill(ImDrawList* draw,const HazzureCombat::Event& event,const Matrix4& matrix,int width,int height,float age){
    float progress=std::clamp(age/g_cfg.killEffectDuration,0.f,1.f),fade=(1-progress)*(1-progress)*g_cfg.killEffectIntensity;
    if(fade<=0)return;
    ImU32 color=VisualColour(g_cfg.colKillEffect,fade);float radius=g_cfg.killEffectSize*(.2f+.8f*progress);
    for(int i=0;i<32;++i){float a=i*6.283185307f/32,b=(i+1)*6.283185307f/32;
        Vec3 p{event.point.x+std::cos(a)*radius,event.point.y+std::sin(a)*radius,event.point.z+progress*8.f};
        Vec3 q{event.point.x+std::cos(b)*radius,event.point.y+std::sin(b)*radius,event.point.z+progress*8.f};
        VisualLine(draw,p,q,matrix,width,height,color,1.5f,true);
    }
    for(int i=0;i<12;++i){float angle=(i+((event.target&31)*.03125f))*2.39996323f;
        float spread=radius*(.6f+.04f*i);Vec3 p{event.point.x+std::cos(angle)*spread,event.point.y+std::sin(angle)*spread,
            event.point.z+g_cfg.killEffectSize*(progress*(.6f+.05f*i)-progress*progress*.45f)};
        Vec3 q=p;q.z-=3.f+3.f*(1-progress);VisualLine(draw,p,q,matrix,width,height,color,1.5f,true);
    }
}
