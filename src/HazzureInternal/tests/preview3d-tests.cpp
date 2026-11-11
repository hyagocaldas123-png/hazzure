// Headless WARP: does not create a window or send OS input.
#define NOMINMAX
#include "../src/ModelPreview.h"
#include "../src/WeaponChoices.h"
#include "../src/PreviewAssets.h"
#include <cstdio>
#include <stdexcept>
#include <set>
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")
using namespace HazzureModelPreview;
static int checks{};
static void Check(bool ok,const char* why){++checks;if(!ok){printf("FAIL: %s\n",why);throw std::runtime_error(why);}}
template<class T>struct Com {T* p{};~Com(){Drop(p);}T** Out(){return &p;}T* operator->(){return p;}};
static std::vector<unsigned char> PixelsOf(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11ShaderResourceView* image){
    Check(image!=nullptr,"render returned an image");Com<ID3D11Resource> resource;image->GetResource(resource.Out());Com<ID3D11Texture2D> source;
    Check(SUCCEEDED(resource->QueryInterface(IID_PPV_ARGS(source.Out()))),"output is a texture");D3D11_TEXTURE2D_DESC d{};source->GetDesc(&d);
    d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Texture2D> staging;
    Check(SUCCEEDED(device->CreateTexture2D(&d,nullptr,staging.Out())),"readback texture");context->CopyResource(staging.p,source.p);
    D3D11_MAPPED_SUBRESOURCE map{};Check(SUCCEEDED(context->Map(staging.p,0,D3D11_MAP_READ,0,&map)),"readback map");
    std::vector<unsigned char> out(static_cast<size_t>(d.Width)*d.Height*4);
    for(unsigned y=0;y<d.Height;++y)memcpy(out.data()+y*d.Width*4,static_cast<unsigned char*>(map.pData)+y*map.RowPitch,d.Width*4);
    context->Unmap(staging.p,0);return out;
}
static void Save(const std::filesystem::path& path,unsigned width,unsigned height,std::vector<unsigned char> pixels){
    for(size_t i=0;i<pixels.size();i+=4)std::swap(pixels[i],pixels[i+2]);
    Com<IWICImagingFactory> factory;Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.Out()))),"WIC factory");
    Com<IWICStream> stream;Check(SUCCEEDED(factory->CreateStream(stream.Out()))&&SUCCEEDED(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE)),"WIC file");
    Com<IWICBitmapEncoder> encoder;Check(SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.Out()))&&SUCCEEDED(encoder->Initialize(stream.p,WICBitmapEncoderNoCache)),"PNG encoder");
    Com<IWICBitmapFrameEncode> frame;Com<IPropertyBag2> props;Check(SUCCEEDED(encoder->CreateNewFrame(frame.Out(),props.Out()))&&SUCCEEDED(frame->Initialize(props.p)),"PNG frame");
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;Check(SUCCEEDED(frame->SetSize(width,height))&&SUCCEEDED(frame->SetPixelFormat(&format))&&format==GUID_WICPixelFormat32bppBGRA,"PNG format");
    Check(SUCCEEDED(frame->WritePixels(height,width*4,static_cast<UINT>(pixels.size()),pixels.data()))&&SUCCEEDED(frame->Commit())&&SUCCEEDED(encoder->Commit()),"PNG complete");
}
static std::shared_ptr<Asset> DepthFixture(){
    auto a=std::make_shared<Asset>();const char json[]=R"({"asset":{"version":"2.0"},"nodes":[{}]})";cgltf_options options{};
    Check(cgltf_parse(&options,json,sizeof(json)-1,&a->data)==cgltf_result_success,"fixture graph");
    a->materials.resize(2);a->materials[0].pixels={1,1,{255,0,0,255}};a->materials[1].pixels={1,1,{0,255,0,255}};
    for(unsigned layer=0;layer<2;++layer){Part p;p.node=&a->data->nodes[0];p.material=layer;p.indices={0,1,2};
        const XMFLOAT3 points[]={{-.7f,-.7f,layer?.3f:-.3f},{0,.7f,layer?.3f:-.3f},{.7f,-.7f,layer?.3f:-.3f}};
        for(auto point:points){Vertex v;v.position=point;v.normal={0,0,-1};v.uv={.5f,.5f};p.vertices.push_back(v);}a->parts.push_back(std::move(p));}
    return a;
}
int wmain(int argc,wchar_t** argv){try{
    if(argc!=2)return 2;auto output=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(output);
    Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    using HazzurePreviewAssets::ModuleAssetRoot;
    Check(ModuleAssetRoot(L"C:/package/modules/0123456789abcdef/Hazzure.Internal.dll")==std::filesystem::path(L"C:/package/assets"),"versioned module resolves package assets");
    Check(ModuleAssetRoot(L"C:/package/modules/Hazzure.Internal.dll")==std::filesystem::path(L"C:/package/assets"),"direct module keeps package asset resolution");
    Check(ModuleAssetRoot(L"C:/package/Hazzure.Internal.dll")==std::filesystem::path(L"C:/package/assets"),"standalone module keeps adjacent assets");
    auto explicitAssets=output/"explicit-assets";std::filesystem::create_directories(explicitAssets/"models/weapons");
    {std::ofstream marker(explicitAssets/"models/weapons/7.glb");marker<<"explicit root takes precedence";}
    HazzurePreviewAssets::SetRoot(explicitAssets);
    Check(HazzurePreviewAssets::Find("models/weapons/7.glb")==explicitAssets/"models/weapons/7.glb","explicit asset root retains priority over module and workspace");
    Check(HazzurePreviewAssets::Find("../escape.glb").empty()&&HazzurePreviewAssets::Find("C:/escape.glb").empty(),"asset lookup still refuses escaping paths");
    HazzurePreviewAssets::SetRoot({});
    Check(!SafeRelative("../escape.png")&&!SafeRelative("C:/file.png")&&!SafeRelative("folder/file.png")&&SafeRelative("texture.png"),"texture paths stay in the asset directory");
    Check(FindModel("agents/models/ctm_fbi/ctm_fbi_variantb.vmdl")&&strcmp(FindModel("agents/models/ctm_fbi/ctm_fbi_variantb.vmdl"),FindModel("agents/models/tm_phoenix/tm_phoenix_variantg.vmdl")),"distinct exact agent mappings");
    Check(!FindModel("")&&!FindModel("invented.vmdl"),"unknown models have no substitute");
    Check(!LoadAsset(root/"Hazzure/assets/models/phoenix-anim.glb")->error.empty(),"legacy one-gigabyte GLB rejected before parsing");
    auto invalid=LoadAsset(output/"missing.glb");Check(!invalid->error.empty(),"missing file rejected");
    {std::ofstream f(output/"malformed.glb",std::ios::binary);f<<"not a glb, never parse arbitrary memory";}
    Check(!LoadAsset(output/"malformed.glb")->error.empty(),"corrupt GLB rejected");
    auto phoenix=LoadAsset(root/"Hazzure/assets/models/agents/t_14.glb");
    if(!phoenix->error.empty())printf("LOAD ERROR: %s\n",phoenix->error.c_str());
    Check(phoenix->error.empty()&&phoenix->parts.size()>0&&phoenix->clips.size()==2,"real agent mesh/material/two animations load");
    Check(phoenix->materials[0].pixels.rgba.size()>4,"real material pixels decoded");
    std::set<std::string> selected;
    for(const auto& entry:Catalogue){Check(selected.insert(entry.asset).second,"catalogue has unique exact assets");
        auto model=LoadAsset(root/"Hazzure/assets"/entry.asset);if(!model->error.empty())printf("%s: %s\n",entry.asset,model->error.c_str());
        Check(model->error.empty()&&!model->parts.empty()&&model->clips.size()==2,"all25 exact models and bounded real clips load");}
    Check(std::size(HazzureWeaponPreview::Catalogue)==35,"all35 base weapon assets catalogued");
    Check(!HazzureWeaponPreview::Find(0)&&!HazzureWeaponPreview::Find(42),"none and knives never substitute a weapon model");
    for(int definition:HazzureWeapons::definitions){if(!definition)continue;
        const auto* entry=HazzureWeaponPreview::Find(definition);Check(entry!=nullptr,"every selectable weapon has exact GLB mapping");
        Check(selected.insert(entry->asset).second,"weapon assets are distinct from agent assets and each other");
        auto model=LoadAsset(root/"Hazzure/assets"/entry->asset);
        if(!model->error.empty())printf("WEAPON %d: %s\n",definition,model->error.c_str());
        Check(model->error.empty()&&!model->parts.empty(),"all35 real base weapon GLBs load within bounds");
        for(size_t i=0;i<model->data->images_count;++i)Check(model->data->images[i].buffer_view&&!model->data->images[i].uri,"weapon texture images are embedded in the shipped GLB");
        for(const auto& part:model->parts)Check(part.name.find("body_legacy")==std::string::npos,"legacy body never overlaps the exported HD body");
    }
    Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;
    Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,device.Out(),nullptr,context.Out())),"WARP device");
    SetDevice(device.p);View view;view.width=512;view.height=512;view.yaw=3.14159f;view.clip=1;view.time=.1f;
    auto* image=Render(phoenix,view);if(!image)printf("RENDER ERROR: %s\n",Status());Check(image!=nullptr,"textured real agent renders");
    auto initial=PixelsOf(device.p,context.p,image);Save(output/"slingshot-front.png",view.width,view.height,initial);
    size_t different=0;for(size_t p=0;p<initial.size();p+=4)if(initial[p]!=initial[0]||initial[p+1]!=initial[1]||initial[p+2]!=initial[2])++different;
    Check(different>2000,"real mesh covers meaningful pixels");
    View orbit=view;orbit.yaw=view.yaw+1.f;auto rotated=PixelsOf(device.p,context.p,Render(phoenix,orbit));Check(rotated!=initial,"orbit changes perspective");Save(output/"slingshot-orbit.png",view.width,view.height,rotated);
    View animated=view;animated.time=.8f;auto animatedPixels=PixelsOf(device.p,context.p,Render(phoenix,animated));Check(animatedPixels!=initial,"actual clip changes skinned vertex output");Save(output/"slingshot-walk-b.png",view.width,view.height,animatedPixels);
    auto ava=LoadAsset(root/"Hazzure/assets/models/agents/ct_1.glb");auto avaPixels=PixelsOf(device.p,context.p,Render(ava,view));Check(avaPixels!=initial,"selected Ava is not Phoenix");Save(output/"ava-front.png",view.width,view.height,avaPixels);
    auto ak=LoadAsset(root/"Hazzure/assets/models/weapons/7.glb");View weaponView;
    weaponView.width=640;weaponView.height=320;weaponView.yaw=.85f;
    auto akPixels=PixelsOf(device.p,context.p,Render(ak,weaponView));Save(output/"ak47-base.png",weaponView.width,weaponView.height,akPixels);
    size_t weaponPixels=0;for(size_t p=0;p<akPixels.size();p+=4)if(akPixels[p]!=akPixels[0]||akPixels[p+1]!=akPixels[1]||akPixels[p+2]!=akPixels[2])++weaponPixels;
    Check(weaponPixels>1000,"real textured AK base covers meaningful pixels");
    auto weaponOrbit=weaponView;weaponOrbit.yaw+=.9f;
    auto orbitPixels=PixelsOf(device.p,context.p,Render(ak,weaponOrbit));Check(orbitPixels!=akPixels,"weapon orbit changes real rendered pixels");Save(output/"ak47-orbit.png",weaponView.width,weaponView.height,orbitPixels);
    auto weaponZoom=weaponView;weaponZoom.zoom=4.f;
    Check(PixelsOf(device.p,context.p,Render(ak,weaponZoom))!=akPixels,"weapon zoom changes real rendered pixels");
    auto fixture=DepthFixture();View depthView;depthView.width=depthView.height=128;auto depthPixels=PixelsOf(device.p,context.p,Render(fixture,depthView));
    size_t center=(64*128+64)*4;Check(depthPixels[center]>200&&depthPixels[center+1]<20,"depth keeps near red triangle over later far green triangle");
    Com<ID3D11Buffer> savedVertex,savedIndex;D3D11_BUFFER_DESC savedDesc{};savedDesc.ByteWidth=1024;savedDesc.Usage=D3D11_USAGE_DEFAULT;savedDesc.BindFlags=D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_INDEX_BUFFER;
    Check(SUCCEEDED(device->CreateBuffer(&savedDesc,nullptr,savedVertex.Out()))&&SUCCEEDED(device->CreateBuffer(&savedDesc,nullptr,savedIndex.Out())),"foreign vertex/index buffers");
    UINT savedStride=12,savedOffset=24;context->IASetVertexBuffers(0,1,&savedVertex.p,&savedStride,&savedOffset);context->IASetIndexBuffer(savedIndex.p,DXGI_FORMAT_R16_UINT,6);
    Com<ID3D11Buffer> vsBuffer;D3D11_BUFFER_DESC bd{};bd.ByteWidth=1024;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    Check(SUCCEEDED(device->CreateBuffer(&bd,nullptr,vsBuffer.Out())),"sentinel VS buffer");
    Com<ID3D11DeviceContext1> context1;Check(SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(context1.Out()))),"DX11.1 state test");UINT first=16,count=16;context1->VSSetConstantBuffers1(0,1,&vsBuffer.p,&first,&count);
    D3D11_VIEWPORT original{3,5,60,80,.1f,.8f};context->RSSetViewports(1,&original);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    Check(Render(phoenix,view)!=nullptr,"render with foreign bindings");Com<ID3D11Buffer> restored;UINT at{},n{};context1->VSGetConstantBuffers1(0,1,restored.Out(),&at,&n);
    Check(restored.p==vsBuffer.p&&at==first&&n==count,"VS b0 buffer and DX11.1 interval restored");
    D3D11_VIEWPORT after{};UINT number=1;context->RSGetViewports(&number,&after);Check(number==1&&!memcmp(&after,&original,sizeof(after)),"viewport restored");
    D3D11_PRIMITIVE_TOPOLOGY topology{};context->IAGetPrimitiveTopology(&topology);Check(topology==D3D11_PRIMITIVE_TOPOLOGY_LINELIST,"topology restored");
    Com<ID3D11Buffer> vb,ib;UINT stride{},offset{},indexOffset{};DXGI_FORMAT indexFormat{};context->IAGetVertexBuffers(0,1,vb.Out(),&stride,&offset);context->IAGetIndexBuffer(ib.Out(),&indexFormat,&indexOffset);
    Check(vb.p==savedVertex.p&&stride==savedStride&&offset==savedOffset&&ib.p==savedIndex.p&&indexFormat==DXGI_FORMAT_R16_UINT&&indexOffset==6,"vertex/index bindings restored");
    auto* own=Render(phoenix,view);context->PSSetShaderResources(1,1,&own);Check(Render(phoenix,view)==nullptr,"unowned SRV alias refuses drawing");
    Com<ID3D11ShaderResourceView> alias;context->PSGetShaderResources(1,1,alias.Out());Check(alias.p==own,"alias binding remains untouched");ID3D11ShaderResourceView* empty=nullptr;context->PSSetShaderResources(1,1,&empty);
    Com<ID3D11Predicate> predicate;D3D11_QUERY_DESC query{D3D11_QUERY_OCCLUSION_PREDICATE,0};Check(SUCCEEDED(device->CreatePredicate(&query,predicate.Out())),"occlusion predicate");
    context->Begin(predicate.p);context->End(predicate.p);BOOL predicateData=TRUE;for(unsigned i=0;i<200&&context->GetData(predicate.p,&predicateData,sizeof(predicateData),0)==S_FALSE;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    context->SetPredication(predicate.p,TRUE);Check(Render(phoenix,view)!=nullptr,"preview ignores unrelated render predicate");Com<ID3D11Predicate> afterPredicate;BOOL predicateValue{};context->GetPredication(afterPredicate.Out(),&predicateValue);
    Check(afterPredicate.p==predicate.p&&predicateValue==TRUE,"predicate binding restored");context->SetPredication(nullptr,FALSE);
    for(unsigned i=0;i<4;++i){ReleaseTargets();view.width=i&1?640:320;view.height=i&1?480:240;Check(Render(phoenix,view)!=nullptr,"targets recreate across dimensions");}
    SetDevice(nullptr);SetDevice(device.p);Check(Render(phoenix,view)!=nullptr,"device reset recreates mesh and textures");
    Loader loads;auto a=loads.Request("phoenix",root/"Hazzure/assets/models/agents/t_14.glb");
    loads.Request("ava",root/"Hazzure/assets/models/agents/ct_1.glb");std::shared_ptr<Asset> loaded;
    for(unsigned i=0;i<200&&!loaded;++i){std::this_thread::sleep_for(std::chrono::milliseconds(20));loaded=loads.Request("ava",root/"Hazzure/assets/models/agents/ct_1.glb");}
    Check(loaded&&loaded->error.empty()&&loaded->sourceFile==(root/"Hazzure/assets/models/agents/ct_1.glb").generic_u8string(),"latest asynchronous selection completes without publishing stale asset");loads.Stop();loads.Stop();Check(!loads.Request(nullptr,{}),"cancelled selection remains empty");
    const auto* selectedWeapon=HazzureWeaponPreview::Find(7);auto weaponPath=root/"Hazzure/assets"/selectedWeapon->asset;
    Check(!loads.Request(selectedWeapon->key,weaponPath),"agent-to-weapon key switch cannot return an old agent");
    std::shared_ptr<Asset> weaponLoaded;
    for(unsigned i=0;i<200&&!weaponLoaded;++i){std::this_thread::sleep_for(std::chrono::milliseconds(20));weaponLoaded=loads.Request(selectedWeapon->key,weaponPath);}
    Check(weaponLoaded&&weaponLoaded->error.empty()&&weaponLoaded->sourceFile==weaponPath.generic_u8string(),"weapon key publishes exact selected base asset");
    Check(!loads.Request("agent:ava",root/"Hazzure/assets/models/agents/ct_1.glb"),"weapon-to-agent switch never returns a stale weapon");
    Check(!loads.Request(nullptr,{}),"leaving the preview cancels pending agent selection");
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    Check(!loads.Request(nullptr,{}),"late completion cannot resurrect a cancelled preview");
    loads.Request("missing",output/"does-not-exist.glb");std::shared_ptr<Asset> missing;
    for(unsigned i=0;i<100&&!missing;++i){std::this_thread::sleep_for(std::chrono::milliseconds(5));missing=loads.Request("missing",output/"does-not-exist.glb");}
    Check(missing&&!missing->error.empty(),"loader restarts after Stop and propagates missing-file error");
    loads.Stop();SetDevice(nullptr);context->ClearState();
    printf("PASS: %d model preview checks;25 agents and35 base weapons, textured WARP, animation, weapon orbit/zoom, state, async and resize. No window or OS input.\n",checks);CoUninitialize();return 0;
}catch(const std::exception& e){printf("ERROR: %s\n",e.what());return 1;}}
