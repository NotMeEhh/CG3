#pragma once

#include "GameComponent.h"
#include "MeshComponent.h"
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <DirectXMath.h>

namespace megaEngine {
    class OrbitRenderer
    {
    public:
        OrbitRenderer();
        ~OrbitRenderer();

        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
        void SetTargets(const std::vector<MeshComponent*>& targets);
        void Render(ID3D11DeviceContext* context, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);
        void Shutdown();

    private:
        std::vector<MeshComponent*> targets_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_; // unit circle
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dssOff_;

        UINT segmentCount_ = 64;
    };
}
