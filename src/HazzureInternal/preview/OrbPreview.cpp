// Ferramenta de bancada (fora do build.cmd): renderiza o orbe de Tsukuyomi offline e despeja
// o buffer composto em BGRA para o helper .NET converter em PNG. Serve para conferir o
// padrao do Sharingan e o halo sem depender do CS2.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../src/TsukuyomiOrb.h"
#include <cstdio>
#include <string>
#include <vector>

#pragma comment(lib, "d3d11.lib")

using HazzureSceneColor::Release;

namespace {
bool SaveRaw(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Resource* source, const wchar_t* path,
             float bgR, float bgG, float bgB, int* diagnostics) {
    D3D11_TEXTURE2D_DESC desc{};
    static_cast<ID3D11Texture2D*>(source)->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.SampleDesc.Count = 1;
    ID3D11Texture2D* staging{};
    if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging))) { *diagnostics = -1; return false; }
    context->CopyResource(staging, source);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
        *diagnostics = -2; staging->Release(); return false;
    }
    const unsigned width = desc.Width, height = desc.Height;
    std::vector<unsigned char> pixels(size_t(width) * height * 4, 0);
    const unsigned char* bytes = static_cast<const unsigned char*>(mapped.pData);
    for (unsigned y = 0; y < height; ++y) {
        const unsigned char* row = bytes + y * mapped.RowPitch;
        unsigned char* line = pixels.data() + size_t(y) * width * 4;
        for (unsigned x = 0; x < width; ++x) {
            const float a = row[x * 4 + 3] / 255.f;
            const float r = row[x * 4 + 0] / 255.f, g = row[x * 4 + 1] / 255.f, b = row[x * 4 + 2] / 255.f;
            line[x * 4 + 0] = static_cast<unsigned char>(255.f * (b * a + bgB * (1.f - a)) + 0.5f);
            line[x * 4 + 1] = static_cast<unsigned char>(255.f * (g * a + bgG * (1.f - a)) + 0.5f);
            line[x * 4 + 2] = static_cast<unsigned char>(255.f * (r * a + bgR * (1.f - a)) + 0.5f);
            line[x * 4 + 3] = 255;
        }
    }
    context->Unmap(staging, 0);
    staging->Release();
    std::FILE* handle{};
    if (_wfopen_s(&handle, path, L"wb") != 0 || !handle) { *diagnostics = -3; return false; }
    const bool written = fwrite(pixels.data(), 1, pixels.size(), handle) == pixels.size();
    fclose(handle);
    *diagnostics = written ? 0 : -4;
    return written;
}
} // namespace

int main() {
    ID3D11Device* device{}; ID3D11DeviceContext* context{}; D3D_FEATURE_LEVEL level{};
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &device, &level, &context))) {
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &device, &level, &context))) {
            std::printf("sem device D3D11\n");
            return 1;
        }
        std::printf("usando WARP\n");
    }

    HazzureTsukuyomi::SetDevice(device);
    struct Shot { const wchar_t* name; float tilt; float glow; float time; bool building; float facing; };
    const Shot shots[] = {
        { L"orb-sky", 0.10f, 1.f, 0.f, false, 1.f },
        { L"orb-spin", 0.10f, 1.f, 41.f, false, 1.f },
        { L"orb-face", 0.02f, 1.f, 0.f, false, 1.f },
        { L"orb-bite", 0.10f, 1.f, 0.f, true, 1.f },   // provavel: a construcao come a direita
        { L"orb-edge", 0.10f, 1.f, 0.f, false, 0.22f },  // quase de lado: fade de angulo do briefing
        { L"orb-dim", 0.10f, 0.35f, 12.f, false, 1.f },
    };
    constexpr unsigned cells = HazzureTsukuyomi::kMaskCells;
    float building[cells];
    for (unsigned i = 0; i < cells; ++i) {
        const float u = 2.f * float(i % HazzureTsukuyomi::kMaskSide) / float(HazzureTsukuyomi::kMaskSide - 1) - 1.f;
        const float v = 2.f * float(i / HazzureTsukuyomi::kMaskSide) / float(HazzureTsukuyomi::kMaskSide - 1) - 1.f;
        building[i] = (u + 0.35f * v > 0.42f) ? 0.f : 1.f;
    }
    int written = 0;
    for (const Shot& shot : shots) {
        HazzureTsukuyomi::Input input{};
        input.up[0] = 0.f; input.up[1] = 0.f; input.up[2] = 1.f;
        input.direction[0] = 0.4545f; input.direction[1] = 0.5454f; input.direction[2] = 0.7043f;
        input.angularRadius = 0.14f;
        input.glow = shot.glow;
        input.tilt = shot.tilt;
        input.time = shot.time;
        input.facing = shot.facing;
        if (shot.building) HazzureTsukuyomi::SetMask(building);
        else HazzureTsukuyomi::SetMask(nullptr);
        float scale = 0.f;
        ID3D11ShaderResourceView* view = HazzureTsukuyomi::Draw(input, &scale);
        ID3D11Resource* resource{};
        if (view) view->GetResource(&resource);
        int diagnostics = 0;
        if (resource) {
            const std::wstring file = std::wstring(shot.name) + L".bgra";
            if (SaveRaw(device, context, resource, file.c_str(), 32.f / 255.f, 8.f / 255.f, 14.f / 255.f, &diagnostics)) ++written;
            else std::printf("%ls: falhou (%s) diag=%d\n", shot.name, HazzureTsukuyomi::Status(), diagnostics);
        } else std::printf("%ls: sem SRV (%s)\n", shot.name, HazzureTsukuyomi::Status());
        Release(resource);
    }
    std::printf("bgra=%d status=%s escala=%.3f\n", written, HazzureTsukuyomi::Status(), 1.f / 0.58f);

    HazzureTsukuyomi::SetDevice(nullptr);
    Release(context); Release(device);
    return written == static_cast<int>(sizeof(shots) / sizeof(shots[0])) ? 0 : 2;
}
