#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <mutex>
#include <string>

namespace HybridPBR {

    /**
     * @brief 事件系统接口
     * 提供类型安全的事件分发机制
     * 
     * 修改理由：
     * 1. 解耦组件间的直接依赖
     * 2. 提供类型安全的事件通信
     * 3. 支持异步事件处理
     * 4. 便于实现观察者模式
     */
    class IEventSystem {
    public:
        virtual ~IEventSystem() = default;
        
        template<typename EventType>
        using EventCallback = std::function<void(const EventType&)>;
        
        // 事件订阅
        template<typename EventType>
        void Subscribe(EventCallback<EventType> callback);
        
        // 事件发布
        template<typename EventType>
        void Publish(const EventType& event);
        
        // 异步事件发布
        template<typename EventType>
        void PublishAsync(const EventType& event);
        
        // 取消订阅
        template<typename EventType>
        void Unsubscribe(EventCallback<EventType> callback);
        
    protected:
        protected:
        virtual void SubscribeInternal(const std::type_index& type, 
                             std::function<void(const void*)> callback) = 0;
        virtual void PublishInternal(const std::type_index& type, const void* event) = 0;
        virtual void PublishAsyncInternal(const std::type_index& type, const void* event) = 0;
        
        // 取消订阅
        virtual void UnsubscribeAll() = 0;
        virtual void ProcessAsyncEvents() = 0;
    };

    /**
     * @brief 事件系统实现
     */
    class EventSystem : public IEventSystem {
    public:
        EventSystem() = default;
        ~EventSystem() override = default;

    protected:
        void SubscribeInternal(const std::type_index& type, 
                             std::function<void(const void*)> callback) override;
        void PublishInternal(const std::type_index& type, const void* event) override;
        void PublishAsyncInternal(const std::type_index& type, const void* event) override;

    public:
        void UnsubscribeAll() override;
        void ProcessAsyncEvents() override;

    private:
        struct EventData {
            std::vector<std::function<void(const void*)>> callbacks;
            std::vector<std::function<void(const void*)>> asyncEvents;
        };
        
        std::unordered_map<std::type_index, EventData> eventMap_;
        std::mutex mutex_;
    };

    // 模板方法实现
    template<typename EventType>
    void IEventSystem::Subscribe(EventCallback<EventType> callback) {
        SubscribeInternal(typeid(EventType), [callback](const void* event) {
            callback(*static_cast<const EventType*>(event));
        });
    }
    
    template<typename EventType>
    void IEventSystem::Publish(const EventType& event) {
        PublishInternal(typeid(EventType), &event);
    }
    
    template<typename EventType>
    void IEventSystem::PublishAsync(const EventType& event) {
        PublishAsyncInternal(typeid(EventType), &event);
    }
    
    template<typename EventType>
    void IEventSystem::Unsubscribe(EventCallback<EventType> callback) {
        // 注意：由于函数对象无法直接比较，这里简化实现
        // 实际项目中可能需要使用回调ID或其他机制
    }

    // 预定义事件类型
    struct WindowResizeEvent {
        int width;
        int height;
    };
    
    struct KeyEvent {
        int key;
        int action;
        int mods;
    };
    
    struct MouseEvent {
        double x;
        double y;
        int button;
        int action;
    };
    
    struct SceneChangedEvent {
        std::string nodeName;
        bool added;
    };
    
    struct ResourceLoadedEvent {
        std::string resourcePath;
        std::string resourceType;
    };

} // namespace HybridPBR