// Bounded glTF/GLB loading for the local model viewer. cgltf 1.15 is MIT licensed.
#pragma once
#include <windows.h>
#include <wincodec.h>
#include <DirectXMath.h>
#include <filesystem>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#pragma warning(push)
#pragma warning(disable:4505 4244 4267 4996)
#define CGLTF_API static
#define CGLTF_IMPLEMENTATION
#include "ModelPreviewCgltf.h"
#undef CGLTF_IMPLEMENTATION
#undef CGLTF_API
#pragma warning(pop)
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
namespace HazzureModelPreview {
using namespace DirectX;
template<class T> inline void Drop(T*& p){if(p){p->Release();p=nullptr;}}
struct Vertex {XMFLOAT3 position{},normal{};XMFLOAT2 uv{};uint32_t joint[8]{};float weight[8]={1,0,0,0,0,0,0,0};};
struct Pixels {unsigned width{},height{};std::vector<unsigned char> rgba;};
struct Material {Pixels pixels;float color[4]={1,1,1,1};float cutoff=-1;};
struct Part {std::vector<Vertex> vertices;std::vector<uint32_t> indices;size_t material{};cgltf_node* node{};std::string name;};
struct Original {cgltf_node* node;float t[3],r[4],s[3];};
struct Asset {
    cgltf_data* data{};std::vector<Part> parts;std::vector<Material> materials;std::vector<Original> original;
    XMFLOAT3 center{};float radius=1;std::vector<std::string> clips;std::string error,sourceFile;
    Asset()=default;Asset(const Asset&)=delete;Asset& operator=(const Asset&)=delete;
    ~Asset(){if(data)cgltf_free(data);}
};
inline bool SafeRelative(const char* s){return s&&*s&&!strstr(s,"..")&&!strchr(s,':')&&!strchr(s,'\\')&&!strchr(s,'/');}
inline bool Finite(const float* p,size_t n){for(size_t i=0;i<n;++i)if(!std::isfinite(p[i]))return false;return true;}
inline bool ImagePixels(const cgltf_image* image,const std::filesystem::path& directory,Pixels& out){
    if(!image)return false;IWICImagingFactory* factory{};IWICBitmapDecoder* decoder{};IWICBitmapFrameDecode* frame{};
    IWICFormatConverter* convert{};IWICBitmapScaler* scaler{};IWICStream* stream{};bool result=false;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))))return false;
    HRESULT hr=E_FAIL;
    if(image->buffer_view&&image->buffer_view->buffer&&image->buffer_view->buffer->data&&image->buffer_view->size<=64*1024*1024){
        const auto* bytes=static_cast<const BYTE*>(image->buffer_view->buffer->data)+image->buffer_view->offset;
        hr=factory->CreateStream(&stream);
        if(SUCCEEDED(hr))hr=stream->InitializeFromMemory(const_cast<BYTE*>(bytes),static_cast<DWORD>(image->buffer_view->size));
        if(SUCCEEDED(hr))hr=factory->CreateDecoderFromStream(stream,nullptr,WICDecodeMetadataCacheOnLoad,&decoder);
    }else if(SafeRelative(image->uri)){
        auto path=directory/std::filesystem::u8path(image->uri);std::error_code ec;
        if(std::filesystem::file_size(path,ec)<=64*1024*1024&&!ec)hr=factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder);
    }
    UINT width{},height{};
    if(SUCCEEDED(hr))hr=decoder->GetFrame(0,&frame);
    if(SUCCEEDED(hr))hr=frame->GetSize(&width,&height);
    if(SUCCEEDED(hr)&&width&&height&&width<=16384&&height<=16384){
        float scale=std::min(1.f,1024.f/std::max(width,height));out.width=std::max(1u,static_cast<UINT>(width*scale));out.height=std::max(1u,static_cast<UINT>(height*scale));
        hr=factory->CreateBitmapScaler(&scaler);if(SUCCEEDED(hr))hr=scaler->Initialize(frame,out.width,out.height,WICBitmapInterpolationModeFant);
        if(SUCCEEDED(hr))hr=factory->CreateFormatConverter(&convert);
        if(SUCCEEDED(hr))hr=convert->Initialize(scaler,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
        out.rgba.resize(static_cast<size_t>(out.width)*out.height*4);
        if(SUCCEEDED(hr))hr=convert->CopyPixels(nullptr,out.width*4,static_cast<UINT>(out.rgba.size()),out.rgba.data());result=SUCCEEDED(hr);
    }
    Drop(convert);Drop(scaler);Drop(frame);Drop(decoder);Drop(stream);Drop(factory);if(!result)out={};return result;
}
inline XMMATRIX Matrix(const float* p){XMFLOAT4X4 value;memcpy(&value,p,sizeof(value));return XMLoadFloat4x4(&value);}
inline bool Palette(const Part& part,std::array<XMFLOAT4X4,256>& matrices,unsigned& count){
    if(!part.node)return false;
    if(!part.node->skin){float world[16];cgltf_node_transform_world(part.node,world);if(!Finite(world,16))return false;
        memcpy(&matrices[0],world,sizeof(world));count=1;return true;}
    const auto* skin=part.node->skin;if(!skin->joints_count||skin->joints_count>256)return false;count=static_cast<unsigned>(skin->joints_count);
    for(size_t i=0;i<skin->joints_count;++i){float world[16],inverse[16];cgltf_node_transform_world(skin->joints[i],world);
        XMMATRIX inv=XMMatrixIdentity();if(skin->inverse_bind_matrices){if(!cgltf_accessor_read_float(skin->inverse_bind_matrices,i,inverse,16)||!Finite(inverse,16))return false;inv=Matrix(inverse);}
        if(!Finite(world,16))return false;XMStoreFloat4x4(&matrices[i],XMMatrixMultiply(inv,Matrix(world)));}
    return true;
}
inline std::shared_ptr<Asset> LoadAsset(const std::filesystem::path& path){
    auto asset=std::make_shared<Asset>();asset->sourceFile=path.generic_u8string();
    auto fail=[&](const char* why){asset->error=why;return asset;};std::error_code ec;
    auto size=std::filesystem::file_size(path,ec);if(ec||size<20||size>96ULL*1024*1024)return fail("Modelo ausente ou tamanho invalido");
    cgltf_options options{};if(cgltf_parse_file(&options,path.u8string().c_str(),&asset->data)!=cgltf_result_success)return fail("GLB invalido");
    auto* d=asset->data;
    if(d->nodes_count>4096||d->meshes_count>128||d->materials_count>128||d->animations_count>16||d->buffers_count>8)return fail("Limite de modelo excedido");
    for(size_t i=0;i<d->buffers_count;++i)if(d->buffers[i].uri)return fail("Buffers externos recusados");
    if(cgltf_load_buffers(&options,d,path.u8string().c_str())!=cgltf_result_success||cgltf_validate(d)!=cgltf_result_success)return fail("Dados GLB incompletos");
    for(size_t i=0;i<d->nodes_count;++i){auto* n=&d->nodes[i];unsigned depth=0;for(auto* at=n;at;at=at->parent)if(++depth>128)return fail("Hierarquia ciclica ou profunda");if(!Finite(n->translation,3)||!Finite(n->rotation,4)||!Finite(n->scale,3))return fail("Transformacao invalida");
        Original original{};original.node=n;memcpy(original.t,n->translation,sizeof(original.t));memcpy(original.r,n->rotation,sizeof(original.r));memcpy(original.s,n->scale,sizeof(original.s));asset->original.push_back(original);}
    HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    struct Apartment{bool own;~Apartment(){if(own)CoUninitialize();}} apartment{SUCCEEDED(com)};
    size_t textureBytes=0;asset->materials.resize(std::max<size_t>(d->materials_count,1));
    for(size_t i=0;i<d->materials_count;++i){const auto& material=d->materials[i];auto& out=asset->materials[i];
        if(material.has_pbr_metallic_roughness){const auto& pbr=material.pbr_metallic_roughness;memcpy(out.color,pbr.base_color_factor,sizeof(out.color));
            if(pbr.base_color_texture.texture&&pbr.base_color_texture.texture->image){
                if(!ImagePixels(pbr.base_color_texture.texture->image,path.parent_path(),out.pixels))return fail("Textura do modelo indisponivel");textureBytes+=out.pixels.rgba.size();}}
        if(textureBytes>128*1024*1024)return fail("Limite de texturas excedido");if(!Finite(out.color,4))return fail("Material invalido");
        if(material.alpha_mode==cgltf_alpha_mode_mask)out.cutoff=material.alpha_cutoff;
    }
    size_t vertices=0,indices=0;
    for(size_t nodeIndex=0;nodeIndex<d->nodes_count;++nodeIndex){auto* node=&d->nodes[nodeIndex];if(!node->mesh)continue;
        if(node->skin&&node->skin->joints_count>256)return fail("Esqueleto excede limite");
        for(size_t pi=0;pi<node->mesh->primitives_count;++pi){const auto& primitive=node->mesh->primitives[pi];
            if(primitive.type!=cgltf_primitive_type_triangles)return fail("Topologia nao suportada");
            const cgltf_accessor *position{},*normal{},*uv{},*joint{},*weight{},*joint1{},*weight1{};
            for(size_t ai=0;ai<primitive.attributes_count;++ai){const auto& a=primitive.attributes[ai];if(a.index==1){if(a.type==cgltf_attribute_type_joints)joint1=a.data;else if(a.type==cgltf_attribute_type_weights)weight1=a.data;continue;}if(a.index)continue;
                if(a.type==cgltf_attribute_type_position)position=a.data;else if(a.type==cgltf_attribute_type_normal)normal=a.data;
                else if(a.type==cgltf_attribute_type_texcoord)uv=a.data;else if(a.type==cgltf_attribute_type_joints)joint=a.data;else if(a.type==cgltf_attribute_type_weights)weight=a.data;}
            if(!position||position->count>500000||!primitive.indices||primitive.indices->count>1500000||primitive.indices->count%3)return fail("Geometria invalida");
            if((vertices+=position->count)>1000000||(indices+=primitive.indices->count)>3000000)return fail("Limite geometria excedido");
            Part part;part.node=node;part.name=node->mesh->name?node->mesh->name:"mesh";part.material=primitive.material?static_cast<size_t>(primitive.material-d->materials):0;
            part.vertices.resize(position->count);part.indices.resize(primitive.indices->count);
            for(size_t v=0;v<position->count;++v){auto& out=part.vertices[v];float p[4]{},n[4]={0,1,0,0},t[4]{};
                if(!cgltf_accessor_read_float(position,v,p,3)||!Finite(p,3))return fail("Vertice invalido");
                if(normal&&(!cgltf_accessor_read_float(normal,v,n,3)||!Finite(n,3)))return fail("Normal invalida");
                if(uv&&(!cgltf_accessor_read_float(uv,v,t,2)||!Finite(t,2)))return fail("UV invalido");
                out.position={p[0],p[1],p[2]};out.normal={n[0],n[1],n[2]};out.uv={t[0],t[1]};
                if(node->skin){cgltf_uint js[8]{};float weights[8]{};if(!joint||!weight||!cgltf_accessor_read_uint(joint,v,js,4)||!cgltf_accessor_read_float(weight,v,weights,4)||!Finite(weights,4))return fail("Skinning invalido");
                    if((joint1!=nullptr)!=(weight1!=nullptr))return fail("Pesos secundarios incompletos");if(joint1&&(!cgltf_accessor_read_uint(joint1,v,js+4,4)||!cgltf_accessor_read_float(weight1,v,weights+4,4)||!Finite(weights+4,4)))return fail("Pesos secundarios invalidos");
                    float sum=0;for(int k=0;k<8;++k){if(js[k]>=node->skin->joints_count||weights[k]<0)return fail("Joint invalido");out.joint[k]=js[k];sum+=weights[k];}
                    if(sum<.0001f)return fail("Pesos invalidos");for(int k=0;k<8;++k)out.weight[k]=weights[k]/sum;}
            }
            for(size_t i=0;i<part.indices.size();++i){auto value=cgltf_accessor_read_index(primitive.indices,i);if(value>=part.vertices.size())return fail("Indice fora do modelo");part.indices[i]=static_cast<uint32_t>(value);}
            asset->parts.push_back(std::move(part));
        }
    }
    if(asset->parts.empty())return fail("Modelo sem geometria");
    XMFLOAT3 low{FLT_MAX,FLT_MAX,FLT_MAX},high{-FLT_MAX,-FLT_MAX,-FLT_MAX};
    for(const auto& part:asset->parts){std::array<XMFLOAT4X4,256> palette;unsigned count{};if(!Palette(part,palette,count))return fail("Bind pose invalida");
        for(const auto& vertex:part.vertices){XMVECTOR point=XMVectorZero();for(int k=0;k<8;++k)if(vertex.weight[k]>0)point=XMVectorAdd(point,XMVectorScale(XMVector3TransformCoord(XMLoadFloat3(&vertex.position),XMLoadFloat4x4(&palette[vertex.joint[k]])),vertex.weight[k]));
            XMFLOAT3 p;XMStoreFloat3(&p,point);if(!Finite(&p.x,3))return fail("Limites invalidos");low.x=std::min(low.x,p.x);low.y=std::min(low.y,p.y);low.z=std::min(low.z,p.z);high.x=std::max(high.x,p.x);high.y=std::max(high.y,p.y);high.z=std::max(high.z,p.z);}}
    asset->center={(low.x+high.x)*.5f,(low.y+high.y)*.5f,(low.z+high.z)*.5f};asset->radius=std::max(.01f,std::sqrt((high.x-low.x)*(high.x-low.x)+(high.y-low.y)*(high.y-low.y)+(high.z-low.z)*(high.z-low.z))*.5f);
    if(!std::isfinite(asset->radius)||asset->radius>10000)return fail("Escala invalida");
    for(size_t a=0;a<d->animations_count;++a){auto& animation=d->animations[a];if(animation.channels_count>4096||animation.samplers_count>4096)return fail("Clip excede limite");
        for(size_t s=0;s<animation.samplers_count;++s){const auto& sampler=animation.samplers[s];if(!sampler.input||!sampler.output||!sampler.input->count||sampler.input->count>100000||sampler.input->count!=sampler.output->count||sampler.interpolation==cgltf_interpolation_type_cubic_spline)return fail("Clip nao suportado");
            float previous=-1;for(size_t key=0;key<sampler.input->count;++key){float value{};if(!cgltf_accessor_read_float(sampler.input,key,&value,1)||!std::isfinite(value)||value<0||value<=previous)return fail("Tempos de clip invalidos");previous=value;}}
        asset->clips.emplace_back(animation.name?animation.name:"Animacao");}
    return asset;
}
inline void Animate(Asset& asset,int clip,float seconds){
    for(const auto& original:asset.original){memcpy(original.node->translation,original.t,sizeof(original.t));memcpy(original.node->rotation,original.r,sizeof(original.r));memcpy(original.node->scale,original.s,sizeof(original.s));}
    if(clip<0||static_cast<size_t>(clip)>=asset.data->animations_count||!std::isfinite(seconds))return;
    auto& animation=asset.data->animations[clip];float duration=0;
    for(size_t i=0;i<animation.samplers_count;++i){const auto& s=animation.samplers[i];float last{};if(s.input->count&&cgltf_accessor_read_float(s.input,s.input->count-1,&last,1)&&std::isfinite(last))duration=std::max(duration,last);}
    float time=duration>0?std::fmod(std::max(seconds,0.f),duration):0;
    for(size_t i=0;i<animation.channels_count;++i){auto& c=animation.channels[i];auto* sampler=c.sampler;auto* node=c.target_node;if(!node||!sampler||!sampler->input->count)continue;
        size_t lo=0,hi=sampler->input->count-1;while(lo<hi){size_t mid=(lo+hi+1)/2;float value{};cgltf_accessor_read_float(sampler->input,mid,&value,1);if(value<=time)lo=mid;else hi=mid-1;}
        size_t next=std::min(lo+1,sampler->input->count-1);float a{},b{};cgltf_accessor_read_float(sampler->input,lo,&a,1);cgltf_accessor_read_float(sampler->input,next,&b,1);
        float blend=sampler->interpolation==cgltf_interpolation_type_step||b<=a?0:std::clamp((time-a)/(b-a),0.f,1.f);
        size_t count=c.target_path==cgltf_animation_path_type_rotation?4:3;if(c.target_path==cgltf_animation_path_type_weights)continue;
        float p[4]{},q[4]{};if(!cgltf_accessor_read_float(sampler->output,lo,p,count)||!cgltf_accessor_read_float(sampler->output,next,q,count)||!Finite(p,count)||!Finite(q,count))continue;
        if(c.target_path==cgltf_animation_path_type_rotation){auto value=XMQuaternionNormalize(XMQuaternionSlerp(XMLoadFloat4(reinterpret_cast<XMFLOAT4*>(p)),XMLoadFloat4(reinterpret_cast<XMFLOAT4*>(q)),blend));XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(node->rotation),value);node->has_rotation=1;}
        else {float* dest=c.target_path==cgltf_animation_path_type_scale?node->scale:node->translation;for(int k=0;k<3;++k)dest[k]=p[k]+(q[k]-p[k])*blend;if(c.target_path==cgltf_animation_path_type_scale)node->has_scale=1;else node->has_translation=1;}
    }
}
}
