#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <mutex>
#include "../rendering/common/Texture.h"
#include "../rendering/common/Material.h"
#include "Mesh.h"
#include "../rendering/Shader.h"
#include "../core/Result.h"
#include "../core/ServiceLocator.h"

namespace HybridPBR {

    /**
     * @brief 资源状态枚举
     * 描述资源的加载和使用状态
     */
    enum class ResourceState {
        Unloaded,
        Loading,
        Loaded,
        Failed,
        Unloading
    };

    /**
     * @brief 资源基类
     * 提供统一的资源接口和生命周期管理
     */
    class IResource {
    public:
        virtual ~IResource() = default;
        
        virtual const std::string& GetName() const = 0;
        virtual const std::string& GetFilePath() const = 0;
        virtual ResourceState GetState() const = 0;
        virtual size_t GetMemoryUsage() const = 0;
        virtual uint32_t GetReferenceCount() const = 0;
        
        virtual Result<void> Load() = 0;
        virtual void Unload() = 0;
        virtual void IncrementReference() = 0;
        virtual void DecrementReference() = 0;
        
        virtual bool IsGPUResource() const = 0;
        virtual void UploadToGPU() = 0;
        virtual void ReleaseFromGPU() = 0;
    };

    /**
     * @brief 资源管理器接口
     * 提供统一的资源管理服务
     */
    class IResourceManager {
    public:
        virtual ~IResourceManager() = default;
        
        // 资源加载
        virtual Result<std::shared_ptr<IResource>> Load(const std::string& filepath) = 0;
        virtual std::shared_ptr<IResource> Get(const std::string& filepath) = 0;
        
        // 资源管理
        virtual void Unload(const std::string& filepath) = 0;
        virtual void Reload(const std::string& filepath) = 0;
        virtual void UnloadUnused() = 0;
        virtual void UnloadAll() = 0;
        
        // 内存管理
        virtual void SetMemoryBudget(size_t bytes) = 0;
        virtual size_t GetMemoryUsage() const = 0;
        virtual size_t GetMemoryBudget() const = 0;
        virtual void GarbageCollect() = 0;
        
        // 热重载
        virtual void EnableHotReload(bool enabled) = 0;
        virtual bool IsHotReloadEnabled() const = 0;
        virtual void WatchDirectory(const std::string& directory) = 0;
        
        // 统计信息
        struct ResourceStats {
            size_t totalResources = 0;
            size_t loadedResources = 0;
            size_t memoryUsage = 0;
            size_t memoryBudget = 0;
            std::unordered_map<std::string, size_t> resourcesByType;
        };
        
        virtual ResourceStats GetStats() const = 0;
        
        // 调试
        virtual void SetDebugMode(bool enabled) = 0;
        virtual bool IsDebugMode() const = 0;
        virtual std::vector<std::string> GetLoadedResources() const = 0;
    };

    /**
     * @brief 资源管理器实现
     * 实现完整的资源生命周期管理
     */
    class ResourceManager : public IResourceManager {
    public:
        ResourceManager();
        ~ResourceManager() override;
        
        // IResourceManager接口实现
        Result<std::shared_ptr<IResource>> Load(const std::string& filepath) override;
        std::shared_ptr<IResource> Get(const std::string& filepath) override;
        
        void Unload(const std::string& filepath) override;
        void Reload(const std::string& filepath) override;
        void UnloadUnused() override;
        void UnloadAll() override;
        
        void SetMemoryBudget(size_t bytes) override { memoryBudget_ = bytes; }
        size_t GetMemoryUsage() const override { return currentMemoryUsage_; }
        size_t GetMemoryBudget() const override { return memoryBudget_; }
        void GarbageCollect() override;
        
        void EnableHotReload(bool enabled) override;
        bool IsHotReloadEnabled() const override { return hotReloadEnabled_; }
        void WatchDirectory(const std::string& directory) override;
        
        ResourceStats GetStats() const override;
        
        void SetDebugMode(bool enabled) override { debugMode_ = enabled; }
        bool IsDebugMode() const override { return debugMode_; }
        std::vector<std::string> GetLoadedResources() const override;

    private:
        std::unordered_map<std::string, std::shared_ptr<IResource>> resources_;
        size_t memoryBudget_ = 1024 * 1024 * 1024; // 1GB默认
        size_t currentMemoryUsage_ = 0;
        bool hotReloadEnabled_ = false;
        bool debugMode_ = false;
        mutable std::mutex mutex_;
        
        // 内部方法
        Result<void> UpdateMemoryUsage();
        void CheckMemoryBudget();
        void StartFileWatcher();
        void StopFileWatcher();
        void OnFileChanged(const std::string& filepath);
    };

} // namespace HybridPBR