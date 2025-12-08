#pragma once
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "ServiceLocator.h"
#include "Result.h"
#include "rendering/ImGuiManager.h"
#include "rendering/rasterization/CameraController.h"
#include "scene/Scene.h"
#include <memory>
#include <vector>

// 前向声明
namespace HybridPBR {
    class ImGuiManager;
    class IRenderDevice;
    class IRenderer;
    class IResourceManager;
    class ISceneSystem;
}

namespace HybridPBR {

    /**
     * @brief 应用程序配置
     * 定义应用程序的运行参数
     */
    struct ApplicationConfig {
        WindowConfig window;
        bool enableImGui = true;
        bool enableValidation = true;
        std::string logLevel = "Info";
        size_t memoryBudget = 1024 * 1024 * 1024; // 1GB
        bool enableAsyncLogging = true;
        std::string assetsPath = "assets/";
    };

    /**
     * @brief 应用程序基类
     * 提供统一的应用程序架构和生命周期管理
     * 
     * 修改理由：
     * 1. 使用依赖注入替代硬编码依赖
     * 2. 提供错误安全的初始化流程
     * 3. 支持模块化的子系统管理
     * 4. 实现可配置的应用程序行为
     */
    class Application {
    public:
        explicit Application(const ApplicationConfig& config = ApplicationConfig{});
        virtual ~Application();
        
        // 应用程序生命周期
        Result<void> Initialize();
        Result<void> Run();
        void Shutdown();
        
        // 子系统访问
        template<typename T>
        std::shared_ptr<T> GetService() const;
        
        Window& GetWindow() { return *window_; }
        Input& GetInput() { return Input::GetInstance(); }
        Timer& GetTimer() { return timer_; }
        
        // 配置
        const ApplicationConfig& GetConfig() const { return config_; }
        void SetConfig(const ApplicationConfig& config) { config_ = config; }
        
        // 状态查询
        bool IsRunning() const { return isRunning_; }
        bool IsInitialized() const { return initialized_; }

    protected:
        // 可重写的生命周期方法
        virtual Result<void> OnInitialize() { return Result<void>::Success(); }
        virtual Result<void> OnUpdate(float deltaTime) { return Result<void>::Success(); }
        virtual Result<void> OnRender() { return Result<void>::Success(); }
        virtual void OnShutdown() {}
        virtual Result<void> OnImGuiRender() { return Result<void>::Success(); }
        
        virtual void OnWindowConfigChanged() {}
        virtual void OnWindowResized(int width, int height) {}
        virtual void OnKeyPressed(int key) {}
        virtual void OnKeyReleased(int key) {}
        virtual void OnMouseMoved(double x, double y) {}
        virtual void OnMouseClicked(int button) {}
        virtual void OnMouseReleased(int button) {}
        virtual void OnMouseScroll(double xoffset, double yoffset) {}

    private:
        ApplicationConfig config_;
        bool initialized_ = false;
        bool isRunning_ = false;
        
        // 核心子系统
        std::unique_ptr<Window> window_;
        std::unique_ptr<ImGuiManager> imguiManager_;
        std::unique_ptr<CameraController> cameraController_;
        Timer timer_;
        
        // 服务注册
        void RegisterServices();
        Result<void> InitializeServices();
        void ShutdownServices();
        
        // 事件处理
        void HandleEvents();
        void SetupEventCallbacks();
        
        // 渲染循环
        Result<void> BeginFrame();
        Result<void> EndFrame();
        Result<void> RenderFrame();
    };

    // 模板方法实现
    template<typename T>
    std::shared_ptr<T> Application::GetService() const {
        return ServiceLocator::Resolve<T>();
    }

} // namespace HybridPBR