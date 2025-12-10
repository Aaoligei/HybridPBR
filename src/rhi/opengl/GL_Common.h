#pragma once
#include <glad/glad.h>
#include "utils/Logger.h" // 复用你现有的 Logger

namespace HybridPBR {

    // 简单的 OpenGL 错误检查宏
    // 在 Debug 模式下启用，Release 模式下定义为空
    #ifdef _DEBUG
        #define GL_CHECK(stmt) do { \
            stmt; \
            CheckGLError(#stmt, __FILE__, __LINE__); \
        } while (0)
    #else
        #define GL_CHECK(stmt) stmt
    #endif

    inline void CheckGLError(const char* stmt, const char* file, int line) {
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("OpenGL Error", 
                std::string("Code: ") + std::to_string(err) + 
                " | Stmt: " + stmt + 
                " | File: " + file + 
                " | Line: " + std::to_string(line));
        }
    }
}