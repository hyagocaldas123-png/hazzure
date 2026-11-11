// Local, selected-model GLB viewer. CPU decoding is isolated from the render thread.
#pragma once
#include "ModelPreviewAsset.h"
#include "ModelPreviewCatalog.h"
#include "WeaponPreviewCatalog.h"
#include "SceneColorGrading.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <chrono>
namespace HazzureModelPreview {
class Loader {
    std::mutex mutex;std::condition_variable wake;std::thread thread;
    bool stopping{},pending{},loadFailed{};uint64_t generation{},completed{};
    std::string requested;std::filesystem::path path;std::shared_ptr<Asset> result;
    void Run(){
        for(;;){std::filesystem::path file;uint64_t ticket{};
            {std::unique_lock<std::mutex> guard(mutex);wake.wait(guard,[&]{return stopping||pending;});if(stopping)return;
                ticket=generation;pending=false;
                try{file=path;}catch(...){result.reset();completed=ticket;loadFailed=true;continue;}}
            std::shared_ptr<Asset> loaded;bool failed=false;
            try{loaded=LoadAsset(file);}catch(...){failed=true;}
            {std::lock_guard<std::mutex> guard(mutex);if(!stopping&&ticket==generation){result=std::move(loaded);completed=ticket;loadFailed=failed;}}
        }
    }
public:
    ~Loader(){Stop();}
    void Stop(){
        {std::lock_guard<std::mutex> guard(mutex);stopping=true;pending=false;++generation;result.reset();}
        wake.notify_all();if(thread.joinable())thread.join();
        std::lock_guard<std::mutex> guard(mutex);requested.clear();path.clear();completed=0;stopping=false;loadFailed=false;
    }
    std::shared_ptr<Asset> Request(const char* model,const std::filesystem::path& file){
        std::lock_guard<std::mutex> guard(mutex);
        if(!model||!*model||file.empty()){if(!requested.empty()){requested.clear();result.reset();++generation;pending=false;}return {};}
        if(requested!=model||path!=file){
            try{std::string next(model);std::filesystem::path nextPath(file);requested=std::move(next);path=std::move(nextPath);
                result.reset();completed=0;loadFailed=false;++generation;pending=true;
                if(!thread.joinable())thread=std::thread([this]{Run();});wake.notify_one();
            }catch(...){result.reset();pending=false;completed=generation;loadFailed=true;}
        }
        return completed==generation?result:std::shared_ptr<Asset>{};
    }
    const char* Status(){std::lock_guard<std::mutex> guard(mutex);return loadFailed?"Falha de memoria/arquivo do modelo":"Carregando geometria e materiais do modelo selecionado...";}
};
struct View {unsigned width=384,height=256;float yaw{},pitch=.08f,zoom=2.8f,time{};int clip=-1;};
inline bool ValidView(const View& v){return v.width>=64&&v.width<=1024&&v.height>=64&&v.height<=1024&&
    std::isfinite(v.yaw)&&std::isfinite(v.pitch)&&std::fabs(v.yaw)<100000&&std::fabs(v.pitch)<=1.3f&&
    std::isfinite(v.zoom)&&v.zoom>=1.6f&&v.zoom<=6&&std::isfinite(v.time)&&v.time>=0;}
struct GPUVertex {XMFLOAT3 p,n;XMFLOAT2 uv;uint32_t j[8];float w[8];};
struct DrawConstants {XMFLOAT4X4 bones[256],mvp,rotation;XMFLOAT4 colour,control;};
struct SavedState {
    HazzureSceneColor::PipelineState base;ID3D11DeviceContext* c;ID3D11Buffer* vertex{},*index{},*vsConstant{};
    UINT stride{},offset{},indexOffset{},first{},constants{};DXGI_FORMAT indexFormat{};
    explicit SavedState(ID3D11DeviceContext* context):base(context),c(context){
        c->IAGetVertexBuffers(0,1,&vertex,&stride,&offset);c->IAGetIndexBuffer(&index,&indexFormat,&indexOffset);
        if(base.c1)base.c1->VSGetConstantBuffers1(0,1,&vsConstant,&first,&constants);else c->VSGetConstantBuffers(0,1,&vsConstant);
    }
    ~SavedState(){if(base.changed){c->IASetVertexBuffers(0,1,&vertex,&stride,&offset);c->IASetIndexBuffer(index,indexFormat,indexOffset);
            if(base.c1)base.c1->VSSetConstantBuffers1(0,1,&vsConstant,&first,&constants);else c->VSSetConstantBuffers(0,1,&vsConstant);}
        Drop(vertex);Drop(index);Drop(vsConstant);}
};
class Renderer {
    struct Mesh {ID3D11Buffer* vertex{},*index{};UINT count{};};
    struct Texture {ID3D11ShaderResourceView* image{};};
    ID3D11Device* device{};ID3D11DeviceContext* context{};
    ID3D11VertexShader* vs{};ID3D11PixelShader* ps{};ID3D11InputLayout* layout{};ID3D11Buffer* constants{};
    ID3D11SamplerState* sampler{};ID3D11RasterizerState* raster{};ID3D11DepthStencilState* depthState{};ID3D11BlendState* blend{};
    ID3D11Texture2D* output{},*depth{};ID3D11RenderTargetView* target{};ID3D11DepthStencilView* depthTarget{};ID3D11ShaderResourceView* image{};
    unsigned width{},height{};bool failed{};const char* status="Modelo nao solicitado";
    std::shared_ptr<Asset> asset;std::vector<Mesh> meshes;std::vector<Texture> textures;
    void ClearModel(){for(auto& m:meshes){Drop(m.vertex);Drop(m.index);}meshes.clear();for(auto& t:textures)Drop(t.image);textures.clear();asset.reset();}
    bool AliasedElsewhere(){
        for(unsigned stage=0;stage<6;++stage){ID3D11ShaderResourceView* views[128]{};
            switch(stage){case 0:context->VSGetShaderResources(0,128,views);break;case 1:context->PSGetShaderResources(0,128,views);break;
                case 2:context->GSGetShaderResources(0,128,views);break;case 3:context->HSGetShaderResources(0,128,views);break;
                case 4:context->DSGetShaderResources(0,128,views);break;default:context->CSGetShaderResources(0,128,views);break;}
            bool found=false;for(unsigned i=0;i<128;++i){if(views[i]&&!(stage==1&&i==0)){ID3D11Resource* resource{};views[i]->GetResource(&resource);found=found||resource==output;Drop(resource);}Drop(views[i]);}
            if(found)return true;
        }return false;
    }
    bool Initialize(){
        if(vs&&ps)return true;if(!device||failed)return false;
        if(device->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0){failed=true;status="Previa requer DX11 feature level 11";return false;}
        static const char source[]=R"(
cbuffer Model:register(b0){row_major float4x4 bones[256];row_major float4x4 mvp;row_major float4x4 rotation;float4 colour;float4 control;};
Texture2D albedo:register(t0);SamplerState sampleAlbedo:register(s0);
struct V{float3 p:POSITION;float3 n:NORMAL;float2 uv:TEXCOORD0;uint4 j0:BLENDINDICES0;uint4 j1:BLENDINDICES1;float4 w0:BLENDWEIGHT0;float4 w1:BLENDWEIGHT1;};
struct P{float4 p:SV_POSITION;float3 n:NORMAL;float2 uv:TEXCOORD0;};
P VS(V v){float4 p=0;float3 n=0;[unroll]for(int i=0;i<4;i++){p+=mul(float4(v.p,1),bones[v.j0[i]])*v.w0[i];n+=mul(float4(v.n,0),bones[v.j0[i]]).xyz*v.w0[i];
p+=mul(float4(v.p,1),bones[v.j1[i]])*v.w1[i];n+=mul(float4(v.n,0),bones[v.j1[i]]).xyz*v.w1[i];}
P o;o.p=mul(p,mvp);o.n=mul(float4(n,0),rotation).xyz;o.uv=v.uv;return o;}
float4 PS(P p):SV_TARGET{float4 t=albedo.Sample(sampleAlbedo,p.uv)*colour;if(control.x>=0)clip(t.a-control.x);
float light=.28+.72*saturate(dot(normalize(p.n),normalize(float3(.35,.65,-.7))));float3 c=saturate(t.rgb*light);
c=lerp(c*12.92,1.055*pow(c,1./2.4)-.055,step(.0031308,c));return float4(c,1);}
)";
        ID3DBlob *v{},*p{},*error{};HRESULT hr=D3DCompile(source,sizeof(source)-1,nullptr,nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&v,&error);Drop(error);
        if(SUCCEEDED(hr))hr=D3DCompile(source,sizeof(source)-1,nullptr,nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&p,&error);Drop(error);
        if(SUCCEEDED(hr))hr=device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs);
        if(SUCCEEDED(hr))hr=device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps);
        D3D11_INPUT_ELEMENT_DESC elements[]={
            {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,offsetof(GPUVertex,p),D3D11_INPUT_PER_VERTEX_DATA,0},
            {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,offsetof(GPUVertex,n),D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,offsetof(GPUVertex,uv),D3D11_INPUT_PER_VERTEX_DATA,0},
            {"BLENDINDICES",0,DXGI_FORMAT_R32G32B32A32_UINT,0,offsetof(GPUVertex,j),D3D11_INPUT_PER_VERTEX_DATA,0},
            {"BLENDINDICES",1,DXGI_FORMAT_R32G32B32A32_UINT,0,offsetof(GPUVertex,j)+16,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"BLENDWEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,offsetof(GPUVertex,w),D3D11_INPUT_PER_VERTEX_DATA,0},
            {"BLENDWEIGHT",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,offsetof(GPUVertex,w)+16,D3D11_INPUT_PER_VERTEX_DATA,0}};
        if(SUCCEEDED(hr))hr=device->CreateInputLayout(elements,7,v->GetBufferPointer(),v->GetBufferSize(),&layout);Drop(v);Drop(p);
        D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(DrawConstants);bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        if(SUCCEEDED(hr))hr=device->CreateBuffer(&bd,nullptr,&constants);
        D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxLOD=D3D11_FLOAT32_MAX;
        if(SUCCEEDED(hr))hr=device->CreateSamplerState(&sd,&sampler);
        D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;
        if(SUCCEEDED(hr))hr=device->CreateRasterizerState(&rd,&raster);
        D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_LESS;
        if(SUCCEEDED(hr))hr=device->CreateDepthStencilState(&dd,&depthState);
        D3D11_BLEND_DESC bl{};bl.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
        if(SUCCEEDED(hr))hr=device->CreateBlendState(&bl,&blend);
        if(FAILED(hr)){Drop(vs);Drop(ps);Drop(layout);Drop(constants);Drop(sampler);Drop(raster);Drop(depthState);Drop(blend);failed=true;status="Falha ao criar renderizador de modelos";return false;}return true;
    }
    bool Targets(unsigned w,unsigned h){
        if(output&&width==w&&height==h)return true;ReleaseTargets();
        D3D11_TEXTURE2D_DESC td{};td.Width=w;td.Height=h;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;
        td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr=device->CreateTexture2D(&td,nullptr,&output);if(SUCCEEDED(hr))hr=device->CreateRenderTargetView(output,nullptr,&target);
        if(SUCCEEDED(hr))hr=device->CreateShaderResourceView(output,nullptr,&image);td.Format=DXGI_FORMAT_D32_FLOAT;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;
        if(SUCCEEDED(hr))hr=device->CreateTexture2D(&td,nullptr,&depth);if(SUCCEEDED(hr))hr=device->CreateDepthStencilView(depth,nullptr,&depthTarget);
        if(FAILED(hr)){ReleaseTargets();status="Falha nos alvos da previa";return false;}width=w;height=h;return true;
    }
    bool Upload(const std::shared_ptr<Asset>& value){
        if(asset==value)return true;ClearModel();asset=value;meshes.resize(asset->parts.size());textures.resize(asset->materials.size());
        HRESULT hr=S_OK;
        for(size_t i=0;i<asset->parts.size()&&SUCCEEDED(hr);++i){const auto& part=asset->parts[i];auto& m=meshes[i];
            static_assert(sizeof(GPUVertex)==sizeof(Vertex),"Vertex layout changed");
            D3D11_BUFFER_DESC d{};d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;d.ByteWidth=static_cast<UINT>(part.vertices.size()*sizeof(Vertex));
            D3D11_SUBRESOURCE_DATA init{part.vertices.data(),0,0};hr=device->CreateBuffer(&d,&init,&m.vertex);
            d.BindFlags=D3D11_BIND_INDEX_BUFFER;d.ByteWidth=static_cast<UINT>(part.indices.size()*4);init.pSysMem=part.indices.data();
            if(SUCCEEDED(hr))hr=device->CreateBuffer(&d,&init,&m.index);m.count=static_cast<UINT>(part.indices.size());
        }
        const unsigned char white[]={255,255,255,255};
        for(size_t i=0;i<textures.size()&&SUCCEEDED(hr);++i){const auto& pixels=asset->materials[i].pixels;D3D11_TEXTURE2D_DESC d{};
            d.Width=pixels.width?pixels.width:1;d.Height=pixels.height?pixels.height:1;d.MipLevels=d.ArraySize=1;d.SampleDesc.Count=1;
            d.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA init{pixels.rgba.empty()?white:pixels.rgba.data(),d.Width*4,0};ID3D11Texture2D* texture{};
            hr=device->CreateTexture2D(&d,&init,&texture);if(SUCCEEDED(hr))hr=device->CreateShaderResourceView(texture,nullptr,&textures[i].image);Drop(texture);
        }
        if(FAILED(hr)){ClearModel();status="Falha ao carregar malha/texturas na GPU";return false;}return true;
    }
