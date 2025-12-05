#include "IBL.h"
#include "utils/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include"rendering/ShaderManager.h"
#include <stb_image.h>
#include <stb_image_write.h>

namespace HybridPBR {

    IBL::IBL() {
        if (!InitializeShaders() || !InitializeCaptureResources()) {
            LOG_ERROR("Failed to initialize IBL system");
        }
    }

    IBL::~IBL() {
        Cleanup();
    }

    bool IBL::SetupFromHDR(const std::string& hdrFilePath, int cubemapSize) {
        // 加载HDR环境贴图
        auto hdrTexture = std::make_shared<Texture>();
        if (!hdrTexture->LoadHDR(hdrFilePath)) {
            lastError = "Failed to load HDR environment map: " + hdrFilePath;
            LOG_ERROR(lastError);
            return false;
        }
        
        // 创建立方体贴图FBO
        glCreateFramebuffers(1, &captureFBO);
        glCreateRenderbuffers(1, &captureRBO);
        
        glNamedRenderbufferStorage(captureFBO, GL_DEPTH_COMPONENT24, cubemapSize, cubemapSize);
        glNamedFramebufferRenderbuffer(captureFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
        
        // 创建立方体贴图
        environmentMap = std::make_shared<Texture>();
        if (!environmentMap->CreateCubemap(cubemapSize, GL_RGB16F)) {
            lastError = "Failed to create environment cubemap";
            LOG_ERROR(lastError);
            return false;
        }
        
        // 将等距柱状投影转换为立方体贴图
        equirectangularToCubemapShader->Use();
        equirectangularToCubemapShader->SetInt("equirectangularMap", 0);
        equirectangularToCubemapShader->SetMat4("projection", captureProjection);
        hdrTexture->Bind(0);
        
        glViewport(0, 0, cubemapSize, cubemapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        
        for (unsigned int i = 0; i < 6; ++i) {
            equirectangularToCubemapShader->SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                  GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environmentMap->GetID(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // 生成mipmaps
        environmentMap->GenerateMipmaps();
        
        LOG_INFO("Successfully created environment map from HDR: " + hdrFilePath);
        return true;
    }

    bool IBL::PrecomputeIrradianceMap(int size) {
        if (!environmentMap) {
            lastError = "No environment map available for irradiance computation";
            return false;
        }
        
        irradianceMap = std::make_shared<Texture>();
        irradianceMap->CreateCubemap(size, GL_RGB16F);
        
        irradianceShader->Use();
        irradianceShader->SetInt("environmentMap", 0);
        irradianceShader->SetMat4("projection", captureProjection);
        environmentMap->Bind(0);
        
        glViewport(0, 0, size, size);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        
        for (unsigned int i = 0; i < 6; ++i) {
            irradianceShader->SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                  GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap->GetID(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        LOG_INFO("Successfully precomputed irradiance map");
        return true;
    }

    bool IBL::PrecomputePrefilterMap(int size, uint32_t maxMipLevels) {
        if (!environmentMap) {
            lastError = "No environment map available for prefilter computation";
            return false;
        }
        
        prefilterMap = std::make_shared<Texture>();
        prefilterMap->CreateCubemap(size, GL_RGB16F);
        prefilterMap->GenerateMipmaps();
        
        prefilterShader->Use();
        prefilterShader->SetInt("environmentMap", 0);
        prefilterShader->SetMat4("projection", captureProjection);
        environmentMap->Bind(0);
        
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
            unsigned int mipWidth = size * std::pow(0.5, mip);
            unsigned int mipHeight = size * std::pow(0.5, mip);
            
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);
            
            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader->SetFloat("roughness", roughness);
            
            for (unsigned int i = 0; i < 6; ++i) {
                prefilterShader->SetMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                      GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap->GetID(), 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                RenderCube();
            }
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        LOG_INFO("Successfully precomputed prefilter map with " + std::to_string(maxMipLevels) + " mip levels");
        return true;
    }

    bool IBL::GenerateBRDFLUT(int size) {
        brdfLUT = std::make_shared<Texture>();
        if (!brdfLUT->Create2D(size, size, GL_RG16F, GL_RG, GL_FLOAT)) {
            lastError = "Failed to create BRDF LUT texture";
            LOG_ERROR(lastError);
            return false;
        }
        
        // 记录生成的纹理ID用于调试
        LOG_INFO("BRDF LUT created with ID: " + std::to_string(brdfLUT->GetID()));
        
        brdfLUT->SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        brdfLUT->SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT->GetID(), 0);
        
        glViewport(0, 0, size, size);
        brdfShader->Use();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderQuad();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        LOG_INFO("Successfully generated BRDF LUT with ID: " + std::to_string(brdfLUT->GetID()));
        return true;
    }

    void IBL::BindIBLTextures(std::shared_ptr<Shader> shader) const {
        if (!shader) return;
        
        shader->Use();
        
        if (irradianceMap) {
            irradianceMap->Bind(10);
            shader->SetInt("irradianceMap", 10);
        }
        
        if (prefilterMap) {
            prefilterMap->Bind(11);
            shader->SetInt("prefilterMap", 11);
        }
        
        if (brdfLUT) {
            brdfLUT->Bind(12);
            shader->SetInt("brdfLUT", 12);
        }
    }
    void IBL::BindIBLTexturesRT(std::shared_ptr<ComputeShader> computeShader) const { 
        if (!computeShader) return;
        
        computeShader->Use();
        
        if(environmentMap) {
            environmentMap->Bind(28);
            computeShader->SetInt("environmentMap", 28);
        }

        if (irradianceMap) {
            irradianceMap->Bind(29);
            computeShader->SetInt("irradianceMap", 29);
        }
        
        if (prefilterMap) {
            prefilterMap->Bind(30);
            computeShader->SetInt("prefilterMap", 30);
        }
        
        if (brdfLUT) {
            brdfLUT->Bind(31);
            computeShader->SetInt("brdfLUT", 31);
        }
    }

    bool IBL::InitializeShaders() {
         auto& shaderManager = ShaderManager::GetInstance();
    
        // 加载等距柱状投影转立方体贴图着色器
        if (!shaderManager.LoadShader("EquirectangularToCubemap", 
            FileIO::GetAssetsPath()+"shaders/equirectangular_to_cubemap.vert", 
            FileIO::GetAssetsPath()+"shaders/equirectangular_to_cubemap.frag")) {
            lastError = "Failed to load equirectangular to cubemap shader";
            LOG_ERROR(lastError);
            return false;
        }
        equirectangularToCubemapShader = shaderManager.GetShader("EquirectangularToCubemap");

        if (!shaderManager.LoadShader("IrradianceConvolution", 
            FileIO::GetAssetsPath()+"shaders/irradiance_convolution.vert", 
            FileIO::GetAssetsPath()+"shaders/irradiance_convolution.frag")) {
            lastError = "Failed to load irradiance_convolution shader";
            LOG_ERROR(lastError);
            return false;
        }
        irradianceShader = shaderManager.GetShader("IrradianceConvolution");
        
        // 加载预滤波着色器
        if (!shaderManager.LoadShader("Prefilter", 
            FileIO::GetAssetsPath()+"shaders/prefilter.vert", 
            FileIO::GetAssetsPath()+"shaders/prefilter.frag")) {
            lastError = "Failed to load prefilter shader";
            LOG_ERROR(lastError);
            return false;
        }
        prefilterShader = shaderManager.GetShader("Prefilter");
        
        // 加载BRDF积分着色器
        if (!shaderManager.LoadShader("BRDFIntegration", 
            FileIO::GetAssetsPath()+"shaders/brdf_integration.vert", 
            FileIO::GetAssetsPath()+"shaders/brdf_integration.frag")) {
            lastError = "Failed to load BRDF integration shader";
            LOG_ERROR(lastError);
            return false;
        }
        brdfShader = shaderManager.GetShader("BRDFIntegration");
        
        if (!equirectangularToCubemapShader || !irradianceShader || !prefilterShader || !brdfShader) {
            lastError = "One or more IBL shaders failed to load";
            LOG_ERROR(lastError);
            return false;
        }
        
        LOG_INFO("All IBL shaders loaded successfully");
        return true;
    }

    bool IBL::InitializeCaptureResources() {
        // 初始化捕获投影矩阵和视图矩阵
        captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        captureViews = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
        
        // 创建捕获用的FBO和RBO
        glCreateFramebuffers(1, &captureFBO);
        glCreateRenderbuffers(1, &captureRBO);
        return true;
    }

    void IBL::RenderCube() {
        // 简化的立方体渲染
        // 实际项目中应该有专门的立方体VAO
        static unsigned int cubeVAO = 0;
        static unsigned int cubeVBO = 0;
        
        if (cubeVAO == 0)
        {
            float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
            };
           // 1. 创建 VAO 和 VBO (DSA: glCreate*)
            glCreateVertexArrays(1, &cubeVAO);
            glCreateBuffers(1, &cubeVBO);

            // 2. 分配并上传数据 (DSA: glNamedBufferStorage)
            // 使用 Storage 代表数据是不可变的(Immutable)，显卡驱动可进行优化，且比 BufferData 更快
            // 最后一个参数 flags 设为 0，表示我们以后不会去 map 读取或修改它
            glNamedBufferStorage(cubeVBO, sizeof(vertices), vertices, 0);

            // 3. 将 VBO 关联到 VAO 的 "绑定点(Binding Point) 0"
            // 参数: VAO, BindingIndex, VBO, Offset, Stride
            // 我们的数据是交错的 (Interleaved)，所以所有属性都来自同一个 Binding Point，Stride 都是 8*float
            glVertexArrayVertexBuffer(cubeVAO, 0, cubeVBO, 0, 8 * sizeof(float));

            // 4. 配置属性 (Format + Binding)
            
            // --- Attribute 0: Position ---
            glEnableVertexArrayAttrib(cubeVAO, 0); 
            // 设置格式: 3个float, 归一化false, 相对偏移量0
            glVertexArrayAttribFormat(cubeVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
            // 将属性0 关联到 绑定点0
            glVertexArrayAttribBinding(cubeVAO, 0, 0);

            // --- Attribute 1: Normal ---
            glEnableVertexArrayAttrib(cubeVAO, 1);
            // 相对偏移量: 3 * sizeof(float)
            glVertexArrayAttribFormat(cubeVAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
            glVertexArrayAttribBinding(cubeVAO, 1, 0);

            // --- Attribute 2: TexCoords ---
            glEnableVertexArrayAttrib(cubeVAO, 2);
            // 相对偏移量: 6 * sizeof(float)
            glVertexArrayAttribFormat(cubeVAO, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
            glVertexArrayAttribBinding(cubeVAO, 2, 0);
        }
        // render Cube
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    void IBL::RenderQuad() {
        // 简化的四边形渲染
        static unsigned int quadVAO = 0;
        static unsigned int quadVBO = 0;
        
        if (quadVAO == 0) {
            float quadVertices[] = {
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            };
            
            glCreateVertexArrays(1, &quadVAO);
            glCreateBuffers(1, &quadVBO);
            glNamedBufferStorage(quadVBO, sizeof(quadVertices), &quadVertices, 0);

            glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 5 * sizeof(float));

            glEnableVertexArrayAttrib(quadVAO, 0);
            glVertexArrayAttribFormat(quadVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(quadVAO, 0, 0);

            glEnableVertexArrayAttrib(quadVAO, 1);
            glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
            glVertexArrayAttribBinding(quadVAO, 1, 0);

        }
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void IBL::Cleanup() {
        if (captureFBO) {
            glDeleteFramebuffers(1, &captureFBO);
            captureFBO = 0;
        }
        
        if (captureRBO) {
            glDeleteRenderbuffers(1, &captureRBO);
            captureRBO = 0;
        }
    }

} // namespace HybridPBR