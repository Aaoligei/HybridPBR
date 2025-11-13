#pragma once
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "utils/Logger.h"
#include <memory>

// 前向声明
namespace HybridPBR {
    class ImGuiManager;
}

namespace HybridPBR {
    
    class Application {
    public:
        Application();
        virtual ~Application();
        
        bool Initialize();
        void Run();
        void Shutdown();
        
        // 可重写的生命周期方法
        virtual void OnWindowConfigChanged() {}
        virtual bool OnInitialize() { return true; }
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnRender() {}
        virtual void OnShutdown() {}
        virtual void OnImGuiRender() {} // 新增ImGui渲染回调
        
        // 获取子系统
        Window& GetWindow() { return *window; }
        Input& GetInput() { return Input::GetInstance(); }
        Timer& GetTimer() { return timer; }

    protected:
        WindowConfig windowConfig;
        std::unique_ptr<Window> window;
        std::unique_ptr<ImGuiManager> imguiManager; // 添加ImGui管理器
        Timer timer;
        bool isRunning = false;
        
    private:
        void HandleEvents();
    };

} // namespace HybridPBR