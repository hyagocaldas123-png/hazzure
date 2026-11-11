// Local catalogue images, uploaded to the overlay's own D3D11 device on demand.
#pragma once
#include <windows.h>
#include <d3d11.h>
#include <wincodec.h>
#include <filesystem>
#include <string>
#include <vector>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
namespace HazzurePreviewAssets {
struct Texture {ID3D11ShaderResourceView* view{};unsigned width{},height{};};
struct Cached {std::string key;Texture texture{};};
inline constexpr size_t CacheCapacity=128;
inline ID3D11Device* device{};inline std::vector<Cached> cache;inline std::filesystem::path explicitRoot;
inline void Clear(){for(auto& entry:cache)if(entry.texture.view)entry.texture.view->Release();cache.clear();}
inline void SetDevice(ID3D11Device* value){if(value==device)return;Clear();if(device)device->Release();device=value;if(device)device->AddRef();}
inline void SetRoot(const std::filesystem::path& root){Clear();explicitRoot=root;}
template<class T> struct Com {T* value{};~Com(){if(value)value->Release();}T** Out(){return &value;}T* operator->(){return value;}};
inline std::filesystem::path ModuleAssetRoot(const std::filesystem::path& module){
    auto directory=module.parent_path();
    if(directory.filename()==L"modules")directory=directory.parent_path();
    else if(directory.parent_path().filename()==L"modules")directory=directory.parent_path().parent_path();
    return directory/L"assets";
}
inline std::filesystem::path Find(const char* relative){
    if(!relative||!*relative||strstr(relative,"..")||strchr(relative,':'))return {};
    std::filesystem::path key(relative);if(key.is_absolute())return {};
    std::vector<std::filesystem::path> roots;
    if(!explicitRoot.empty())roots.push_back(explicitRoot);
    HMODULE module{};wchar_t filename[32768]{};
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Find),&module)&&GetModuleFileNameW(module,filename,32768)){
        auto directory=std::filesystem::path(filename).parent_path();roots.push_back(directory/L"assets");
        auto package=ModuleAssetRoot(filename);if(package!=roots.back())roots.push_back(package);
    }
    roots.push_back(L"C:/HazzureKernel/assets");
    roots.push_back(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/L"Hazzure/assets");
    std::error_code error;
    for(const auto& root:roots){auto file=root/key;if(std::filesystem::is_regular_file(file,error))return file;error.clear();}
    return {};
}
inline Texture Load(const std::filesystem::path& path){
    Texture result{};if(!device||path.empty())return result;
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    struct Apartment {bool own;~Apartment(){if(own)CoUninitialize();}} apartment{SUCCEEDED(initialized)};
    Com<IWICImagingFactory> factory;Com<IWICBitmapDecoder> decoder;Com<IWICBitmapFrameDecode> frame;Com<IWICFormatConverter> converter;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.Out())))||
       FAILED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,decoder.Out()))||
       FAILED(decoder->GetFrame(0,frame.Out()))||FAILED(frame->GetSize(&result.width,&result.height))||
       !result.width||!result.height||result.width>2048||result.height>2048||
       FAILED(factory->CreateFormatConverter(converter.Out()))||
       FAILED(converter->Initialize(frame.value,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return {};
    std::vector<unsigned char> pixels(static_cast<size_t>(result.width)*result.height*4);
    if(FAILED(converter->CopyPixels(nullptr,result.width*4,static_cast<UINT>(pixels.size()),pixels.data())))return {};
    // Economy portraits have generous transparent margins. Fit the visible subject,
    // keeping its exact pixels and aspect ratio, instead of shrinking the padded canvas.
    unsigned left=result.width,top=result.height,right=0,bottom=0;
    for(unsigned y=0;y<result.height;++y)for(unsigned x=0;x<result.width;++x)
        if(pixels[(static_cast<size_t>(y)*result.width+x)*4+3]>8){left=std::min(left,x);top=std::min(top,y);right=std::max(right,x);bottom=std::max(bottom,y);}
    if(left<result.width&&top<result.height){unsigned width=right-left+1,height=bottom-top+1;
        std::vector<unsigned char> trimmed(static_cast<size_t>(width)*height*4);
        for(unsigned y=0;y<height;++y)memcpy(trimmed.data()+static_cast<size_t>(y)*width*4,
            pixels.data()+(static_cast<size_t>(top+y)*result.width+left)*4,static_cast<size_t>(width)*4);
        pixels.swap(trimmed);result.width=width;result.height=height;
    }
    D3D11_TEXTURE2D_DESC description{};description.Width=result.width;description.Height=result.height;
    description.MipLevels=1;description.ArraySize=1;description.Format=DXGI_FORMAT_R8G8B8A8_UNORM;description.SampleDesc.Count=1;
    description.Usage=D3D11_USAGE_IMMUTABLE;description.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{pixels.data(),result.width*4,0};Com<ID3D11Texture2D> texture;
    if(FAILED(device->CreateTexture2D(&description,&initial,texture.Out()))||FAILED(device->CreateShaderResourceView(texture.value,nullptr,&result.view)))return {};
    return result;
}
inline Texture Get(const char* relative){
    if(!device||!relative)return {};
    for(const auto& entry:cache)if(entry.key==relative)return entry.texture;
    // Never evict a view already referenced by an ImGui draw list. Lifecycle resets
    // run outside drawing; saturation uses the caller's fallback until that reset.
    if(cache.size()>=CacheCapacity)return {};
    // Commit every potentially allocating cache operation before creating the SRV.
    // A failed insertion therefore has no newly loaded resource to orphan.
    cache.push_back({relative,{}});auto& next=cache.back();
    try{next.texture=Load(Find(relative));return next.texture;}
    catch(...){cache.pop_back();throw;} // An exception must not cache a failed attempt.
}
inline Texture GetWeaponThumbnail(int def, int paint) {
    char asset[80]{};
    if (paint > 0) {
        sprintf_s(asset, "skins/%d_%d.png", def, paint);
        Texture t = Get(asset);
        if (t.view) return t;
    }
    sprintf_s(asset, "skins/%d_0.png", def);
    return Get(asset);
}
}
