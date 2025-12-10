#include "core/Application.h"
#include "core/Result.h"
#include "utils/Logger.h"
#include "utils/FileIO.h"

// --- 新架构头文件 ---
#include "rendering/HybridRenderer.h"
#include "scene/Scene.h"
#include "resources/ModelLoader.h"
#include "rendering/rasterization/CameraController.h"
#include "resources/Mesh.h" 
#include "rendering/common/Material.h"
#include "rendering/common/Light.h"

// 包含 RHI 以便使用 RHI_Device 类型
#include "rhi/RHI_Device.h"

using namespace HybridPBR;

class TestApp : public Application {
public:
    // 1. 配置窗口
    void OnWindowConfigChanged() override {
        auto config = GetConfig();
        config.window.width = 1600;
        config.window.height = 900;
        config.window.title = "HybridPBR - RHI Refactor Test (Bindless)";
        config.window.vsync = true;
        SetConfig(config);
    }

    // 2. 初始化
    Result<void> OnInitialize() override {
        LOG_INFO("App", "Initializing TestApp with RHI Architecture...");

        // A. 初始化新版渲染器
        // 传入 nullptr，因为 HybridRenderer 内部会自动创建 OpenGLDevice
        m_renderer = std::make_unique<HybridRenderer>();
        Result<void> initRes = m_renderer->Initialize(); // [修改] 移除参数
        if (initRes.IsFailure()) {
            return initRes;
        }
        
        // 获取 RHI Device 指针，供资源创建使用
        RHI_Device* device = m_renderer->GetRHIDevice(); // [修改] 需要在 HybridRenderer 中公开此方法

        // 设置背景色
        m_renderer->SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});

        // B. 初始化场景
        m_scene = std::make_unique<Scene>();
        m_scene->Initialize();

        // C. 设置相机
        auto camera = std::make_shared<Camera>();
        camera->SetPerspective(45.0f, GetWindow().GetAspectRatio(), 0.1f, 100.0f);
        camera->LookAt({0.0f, 2.0f, 8.0f}, {0.0f, 0.0f, 0.0f});
        m_scene->SetMainCamera(camera);

        m_cameraController = std::make_unique<CameraController>(camera.get());
        m_cameraController->SetMovementSpeed(5.0f);

        // D. 加载场景内容
        CreateLights();
        
        // [修改] 传递 device
        CreatePBRTestSpheres(device);
        LoadModels(device); 
        LoadCornellBox(device); 

        LOG_INFO("App", "Scene loaded successfully.");
        return Result<void>::Success();
    }

    // 3. 更新逻辑
    Result<void> OnUpdate(float deltaTime) override {
        if (m_cameraController) {
            m_cameraController->Update(deltaTime);
        }
        
        // 简单的旋转动画
        static float time = 0.0f;
        time += deltaTime;
        
        if (auto node = m_scene->FindNode("Chair")) {
            node->GetTransform().SetRotation({0.0f, time * 30.0f, 0.0f});
        }

        m_scene->Update(deltaTime);
        return Result<void>::Success();
    }

    // 4. 渲染循环
    Result<void> OnRender() override {
        return m_renderer->Render(*m_scene);
    }

    // 5. 输入处理
    void OnMouseMoved(double x, double y) override {
        if (m_cameraController) m_cameraController->OnMouseMove(x, y);
    }
    void OnMouseClicked(int button) override {
        if (m_cameraController) m_cameraController->OnMouseButton(button, 1, 0); 
    }
    void OnMouseReleased(int button) override {
        if (m_cameraController) m_cameraController->OnMouseButton(button, 0, 0); 
    }
    void OnMouseScroll(double xoffset, double yoffset) override {
        if (m_cameraController) m_cameraController->OnMouseScroll(xoffset, yoffset);
    }
    void OnWindowResized(int width, int height) override {
        if (m_renderer) m_renderer->Resize(width, height);
        if (m_scene && m_scene->GetMainCamera()) 
            m_scene->GetMainCamera()->SetViewport(width, height);
    }

