#pragma once
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "utils/Logger.h"
#include <memory>

namespace HybridPBR {
    
    class Application {
    public:
        Application();
        virtual ~Application();
        
        bool Initialize();
        void Run();
        void Shutdown();
        
        // 可重写的生命周期方法
        virtual bool OnInitialize() { return true; }
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnRender() {}
        virtual void OnShutdown() {}
        
        // 获取子系统
        Window& GetWindow() { return *window; }
        Input& GetInput() { return Input::GetInstance(); }
        Timer& GetTimer() { return timer; }

    protected:
        std::unique_ptr<Window> window;
        Timer timer;
        bool isRunning = false;
        
    private:
        void HandleEvents();
    };

} // namespace HybridPBR