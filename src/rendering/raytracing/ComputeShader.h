#pragma once
#include "rendering/Shader.h"
#include <string>

namespace HybridPBR {

    class ComputeShader : public Shader {
    public:
        ComputeShader();
        ~ComputeShader();
        
        bool LoadFromFile(const std::string& filepath);
        bool LoadFromSource(const std::string& source);
        
        void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) const;
        void Dispatch(uint32_t workItemsX, uint32_t workItemsY, uint32_t workItemsZ, 
                     uint32_t localSizeX, uint32_t localSizeY, uint32_t localSizeZ) const;
        
        // 内存屏障
        static void MemoryBarrier();
        static void ShaderStorageBarrier();
        static void ImageAccessBarrier();

    private:
        bool CompileShader(const std::string& source);
    };

} // namespace HybridPBR