public:
    ~Renderer(){SetDevice(nullptr);}
    void ReleaseTargets(){Drop(image);Drop(target);Drop(depthTarget);Drop(output);Drop(depth);width=height=0;}
    void SetDevice(ID3D11Device* d){if(device==d)return;ClearModel();ReleaseTargets();Drop(vs);Drop(ps);Drop(layout);Drop(constants);Drop(sampler);Drop(raster);Drop(depthState);Drop(blend);Drop(context);Drop(device);failed=false;
        device=d;if(device){device->AddRef();device->GetImmediateContext(&context);}status="Modelo nao solicitado";}
    const char* Status()const{return status;}
    ID3D11ShaderResourceView* Draw(const std::shared_ptr<Asset>& value,const View& view){
        if(!value){status="Carregando modelo selecionado...";return nullptr;}
        if(!value->error.empty()){status="Modelo GLB recusado";return nullptr;}
        if(!ValidView(view)){status="Parametros da previa invalidos";return nullptr;}
        if(!Initialize()||!Targets(view.width,view.height)||!Upload(value))return nullptr;
        if(AliasedElsewhere()){status="Previa em uso por outro shader";return nullptr;}
        SavedState saved(context);if(saved.base.streamOutput){status="Previa aguardando stream output";return nullptr;}
        ID3D11UnorderedAccessView* uav[8]{};context->OMGetRenderTargetsAndUnorderedAccessViews(0,nullptr,nullptr,0,8,uav);bool occupied=false;
        for(auto*& u:uav){occupied=occupied||u;Drop(u);}if(occupied){status="Previa aguardando UAV";return nullptr;}
        Animate(*asset,view.clip,view.time);
        const float radius=asset->radius;auto rot=XMMatrixRotationY(view.yaw)*XMMatrixRotationX(view.pitch);
        auto world=XMMatrixTranslation(-asset->center.x,-asset->center.y,-asset->center.z)*rot;
        auto camera=XMMatrixLookAtLH(XMVectorSet(0,0,-radius*view.zoom,1),XMVectorZero(),XMVectorSet(0,1,0,0));
        auto projection=XMMatrixPerspectiveFovLH(.75f,static_cast<float>(view.width)/view.height,std::max(.001f,radius*.01f),radius*20.f);
        DrawConstants data{};XMStoreFloat4x4(&data.mvp,world*camera*projection);XMStoreFloat4x4(&data.rotation,rot);
        saved.base.changed=true;context->SetPredication(nullptr,FALSE);context->OMSetRenderTargets(1,&target,depthTarget);
        context->OMSetBlendState(blend,nullptr,0xffffffffu);context->OMSetDepthStencilState(depthState,0);context->RSSetState(raster);
        D3D11_VIEWPORT viewport{0,0,static_cast<float>(width),static_cast<float>(height),0,1};context->RSSetViewports(1,&viewport);
        const float background[4]={.035f,.045f,.065f,1};context->ClearRenderTargetView(target,background);context->ClearDepthStencilView(depthTarget,D3D11_CLEAR_DEPTH,1,0);
        context->IASetInputLayout(layout);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);context->GSSetShader(nullptr,nullptr,0);context->HSSetShader(nullptr,nullptr,0);context->DSSetShader(nullptr,nullptr,0);
        context->VSSetConstantBuffers(0,1,&constants);context->PSSetConstantBuffers(0,1,&constants);context->PSSetSamplers(0,1,&sampler);
        for(size_t i=0;i<meshes.size();++i){const auto& part=asset->parts[i];unsigned count{};std::array<XMFLOAT4X4,256> palette{};
            if(!Palette(part,palette,count)){status="Paleta de animacao invalida";return nullptr;}memcpy(data.bones,palette.data(),sizeof(data.bones));
            const auto& material=asset->materials[part.material];memcpy(&data.colour,material.color,sizeof(data.colour));data.control={material.cutoff,0,0,0};
            D3D11_MAPPED_SUBRESOURCE mapped{};if(FAILED(context->Map(constants,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))){status="Falha de constante da previa";return nullptr;}
            memcpy(mapped.pData,&data,sizeof(data));context->Unmap(constants,0);UINT stride=sizeof(GPUVertex),offset=0;
            context->IASetVertexBuffers(0,1,&meshes[i].vertex,&stride,&offset);context->IASetIndexBuffer(meshes[i].index,DXGI_FORMAT_R32_UINT,0);
            auto* texture=textures[part.material].image;context->PSSetShaderResources(0,1,&texture);context->DrawIndexed(meshes[i].count,0,0);
        }
        ID3D11ShaderResourceView* empty=nullptr;context->PSSetShaderResources(0,1,&empty);status="Modelo e animacao reais / material glTF";return image;
    }
};
// Resident DLL: avoid joining a worker from CRT teardown while the loader lock is held.
// Explicit presentation shutdown still calls Stop; the OS reclaims the singleton at process exit.
inline Loader& GetLoader(){static auto* loader=new Loader;return *loader;}
inline Renderer renderer;
inline void SetDevice(ID3D11Device* device){if(!device)GetLoader().Stop();renderer.SetDevice(device);}
inline void ReleaseTargets(){renderer.ReleaseTargets();}
inline void Reset(){GetLoader().Stop();renderer.SetDevice(nullptr);}
inline std::shared_ptr<Asset> Request(const char* model,const std::filesystem::path& path){return GetLoader().Request(model,path);}
inline const char* LoadingStatus(){return GetLoader().Status();}
inline ID3D11ShaderResourceView* Render(const std::shared_ptr<Asset>& asset,const View& view){return renderer.Draw(asset,view);}
inline const char* Status(){return renderer.Status();}
}
