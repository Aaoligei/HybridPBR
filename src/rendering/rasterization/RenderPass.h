#pragma once
#include "../ShaderManager.h"
#include "../../scene/Scene.h"
#include "rendering/deferred/GBuffer.h"
#include "pbr/IBL.h"

namespace HybridPBR {
    
    // 渲染通道基类
    class RenderPass {
    public:
        virtual ~RenderPass() = default;
        
        virtual void Initialize() = 0;
        virtual void Execute(const Scene& scene) = 0;
        virtual void Cleanup() = 0;
        
        virtual std::string GetName() const = 0;
    };

    // 几何通道 - 渲染所有不透明物体
    class GeometryPass : public RenderPass {
    public:
        GeometryPass() = default;
        
        void Initialize() override;
        void Execute(const Scene& scene) override;
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
        LightingPass();
        
        void Initialize() override;
        void Execute(const Scene& scene) override;
        void Cleanup() override;
        
        std::string GetName() const override { return "LightingPass"; }
        std::shared_ptr<IBL> GetIBLSystem() const { return iblSystem; }
        
        // 设置G-Buffer输入
        void SetGBuffer(std::shared_ptr<GBuffer> gbuffer) { this->gbuffer = gbuffer; }
        void SetIBLSystem(std::shared_ptr<IBL> ibl) { iblSystem = ibl; }
        void SetHybridMaps(std::shared_ptr<Texture> shadowMap, std::shared_ptr<Texture> reflectionMap) {
        rtShadowMap = shadowMap;
        rtReflectionMap = reflectionMap;
    }
        
        // 光源管理
        void SetMaxPointLights(int count) { maxPointLights = count; }
        void SetMaxSpotLights(int count) { maxSpotLights = count; }



    private:
        std::shared_ptr<GBuffer> gbuffer;
        std::shared_ptr<IBL> iblSystem;
        std::shared_ptr<Shader> lightingShader;

        std::shared_ptr<Texture> rtShadowMap;
        std::shared_ptr<Texture> rtReflectionMap;
        
        int maxPointLights = 32;
        int maxSpotLights = 8;
        
        uint32_t quadVAO = 0, quadVBO = 0;
        
        void RenderFullscreenQuad();
        void SetupLightingUniforms(const Scene& scene);
        void SetupGBufferUniforms();
        void SetupIBLUniforms();
    };
    
    // 天空盒通道
    class SkyboxPass : public RenderPass {
    public:
        SkyboxPass() = default;
        
        void Initialize() override;
        void Execute(const Scene& scene) override;
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
        void Execute(const Scene& scene) override;
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
        GBufferPass() = default;
        void Initialize() override;
        void Execute(const Scene& scene) override;
        void Cleanup() override;
        std::string GetName() const override { return "GBufferPass"; }
        
        std::shared_ptr<GBuffer> GetGBuffer() const { return gBuffer; }
        void Resize(int width, int height);

        void SetWireframe(bool enabled) { wireframe = enabled; }
        void SetBackfaceCulling(bool enabled) { backfaceCulling = enabled; }

    private:
        bool wireframe = false;
        bool backfaceCulling = true;
        std::shared_ptr<GBuffer> gBuffer;
        std::shared_ptr<Shader> gBufferShader;

        void ApplyRenderState();
        void RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform,const Scene& scene);
        void RenderMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);
        //void SetupCameraUniforms(const Scene& scene);
    };
} // namespace HybridPBR