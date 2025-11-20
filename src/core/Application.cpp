#include "Application.h"
#include "rendering/ImGuiManager.h" // 添加ImGui管理器头文件
#include "rendering/rasterization/Camera.h"
#include "rendering/rasterization/CameraController.h"

namespace HybridPBR {
    
    Application::Application() {
        window = std::make_unique<Window>();
        imguiManager = std::make_unique<ImGuiManager>(); // 初始化ImGui管理器
    }
    
    Application::~Application() {
        Shutdown();
    }
    
    bool Application::Initialize() {
        LOG_INFO("Initializing HybridPBR Application");

        OnWindowConfigChanged();
        
        if (!window->Initialize(windowConfig)) {
            LOG_CRITICAL("Failed to initialize window");
            return false;
        }
        
        // 初始化ImGui
        if (!imguiManager->Initialize(window->GetNativeWindow())) {
            LOG_CRITICAL("Failed to initialize ImGui");
            return false;
        }
        
        // 设置窗口事件回调
        window->SetMouseCallback([this](double xpos, double ypos) {
            if (cameraController) {
                cameraController->OnMouseMove(xpos, ypos);
            }
        });
        
        window->SetMouseButtonCallback([this](int button, int action, int mods) {
            if (cameraController) {
                cameraController->OnMouseButton(button, action, mods);
            }
        });
        
        window->SetScrollCallback([this](double xoffset, double yoffset) {
            if (cameraController) {
                cameraController->OnMouseScroll(xoffset, yoffset);
            }
        });
        
        // 用户初始化
        if (!OnInitialize()) {
            LOG_CRITICAL("User initialization failed");
            return false;
        }
        
        isRunning = true;
        LOG_INFO("Application initialized successfully");
        return true;
    }
    
    void Application::Run() {
        LOG_INFO("Starting main loop");
        
        while (isRunning && !window->ShouldClose()) {
            timer.Tick();

            // 更新输入状态
            Input::GetInstance().Update();
 
            // 开始ImGui帧
            imguiManager->BeginFrame();
            
            HandleEvents();
            OnUpdate(timer.GetDeltaTime());
            
            // 更新相机控制器
            if (cameraController) {
                cameraController->Update(timer.GetDeltaTime());
            }
            
            OnRender();
            
            // 渲染ImGui界面
            OnImGuiRender();
            
            // 显示调试信息
            imguiManager->ShowDebugInfo(timer.GetDeltaTime(), timer.GetFPS());
            
            // 结束ImGui帧
            imguiManager->EndFrame();
            
            window->SwapBuffers();
            

        }
        
        LOG_INFO("Main loop ended");
    }
    
    void Application::Shutdown() {
        if (!isRunning) return;
        
        LOG_INFO("Shutting down application");
        
        OnShutdown();
        imguiManager->Shutdown(); // 关闭ImGui
        window->Shutdown();
        
        isRunning = false;
        LOG_INFO("Application shutdown complete");
    }
    
    void Application::HandleEvents() {
        window->ProcessEvents();
        
        // ESC键退出
        if (Input::GetInstance().IsKeyPressed(GLFW_KEY_ESCAPE)) {
            isRunning = false;
        }
    }

} // namespace HybridPBR