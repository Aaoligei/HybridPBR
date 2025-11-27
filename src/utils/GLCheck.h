#pragma once
#include <iostream>
#include <string>

// ==========================================================================
// 注意：在这里包含你的 OpenGL 加载器头文件 (比如 GLEW, GLAD, GLFW 等)
// 根据你的项目实际情况取消注释或修改
// ==========================================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h> 

// 如果你不想在头文件里包含 gl 头文件，请确保在使用此头文件前先包含 gl 头文件

// ==========================================================================
// 断点宏定义 (用于出错时暂停程序，方便调试器挂载)
// ==========================================================================
#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #define DEBUG_BREAK() __builtin_trap()
#else
    #include <cstdlib>
    #define DEBUG_BREAK() abort()
#endif

// ==========================================================================
// 核心宏：GLCall
// 用法: GLCall(glDrawElements(...));
// ==========================================================================
#ifdef NDEBUG
    // Release 模式下，直接调用函数，不进行检查，保证性能
    #define GLCall(x) x
#else
    // Debug 模式下，先清空错误，执行函数，然后检查错误
    #define GLCall(x) GLClearError();\
        x;\
        if (!GLLogCall(#x, __FILE__, __LINE__)) DEBUG_BREAK();
#endif

// ==========================================================================
// 辅助函数实现
// ==========================================================================

// 将错误枚举转换为可读字符串
inline std::string GLEnumToString(unsigned int error) {
    switch (error) {
        // 这些宏定义在标准的 gl.h 或 加载器中
        case 0x0500: return "GL_INVALID_ENUM";
        case 0x0501: return "GL_INVALID_VALUE";
        case 0x0502: return "GL_INVALID_OPERATION";
        case 0x0503: return "GL_STACK_OVERFLOW";
        case 0x0504: return "GL_STACK_UNDERFLOW";
        case 0x0505: return "GL_OUT_OF_MEMORY";
        case 0x0506: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default:     return "Unknown Error (" + std::to_string(error) + ")";
    }
}

// 清空当前 OpenGL 错误栈，防止捕捉到之前的错误
inline void GLClearError() {
    // 循环调用直到没有错误为止
    // 这里的 glGetError 需要确保你的环境中已经声明 (通常由 glad/glew 提供)
    // 为了通用性，这里假设 glGetError 已可用。
    while (glGetError() != 0); 
}

// 打印错误日志
inline bool GLLogCall(const char* function, const char* file, int line) {
    while (unsigned int error = glGetError()) {
        std::cerr << "[OpenGL Error] (" << GLEnumToString(error) << ")\n"
                  << "   Function: " << function << "\n"
                  << "   File:     " << file << "\n"
                  << "   Line:     " << line << std::endl;
        return false; // 返回 false 表示有错误发生
    }
    return true;
}
