#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h> 
#include "core/Application.h"
#include "rendering/Shader.h"
#include "utils/FileIO.h"
#include "rendering/rasterization/Rasterizer.h"
#include "scene/Scene.h"
#include "resources/ResourceManager.h"
#include "utils/Logger.h"
#include "resources/ModelLoader.h"
#include "rendering/ImGuiManager.h"
#include"rendering/ImGuiComponentManager.h"

class TestApp : public HybridPBR::Application {
public:
    bool OnInitialize() override {
        // 创建测试着色器
        shader = std::make_unique<HybridPBR::Shader>();
        if (!shader->LoadFromFile(HybridPBR::FileIO::GetAssetsPath()+"shaders/basic.vert", 
            HybridPBR::FileIO::GetAssetsPath()+"shaders/basic.frag")) {
            return false;
        }
        
        // 设置三角形顶点数据
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,  // 左下
             0.5f, -0.5f, 0.0f,  // 右下
             0.0f,  0.5f, 0.0f   // 顶部
        };
        
        // 创建VAO, VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return true;
    }
    
    void OnUpdate(float deltaTime) override {
        // 简单的颜色动画
        time += deltaTime;
        float timeScaled =animate ? time*animationSpeed:time;
        color.r = (sin(timeScaled) + 1.0f) / 2.0f;
        color.g = (cos(timeScaled * 0.7f) + 1.0f) / 2.0f;
        color.b = (sin(timeScaled * 1.3f) + 1.0f) / 2.0f;
    }
    
    void OnRender() override {
        // 清除屏幕
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // 使用着色器
        shader->Use();
        shader->SetVec3("color", color);
        
        // 渲染三角形
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        // 检查错误
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            HybridPBR::LOG_ERROR("OpenGL error: " + std::to_string(error));
        }
    }
    
    void OnImGuiRender() override {
        // 创建一个控制面板窗口
        ImGui::Begin("Control Panel");
        
        ImGui::Text("Application Info");
        ImGui::Separator();
        ImGui::Text("Time: %.2f s", time);
        ImGui::Text("FPS: %.1f", GetTimer().GetFPS());
        
        ImGui::Spacing();
        
        // 颜色控制
        ImGui::Text("Triangle Color");
        ImGui::ColorEdit3("Color", (float*)&color);
        
        // 动画控制
        ImGui::Spacing();
        ImGui::Checkbox("Animate", &animate);
        
        if (animate) {
            ImGui::SliderFloat("Speed", &animationSpeed, 0.1f, 5.0f);
        }
        
        ImGui::End();
        
        // 显示ImGui演示窗口（可选）
        // ImGui::ShowDemoWindow();
    }
    
    void OnShutdown() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

private:
    std::unique_ptr<HybridPBR::Shader> shader;
    unsigned int VAO = 0, VBO = 0;
    float time = 0.0f;
    float animationSpeed = 1.0f;
    bool animate = true;
    glm::vec3 color = glm::vec3(1.0f, 0.5f, 0.2f);
};

