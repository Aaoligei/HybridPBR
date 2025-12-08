#include "HybridRenderer.h"
#include "OpenGLRenderDevice.h"
#include "deferred/DeferredRenderer.h"
#include "rasterization/Rasterizer.h"
#include "raytracing/RayTracer.h"
#include "utils/Logger.h"
#include <algorithm>

namespace HybridPBR {

    HybridRenderer::HybridRenderer() : initialized_(false), debugMode_(false) {
        pipelineManager_ = std::make_unique<RenderPipelineManager>();
    }

    HybridRenderer::~HybridRenderer() {
        Shutdown();
    }

    Result<void> HybridRenderer::Initialize(std::shared_ptr<IRenderDevice> device) {
        if (initialized_) {
            return Result<void>::Success();
        }

        if (!device) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Invalid render device"));
        }

        device_ = device;
        
        LOG_INFO("Renderer", "Initializing Hybrid Renderer");

        // 初始化默认管线
        RETURN_IF_ERROR(InitializeDefaultPipelines());
        
        // 设置管线依赖关系
        RETURN_IF_ERROR(SetupPipelineDependencies());

        initialized_ = true;
        LOG_INFO("Renderer", "Hybrid Renderer initialized successfully");
        
        return Result<void>::Success();
    }

    void HybridRenderer::Shutdown() {
        if (!initialized_) {
            return;
        }

        LOG_INFO("Renderer", "Shutting down Hybrid Renderer");
        
        // 清理所有管线
        if (pipelineManager_) {
            auto pipelines = pipelineManager_->GetAllPipelines();
            for (auto& pipeline : pipelines) {
                pipelineManager_->UnregisterPipeline(pipeline->GetDescription().name);
            }
        }

        device_.reset();
        initialized_ = false;
    }

    Result<void> HybridRenderer::BeginFrame() {
        if (!initialized_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Renderer not initialized"));
        }

        // 重置统计信息
        stats_.Reset();

        return Result<void>::Success();
    }

    Result<void> HybridRenderer::Render(const Scene& scene) {
        if (!initialized_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Renderer not initialized"));
        }

        // 自动选择最优策略
        if (autoPipelineSelection_) {
            RETURN_IF_ERROR(SelectOptimalStrategy(scene));
        }

        // 执行渲染策略
        switch (strategy_) {
            case RenderStrategy::RASTERIZATION_ONLY:
                return ExecuteRasterizationStrategy(scene);
                
            case RenderStrategy::DEFERRED_ONLY:
                return ExecuteDeferredStrategy(scene);
                
            case RenderStrategy::RAYTRACING_ONLY:
                return ExecuteRayTracingStrategy(scene);
                
            case RenderStrategy::HYBRID_RASTER_RT:
                return ExecuteHybridRasterRTStrategy(scene);
                
            case RenderStrategy::HYBRID_DEFERRED_RT:
                return ExecuteHybridDeferredRTStrategy(scene);
                
            default:
                return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Unknown render strategy"));
        }
    }

    Result<void> HybridRenderer::EndFrame() {
        if (!initialized_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Renderer not initialized"));
        }

        // 更新渲染统计
        UpdateRenderStats();

        return Result<void>::Success();
    }

    Result<void> HybridRenderer::SetViewport(int width, int height) {
        if (!device_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "No render device"));
        }

        device_->SetViewport(0, 0, width, height);
        
        // 调整所有管线的大小
        auto pipelines = pipelineManager_->GetAllPipelines();
        for (auto& pipeline : pipelines) {
            pipeline->Resize(width, height);
        }

        return Result<void>::Success();
    }

    Result<void> HybridRenderer::SetClearColor(const glm::vec4& color) {
        if (!device_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "No render device"));
        }

        device_->SetClearColor(color.r, color.g, color.b, color.a);
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::Resize(uint32_t width, uint32_t height) {
        return SetViewport(static_cast<int>(width), static_cast<int>(height));
    }

    std::shared_ptr<IRenderPipelineManager> HybridRenderer::GetPipelineManager() {
        return std::shared_ptr<IRenderPipelineManager>(pipelineManager_.get(), [](IRenderPipelineManager*) {});
    }

    Result<void> HybridRenderer::AddPipeline(std::shared_ptr<IRenderPipeline> pipeline) {
        if (!pipelineManager_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Pipeline manager not initialized"));
        }

        return pipelineManager_->RegisterPipeline(pipeline);
    }

    std::shared_ptr<Texture> HybridRenderer::GetOutputTexture() const {
        // HybridRenderer doesn't directly manage textures
        // The individual renderers should be queried for their output
        return nullptr;
    }

    const RenderStats& HybridRenderer::GetStats() const {
        return stats_;
    }

    void HybridRenderer::ResetStats() {
        stats_.Reset();
        
        // 重置所有管线的统计信息
        if (pipelineManager_) {
            auto pipelines = pipelineManager_->GetAllPipelines();
            for (auto& pipeline : pipelines) {
                pipeline->ResetStats();
            }
        }
    }

    void HybridRenderer::SetRenderStrategy(RenderStrategy strategy) {
        if (strategy_ != strategy) {
            LOG_INFO("Renderer", "Switching render strategy to " + std::to_string(static_cast<int>(strategy)));
            strategy_ = strategy;
        }
    }

    std::shared_ptr<IRenderPipeline> HybridRenderer::GetRasterizationPipeline() const {
        return rasterizationPipelineName_.empty() ? nullptr : 
               pipelineManager_->GetPipeline(rasterizationPipelineName_);
    }

    std::shared_ptr<IRenderPipeline> HybridRenderer::GetDeferredPipeline() const {
        return deferredPipelineName_.empty() ? nullptr : 
               pipelineManager_->GetPipeline(deferredPipelineName_);
    }

    std::shared_ptr<IRenderPipeline> HybridRenderer::GetRayTracingPipeline() const {
        return rayTracingPipelineName_.empty() ? nullptr : 
               pipelineManager_->GetPipeline(rayTracingPipelineName_);
    }

    void HybridRenderer::SetQualityPreset(int level) {
        qualityPreset_ = std::clamp(level, 0, 2);
        
        switch (qualityPreset_) {
            case 0: // 性能优先
                LOG_INFO("Renderer", "Setting quality preset: Performance");
                break;
            case 1: // 平衡
                LOG_INFO("Renderer", "Setting quality preset: Balanced");
                break;
            case 2: // 质量优先
                LOG_INFO("Renderer", "Setting quality preset: Quality");
                break;
        }
    }

    // 私有方法实现
    Result<void> HybridRenderer::InitializeDefaultPipelines() {
        // 这里应该创建实际的管线实现
        // 由于我们还没有完整的管线实现，这里先创建占位符
        
        // 创建光栅化管线
        // auto rasterizer = std::make_shared<Rasterizer>();
        // RETURN_IF_ERROR(rasterizer->Initialize(device_));
        // RETURN_IF_ERROR(AddPipeline(rasterizer));
        // rasterizationPipelineName_ = "Rasterizer";

        // 创建延迟渲染管线
        // auto deferred = std::make_shared<DeferredRenderer>();
        // RETURN_IF_ERROR(deferred->Initialize(device_));
        // RETURN_IF_ERROR(AddPipeline(deferred));
        // deferredPipelineName_ = "DeferredRenderer";

        // 创建光线追踪管线
        // auto raytracer = std::make_shared<RayTracer>();
        // RETURN_IF_ERROR(raytracer->Initialize(device_));
        // RETURN_IF_ERROR(AddPipeline(raytracer));
        // rayTracingPipelineName_ = "RayTracer";

        LOG_WARNING("Renderer", "Default pipelines not yet implemented - using placeholder");
        
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::SetupPipelineDependencies() {
        // 设置管线间的依赖关系
        // 例如：光线追踪管线可能需要光栅化管线的G-Buffer
        
        if (!rasterizationPipelineName_.empty() && !rayTracingPipelineName_.empty()) {
            // RETURN_IF_ERROR(pipelineManager_->AddPipelineDependency(rayTracingPipelineName_, rasterizationPipelineName_));
        }

        return Result<void>::Success();
    }

    Result<void> HybridRenderer::SelectOptimalStrategy(const Scene& scene) {
        // 根据场景复杂度和质量预设选择最优策略
        
        // 简单的策略选择逻辑
        size_t nodeCount = scene.GetNodeCount();
        size_t lightCount = scene.GetAllLights().size();
        
        if (qualityPreset_ == 0) { // 性能优先
            if (nodeCount < 100 && lightCount < 4) {
                SetRenderStrategy(RenderStrategy::RASTERIZATION_ONLY);
            } else {
                SetRenderStrategy(RenderStrategy::DEFERRED_ONLY);
            }
        } else if (qualityPreset_ == 1) { // 平衡
            if (nodeCount < 50) {
                SetRenderStrategy(RenderStrategy::RASTERIZATION_ONLY);
            } else if (nodeCount < 200) {
                SetRenderStrategy(RenderStrategy::DEFERRED_ONLY);
            } else {
                SetRenderStrategy(RenderStrategy::HYBRID_DEFERRED_RT);
            }
        } else { // 质量优先
            if (nodeCount < 100) {
                SetRenderStrategy(RenderStrategy::HYBRID_RASTER_RT);
            } else {
                SetRenderStrategy(RenderStrategy::HYBRID_DEFERRED_RT);
            }
        }

        return Result<void>::Success();
    }

    Result<void> HybridRenderer::ValidatePipelineConfiguration() {
        // 验证当前策略所需的管线是否可用
        
        switch (strategy_) {
            case RenderStrategy::RASTERIZATION_ONLY:
                if (!GetRasterizationPipeline()) {
                    return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Rasterization pipeline not available"));
                }
                break;
                
            case RenderStrategy::DEFERRED_ONLY:
                if (!GetDeferredPipeline()) {
                    return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Deferred pipeline not available"));
                }
                break;
                
            case RenderStrategy::RAYTRACING_ONLY:
                if (!GetRayTracingPipeline()) {
                    return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Ray tracing pipeline not available"));
                }
                break;
                
            case RenderStrategy::HYBRID_RASTER_RT:
                if (!GetRasterizationPipeline() || !GetRayTracingPipeline()) {
                    return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Hybrid raster+RT pipelines not available"));
                }
                break;
                
            case RenderStrategy::HYBRID_DEFERRED_RT:
                if (!GetDeferredPipeline() || !GetRayTracingPipeline()) {
                    return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Hybrid deferred+RT pipelines not available"));
                }
                break;
        }

        return Result<void>::Success();
    }

    void HybridRenderer::UpdateRenderStats() {
        // 聚合所有管线的统计信息
        if (pipelineManager_) {
            auto pipelines = pipelineManager_->GetAllPipelines();
            for (auto& pipeline : pipelines) {
                const auto& pipelineStats = pipeline->GetStats();
                stats_.drawCalls += pipelineStats.drawCalls;
                stats_.triangleCount += pipelineStats.triangleCount;
                stats_.vertexCount += pipelineStats.vertexCount;
                stats_.computeDispatches += pipelineStats.computeDispatches;
                stats_.frameTime = std::max(stats_.frameTime, pipelineStats.frameTime);
                stats_.gpuTime = std::max(stats_.gpuTime, pipelineStats.gpuTime);
                stats_.cpuTime = std::max(stats_.cpuTime, pipelineStats.cpuTime);
                stats_.memoryUsed += pipelineStats.memoryUsed;
                stats_.memoryAllocated += pipelineStats.memoryAllocated;
            }
        }
    }

    // 策略执行方法
    Result<void> HybridRenderer::ExecuteRasterizationStrategy(const Scene& scene) {
        LOG_DEBUG("Renderer", "Executing rasterization strategy");
        
        auto pipeline = GetRasterizationPipeline();
        if (!pipeline) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Rasterization pipeline not available"));
        }

        RETURN_IF_ERROR(pipeline->BeginFrame());
        RETURN_IF_ERROR(pipeline->Render(scene));
        RETURN_IF_ERROR(pipeline->EndFrame());
        
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::ExecuteDeferredStrategy(const Scene& scene) {
        LOG_DEBUG("Renderer", "Executing deferred strategy");
        
        auto pipeline = GetDeferredPipeline();
        if (!pipeline) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Deferred pipeline not available"));
        }

        RETURN_IF_ERROR(pipeline->BeginFrame());
        RETURN_IF_ERROR(pipeline->Render(scene));
        RETURN_IF_ERROR(pipeline->EndFrame());
        
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::ExecuteRayTracingStrategy(const Scene& scene) {
        LOG_DEBUG("Renderer", "Executing ray tracing strategy");
        
        auto pipeline = GetRayTracingPipeline();
        if (!pipeline) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Ray tracing pipeline not available"));
        }

        RETURN_IF_ERROR(pipeline->BeginFrame());
        RETURN_IF_ERROR(pipeline->Render(scene));
        RETURN_IF_ERROR(pipeline->EndFrame());
        
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::ExecuteHybridRasterRTStrategy(const Scene& scene) {
        LOG_DEBUG("Renderer", "Executing hybrid raster+RT strategy");
        
        auto rasterPipeline = GetRasterizationPipeline();
        auto rtPipeline = GetRayTracingPipeline();
        
        if (!rasterPipeline || !rtPipeline) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Hybrid pipelines not available"));
        }

        // 首先执行光栅化
        RETURN_IF_ERROR(rasterPipeline->BeginFrame());
        RETURN_IF_ERROR(rasterPipeline->Render(scene));
        RETURN_IF_ERROR(rasterPipeline->EndFrame());
        
        // 然后执行光线追踪增强
        RETURN_IF_ERROR(rtPipeline->BeginFrame());
        RETURN_IF_ERROR(rtPipeline->Render(scene));
        RETURN_IF_ERROR(rtPipeline->EndFrame());
        
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::ExecuteHybridDeferredRTStrategy(const Scene& scene) {
        LOG_DEBUG("Renderer", "Executing hybrid deferred+RT strategy");
        
        auto deferredPipeline = GetDeferredPipeline();
        auto rtPipeline = GetRayTracingPipeline();
        
        if (!deferredPipeline || !rtPipeline) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Hybrid pipelines not available"));
        }

        // 首先执行延迟渲染
        RETURN_IF_ERROR(deferredPipeline->BeginFrame());
        RETURN_IF_ERROR(deferredPipeline->Render(scene));
        RETURN_IF_ERROR(deferredPipeline->EndFrame());
        
        // 然后执行光线追踪增强
        RETURN_IF_ERROR(rtPipeline->BeginFrame());
        RETURN_IF_ERROR(rtPipeline->Render(scene));
        RETURN_IF_ERROR(rtPipeline->EndFrame());
        
        return Result<void>::Success();
    }

} // namespace HybridPBR