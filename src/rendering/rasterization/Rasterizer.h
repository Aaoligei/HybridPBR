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
        Rasterizer() {stats = RenderStats();};
        ~Rasterizer();
        
        // IRenderer接口实现
        bool Initialize() override;
        void Shutdown() override;
        void Render(const Scene& scene) override;
        void BeginFrame() override;
        void EndFrame() override;
        void SetViewport(int width, int height) override;
        void SetClearColor(const glm::vec4& color) override;
        
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
        static RenderStats& GetStats() { return stats; }
        // 设置
        void SetSkyboxTexture(std::shared_ptr<Texture> texture);
    private:
        // 渲染状态
        bool wireframe = false;
        bool backfaceCulling = true;
        bool depthTest = true;
        glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        
        // 渲染通道
        std::vector<std::shared_ptr<RenderPass>> renderPass;

        // 当前场景
        static const Scene* currentScene;
        static RenderStats stats;

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