class ThreeDApp : public HybridPBR::Application {
public:
    void OnWindowConfigChanged() override {
        windowConfig.width = 1920;
        windowConfig.height = 1080;
        windowConfig.title = "HybridPBR 3D Application";
        windowConfig.vsync = false;
        windowConfig.fullscreen = false;
    }
    bool OnInitialize() override {
        // 创建光栅化渲染器
        rasterizer = std::make_unique<HybridPBR::Rasterizer>();
        if (!rasterizer->Initialize()) {
            return false;
        }
        
        // 创建场景
        scene = std::make_unique<HybridPBR::Scene>();
        
        // 创建主相机
        auto camera = std::make_shared<HybridPBR::Camera>();
        camera->SetPerspective(45.0f, GetWindow().GetAspectRatio(), 0.1f, 100.0f);
        camera->SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
        scene->SetMainCamera(camera);
        
        // 尝试加载模型
        auto modelResult = HybridPBR::ModelLoader::LoadFromFile(HybridPBR::FileIO::GetAssetsPath()
                                                                             + "models/monkey.obj");
        if (modelResult.success && !modelResult.meshes.empty()) {
            // 使用加载的模型
            auto mesh = modelResult.meshes[0];
            auto material = modelResult.materials.empty() ? 
                HybridPBR::ResourceManager::GetInstance().CreateMaterial("Default", HybridPBR::MaterialProperties{}) : 
                modelResult.materials[0];
                
            // 创建场景节点
            auto modelNode = scene->CreateNode("Model");
            modelNode->SetMesh(mesh);
            modelNode->SetMaterial(material);
        } else {
            // 创建测试网格
            auto cubeMesh = std::make_shared<HybridPBR::Mesh>("TestCube");
            cubeMesh->GenerateCube(1.0f);
            
            // 创建材质
            HybridPBR::MaterialProperties props;
            props.albedo = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);
            props.metallic = 0.1f;
            props.roughness = 0.5f;
            
            auto material = HybridPBR::ResourceManager::GetInstance().CreateMaterial("RedMaterial", props);
            
            // 创建场景节点
            auto cubeNode = scene->CreateNode("Cube");
            auto Transform = cubeNode->GetTransform();

            cubeNode->SetMesh(cubeMesh);
            cubeNode->SetMaterial(material);
            cubeNode->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        }
        
        // 创建光源
        auto light = std::make_shared<HybridPBR::Light>(HybridPBR::LightType::DIRECTIONAL, "MainLight");
        light->SetDirection(glm::vec3(-0.5f, -1.0f, -0.5f));
        HybridPBR::LightProperties lightProps;
        lightProps.color = glm::vec3(1.0f, 1.0f, 0.9f);
        lightProps.intensity = 1.0f;
        light->SetProperties(lightProps);
        scene->AddLight(light);
        
        rotationSpeed = 45.0f; // 度/秒
        
        HybridPBR::LOG_INFO("Test application initialized with scene");
        return true;
    }
    
    void OnUpdate(float deltaTime) override {
        // 旋转模型
        if (auto modelNode = scene->FindNode("Model")) {
            auto rotation = modelNode->GetTransform().GetRotation();
            rotation.y += rotationSpeed * deltaTime;
            modelNode->GetTransform().SetRotation(rotation);
        } else if (auto cubeNode = scene->FindNode("Cube")) {
            auto rotation = cubeNode->GetTransform().GetRotation();
            rotation.y += rotationSpeed * deltaTime;
            cubeNode->GetTransform().SetRotation(rotation);
        }
        
        // 更新相机纵横比
        if (auto camera = scene->GetMainCamera()) {
            camera->SetViewport(GetWindow().GetWidth(), GetWindow().GetHeight());
        }
        
        scene->Update();
    }
    
    void OnRender() override {
        rasterizer->Render(*scene);
        
        // 显示渲染统计
        auto stats = rasterizer->GetStats();
        if (GetTimer().GetDeltaTime() > 0) {
            // 可以在这里显示FPS和渲染统计
        }
    }

    void OnImGuiRender() override {
        // 使用新的ImGui组件系统
        auto componentManager = imguiManager->GetComponentManager();
        
        componentManager->ShowSceneStats(scene);
        componentManager->ShowSceneHierarchy(scene);
        componentManager->ShowTransformEditor(componentManager->GetSelectedNode());

    }
    
    void OnShutdown() override {
        rasterizer->Shutdown();
    }

private:
    std::unique_ptr<HybridPBR::Rasterizer> rasterizer;
    std::unique_ptr<HybridPBR::Scene> scene;
    float rotationSpeed;
    
};


int main() {
    ThreeDApp app;
 
    if (app.Initialize()) {
        app.Run();
    }
    
    app.Shutdown();
    return 0;
}