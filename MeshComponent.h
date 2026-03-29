#pragma once

#include "GameComponent.h"
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>

namespace megaEngine {

    class MeshComponent : public GameComponent {
    public:
        enum class Type { Box, Sphere };

        MeshComponent(Type type);
        ~MeshComponent();

        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd) override;
        void Update(float deltaTime) override;
        void Render(ID3D11DeviceContext* context, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj) override;
        void Shutdown() override;

        // orbit/self rotation params
        // removed scalar selfSpeed here; use SetSelfRotationSpeeds to set per-axis speeds
        void SetOrbitParams(MeshComponent* parent, float radius, float orbitSpeed, const DirectX::XMFLOAT4& color);
        DirectX::XMFLOAT3 GetWorldPosition() const;

        // set per-axis self rotation speeds (radians per second)
        void SetSelfRotationSpeeds(const DirectX::XMFLOAT3& speeds) { selfRotSpeed_ = speeds; }

        // scale
        void SetScale(float s) { scale_ = s; }

    private:
        bool CompileShader(const wchar_t* filename, const char* entryPoint,
            const char* target, ID3DBlob** blob, D3D_SHADER_MACRO* macros = nullptr);

        void CreateBox(float size);
        void CreateSphere(float radius, int slices, int stacks);

        struct Vertex { DirectX::XMFLOAT4 pos; DirectX::XMFLOAT4 color; };

        Type type_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;

        ID3D11DeviceContext* context_ = nullptr;
        UINT stride_ = sizeof(Vertex);
        UINT offset_ = 0;
        UINT indexCount_ = 0;

        // transform
        DirectX::XMFLOAT3 position_ = { 0,0,0 };
        float orbitRadius_ = 0.0f;
        float orbitSpeed_ = 0.0f;
        // per-axis self rotation speeds and angles
        DirectX::XMFLOAT3 selfRotSpeed_ = {0,0,0};
        DirectX::XMFLOAT3 selfAngles_ = {0,0,0};
        float orbitAngle_ = 0.0f;
        MeshComponent* parent_ = nullptr;
        DirectX::XMFLOAT4 color_ = {1,1,1,1};

        // scale
        float scale_ = 1.0f;
    };
}
