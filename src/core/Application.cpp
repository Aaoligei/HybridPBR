#include "Application.h"
#include "rendering/ImGuiManager.h" // 添加ImGui管理器头文件

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
            
            // 开始ImGui帧
            imguiManager->BeginFrame();
            
            HandleEvents();
            OnUpdate(timer.GetDeltaTime());
            OnRender();
            
            // 渲染ImGui界面
            OnImGuiRender();
            
            // 显示调试信息
            imguiManager->ShowDebugInfo(timer.GetDeltaTime(), timer.GetFPS());
            
            // 结束ImGui帧
            imguiManager->EndFrame();
            
            window->SwapBuffers();
            
            // 更新输入状态
            Input::GetInstance().Update();
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