#pragma once

#include "RenderPass.h"
#include "rendering/Shader.h"
#include "scene/Scene.h" // 添加 Scene 头文件以支持构造函数参数

namespace HybridPBR
{
    class ShadowPass : public RenderPass
    {
    public:
        ShadowPass() = default;
        ShadowPass(Scene* scene, Shader& shader); // 添加新的构造函数
        void Initialize() override;
        void Execute(const Scene& scene) override;
        void Cleanup() override;
        std::string GetName() const override { return "ShadowPass"; }
        
        unsigned int GetShadowMapTexture() const;

    private:
        unsigned int m_FBO;
        unsigned int m_shadowMapTexture;
        std::shared_ptr<Shader> m_shader;

        const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    private:
        void RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform, const std::shared_ptr<Shader>& shader);
    };
}