#pragma once
#include "../rendering/common/Texture.h"
#include "../rendering/Shader.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace HybridPBR {

    class IBL {
    public:
        IBL();
        ~IBL();
        
        // IBL环境设置
        bool SetupFromHDR(const std::string& hdrFilePath, int cubemapSize = 512);
        bool SetupFromCubemap(const std::vector<std::string>& cubemapFaces);
        
        // 预计算
        bool PrecomputeIrradianceMap(int size = 32);
        bool PrecomputePrefilterMap(int size = 128, uint32_t maxMipLevels = 5);
        bool GenerateBRDFLUT(int size = 512);
        
        // 获取IBL纹理
        std::shared_ptr<Texture> GetEnvironmentMap() const { return environmentMap; }
        std::shared_ptr<Texture> GetIrradianceMap() const { return irradianceMap; }
        std::shared_ptr<Texture> GetPrefilterMap() const { return prefilterMap; }
        std::shared_ptr<Texture> GetBRDFLUT() const { return brdfLUT; }
        
        // 应用到着色器
        void BindIBLTextures(std::shared_ptr<Shader> shader) const;
        
        // 状态查询
        bool IsReady() const { return ready; }
        std::string GetLastError() const { return lastError; }

    private:
        std::shared_ptr<Texture> environmentMap;
        std::shared_ptr<Texture> irradianceMap;
        std::shared_ptr<Texture> prefilterMap;
        std::shared_ptr<Texture> brdfLUT;
        
        std::shared_ptr<Shader> equirectangularToCubemapShader;
        std::shared_ptr<Shader> irradianceShader;
        std::shared_ptr<Shader> prefilterShader;
        std::shared_ptr<Shader> brdfShader;

        glm::mat4 captureProjection;
        std::vector<glm::mat4> captureViews;
        
        uint32_t captureFBO = 0;
        uint32_t captureRBO = 0;
        
        bool ready = false;
        std::string lastError;
        
        // 初始化方法
        bool InitializeShaders();
        bool InitializeCaptureResources();
        
        // 工具方法
        void RenderCube();
        void RenderQuad();
        void SetupCaptureProjection();
        
        // 清理资源
        void Cleanup();
    };

} // namespace HybridPBR