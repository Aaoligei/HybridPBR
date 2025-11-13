#pragma once
#include "../interfaces/IRasterizer.h"
#include "../Shader.h"
#include "Camera.h"
#include "../../resources/ResourceManager.h"

namespace HybridPBR {

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
        
        // 渲染设置
        void SetSkybox(std::shared_ptr<Texture> skybox);
        void SetAmbientLight(const glm::vec3& color, float intensity = 0.1f);

    private:
        // 渲染状态
        bool wireframe = false;
        bool backfaceCulling = true;
        bool depthTest = true;
        glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        
        // 着色器
        std::shared_ptr<Shader> defaultShader;
        std::shared_ptr<Shader> skyboxShader;
        
        // 环境设置
        glm::vec3 ambientLight = glm::vec3(0.1f);
        std::shared_ptr<Texture> skyboxTexture;
        
        // 渲染方法
        void RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform);
        void RenderMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);
        void RenderSkybox();
        void SetupLighting(std::shared_ptr<Shader> shader, const Scene& scene);
        
        // 工具方法
        void ApplyRenderState();
        bool SetupDefaultShaders();
    };

} // namespace HybridPBR