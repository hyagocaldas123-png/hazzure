// Scene-only DX11 pass. No engine offsets, no modification of overlay colours.
#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#pragma comment(lib,"d3dcompiler.lib")
namespace HazzureSceneColor {
struct Settings {bool enabled{};float saturation=1,contrast=1,temperature=0,tint=0;int curve=0;};
inline bool Valid(const Settings& s){return std::isfinite(s.saturation)&&s.saturation>=0&&s.saturation<=2&&
    std::isfinite(s.contrast)&&s.contrast>=.5f&&s.contrast<=2&&std::isfinite(s.temperature)&&std::fabs(s.temperature)<=1&&
    std::isfinite(s.tint)&&std::fabs(s.tint)<=1&&s.curve>=0&&s.curve<=1;}
inline bool Active(const Settings& s){return s.enabled&&Valid(s)&&(s.saturation!=1||s.contrast!=1||s.temperature!=0||s.tint!=0||s.curve!=0);}
template<class T> inline void Release(T*& value){if(value){value->Release();value=nullptr;}}
struct PipelineState {
    ID3D11DeviceContext* c;ID3D11DeviceContext1* c1{};bool changed{};UINT firstConstant{},constantCount{};
    ID3D11RenderTargetView* rt[8]{};ID3D11DepthStencilView* ds{};
    ID3D11BlendState* blend{};FLOAT factor[4]{};UINT mask{};
    ID3D11DepthStencilState* depth{};UINT stencil{};ID3D11RasterizerState* raster{};
    D3D11_VIEWPORT view[16]{};UINT views=16;D3D11_RECT scissors[16]{};UINT scissorCount=16;
    ID3D11InputLayout* layout{};D3D11_PRIMITIVE_TOPOLOGY topology{};
    ID3D11VertexShader* vs{};ID3D11PixelShader* ps{};ID3D11GeometryShader* gs{};
    ID3D11HullShader* hs{};ID3D11DomainShader* dsShader{};
    ID3D11ClassInstance* vc[256]{},*pc[256]{},*gc[256]{},*hc[256]{},*dc[256]{};
    UINT vn=256,pn=256,gn=256,hn=256,dn=256;
    ID3D11ShaderResourceView* resource{};ID3D11SamplerState* sampler{};ID3D11Buffer* constant{};
    ID3D11Buffer* streamBuffers[4]{};bool streamOutput{};
    ID3D11Predicate* predicate{};BOOL predicateValue{};
    explicit PipelineState(ID3D11DeviceContext* context):c(context){
        c->OMGetRenderTargets(8,rt,&ds);c->OMGetBlendState(&blend,factor,&mask);c->OMGetDepthStencilState(&depth,&stencil);
        c->RSGetState(&raster);c->RSGetViewports(&views,view);c->RSGetScissorRects(&scissorCount,scissors);
        c->IAGetInputLayout(&layout);c->IAGetPrimitiveTopology(&topology);
        c->VSGetShader(&vs,vc,&vn);c->PSGetShader(&ps,pc,&pn);c->GSGetShader(&gs,gc,&gn);
        c->HSGetShader(&hs,hc,&hn);c->DSGetShader(&dsShader,dc,&dn);
        c->PSGetShaderResources(0,1,&resource);c->PSGetSamplers(0,1,&sampler);
        if(SUCCEEDED(c->QueryInterface(__uuidof(ID3D11DeviceContext1),reinterpret_cast<void**>(&c1))))c1->PSGetConstantBuffers1(0,1,&constant,&firstConstant,&constantCount);
        else c->PSGetConstantBuffers(0,1,&constant);
        c->GetPredication(&predicate,&predicateValue);
        // Stream-output offsets cannot be queried. Refuse this uncommon state rather
        // than emitting our fullscreen vertices into an application's SO buffer.
        c->SOGetTargets(4,streamBuffers);for(auto* b:streamBuffers)streamOutput=streamOutput||b!=nullptr;
    }
    ~PipelineState(){
        if(changed){
        c->OMSetRenderTargets(8,rt,ds);c->OMSetBlendState(blend,factor,mask);c->OMSetDepthStencilState(depth,stencil);
        c->RSSetState(raster);c->RSSetViewports(views,view);c->RSSetScissorRects(scissorCount,scissors);
        c->IASetInputLayout(layout);c->IASetPrimitiveTopology(topology);
        c->VSSetShader(vs,vc,vn);c->PSSetShader(ps,pc,pn);c->GSSetShader(gs,gc,gn);
        c->HSSetShader(hs,hc,hn);c->DSSetShader(dsShader,dc,dn);
        c->PSSetShaderResources(0,1,&resource);c->PSSetSamplers(0,1,&sampler);
        if(c1)c1->PSSetConstantBuffers1(0,1,&constant,&firstConstant,&constantCount);else c->PSSetConstantBuffers(0,1,&constant);
        c->SetPredication(predicate,predicateValue);
        }
        for(auto*& r:rt)Release(r);Release(ds);Release(blend);Release(depth);Release(raster);Release(layout);
        Release(vs);Release(ps);Release(gs);Release(hs);Release(dsShader);Release(resource);Release(sampler);Release(constant);
        for(UINT i=0;i<vn;++i)Release(vc[i]);for(UINT i=0;i<pn;++i)Release(pc[i]);for(UINT i=0;i<gn;++i)Release(gc[i]);
        for(UINT i=0;i<hn;++i)Release(hc[i]);for(UINT i=0;i<dn;++i)Release(dc[i]);for(auto*& b:streamBuffers)Release(b);Release(predicate);Release(c1);
    }
};
class Pass {
    ID3D11Device* failedFor{};
    ID3D11Device* device{};ID3D11VertexShader* vs{};ID3D11PixelShader* ps{};
    ID3D11Buffer* parameters{};ID3D11SamplerState* sampler{};ID3D11RasterizerState* raster{};
    ID3D11DepthStencilState* depth{};ID3D11BlendState* blend{};
    ID3D11Texture2D* copy{};ID3D11ShaderResourceView* image{};
    UINT width{},height{};DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN,viewFormat=DXGI_FORMAT_UNKNOWN;
    UINT rejectedWidth{},rejectedHeight{};DXGI_FORMAT rejectedFormat=DXGI_FORMAT_UNKNOWN,rejectedView=DXGI_FORMAT_UNKNOWN;
    bool failed{};const char* status="Desligada";
    static bool SourceBoundForReading(ID3D11DeviceContext* context,ID3D11Resource* source,UINT uavSlots){
        // OMSetRenderTargets implicitly unbinds any aliasing SRV in every stage.
        // Decline instead of disturbing bindings beyond the one slot we own.
        bool found=false;
        for(int stage=0;stage<6;++stage){ID3D11ShaderResourceView* views[128]{};
            switch(stage){case 0:context->VSGetShaderResources(0,128,views);break;case 1:context->PSGetShaderResources(0,128,views);break;
                case 2:context->GSGetShaderResources(0,128,views);break;case 3:context->HSGetShaderResources(0,128,views);break;
                case 4:context->DSGetShaderResources(0,128,views);break;default:context->CSGetShaderResources(0,128,views);break;}
            for(auto*& view:views){if(view){ID3D11Resource* resource=nullptr;view->GetResource(&resource);found=found||resource==source;Release(resource);Release(view);}}
        }
        ID3D11UnorderedAccessView* outputs[64]{};context->CSGetUnorderedAccessViews(0,uavSlots,outputs);
        for(auto*& view:outputs){if(view){ID3D11Resource* resource=nullptr;view->GetResource(&resource);found=found||resource==source;Release(resource);Release(view);}}
        return found;
    }
    bool Initialize(ID3D11Device* d){
        if(failedFor==d)return false;
        if(device==d&&vs&&ps)return true;if(device!=d)Reset();
        if(failed)return false;
        if(d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0){status="Requer DX11 feature level 11";failed=true;failedFor=d;failedFor->AddRef();return false;}
        device=d;device->AddRef();
        static const char shader[]=R"(
cbuffer Controls:register(b0){float saturation;float contrast;float temperature;float tint;float curve;float encoded;float2 padding;};
Texture2D scene:register(t0);SamplerState clampSampler:register(s0);
struct Vertex{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
Vertex VS(uint id:SV_VertexID){Vertex v;v.uv=float2((id<<1)&2,id&2);v.position=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float3 Decode(float3 c){return lerp(c/12.92,pow((c+.055)/1.055,2.4),step(.04045,c));}
float3 Encode(float3 c){return lerp(c*12.92,1.055*pow(max(c,0),1./2.4)-.055,step(.0031308,c));}
float4 PS(Vertex v):SV_TARGET{
float4 source=scene.Sample(clampSampler,v.uv);float3 c=source.rgb;
if(encoded>.5)c=Decode(c);
c*=float3(exp2(temperature*.35-tint*.12),exp2(tint*.24),exp2(-temperature*.35-tint*.12));
float luma=dot(c,float3(.2126,.7152,.0722));c=lerp(luma.xxx,c,saturation);
c=(c-.18)*contrast+.18;
if(curve>.5){c=max(c,0);c=(c*(2.51*c+.03))/(c*(2.43*c+.59)+.14);}
c=saturate(c);if(encoded>.5)c=Encode(c);return float4(saturate(c),source.a);
})";
        ID3DBlob *v=nullptr,*p=nullptr,*errors=nullptr;
        HRESULT result=D3DCompile(shader,sizeof(shader)-1,nullptr,nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&v,&errors);Release(errors);
        if(SUCCEEDED(result))result=D3DCompile(shader,sizeof(shader)-1,nullptr,nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&p,&errors);Release(errors);
        if(SUCCEEDED(result))result=d->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs);
        if(SUCCEEDED(result))result=d->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps);
        Release(v);Release(p);
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=32;cb.Usage=D3D11_USAGE_DYNAMIC;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;cb.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        if(SUCCEEDED(result))result=d->CreateBuffer(&cb,nullptr,&parameters);
        D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
        if(SUCCEEDED(result))result=d->CreateSamplerState(&sd,&sampler);
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;
        if(SUCCEEDED(result))result=d->CreateRasterizerState(&rs,&raster);
        D3D11_DEPTH_STENCIL_DESC ds{};ds.DepthEnable=FALSE;ds.StencilEnable=FALSE;
        if(SUCCEEDED(result))result=d->CreateDepthStencilState(&ds,&depth);
        D3D11_BLEND_DESC bs{};bs.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
        if(SUCCEEDED(result))result=d->CreateBlendState(&bs,&blend);
        if(FAILED(result)){Reset();failed=true;failedFor=d;failedFor->AddRef();status="Falha ao criar shader DX11";return false;}return true;
    }
    bool Texture(const D3D11_TEXTURE2D_DESC& input,DXGI_FORMAT inputView){
        if(copy&&width==input.Width&&height==input.Height&&format==input.Format&&viewFormat==inputView)return true;
        if(rejectedWidth==input.Width&&rejectedHeight==input.Height&&rejectedFormat==input.Format&&rejectedView==inputView)return false;
        ReleaseTargets();D3D11_TEXTURE2D_DESC desc=input;desc.ArraySize=1;desc.MipLevels=1;desc.SampleDesc={1,0};
        desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=0;desc.MiscFlags=0;
        HRESULT result=device->CreateTexture2D(&desc,nullptr,&copy);
        D3D11_SHADER_RESOURCE_VIEW_DESC view{};view.Format=inputView;view.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;view.Texture2D.MipLevels=1;
        if(SUCCEEDED(result))result=device->CreateShaderResourceView(copy,&view,&image);
        if(FAILED(result)){ReleaseTargets();rejectedWidth=input.Width;rejectedHeight=input.Height;rejectedFormat=input.Format;rejectedView=inputView;
            status="Falha de textura; aguardando recriacao";return false;}
        width=input.Width;height=input.Height;format=input.Format;viewFormat=inputView;return true;
    }
public:
    Pass()=default;Pass(const Pass&)=delete;Pass& operator=(const Pass&)=delete;~Pass(){Reset();}
    const char* Status()const{return status;}
    void ReleaseTargets(){Release(image);Release(copy);width=height=rejectedWidth=rejectedHeight=0;format=viewFormat=rejectedFormat=rejectedView=DXGI_FORMAT_UNKNOWN;}
    void Reset(){ReleaseTargets();Release(vs);Release(ps);Release(parameters);Release(sampler);Release(raster);Release(depth);Release(blend);Release(device);Release(failedFor);failed=false;status="Desligada";}
    bool Apply(ID3D11Device* d,ID3D11DeviceContext* context,ID3D11Texture2D* source,ID3D11RenderTargetView* target,const Settings& settings){
        if(!Active(settings)){status=settings.enabled?"Neutra":"Desligada";return true;}
        if(!d||!context||!source||!target){status="Cena indisponivel";return false;}
        D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);D3D11_RENDER_TARGET_VIEW_DESC rt{};target->GetDesc(&rt);
        bool supported=desc.ArraySize==1&&desc.MipLevels==1&&desc.Width&&desc.Height&&
            (desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM||desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB||
             desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM||desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        if(!supported){status="Formato HDR/typeless nao suportado";return false;}
        if(!Initialize(d)||!Texture(desc,rt.Format))return false;
        PipelineState saved(context);if(saved.streamOutput){status="Aguardando fim de stream output";return false;}
        UINT uavSlots=d->GetFeatureLevel()>=D3D_FEATURE_LEVEL_11_1?64:8;
        if(SourceBoundForReading(context,source,uavSlots)){status="Cena em uso em outro shader";return false;}
        // Pixel UAVs would interfere with a fullscreen pass. Do not mutate them.
        ID3D11UnorderedAccessView* uav[64]{};context->OMGetRenderTargetsAndUnorderedAccessViews(0,nullptr,nullptr,0,uavSlots,uav);
        bool haveUav=false;for(auto*& u:uav){haveUav=haveUav||u!=nullptr;Release(u);}if(haveUav){status="Aguardando fim de pixel UAV";return false;}
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if(FAILED(context->Map(parameters,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))){status="Falha ao atualizar cor";return false;}
        bool srgb=rt.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB||rt.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        float values[8]={settings.saturation,settings.contrast,settings.temperature,settings.tint,static_cast<float>(settings.curve),srgb?0.f:1.f,0,0};
        memcpy(mapped.pData,values,sizeof(values));context->Unmap(parameters,0);
        saved.changed=true;context->SetPredication(nullptr,FALSE);context->OMSetRenderTargets(0,nullptr,nullptr);
        if(desc.SampleDesc.Count>1){UINT support{};d->CheckFormatSupport(rt.Format,&support);
            if(!(support&D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE)){status="MSAA resolve indisponivel";return false;}
            context->ResolveSubresource(copy,0,source,0,rt.Format);
        }else context->CopyResource(copy,source);
        context->OMSetRenderTargets(1,&target,nullptr);context->OMSetBlendState(blend,nullptr,0xffffffffu);context->OMSetDepthStencilState(depth,0);
        context->RSSetState(raster);D3D11_VIEWPORT viewport{0,0,static_cast<float>(desc.Width),static_cast<float>(desc.Height),0,1};context->RSSetViewports(1,&viewport);
        context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);context->GSSetShader(nullptr,nullptr,0);
        context->HSSetShader(nullptr,nullptr,0);context->DSSetShader(nullptr,nullptr,0);
        context->PSSetShaderResources(0,1,&image);context->PSSetSamplers(0,1,&sampler);context->PSSetConstantBuffers(0,1,&parameters);
        context->Draw(3,0);ID3D11ShaderResourceView* empty=nullptr;context->PSSetShaderResources(0,1,&empty);status="Ativa (cena)";return true;
    }
};
inline Pass scenePass;
}
