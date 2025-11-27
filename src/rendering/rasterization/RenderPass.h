#pragma once
#include "../ShaderManager.h"
#include "../../scene/Scene.h"
#include "rendering/deferred/GBuffer.h"
#include "pbr/IBL.h"

namespace HybridPBR {
    
    struct RenderContext{
        const Scene* scene = nullptr;
        GBuffer* gBuffer = nullptr;       // 延迟渲染核心资源
        uint32_t outputFBO = 0;           // 当前应该画到哪里
        int width = 0;
        int height = 0;
    };
    // 渲染通道基类
    class RenderPass {
    public:
        virtual ~RenderPass() = default;
        
        virtual void Initialize() = 0;
        virtual void Execute(RenderContext& context) = 0;
        virtual void Cleanup() = 0;
        
        virtual std::string GetName() const = 0;
    };

    // 几何通道 - 渲染所有不透明物体
    class GeometryPass : public RenderPass {
    public:
        GeometryPass() = default;
        
        void Initialize() override;
        void Execute(RenderContext& context) override;
        void Cleanup() override;
        
        std::string GetName() const override { return "GeometryPass"; }
        
        // 设置
        void SetWireframe(bool enabled) { wireframe = enabled; }
        void SetBackfaceCulling(bool enabled) { backfaceCulling = enabled; }

    private:
        bool wireframe = false;
        bool backfaceCulling = true;
        
        void ApplyRenderState();
        void RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform, const Scene& scene);
        void RenderMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);
    };

    // 光照通道 - 使用G-Buffer计算光照
    class LightingPass : public RenderPass {
    public:
        void Initialize() override;
        void Execute(RenderContext& context) override;
        void Cleanup() override;
        std::string GetName() const override { return "LightingPass"; }

    private:
        std::shared_ptr<Shader> lightingShader;
        uint32_t quadVAO = 0, quadVBO = 0;
        void RenderQuad();
    };
    
    // 天空盒通道
    class SkyboxPass : public RenderPass {
    public:
        SkyboxPass() = default;
        
        void Initialize() override;
        void Execute(RenderContext& context) override;
        void Cleanup() override;
        
        std::string GetName() const override { return "SkyboxPass"; }
        
        void SetSkyboxTexture(std::shared_ptr<Texture> texture) { skyboxTexture = texture; }

    private:
        unsigned cubeVAO=0,cubeVBO=0;
        static inline float skyboxVertices[36*8] = {
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,

            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f
        };
        std::shared_ptr<Texture> skyboxTexture;
    };

    // 后处理通道
    class PostProcessPass : public RenderPass {
    public:
        PostProcessPass() = default;
        
        void Initialize() override;
        void Execute(RenderContext& context) override;
        void Cleanup() override;
        
        std::string GetName() const override { return "PostProcessPass"; }
        
        // 添加设置场景纹理的方法
        void SetSceneTexture(uint32_t textureId) { sceneTexture = textureId; }

    private:
        std::shared_ptr<Shader> postProcessShader;
        uint32_t quadVAO = 0, quadVBO = 0;
        uint32_t sceneTexture = 0; // 添加场景纹理变量
        
        void RenderQuad();
    };

    class GBufferPass : public RenderPass {
    public:
        void Initialize() override;
        void Execute(RenderContext& context) override;
        void Cleanup() override;
        std::string GetName() const override { return "GBufferPass"; }
        
        void SetWireframe(bool enabled) { wireframe = enabled; }
        void SetBackfaceCulling(bool enabled) { backfaceCulling = enabled; }

    private:
        bool wireframe = false;
        bool backfaceCulling = true;
        std::shared_ptr<Shader> gBufferShader;

        void RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform,const Scene& scene);
        void RenderMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);
    };
} // namespace HybridPBR