#pragma once

#include <windows.h>
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include "GameComponent.h"
#include "DisplayWin32.h"
#include "InputDevice.h"
#include "Camera.h"

using megaEngine::GameComponent;
using megaEngine::DisplayWin32;
using megaEngine::InputDevice;
using megaEngine::Camera;

namespace game {

    class Game
    {
    public:
        Game();
        ~Game();

        bool Initialize(HINSTANCE hInstance);
        void Run();
        void Shutdown();

    private:
        bool InitializeDirect3D();
        void RenderFrame();
        void UpdateFPS(float deltaTime);
        bool ResizeResources(int width, int height); // handle window resize

        DisplayWin32 display_;
        InputDevice input_;

        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
        // Depth/stencil
        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilTexture_;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView_;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState_;

        std::vector<std::unique_ptr<GameComponent>> components_;

        int screenWidth_ = 800;
        int screenHeight_ = 800;

        // FPS counter
        float totalTime_ = 0.0f;
        unsigned int frameCount_ = 0;

        // Camera
        Camera camera_;
    };
}