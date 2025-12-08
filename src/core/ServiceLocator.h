#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <stdexcept>

namespace HybridPBR {

    /**
     * @brief 服务定位器模式实现
     * 提供全局依赖注入和生命周期管理
     * 
     * 修改理由：
     * 1. 解决全局单例模式的测试困难问题
     * 2. 提供统一的依赖管理
     * 3. 支持服务的生命周期控制
     * 4. 便于单元测试和模块解耦
     */
    class ServiceLocator {
    public:
        template<typename T>
        static void Register(std::shared_ptr<T> service) {
            GetInstance().services[typeid(T)] = service;
        }

        template<typename T>
        static std::shared_ptr<T> Resolve() {
            auto& instance = GetInstance();
            auto it = instance.services.find(typeid(T));
            if (it == instance.services.end()) {
                throw std::runtime_error("Service not registered: " + std::string(typeid(T).name()));
            }
            return std::static_pointer_cast<T>(it->second);
        }

        template<typename T>
        static bool IsRegistered() {
            auto& instance = GetInstance();
            return instance.services.find(typeid(T)) != instance.services.end();
        }

        static void Clear() {
            GetInstance().services.clear();
        }

    private:
        static ServiceLocator& GetInstance() {
            static ServiceLocator instance;
            return instance;
        }

        std::unordered_map<std::type_index, std::shared_ptr<void>> services;
    };

    // RAII服务注册器
    template<typename T>
    class ServiceRegistrar {
    public:
        ServiceRegistrar(std::shared_ptr<T> service) {
            ServiceLocator::Register<T>(service);
        }
    };

} // namespace HybridPBR