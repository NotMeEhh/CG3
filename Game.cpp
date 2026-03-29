#include "Game.h"
#include "MeshComponent.h"
#include "OrbitRenderer.h"

#include <dxgi.h>
#include <chrono>
#include <iostream>
#include <windows.h>
#include <WinUser.h>
#include <wrl.h>
#include <d3d.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <random>


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")


using megaEngine::MeshComponent;
namespace game {

    Game::Game() = default;
    Game::~Game() { Shutdown(); }

    bool Game::Initialize(HINSTANCE hInstance)
    {
        if (!display_.Initialize(L"My3DApp", screenWidth_, screenHeight_, hInstance, &input_))
            return false;

        if (!InitializeDirect3D()) return false;

        // Initialize camera
        camera_.SetPerspective(DirectX::XM_PIDIV4, static_cast<float>(screenWidth_) / screenHeight_, 0.1f, 1000.0f);

        // random generator for varied speeds
        std::random_device rd;
        std::mt19937 rng(rd());
        // reduced ranges: max speeds halved
        std::uniform_real_distribution<float> orbitSpeedDist(0.05f, 1.5f); // was 3.0f
        std::uniform_real_distribution<float> selfSpeedDist(0.1f, 3.0f);   // was 6.0f
        std::uniform_real_distribution<float> radiusDist(2.0f, 14.0f);
        // reduce maximum scale by half
        std::uniform_real_distribution<float> scaleDist(0.3f, 1.75f);
        // per-axis self-rotation speeds (radians/sec), allow negative for opposite direction
        std::uniform_real_distribution<float> selfAxisDist(-1.5f, 1.5f); // was -3.0f..3.0f

        // Create several planets and moons with varied speeds
        // Planet 0 (central) - slow self-rotation
        auto planet0 = std::make_unique<MeshComponent>(MeshComponent::Type::Sphere);
        if (!planet0->Initialize(device_.Get(), context_.Get(), display_.GetHwnd())) return false;
        planet0->SetOrbitParams(nullptr, 0.0f, 0.0f, DirectX::XMFLOAT4(0.8f,0.3f,0.3f,1.0f));
        planet0->SetSelfRotationSpeeds(DirectX::XMFLOAT3(selfAxisDist(rng)*0.1f, selfAxisDist(rng)*0.05f, selfAxisDist(rng)*0.08f));
        planet0->SetScale(scaleDist(rng) * 1.8f); // central larger
        components_.push_back(std::move(planet0));

        // Planet 1
        auto planet1 = std::make_unique<MeshComponent>(MeshComponent::Type::Box);
        if (!planet1->Initialize(device_.Get(), context_.Get(), display_.GetHwnd())) return false;
        planet1->SetOrbitParams(nullptr, 5.0f, orbitSpeedDist(rng), DirectX::XMFLOAT4(0.3f,0.8f,0.3f,1.0f));
        planet1->SetSelfRotationSpeeds(DirectX::XMFLOAT3(selfAxisDist(rng), selfAxisDist(rng), selfAxisDist(rng)));
        planet1->SetScale(scaleDist(rng));
        components_.push_back(std::move(planet1));

        // Planet 2
        auto planet2 = std::make_unique<MeshComponent>(MeshComponent::Type::Sphere);
        if (!planet2->Initialize(device_.Get(), context_.Get(), display_.GetHwnd())) return false;
        planet2->SetOrbitParams(nullptr, 8.0f, orbitSpeedDist(rng) * 0.5f, DirectX::XMFLOAT4(0.3f,0.3f,0.8f,1.0f));
        planet2->SetSelfRotationSpeeds(DirectX::XMFLOAT3(selfAxisDist(rng)*0.5f, selfAxisDist(rng)*0.7f, selfAxisDist(rng)*0.2f));
        planet2->SetScale(scaleDist(rng) * 1.2f);
        components_.push_back(std::move(planet2));

        // Add moons for planet1 with faster orbit around planet1
        MeshComponent* p1 = static_cast<MeshComponent*>(components_[1].get());
        auto moon1 = std::make_unique<MeshComponent>(MeshComponent::Type::Sphere);
        if (!moon1->Initialize(device_.Get(), context_.Get(), display_.GetHwnd())) return false;
        moon1->SetOrbitParams(p1, 1.2f, orbitSpeedDist(rng) * 2.5f, DirectX::XMFLOAT4(0.8f,0.8f,0.3f,1.0f));
        moon1->SetSelfRotationSpeeds(DirectX::XMFLOAT3(selfAxisDist(rng)*2.0f, selfAxisDist(rng)*1.2f, selfAxisDist(rng)*0.8f));
        moon1->SetScale(scaleDist(rng) * 0.6f);
        components_.push_back(std::move(moon1));

        auto moon2 = std::make_unique<MeshComponent>(MeshComponent::Type::Box);
        if (!moon2->Initialize(device_.Get(), context_.Get(), display_.GetHwnd())) return false;
        moon2->SetOrbitParams(p1, 2.2f, orbitSpeedDist(rng) * 2.0f, DirectX::XMFLOAT4(0.7f,0.5f,0.7f,1.0f));
        moon2->SetSelfRotationSpeeds(DirectX::XMFLOAT3(selfAxisDist(rng)*1.0f, selfAxisDist(rng)*1.5f, selfAxisDist(rng)*0.6f));
        moon2->SetScale(scaleDist(rng) * 0.9f);
        components_.push_back(std::move(moon2));

        // Add additional objects to reach 10+ with varied radii and speeds
        for (int i = 0; i < 5; ++i) {
            auto obj = std::make_unique<MeshComponent>((i % 2 == 0) ? MeshComponent::Type::Box : MeshComponent::Type::Sphere);
            if (!obj->Initialize(device_.Get(), context_.Get(), display_.GetHwnd())) return false;
            float radius = radiusDist(rng) + i * 2.0f;
            float orbitS = orbitSpeedDist(rng) * (0.3f + i * 0.4f);
            // choose per-axis self-rotation speeds
            DirectX::XMFLOAT3 self = DirectX::XMFLOAT3(selfAxisDist(rng) * (0.2f + i * 0.5f), selfAxisDist(rng) * (0.1f + i * 0.6f), selfAxisDist(rng) * (0.3f + i * 0.4f));
            obj->SetOrbitParams(nullptr, radius, orbitS, DirectX::XMFLOAT4(0.4f + i*0.1f,0.2f,0.5f,1.0f));
            obj->SetSelfRotationSpeeds(self);
            obj->SetScale(scaleDist(rng) * (0.5f + i * 0.4f));
            components_.push_back(std::move(obj));
        }

        // Create orbit renderer and set targets (all mesh components)
        orbitRenderer_ = std::make_unique<megaEngine::OrbitRenderer>();
        if (!orbitRenderer_->Initialize(device_.Get(), context_.Get())) return false;
        // collect mesh component pointers
        std::vector<megaEngine::MeshComponent*> meshTargets;
        for (auto& c : components_) {
            auto m = dynamic_cast<megaEngine::MeshComponent*>(c.get());
            if (m) meshTargets.push_back(m);
        }
        orbitRenderer_->SetTargets(meshTargets);

        return true;
    }

