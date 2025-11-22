#include "Logger.h"
#include <GLFW/glfw3.h>

namespace HybridPBR {
    
    Logger& Logger::GetInstance() {
        static Logger instance;
        return instance;
    }
    
    void Logger::SetLogLevel(LogLevel level) {
        currentLevel = level;
    }
    
    void Logger::Log(LogLevel level, const std::string& message) {
        if (level < currentLevel) return;
        
        std::cout << "[" << LevelToString(level) << "] " << message << std::endl;
    }
    
    void Logger::Debug(const std::string& message) {
        Log(LogLevel::DEBUG, message);
    }
    
    void Logger::Info(const std::string& message) {
        Log(LogLevel::INFO, message);
    }
    
    void Logger::Warning(const std::string& message) {
        Log(LogLevel::WARNING, message);
    }
    
    void Logger::Error(const std::string& message) {
        Log(LogLevel::ERROR, message);
    }
    
    void Logger::Critical(const std::string& message) {
        Log(LogLevel::CRITICAL, message);
    }
    
    const char* Logger::LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }

    inline void Logger::CheckGLError(const char* stmt, const char* file, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "GL error %s at %s:%d -> 0x%x\n", stmt, file, line, err);
        exit(EXIT_FAILURE);
    }
}

} // namespace HybridPBR