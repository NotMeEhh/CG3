#include "OrbitRenderer.h"
#include <d3dcompiler.h>
#include <iostream>

using namespace DirectX;
namespace megaEngine {

    struct CBPerObject {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX proj;
        XMFLOAT4 color;
    };

    OrbitRenderer::OrbitRenderer() {}
    OrbitRenderer::~OrbitRenderer() { Shutdown(); }

    bool OrbitRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        // compile shaders
        ID3DBlob* vsBlob = nullptr;
        if (FAILED(D3DCompileFromFile(L"./Shaders/MyVeryFirstShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vsBlob, nullptr))) {
            std::cerr << "Failed to compile VS for OrbitRenderer" << std::endl;
            return false;
        }
        if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs_))) { vsBlob->Release(); return false; }

        ID3DBlob* psBlob = nullptr;
        if (FAILED(D3DCompileFromFile(L"./Shaders/MyVeryFirstShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &psBlob, nullptr))) {
            vsBlob->Release(); std::cerr << "Failed to compile PS for OrbitRenderer" << std::endl; return false;
        }
        if (FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps_))) { vsBlob->Release(); psBlob->Release(); return false; }

        // input layout (POSITION + COLOR)
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        if (FAILED(device->CreateInputLayout(layoutDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout_))) { vsBlob->Release(); psBlob->Release(); return false; }
        vsBlob->Release(); psBlob->Release();

        // create unit circle vertex buffer (positions with w=1)
        std::vector<XMFLOAT4> verts;
        float twoPi = XM_2PI;
        for (UINT i = 0; i <= segmentCount_; ++i) {
            float t = (float)i / (float)segmentCount_;
            float ang = t * twoPi;
            verts.push_back(XMFLOAT4(cosf(ang), 0.0f, sinf(ang), 1.0f));
            // append color per-vertex in-place (we will use constant color in CB, so duplicate color as white)
            verts.push_back(XMFLOAT4(1,1,1,1));
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(XMFLOAT4) * verts.size());
        D3D11_SUBRESOURCE_DATA sd = { verts.data(), 0, 0 };
        if (FAILED(device->CreateBuffer(&vbDesc, &sd, &vertexBuffer_))) return false;

        // constant buffer
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.ByteWidth = sizeof(CBPerObject);
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        if (FAILED(device->CreateBuffer(&cbDesc, nullptr, &constantBuffer_))) return false;

        // create depth-stencil state with depth disabled so orbits draw on top
        D3D11_DEPTH_STENCIL_DESC dsOff = {};
        dsOff.DepthEnable = FALSE;
        dsOff.StencilEnable = FALSE;
        if (FAILED(device->CreateDepthStencilState(&dsOff, & /* temporary */ *(ID3D11DepthStencilState**)(&dssOff_)))) {
            // fallback: ignore depth state creation failure
        }

        return true;
    }

    void OrbitRenderer::SetTargets(const std::vector<MeshComponent*>& targets)
    {
        targets_ = targets;
    }

    void OrbitRenderer::Render(ID3D11DeviceContext* context, const XMMATRIX& view, const XMMATRIX& proj)
    {
        if (!vertexBuffer_) return;

        // save previous depth stencil state
        ID3D11DepthStencilState* prevDSS = nullptr;
        UINT prevRef = 0;
        context->OMGetDepthStencilState(&prevDSS, &prevRef);

        // set depth stencil off if available
        if (dssOff_) context->OMSetDepthStencilState(dssOff_.Get(), 0);

        context->IASetInputLayout(layout_.Get());
        UINT stride = sizeof(XMFLOAT4) * 2;
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);
        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
        context->VSSetShader(vs_.Get(), nullptr, 0);
        context->PSSetShader(ps_.Get(), nullptr, 0);

        for (auto target : targets_) {
            if (!target) continue;
            float radius = target->GetOrbitRadius();
            if (radius <= 0.001f) continue;
            XMFLOAT3 center = {0,0,0};
            MeshComponent* parent = target->GetParent();
            if (parent) center = parent->GetWorldPosition();

            XMMATRIX world = XMMatrixScaling(radius, radius, radius) * XMMatrixTranslation(center.x, center.y, center.z);
            CBPerObject cb;
            cb.world = XMMatrixTranspose(world);
            cb.view = XMMatrixTranspose(view);
            cb.proj = XMMatrixTranspose(proj);
            cb.color = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);

            context->UpdateSubresource(constantBuffer_.Get(), 0, nullptr, &cb, 0, 0);
            context->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
            context->PSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());

            context->Draw(segmentCount_ + 1, 0);
        }

        // restore previous depth stencil state
        context->OMSetDepthStencilState(prevDSS, prevRef);
        if (prevDSS) prevDSS->Release();
    }

    void OrbitRenderer::Shutdown()
    {
        vertexBuffer_.Reset();
        vs_.Reset(); ps_.Reset(); layout_.Reset(); constantBuffer_.Reset();
        dssOff_.Reset();
        targets_.clear();
    }

}
