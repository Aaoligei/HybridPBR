#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <functional>
#include <vector>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace HybridPBR {

    /**
     * @brief 日志级别枚举
     * 提供详细的日志分类
     */
    enum class LogLevel {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Critical = 5,
        Off = 6
    };

    /**
     * @brief 日志条目结构
     * 包含完整的日志信息
     */
    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        LogLevel level;
        std::string category;
        std::string message;
        std::string file;
        int line;
        std::string function;
        
        LogEntry(LogLevel lvl, const std::string& cat, const std::string& msg,
                const std::string& f = "", int l = 0, const std::string& func = "")
            : timestamp(std::chrono::system_clock::now()), level(lvl), category(cat), 
              message(msg), file(f), line(l), function(func) {}
    };

    /**
     * @brief 日志输出器接口
     * 定义日志输出的抽象接口
     * 
     * 修改理由：
     * 1. 支持多种输出目标（文件、控制台、网络等）
     * 2. 便于添加新的日志输出方式
     * 3. 支持日志格式化和过滤
     * 4. 实现线程安全的日志输出
     */
    class ILogSink {
    public:
        virtual ~ILogSink() = default;
        virtual void Write(const LogEntry& entry) = 0;
        virtual void Flush() = 0;
        virtual void SetLevel(LogLevel level) = 0;
        virtual LogLevel GetLevel() const = 0;
    };

    /**
     * @brief 日志格式化器接口
     * 定义日志消息的格式化规则
     */
    class ILogFormatter {
    public:
        virtual ~ILogFormatter() = default;
        virtual std::string Format(const LogEntry& entry) = 0;
    };

    /**
     * @brief 日志过滤器接口
     * 定义日志过滤规则
     */
    class ILogFilter {
    public:
        virtual ~ILogFilter() = default;
        virtual bool ShouldLog(const LogEntry& entry) = 0;
    };

    /**
     * @brief 高性能日志系统
     * 提供工业级日志功能
     * 
     * 修改理由：
     * 1. 替代简单的日志实现，提供企业级功能
     * 2. 支持异步日志，提高性能
     * 3. 提供结构化日志和上下文信息
     * 4. 支持日志轮转和压缩
     * 5. 实现可配置的日志策略
     */
    class Logger {
    public:
        static Logger& GetInstance();
        
        // 配置管理
        void SetLevel(LogLevel level);
        LogLevel GetLevel() const { return level_; }
        
        void AddSink(std::shared_ptr<ILogSink> sink);
        void RemoveSink(std::shared_ptr<ILogSink> sink);
        void ClearSinks();
        
        void SetFormatter(std::shared_ptr<ILogFormatter> formatter);
        void AddFilter(std::shared_ptr<ILogFilter> filter);
        void ClearFilters();
        
        // 异步日志控制
        void SetAsyncMode(bool enabled, size_t queueSize = 1024);
        bool IsAsyncMode() const { return asyncMode_; }
        void Flush();
        
        // 日志记录方法
        void Log(LogLevel level, const std::string& category, const std::string& message);
    
    // 便利方法
    void Trace(const std::string& category, const std::string& message);
    void Debug(const std::string& category, const std::string& message);
    void Info(const std::string& category, const std::string& message);
    void Warning(const std::string& category, const std::string& message);
    void Error(const std::string& category, const std::string& message);
    void Critical(const std::string& category, const std::string& message);
    
    // 单参数版本
    void Trace(const std::string& message);
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Critical(const std::string& message);
        
        // 性能分析
        void EnablePerformanceLogging(bool enabled) { perfLoggingEnabled_ = enabled; }
        void BeginPerformanceEvent(const std::string& name);
        void EndPerformanceEvent(const std::string& name);
        
        // 统计信息
        struct LogStats {
            size_t totalLogs = 0;
            size_t logsByLevel[7] = {0}; // 对应LogLevel枚举
            size_t droppedLogs = 0;
            std::chrono::system_clock::time_point lastLogTime;
        };
        
        const LogStats& GetStats() const { return stats_; }
        void ResetStats();

    private:
        Logger();
        ~Logger();
        
        LogLevel level_ = LogLevel::Info;
        std::vector<std::shared_ptr<ILogSink>> sinks_;
        std::shared_ptr<ILogFormatter> formatter_;
        std::vector<std::shared_ptr<ILogFilter>> filters_;
        
        bool asyncMode_ = false;
        bool perfLoggingEnabled_ = false;
        
        // 线程安全
        mutable std::mutex mutex_;
        
        // 异步日志队列
        struct AsyncLogData;
        std::unique_ptr<AsyncLogData> asyncData_;
        
        // 统计信息
        mutable LogStats stats_;
        
        // 内部方法
        void WriteLog(const LogEntry& entry);
        bool ShouldLog(const LogEntry& entry) const;
        std::string FormatMessage(const std::string& format, const std::vector<std::string>& args);
        void ProcessAsyncQueue();
        void UpdateStats(const LogEntry& entry);
    };

    // 便利宏定义 - 支持单参数和双参数形式
    #define LOG_TRACE(...) HybridPBR::Logger::GetInstance().Trace(__VA_ARGS__)
    #define LOG_DEBUG(...) HybridPBR::Logger::GetInstance().Debug(__VA_ARGS__)
    #define LOG_INFO(...) HybridPBR::Logger::GetInstance().Info(__VA_ARGS__)
    #define LOG_WARNING(...) HybridPBR::Logger::GetInstance().Warning(__VA_ARGS__)
    #define LOG_ERROR(...) HybridPBR::Logger::GetInstance().Error(__VA_ARGS__)
    #define LOG_CRITICAL(...) HybridPBR::Logger::GetInstance().Critical(__VA_ARGS__)
    
    // 性能分析宏
    #define PERF_SCOPE(name) HybridPBR::PerfTimer _perfTimer(name)
    #define PERF_BEGIN(name) HybridPBR::Logger::GetInstance().BeginPerformanceEvent(name)
    #define PERF_END(name) HybridPBR::Logger::GetInstance().EndPerformanceEvent(name)

    /**
     * @brief 性能计时器辅助类
     */
    class PerfTimer {
    public:
        explicit PerfTimer(const std::string& name) : name_(name) {
            Logger::GetInstance().BeginPerformanceEvent(name_);
        }
        
        ~PerfTimer() {
            Logger::GetInstance().EndPerformanceEvent(name_);
        }

    private:
        std::string name_;
    };

} // namespace HybridPBR