    bool Game::InitializeDirect3D()
    {
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

        DXGI_SWAP_CHAIN_DESC swapDesc = {};
        swapDesc.BufferCount = 2;
        swapDesc.BufferDesc.Width = screenWidth_;
        swapDesc.BufferDesc.Height = screenHeight_;
        swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapDesc.BufferDesc.RefreshRate = { 60, 1 };
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.OutputWindow = display_.GetHwnd();
        swapDesc.Windowed = TRUE;
        swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapDesc.SampleDesc.Count = 1;

        HRESULT res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
            D3D11_SDK_VERSION, &swapDesc, &swapChain_, &device_, nullptr, &context_);

        if (FAILED(res)) { std::cerr << "D3D11CreateDeviceAndSwapChain failed: " << res << std::endl; return false; }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) return false;
        if (FAILED(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView_))) return false;

        // Create depth/stencil texture
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = screenWidth_;
        depthDesc.Height = screenHeight_;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        if (FAILED(device_->CreateTexture2D(&depthDesc, nullptr, &depthStencilTexture_))) return false;
        if (FAILED(device_->CreateDepthStencilView(depthStencilTexture_.Get(), nullptr, &depthStencilView_))) return false;

        // Create depth stencil state
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        dsDesc.StencilEnable = FALSE;

        if (FAILED(device_->CreateDepthStencilState(&dsDesc, &depthStencilState_))) return false;

        // Bind render target and depth stencil
        context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());
        context_->OMSetDepthStencilState(depthStencilState_.Get(), 0);

        return true;
    }

    bool Game::ResizeResources(int width, int height)
    {
        if (width <= 0 || height <= 0) return false;
        if (!swapChain_) return false;

        screenWidth_ = width;
        screenHeight_ = height;

        // release old
        renderTargetView_.Reset();
        depthStencilView_.Reset();
        depthStencilTexture_.Reset();

        HRESULT hr = swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr)) return false;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) return false;
        if (FAILED(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView_))) return false;

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        if (FAILED(device_->CreateTexture2D(&depthDesc, nullptr, &depthStencilTexture_))) return false;
        if (FAILED(device_->CreateDepthStencilView(depthStencilTexture_.Get(), nullptr, &depthStencilView_))) return false;

        context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());

        // update camera aspect
        camera_.SetAspect(static_cast<float>(width) / static_cast<float>(height));

        return true;
    }

    void Game::Run()
    {
        auto prevTime = std::chrono::steady_clock::now();

        std::cout << "display_.IsRunning(): "<<display_.IsRunning()<<"\n";

        bool prevC = false;
        bool prevP = false;
        bool prevO = false;
        bool prevL = false;

        while (display_.IsRunning())
        {
            if (!display_.ProcessMessages(&input_)) break;

            auto curTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(curTime - prevTime).count();
            prevTime = curTime;

            // detect window size change
            RECT rc; GetClientRect(display_.GetHwnd(), &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (w != screenWidth_ || h != screenHeight_) {
                ResizeResources(w, h);
            }

            // handle camera toggles on key edges
            bool cPressed = input_.IsKeyPressed('C');
            if (cPressed && !prevC) camera_.ToggleMode();
            prevC = cPressed;

            bool pPressed = input_.IsKeyPressed('P');
            if (pPressed && !prevP) camera_.CyclePerspective();
            prevP = pPressed;

            bool oPressed = input_.IsKeyPressed('O');
            if (oPressed && !prevO) camera_.ToggleProjection();
            prevO = oPressed;

            bool lPressed = input_.IsKeyPressed('L');
            if (lPressed && !prevL) showOrbits_ = !showOrbits_;
            prevL = lPressed;

            camera_.Update(deltaTime, input_);

            for (auto& comp : components_) comp->Update(deltaTime);
            UpdateFPS(deltaTime);

            RenderFrame();
        }
    }

    void Game::RenderFrame()
    {
        // fixed dark background color instead of pulsing red
        float clearColor[] = { 0.05f, 0.06f, 0.09f, 1.0f };
        context_->ClearState();
        context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);
        if (depthStencilView_) context_->ClearDepthStencilView(depthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        D3D11_VIEWPORT vp = { 0, 0, static_cast<float>(screenWidth_),
                             static_cast<float>(screenHeight_), 0.0f, 1.0f };
        context_->RSSetViewports(1, &vp);
        context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());

        // prepare camera matrices
        auto view = camera_.GetViewMatrix();
        auto proj = camera_.GetProjMatrix();

        // If camera is orthographic and in FPS mode, draw orbits first
        if (orbitRenderer_ && showOrbits_) {
            orbitRenderer_->Render(context_.Get(), view, proj);
        }

        for (auto& comp : components_) comp->Render(context_.Get(), view, proj);

        swapChain_->Present(1, 0);
    }

    void Game::UpdateFPS(float deltaTime)
    {
        totalTime_ += deltaTime;
        frameCount_++;

        if (totalTime_ >= 1.0f) {
            float fps = frameCount_ / totalTime_;
            wchar_t title[128];
            swprintf_s(title, L"My3DApp - FPS: %.1f", fps);
            SetWindowText(display_.GetHwnd(), title);

            totalTime_ = 0.0f;
            frameCount_ = 0;
        }
    }

    void Game::Shutdown()
    {
        if (orbitRenderer_) orbitRenderer_->Shutdown();
        orbitRenderer_.reset();

        for (auto& comp : components_) comp->Shutdown();
        components_.clear();

        renderTargetView_.Reset();
        depthStencilView_.Reset();
        depthStencilTexture_.Reset();
        depthStencilState_.Reset();
        context_.Reset();
        device_.Reset();
        swapChain_.Reset();

        display_.Shutdown();
    }

}