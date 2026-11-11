// Lua de Tsukuyomi: orbe 3D real. Uma esfera iluminada no espaco do mundo, renderizada
// offscreen e composta no ceu pelo overlay. A oclusao usa o mesmo trace nativo do ESP, mas
// bombeado no frame hook do jogo; a thread de presentacao nunca entra na engine.
#pragma once
#include "SceneColorGrading.h"
#include "Visibility.h"
#include <DirectXMath.h>
#include <atomic>
#include <vector>

namespace HazzureTsukuyomi {

using namespace DirectX;
using HazzureSceneColor::Release;

// Grade de oclusao: celulas dentro do circulo unitario do disco, amostradas pelo pixel shader.
inline constexpr unsigned kMaskSide = 11;
inline constexpr unsigned kMaskCells = kMaskSide * kMaskSide;
inline constexpr unsigned kMaskVecs = (kMaskCells + 3u) / 4u;

struct Input {
    // Up da camera em coordenadas de mundo (linha 1 da matriz world->clip).
    float up[3]{};
    float direction[3]{};          // direcao do orbe no mundo (unidade)
    float angularRadius = 0.14f;   // raio solido em radianos
    float glow = 1.f;              // halo ao redor do disco
    float tilt = 0.10f;            // inclinacao do polo da iris, em radianos
    float time = 0.f;              // segundos, para o giro do padrao
    float facing = 1.f;            // 1 de frente para a camera, 0 a 90 graus do eixo de visao
};

struct alignas(16) Constants {
    XMFLOAT4X4 mvp;
    XMFLOAT4 axis{}, poleUp{}, poleRight{}, lightDir{}, eyeDir{}, params{}, surface{};
    XMFLOAT4 mask[kMaskVecs]{};   // 121 celulas em 31 vetores; o FXC usa passo de 16 bytes
};

inline bool Finite(const float* v, unsigned n) {
    for (unsigned i = 0; i < n; ++i) if (!std::isfinite(v[i])) return false;
    return true;
}
inline bool Valid(const Input& in) {
    return Finite(in.up, 3) && Finite(in.direction, 3) &&
        std::isfinite(in.angularRadius) && in.angularRadius >= .02f && in.angularRadius <= .45f &&
        std::isfinite(in.glow) && in.glow >= 0.f && in.glow <= 2.f &&
        std::isfinite(in.tilt) && in.tilt >= 0.f && in.tilt <= .8f && std::isfinite(in.time) && in.time >= 0.f &&
        std::isfinite(in.facing) && in.facing >= 0.f && in.facing <= 1.f;
}

static const char* const source = R"(
cbuffer Sky : register(b0)
{
    row_major float4x4 mvp;
    float4 axis;      // polo da iris, preso ao mundo
    float4 poleUp;
    float4 poleRight;
    float4 lightDir;  // sol do mapa, em coordenadas de mundo
    float4 eyeDir;    // do centro do orbe ate a camera
    float4 params;    // x tempo, y velocidade do giro, z brilho do halo, w emissao
    float4 surface;   // x largura, y altura, z raio da esfera em meia tela, w fade de angulo
    float4 maskGrid[31];     // grade 11x11 (121 celulas) em 31 vetores: 1 livre, 0 atras de geometria
};

static const float PI = 3.14159265;

float h21(float2 p) { float3 q = frac(float3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return frac((q.x + q.y) * q.z); }
float vn(float2 p)
{
    float2 i = floor(p), f = frac(p); f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(h21(i), h21(i + float2(1, 0)), f.x),
                lerp(h21(i + float2(0, 1)), h21(i + float2(1, 1)), f.x), f.y);
}
float wrapPi(float a) { return (frac(a * (1.0 / (2.0 * PI)) + 0.5) - 0.5) * 2.0 * PI; }

// Coordenada do disco: (-1,-1) a (1,1) sobre a silhueta projetada, y para cima.
float2 discUv(float4 pixel) {
    const float span = 0.5 * surface.x;
    return (pixel.xy - span) / (surface.z * span);
}
// Bilinear da grade de oclusao. Fora do circulo a grade satura na borda, entao o halo tambem
// e mordido pela construcao em vez de vazar por cima dela.
float cellAt(uint idx)
{
    const float4 v = maskGrid[idx >> 2];
    const uint k = idx & 3;
    return k == 0 ? v.x : (k == 1 ? v.y : (k == 2 ? v.z : v.w));
}
float maskAt(float2 uv)
{
    float2 g = saturate(uv * 0.5 + 0.5) * 10.0;
    int2 i0 = int2(floor(g));
    float2 f = g - float2(i0);
    int2 i1 = min(i0 + 1, 10);
    const uint r0 = uint(i0.y) * 11u, r1 = uint(i1.y) * 11u;
    const float a = cellAt(r0 + uint(i0.x));
    const float b = cellAt(r0 + uint(i1.x));
    const float c = cellAt(r1 + uint(i0.x));
    const float d = cellAt(r1 + uint(i1.x));
    const float m = lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
    return saturate((m - 0.5) * 3.0 + 0.5);   // estreita o degrau entre celulas: canto de predio
}

struct VSIN { float3 p : POSITION; };
struct IN { float4 p : SV_POSITION; float3 n : NORMAL; };
struct Q { float4 p : SV_POSITION; };

IN SphereVS(VSIN v) { IN o; o.p = mul(float4(v.p, 1), mvp); o.n = normalize(v.p); return o; }
Q QuadVS(uint id : SV_VertexID) { Q o; o.p = float4(id == 1 ? 3.0 : -1.0, id == 2 ? 3.0 : -1.0, 0, 1); return o; }

float4 HaloPS(Q i) : SV_TARGET
{
    float2 uv = discUv(i.p);                          // silhueta do disco em |uv| = 1
    const float k = max(surface.z, 1e-3);
    float r = max(length(uv), 1e-4) * k;              // volta ao espaco de meia-tela: orbe em r = k
    float bloom = pow(min(k / r, 2.0), 2.6);
    float skirt = saturate(1.0 - (r - k) / (k * 0.9));
    float edge = saturate((0.92 - r) / 0.42);         // zero antes da borda do quadrado: nunca ha
                                                     // corte reto de textura na tela
    float occ = maskAt(uv * (k / max(r, k)));         // projecao radial para a grade
    return float4(0.80, 0.07, 0.11, saturate(params.z * bloom * (0.20 + 0.80 * skirt) * 1.25 * edge * occ * surface.w));
}

float4 OrbPS(IN i) : SV_TARGET
{
    float3 n = normalize(i.n);
    float nv = dot(n, eyeDir.xyz);
    clip(nv);                                  // apenas a calota voltada para a camera

    // Oclusao por fatias: cada celula da grade foi medida no frame hook do jogo e vira alpha
    // por pixel aqui, entao a construcao come o pedaco do disco que esta atras dela. A lua
    // afunda na geografia em vez de esmaecer inteira.
    float2 uv = discUv(i.p);
    float occ = maskAt(uv);
    clip(occ - 0.004);

    // Raio polar desde o polo da iris, normalizado pelo raio do padrao. As proporcoes abaixo
    // foram medidas na referencia (fracao do raio de tela -> rho = asin(f)/1.4): pupila 0.16 f,
    // aneis 0.31/0.55/0.85 f e a borda preta comeca em 0.90 f, colada no limbo.
    const float edge = 1.40, pupil = 0.116;
    float theta = acos(clamp(dot(n, axis.xyz), -1.0, 1.0));
    float phi = atan2(dot(n, poleRight.xyz), dot(n, poleUp.xyz)) - params.x * params.y;
    float rho = theta / edge;
    float2 D = rho * float2(sin(phi), cos(phi));   // projecao equidistante azimuthal
    float disc = length(uv);                       // raio projetado: 1.0 exatamente no limbo

    // Corpo de lua: marmore/plasma entre #8B0000 e #B22222 (especificacao), nunca vermelho chato.
    float blot = vn(D * 2.2 + 11.0) * 0.55 + vn(D * 4.7 - 4.0) * 0.30 + vn(D * 9.3 + 3.0) * 0.15;
    float3 col = lerp(float3(0.545, 0.000, 0.000), float3(0.698, 0.133, 0.133), smoothstep(0.28, 0.76, blot));
    col = lerp(col, float3(0.400, 0.020, 0.030), vn(D * 7.4 + 27.0) * 0.35);            // manchas escuras
    col += float3(0.860, 0.330, 0.300) * smoothstep(0.78, 0.99, vn(D * 15.0)) * 0.13;   // plasma claro
    col *= 1.0 - 0.16 * smoothstep(0.84, 1.00, rho);

    // Tinta (#000000): pupila cheia, tres aneis concentricos finos como orbitas e uma triade de
    // tomoe a 120 graus sobre CADA anel (9 no total, como na referencia). A cabeca e um circulo
    // preso ao anel; a calda e um gancho curto (~49 graus) que arqueia para fora, corre no sentido
    // horario da tela -- a cauda sempre fica atras do proximo tomoe -- e afina ate virar ponta.
    float ink = 0.0;
    ink = max(ink, 1.0 - smoothstep(pupil, pupil + 0.014, rho));
    ink = max(ink, smoothstep(0.774, 0.792, rho));
    const float ringR[3] = { 0.226, 0.418, 0.718 };
    const float ringA[3] = { 0.52, 1.05, 0.0 };
    const float headR[3] = { 0.048, 0.056, 0.066 };
    for (int r = 0; r < 3; ++r)
    {
        ink = max(ink, 1.0 - smoothstep(0.005, 0.013, abs(rho - ringR[r])));
        for (int k = 0; k < 3; ++k)
        {
            const float ak = ringA[r] + float(k) * (2.0 * PI / 3.0);
            const float hr = headR[r];
            const float2 c = ringR[r] * float2(sin(ak), cos(ak));
            ink = max(ink, 1.0 - smoothstep(hr, hr + 0.012, length(D - c)));        // cabeca
            const float a = wrapPi(phi - ak);                // 0 na cabeca, + no sentido do giro
            const float t = a / 0.85;                        // 0..1 ao longo da calda
            const float bow = ringR[r] + hr * (0.55 * sin(t * 2.4) - 0.18 * t);
            const float thick = hr * (0.46 - 0.20 * t) * (1.0 - t * t);
            ink = max(ink, step(0.0, a) * step(a, 0.85) *
                           (1.0 - smoothstep(thick, thick + 0.008, abs(rho - bow))));
        }
    }
    col = lerp(col, float3(0.004, 0.001, 0.002), saturate(ink));

    // Esfera de verdade, mas discreta: o padrao precisa continuar legivel de longe, entao o
    // terminador so modela o corpo e o especular de cornea ficou pequeno.
    float ndl = dot(n, lightDir.xyz);
    float term = saturate(ndl * 0.5 + 0.5);
    col *= lerp(float3(0.70, 0.34, 0.36), float3(1.05, 1.01, 1.00), pow(term, 1.1));
    col += float3(0.38, 0.05, 0.06) * saturate(1.0 - abs(ndl) / 0.30) * 0.14;
    col *= 0.84 + 0.16 * sqrt(saturate(nv));
    col += float3(1.0, 0.26, 0.20) * pow(1.0 - saturate(nv), 4.0) * 0.22;
    float3 h = normalize(lightDir.xyz + eyeDir.xyz);
    float nh = saturate(dot(n, h));
    col += float3(1.0, 0.88, 0.84) * pow(nh, 220.0) * 0.38 + float3(0.95, 0.40, 0.35) * pow(nh, 26.0) * 0.04;
    // Alpha do corpo celeste: a fatia mordida pela geometria, o limbo suave nos ultimos 3% do
    // raio projetado (sem aresta dura) e o fade de angulo do briefing (surface.w).
    const float limb = 1.0 - smoothstep(0.97, 1.0, disc);
    return float4(max(col * params.w, 0.0), occ * limb * surface.w);
}
)";

