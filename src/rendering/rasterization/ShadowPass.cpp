#include "ShadowPass.h"
#include "utils/GLCheck.h"
#include "scene/Scene.h"
#include "rendering/Shader.h"
#include "scene/SceneNode.h"
#include "rendering/rasterization/Mesh.h"

namespace HybridPBR
{
    // 添加新的构造函数实现
    ShadowPass::ShadowPass(Scene* scene, Shader& shader)
    {
        // 这里可以初始化 shadow pass，但目前我们保留 Initialize 方法来完成初始化
        // 因为从错误来看，构造函数可能不需要特殊处理，只是需要这个签名来匹配 make_shared 的调用
        m_shader = std::make_shared<Shader>(shader);
    }

    void ShadowPass::Initialize()
    {
        auto& shaderManager = ShaderManager::GetInstance();
        shaderManager.LoadShader("Shadow", 
                                FileIO::GetAssetsPath()+"shaders/deferred/shadow.vert",
                                FileIO::GetAssetsPath()+"shaders/deferred/shadow.frag");
        m_shader = ShaderManager::GetInstance().GetShader("Shadow"); 

        // Create framebuffer
        glGenFramebuffers(1, &m_FBO);

        // Create depth texture
        glGenTextures(1, &m_shadowMapTexture);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            // Handle error
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void ShadowPass::Cleanup()
    {
        if (m_FBO != 0)
        {
            glDeleteFramebuffers(1, &m_FBO);
        }
        if (m_shadowMapTexture != 0)
        {
            glDeleteTextures(1, &m_shadowMapTexture);
        }
        
    }
    
    void ShadowPass::RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform, const std::shared_ptr<Shader>& shader) {
        auto transform = parentTransform * node.GetTransform().GetLocalMatrix();

        if (auto mesh = node.GetMesh()) {
            shader->SetMat4("model", transform);
            mesh->Render();
        }

        for (auto& child : node.GetChildren()) {
            RenderSceneNode(*child, transform, shader);
        }
    }

    void ShadowPass::Execute(const Scene& scene)
    {
        // 1. first render to depth map
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        m_shader->Use();
        
        RenderSceneNode(*scene.GetRoot(), glm::mat4(1.0f), m_shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    unsigned int ShadowPass::GetShadowMapTexture() const
    {
        return m_shadowMapTexture;
    }
}