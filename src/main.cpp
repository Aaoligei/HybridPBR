#include "core/Application.h"
#include "core/Result.h"
#include "utils/Logger.h"
#include "utils/FileIO.h"

// --- 新架构头文件 ---
#include "rendering/HybridRenderer.h"
#include "scene/Scene.h"
#include "resources/ModelLoader.h"
#include "rendering/rasterization/CameraController.h"
// 注意：现在使用的是 resources 目录下的纯数据 Mesh
#include "resources/Mesh.h" 
#include "rendering/common/Material.h"
#include "rendering/common/Light.h"

using namespace HybridPBR;

class TestApp : public Application {
public:
    // 1. 配置窗口
    void OnWindowConfigChanged() override {
        auto config = GetConfig();
        config.window.width = 1600;
        config.window.height = 900;
        config.window.title = "HybridPBR - RHI Refactor Test";
        config.window.vsync = true;
        SetConfig(config);
    }

    // 2. 初始化
    Result<void> OnInitialize() override {
        LOG_INFO("App", "Initializing TestApp with RHI Architecture...");

        // A. 初始化新版渲染器
        // 传入 nullptr，因为 HybridRenderer 内部会自动创建 OpenGLDevice
        m_renderer = std::make_unique<HybridRenderer>();
        Result<void> initRes = m_renderer->Initialize(nullptr);
        if (initRes.IsFailure()) {
            return initRes;
        }
        
        // 设置背景色 (测试 RHI Clear 指令)
        m_renderer->SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});

        // B. 初始化场景
        m_scene = std::make_unique<Scene>();
        m_scene->Initialize();

        // C. 设置相机
        auto camera = std::make_shared<Camera>();
        camera->SetPerspective(45.0f, GetWindow().GetAspectRatio(), 0.1f, 100.0f);
        camera->LookAt({0.0f, 2.0f, 8.0f}, {0.0f, 0.0f, 0.0f}); //稍微抬高视角
        m_scene->SetMainCamera(camera);

        m_cameraController = std::make_unique<CameraController>(camera.get());
        m_cameraController->SetMovementSpeed(5.0f); // 移动稍微快点

        // D. 加载场景内容 (保留原逻辑)
        CreateLights();
        CreatePBRTestSpheres();
        LoadModels(); // 椅子
        LoadCornellBox(); 

        LOG_INFO("App", "Scene loaded successfully. GPU resources will be created lazily on first frame.");
        return Result<void>::Success();
    }

    // 3. 更新逻辑
    Result<void> OnUpdate(float deltaTime) override {
        // 更新相机
        if (m_cameraController) {
            m_cameraController->Update(deltaTime);
        }
        
        // 简单的旋转动画 (测试 PushConstants 更新)
        static float time = 0.0f;
        time += deltaTime;
        
        // 让第一个球体旋转一下，证明 Transform 更新有效
        if (auto node = m_scene->FindNode("Chair")) {
            node->GetTransform().SetRotation({0.0f, time * 30.0f, 0.0f});
        }

        m_scene->Update(deltaTime);
        return Result<void>::Success();
    }

    // 4. 渲染循环
    Result<void> OnRender() override {
        // 调用新版渲染器
        // 内部流程: BeginFrame -> GeometryPass(Record Commands) -> EndFrame(Present)
        return m_renderer->Render(*m_scene);
    }

    // 5. 输入处理
    void OnMouseMoved(double x, double y) override {
        if (m_cameraController) m_cameraController->OnMouseMove(x, y);
    }
    void OnMouseClicked(int button) override {
        if (m_cameraController) m_cameraController->OnMouseButton(button, 1, 0); // 1 = Press
    }
    void OnMouseReleased(int button) override {
        if (m_cameraController) m_cameraController->OnMouseButton(button, 0, 0); // 0 = Release
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

    // --- 场景构建辅助函数 (复刻自 DefferedApplication.h) ---

    void CreateLights() {
        // 创建一个简单的方向光
        auto light = std::make_shared<Light>(LightType::DIRECTIONAL, "Sun");
        light->SetDirection({-1.0f, -1.0f, -1.0f});
        light->GetProperties().color = {1.0f, 0.95f, 0.8f};
        light->GetProperties().intensity = 5.0f;
        m_scene->AddLight(light);
    }

    void CreatePBRTestSpheres() {
        // 创建共享的球体网格 (纯 CPU 数据)
        auto sphereMesh = std::make_shared<Mesh>("SphereMesh");
        sphereMesh->GenerateSphere(1.0f, 64);

        int gridSize = 3;
        float spacing = 2.5f;

        for (int i = 0; i < 6; ++i) { // 创建6个不同材质的球
            // 创建节点
            auto nodeResult = m_scene->CreateNode("Sphere_" + std::to_string(i));
            if (nodeResult.IsFailure()) continue;
            auto node = nodeResult.GetValue();

            // 设置网格
            node->SetMesh(sphereMesh);

            // 设置材质 (注意：目前的 GeometryPass 只是画出形状，尚未完全接入 PBR 材质参数到 Shader)
            // 但我们需要设置 Material 对象以避免空指针
            auto mat = std::make_shared<Material>("Mat_" + std::to_string(i));
            
            // 设置位置
            int row = i / gridSize;
            int col = i % gridSize;
            float x = (col - gridSize / 2.0f + 0.5f) * spacing;
            float y = (row - gridSize / 2.0f + 0.5f) * spacing;
            
            node->GetTransform().SetPosition({x, y + 5.0f, 0.0f}); // 放在空中
            node->SetMaterial(mat);
        }
    }

    void LoadModels() {
        // 加载椅子
        std::string path = FileIO::GetAssetsPath() + "models/mid_century_lounge_chair_4k.gltf/mid_century_lounge_chair_4k.gltf";
        auto result = ModelLoader::LoadFromFile(path);
        
        if (result.success && !result.meshes.empty()) {
            auto nodeRes = m_scene->CreateNode("Chair");
            if (nodeRes.IsSuccess()) {
                auto node = nodeRes.GetValue();
                // 这里的 mesh 已经是新版 Mesh (纯数据)
                node->SetMesh(result.meshes[0]); 
                // 设置材质 (暂时使用默认材质，如果 ModelLoader 加载了材质也可以用)
                if (!result.materials.empty()) {
                    node->SetMaterial(result.materials[0]);
                } else {
                    node->SetMaterial(std::make_shared<Material>("ChairMat"));
                }
                node->GetTransform().SetPosition({0.0f, -2.0f, 2.0f});
                node->GetTransform().SetScale({2.0f, 2.0f, 2.0f});
            }
        } else {
            LOG_WARNING("TestApp", "Failed to load chair model: " + path);
        }
    }

    void LoadCornellBox() {
        std::string path = FileIO::GetAssetsPath() + "models/CornellBox-Original/CornellBox-Original.obj";
        auto result = ModelLoader::LoadFromFile(path);

        if (result.success) {
            for (size_t i = 0; i < result.meshes.size(); ++i) {
                auto nodeRes = m_scene->CreateNode("CornellBox_" + std::to_string(i));
                if (nodeRes.IsSuccess()) {
                    auto node = nodeRes.GetValue();
                    node->SetMesh(result.meshes[i]);
                    // 同样，材质暂时从简
                    if (i < result.materials.size()) {
                        node->SetMaterial(result.materials[i]);
                    } else {
                        node->SetMaterial(std::make_shared<Material>("BoxMat"));
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