class Renderer {
    ID3D11Device* device{}; ID3D11DeviceContext* context{};
    ID3D11VertexShader* sphereVs{}; ID3D11PixelShader* orbPs{};
    ID3D11VertexShader* quadVs{}; ID3D11PixelShader* haloPs{};
    ID3D11InputLayout* layout{}; ID3D11Buffer* vertex{}; ID3D11Buffer* index{}; ID3D11Buffer* constants{};
    ID3D11Texture2D* multi{}; ID3D11RenderTargetView* multiTarget{};
    ID3D11Texture2D* resolved{}; ID3D11ShaderResourceView* image{};
    ID3D11Texture2D* depth{}; ID3D11DepthStencilView* depthTarget{};
    ID3D11RasterizerState* raster{}; ID3D11DepthStencilState* solid{}; ID3D11DepthStencilState* open{};
    ID3D11BlendState* opaque{};
    UINT count{}; bool failed{}; const char* status = "Lua Tsukuyomi ociosa";
    static constexpr unsigned extent = 512;      // alvo quadrado com MSAA 4x
    static constexpr float cover = 0.58f;        // raio da esfera sobre a meia-tela; sobra halo
    static constexpr float kProbeReach = 24000.f;
    static constexpr unsigned kRaysPerFrame = 12;   // teto de traces por frame do hook
    static constexpr uint64_t kSweepIntervalMs = 90;
    struct Probe {
        HazzureGame::Vec3 eye{}, dir{}, right{}, up{};
        float tan{};
        bool armed{};
        unsigned cursor{};
        uint64_t nextSweep{};
        float value[kMaskCells];   // 1 = ceu livre, 0 = atras de geometria
        bool dirty[kMaskCells];
    };
    Probe probe{ MakeProbe() };    // armed na thread de presentacao, pumpada no frame hook
    SRWLOCK probeLock{};
    std::atomic<float> visibility{ 1.f };

