// Included inside HazzureUI after SkinSelections.h. Images identify the selected catalogue item.
#include "AgentPreviewCatalog.h"
struct EquipmentPreview {int definition{},paint{},seed{};float wear{};const char* name="Nenhuma arma selecionada";const char* finish="Sem pintura";};
static EquipmentPreview SelectedEquipmentPreview(int type=0,Shared* shared=nullptr){
    std::array<SkinConfig,64> snapshot{};const SkinConfig* items=g_cfg.skins;
    if(shared){LONG before=InterlockedCompareExchange(&shared->revision,0,0);
        if(!(before&1)){memcpy(snapshot.data(),shared->skins,sizeof(shared->skins));
            if(before==InterlockedCompareExchange(&shared->revision,0,0))items=snapshot.data();}}
    EquipmentPreview value{};int slot=type==1?0:type==2?1:-1;
    if(type==0){value.definition=HazzureWeapons::definitions[std::clamp(g_cfg.weaponIndex,0,HazzureWeapons::count-1)];
        value.name=HazzureWeapons::names[std::clamp(g_cfg.weaponIndex,0,HazzureWeapons::count-1)];
        for(int i=2;i<64;++i)if(items[i].enabled&&items[i].definition==value.definition){slot=i;break;}}
    if(slot>=0){const auto& skin=items[slot];value.definition=type==1?skin.model:skin.definition;value.paint=skin.paint;value.seed=skin.seed;value.wear=skin.wear;
        if(type==1)value.name="Faca selecionada";if(type==2)value.name="Luvas selecionadas";}
    else if(value.definition)value.paint=FirstPaintFor(value.definition);
    for(const auto& choice:kSkins)if(choice.def==value.definition&&choice.kit==value.paint){value.finish=choice.name;break;}
    return value;
}
static bool DrawCatalogueArtwork(const char* asset,float height,bool portrait=false){
    float width=std::max(80.f,g_cleanCardContentW);auto position=ImGui::GetCursorScreenPos();
    auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(position,{position.x+width,position.y+height},IM_COL32(12,16,23,255),5);
    auto texture=HazzurePreviewAssets::Get(asset);
    if(texture.view&&texture.width&&texture.height){
        float scale=std::min((width-12)/texture.width,(height-8)/texture.height);
        ImVec2 imageSize(texture.width*scale,texture.height*scale),minimum{position.x+(width-imageSize.x)*.5f,position.y+(height-imageSize.y)*.5f};
        draw->AddImage(reinterpret_cast<ImTextureID>(texture.view),minimum,{minimum.x+imageSize.x,minimum.y+imageSize.y});
    }else if(portrait){
        ImVec2 center{position.x+width*.5f,position.y+height*.5f};
        draw->AddCircle({center.x,center.y-20},18,IM_COL32(95,110,130,255),24,2);
        draw->AddBezierCubic({center.x-44,center.y+45},{center.x-44,center.y-3},{center.x+44,center.y-3},{center.x+44,center.y+45},IM_COL32(95,110,130,255),2);
    }else{
        draw->AddText({position.x+14,position.y+height*.5f-6},IM_COL32(140,155,176,255),"Imagem de catalogo indisponivel");
    }
    ImGui::Dummy({width,height});return texture.view!=nullptr;
}
static const AgentPreviewChoice& SelectedAgentPreview(bool ct,int index){
    return ct?kPreviewAgentsCT[std::clamp(index,0,static_cast<int>(IM_ARRAYSIZE(kPreviewAgentsCT))-1)]:
        kPreviewAgentsT[std::clamp(index,0,static_cast<int>(IM_ARRAYSIZE(kPreviewAgentsT))-1)];
}
static int g_previewAgentTeam=0;
static bool g_agentPreview3D=true;
struct EquipmentModelView {
    std::string selected;float yaw=3.14159f,pitch=.08f,zoom=2.8f;int clip{};
    bool rotate=true,animate=true;
};
static int g_equipmentPreviewFrame=-1;
static void FinishEquipmentPreviewFrame(){
    if(g_equipmentPreviewFrame!=ImGui::GetFrameCount())HazzureModelPreview::Request(nullptr,{});
}
static bool DrawEquipmentModel3D(const char* key,const char* relative,float height,EquipmentModelView& state,bool animated){
    auto file=relative?HazzurePreviewAssets::Find(relative):std::filesystem::path{};
    if(!key||!*key||file.empty()){HazzureModelPreview::Request(nullptr,{});return false;}
    if(state.selected!=key){state.selected=key;state.clip=0;state.yaw=animated?3.14159f:.85f;state.pitch=.08f;state.zoom=2.8f;}
    g_equipmentPreviewFrame=ImGui::GetFrameCount();
    auto asset=HazzureModelPreview::Request(key,file);
    if(!asset){ImGui::TextDisabled("%s",HazzureModelPreview::LoadingStatus());return false;}
    if(!asset->error.empty()){ImGui::TextWrapped("%s",asset->error.c_str());return false;}
    float width=std::max(80.f,g_cleanCardContentW);
    if(state.rotate)state.yaw=std::remainder(state.yaw+std::min(ImGui::GetIO().DeltaTime,.1f)*.6f,6.283185307f);
    HazzureModelPreview::View view;view.width=static_cast<unsigned>(std::clamp(width,64.f,1024.f));
    view.height=static_cast<unsigned>(std::clamp(height,64.f,1024.f));view.yaw=state.yaw;view.pitch=state.pitch;
    view.zoom=state.zoom;view.clip=animated&&state.animate?state.clip:-1;view.time=static_cast<float>(ImGui::GetTime());
    auto* texture=HazzureModelPreview::Render(asset,view);
    if(!texture){ImGui::TextWrapped("%s",HazzureModelPreview::Status());return false;}
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),{width,height});
    if(ImGui::IsItemHovered()){
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
            state.yaw=std::remainder(state.yaw+ImGui::GetIO().MouseDelta.x*.015f,6.283185307f);
            state.pitch=std::clamp(state.pitch+ImGui::GetIO().MouseDelta.y*.015f,-.6f,.6f);
        }
        state.zoom=std::clamp(state.zoom-ImGui::GetIO().MouseWheel*.2f,1.6f,6.f);
    }
    CleanToggle("Auto-Rotacionar",&state.rotate);
    CleanSliderFloat("Rotacao Horizontal",&state.yaw,-3.14159f,3.14159f,"%.2f rad");
    CleanSliderFloat("Zoom da Previa",&state.zoom,1.6f,6.f,"%.2f");
    if(animated&&!asset->clips.empty()){
        CleanToggle("Animar Modelo da Previa",&state.animate);const char* names[16]{};
        for(size_t i=0;i<asset->clips.size()&&i<16;++i){auto* clip=asset->clips[i].c_str();auto* slash=strrchr(clip,'/');names[i]=slash?slash+1:clip;}
        state.clip=std::clamp(state.clip,0,static_cast<int>(asset->clips.size())-1);
        CleanCombo("Clip Real da Previa",&state.clip,names,static_cast<int>(asset->clips.size()));
    }
    ImGui::TextWrapped(animated?"Geometria, texturas e animacao do modelo selecionado. Material glTF aproximado; nao simula pintura, wear, seed ou tecido da engine.":
        "Modelo base 3D da arma selecionada, com material glTF aproximado. Nao simula pintura, wear, seed ou anexos.");
    return true;
}
static bool DrawModelPreview3D(const AgentPreviewChoice& agent,float height){
    static EquipmentModelView state;std::string key=std::string("agent:")+agent.model;
    return DrawEquipmentModel3D(key.c_str(),HazzureModelPreview::FindModel(agent.model),height,state,true);
}
static void DrawWeaponSelectionPreview(Shared* shared=nullptr){
    BeginCleanCard("Previa do Item","MODELO BASE / CATALOGO");
    static int type=0;const char* types[]={"Arma","Faca","Luvas"};CleanCombo("Visualizar",&type,types,3);
    auto item=SelectedEquipmentPreview(type,shared);char asset[80]{};sprintf_s(asset,"skins/%d_%d.png",item.definition,item.paint);
    static bool preview3d=true;static EquipmentModelView state;
    CleanToggle("Visualizacao 3D do modelo base",&preview3d);
    const auto* entry=type==0?HazzureWeaponPreview::Find(item.definition):nullptr;
    bool drawn=false;
    if(preview3d&&entry)drawn=DrawEquipmentModel3D(entry->key,entry->asset,220.f,state,false);
    else HazzureModelPreview::Request(nullptr,{});
    if(preview3d&&!drawn)ImGui::TextDisabled("Modelo base indisponivel ou carregando; catalogo preservado.");
    ImGui::TextDisabled("Catalogo da pintura selecionada");
    bool image=DrawCatalogueArtwork(asset,drawn?96.f:174.f);
    ImGui::TextWrapped("%s",item.name);ImGui::TextWrapped("%s",item.finish);
    ImGui::TextDisabled("Modelo %d  /  pintura %d",item.definition,item.paint);
    ImGui::TextDisabled("Wear %.6f  /  seed %d",item.wear,item.seed);
    ImGui::Separator();ImGui::TextWrapped(image?"Imagem do item selecionado. Wear, seed e anexos sao conferidos na arma em jogo.":"O item continua selecionado. Instale os assets de catalogo para ver sua imagem.");
    EndCleanCard();
}
static void DrawAgentSelectionPreview(Shared* shared){
    BeginCleanCard("Previa do Agente","CT / TR");
    const char* sides[]={"Contra-Terrorista","Terrorista"};CleanCombo("Equipe da Previa",&g_previewAgentTeam,sides,2);
    bool ct=g_previewAgentTeam==0;
    int index=shared?static_cast<int>(InterlockedCompareExchange(ct?&shared->agentCT:&shared->agentT,0,0)):(ct?g_cfg.agentCT:g_cfg.agentT);
    const auto& agent=SelectedAgentPreview(ct,index);
    CleanToggle("Visualizacao 3D do Agente",&g_agentPreview3D);
    bool drawn=g_agentPreview3D&&DrawModelPreview3D(agent,234.f);
    if(!g_agentPreview3D)HazzureModelPreview::Request(nullptr,{});
    if(!drawn){
        bool image=DrawCatalogueArtwork(agent.asset,234,true);
        ImGui::TextWrapped(image?"Retrato de catalogo do agente selecionado.":"Sem retrato especifico para esta selecao.");
        if(g_agentPreview3D)ImGui::TextDisabled("Previa 3D indisponivel ou carregando; retrato preservado.");
    }
    ImGui::TextWrapped("%s",agent.name);
    if(*agent.model){const char* file=strrchr(agent.model,'/');ImGui::TextWrapped("%s",file?file+1:agent.model);}
    else ImGui::TextWrapped(index==0?"Mantem o agente padrao do mapa.":"Nao ha recurso completo para este modelo.");
    EndCleanCard();
}
static void DrawESPSelectionPreview(){
    BeginCleanCard("Previa do ESP","EXEMPLO");
    auto position=ImGui::GetCursorScreenPos();float width=g_cleanCardContentW;auto* draw=ImGui::GetWindowDrawList();
    float cx=position.x+width*.5f,top=position.y+30,bottom=position.y+235;
    draw->AddRectFilled(position,{position.x+width,position.y+270},IM_COL32(11,15,21,255),4);
    ImU32 colour=Float4ToImU32(g_cfg.colEnemy);float left=cx-49,right=cx+49;
    if(g_cfg.espBoxes){if(g_cfg.espBoxType==0)draw->AddRect({left,top},{right,bottom},colour,0,0,1.4f);
        else {for(float x:{left,right})for(float y:{top,bottom}){
            draw->AddLine({x,y},{x+(x==left?17:-17),y},colour,1.4f);draw->AddLine({x,y},{x,y+(y==top?17:-17)},colour,1.4f);}}}
    if(g_cfg.espSkeleton){auto skeleton=Float4ToImU32(g_cfg.colSkeleton);
        draw->AddCircle({cx,top+22},12,skeleton,20,1.5f);draw->AddLine({cx,top+34},{cx,bottom-63},skeleton,1.5f);
        draw->AddLine({cx,top+55},{cx-31,top+104},skeleton,1.5f);draw->AddLine({cx,top+55},{cx+31,top+104},skeleton,1.5f);
        draw->AddLine({cx,bottom-63},{cx-24,bottom-4},skeleton,1.5f);draw->AddLine({cx,bottom-63},{cx+24,bottom-4},skeleton,1.5f);}
    if(g_cfg.espHealth){draw->AddRectFilled({left-8,top},{left-4,bottom},IM_COL32(35,42,48,255));
        draw->AddRectFilled({left-8,top+(bottom-top)*.27f},{left-4,bottom},IM_COL32(95,215,135,255));}
    auto centered=[&](const char* text,float y){draw->AddText({cx-ImGui::CalcTextSize(text).x*.5f,y},IM_COL32(220,228,240,255),text);};
    if(g_cfg.espName)centered("Jogador / exemplo",top-21);
    if(g_cfg.espWeapon)centered("AK-47",bottom+7);
    if(g_cfg.espDistance)centered("18 m",bottom+23);
    ImGui::Dummy({width,270});ImGui::TextWrapped("Exemplo sintetico para conferir cores e elementos. Nao representa uma entidade detectada.");EndCleanCard();
}
