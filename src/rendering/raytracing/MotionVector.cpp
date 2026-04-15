#include "MotionVector.h"

namespace HybridPBR{
    MotionVectorPass::MotionVectorPass(uint32_t _width,uint32_t _height)
    :width(_width),height(_height)
    {
    }

    MotionVectorPass::~MotionVectorPass()
    {
    }

    void MotionVectorPass::Initialize()
    {
        motion_shader=std::make_shared<ComputeShader>();
        if(!motion_shader->LoadFromFile(FileIO::GetAssetsPath()+"shader/compute/motion.comp")){
            LOG_ERROR("Failed to load motion vector shader");
        }
        motion_tex=std::make_shared<Texture>();
        motion_tex->Create2D(width,height,GL_RG16F, GL_RG, GL_FLOAT);
        motion_tex->SetFilter(TextureFilter::NEAREST, TextureFilter::NEAREST);
        previousMVP=glm::mat4(1.0f);
    }
    void MotionVectorPass::Execute(const Scene& scene)
    {
        auto& cam=scene.GetMainCamera();
        previousMVP=currentMVP;
        currentMVP=cam->GetViewProjectionMatrix();
        
        motion_shader->SetMat4("previousMVP",previousMVP);
        motion_shader->SetMat4("currentMVP",currentMVP);

        pos_tex->Bind(0);
        motion_shader->SetInt("postex",0);
        motion_tex->BindImage(0,0,GL_WRITE_ONLY);

        uint32_t groupsX = (width + 15) / 16;
        uint32_t groupsY = (height + 15) / 16;
        motion_shader->Dispatch(groupsX, groupsY, 1);
        
        ComputeShader::ImageAccessBarrier();
    }
    void MotionVectorPass::CleanUp()
    {
    }
}