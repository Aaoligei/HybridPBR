#include "Application.h"

namespace HybridPBR {
    
    Application::Application() {
        window = std::make_unique<Window>();
    }
    
    Application::~Application() {
        Shutdown();
    }
    
    bool Application::Initialize() {
        LOG_INFO("Initializing HybridPBR Application");
        
        // 初始化窗口
        WindowConfig config;
        if (!window->Initialize(config)) {
            LOG_CRITICAL("Failed to initialize window");
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
            
            HandleEvents();
            OnUpdate(timer.GetDeltaTime());
            OnRender();
            
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