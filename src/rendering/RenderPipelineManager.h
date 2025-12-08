#pragma once

#include "interfaces/IRenderPipeline.h"
#include "core/Result.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>

namespace HybridPBR {

    /**
     * @brief 渲染管线管理器实现
     * 管理多个渲染管线的执行顺序和依赖关系
     * 
     * 修改理由：
     * 1. 提供统一的管线管理机制
     * 2. 支持复杂的依赖关系管理
     * 3. 实现拓扑排序确保正确的执行顺序
     * 4. 提供错误安全的管线执行
     */
    class RenderPipelineManager : public IRenderPipelineManager {
    public:
        RenderPipelineManager();
        ~RenderPipelineManager() override;

        // IRenderPipelineManager接口实现
        Result<void> RegisterPipeline(std::shared_ptr<IRenderPipeline> pipeline) override;
        void UnregisterPipeline(const std::string& name) override;
        std::shared_ptr<IRenderPipeline> GetPipeline(const std::string& name) override;
        std::vector<std::shared_ptr<IRenderPipeline>> GetAllPipelines() const override;

        Result<void> ExecutePipelines(const Scene& scene) override;
        void SetPipelineExecutionOrder(const std::vector<std::string>& order) override;
        void EnablePipeline(const std::string& name, bool enabled) override;

        Result<void> AddPipelineDependency(const std::string& pipeline, const std::string& dependency) override;
        void RemovePipelineDependency(const std::string& pipeline, const std::string& dependency) override;
        std::vector<std::string> GetPipelineDependencies(const std::string& pipeline) const override;

        // 扩展功能
        Result<void> ValidateDependencies();
        std::vector<std::string> GetExecutionOrder() const;
        void SetParallelExecution(bool enabled) { parallelExecution_ = enabled; }
        bool IsParallelExecution() const { return parallelExecution_; }

    private:
        struct PipelineNode {
            std::shared_ptr<IRenderPipeline> pipeline;
            std::vector<std::string> dependencies;
            std::vector<std::string> dependents;
            bool enabled = true;
            bool visited = false;
            bool visiting = false;
        };

        std::unordered_map<std::string, PipelineNode> pipelines_;
        std::vector<std::string> executionOrder_;
        bool executionOrderDirty_ = true;
        bool parallelExecution_ = false;

        // 拓扑排序
        Result<void> UpdateExecutionOrder();
        bool TopologicalSort(const std::string& name, std::vector<std::string>& order);
        void DetectCircularDependencies(const std::string& name, std::vector<std::string>& cycle);

        // 依赖验证
        bool HasPipeline(const std::string& name) const;
        Result<void> ValidateDependency(const std::string& pipeline, const std::string& dependency);
        
        // 执行辅助
        Result<void> ExecutePipelineSequentially(const std::vector<std::string>& order, const Scene& scene);
        Result<void> ExecutePipelineParallel(const std::vector<std::string>& order, const Scene& scene);
    };

} // namespace HybridPBR