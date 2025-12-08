#include <glad/glad.h>
#include "Window.h"
#include "Input.h"
#include <GLFW/glfw3.h>

namespace HybridPBR {
    
    Window::Window() {
        // GLFW初始化
        if (!glfwInit()) {
            LOG_CRITICAL("Window", "Failed to initialize GLFW");
            return;
        }
        
        // 设置OpenGL版本和配置
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        #ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
    }
    
    Window::~Window() {
        Shutdown();
    }
    
    bool Window::Initialize(const WindowConfig& config) {
        width = config.width;
        height = config.height;
        title = config.title;
        
        // 创建窗口
        window = glfwCreateWindow(width, height, title.c_str(), 
                                 config.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
        if (!window) {
            LOG_CRITICAL("Window", "Failed to create GLFW window");
            return false;
        }
        
        glfwMakeContextCurrent(window);
        
        // 初始化GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            LOG_CRITICAL("Window", "Failed to initialize GLAD");
            return false;
        }
        
        // 设置VSync
        glfwSwapInterval(config.vsync ? 1 : 0);
        
        // 设置回调
        SetupCallbacks();
        
        LOG_INFO("Window", "Window created: " + std::to_string(width) + "x" + std::to_string(height));
        LOG_INFO("Window", "OpenGL Version: " + std::string((char*)glGetString(GL_VERSION)));
        
        return true;
    }
    
    void Window::Shutdown() {
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
    }
    
    void Window::ProcessEvents() {
        glfwPollEvents();
    }
    
    void Window::SwapBuffers() {
        glfwSwapBuffers(window);
    }
    
    bool Window::ShouldClose() const {
        return glfwWindowShouldClose(window);
    }
    
    void Window::SetShouldClose(bool shouldClose) {
        glfwSetWindowShouldClose(window, shouldClose);
    }
    
    void Window::SetupCallbacks() {
        glfwSetWindowUserPointer(window, this);
        
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetCursorPosCallback(window, MouseCallback);
        glfwSetScrollCallback(window, ScrollCallback);
    }
    
    // 静态回调函数
    void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win) {
            win->width = width;
            win->height = height;
            glViewport(0, 0, width, height);
            if (win->resizeCallback) {
                win->resizeCallback(width, height);
            }
        }
    }
    
    void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win) {
            Input::GetInstance().SetKeyState(key, action != GLFW_RELEASE);
            if (win->keyCallback) {
                win->keyCallback(key, scancode, action, mods);
            }
        }
    }
    
    void Window::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win) {
            Input::GetInstance().SetMousePositionInternal(xpos, ypos);
            if (win->mouseCallback) {
                win->mouseCallback(xpos, ypos);
            }
        }
    }

    void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win) {
            Input::GetInstance().SetMouseButtonState(button, action != GLFW_RELEASE);
            if (win->mouseButtonCallback) {
                win->mouseButtonCallback(button, action, mods);
            }
        }
    }
    
    void Window::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win && win->scrollCallback) {
            win->scrollCallback(xoffset, yoffset);
        }
    }
    
    // 回调设置方法
    void Window::SetKeyCallback(std::function<void(int, int, int, int)> callback) {
        keyCallback = callback;
    }
    
    void Window::SetMouseCallback(std::function<void(double, double)> callback) {
        mouseCallback = callback;
    }

    void Window::SetMouseButtonCallback(std::function<void(int, int, int)> callback) {
        mouseButtonCallback = callback;
    }
    
    void Window::SetScrollCallback(std::function<void(double, double)> callback) {
        scrollCallback = callback;
    }
    
    void Window::SetResizeCallback(std::function<void(int, int)> callback) {
        resizeCallback = callback;
    }

} // namespace HybridPBR