    static Probe MakeProbe() {
        Probe p{};
        for (auto& v : p.value) v = 1.f;         // sem medida, o disco segue inteiro
        return p;
    }
    // Celula dentro do circulo do disco? As quatro esquinas do quadrado nao tem raio a medir.
    static bool MaskInside(unsigned i) {
        const float u = 2.f * float(i % kMaskSide) / float(kMaskSide - 1) - 1.f;
        const float v = 2.f * float(i / kMaskSide) / float(kMaskSide - 1) - 1.f;
        return u * u + v * v <= 1.05f;
    }
    static float Dot(const HazzureGame::Vec3& a, const HazzureGame::Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    void Clear() {
        Release(layout); Release(vertex); Release(index); Release(constants);
        Release(multi); Release(multiTarget); Release(resolved); Release(image);
        Release(depth); Release(depthTarget);
        Release(raster); Release(solid); Release(open); Release(opaque);
        Release(sphereVs); Release(orbPs); Release(quadVs); Release(haloPs);
        count = 0; failed = false;
    }
    bool Compiled(ID3DBlob*& blob, const char* entry, const char* target, ID3DBlob*& error) {
        return SUCCEEDED(D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry, target,
            D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &blob, &error));
    }
    bool Mesh() {
        std::vector<XMFLOAT3> positions; std::vector<unsigned> indices;
        constexpr unsigned rings = 48, sectors = 96;
        const float pi = 3.14159265f;
        for (unsigned r = 0; r <= rings; ++r) {
            float polar = pi * float(r) / float(rings), sp = std::sin(polar), cp = std::cos(polar);
            for (unsigned s = 0; s <= sectors; ++s) {
                float az = 2.f * pi * float(s) / float(sectors);
                positions.push_back({ sp * std::cos(az), sp * std::sin(az), cp });
            }
        }
        for (unsigned r = 0; r < rings; ++r) for (unsigned s = 0; s < sectors; ++s) {
            unsigned a = r * (sectors + 1) + s, b = a + sectors + 1;
            indices.insert(indices.end(), { a, b, a + 1, b, b + 1, a + 1 });
        }
        D3D11_BUFFER_DESC vb{}; vb.ByteWidth = UINT(positions.size() * sizeof(XMFLOAT3));
        vb.Usage = D3D11_USAGE_IMMUTABLE; vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA data{ positions.data(), 0, 0 };
        if (FAILED(device->CreateBuffer(&vb, &data, &vertex))) return false;
        D3D11_BUFFER_DESC ib{}; ib.ByteWidth = UINT(indices.size() * sizeof(unsigned));
        ib.Usage = D3D11_USAGE_IMMUTABLE; ib.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA init{ indices.data(), 0, 0 };
        if (FAILED(device->CreateBuffer(&ib, &init, &index))) { Release(vertex); return false; }
        count = UINT(indices.size()); return true;
    }
    bool Initialize() {
        if (sphereVs && orbPs && quadVs && haloPs && layout && vertex && index && constants && raster && solid && open && opaque)
            return true;
        if (!device || failed) return false;
        ID3DBlob* vs1{}; ID3DBlob* ps1{}; ID3DBlob* vs2{}; ID3DBlob* ps2{}; ID3DBlob* error{};
        bool ok = Compiled(vs1, "SphereVS", "vs_5_0", error) && Compiled(ps1, "OrbPS", "ps_5_0", error) &&
            Compiled(vs2, "QuadVS", "vs_5_0", error) && Compiled(ps2, "HaloPS", "ps_5_0", error);
        if (ok) ok = SUCCEEDED(device->CreateVertexShader(vs1->GetBufferPointer(), vs1->GetBufferSize(), nullptr, &sphereVs));
        if (ok) ok = SUCCEEDED(device->CreatePixelShader(ps1->GetBufferPointer(), ps1->GetBufferSize(), nullptr, &orbPs));
        if (ok) ok = SUCCEEDED(device->CreateVertexShader(vs2->GetBufferPointer(), vs2->GetBufferSize(), nullptr, &quadVs));
        if (ok) ok = SUCCEEDED(device->CreatePixelShader(ps2->GetBufferPointer(), ps2->GetBufferSize(), nullptr, &haloPs));
        D3D11_INPUT_ELEMENT_DESC elements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };
        if (ok) ok = SUCCEEDED(device->CreateInputLayout(elements, 1, vs1->GetBufferPointer(), vs1->GetBufferSize(), &layout));
        Release(error); Release(vs1); Release(ps1); Release(vs2); Release(ps2);
        if (ok && !vertex) ok = Mesh();
        if (ok) {
            D3D11_BUFFER_DESC cb{}; cb.ByteWidth = sizeof(Constants); cb.Usage = D3D11_USAGE_DYNAMIC;
            cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            ok = SUCCEEDED(device->CreateBuffer(&cb, nullptr, &constants));
        }
        if (ok) {
            D3D11_RASTERIZER_DESC rd{}; rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE;
            rd.DepthClipEnable = TRUE; ok = SUCCEEDED(device->CreateRasterizerState(&rd, &raster));
        }
        if (ok) {
            D3D11_DEPTH_STENCIL_DESC dd{}; dd.DepthEnable = TRUE; dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            dd.DepthFunc = D3D11_COMPARISON_LESS;
            ok = SUCCEEDED(device->CreateDepthStencilState(&dd, &solid));
            D3D11_DEPTH_STENCIL_DESC none{}; none.DepthEnable = FALSE;
            if (ok) ok = SUCCEEDED(device->CreateDepthStencilState(&none, &open));
        }
        // Os dois passes escrevem alfa nao pre-multiplicado porque o ImGui recompoem a imagem
        // com SRC_ALPHA: o halo cobre o alvo inteiro e a esfera vem depois, opaca, por cima.
        if (ok) {
            D3D11_BLEND_DESC bd{}; bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            ok = SUCCEEDED(device->CreateBlendState(&bd, &opaque));
        }
        if (!ok) { Clear(); failed = true; status = "Falha ao criar o orbe de Tsukuyomi"; return false; }
        return true;
    }
    bool Targets() {
        if (multiTarget && image && depthTarget) return true;
        Release(multi); Release(multiTarget); Release(resolved); Release(image); Release(depth); Release(depthTarget);
        D3D11_TEXTURE2D_DESC td{}; td.Width = td.Height = extent; td.MipLevels = td.ArraySize = 1;
        td.SampleDesc.Count = 4; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr = device->CreateTexture2D(&td, nullptr, &multi);
        if (SUCCEEDED(hr)) hr = device->CreateRenderTargetView(multi, nullptr, &multiTarget);
        td.SampleDesc.Count = 1; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (SUCCEEDED(hr)) hr = device->CreateTexture2D(&td, nullptr, &resolved);
        if (SUCCEEDED(hr)) hr = device->CreateShaderResourceView(resolved, nullptr, &image);
        td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; td.SampleDesc.Count = 4; td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        if (SUCCEEDED(hr)) hr = device->CreateTexture2D(&td, nullptr, &depth);
        if (SUCCEEDED(hr)) hr = device->CreateDepthStencilView(depth, nullptr, &depthTarget);
        if (FAILED(hr)) {
            Release(multi); Release(multiTarget); Release(resolved); Release(image); Release(depth); Release(depthTarget);
            status = "Falha nos alvos do orbe"; return false;
        }
        return true;
    }
    // O alvo resolvido nao pode estar amarrado a nenhum estagio no momento do resolve.
    // O slot zero de cada etapa e limpo logo antes, entso aqui so fiscalizamos os demais.
    bool Busy() {
        for (unsigned stage = 0; stage < 6; ++stage) {
            ID3D11ShaderResourceView* views[128]{};
            switch (stage) {
            case 0: context->VSGetShaderResources(0, 128, views); break;
            case 1: context->PSGetShaderResources(0, 128, views); break;
            case 2: context->GSGetShaderResources(0, 128, views); break;
            case 3: context->HSGetShaderResources(0, 128, views); break;
            case 4: context->DSGetShaderResources(0, 128, views); break;
            default: context->CSGetShaderResources(0, 128, views); break;
            }
            bool found = false;
            for (unsigned i = 1; i < 128; ++i) if (views[i]) {
                ID3D11Resource* resource{}; views[i]->GetResource(&resource);
                found = found || resource == resolved; Release(resource);
            }
            for (unsigned i = 0; i < 128; ++i) Release(views[i]);
            if (found) return true;
        }
        return false;
    }
    static XMFLOAT3 Normalize(const float* v) {
        float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        return l > 1e-5f ? XMFLOAT3{ v[0] / l, v[1] / l, v[2] / l } : XMFLOAT3{};
    }
    static float Dot(const XMFLOAT3& a, const XMFLOAT3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b) {
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }
    static XMFLOAT3 Perpendicular(const XMFLOAT3& axis, const XMFLOAT3& hint) {
        float k = Dot(axis, hint);
        XMFLOAT3 flat{ hint.x - axis.x * k, hint.y - axis.y * k, hint.z - axis.z * k };
        return Normalize(flat);
    }
    static XMFLOAT3 Normalize(const XMFLOAT3& v) {
        const float raw[3]{ v.x, v.y, v.z }; return Normalize(raw);
    }

