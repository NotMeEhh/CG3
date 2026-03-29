#include "MeshComponent.h"
#include <d3dcompiler.h>
#include <vector>
#include <iostream>

using namespace DirectX;
namespace megaEngine {

    struct CBPerObject {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX proj;
        XMFLOAT4 color;
    };

    MeshComponent::MeshComponent(Type type) : type_(type) {}
    MeshComponent::~MeshComponent() { Shutdown(); }

    bool MeshComponent::CompileShader(const wchar_t* filename, const char* entryPoint,
        const char* target, ID3DBlob** blob, D3D_SHADER_MACRO* macros)
    {
        ID3DBlob* errorBlob = nullptr;
        HRESULT res = D3DCompileFromFile(filename, macros, nullptr, entryPoint, target,
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0, blob, &errorBlob);

        if (FAILED(res)) {
            if (errorBlob) {
                std::cout << static_cast<char*>(errorBlob->GetBufferPointer()) << std::endl;
                errorBlob->Release();
            }
            else {
                std::wcout << L"Shader file not found: " << filename << std::endl;
            }
            return false;
        }
        return true;
    }

    void MeshComponent::CreateBox(float size)
    {
        float s = size * 0.5f;
        std::vector<Vertex> verts = {
            {{-s, -s, -s, 1}, color_}, {{s, -s, -s,1}, color_}, {{s,s,-s,1}, color_}, {{-s,s,-s,1}, color_},
            {{-s,-s,s,1}, color_}, {{s,-s,s,1}, color_}, {{s,s,s,1}, color_}, {{-s,s,s,1}, color_}
        };
        std::vector<UINT> indices = {
            0,1,2, 0,2,3, // back
            4,6,5, 4,7,6, // front
            4,5,1, 4,1,0, // bottom
            3,2,6, 3,6,7, // top
            1,5,6, 1,6,2, // right
            4,0,3, 4,3,7  // left
        };

        indexCount_ = (UINT)indices.size();

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * verts.size());

