#pragma once
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
            
        // HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_basecolor.png",HybridPBR::TextureType::DIFFUSE);
        // HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_normal.png",HybridPBR::TextureType::NORMAL);
        // HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_metallic.png",HybridPBR::TextureType::METALLIC);
        // HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_roughness.png",HybridPBR::TextureType::ROUGHNESS);
        // HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_ambientocclusion.png",HybridPBR::TextureType::AMBIENT_OCCLUSION);

        HybridPBR::ShaderManager::GetInstance().LoadShader("sphere_pbr",HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.vert", HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.frag");
        iblSystem->BindIBLTextures(HybridPBR::ShaderManager::GetInstance().GetShader("sphere_pbr"));
        HybridPBR::ResourceManager::GetInstance().LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/HDR/brdf_lut.hdr",HybridPBR::TextureType::HDR);

        // auto brdfLUT = HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/HDR/brdf_lut.hdr");
        // HybridPBR::ShaderManager::GetInstance().GetShader("sphere_pbr")->Use();
        // HybridPBR::ShaderManager::GetInstance().GetShader("sphere_pbr")->SetInt("brdfLUT",brdfLUT->GetID());

        for (int i = 0; i < materialParams.size(); ++i) {
            auto sphereMesh = std::make_shared<HybridPBR::Mesh>("Sphere_" + std::to_string(i));
            sphereMesh->GenerateSphere(1.0f, 64);
            
            HybridPBR::MaterialProperties props;
            props.albedo = glm::vec4(colors[i], 1.0f);
            props.metallic = materialParams[i].x;
            props.roughness = materialParams[i].y;
            props.ambientOcclusion = 1.0f;
            props.customShaderName = "sphere_pbr";
            
            auto pbrMaterial = std::make_shared<HybridPBR::PBRMaterial>("PBR_Sphere_" + std::to_string(i), props);
            // pbrMaterial->SetTexture(HybridPBR::TextureType::DIFFUSE,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_basecolor.png"));
            // pbrMaterial->SetTexture(HybridPBR::TextureType::NORMAL,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_normal.png"));
            // pbrMaterial->SetTexture(HybridPBR::TextureType::METALLIC,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_metallic.png"));
            // pbrMaterial->SetTexture(HybridPBR::TextureType::ROUGHNESS,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_roughness.png"));
            // pbrMaterial->SetTexture(HybridPBR::TextureType::AMBIENT_OCCLUSION,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_ambientocclusion.png"));

            
            int row = i / gridSize;
            int col = i % gridSize;
            
            float x = (col - gridSize / 2.0f + 0.5f) * spacing;
            float y = (row - gridSize / 2.0f + 0.5f) * spacing;
            
            auto sphereNode = scene->CreateNode("Sphere_" + std::to_string(i));
            sphereNode->SetMesh(sphereMesh);
            sphereNode->SetMaterial(pbrMaterial);
            sphereNode->GetTransform().SetPosition(glm::vec3(x, y, 0.0f));

        }
        
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

    void LoadModels2() { 
         // 尝试加载模型
        auto modelResult = HybridPBR::ModelLoader::LoadFromFile(HybridPBR::FileIO::GetAssetsPath()
         + "models/ornate_mirror_01_4k.gltf/ornate_mirror_01_4k.gltf");
        if (modelResult.success && !modelResult.meshes.empty()) {
            // 使用加载的模型
            auto mesh = modelResult.meshes[0];
            auto material = modelResult.materials.empty() ? 
                HybridPBR::ResourceManager::GetInstance().CreateMaterial("Default", HybridPBR::MaterialProperties{}) : 
                modelResult.materials[0];
            material->SetShaderType(HybridPBR::ShaderType::PBR);
            // 创建场景节点
            auto modelNode = scene->CreateNode("Mirror");
            modelNode->SetMesh(mesh);
            modelNode->SetMaterial(material);
            modelNode->GetTransform().SetPosition(glm::vec3(2.0f, 0.0f, 2.0f));
        } 
    }
    void CreateLights() { 
            glm::vec3 lightPositions[] = {
            glm::vec3(-10.0f,  10.0f, 10.0f),
            glm::vec3( 10.0f,  10.0f, 10.0f),
            glm::vec3(-10.0f, -10.0f, 10.0f),
            glm::vec3( 10.0f, -10.0f, 10.0f),
            };
        for (int i = 0; i < 4; ++i) { 
            auto light = std::make_shared<HybridPBR::Light>(HybridPBR::LightType::POINT, "Light" + std::to_string(i));
            light->SetPosition(lightPositions[i]);
            HybridPBR::LightProperties lightProps;
            lightProps.color = glm::vec3(1.0f, 1.0f, 1.0f);
            lightProps.intensity = 300.0f;
            light->SetProperties(lightProps);
            scene->AddLight(light);
        }
    }
    
    void CreateIBLlSystem() { 
        iblSystem = std::make_unique<HybridPBR::IBL>();
        if (!iblSystem->SetupFromHDR(HybridPBR::FileIO::GetAssetsPath() + "textures/HDR/ibl_hdr_radiance.png",512)) {
            return;
        }
        iblSystem->PrecomputeIrradianceMap(32);
        iblSystem->PrecomputePrefilterMap(128,5);
        iblSystem->GenerateBRDFLUT(512);

    }
    bool OnInitialize() override {
        //创建IBL系统
        CreateIBLlSystem();
        
        // 初始化渲染器
        rasterizer = std::make_unique<HybridPBR::Rasterizer>();
        if (!rasterizer->Initialize()) {
            return false;
        }
        auto& shaderManager = HybridPBR::ShaderManager::GetInstance();
        iblSystem->BindIBLTextures(shaderManager.GetShader(HybridPBR::ShaderType::PBR));

        //延迟渲染
        auto gbufferPass = std::make_shared<HybridPBR::GBufferPass>();
        rasterizer->AddRenderPass(std::move(gbufferPass), true);

        auto lightingPass = std::make_shared<HybridPBR::LightingPass>();
        rasterizer->AddRenderPass(std::move(lightingPass), true);

        //添加天空盒通道
        auto skyboxPass = std::make_shared<HybridPBR::SkyboxPass>();
        skyboxPass->SetSkyboxTexture(iblSystem->GetEnvironmentMap());
        rasterizer->AddRenderPass(skyboxPass);
        rasterizer->AddRenderPass(skyboxPass, true);
        
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
        LoadModels2();
        // 创建光源
        CreateLights();
        
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
        // if (auto modelNode = scene->FindNode("Model")) {
        //     auto rotation = modelNode->GetTransform().GetRotation();
        //     rotation.y += rotationSpeed * deltaTime;
        //     modelNode->GetTransform().SetRotation(rotation);
        // } else if (auto cubeNode = scene->FindNode("Cube")) {
        //     auto rotation = cubeNode->GetTransform().GetRotation();
        //     rotation.y += rotationSpeed * deltaTime;
        //     cubeNode->GetTransform().SetRotation(rotation);
        // }
        
        // 更新相机纵横比
        if (auto camera = scene->GetMainCamera()) {
            camera->SetViewport(GetWindow().GetWidth(), GetWindow().GetHeight());
        }
        
        scene->Update();
    }
    
    void OnRender() override {
        rasterizer->SetViewport(GetWindow().GetWidth(), GetWindow().GetHeight());
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

        // 处理gizmo交互
        componentManager->HandleGizmoInteraction(*camera, *scene, timer.GetDeltaTime());
        // 渲染gizmo
        componentManager->RenderGizmo(*camera, *scene);
        
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
    std::shared_ptr<HybridPBR::Camera> camera;
    float rotationSpeed;
    std::shared_ptr<HybridPBR::IBL> iblSystem;

};