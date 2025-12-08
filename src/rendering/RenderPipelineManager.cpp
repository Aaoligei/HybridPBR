#include "RenderPipelineManager.h"
#include "utils/Logger.h"
#include <algorithm>
#include <stack>

namespace HybridPBR {

    RenderPipelineManager::RenderPipelineManager() : executionOrderDirty_(true), parallelExecution_(false) {
    }

    RenderPipelineManager::~RenderPipelineManager() {
        // 清理所有管线
        for (auto& [name, node] : pipelines_) {
            if (node.pipeline) {
                node.pipeline->Shutdown();
            }
        }
        pipelines_.clear();
    }

    Result<void> RenderPipelineManager::RegisterPipeline(std::shared_ptr<IRenderPipeline> pipeline) {
        if (!pipeline) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Invalid pipeline"));
        }

        const std::string& name = pipeline->GetDescription().name;
        
        if (pipelines_.find(name) != pipelines_.end()) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Pipeline already registered: " + name));
        }

        PipelineNode node;
        node.pipeline = pipeline;
        node.enabled = true;
        
        pipelines_[name] = node;
        executionOrderDirty_ = true;
        
        LOG_INFO("PipelineManager", "Registered pipeline: " + name);
        
        return Result<void>::Success();
    }

    void RenderPipelineManager::UnregisterPipeline(const std::string& name) {
        auto it = pipelines_.find(name);
        if (it != pipelines_.end()) {
            // 关闭管线
            if (it->second.pipeline) {
                it->second.pipeline->Shutdown();
            }
            
            // 移除所有相关的依赖关系
            for (auto& [otherName, otherNode] : pipelines_) {
                auto& deps = otherNode.dependencies;
                deps.erase(std::remove(deps.begin(), deps.end(), name), deps.end());
                
                auto& dependents = otherNode.dependents;
                dependents.erase(std::remove(dependents.begin(), dependents.end(), name), dependents.end());
            }
            
            pipelines_.erase(it);
            executionOrderDirty_ = true;
            
            LOG_INFO("PipelineManager", "Unregistered pipeline: " + name);
        }
    }

    std::shared_ptr<IRenderPipeline> RenderPipelineManager::GetPipeline(const std::string& name) {
        auto it = pipelines_.find(name);
        return (it != pipelines_.end()) ? it->second.pipeline : nullptr;
    }

    std::vector<std::shared_ptr<IRenderPipeline>> RenderPipelineManager::GetAllPipelines() const {
        std::vector<std::shared_ptr<IRenderPipeline>> result;
        result.reserve(pipelines_.size());
        
        for (const auto& [name, node] : pipelines_) {
            if (node.pipeline) {
                result.push_back(node.pipeline);
            }
        }
        
        return result;
    }

    Result<void> RenderPipelineManager::ExecutePipelines(const Scene& scene) {
        // 更新执行顺序
        if (executionOrderDirty_) {
            RETURN_IF_ERROR(UpdateExecutionOrder());
        }

        // 验证依赖关系
        RETURN_IF_ERROR(ValidateDependencies());

        // 执行管线
        if (parallelExecution_) {
            return ExecutePipelineParallel(executionOrder_, scene);
        } else {
            return ExecutePipelineSequentially(executionOrder_, scene);
        }
    }

    void RenderPipelineManager::SetPipelineExecutionOrder(const std::vector<std::string>& order) {
        executionOrder_ = order;
        executionOrderDirty_ = false;
    }

    void RenderPipelineManager::EnablePipeline(const std::string& name, bool enabled) {
        auto it = pipelines_.find(name);
        if (it != pipelines_.end()) {
            it->second.enabled = enabled;
            LOG_INFO("PipelineManager", "Pipeline " + name + " " + (enabled ? "enabled" : "disabled"));
        }
    }

    Result<void> RenderPipelineManager::AddPipelineDependency(const std::string& pipeline, const std::string& dependency) {
        RETURN_IF_ERROR(ValidateDependency(pipeline, dependency));
        
        auto& pipelineNode = pipelines_[pipeline];
        auto& dependencyNode = pipelines_[dependency];
        
        // 添加依赖关系
        if (std::find(pipelineNode.dependencies.begin(), pipelineNode.dependencies.end(), dependency) == pipelineNode.dependencies.end()) {
            pipelineNode.dependencies.push_back(dependency);
            dependencyNode.dependents.push_back(pipeline);
            executionOrderDirty_ = true;
            
            LOG_INFO("PipelineManager", "Added dependency: " + pipeline + " -> " + dependency);
        }
        
        return Result<void>::Success();
    }

    void RenderPipelineManager::RemovePipelineDependency(const std::string& pipeline, const std::string& dependency) {
        auto pipelineIt = pipelines_.find(pipeline);
        auto dependencyIt = pipelines_.find(dependency);
        
        if (pipelineIt != pipelines_.end() && dependencyIt != pipelines_.end()) {
            auto& deps = pipelineIt->second.dependencies;
            deps.erase(std::remove(deps.begin(), deps.end(), dependency), deps.end());
            
            auto& dependents = dependencyIt->second.dependents;
            dependents.erase(std::remove(dependents.begin(), dependents.end(), pipeline), dependents.end());
            
            executionOrderDirty_ = true;
            
            LOG_INFO("PipelineManager", "Removed dependency: " + pipeline + " -> " + dependency);
        }
    }

    std::vector<std::string> RenderPipelineManager::GetPipelineDependencies(const std::string& pipeline) const {
        auto it = pipelines_.find(pipeline);
        return (it != pipelines_.end()) ? it->second.dependencies : std::vector<std::string>();
    }

    Result<void> RenderPipelineManager::ValidateDependencies() {
        // 检查循环依赖
        for (const auto& [name, node] : pipelines_) {
            std::vector<std::string> cycle;
            DetectCircularDependencies(name, cycle);
            
            if (!cycle.empty()) {
                std::string cycleStr;
                for (size_t i = 0; i < cycle.size(); ++i) {
                    if (i > 0) cycleStr += " -> ";
                    cycleStr += cycle[i];
                }
                return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Circular dependency detected: " + cycleStr));
            }
        }
        
        // 检查依赖的管线是否存在
        for (const auto& [name, node] : pipelines_) {
            for (const auto& dep : node.dependencies) {
                if (!HasPipeline(dep)) {
                    return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Dependency not found: " + name + " -> " + dep));
                }
            }
        }
        
        return Result<void>::Success();
    }

    std::vector<std::string> RenderPipelineManager::GetExecutionOrder() const {
        return executionOrder_;
    }

    // 私有方法实现
    Result<void> RenderPipelineManager::UpdateExecutionOrder() {
        executionOrder_.clear();
        
        // 重置访问标记
        for (auto& [name, node] : pipelines_) {
            node.visited = false;
            node.visiting = false;
        }
        
        // 对每个节点进行拓扑排序
        for (const auto& [name, node] : pipelines_) {
            if (!node.visited) {
                if (!TopologicalSort(name, executionOrder_)) {
                    return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Failed to compute execution order"));
                }
            }
        }
        
        std::reverse(executionOrder_.begin(), executionOrder_.end());
        executionOrderDirty_ = false;
        
        LOG_DEBUG("PipelineManager", "Updated execution order: " + std::to_string(executionOrder_.size()) + " pipelines");
        
        return Result<void>::Success();
    }

    bool RenderPipelineManager::TopologicalSort(const std::string& name, std::vector<std::string>& order) {
        auto& node = pipelines_[name];
        
        if (node.visiting) {
            // 检测到循环依赖
            return false;
        }
        
        if (node.visited) {
            return true;
        }
        
        node.visiting = true;
        
        // 先访问所有依赖
        for (const auto& dep : node.dependencies) {
            if (!TopologicalSort(dep, order)) {
                return false;
            }
        }
        
        node.visiting = false;
        node.visited = true;
        order.push_back(name);
        
        return true;
    }

    void RenderPipelineManager::DetectCircularDependencies(const std::string& name, std::vector<std::string>& cycle) {
        auto& node = pipelines_[name];
        
        if (node.visiting) {
            // 找到循环
            cycle.push_back(name);
            return;
        }
        
        if (node.visited) {
            return;
        }
        
        node.visiting = true;
        cycle.push_back(name);
        
        for (const auto& dep : node.dependencies) {
            DetectCircularDependencies(dep, cycle);
            if (!cycle.empty() && cycle[0] == name) {
                // 找到完整循环
                return;
            }
        }
        
        node.visiting = false;
        node.visited = true;
        cycle.pop_back();
    }

    bool RenderPipelineManager::HasPipeline(const std::string& name) const {
        return pipelines_.find(name) != pipelines_.end();
    }

    Result<void> RenderPipelineManager::ValidateDependency(const std::string& pipeline, const std::string& dependency) {
        if (!HasPipeline(pipeline)) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Pipeline not found: " + pipeline));
        }
        
        if (!HasPipeline(dependency)) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Dependency not found: " + dependency));
        }
        
        if (pipeline == dependency) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Pipeline cannot depend on itself: " + pipeline));
        }
        
        return Result<void>::Success();
    }

    Result<void> RenderPipelineManager::ExecutePipelineSequentially(const std::vector<std::string>& order, const Scene& scene) {
        for (const auto& name : order) {
            auto it = pipelines_.find(name);
            if (it != pipelines_.end() && it->second.enabled) {
                auto& pipeline = it->second.pipeline;
                if (pipeline) {
                    LOG_DEBUG("PipelineManager", "Executing pipeline: " + name);
                    
                    RETURN_IF_ERROR(pipeline->BeginFrame());
                    RETURN_IF_ERROR(pipeline->Render(scene));
                    RETURN_IF_ERROR(pipeline->EndFrame());
                }
            }
        }
        
        return Result<void>::Success();
    }

    Result<void> RenderPipelineManager::ExecutePipelineParallel(const std::vector<std::string>& order, const Scene& scene) {
        // 简化的并行执行实现
        // 实际实现需要考虑依赖关系和线程安全
        
        LOG_WARNING("PipelineManager", "Parallel execution not yet implemented, falling back to sequential");
        
        return ExecutePipelineSequentially(order, scene);
    }

} // namespace HybridPBR