        D3D11_SUBRESOURCE_DATA vbData = { verts.data(), 0,0 };
        ID3D11Device* devicePtr = nullptr;
        context_->GetDevice(&devicePtr);
        devicePtr->CreateBuffer(&vbDesc, &vbData, vertexBuffer_.GetAddressOf());
        devicePtr->Release();

        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
        D3D11_SUBRESOURCE_DATA ibData = { indices.data(),0,0 };
        context_->GetDevice(&devicePtr);
        devicePtr->CreateBuffer(&ibDesc, &ibData, indexBuffer_.GetAddressOf());
        devicePtr->Release();
    }

    void MeshComponent::CreateSphere(float radius, int slices, int stacks)
    {
        std::vector<Vertex> verts;
        std::vector<UINT> indices;

        for (int y = 0; y <= stacks; ++y) {
            float v = (float)y / stacks;
            float phi = v * XM_PI;
            for (int x = 0; x <= slices; ++x) {
                float u = (float)x / slices;
                float theta = u * XM_2PI;
                float sx = sinf(phi) * cosf(theta);
                float sy = cosf(phi);
                float sz = sinf(phi) * sinf(theta);
                verts.push_back({ XMFLOAT4(radius * sx, radius * sy, radius * sz, 1.0f), color_ });
            }
        }

        for (int y = 0; y < stacks; ++y) {
            for (int x = 0; x < slices; ++x) {
                int i0 = y * (slices + 1) + x;
                int i1 = i0 + 1;
                int i2 = i0 + (slices + 1);
                int i3 = i2 + 1;
                indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
                indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
            }
        }

        indexCount_ = (UINT)indices.size();

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * verts.size());
        D3D11_SUBRESOURCE_DATA vbData = { verts.data(),0,0 };
        ID3D11Device* devicePtr = nullptr;
        context_->GetDevice(&devicePtr);
        devicePtr->CreateBuffer(&vbDesc, &vbData, vertexBuffer_.GetAddressOf());
        devicePtr->Release();

        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
        D3D11_SUBRESOURCE_DATA ibData = { indices.data(),0,0 };
        context_->GetDevice(&devicePtr);
        devicePtr->CreateBuffer(&ibDesc, &ibData, indexBuffer_.GetAddressOf());
        devicePtr->Release();
    }

    bool MeshComponent::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd)
    {
        context_ = context;
        ID3DBlob* vsBlob = nullptr;
        if (!CompileShader(L"./Shaders/MyVeryFirstShader.hlsl", "VSMain", "vs_5_0", &vsBlob)) return false;
        if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader_))) return false;

        ID3DBlob* psBlob = nullptr;
        if (!CompileShader(L"./Shaders/MyVeryFirstShader.hlsl", "PSMain", "ps_5_0", &psBlob)) { vsBlob->Release(); return false; }
        if (FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader_))) { vsBlob->Release(); psBlob->Release(); return false; }

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        if (FAILED(device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout_))) { vsBlob->Release(); psBlob->Release(); return false; }
        vsBlob->Release(); psBlob->Release();

        // create geometry
        if (type_ == Type::Box) CreateBox(1.0f);
        else CreateSphere(0.5f, 16, 16);

        // rasterizer
        CD3D11_RASTERIZER_DESC rsDesc(D3D11_DEFAULT);
        rsDesc.CullMode = D3D11_CULL_NONE;
        if (FAILED(device->CreateRasterizerState(&rsDesc, &rasterizerState_))) return false;

        // constant buffer
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.ByteWidth = sizeof(CBPerObject);
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        if (FAILED(device->CreateBuffer(&cbDesc, nullptr, &constantBuffer_))) return false;

        return true;
    }

    void MeshComponent::SetOrbitParams(MeshComponent* parent, float radius, float orbitSpeed, const XMFLOAT4& color)
    {
        parent_ = parent;
        orbitRadius_ = radius;
        orbitSpeed_ = orbitSpeed;
        color_ = color;
    }

    void MeshComponent::Update(float deltaTime)
    {
        orbitAngle_ += orbitSpeed_ * deltaTime;
        // update per-axis self rotation angles
        selfAngles_.x += selfRotSpeed_.x * deltaTime;
        selfAngles_.y += selfRotSpeed_.y * deltaTime;
        selfAngles_.z += selfRotSpeed_.z * deltaTime;

        XMFLOAT3 center = { 0,0,0 };
        if (parent_) center = parent_->GetWorldPosition();
        position_.x = center.x + orbitRadius_ * cosf(orbitAngle_);
        position_.y = center.y;
        position_.z = center.z + orbitRadius_ * sinf(orbitAngle_);
    }

    XMFLOAT3 MeshComponent::GetWorldPosition() const { return position_; }

    void MeshComponent::Render(ID3D11DeviceContext* context, const XMMATRIX& view, const XMMATRIX& proj)
    {
        context->RSSetState(rasterizerState_.Get());
        context->IASetInputLayout(inputLayout_.Get());
        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->IASetIndexBuffer(indexBuffer_.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride_, &offset_);
        context->VSSetShader(vertexShader_.Get(), nullptr, 0);
        context->PSSetShader(pixelShader_.Get(), nullptr, 0);

        // build rotation from X, Y, Z self angles (roll-pitch-yaw)
        XMMATRIX rot = XMMatrixRotationX(selfAngles_.x) * XMMatrixRotationY(selfAngles_.y) * XMMatrixRotationZ(selfAngles_.z);
        XMMATRIX world = XMMatrixScaling(scale_, scale_, scale_) * rot * XMMatrixTranslation(position_.x, position_.y, position_.z);
        CBPerObject cb;
        cb.world = XMMatrixTranspose(world);
        cb.view = XMMatrixTranspose(view);
        cb.proj = XMMatrixTranspose(proj);
        cb.color = color_;

        context->UpdateSubresource(constantBuffer_.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());

        context->DrawIndexed(indexCount_, 0, 0);
    }

    void MeshComponent::Shutdown()
    {
        vertexBuffer_.Reset(); indexBuffer_.Reset(); vertexShader_.Reset(); pixelShader_.Reset(); inputLayout_.Reset(); rasterizerState_.Reset(); constantBuffer_.Reset();
    }

}
