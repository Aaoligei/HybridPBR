#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Shader.h"
#include "utils/Logger.h"

namespace HybridPBR {

    /**
     * @brief 着色器类型枚举
     * 定义了系统中支持的所有着色器类型
     */
    enum class ShaderType {
        DEFAULT,      // 默认着色器
        PBR,          // PBR着色器
        SKYBOX,       // 天空盒着色器
        UNLIT,        // 无光照着色器
        WIREFRAME,    // 线框着色器
        DEPTH,        // 深度着色器
        POST_PROCESS, // 后处理着色器
        CUSTOM        // 自定义着色器
    };

    /**
     * @brief 着色器管理器类
     * 负责管理系统中所有着色器的加载、存储和状态管理
     */
    class ShaderManager {
    public:
        /**
         * @brief 获取单例实例
         * @return 返回ShaderManager的单例引用
         */
        static ShaderManager& GetInstance();
        
        // 着色器管理
        bool LoadShader(ShaderType type, const std::string& name, 
                       const std::string& vertexPath, const std::string& fragmentPath);
        bool LoadShader(const std::string& name, 
                       const std::string& vertexPath, const std::string& fragmentPath);
        
        std::shared_ptr<Shader> GetShader(ShaderType type);
        std::shared_ptr<Shader> GetShader(const std::string& name);
        std::shared_ptr<Shader> GetDefaultShader();
        
        // 着色器状态管理
        void SetCurrentShader(std::shared_ptr<Shader> shader);
        std::shared_ptr<Shader> GetCurrentShader() const { return currentShader; }
        
        // 预定义着色器设置
        void SetupPredefinedShaders();
        
        // 清理
        void ClearShaders();

        /**
         * @brief 根据着色器程序ID获取对应的着色器名称
         * @param programID OpenGL着色器程序ID
         * @return 如果找到返回着色器名称，否则返回空字符串
         */
        std::string GetShaderNameByID(unsigned int programID) const;

    private:
        ShaderManager() = default;
        
        std::unordered_map<ShaderType, std::shared_ptr<Shader>> predefinedShaders;
        std::unordered_map<std::string, std::shared_ptr<Shader>> customShaders;
        std::unordered_map<unsigned int, std::string> shaderIDToNameMap; // 新增：程序ID到名称的映射
        std::shared_ptr<Shader> currentShader = nullptr;
        
        // 默认着色器后备
        std::shared_ptr<Shader> CreateFallbackShader();
    };

} // namespace HybridPBR