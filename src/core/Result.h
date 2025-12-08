#pragma once

#include <string>
#include <variant>
#include <optional>
#include <stdexcept>

namespace HybridPBR {

    /**
     * @brief 错误类型枚举
     * 提供统一的错误分类
     */
    enum class ErrorType {
        None,
        Initialization,
        ResourceNotFound,
        InvalidParameter,
        OutOfMemory,
        DeviceLost,
        ShaderCompilation,
        TextureCreation,
        BufferCreation,
        RenderPass
    };

    /**
     * @brief 错误信息结构
     * 包含错误类型、消息和可选的错误代码
     */
    struct Error {
        ErrorType type;
        std::string message;
        int code = 0;

        Error(ErrorType t, const std::string& msg, int c = 0)
            : type(t), message(msg), code(c) {}
    };

    /**
     * @brief 结果类型模板
     * 提供统一的成功/失败处理机制
     * 
     * 修改理由：
     * 1. 替代异常处理，提高性能
     * 2. 强制错误处理，避免未检查的错误
     * 3. 提供链式操作支持
     * 4. 统一错误报告格式
     */
    template<typename T>
    class Result {
    public:
        static Result Success(T value) {
            return Result(std::move(value));
        }

        static Result Failure(Error error) {
            return Result(std::move(error));
        }

        bool IsSuccess() const { return std::holds_alternative<T>(data_); }
        bool IsFailure() const { return std::holds_alternative<Error>(data_); }

        const T& GetValue() const {
            if (IsFailure()) {
                throw std::runtime_error("Attempting to get value from failed result");
            }
            return std::get<T>(data_);
        }

        T& GetValue() {
            if (IsFailure()) {
                throw std::runtime_error("Attempting to get value from failed result");
            }
            return std::get<T>(data_);
        }

        const Error& GetError() const {
            if (IsSuccess()) {
                throw std::runtime_error("Attempting to get error from successful result");
            }
            return std::get<Error>(data_);
        }

        // 链式操作支持
        template<typename F>
        auto Map(F&& func) -> Result<decltype(func(std::declval<T>()))> {
            using ReturnType = decltype(func(std::declval<T>()));
            
            if (IsFailure()) {
                return Result<ReturnType>::Failure(GetError());
            }
            
            try {
                return Result<ReturnType>::Success(func(GetValue()));
            } catch (const std::exception& e) {
                return Result<ReturnType>::Failure(Error(ErrorType::InvalidParameter, e.what()));
            }
        }

        // 可选值转换
        std::optional<T> ToOptional() const {
            if (IsSuccess()) {
                return GetValue();
            }
            return std::nullopt;
        }

    private:
        Result(T value) : data_(std::move(value)) {}
        Result(Error error) : data_(std::move(error)) {}

        std::variant<T, Error> data_;
    };

    // void特化
    template<>
    class Result<void> {
    public:
        static Result Success() {
            return Result();
        }

        static Result Failure(Error error) {
            return Result(std::move(error));
        }

        bool IsSuccess() const { return !error_.has_value(); }
        bool IsFailure() const { return error_.has_value(); }

        const Error& GetError() const {
            if (IsSuccess()) {
                throw std::runtime_error("Attempting to get error from successful result");
            }
            return *error_;
        }

    private:
        Result() = default;
        Result(Error error) : error_(std::move(error)) {}

        std::optional<Error> error_;
    };

    // 便利宏
    #define RETURN_IF_ERROR(result) \
        do { \
            auto _res = (result); \
            if (_res.IsFailure()) { \
                return Result<void>::Failure(_res.GetError()); \
            } \
        } while(0)

    #define TRY_ASSIGN(var, result) \
        auto _result_##var = (result); \
        if (_result_##var.IsFailure()) { \
            return Result<void>::Failure(_result_##var.GetError()); \
        } \
        auto var = _result_##var.GetValue()

} // namespace HybridPBR
