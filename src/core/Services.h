#pragma once

#include "ServiceLocator.h"
#include "Result.h"
#include "rendering/interfaces/IRenderDevice.h"
#include "rendering/interfaces/IRenderer.h"
#include "resources/ResourceManager.h"
#include "scene/Scene.h"
#include <memory>

namespace HybridPBR {

    /**
     * @brief 服务注册器
     * 统一管理所有核心服务的注册和初始化
     * 
     * 修改理由：
     * 1. 提供统一的服务注册入口
     * 2. 确保服务初始化的正确顺序
     * 3. 支持服务的依赖关系管理
     * 4. 便于测试和模拟
     */
    class Services {
    public:
        // 服务注册
        static Result<void> RegisterCoreServices();
        static Result<void> RegisterRenderServices();
        static Result<void> RegisterResourceServices();
        static Result<void> RegisterSceneServices();
        
        // 服务初始化
        static Result<void> InitializeServices();
        static Result<void> InitializeRenderDeviceAfterContext();
        static Result<void> InitializeRenderer();
        static void ShutdownServices();
        
        // 便利访问方法
        static std::shared_ptr<IRenderDevice> GetRenderDevice();
        static std::shared_ptr<IRenderer> GetRenderer();
        static std::shared_ptr<IResourceManager> GetResourceManager();
        static std::shared_ptr<ISceneSystem> GetSceneSystem();

    private:
        static bool servicesRegistered_;
        static bool servicesInitialized_;
        
        // 初始化顺序控制
        static Result<void> InitializeRenderDevice();
        static Result<void> InitializeResourceManager();
        static Result<void> InitializeSceneSystem();
    };

} // namespace HybridPBR