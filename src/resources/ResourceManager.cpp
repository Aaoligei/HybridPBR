#include "ResourceManager.h"
#include "utils/Logger.h"
#include "core/Result.h"
#include <algorithm>
#include <fstream>

using namespace HybridPBR;

namespace HybridPBR {

    // 简化的Resource实现
    class SimpleResource : public IResource {
    public:
        explicit SimpleResource(const std::string& filepath) 
            : filepath_(filepath), state_(ResourceState::Unloaded), referenceCount_(0) {
        }

        const std::string& GetName() const override {
            return name_;
        }

        const std::string& GetFilePath() const override {
            return filepath_;
        }

        ResourceState GetState() const override {
            return state_;
        }

        size_t GetMemoryUsage() const override {
            return memoryUsage_;
        }

        uint32_t GetReferenceCount() const override {
            return referenceCount_;
        }

        Result<void> Load() override {
            if (state_ == ResourceState::Loading) {
                return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Resource already loading"));
            }

            state_ = ResourceState::Loading;

            // 简化的加载实现
            std::ifstream file(filepath_, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                state_ = ResourceState::Failed;
                return Result<void>::Failure(Error(ErrorType::ResourceNotFound, "File not found: " + filepath_));
            }

            size_t fileSize = file.tellg();
            memoryUsage_ = fileSize;
            
            state_ = ResourceState::Loaded;
            return Result<void>::Success();
        }

        void Unload() override {
            if (state_ == ResourceState::Loaded) {
                state_ = ResourceState::Unloaded;
                memoryUsage_ = 0;
            }
        }

        void IncrementReference() override {
            referenceCount_++;
        }

        void DecrementReference() override {
            if (referenceCount_ > 0) {
                referenceCount_--;
            }
        }

        bool IsGPUResource() const override {
            return false; // 简化实现
        }

        void UploadToGPU() override {
            // GPU上传实现
        }

        void ReleaseFromGPU() override {
            // GPU释放实现
        }

    private:
        std::string filepath_;
        std::string name_;
        ResourceState state_;
        size_t memoryUsage_ = 0;
        uint32_t referenceCount_ = 0;
    };

    ResourceManager::ResourceManager() : currentMemoryUsage_(0) {
    }

    ResourceManager::~ResourceManager() {
        UnloadAll();
    }

    Result<std::shared_ptr<IResource>> ResourceManager::Load(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = resources_.find(filepath);
        if (it != resources_.end()) {
            it->second->IncrementReference();
            return Result<std::shared_ptr<IResource>>::Success(it->second);
        }

        // 检查内存预算
        if (currentMemoryUsage_ >= memoryBudget_) {
            // 尝试清理未使用的资源
            GarbageCollect();
            
            if (currentMemoryUsage_ >= memoryBudget_) {
                return Result<std::shared_ptr<IResource>>::Failure(
                    Error(ErrorType::OutOfMemory, "Memory budget exceeded"));
            }
        }

        // 创建新资源
        auto resource = std::make_shared<SimpleResource>(filepath);
        resources_[filepath] = resource;
        
        // 加载资源
        auto loadResult = resource->Load();
        if (loadResult.IsFailure()) {
            resources_.erase(filepath);
            return Result<std::shared_ptr<IResource>>::Failure(loadResult.GetError());
        }

        UpdateMemoryUsage();
        LOG_INFO("ResourceManager", std::string("Loaded resource: ") + filepath);
        
        return Result<std::shared_ptr<IResource>>::Success(resource);
    }

    std::shared_ptr<IResource> ResourceManager::Get(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = resources_.find(filepath);
        if (it != resources_.end()) {
            it->second->IncrementReference();
            return it->second;
        }
        
        return nullptr;
    }

    void ResourceManager::Unload(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = resources_.find(filepath);
        if (it != resources_.end()) {
            it->second->DecrementReference();
            
            if (it->second->GetReferenceCount() == 0) {
                currentMemoryUsage_ -= it->second->GetMemoryUsage();
                resources_.erase(it);
                LOG_INFO("ResourceManager", std::string("Unloaded resource: ") + filepath);
            }
        }
    }

    void ResourceManager::Reload(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = resources_.find(filepath);
        if (it != resources_.end()) {
            auto& resource = it->second;
            currentMemoryUsage_ -= resource->GetMemoryUsage();
            
            auto reloadResult = resource->Load();
            if (reloadResult.IsSuccess()) {
                LOG_INFO("ResourceManager", std::string("Reloaded resource: ") + filepath);
            } else {
                LOG_ERROR("ResourceManager", std::string("Failed to reload resource: ") + filepath);
            }
            
            UpdateMemoryUsage();
        }
    }

    void ResourceManager::UnloadUnused() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = resources_.begin();
        while (it != resources_.end()) {
            if (it->second->GetReferenceCount() == 0) {
                currentMemoryUsage_ -= it->second->GetMemoryUsage();
                LOG_INFO("ResourceManager", std::string("Unloaded unused resource: ") + it->first);
                it = resources_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ResourceManager::UnloadAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        resources_.clear();
        currentMemoryUsage_ = 0;
        LOG_INFO("ResourceManager", "All resources unloaded");
    }

    void ResourceManager::GarbageCollect() {
        UnloadUnused();
    }

    void ResourceManager::EnableHotReload(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (enabled != hotReloadEnabled_) {
            hotReloadEnabled_ = enabled;
            
            if (enabled) {
                StartFileWatcher();
            } else {
                StopFileWatcher();
            }
        }
    }

    void ResourceManager::WatchDirectory(const std::string& directory) {
        // 实现目录监控
        LOG_INFO("ResourceManager", std::string("Watching directory: ") + directory);
    }

    std::vector<std::string> ResourceManager::GetLoadedResources() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> resources;
        for (const auto& [filepath, resource] : resources_) {
            resources.push_back(filepath);
        }
        
        return resources;
    }

    ResourceManager::ResourceStats ResourceManager::GetStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        ResourceStats stats;
        stats.totalResources = resources_.size();
        stats.memoryUsage = currentMemoryUsage_;
        stats.memoryBudget = memoryBudget_;
        
        for (const auto& [filepath, resource] : resources_) {
            if (resource->GetState() == ResourceState::Loaded) {
                stats.loadedResources++;
            }
            
            // 按类型统计（简化实现）
            std::string extension = filepath.substr(filepath.find_last_of('.') + 1);
            stats.resourcesByType[extension]++;
        }
        
        return stats;
    }

    // 私有方法实现
    Result<void> ResourceManager::UpdateMemoryUsage() {
        currentMemoryUsage_ = 0;
        
        for (const auto& [filepath, resource] : resources_) {
            currentMemoryUsage_ += resource->GetMemoryUsage();
        }
        
        CheckMemoryBudget();
        
        return Result<void>::Success();
    }

    void ResourceManager::CheckMemoryBudget() {
        if (currentMemoryUsage_ > memoryBudget_) {
            LOG_WARNING("ResourceManager", std::string("Memory usage exceeds budget"));
        }
    }

    void ResourceManager::StartFileWatcher() {
        // 实现文件监控启动
        LOG_INFO("ResourceManager", "File watcher started");
    }

    void ResourceManager::StopFileWatcher() {
        // 实现文件监控停止
        LOG_INFO("ResourceManager", "File watcher stopped");
    }

    void ResourceManager::OnFileChanged(const std::string& filepath) {
        if (hotReloadEnabled_) {
            Reload(filepath);
        }
    }

} // namespace HybridPBR