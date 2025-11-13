#pragma once
#include <memory>

struct GLFWwindow;
struct ImGuiContext;

namespace HybridPBR {

    class ImGuiComponentManager;

    class ImGuiManager {
    public:
        ImGuiManager();
        ~ImGuiManager();

        bool Initialize(GLFWwindow* window);
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        void ShowDebugInfo(float deltaTime, float fps);
        
        // 获取组件管理器
        std::shared_ptr<ImGuiComponentManager> GetComponentManager() { return componentManager; }

    private:
        ImGuiContext* context = nullptr;
        bool initialized = false;
        std::shared_ptr<ImGuiComponentManager> componentManager;
    };

} // namespace HybridPBR