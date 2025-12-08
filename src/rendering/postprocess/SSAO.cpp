#include "SSAO.h"
#include "utils/MathUtils.h"
#include "rendering/Shader.h"
#include "utils/GLCheck.h"
#include "utils/FileIO.h"

namespace HybridPBR {

static const float QUAD_VERTICES[24] = {
    -1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f
};

SSAO::SSAO() {
    GenerateSamples();
    GenerateNoiseTexture();
}

SSAO::~SSAO() {
    Shutdown();
}

bool SSAO::Initialize() {
    // 创建帧缓冲
    if (!CreateSSAOFramebuffer(1280, 720) || !CreateBlurFramebuffer(1280, 720)) {
        LOG_ERROR("Failed to create SSAO framebuffers");
        return false;
    }

    // 加载着色器
    ssaoShader = std::make_shared<Shader>();
    blurShader = std::make_shared<Shader>();
    
    if (!ssaoShader->LoadFromFile(FileIO::GetAssetsPath() + "shaders/ssao.vert", FileIO::GetAssetsPath() + "shaders/ssao.frag") || 
        !blurShader->LoadFromFile(FileIO::GetAssetsPath() + "shaders/blur.vert", FileIO::GetAssetsPath() + "shaders/blur.frag")) {
        LOG_ERROR("Failed to compile SSAO shaders");
        return false;
    }

    // 创建全屏四边形
    glCreateVertexArrays(1, &quadVAO);
    glCreateBuffers(1, &quadVBO);
    glNamedBufferStorage(quadVBO, sizeof(QUAD_VERTICES), QUAD_VERTICES, 0);
    
    glEnableVertexArrayAttrib(quadVAO, 0);
    glVertexArrayAttribFormat(quadVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(quadVAO, 0, 0);
    
    glEnableVertexArrayAttrib(quadVAO, 1);
    glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(quadVAO, 1, 0);
    
    glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 4 * sizeof(float));

    return true;
}

void SSAO::Shutdown() {
    Cleanup();
}

void SSAO::Execute(const Scene& scene) {
    if (!gbuffer || !camera) return;

    // 第1步：渲染SSAO
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, ssaoTexture->GetWidth(), ssaoTexture->GetHeight());

    ssaoShader->Use();
    gbuffer->BindForLightingPass();
    
    // 传递SSAO参数
    ssaoShader->SetFloat("radius", radius);
    ssaoShader->SetFloat("bias", bias);
    ssaoShader->SetFloat("power", power);
    ssaoShader->SetVec2("screenSize", glm::vec2(ssaoTexture->GetWidth(), ssaoTexture->GetHeight()));

    // 传递采样核
    for (int i = 0; i < kernelSize; ++i) {
        ssaoShader->SetVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    }

    // 绑定噪声纹理
    noiseTexture->Bind(0);
    ssaoShader->SetInt("noiseTex", 0);

    RenderFullscreenQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 第2步：模糊SSAO纹理
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    blurShader->Use();
    ssaoTexture->Bind(0);
    blurShader->SetInt("ssaoInput", 0);
    blurShader->SetVec2("blurDirection", glm::vec2(1.0f, 0.0f));
    RenderFullscreenQuad();

    // 第3步：垂直模糊
    std::swap(ssaoFBO, blurFBO);
    std::swap(ssaoTexture, blurTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    blurShader->SetVec2("blurDirection", glm::vec2(0.0f, 1.0f));
    RenderFullscreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::Resize(int width, int height) {
    Cleanup();
    CreateSSAOFramebuffer(width, height);
    CreateBlurFramebuffer(width, height);
}

void SSAO::Cleanup() {
    if (ssaoFBO) glDeleteFramebuffers(1, &ssaoFBO);
    if (blurFBO) glDeleteFramebuffers(1, &blurFBO);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    ssaoFBO = blurFBO = quadVAO = quadVBO = 0;
}

bool SSAO::CreateSSAOFramebuffer(int width, int height) {
    // 创建SSAO纹理
    ssaoTexture = std::make_shared<Texture>();
    if (!ssaoTexture->Create2D(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE)) return false;

    // 创建FBO
    glCreateFramebuffers(1, &ssaoFBO);
    glNamedFramebufferTexture(ssaoFBO, GL_COLOR_ATTACHMENT0, ssaoTexture->GetID(), 0);

    if (glCheckNamedFramebufferStatus(ssaoFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("SSAO Framebuffer is not complete");
        return false;
    }

    return true;
}

bool SSAO::CreateBlurFramebuffer(int width, int height) {
    // 创建模糊纹理
    blurTexture = std::make_shared<Texture>();
    if (!blurTexture->Create2D(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE)) return false;

    // 创建FBO
    glCreateFramebuffers(1, &blurFBO);
    glNamedFramebufferTexture(blurFBO, GL_COLOR_ATTACHMENT0, blurTexture->GetID(), 0);

    if (glCheckNamedFramebufferStatus(blurFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Blur Framebuffer is not complete");
        return false;
    }

    return true;
}

void SSAO::GenerateSamples() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    ssaoKernel.resize(kernelSize);
    for (int i = 0; i < kernelSize; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // 使用MathUtils::PI确保一致性
        float scale = (float)i / (float)kernelSize;
        scale = scale * scale;
        scale = 0.1f + scale * 0.9f; // 等效于Lerp(0.1f, 1.0f, scale*scale)
        ssaoKernel[i] = sample * scale;
    }
}

void SSAO::GenerateNoiseTexture() {
    std::vector<glm::vec3> noiseData;
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    for (int i = 0; i < noiseSize * noiseSize; ++i) {
        glm::vec3 noiseVec(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );
        noiseData.push_back(noiseVec);
    }

    // 创建噪声纹理对象
    noiseTexture = std::make_shared<Texture>();
    if (!noiseTexture->Create2D(noiseSize, noiseSize, GL_RGB16F, GL_RGB, GL_FLOAT, noiseData.data())) {
        LOG_ERROR("Failed to create noise texture for SSAO");
        return;
    }
    
    noiseTexture->SetWrapMode(TextureWrap::REPEAT, TextureWrap::REPEAT);
}

void SSAO::RenderFullscreenQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace HybridPBR