#include "Logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <queue>
#include <atomic>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

namespace HybridPBR {

    // 默认格式化器实现
    class DefaultLogFormatter : public ILogFormatter {
    public:
        std::string Format(const LogEntry& entry) override {
            std::stringstream ss;
            
            // 时间戳
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                entry.timestamp.time_since_epoch()) % 1000;
            
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            
            // 日志级别
            ss << " [" << LevelToString(entry.level) << "]";
            
            // 分类
            if (!entry.category.empty()) {
                ss << " [" << entry.category << "]";
            }
            
            // 位置信息
            if (!entry.file.empty()) {
                ss << " [" << entry.file << ":" << entry.line << "]";
                if (!entry.function.empty()) {
                    ss << " (" << entry.function << ")";
                }
            }
            
            // 消息
            ss << " " << entry.message;
            
            return ss.str();
        }

    private:
        std::string LevelToString(LogLevel level) {
            switch (level) {
                case LogLevel::Trace: return "TRACE";
                case LogLevel::Debug: return "DEBUG";
                case LogLevel::Info: return "INFO";
                case LogLevel::Warning: return "WARN";
                case LogLevel::Error: return "ERROR";
                case LogLevel::Critical: return "CRITICAL";
                default: return "UNKNOWN";
            }
        }
    };

    // 控制台输出器实现
    class ConsoleLogSink : public ILogSink {
    public:
        ConsoleLogSink() : level_(LogLevel::Trace) {}

        void Write(const LogEntry& entry) override {
            if (entry.level < level_) {
                return;
            }

            // 根据级别设置颜色
            SetColor(entry.level);
            
            // 输出格式化后的消息
            DefaultLogFormatter formatter;
            std::cout << formatter.Format(entry) << std::endl;
            
            // 重置颜色
            ResetColor();
        }

        void Flush() override {
            std::cout.flush();
        }

        void SetLevel(LogLevel level) override {
            level_ = level;
        }

        LogLevel GetLevel() const override {
            return level_;
        }

    private:
        LogLevel level_;
        
        void SetColor(LogLevel level) {
            #ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            WORD color = 7; // 白色
            
            switch (level) {
                case LogLevel::Trace: color = 8; break; // 灰色
                case LogLevel::Debug: color = 7; break; // 白色
                case LogLevel::Info: color = 2; break;  // 绿色
                case LogLevel::Warning: color = 14; break; // 黄色
                case LogLevel::Error: color = 12; break; // 红色
                case LogLevel::Critical: color = 12 | 128; break; // 红色 + 闪烁
            }
            
            SetConsoleTextAttribute(hConsole, color);
            #else
            switch (level) {
                case LogLevel::Trace: std::cout << "\033[90m"; break; // 灰色
                case LogLevel::Debug: std::cout << "\033[37m"; break; // 白色
                case LogLevel::Info: std::cout << "\033[32m"; break;  // 绿色
                case LogLevel::Warning: std::cout << "\033[33m"; break; // 黄色
                case LogLevel::Error: std::cout << "\033[31m"; break; // 红色
                case LogLevel::Critical: std::cout << "\033[31;5m"; break; // 红色闪烁
            }
            #endif
        }
        
        void ResetColor() {
            #ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, 7); // 白色
            #else
            std::cout << "\033[0m";
            #endif
        }
    };

    // 异步日志数据结构
    struct Logger::AsyncLogData {
        std::queue<LogEntry> asyncQueue;
        std::mutex queueMutex;
        std::thread workerThread;
        std::atomic<bool> shouldStop{false};
        
        AsyncLogData() {
            workerThread = std::thread([this]() {
                while (!shouldStop) {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    
                    if (asyncQueue.empty()) {
                        lock.unlock();
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    
                    LogEntry entry = asyncQueue.front();
                    asyncQueue.pop();
                    lock.unlock();
                    
                    // 处理日志条目
                    Logger::GetInstance().WriteLog(entry);
                }
            });
        }
        
        ~AsyncLogData() {
            shouldStop = true;
            if (workerThread.joinable()) {
                workerThread.join();
            }
        }
    };

    Logger& Logger::GetInstance() {
        static Logger instance;
        return instance;
    }

    Logger::Logger() : level_(LogLevel::Info), perfLoggingEnabled_(false) {
        // 设置默认格式化器
        formatter_ = std::make_shared<DefaultLogFormatter>();
        
        // 添加默认的控制台输出器
        auto consoleSink = std::make_shared<ConsoleLogSink>();
        AddSink(consoleSink);
    }

    Logger::~Logger() {
        if (asyncData_) {
            // 处理剩余的异步日志
            ProcessAsyncQueue();
        }
        
        Flush();
    }

    void Logger::SetLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    void Logger::AddSink(std::shared_ptr<ILogSink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.push_back(sink);
    }

    void Logger::RemoveSink(std::shared_ptr<ILogSink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.erase(std::remove(sinks_.begin(), sinks_.end(), sink), sinks_.end());
    }

    void Logger::ClearSinks() {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.clear();
    }

    void Logger::SetFormatter(std::shared_ptr<ILogFormatter> formatter) {
        std::lock_guard<std::mutex> lock(mutex_);
        formatter_ = formatter;
    }

    void Logger::AddFilter(std::shared_ptr<ILogFilter> filter) {
        std::lock_guard<std::mutex> lock(mutex_);
        filters_.push_back(filter);
    }

    void Logger::ClearFilters() {
        std::lock_guard<std::mutex> lock(mutex_);
        filters_.clear();
    }

    void Logger::SetAsyncMode(bool enabled, size_t queueSize) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (enabled && !asyncData_) {
            asyncData_ = std::make_unique<AsyncLogData>();
        } else if (!enabled && asyncData_) {
            ProcessAsyncQueue();
            asyncData_.reset();
        }
    }

    void Logger::Flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& sink : sinks_) {
            sink->Flush();
        }
    }

    void Logger::WriteLog(const LogEntry& entry) {
        if (!ShouldLog(entry)) {
            return;
        }

        UpdateStats(entry);

        if (formatter_) {
            for (auto& sink : sinks_) {
                if (entry.level >= sink->GetLevel()) {
                    sink->Write(entry);
                }
            }
        }
    }

    void Logger::Log(LogLevel level, const std::string& category, const std::string& message) {
        LogEntry entry(level, category, message);
        WriteLog(entry);
    }

    void Logger::Trace(const std::string& category, const std::string& message) {
        Log(LogLevel::Trace, category, message);
    }

    void Logger::Debug(const std::string& category, const std::string& message) {
        Log(LogLevel::Debug, category, message);
    }

    void Logger::Info(const std::string& category, const std::string& message) {
        Log(LogLevel::Info, category, message);
    }

    void Logger::Warning(const std::string& category, const std::string& message) {
        Log(LogLevel::Warning, category, message);
    }

    void Logger::Error(const std::string& category, const std::string& message) {
        Log(LogLevel::Error, category, message);
    }

    void Logger::Critical(const std::string& category, const std::string& message) {
        Log(LogLevel::Critical, category, message);
    }

    void Logger::Trace(const std::string& message) {
        Log(LogLevel::Trace, "Default", message);
    }

    void Logger::Debug(const std::string& message) {
        Log(LogLevel::Debug, "Default", message);
    }

    void Logger::Info(const std::string& message) {
        Log(LogLevel::Info, "Default", message);
    }

    void Logger::Warning(const std::string& message) {
        Log(LogLevel::Warning, "Default", message);
    }

    void Logger::Error(const std::string& message) {
        Log(LogLevel::Error, "Default", message);
    }

    void Logger::Critical(const std::string& message) {
        Log(LogLevel::Critical, "Default", message);
    }

    bool Logger::ShouldLog(const LogEntry& entry) const {
        if (entry.level < level_) {
            return false;
        }

        for (auto& filter : filters_) {
            if (!filter->ShouldLog(entry)) {
                return false;
            }
        }

        return true;
    }

    void Logger::UpdateStats(const LogEntry& entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        stats_.totalLogs++;
        stats_.logsByLevel[static_cast<int>(entry.level)]++;
        stats_.lastLogTime = entry.timestamp;
    }

    void Logger::ProcessAsyncQueue() {
        if (!asyncData_) {
            return;
        }

        std::unique_lock<std::mutex> lock(asyncData_->queueMutex);
        while (!asyncData_->asyncQueue.empty()) {
            LogEntry entry = asyncData_->asyncQueue.front();
            asyncData_->asyncQueue.pop();
            lock.unlock();
            
            WriteLog(entry);
            lock.lock();
        }
    }

    void Logger::BeginPerformanceEvent(const std::string& name) {
        if (perfLoggingEnabled_) {
            // 实现性能事件开始
            LOG_DEBUG("Performance", "Begin: " + name);
        }
    }

    void Logger::EndPerformanceEvent(const std::string& name) {
        if (perfLoggingEnabled_) {
            // 实现性能事件结束
            LOG_DEBUG("Performance", "End: " + name);
        }
    }

    void Logger::ResetStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = LogStats{};
    }

    

    

    // 文件输出器实现
    class FileLogSink : public ILogSink {
    public:
        FileLogSink(const std::string& filename) : level_(LogLevel::Trace) {
            file_.open(filename, std::ios::app);
        }

        ~FileLogSink() {
            if (file_.is_open()) {
                file_.close();
            }
        }

        void Write(const LogEntry& entry) override {
            if (entry.level < level_ || !file_.is_open()) {
                return;
            }

            DefaultLogFormatter formatter;
            file_ << formatter.Format(entry) << std::endl;
        }

        void Flush() override {
            if (file_.is_open()) {
                file_.flush();
            }
        }

        void SetLevel(LogLevel level) override {
            level_ = level;
        }

        LogLevel GetLevel() const override {
            return level_;
        }

    private:
        std::ofstream file_;
        LogLevel level_;
    };

} // namespace HybridPBR