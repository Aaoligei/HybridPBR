#pragma once
#include "../interfaces/IRasterizer.h"
#include "RenderPass.h"
#include "../ShaderManager.h"
#include "../../scene/Scene.h"
#include "../common/UniformBuffer.h" 
#include <memory>
#include <vector>

namespace HybridPBR {


    class Rasterizer : public IRasterizer {
    public:
        Rasterizer() = default;
        ~Rasterizer();
        
        // IRenderer接口实现
        Result<void> Initialize(std::shared_ptr<IRenderDevice> device) override;
        void Shutdown() override;
        bool IsInitialized() const override { return initialized_; }
        Result<void> Render(const Scene& scene) override;
        Result<void> BeginFrame() override;
        Result<void> EndFrame() override;
        Result<void> SetViewport(int width, int height) override;
        Result<void> SetClearColor(const glm::vec4& color) override;
        Result<void> Resize(uint32_t width, uint32_t height) override;
        std::shared_ptr<IRenderPipelineManager> GetPipelineManager() override;
        Result<void> AddPipeline(std::shared_ptr<IRenderPipeline> pipeline) override;
        std::shared_ptr<Texture> GetOutputTexture() const override;
        std::shared_ptr<IRenderDevice> GetDevice() const override;
        void SetDebugMode(bool enabled) override;
        bool IsDebugMode() const override;
        
        // IRasterizer接口实现
        void SetWireframe(bool enabled) override;
        void SetBackfaceCulling(bool enabled) override;
        void SetDepthTest(bool enabled) override;
        bool IsWireframe() const override { return wireframe; }
        bool IsBackfaceCulling() const override { return backfaceCulling; }
        bool IsDepthTest() const override { return depthTest; }
        
        // 渲染通道管理
        void AddRenderPass(std::shared_ptr<RenderPass> pass);
        void RemoveRenderPass(const std::string& passName);
        std::vector<std::string> GetRenderPassNames();
        void ClearRenderPasses();

        
        // 场景管理
        void SetCurrentScene(const Scene& scene) { currentScene = &scene; }
        static const Scene* GetCurrentScene() { return currentScene; }
        static Rasterizer* GetCurrentInstance() { return currentInstance; }
        static void SetCurrentInstance(Rasterizer* instance) { currentInstance = instance; }
        const RenderStats& GetStats() const override { return stats; }
        void ResetStats() override { stats.Reset(); }
        // 设置
        void SetSkyboxTexture(std::shared_ptr<Texture> texture);
    private:
        // 渲染状态
        bool initialized_ = false;
        bool wireframe = false;
        bool backfaceCulling = true;
        bool depthTest = true;
        bool debugMode_ = false;
        glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        
        // 渲染通道
        std::vector<std::shared_ptr<RenderPass>> renderPass;

        // 当前场景
        static const Scene* currentScene;
        static Rasterizer* currentInstance;
        RenderStats stats;

        // UBO
        std::unique_ptr<UniformBuffer> cameraUBO;
        std::unique_ptr<UniformBuffer> lightUBO;
        
        void UpdateGlobalUniforms(const Scene& scene);
        
        // 初始化默认渲染通道
        void SetupDefaultRenderPasses();
        // 初始化默认 Pass (修改名字以体现意图)
        void SetupDeferredPipeline();
    };

} // namespace HybridPBR