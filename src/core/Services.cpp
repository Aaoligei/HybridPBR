#include "Services.h"
#include "ServiceLocator.h"
#include "rendering/OpenGLRenderDevice.h"
#include "rendering/HybridRenderer.h"
#include "resources/ResourceManager.h"
#include "scene/Scene.h"
#include "utils/Logger.h"

namespace HybridPBR {

    bool Services::servicesRegistered_ = false;
    bool Services::servicesInitialized_ = false;

    Result<void> Services::RegisterCoreServices() {
        if (servicesRegistered_) {
            return Result<void>::Success();
        }

        LOG_INFO("Services", "Registering core services");

        // 注册渲染设备
        auto renderDevice = std::make_shared<OpenGLRenderDevice>();
        ServiceLocator::Register<IRenderDevice>(renderDevice);

        // 注册资源管理器
        auto resourceManager = std::make_shared<ResourceManager>();
        ServiceLocator::Register<IResourceManager>(resourceManager);

        // 注册场景系统
        auto sceneSystem = std::make_shared<SceneSystem>();
        ServiceLocator::Register<ISceneSystem>(sceneSystem);

        servicesRegistered_ = true;
        LOG_INFO("Services", "Core services registered");

        return Result<void>::Success();
    }

    Result<void> Services::RegisterRenderServices() {
        if (!servicesRegistered_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Core services not registered"));
        }

        LOG_INFO("Services", "Registering render services");

        // 注册渲染器
        auto renderer = std::make_shared<HybridRenderer>();
        ServiceLocator::Register<IRenderer>(renderer);

        LOG_INFO("Services", "Render services registered");

        return Result<void>::Success();
    }

    Result<void> Services::RegisterResourceServices() {
        // 资源服务已在核心服务中注册
        return Result<void>::Success();
    }

    Result<void> Services::RegisterSceneServices() {
        // 场景服务已在核心服务中注册
        return Result<void>::Success();
    }

    Result<void> Services::InitializeServices() {
        if (servicesInitialized_) {
            return Result<void>::Success();
        }

        LOG_INFO("Services", "Initializing services");

        // 按顺序初始化服务（跳过渲染设备初始化，将在上下文创建后进行）
        RETURN_IF_ERROR(InitializeResourceManager());
        RETURN_IF_ERROR(InitializeSceneSystem());

        servicesInitialized_ = true;
        LOG_INFO("Services", "All services initialized successfully");

        return Result<void>::Success();
    }

    void Services::ShutdownServices() {
        if (!servicesInitialized_) {
            return;
        }

        LOG_INFO("Services", "Shutting down services");

        // 按相反顺序关闭服务
        auto renderer = GetRenderer();
        if (renderer) {
            renderer->Shutdown();
        }

        auto sceneSystem = GetSceneSystem();
        if (sceneSystem) {
            sceneSystem->Shutdown();
        }

        auto resourceManager = GetResourceManager();
        if (resourceManager) {
            resourceManager->UnloadAll();
        }

        auto renderDevice = GetRenderDevice();
        if (renderDevice) {
            renderDevice->Shutdown();
        }

        servicesInitialized_ = false;
        LOG_INFO("Services", "All services shutdown");
    }

    std::shared_ptr<IRenderDevice> Services::GetRenderDevice() {
        return ServiceLocator::Resolve<IRenderDevice>();
    }

    std::shared_ptr<IRenderer> Services::GetRenderer() {
        return ServiceLocator::Resolve<IRenderer>();
    }

    std::shared_ptr<IResourceManager> Services::GetResourceManager() {
        return ServiceLocator::Resolve<IResourceManager>();
    }

    std::shared_ptr<ISceneSystem> Services::GetSceneSystem() {
        return ServiceLocator::Resolve<ISceneSystem>();
    }

    Result<void> Services::InitializeRenderDevice() {
        auto renderDevice = GetRenderDevice();
        if (!renderDevice) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Render device not registered"));
        }

        return renderDevice->Initialize();
    }
    
    Result<void> Services::InitializeRenderDeviceAfterContext() {
        auto renderDevice = GetRenderDevice();
        if (!renderDevice) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Render device not registered"));
        }
        
        // 首先调用基础初始化
        auto initResult = renderDevice->Initialize();
        if (initResult.IsFailure()) {
            return initResult;
        }
        
        auto glDevice = std::dynamic_pointer_cast<OpenGLRenderDevice>(renderDevice);
        if (!glDevice) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Render device is not OpenGL device"));
        }

        return glDevice->InitializeAfterContext();
    }

    Result<void> Services::InitializeRenderer() {
        // 注册并初始化渲染服务
        RETURN_IF_ERROR(RegisterRenderServices());

        auto renderer = GetRenderer();
        if (!renderer) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Renderer not registered"));
        }

        auto renderDevice = GetRenderDevice();
        if (!renderDevice) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Render device not initialized"));
        }

        return renderer->Initialize(renderDevice);
    }

    Result<void> Services::InitializeResourceManager() {
        auto resourceManager = GetResourceManager();
        if (!resourceManager) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Resource manager not registered"));
        }

        // ResourceManager不需要显式初始化
        return Result<void>::Success();
    }

    Result<void> Services::InitializeSceneSystem() {
        auto sceneSystem = GetSceneSystem();
        if (!sceneSystem) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Scene system not registered"));
        }

        return sceneSystem->Initialize();
    }

} // namespace HybridPBR