#include "ComputeShader.h"
#include "utils/Logger.h"
#include "utils/FileIO.h"
#include "utils/GLCheck.h"
#include <glad/glad.h>

namespace HybridPBR {

    ComputeShader::ComputeShader() {
        programID = 0;
    }

    ComputeShader::~ComputeShader() {
        Destroy();
    }

    bool ComputeShader::LoadFromFile(const std::string& filepath) {
        std::string source = FileIO::ReadTextFile(filepath);
        if (source.empty()) {
            LOG_ERROR("Failed to read compute shader file: " + filepath);
            return false;
        }
        
        return LoadFromSource(source);
    }

    bool ComputeShader::LoadFromSource(const std::string& source) {
        if (programID != 0) {
            glDeleteProgram(programID);
            programID = 0;
        }
        
        return CompileShader(source);
    }

    void ComputeShader::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) const {
        if (programID != 0) {
            Use();
            glDispatchCompute(groupsX, groupsY, groupsZ);
        }
    }

    void ComputeShader::Dispatch(uint32_t workItemsX, uint32_t workItemsY, uint32_t workItemsZ,
                                uint32_t localSizeX, uint32_t localSizeY, uint32_t localSizeZ) const {
        uint32_t groupsX = (workItemsX + localSizeX - 1) / localSizeX;
        uint32_t groupsY = (workItemsY + localSizeY - 1) / localSizeY;
        uint32_t groupsZ = (workItemsZ + localSizeZ - 1) / localSizeZ;
        
        Dispatch(groupsX, groupsY, groupsZ);
    }

    void ComputeShader::MemoryBarrier() {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    void ComputeShader::ShaderStorageBarrier() {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void ComputeShader::ImageAccessBarrier() {
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    bool ComputeShader::CompileShader(const std::string& source) {
        // 创建着色器对象
        GLuint shaderID = glCreateShader(GL_COMPUTE_SHADER);
        
        // 编译着色器
        const char* sourceCStr = source.c_str();
        glShaderSource(shaderID, 1, &sourceCStr, nullptr);
        glCompileShader(shaderID);
        
        // 检查编译错误
        int success;
        char infoLog[512];
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shaderID, 512, nullptr, infoLog);
            LOG_ERROR("Compute shader compilation failed: " + std::string(infoLog));
            glDeleteShader(shaderID);
            return false;
        }
        
        // 创建程序对象
        programID = glCreateProgram();
        glAttachShader(programID, shaderID);
        glLinkProgram(programID);
        
        // 检查链接错误
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, nullptr, infoLog);
            LOG_ERROR("Compute shader program linking failed: " + std::string(infoLog));
            glDeleteProgram(programID);
            programID = 0;
            glDeleteShader(shaderID);
            return false;
        }
        
        // 清理着色器对象
        glDeleteShader(shaderID);
        
        LOG_INFO("Compute shader compiled and linked successfully");
        return true;
    }

} // namespace HybridPBR