public:
    ~Renderer() { Clear(); Release(context); Release(device); }
    void SetDevice(ID3D11Device* next) {
        if (device == next) return;
        Clear(); Release(context); Release(device);
        device = next;
        if (device) { device->AddRef(); device->GetImmediateContext(&context); }
        status = "Lua Tsukuyomi ociosa";
    }
    const char* Status() const { return status; }

    // ---- Oclusao por geometria ----
    // A thread de apresentacao apenas publica a base de medida; o trace nativo roda no frame
    // hook do jogo (SkinEngine.h), o mesmo caminho ja provado do ESP. A grade 11x11 vira alpha
    // por pixel no shader, entao a construcao come a fatia certa do disco em vez de apagar a
    // lua inteira.
    void RequestProbe(const float eye[3], const float dir[3], const float right[3], const float up[3],
                      float spread, uint64_t now) {
        if (!Finite(eye, 3) || !Finite(dir, 3) || !Finite(right, 3) || !Finite(up, 3)) return;
        const XMFLOAT3 d = Normalize(dir), r = Normalize(right), u = Normalize(up);
        if ((!d.x && !d.y && !d.z) || (!r.x && !r.y && !r.z)) return;
        const float lean = std::tan(std::clamp(spread, .02f, .45f));
        const HazzureGame::Vec3 next{ d.x, d.y, d.z }, pos{ eye[0], eye[1], eye[2] };
        AcquireSRWLockExclusive(&probeLock);
        const float dx = pos.x - probe.eye.x, dy = pos.y - probe.eye.y, dz = pos.z - probe.eye.z;
        const bool moved = !probe.armed || Dot(next, probe.dir) < 0.99995f ||
            dx * dx + dy * dy + dz * dz > 64.f || lean != probe.tan;
        probe.eye = pos;
        probe.dir = next;
        probe.right = { r.x, r.y, r.z };
        probe.up = { u.x, u.y, u.z };
        probe.tan = lean;
        // Re-varrer no maximo uma vez por janela: girando rapido a grade medida ha 90 ms ainda
        // vale, e o sweep recomeca de onde parou em vez de nunca fechar.
        if (moved && now >= probe.nextSweep) {
            for (unsigned i = 0; i < kMaskCells; ++i) probe.dirty[i] = MaskInside(i);
            probe.cursor = 0; probe.armed = true; probe.nextSweep = now + kSweepIntervalMs;
        }
        ReleaseSRWLockExclusive(&probeLock);
    }

    // Ate 12 celulas por chamada, com o mesmo orcamento de tempo do ESP: um sweep sao ~93
    // traces divididos em alguns frames. Unavailable mantem o ultimo valor da celula
    // (fail-open): sem prova a lua nao e punida.
    void ProcessPending(uintptr_t client) {
        const auto started = HazzurePerformance::Clock();
        unsigned traced = 0;
        while (traced < kRaysPerFrame) {
            HazzureGame::Vec3 start{}, end{};
            unsigned slot = kMaskCells;
            AcquireSRWLockExclusive(&probeLock);
            for (unsigned attempt = 0; attempt < kMaskCells; ++attempt) {
                const unsigned i = (probe.cursor + attempt) % kMaskCells;
                if (!probe.dirty[i]) continue;
                probe.dirty[i] = false;
                probe.cursor = (i + 1) % kMaskCells;
                slot = i;
                break;
            }
            if (slot < kMaskCells) {
                const float cu = 2.f * float(slot % kMaskSide) / float(kMaskSide - 1) - 1.f;
                const float cv = 2.f * float(slot / kMaskSide) / float(kMaskSide - 1) - 1.f;
                const float raw[3]{ probe.dir.x + probe.right.x * cu * probe.tan + probe.up.x * cv * probe.tan,
                                    probe.dir.y + probe.right.y * cu * probe.tan + probe.up.y * cv * probe.tan,
                                    probe.dir.z + probe.right.z * cu * probe.tan + probe.up.z * cv * probe.tan };
                const XMFLOAT3 ray = Normalize(raw);
                start = probe.eye;
                end = { start.x + ray.x * kProbeReach, start.y + ray.y * kProbeReach,
                        start.z + ray.z * kProbeReach };
            }
            ReleaseSRWLockExclusive(&probeLock);
            if (slot >= kMaskCells) break;
            const auto verdict = HazzureVisibility::SkyQuery(client, start, end);
            AcquireSRWLockExclusive(&probeLock);
            if (verdict == HazzureVisibility::Verdict::Blocked) probe.value[slot] = 0.f;
            else if (verdict == HazzureVisibility::Verdict::Visible) probe.value[slot] = 1.f;
            ReleaseSRWLockExclusive(&probeLock);
            ++traced;
            if (HazzurePerformance::Budget(started)) break;
        }
        if (!traced) return;
        float cells = 0.f, kept = 0.f;
        AcquireSRWLockExclusive(&probeLock);
        for (unsigned i = 0; i < kMaskCells; ++i) {
            if (!MaskInside(i)) continue;
            cells += 1.f; kept += probe.value[i];
        }
        ReleaseSRWLockExclusive(&probeLock);
        visibility.store(cells > 0.f ? kept / cells : 1.f, std::memory_order_relaxed);
    }

    // Seam da bancada de preview (preview/OrbPreview.cpp): preenche a grade sem client.dll
    // carregado; nullptr devolve o ceu limpo. No jogo quem escreve na grade e o ProcessPending.
    void SetMask(const float* coverage) {
        AcquireSRWLockExclusive(&probeLock);
        float cells = 0.f, kept = 0.f;
        for (unsigned i = 0; i < kMaskCells; ++i) {
            probe.value[i] = coverage ? std::clamp(coverage[i], 0.f, 1.f) : 1.f;
            if (!MaskInside(i)) continue;
            cells += 1.f; kept += probe.value[i];
        }
        ReleaseSRWLockExclusive(&probeLock);
        visibility.store(cells > 0.f ? kept / cells : 1.f, std::memory_order_relaxed);
    }

    float Visibility() const { return visibility.load(std::memory_order_relaxed); }

    // Devolve a SRV do disco ja sombreado; blitScale converte o raio do orbe em raio da imagem.
    ID3D11ShaderResourceView* Draw(const Input& in, float* blitScale) {
        if (blitScale) *blitScale = 1.f / cover;
        if (!context || !Valid(in)) { if (context) status = "Entrada do orbe rejeitada"; return nullptr; }
        const XMFLOAT3 dir = Normalize(in.direction);
        if (!dir.x && !dir.y && !dir.z) { status = "Direcao do orbe invalida"; return nullptr; }

        // Up da pre-visualizacao = up da camera contraido ao plano perpendicular ao orbe, entao
        // a imagem ja sai com a rolagem correta e o blit continua alinhado aos eixos da tela.
        XMFLOAT3 up = Perpendicular(dir, Normalize(in.up));
        if (!up.x && !up.y && !up.z) {
            const float fallback[3]{ -dir.x, -dir.y, dir.z };
            up = Perpendicular(dir, Normalize(fallback));
            if (!up.x && !up.y && !up.z) { status = "Base do orbe degenerada"; return nullptr; }
        }
        // Polo da iris inclinado para uma direcao fixa do mundo: ao girar no lugar o padrao gira
        // junto na tela, e e isso que o circulo plano de ImDrawList nunca conseguiu.
        const XMFLOAT3 sky{ 0.f, 0.f, 1.f }, east{ 1.f, 0.f, 0.f };
        XMFLOAT3 tangent = Cross(dir, std::fabs(Dot(dir, sky)) > 0.9f ? east : sky);
        tangent = Normalize(tangent);
        if (!tangent.x && !tangent.y && !tangent.z) { status = "Tangente do orbe degenerada"; return nullptr; }
        const float tilt = std::sin(in.tilt), kept = std::cos(in.tilt);
        // A face visivel do orbe e a que aponta para -dir (a camera esta desse lado), entao o
        // polo da iris tambem tem de apontar para la. Com o polo em +dir o Sharingan inteiro
        // ficava no hemisferio de tras e so o corpo escuro aparecia na tela.
        const XMFLOAT3 leaned{ -(dir.x * kept + tangent.x * tilt), -(dir.y * kept + tangent.y * tilt),
                               -(dir.z * kept + tangent.z * tilt) };
        const XMFLOAT3 axis = Normalize(leaned);
        XMFLOAT3 poleRight = Normalize(Cross(axis, std::fabs(axis.z) > 0.9f ? east : sky));
        XMFLOAT3 poleUp = Normalize(Cross(axis, poleRight));
        if (!poleRight.x && !poleRight.y && !poleRight.z) { status = "Base da iris degenerada"; return nullptr; }

        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(-dir.x * 10.f, -dir.y * 10.f, -dir.z * 10.f, 1.f),
            XMVectorZero(), XMVectorSet(up.x, up.y, up.z, 0.f));
        Constants data{};
        XMStoreFloat4x4(&data.mvp, view * XMMatrixOrthographicLH(2.f / cover, 2.f / cover, 0.1f, 100.f));
        if (!Finite(&data.mvp.m[0][0], 16)) { status = "Projecao do orbe invalida"; return nullptr; }
        data.axis = { axis.x, axis.y, axis.z, 0.f };
        data.poleUp = { poleUp.x, poleUp.y, poleUp.z, 0.f };
        data.poleRight = { poleRight.x, poleRight.y, poleRight.z, 0.f };
        // Lua cheia: o sol fica do lado OPOSTO do observador. Usar o hint do mapa direto faz o
        // hemisferio visivel cair no lado noturno e a esfera sair preta com uma borda vermelha
        // (foi exatamente o que a bancada de preview mostrou). Espelha-se o sol no plano
        // perpendicular ao eixo do orbe e mantem-se um desvio lateral para conservar o
        // terminador, o limb darkening e o especular de cornea.
        const float sunHint[3]{ 0.34f, 0.52f, 0.78f };
        const XMFLOAT3 sun = Normalize(sunHint);
        const float towards = Dot(sun, dir);
        const XMFLOAT3 mirrored{ sun.x - 2.f * towards * dir.x, sun.y - 2.f * towards * dir.y,
                                 sun.z - 2.f * towards * dir.z };
        const float deviation = 0.55f;
        const XMFLOAT3 leanedLight{ mirrored.x * std::cos(deviation) + tangent.x * std::sin(deviation),
                                    mirrored.y * std::cos(deviation) + tangent.y * std::sin(deviation),
                                    mirrored.z * std::cos(deviation) + tangent.z * std::sin(deviation) };
        const XMFLOAT3 light = Normalize(leanedLight);
        data.lightDir = { light.x, light.y, light.z, 0.f };
        data.eyeDir = { -dir.x, -dir.y, -dir.z, 0.f };
        data.params = { in.time, 0.11f, in.glow, 1.0f };
        data.surface = { float(extent), float(extent), cover, std::clamp(in.facing, 0.f, 1.f) };
        AcquireSRWLockExclusive(&probeLock);
        std::memcpy(&data.mask[0], probe.value, sizeof(float) * kMaskCells);
        ReleaseSRWLockExclusive(&probeLock);

        if (!Initialize() || !Targets()) return nullptr;
        if (Busy()) { status = "Orbe aguardando o contexto"; return image; }

        // Todo o estado do jogo e capturado e devolvido no fim do escopo: o overlay nao pode
        // deixar rasterizador, blend ou viewport do orbe amarrados no contexto do client.
        HazzureSceneColor::PipelineState saved(context);
        if (saved.streamOutput) { status = "Orbe aguardando stream output"; return image; }
        ID3D11Buffer* boundVertex{}; ID3D11Buffer* boundIndex{};
        UINT boundStride{}, boundOffset{}, boundIndexOffset{}; DXGI_FORMAT boundFormat{};
        context->IAGetVertexBuffers(0, 1, &boundVertex, &boundStride, &boundOffset);
        context->IAGetIndexBuffer(&boundIndex, &boundFormat, &boundIndexOffset);
        saved.changed = true;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(constants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            Release(boundVertex); Release(boundIndex); status = "Falha de constante do orbe"; return image;
        }
        std::memcpy(mapped.pData, &data, sizeof(data)); context->Unmap(constants, 0);

        const float background[4]{};
        context->OMSetRenderTargets(1, &multiTarget, depthTarget);
        context->ClearRenderTargetView(multiTarget, background);
        context->ClearDepthStencilView(depthTarget, D3D11_CLEAR_DEPTH, 1.f, 0);
        D3D11_VIEWPORT viewport{ 0, 0, float(extent), float(extent), 0.f, 1.f };
        context->RSSetViewports(1, &viewport);
        context->RSSetScissorRects(0, nullptr);
        context->RSSetState(raster);
        context->OMSetBlendState(opaque, nullptr, 0xffffffffu);
        context->VSSetConstantBuffers(0, 1, &constants);
        context->PSSetConstantBuffers(0, 1, &constants);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->OMSetDepthStencilState(open, 0);
        context->IASetInputLayout(nullptr);
        context->VSSetShader(quadVs, nullptr, 0); context->PSSetShader(haloPs, nullptr, 0);
        context->Draw(3, 0);
        context->OMSetDepthStencilState(solid, 0);
        context->IASetInputLayout(layout);
        context->VSSetShader(sphereVs, nullptr, 0); context->PSSetShader(orbPs, nullptr, 0);
        UINT stride = sizeof(XMFLOAT3), offset = 0;
        context->IASetVertexBuffers(0, 1, &vertex, &stride, &offset);
        context->IASetIndexBuffer(index, DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(count, 0, 0);

        ID3D11RenderTargetView* none[8]{};
        context->OMSetRenderTargets(8, none, nullptr);
        // O resolve nao pode escrever em uma textura ainda amarrada a algum estagio. Cada slot
        // zero e guardado, limpo, e devolvido logo depois, porque o ImGui usa justamente o slot
        // zero do pixel shader para exibir esta imagem.
        ID3D11ShaderResourceView* held[6]{};
        context->VSGetShaderResources(0, 1, &held[0]); context->PSGetShaderResources(0, 1, &held[1]);
        context->GSGetShaderResources(0, 1, &held[2]); context->HSGetShaderResources(0, 1, &held[3]);
        context->DSGetShaderResources(0, 1, &held[4]); context->CSGetShaderResources(0, 1, &held[5]);
        ID3D11ShaderResourceView* empty{};
        context->VSSetShaderResources(0, 1, &empty); context->PSSetShaderResources(0, 1, &empty);
        context->GSSetShaderResources(0, 1, &empty); context->HSSetShaderResources(0, 1, &empty);
        context->DSSetShaderResources(0, 1, &empty); context->CSSetShaderResources(0, 1, &empty);
        context->ResolveSubresource(resolved, 0, multi, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        context->VSSetShaderResources(0, 1, &held[0]); context->PSSetShaderResources(0, 1, &held[1]);
        context->GSSetShaderResources(0, 1, &held[2]); context->HSSetShaderResources(0, 1, &held[3]);
        context->DSSetShaderResources(0, 1, &held[4]); context->CSSetShaderResources(0, 1, &held[5]);
        for (auto*& slot : held) Release(slot);
        context->IASetVertexBuffers(0, 1, &boundVertex, &boundStride, &boundOffset);
        context->IASetIndexBuffer(boundIndex, boundFormat, boundIndexOffset);
        Release(boundVertex); Release(boundIndex);
        status = "Orbe 3D ativo";
        return image;
    }
};

inline Renderer renderer;
inline void SetDevice(ID3D11Device* device) { renderer.SetDevice(device); }
inline ID3D11ShaderResourceView* Draw(const Input& input, float* blitScale) { return renderer.Draw(input, blitScale); }
inline const char* Status() { return renderer.Status(); }
inline void RequestProbe(const float eye[3], const float dir[3], const float right[3], const float up[3],
                         float spread, uint64_t now) {
    renderer.RequestProbe(eye, dir, right, up, spread, now);
}
inline void ProcessPending(uintptr_t client) { renderer.ProcessPending(client); }
inline float Visibility() { return renderer.Visibility(); }
inline void SetMask(const float* coverage) { renderer.SetMask(coverage); }
}
