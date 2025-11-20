#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <functional>
#include "utils/Logger.h"

namespace HybridPBR {
    
    struct WindowConfig {
        int width = 1280;
        int height = 720;
        std::string title = "HybridPBR Renderer";
        bool vsync = true;
        bool fullscreen = false;

        WindowConfig() = default;
        WindowConfig(int width, int height, std::string title, bool vsync, bool fullscreen)
            : width(width), height(height), title(std::move(title)), 
              vsync(vsync), fullscreen(fullscreen) {};
    };

    class Window {
    public:
        using EventCallback = std::function<void()>;
        
        Window();
        ~Window();
        
        bool Initialize(const WindowConfig& config);
        void Shutdown();
        
        void ProcessEvents();
        void SwapBuffers();
        
        // 窗口状态
        bool ShouldClose() const;
        void SetShouldClose(bool shouldClose);
        
        // 获取窗口信息
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        float GetAspectRatio() const { return static_cast<float>(width) / height; }
        GLFWwindow* GetNativeWindow() const { return window; }
        
        // 输入回调设置
        void SetKeyCallback(std::function<void(int, int, int, int)> callback);
        void SetMouseCallback(std::function<void(double, double)> callback);
        void SetMouseButtonCallback(std::function<void(int, int, int)> callback);
        void SetScrollCallback(std::function<void(double, double)> callback);
        void SetResizeCallback(std::function<void(int, int)> callback);

    private:
        GLFWwindow* window = nullptr;
        int width = 0;
        int height = 0;
        std::string title;
        
        // 回调函数
        std::function<void(int, int, int, int)> keyCallback;
        std::function<void(double, double)> mouseCallback;
        std::function<void(int, int, int)> mouseButtonCallback;
        std::function<void(double, double)> scrollCallback;
        std::function<void(int, int)> resizeCallback;
        
        // 静态GLFW回调函数
        static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        
        void SetupCallbacks();
    };

} // namespace HybridPBR