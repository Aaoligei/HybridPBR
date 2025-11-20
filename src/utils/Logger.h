#pragma once
#include <string>
#include <iostream>
#include <sstream>

namespace HybridPBR {
    
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    class Logger {
    public:
        static Logger& GetInstance();
        
        void SetLogLevel(LogLevel level);
        void Log(LogLevel level, const std::string& message);
        
        // 便捷方法
        void Debug(const std::string& message);
        void Info(const std::string& message);
        void Warning(const std::string& message);
        void Error(const std::string& message);
        void Critical(const std::string& message);

    private:
        Logger() = default;
        LogLevel currentLevel = LogLevel::INFO; // 默认级别
        
        const char* LevelToString(LogLevel level);
    };

    // 宏定义便于使用
    #define LOG_DEBUG(message) Logger::GetInstance().Debug(message)
    #define LOG_INFO(message) Logger::GetInstance().Info(message)
    #define LOG_WARNING(message) Logger::GetInstance().Warning(message)
    #define LOG_ERROR(message) Logger::GetInstance().Error(message)
    #define LOG_CRITICAL(message) Logger::GetInstance().Critical(message)

} // namespace HybridPBR