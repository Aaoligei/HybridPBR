#include "Events.h"

namespace HybridPBR {

    void EventSystem::SubscribeInternal(const std::type_index& type, 
                                       std::function<void(const void*)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        eventMap_[type].callbacks.push_back(callback);
    }

    void EventSystem::PublishInternal(const std::type_index& type, const void* event) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = eventMap_.find(type);
        if (it != eventMap_.end()) {
            for (auto& callback : it->second.callbacks) {
                callback(event);
            }
        }
    }

    void EventSystem::PublishAsyncInternal(const std::type_index& type, const void* event) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = eventMap_.find(type);
        if (it != eventMap_.end()) {
            // 将事件存储到异步队列中
            it->second.asyncEvents.push_back([callback = it->second.callbacks, event](const void*) {
                for (auto& cb : callback) {
                    cb(event);
                }
            });
        }
    }

    void EventSystem::UnsubscribeAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        eventMap_.clear();
    }

    void EventSystem::ProcessAsyncEvents() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& [type, data] : eventMap_) {
            for (auto& asyncEvent : data.asyncEvents) {
                asyncEvent(nullptr);
            }
            data.asyncEvents.clear();
        }
    }

} // namespace HybridPBR