// Separable Gaussian blur of the real scene, composed only inside the menu.
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SceneColorGrading.h"
namespace HazzureBlur {
using HazzureSceneColor::Release;
class Pass {
    ID3D11Device* device{};ID3D11Device* failedDevice{};ID3D11VertexShader* vs{};ID3D11PixelShader* ps{};
    ID3D11Buffer* constants{};ID3D11SamplerState* sampler{};ID3D11RasterizerState* raster{};
    ID3D11DepthStencilState* depth{};ID3D11BlendState* blend{};
    ID3D11Texture2D* scene{};ID3D11ShaderResourceView* source{};
    ID3D11Texture2D* ping[2]{};ID3D11ShaderResourceView* images[2]{};ID3D11RenderTargetView* targets[2]{};
    UINT width{},height{},halfWidth{},halfHeight{};DXGI_FORMAT format{};bool ready{};
    const char* status="Desligado";
    bool Initialize(ID3D11Device* d){
        if(failedDevice==d)return false;if(device==d&&vs&&ps)return true;Reset();
        device=d;device->AddRef();if(d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)return Fail("Blur requer DX11");
        static const char shader[]=R"(
cbuffer Controls:register(b0){float2 stepUV;float chromatic;float encode;};
Texture2D scene:register(t0);SamplerState clampSampler:register(s0);
struct Vertex{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
Vertex VS(uint id:SV_VertexID){Vertex v;v.uv=float2((id<<1)&2,id&2);v.position=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float4 PS(Vertex v):SV_TARGET{
 float4 c=scene.Sample(clampSampler,v.uv)*.227027027;
 c+=(scene.Sample(clampSampler,v.uv+stepUV*1.384615385)+scene.Sample(clampSampler,v.uv-stepUV*1.384615385))*.316216216;
 c+=(scene.Sample(clampSampler,v.uv+stepUV*3.230769231)+scene.Sample(clampSampler,v.uv-stepUV*3.230769231))*.070270270;
 if(chromatic>0){float2 d=(v.uv-.5)*chromatic;c.r=lerp(c.r,scene.Sample(clampSampler,v.uv+d).r,.12);c.b=lerp(c.b,scene.Sample(clampSampler,v.uv-d).b,.12);}
 if(encode>0)c.rgb=lerp(c.rgb*12.92,1.055*pow(max(c.rgb,0),1./2.4)-.055,step(.0031308,c.rgb));
 return float4(c.rgb,1);
})";
        ID3DBlob *vertex{},*pixel{},*errors{};HRESULT a=D3DCompile(shader,sizeof(shader)-1,nullptr,nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vertex,&errors);Release(errors);
        HRESULT b=D3DCompile(shader,sizeof(shader)-1,nullptr,nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&pixel,&errors);Release(errors);
        bool ok=SUCCEEDED(a)&&SUCCEEDED(b)&&SUCCEEDED(d->CreateVertexShader(vertex->GetBufferPointer(),vertex->GetBufferSize(),nullptr,&vs))&&
            SUCCEEDED(d->CreatePixelShader(pixel->GetBufferPointer(),pixel->GetBufferSize(),nullptr,&ps));Release(vertex);Release(pixel);
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=16;cb.Usage=D3D11_USAGE_DYNAMIC;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;cb.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
        D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;
        D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=FALSE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;dd.DepthFunc=D3D11_COMPARISON_ALWAYS;
        D3D11_BLEND_DESC bd{};bd.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
        ok=ok&&SUCCEEDED(d->CreateBuffer(&cb,nullptr,&constants))&&SUCCEEDED(d->CreateSamplerState(&sd,&sampler))&&SUCCEEDED(d->CreateRasterizerState(&rd,&raster))&&
            SUCCEEDED(d->CreateDepthStencilState(&dd,&depth))&&SUCCEEDED(d->CreateBlendState(&bd,&blend));
        return ok?true:Fail("Falha nos recursos de blur");
    }
    bool Fail(const char* reason){status=reason;if(!failedDevice&&device){failedDevice=device;failedDevice->AddRef();}ready=false;return false;}
    bool Targets(const D3D11_TEXTURE2D_DESC& input){
        if(scene&&width==input.Width&&height==input.Height&&format==input.Format)return true;
        ReleaseTargets();width=input.Width;height=input.Height;format=input.Format;halfWidth=std::max(1u,width/2);halfHeight=std::max(1u,height/2);
        D3D11_TEXTURE2D_DESC desc=input;desc.MipLevels=1;desc.ArraySize=1;desc.SampleDesc={1,0};desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=0;desc.MiscFlags=0;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        if(FAILED(device->CreateTexture2D(&desc,nullptr,&scene))||FAILED(device->CreateShaderResourceView(scene,nullptr,&source))){ReleaseTargets();return false;}
        desc.Width=halfWidth;desc.Height=halfHeight;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
        for(int i=0;i<2;++i)if(FAILED(device->CreateTexture2D(&desc,nullptr,&ping[i]))||FAILED(device->CreateShaderResourceView(ping[i],nullptr,&images[i]))||FAILED(device->CreateRenderTargetView(ping[i],nullptr,&targets[i]))){ReleaseTargets();return false;}
        return true;
    }
    bool Aliased(ID3D11DeviceContext* c)const{
        // Writing any of our textures would implicitly clear aliases in foreign stages.
        bool found=false;
        for(int stage=0;stage<6;++stage){ID3D11ShaderResourceView* views[128]{};
            switch(stage){case 0:c->VSGetShaderResources(0,128,views);break;case 1:c->PSGetShaderResources(0,128,views);break;case 2:c->GSGetShaderResources(0,128,views);break;
                case 3:c->HSGetShaderResources(0,128,views);break;case 4:c->DSGetShaderResources(0,128,views);break;default:c->CSGetShaderResources(0,128,views);break;}
            for(int slot=0;slot<128;++slot)if(views[slot]){ID3D11Resource* r{};views[slot]->GetResource(&r);
                // PS0 is saved/restored explicitly, but cannot be left reading a texture being overwritten.
                found=found||r==scene||r==ping[0]||r==ping[1];Release(r);Release(views[slot]);}}
        return found;
    }
public:
    ~Pass(){Reset();}
    void ReleaseTargets(){ready=false;Release(scene);Release(source);for(int i=0;i<2;++i){Release(images[i]);Release(targets[i]);Release(ping[i]);}width=height=halfWidth=halfHeight=0;format=DXGI_FORMAT_UNKNOWN;}
    void Reset(){ReleaseTargets();Release(vs);Release(ps);Release(constants);Release(sampler);Release(raster);Release(depth);Release(blend);Release(device);Release(failedDevice);status="Desligado";}
    ID3D11ShaderResourceView* Image()const{return ready?images[1]:nullptr;}
    const char* Status()const{return status;}
    bool Capture(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Texture2D* backbuffer){
        ready=false;if(!d||!c||!backbuffer)return false;D3D11_TEXTURE2D_DESC input{};backbuffer->GetDesc(&input);
        bool supported=input.Format==DXGI_FORMAT_R8G8B8A8_UNORM||input.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB||
            input.Format==DXGI_FORMAT_B8G8R8A8_UNORM||input.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        if(input.SampleDesc.Count>1){UINT support{};if(FAILED(d->CheckFormatSupport(input.Format,&support))||!(support&D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE)){status="MSAA nao suportado pelo blur";return false;}}
        if(!supported||!input.Width||!input.Height||input.Width>16384||input.Height>16384||input.ArraySize!=1||input.MipLevels!=1){status="Formato da cena nao suportado pelo blur";return false;}
        if(!Initialize(d))return false;if(!Targets(input))return Fail("Recursos do blur indisponiveis");
        HazzureSceneColor::PipelineState saved(c);if(saved.streamOutput||Aliased(c)){status="Blur suspenso: recursos em uso";return false;}
        UINT uavCount=d->GetFeatureLevel()>=D3D_FEATURE_LEVEL_11_1?64:8;ID3D11UnorderedAccessView* uavs[64]{};c->OMGetRenderTargetsAndUnorderedAccessViews(0,nullptr,nullptr,0,uavCount,uavs);
        bool pixelUAV=false;for(auto*& u:uavs){pixelUAV=pixelUAV||u!=nullptr;Release(u);}if(pixelUAV){status="Blur suspenso: pixel UAV ativo";return false;}
        saved.changed=true;c->SetPredication(nullptr,FALSE);c->OMSetRenderTargets(0,nullptr,nullptr);
        if(input.SampleDesc.Count>1)c->ResolveSubresource(scene,0,backbuffer,0,input.Format);else c->CopyResource(scene,backbuffer);
        c->IASetInputLayout(nullptr);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        c->VSSetShader(vs,nullptr,0);c->PSSetShader(ps,nullptr,0);c->GSSetShader(nullptr,nullptr,0);c->HSSetShader(nullptr,nullptr,0);c->DSSetShader(nullptr,nullptr,0);
        c->OMSetDepthStencilState(depth,0);float blendFactor[4]{};c->OMSetBlendState(blend,blendFactor,~0u);c->RSSetState(raster);
        D3D11_VIEWPORT vp{0,0,static_cast<float>(halfWidth),static_cast<float>(halfHeight),0,1};c->RSSetViewports(1,&vp);c->PSSetSamplers(0,1,&sampler);c->PSSetConstantBuffers(0,1,&constants);
        for(int pass=0;pass<2;++pass){float values[4]={pass==0?2.f/width:0,pass==1?1.f/halfHeight:0,pass==1?.002f:0,pass==0&&(input.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB||input.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)?1.f:0};
            D3D11_MAPPED_SUBRESOURCE mapped{};if(FAILED(c->Map(constants,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))){status="Falha ao atualizar blur";return false;}
            memcpy(mapped.pData,values,sizeof(values));c->Unmap(constants,0);
            ID3D11ShaderResourceView* empty{};c->PSSetShaderResources(0,1,&empty);c->OMSetRenderTargets(1,&targets[pass],nullptr);
            auto* texture=pass==0?source:images[0];c->PSSetShaderResources(0,1,&texture);c->Draw(3,0);c->PSSetShaderResources(0,1,&empty);
        }
        ready=true;status="Gaussian blur da cena / duas passagens";return true;
    }
};
inline Pass pass;
}
