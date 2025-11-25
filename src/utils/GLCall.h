#pragma once
#include <glad/glad.h>
#include <sstream>
#include <iostream>

//  Release 下关闭检查
#ifdef NDEBUG
#  define GL_CALL(x) x
#else
#  define GL_CALL(x) ::GL::CheckError(x, #x, __FILE__, __LINE__)
#endif

namespace GL {

    inline const char* ErrorString(GLenum err)
    {
        switch (err)
        {
        case GL_NO_ERROR:                      return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
    #ifdef GL_INVALID_FRAMEBUFFER_OPERATION
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    #endif
        default:                               return "UNKNOWN_ERROR";
        }
    }

    inline void CheckError([[maybe_unused]] GLenum errPlaceholder,
                            const char* call,
                            const char* file,
                            int         line)
    {
        GLenum err = glGetError();
        if (err == GL_NO_ERROR) return;

        std::ostringstream ss;
        ss << "[OpenGL Error] 0x" << std::hex << err << std::dec
        << "  \"" << ErrorString(err) << "\"\n"
        << "    in " << file << ":" << line
        << "  |  " << call;
        std::cerr << ss.str() << std::endl;

        // 触发断点，方便调试
        #if defined(_MSC_VER)
            __debugbreak();
        #elif defined(__GNUC__) || defined(__clang__)
            __builtin_trap();
        #else
            *(volatile int*)0 = 0;   // 强行崩溃
        #endif
        }

} // namespace GL