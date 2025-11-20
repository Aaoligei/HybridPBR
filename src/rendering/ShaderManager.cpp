#include "ShaderManager.h"
#include "utils/FileIO.h"

namespace HybridPBR {

    // 获取ShaderManager单例实例
    ShaderManager& ShaderManager::GetInstance() {
        static ShaderManager instance;  // 静态局部变量，确保单例模式
        return instance;
    }

    // 加载预定义类型的着色器
    bool ShaderManager::LoadShader(ShaderType type, const std::string& name, 
                                  const std::string& vertexPath, const std::string& fragmentPath) {
        auto shader = std::make_shared<Shader>();  // 创建共享指针管理的着色器对象
        if (shader->LoadFromFile(vertexPath, fragmentPath)) {
            predefinedShaders[type] = shader;    // 存储到预定义着色器映射中
            customShaders[name] = shader;        // 同时存储到自定义着色器映射中
            LOG_INFO("Loaded predefined shader: " + name + " (Type: " + std::to_string(static_cast<int>(type)) + ")");
            return true;
        }
        
        LOG_ERROR("Failed to load predefined shader: " + name + " (Type: " + std::to_string(static_cast<int>(type)) + ")");
        LOG_ERROR("Vertex shader path: " + vertexPath);
        LOG_ERROR("Fragment shader path: " + fragmentPath);
        return false;
    }

    // 加载自定义着色器
    bool ShaderManager::LoadShader(const std::string& name, 
                                  const std::string& vertexPath, const std::string& fragmentPath) {
        auto shader = std::make_shared<Shader>();
        if (shader->LoadFromFile(vertexPath, fragmentPath)) {
            customShaders[name] = shader;  // 只存储到自定义着色器映射中
            LOG_INFO("Loaded custom shader: " + name);
            return true;
        }
        
        LOG_ERROR("Failed to load custom shader: " + name);
        LOG_ERROR("Vertex shader path: " + vertexPath);
        LOG_ERROR("Fragment shader path: " + fragmentPath);
        return false;
    }

    // 获取预定义类型的着色器
    std::shared_ptr<Shader> ShaderManager::GetShader(ShaderType type) {
        auto it = predefinedShaders.find(type);
        if (it != predefinedShaders.end()) {
            return it->second;  // 如果找到，返回对应的着色器
        }
        
        // 如果预定义着色器不存在，返回默认着色器
        LOG_WARNING("Shader type not found: " + std::to_string(static_cast<int>(type)) + ", using default");
        return GetDefaultShader();
    }

    // 获取自定义着色器
    std::shared_ptr<Shader> ShaderManager::GetShader(const std::string& name) {
        auto it = customShaders.find(name);
        if (it != customShaders.end()) {
            return it->second;  // 如果找到，返回对应的着色器
        }
        
        // 如果自定义着色器不存在，返回默认着色器
        LOG_WARNING("Shader not found: " + name + ", using default");
        return GetDefaultShader();
    }

    // 根据着色器程序ID获取着色器名称
    std::string ShaderManager::GetShaderNameByID(unsigned int programID) const {
        // 在预定义着色器中查找
        for (const auto& pair : predefinedShaders) {
            if (pair.second && pair.second->GetID() == programID) {
                // 在自定义着色器中查找对应的名称
                for (const auto& customPair : customShaders) {
                    if (customPair.second == pair.second) {
                        return customPair.first;
                    }
                }
                // 如果找不到自定义名称，返回类型名称
                switch (pair.first) {
                    case ShaderType::DEFAULT: return "Default";
                    case ShaderType::PBR: return "PBR";
                    case ShaderType::SKYBOX: return "Skybox";
                    case ShaderType::UNLIT: return "Unlit";
                    default: return "Unknown";
                }
            }
        }
        
        // 在自定义着色器中查找
        for (const auto& pair : customShaders) {
            if (pair.second && pair.second->GetID() == programID) {
                return pair.first;
            }
        }
        
        return "Not Found";
    }

    // 获取默认着色器
    std::shared_ptr<Shader> ShaderManager::GetDefaultShader() {
        // 确保默认着色器存在
        if (predefinedShaders.find(ShaderType::DEFAULT) == predefinedShaders.end()) {
            SetupPredefinedShaders();  // 如果不存在，则设置预定义着色器
        }
        return predefinedShaders[ShaderType::DEFAULT];
    }

    // 设置当前活动的着色器
    void ShaderManager::SetCurrentShader(std::shared_ptr<Shader> shader) {
        if (shader != currentShader) {
            currentShader = shader;
            if (currentShader) {
                currentShader->Use();  // 使用新的着色器
            }
        }
    }

    // 设置预定义着色器
    void ShaderManager::SetupPredefinedShaders() {
        // 加载默认着色器
        if (!LoadShader(ShaderType::DEFAULT, "Default", 
                       FileIO::GetAssetsPath()+"shaders/default.vert", FileIO::GetAssetsPath()+"shaders/default.frag")) {
            LOG_ERROR("Failed to load default shader, creating fallback");
            predefinedShaders[ShaderType::DEFAULT] = CreateFallbackShader();  // 创建后备着色器
        }
        
        // 加载PBR着色器
        std::string pbrVertexPath = FileIO::GetAssetsPath()+"shaders/pbr.vert";
        std::string pbrFragmentPath = FileIO::GetAssetsPath()+"shaders/pbr.frag";
        
        if (!LoadShader(ShaderType::PBR, "PBR", pbrVertexPath, pbrFragmentPath)) {
            LOG_WARNING("Failed to load PBR shader, will use default");
            LOG_WARNING("PBR vertex path: " + pbrVertexPath);
            LOG_WARNING("PBR fragment path: " + pbrFragmentPath);
        }
        
        // 加载天空盒着色器
        if (!LoadShader(ShaderType::SKYBOX, "Skybox", 
                       FileIO::GetAssetsPath()+"shaders/skybox.vert", FileIO::GetAssetsPath()+"shaders/skybox.frag")) {
            LOG_WARNING("Failed to load skybox shader");
        }
        
        // 加载无光照着色器
        if (!LoadShader(ShaderType::UNLIT, "Unlit", 
                       FileIO::GetAssetsPath()+"shaders/unlit.vert", FileIO::GetAssetsPath()+"shaders/unlit.frag")) {
            LOG_WARNING("Failed to load unlit shader");
        }
    }

    // 创建后备着色器
    std::shared_ptr<Shader> ShaderManager::CreateFallbackShader() {
        auto shader = std::make_shared<Shader>();
        
        // 简单的后备着色器源码
        const char* vertexSource = R"(
            #version 460 core
            layout (location = 0) in vec3 aPos;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
            }
        )";
        
        const char* fragmentSource = R"(
            #version 460 core
            out vec4 FragColor;
            uniform vec3 color;
            void main() {
                FragColor = vec4(color, 1.0);
            }
        )";
        
        if (shader->LoadFromSource(vertexSource, fragmentSource)) {
            LOG_INFO("Created fallback shader");
            return shader;
        }
        
        LOG_ERROR("Failed to create fallback shader");
        return nullptr;
    }

    // 清除所有着色器
    void ShaderManager::ClearShaders() {
        predefinedShaders.clear();  // 清空预定义着色器映射
        customShaders.clear();      // 清空自定义着色器映射
        currentShader = nullptr;     // 重置当前着色器
    }

} // namespace HybridPBR