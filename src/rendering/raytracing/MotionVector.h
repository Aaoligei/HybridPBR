#pragma once
#include<glm/glm.hpp>
#include"rendering/common/Texture.h"
#include"rendering/raytracing/ComputeShader.h"
#include"rendering/rasterization/RenderPass.h"

namespace HybridPBR{
    class MotionVectorPass:public RenderPass{
    public:
        MotionVectorPass(uint32_t _width,uint32_t _height);
        ~MotionVectorPass();

        void Initialize() override;
        void Execute(const Scene& scene) override;
        void Cleanup() override;
        std::string GetName() const override { return "MotionVectorPass"; }

    private:
        glm::mat4 currentMVP;
        glm::mat4 previousMVP;
        uint32_t width;
        uint32_t height;
        std::shared_ptr<Texture> pos_tex;
        std::shared_ptr<ComputeShader> motion_shader;
        std::shared_ptr<Texture> motion_tex;
    };
}