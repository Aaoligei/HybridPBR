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
#include "rendering/ImGuiComponentManager.h"
#include "pbr/IBL.h"
#include "pbr/PBRMaterial.h"

 

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
    
    void CreatePBRTestSpheres() {
        // 创建不同金属度和粗糙度的测试球体
        std::vector<glm::vec2> materialParams = {
            {0.0f, 0.1f},  // 非金属，光滑
            {0.0f, 0.5f},  // 非金属，中等粗糙
            {0.0f, 0.9f},  // 非金属，粗糙
            {1.0f, 0.1f},  // 金属，光滑
            {1.0f, 0.5f},  // 金属，中等粗糙
            {1.0f, 0.9f}   // 金属，粗糙
        };
        
        std::vector<glm::vec3> colors = {
            {0.8f, 0.2f, 0.2f},  // 红
            {0.2f, 0.8f, 0.2f},  // 绿
            {0.2f, 0.2f, 0.8f},  // 蓝
            {0.8f, 0.8f, 0.2f},  // 黄
            {0.8f, 0.2f, 0.8f},  // 紫
            {0.2f, 0.8f, 0.8f}   // 青
        };
        
        int gridSize = 3;
        float spacing = 2.5f;
        auto sphere_shader=HybridPBR::ShaderManager::GetInstance().LoadShader("SphereShader",
            HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.vert", 
             HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.frag");
            
        HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_basecolor.png",HybridPBR::TextureType::DIFFUSE);
        HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_normal.png",HybridPBR::TextureType::NORMAL);
        HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_metallic.png",HybridPBR::TextureType::METALLIC);
        HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_roughness.png",HybridPBR::TextureType::ROUGHNESS);
        HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_ambientocclusion.png",HybridPBR::TextureType::AMBIENT_OCCLUSION);


        for (int i = 0; i < materialParams.size(); ++i) {
            auto sphereMesh = std::make_shared<HybridPBR::Mesh>("Sphere_" + std::to_string(i));
            sphereMesh->GenerateSphere(1.0f, 32);
            
            HybridPBR::MaterialProperties props;
            props.albedo = glm::vec4(colors[i], 1.0f);
            props.metallic = materialParams[i].x;
            props.roughness = materialParams[i].y;
            props.ambientOcclusion = 1.0f;
            props.customShaderName = "null";
            props.shaderType = HybridPBR::ShaderType::PBR;
            
            auto pbrMaterial = std::make_shared<HybridPBR::PBRMaterial>("PBR_Sphere_" + std::to_string(i), props);
            pbrMaterial->SetTexture(HybridPBR::TextureType::DIFFUSE,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_basecolor.png"));
            pbrMaterial->SetTexture(HybridPBR::TextureType::NORMAL,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_normal.png"));
            pbrMaterial->SetTexture(HybridPBR::TextureType::METALLIC,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_metallic.png"));
            pbrMaterial->SetTexture(HybridPBR::TextureType::ROUGHNESS,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_roughness.png"));
            pbrMaterial->SetTexture(HybridPBR::TextureType::AMBIENT_OCCLUSION,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_ambientocclusion.png"));

            
            int row = i / gridSize;
            int col = i % gridSize;
            
            float x = (col - gridSize / 2.0f + 0.5f) * spacing;
            float y = (row - gridSize / 2.0f + 0.5f) * spacing;
            
            auto sphereNode = scene->CreateNode("Sphere_" + std::to_string(i));
            sphereNode->SetMesh(sphereMesh);
            sphereNode->SetMaterial(pbrMaterial);
            sphereNode->GetTransform().SetPosition(glm::vec3(x, y, 0.0f));

        }
         // 创建一个立方体作为对比
            auto cubeMesh = std::make_shared<HybridPBR::Mesh>("Cube");
            cubeMesh->GenerateCube(1.0f);
            
            HybridPBR::MaterialProperties props;
            props.albedo = glm::vec4(0.2f, 0.7f, 0.3f, 1.0f);
            props.metallic = 0.5f;
            props.roughness = 0.3f;
            props.ambientOcclusion = 1.0f;
            props.customShaderName = "null";
            props.shaderType = HybridPBR::ShaderType::PBR;
            
            auto cubeMaterial = std::make_shared<HybridPBR::PBRMaterial>("PBR_Cube", props);
            cubeMaterial->SetTexture(HybridPBR::TextureType::DIFFUSE,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_basecolor.png"));
            cubeMaterial->SetTexture(HybridPBR::TextureType::NORMAL,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_normal.png"));
            cubeMaterial->SetTexture(HybridPBR::TextureType::METALLIC,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_metallic.png"));
            cubeMaterial->SetTexture(HybridPBR::TextureType::ROUGHNESS,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_roughness.png"));
            cubeMaterial->SetTexture(HybridPBR::TextureType::AMBIENT_OCCLUSION,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_ambientocclusion.png"));

            
            auto cubeNode = scene->CreateNode("Cube");
            cubeNode->SetMesh(cubeMesh);
            cubeNode->SetMaterial(cubeMaterial);
            cubeNode->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
    }
    void LoadModels() {
         // 尝试加载模型
        auto modelResult = HybridPBR::ModelLoader::LoadFromFile(HybridPBR::FileIO::GetAssetsPath()
         + "models/mid_century_lounge_chair_4k.gltf/mid_century_lounge_chair_4k.gltf");
        if (modelResult.success && !modelResult.meshes.empty()) {
            // 使用加载的模型
            auto mesh = modelResult.meshes[0];
            auto material = modelResult.materials.empty() ? 
                HybridPBR::ResourceManager::GetInstance().CreateMaterial("Default", HybridPBR::MaterialProperties{}) : 
                modelResult.materials[0];
            material->SetShaderType(HybridPBR::ShaderType::PBR);
            // 创建场景节点
            auto modelNode = scene->CreateNode("Model");
            modelNode->SetMesh(mesh);
            modelNode->SetMaterial(material);
            modelNode->GetTransform().SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
        } 
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
        camera = std::make_shared<HybridPBR::Camera>();
        camera->SetPerspective(45.0f, GetWindow().GetAspectRatio(), 0.1f, 100.0f);
        camera->SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
        scene->SetMainCamera(camera);

        // 初始化相机控制器
        cameraController = std::make_unique<HybridPBR::CameraController>(camera.get());
        cameraController->SetMovementSpeed(0.05f);
        cameraController->SetMouseSensitivity(0.1f);
        cameraController->SetZoomSensitivity(0.3f);

        // 创建PBR材质测试球体
        CreatePBRTestSpheres();
        
        // 加载3D模型
        LoadModels();
        // 创建光源
        auto light = std::make_shared<HybridPBR::Light>(HybridPBR::LightType::POINT, "MainLight");
        light->SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
        HybridPBR::LightProperties lightProps;
        lightProps.color = glm::vec3(1.0f, 1.0f, 0.9f);
        lightProps.intensity = 150.0f;
        light->SetProperties(lightProps);
        scene->AddLight(light);
        
        rotationSpeed = 45.0f; // 度/秒
        
        HybridPBR::LOG_INFO("Test application initialized with scene");
        return true;
    }
    
    void OnUpdate(float deltaTime) override {
        
        // 更新相机控制器
        if (cameraController) {
            cameraController->Update(deltaTime);
        }
        
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

    }

    void OnImGuiRender() override {
        ImGui::SetWindowFontScale(1.5f);
        // 显示渲染统计
        auto stats = rasterizer->GetStats();
        ImGui::Begin("Renderer Stats");
        ImGui::Text("Renderer Stats");
        ImGui::Text("Draw calls: %d", stats.drawCalls);
        ImGui::Text("Triangles: %d", stats.triangleCount);
        ImGui::Text("Vertices: %d", stats.vertexCount);
        if(ImGui::CollapsingHeader("Render Passes")){
            for(auto passName : rasterizer->GetRenderPassNames()){
                ImGui::Text("%s", passName.c_str());
            }
        }
        ImGui::End();

        auto componentManager = imguiManager->GetComponentManager();
        
        componentManager->ShowSceneStats(scene);
        componentManager->ShowSceneHierarchy(scene);
        componentManager->ShowInspector(componentManager->GetSelectedNode());
        componentManager->ShowLightHierarchy(scene);

    }
    
    void OnShutdown() override {
        rasterizer->Shutdown();
    }

private:
    std::unique_ptr<HybridPBR::Rasterizer> rasterizer;
    std::unique_ptr<HybridPBR::Scene> scene;
    std::shared_ptr<HybridPBR::Camera> camera;
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