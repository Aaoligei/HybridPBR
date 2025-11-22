#pragma once
#include "../interfaces/IRasterizer.h"
#include "RenderPass.h"
#include "../ShaderManager.h"
#include "../../scene/Scene.h"
#include "../common/UniformBuffer.h" 
#include <memory>
#include <vector>

namespace HybridPBR {

    // 必须遵循 std140 内存对齐规则
    // vec3 实际上占用 vec4 的空间 (16 bytes)
    struct CameraData {
        glm::mat4 view;          // 64 bytes
        glm::mat4 projection;    // 64 bytes
        glm::vec3 viewPos;       // 12 bytes
        float padding;           // 4 bytes (补齐到16字节)
    }; 

    struct GPULight {
        glm::vec3 position;  float padding1; // 16 bytes
        glm::vec3 direction; float padding2; // 16 bytes
        glm::vec3 color;     float intensity; // 16 bytes
        
        // 聚光灯/点光源参数打包
        float range;
        float constant;
        float linear;
        float quadratic;     // 16 bytes
        
        float innerCutoff;
        float outerCutoff;
        int type;            // 0:Directional, 1:Point, 2:Spot
        float padding3;      // 16 bytes
    };

    struct LightData {
        int lightCount;
        int padding[3];       // 补齐 16 bytes
        GPULight lights[16];  // 支持最多16个光源
    };

    class Rasterizer : public IRasterizer {
    public:
        Rasterizer();
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
        void AddRenderPass(std::unique_ptr<RenderPass> pass);
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
        std::vector<std::unique_ptr<RenderPass>> renderPasses;
        
        // 当前场景
        static const Scene* currentScene;
        static RenderStats stats;

        // UBO
        std::unique_ptr<UniformBuffer> cameraUBO;
        std::unique_ptr<UniformBuffer> lightUBO;
        
        void UpdateGlobalUniforms(const Scene& scene);
        
        // 初始化默认渲染通道
        void SetupDefaultRenderPasses();
    };

} // namespace HybridPBR