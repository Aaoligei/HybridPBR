#pragma once

#include "ServiceLocator.h"
#include "Result.h"
// [修改] 移除 IRenderDevice.h
#include "rendering/interfaces/IRenderer.h"
#include "resources/ResourceManager.h"
#include "scene/Scene.h"
#include <memory>

namespace HybridPBR {

    class Services {
    public:
        static Result<void> RegisterCoreServices();
        static Result<void> RegisterRenderServices();
        
        static Result<void> InitializeServices();
        static Result<void> InitializeRenderer();
        static void ShutdownServices();
        
        // [修改] 移除 GetRenderDevice()
        static std::shared_ptr<IRenderer> GetRenderer();
        static std::shared_ptr<IResourceManager> GetResourceManager();
        static std::shared_ptr<ISceneSystem> GetSceneSystem();

    private:
        static bool servicesRegistered_;
        static bool servicesInitialized_;
        
        // [修改] 移除 InitializeRenderDevice
        static Result<void> InitializeResourceManager();
        static Result<void> InitializeSceneSystem();
    };

}