#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "utils/Logger.h"
#include "utils/FileIO.h"

namespace HybridPBR {
    
    class Shader {
    public:
        Shader();
        ~Shader();
        
        bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
        bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
        void Use() const;
        void Destroy();
        
        // Uniform设置方法
        void SetBool(const std::string& name, bool value) const;
        void SetInt(const std::string& name, int value) const;
        void SetFloat(const std::string& name, float value) const;
        void SetVec2(const std::string& name, const glm::vec2& value) const;
        void SetVec3(const std::string& name, const glm::vec3& value) const;
        void SetVec4(const std::string& name, const glm::vec4& value) const;
        void SetMat3(const std::string& name, const glm::mat3& value)const;
        void SetMat4(const std::string& name, const glm::mat4& value) const;
        
        unsigned int GetID() const { return programID; }
        std::string GetName() const { return shaderName; }

    private:
        unsigned int programID = 0;
        std::string shaderName; // 用于调试
        mutable std::unordered_map<std::string, int> uniformLocationCache;
        
        bool CompileShader(unsigned int& shaderID, const std::string& source, unsigned int type);
        bool LinkProgram(unsigned int vertexID, unsigned int fragmentID);
        int GetUniformLocation(const std::string& name) const;
    };

} // namespace HybridPBR