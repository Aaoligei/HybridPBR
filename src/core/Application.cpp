#include "Application.h"
#include "Services.h"
#include "Events.h"
#include "utils/Logger.h"
#include <GLFW/glfw3.h>

namespace HybridPBR {

    Application::Application(const ApplicationConfig& config) 
        : config_(config), initialized_(false), isRunning_(false) {
    }

    Application::~Application() {
        Shutdown();
    }

    Result<void> Application::Initialize() {
        if (initialized_) {
            return Result<void>::Success();
        }

        LOG_INFO("Application", "Initializing application");

        // 注册核心服务
        RegisterServices();
        
        // 初始化服务
        RETURN_IF_ERROR(InitializeServices());

        // 创建窗口
        OnWindowConfigChanged();
        window_ = std::make_unique<Window>();
        
        if (!window_->Initialize(config_.window)) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Failed to initialize window"));
        }

        // 在OpenGL上下文创建后初始化渲染设备
        RETURN_IF_ERROR(Services::InitializeRenderDeviceAfterContext());
        
        // 初始化渲染器
        RETURN_IF_ERROR(Services::InitializeRenderer());

        // 初始化ImGui
        if (config_.enableImGui) {
            imguiManager_ = std::make_unique<ImGuiManager>();
            if (!imguiManager_->Initialize(window_->GetNativeWindow())) {
                LOG_WARNING("Application", "Failed to initialize ImGui");
            } else {
                // 注册ImGuiManager到服务定位器
                ServiceLocator::Register<ImGuiManager>(std::shared_ptr<ImGuiManager>(imguiManager_.get()));
            }
        }

        // 设置事件回调
        SetupEventCallbacks();

        // 调用用户初始化
        RETURN_IF_ERROR(OnInitialize());

        initialized_ = true;
        LOG_INFO("Application", "Application initialized successfully");

        return Result<void>::Success();
    }

    Result<void> Application::Run() {
        if (!initialized_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Application not initialized"));
        }

        LOG_INFO("Application", "Starting application main loop");
        isRunning_ = true;

        while (isRunning_ && !window_->ShouldClose()) {
            // 处理事件
            timer_.Tick();
            HandleEvents();

            // 更新
            float deltaTime = timer_.GetDeltaTime();
            
            RETURN_IF_ERROR(OnUpdate(deltaTime));

            // 渲染
            RETURN_IF_ERROR(BeginFrame());
            RETURN_IF_ERROR(OnRender());
            RETURN_IF_ERROR(EndFrame());

            // ImGui渲染
            if (imguiManager_) {
                imguiManager_->BeginFrame();
                RETURN_IF_ERROR(OnImGuiRender());
                imguiManager_->ShowDebugInfo(timer_.GetDeltaTime(), timer_.GetFPS());
                imguiManager_->EndFrame();
            }

            // 交换缓冲区
            window_->SwapBuffers();
        }

        LOG_INFO("Application", "Application main loop ended");
        return Result<void>::Success();
    }

    void Application::Shutdown() {
        if (!initialized_) {
            return;
        }

        LOG_INFO("Application", "Shutting down application");

        isRunning_ = false;

        // 调用用户清理
        OnShutdown();

        // 关闭ImGui
        if (imguiManager_) {
            imguiManager_->Shutdown();
            imguiManager_.reset();
        }

        // 关闭窗口
        if (window_) {
            window_->Shutdown();
            window_.reset();
        }

        // 关闭服务
        ShutdownServices();

        initialized_ = false;
        LOG_INFO("Application", "Application shutdown complete");
    }

    void Application::RegisterServices() {
        LOG_DEBUG("Application", "Registering core services");
        
        // 注册事件系统
        ServiceLocator::Register<IEventSystem>(std::make_shared<EventSystem>());
        
        // 注册其他核心服务将在Services.cpp中实现
    }

    Result<void> Application::InitializeServices() {
        LOG_DEBUG("Application", "Initializing services");
        
        // 初始化核心服务
        RETURN_IF_ERROR(Services::RegisterCoreServices());
        RETURN_IF_ERROR(Services::InitializeServices());
        
        return Result<void>::Success();
    }

    void Application::ShutdownServices() {
        LOG_DEBUG("Application", "Shutting down services");
        
        Services::ShutdownServices();
        ServiceLocator::Clear();
    }

    void Application::HandleEvents() {
        glfwPollEvents();
        
        // 处理输入事件
        Input::GetInstance().Update();
    }

    void Application::SetupEventCallbacks() {
        if (!window_) {
            return;
        }

        // 设置窗口回调
        window_->SetResizeCallback([this](int width, int height) {
            OnWindowResized(width, height);
        });

        window_->SetKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS) {
                OnKeyPressed(key);
            } else if (action == GLFW_RELEASE) {
                OnKeyReleased(key);
            }
        });

        window_->SetMouseCallback([this](double x, double y) {
            OnMouseMoved(x, y);
        });

        window_->SetMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS) {
                OnMouseClicked(button);
            } else if (action == GLFW_RELEASE) {
                OnMouseReleased(button);
            }
        });

        window_->SetScrollCallback([this](double xoffset, double yoffset) {
            OnMouseScroll(xoffset, yoffset);
        });
    }

    Result<void> Application::BeginFrame() {
        // 获取渲染器并开始帧
        auto renderer = GetService<IRenderer>();
        if (renderer) {
            return renderer->BeginFrame();
        }
        
        return Result<void>::Success();
    }

    Result<void> Application::EndFrame() {
        // 获取渲染器并结束帧
        auto renderer = GetService<IRenderer>();
        if (renderer) {
            return renderer->EndFrame();
        }
        
        return Result<void>::Success();
    }

    Result<void> Application::RenderFrame() {
        // 获取场景和渲染器
        auto sceneSystem = GetService<ISceneSystem>();
        auto renderer = GetService<IRenderer>();
        
        if (sceneSystem && renderer) {
            // 需要获取实际的Scene对象，这里暂时跳过
            // return renderer->Render(*scene);
        }
        
        return Result<void>::Success();
    }

} // namespace HybridPBR