private:
    std::unique_ptr<HybridRenderer> m_renderer;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<CameraController> m_cameraController;

    void CreateLights() {
        auto light = std::make_shared<Light>(LightType::DIRECTIONAL, "Sun");
        light->SetDirection({-1.0f, -1.0f, -1.0f});
        light->GetProperties().color = {1.0f, 0.95f, 0.8f};
        light->GetProperties().intensity = 5.0f;
        m_scene->AddLight(light);
    }

    // [修改] 增加 device 参数
    void CreatePBRTestSpheres(RHI_Device* device) {
        auto sphereMesh = std::make_shared<Mesh>("SphereMesh");
        sphereMesh->GenerateSphere(1.0f, 64);

        int gridSize = 3;
        float spacing = 2.5f;

        // 材质参数组合 (金属度, 粗糙度)
        std::vector<glm::vec2> params = {
            {0.0f, 0.1f}, {0.0f, 0.5f}, {0.0f, 0.9f}, // 非金属
            {1.0f, 0.1f}, {1.0f, 0.5f}, {1.0f, 0.9f}  // 金属
        };
        
        std::vector<glm::vec3> colors = {
            {0.8f, 0.2f, 0.2f}, {0.2f, 0.8f, 0.2f}, {0.2f, 0.2f, 0.8f},
            {0.8f, 0.8f, 0.2f}, {0.8f, 0.2f, 0.8f}, {0.2f, 0.8f, 0.8f}
        };

        for (int i = 0; i < 6; ++i) { 
            auto nodeResult = m_scene->CreateNode("Sphere_" + std::to_string(i));
            if (nodeResult.IsFailure()) continue;
            auto node = nodeResult.GetValue();

            node->SetMesh(sphereMesh);

            // [修改] 使用新版 Material API
            auto mat = std::make_shared<Material>("Mat_" + std::to_string(i));
            
            // 1. 初始化 UBO
            mat->Initialize(device); 
            
            // 2. 使用 Setter 设置属性 (这会自动更新 UBO 数据结构)
            glm::vec3 color = colors[i % colors.size()];
            float metallic = params[i].x;
            float roughness = params[i].y;

            mat->SetAlbedoColor(glm::vec4(color, 1.0f));
            mat->SetMetallic(metallic);
            mat->SetRoughness(roughness);
            mat->SetAO(1.0f);
            
            // 3. 将数据上传到 GPU (UBO)
            mat->UpdateToGPU();

            node->SetMaterial(mat);
            
            // 设置位置
            int row = i / gridSize;
            int col = i % gridSize;
            float x = (col - gridSize / 2.0f + 0.5f) * spacing;
            float y = (row - gridSize / 2.0f + 0.5f) * spacing;
            
            node->GetTransform().SetPosition({x, y + 5.0f, 0.0f});
        }
    }

    // [修改] 增加 device 参数
    void LoadModels(RHI_Device* device) {
        std::string path = FileIO::GetAssetsPath() + "models/mid_century_lounge_chair_4k.gltf/mid_century_lounge_chair_4k.gltf";

        // [修改] 调用 ModelLoader 时传入 device
        auto result = ModelLoader::LoadFromFile(path, device);
        
        if (result.success && !result.meshes.empty()) {
            auto nodeRes = m_scene->CreateNode("Chair");
            if (nodeRes.IsSuccess()) {
                auto node = nodeRes.GetValue();
                node->SetMesh(result.meshes[0]); 
                
                if (!result.materials.empty()) {
                    node->SetMaterial(result.materials[0]);
                } else {
                    // 创建默认材质并初始化
                    auto defMat = std::make_shared<Material>("ChairMat");
                    defMat->Initialize(device);
                    defMat->UpdateToGPU();
                    node->SetMaterial(defMat);
                }
                node->GetTransform().SetPosition({0.0f, -2.0f, 2.0f});
                node->GetTransform().SetScale({2.0f, 2.0f, 2.0f});
            }
        } else {
            LOG_WARNING("TestApp", "Failed to load chair model: " + path);
        }
    }

    // [修改] 增加 device 参数
    void LoadCornellBox(RHI_Device* device) {
        std::string path = FileIO::GetAssetsPath() + "models/CornellBox-Original/CornellBox-Original.obj";
        
        // [修改] 传入 device
        auto result = ModelLoader::LoadFromFile(path, device);

        if (result.success) {
            for (size_t i = 0; i < result.meshes.size(); ++i) {
                auto nodeRes = m_scene->CreateNode("CornellBox_" + std::to_string(i));
                if (nodeRes.IsSuccess()) {
                    auto node = nodeRes.GetValue();
                    node->SetMesh(result.meshes[i]);
                    
                    if (i < result.materials.size()) {
                        node->SetMaterial(result.materials[i]);
                    } else {
                        auto defMat = std::make_shared<Material>("BoxMat");
                        defMat->Initialize(device);
                        defMat->UpdateToGPU();
                        node->SetMaterial(defMat);
                    }
                    node->GetTransform().SetPosition({0.0f, 5.0f, -5.0f});
                }
            }
        } else {
            LOG_WARNING("TestApp", "Failed to load Cornell Box: " + path);
        }
    }
};

int main() {
    TestApp app;
    
    if (app.Initialize().IsFailure()) {
        LOG_ERROR("Main", "Failed to initialize application");
        return -1;
    }
    
    if (app.Run().IsFailure()) {
        LOG_ERROR("Main", "Application run failed");
        return -1;
    }
    
    return 0;
}