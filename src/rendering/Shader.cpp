#include "Shader.h"
#include <glad/glad.h>
#include<GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace HybridPBR {
    
    Shader::Shader() {
        programID = 0;
    }
    
    Shader::~Shader() {
        Destroy();
    }
    
    bool Shader::LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexSource = FileIO::ReadTextFile(vertexPath);
        std::string fragmentSource = FileIO::ReadTextFile(fragmentPath);
        
        if (vertexSource.empty() || fragmentSource.empty()) {
            LOG_ERROR("Failed to read shader files");
            return false;
        }
        /* --------- 取文件名 --------- */
        std::string fileName = std::filesystem::path(vertexPath).filename().string();
        shaderName = fileName; // 设置着色器名称
        
        return LoadFromSource(vertexSource, fragmentSource);
    }
    
    bool Shader::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
        unsigned int vertexID, fragmentID;
        
        // 编译顶点着色器
        if (!CompileShader(vertexID, vertexSource, GL_VERTEX_SHADER)) {
            return false;
        }
        
        // 编译片段着色器
        if (!CompileShader(fragmentID, fragmentSource, GL_FRAGMENT_SHADER)) {
            glDeleteShader(vertexID);
            return false;
        }
        
        // 链接着色器程序
        if (!LinkProgram(vertexID, fragmentID)) {
            glDeleteShader(vertexID);
            glDeleteShader(fragmentID);
            return false;
        }
        
        // 清理着色器对象
        glDeleteShader(vertexID);
        glDeleteShader(fragmentID);
        
        LOG_INFO("Shader program created successfully");
        return true;
    }
    
    void Shader::Use() const {
        if (programID != 0) {
            glUseProgram(programID);
        }
    }
    
    void Shader::Destroy() {
        if (programID != 0) {
            glDeleteProgram(programID);
            programID = 0;
            uniformLocationCache.clear();
        }
    }
    
    bool Shader::CompileShader(unsigned int& shaderID, const std::string& source, unsigned int type) {
        shaderID = glCreateShader(type);
        const char* sourceCStr = source.c_str();
        glShaderSource(shaderID, 1, &sourceCStr, nullptr);
        glCompileShader(shaderID);
        
        // 检查编译错误
        int success;
        char infoLog[512];
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shaderID, 512, nullptr, infoLog);
            LOG_ERROR("Shader compilation failed: " + std::string(infoLog));
            return false;
        }
        
        return true;
    }
    
    bool Shader::LinkProgram(unsigned int vertexID, unsigned int fragmentID) {
        programID = glCreateProgram();
        glAttachShader(programID, vertexID);
        glAttachShader(programID, fragmentID);
        glLinkProgram(programID);
        
        // 检查链接错误
        int success;
        char infoLog[512];
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, nullptr, infoLog);
            LOG_ERROR("Shader program linking failed: " + std::string(infoLog));
            return false;
        }
        
        return true;
    }
    
    int Shader::GetUniformLocation(const std::string& name) const {
        auto it = uniformLocationCache.find(name);
        if (it != uniformLocationCache.end()) {
            return it->second;
        }
        
        int location = glGetUniformLocation(programID, name.c_str());
        if (location == -1) {
            LOG_WARNING("Uniform '" + name + "' not found in shader:"+ shaderName);
        }
        
        uniformLocationCache[name] = location;
        return location;
    }
    
    // Uniform设置方法
    void Shader::SetBool(const std::string& name, bool value) const {
        glUniform1i(GetUniformLocation(name), (int)value);
    }
    
    void Shader::SetInt(const std::string& name, int value) const {
        glUniform1i(GetUniformLocation(name), value);
    }
    void Shader::SetUint(const std::string& name, unsigned int value) const{
        glUniform1ui(GetUniformLocation(name), value);
    }
    
    void Shader::SetFloat(const std::string& name, float value) const {
        glUniform1f(GetUniformLocation(name), value);
    }
    
    void Shader::SetVec2(const std::string& name, const glm::vec2& value) const {
        glUniform2f(GetUniformLocation(name), value.x, value.y);
    }
    
    void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
    }
    
    void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
    }
    
    void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }
    void Shader::SetMat3(const std::string& name, const glm::mat3& value) const {
        glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

} // namespace HybridPBR