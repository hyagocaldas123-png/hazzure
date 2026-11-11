// Hidden WARP-only verification. No window, input, game process or audio.
#include "../src/DX11Blur.h"
#include <vector>
#include <cstdio>
#pragma comment(lib,"d3d11.lib")
static int checks{};
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
using HazzureSceneColor::Release;
int main(){
    ID3D11Device* device{};ID3D11DeviceContext* context{};D3D_FEATURE_LEVEL level{};
    CHECK(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context)));
    D3D11_TEXTURE2D_DESC desc{};desc.Width=128;desc.Height=64;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc={1,0};desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    std::vector<uint32_t> pixels(desc.Width*desc.Height);for(unsigned y=0;y<desc.Height;++y)for(unsigned x=0;x<desc.Width;++x)pixels[y*desc.Width+x]=x<64?0xff000000:0xffffffff;
    D3D11_SUBRESOURCE_DATA data{pixels.data(),desc.Width*4,0};ID3D11Texture2D* source{};ID3D11RenderTargetView* target{};
    CHECK(SUCCEEDED(device->CreateTexture2D(&desc,&data,&source)));CHECK(SUCCEEDED(device->CreateRenderTargetView(source,nullptr,&target)));
    context->OMSetRenderTargets(1,&target,nullptr);D3D11_VIEWPORT vp{3,7,100,50,.1f,.9f};context->RSSetViewports(1,&vp);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    D3D11_QUERY_DESC q{D3D11_QUERY_OCCLUSION_PREDICATE,0};ID3D11Predicate* predicate{};CHECK(SUCCEEDED(device->CreatePredicate(&q,&predicate)));context->SetPredication(predicate,TRUE);
    ID3D11DeviceContext1* c1{};context->QueryInterface(__uuidof(ID3D11DeviceContext1),reinterpret_cast<void**>(&c1));
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=1024;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;ID3D11Buffer* cb{};CHECK(SUCCEEDED(device->CreateBuffer(&bd,nullptr,&cb)));
    UINT first=16,count=16;if(c1)c1->PSSetConstantBuffers1(0,1,&cb,&first,&count);else context->PSSetConstantBuffers(0,1,&cb);
    HazzureBlur::Pass blur;CHECK(blur.Capture(device,context,source));CHECK(blur.Image()!=nullptr);
    ID3D11RenderTargetView* after{};context->OMGetRenderTargets(1,&after,nullptr);CHECK(after==target);Release(after);
    D3D11_VIEWPORT restored{};UINT n=1;context->RSGetViewports(&n,&restored);CHECK(n==1&&restored.TopLeftX==vp.TopLeftX&&restored.Height==vp.Height&&restored.MinDepth==vp.MinDepth);
    D3D11_PRIMITIVE_TOPOLOGY topology{};context->IAGetPrimitiveTopology(&topology);CHECK(topology==D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ID3D11Predicate* pred{};BOOL predValue{};context->GetPredication(&pred,&predValue);CHECK(pred==predicate&&predValue);Release(pred);context->SetPredication(nullptr,FALSE);
    ID3D11Buffer* restoredBuffer{};UINT actualFirst{},actualCount{};
    if(c1){c1->PSGetConstantBuffers1(0,1,&restoredBuffer,&actualFirst,&actualCount);CHECK(restoredBuffer==cb&&actualFirst==first&&actualCount==count);}
    else{context->PSGetConstantBuffers(0,1,&restoredBuffer);CHECK(restoredBuffer==cb);}Release(restoredBuffer);
    ID3D11Resource* output{};blur.Image()->GetResource(&output);ID3D11Texture2D* texture{};CHECK(SUCCEEDED(output->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture))));Release(output);
    D3D11_TEXTURE2D_DESC result{};texture->GetDesc(&result);CHECK(result.Width==64&&result.Height==32);
    result.BindFlags=0;result.Usage=D3D11_USAGE_STAGING;result.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage{};CHECK(SUCCEEDED(device->CreateTexture2D(&result,nullptr,&stage)));
    context->CopyResource(stage,texture);D3D11_MAPPED_SUBRESOURCE mapped{};CHECK(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&mapped)));
    auto* row=static_cast<unsigned char*>(mapped.pData)+mapped.RowPitch*16;CHECK(row[4*2]<5);CHECK(row[4*61]>250);CHECK(row[4*31]>20&&row[4*31]<230);CHECK(row[4*32]>20&&row[4*32]<245);
    FILE* fp{};fopen_s(&fp,"blur.ppm","wb");CHECK(fp!=nullptr);fprintf(fp,"P6\n64 32\n255\n");for(unsigned y=0;y<32;++y){auto* line=static_cast<unsigned char*>(mapped.pData)+mapped.RowPitch*y;for(unsigned x=0;x<64;++x)fwrite(line+x*4,1,3,fp);}fclose(fp);
    context->Unmap(stage,0);Release(stage);Release(texture);
    auto* image=blur.Image();context->VSSetShaderResources(5,1,&image);CHECK(!blur.Capture(device,context,source));CHECK(!blur.Image());
    ID3D11ShaderResourceView* still{};context->VSGetShaderResources(5,1,&still);CHECK(still==image);Release(still);ID3D11ShaderResourceView* empty{};context->VSSetShaderResources(5,1,&empty);
    CHECK(blur.Capture(device,context,source));blur.ReleaseTargets();CHECK(!blur.Image());CHECK(blur.Capture(device,context,source));blur.Reset();CHECK(!blur.Image());CHECK(blur.Capture(device,context,source));
    desc.Width=80;desc.Height=48;ID3D11Texture2D* resized{};CHECK(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&resized)));CHECK(blur.Capture(device,context,resized));
    blur.Image()->GetResource(&output);CHECK(SUCCEEDED(output->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture))));texture->GetDesc(&result);CHECK(result.Width==40&&result.Height==24);Release(output);Release(texture);
    desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;ID3D11Texture2D* unsupported{};CHECK(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&unsupported)));CHECK(!blur.Capture(device,context,unsupported));CHECK(!blur.Image());
    CHECK(blur.Capture(device,context,source));blur.Reset();context->ClearState();Release(unsupported);Release(resized);Release(predicate);Release(cb);Release(c1);Release(target);Release(source);Release(context);Release(device);
    printf("PASS %d blur WARP checks\n",checks);return 0;
}
