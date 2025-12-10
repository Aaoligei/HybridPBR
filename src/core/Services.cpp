#include "Services.h"
#include "ServiceLocator.h"
// [修改] 移除 OpenGLRenderDevice.h
#include "rendering/HybridRenderer.h"
#include "resources/ResourceManager.h"
#include "scene/Scene.h"
#include "utils/Logger.h"

namespace HybridPBR {

    bool Services::servicesRegistered_ = false;
    bool Services::servicesInitialized_ = false;

    Result<void> Services::RegisterCoreServices() {
        if (servicesRegistered_) return Result<void>::Success();

        LOG_INFO("Services", "Registering core services");

        // [修改] 移除 IRenderDevice 注册
        
        auto resourceManager = std::make_shared<ResourceManager>();
        ServiceLocator::Register<IResourceManager>(resourceManager);

        auto sceneSystem = std::make_shared<SceneSystem>();
        ServiceLocator::Register<ISceneSystem>(sceneSystem);

        servicesRegistered_ = true;
        return Result<void>::Success();
    }

    Result<void> Services::RegisterRenderServices() {
        if (!servicesRegistered_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Core services not registered"));
        }

        LOG_INFO("Services", "Registering render services");

        auto renderer = std::make_shared<HybridRenderer>();
        ServiceLocator::Register<IRenderer>(renderer);

        return Result<void>::Success();
    }

    Result<void> Services::InitializeServices() {
        if (servicesInitialized_) return Result<void>::Success();

        LOG_INFO("Services", "Initializing services");
        RETURN_IF_ERROR(InitializeResourceManager());
        RETURN_IF_ERROR(InitializeSceneSystem());

        servicesInitialized_ = true;
        return Result<void>::Success();
    }

    void Services::ShutdownServices() {
        if (!servicesInitialized_) return;

        LOG_INFO("Services", "Shutting down services");

        auto renderer = GetRenderer();
        if (renderer) renderer->Shutdown();

        auto sceneSystem = GetSceneSystem();
        if (sceneSystem) sceneSystem->Shutdown();

        auto resourceManager = GetResourceManager();
        if (resourceManager) resourceManager->UnloadAll();

        // [修改] 移除 RenderDevice shutdown
        
        servicesInitialized_ = false;
    }

    // [修改] 移除 GetRenderDevice

    std::shared_ptr<IRenderer> Services::GetRenderer() {
        return ServiceLocator::Resolve<IRenderer>();
    }

    std::shared_ptr<IResourceManager> Services::GetResourceManager() {
        return ServiceLocator::Resolve<IResourceManager>();
    }

    std::shared_ptr<ISceneSystem> Services::GetSceneSystem() {
        return ServiceLocator::Resolve<ISceneSystem>();
    }

    // [修改] 移除 InitializeRenderDevice 和 InitializeRenderDeviceAfterContext

    Result<void> Services::InitializeRenderer() {
        RETURN_IF_ERROR(RegisterRenderServices());

        auto renderer = GetRenderer();
        if (!renderer) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Renderer not registered"));
        }

        // [修改] 直接调用无参 Initialize
        return renderer->Initialize();
    }

    Result<void> Services::InitializeResourceManager() {
        if (!GetResourceManager()) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Resource manager not registered"));
        }
        return Result<void>::Success();
    }

    Result<void> Services::InitializeSceneSystem() {
        auto sceneSystem = GetSceneSystem();
        if (!sceneSystem) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Scene system not registered"));
        }
        return sceneSystem->Initialize();
    }
}