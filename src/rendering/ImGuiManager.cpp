#include "ImGuiManager.h"
#include "ImGuiComponentManager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include "utils/Logger.h"

namespace HybridPBR {

    ImGuiManager::ImGuiManager() {
        componentManager = std::make_shared<ImGuiComponentManager>();
    }

    ImGuiManager::~ImGuiManager() {
        Shutdown();
    }

    bool ImGuiManager::Initialize(GLFWwindow* window) {
        if (initialized) {
            return true;
        }

        // 创建ImGui上下文
        IMGUI_CHECKVERSION();
        context = ImGui::CreateContext();
        if (!context) {
            LOG_ERROR("Failed to create ImGui context");
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // 启用键盘控制
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // 启用Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // 启用多视口支持

        // 设置样式
        ImGui::StyleColorsDark();
        
        // 启用视口后，调整样式以适应
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // 初始化平台和渲染器后端
        if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
            LOG_ERROR("Failed to initialize ImGui GLFW backend");
            return false;
        }

        // 获取OpenGL版本字符串
        const char* glsl_version = "#version 330";
        if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
            LOG_ERROR("Failed to initialize ImGui OpenGL3 backend");
            return false;
        }

        initialized = true;
        LOG_INFO("ImGui initialized successfully with docking support");
        return true;
    }

    void ImGuiManager::Shutdown() {
        if (!initialized) {
            return;
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(context);

        initialized = false;
        context = nullptr;
        LOG_INFO("ImGui shutdown completed");
    }

    void ImGuiManager::BeginFrame() {
        if (!initialized) {
            return;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiManager::EndFrame() {
        if (!initialized) {
            return;
        }

        // 渲染
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // 更新并渲染额外的视口(平台窗口)
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    void ImGuiManager::ShowDebugInfo(float deltaTime, float fps) {
        if (!initialized) {
            return;
        }

        // 创建一个简单的调试窗口
        ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_NoTitleBar | 
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        
        ImGui::SetWindowPos(ImVec2(10, 10));
        
        ImGui::Text("Application Metrics");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
        
        ImGui::End();
    }

} // namespace HybridPBR