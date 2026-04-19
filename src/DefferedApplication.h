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
#include "rendering/deferred/DeferredRenderer.h"
#include "rendering/raytracing/RayTracer.h"
#include "rendering/rasterization/ShadowPass.h"


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
            // pbrMaterial->SetTexture(HybridPBR::TextureType::METALLIC,HybridPBR::ResourceManager::GetInstance().GetTexture(HybridPBR::FileIO::GetAssets.GetPath() +"textures/rustediron2_metallic.png"));
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
            modelNode->GetTransform().SetPosition(glm::vec3(0.0f, -1.0f, 2.0f));
        } 
    }
    void LoadCornellBox() {
         // 尝试加载模型
        auto modelResult = HybridPBR::ModelLoader::LoadFromFile(HybridPBR::FileIO::GetAssetsPath()
         + "models/CornellBox-Original/CornellBox-Original.obj");
        if (modelResult.success && !modelResult.meshes.empty()) {
            // 使用加载的所有网格和材质
            for (size_t i = 0; i < modelResult.meshes.size(); ++i) {
                auto mesh = modelResult.meshes[i];
                auto material = modelResult.materials.size() > i ? 
                    modelResult.materials[i] : 
                    HybridPBR::ResourceManager::GetInstance().CreateMaterial("Default", HybridPBR::MaterialProperties{});
                    
                material->SetCustomShader("sphere_pbr");
                
                // 创建场景节点
                auto modelNode = scene->CreateNode("cornellBox_" + std::to_string(i));
                modelNode->SetMesh(mesh);
                modelNode->SetMaterial(material);
                modelNode->GetTransform().SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
            }
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
        // 主方向光（模拟太阳）
        auto dirLight = std::make_shared<HybridPBR::Light>(HybridPBR::LightType::DIRECTIONAL, "DirLight");
        dirLight->SetDirection(glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f)));
        HybridPBR::LightProperties dirProps;
        dirProps.color = glm::vec3(1.0f, 0.95f, 0.85f); // slightly warm
        dirProps.intensity = 3.0f;
        dirLight->SetProperties(dirProps);
        scene->AddLight(dirLight);

        // 补光点光源（填充暗部）
        auto fillLight = std::make_shared<HybridPBR::Light>(HybridPBR::LightType::POINT, "FillLight");
        fillLight->SetPosition(glm::vec3(-3.0f, 5.0f, 6.0f));
        HybridPBR::LightProperties fillProps;
        fillProps.color = glm::vec3(0.7f, 0.8f, 1.0f); // cool fill
        fillProps.intensity = 80.0f;
        fillProps.constant = 1.0f;
        fillProps.linear = 0.35f;
        fillProps.quadratic = 0.44f;
        fillLight->SetProperties(fillProps);
        scene->AddLight(fillLight);
    }
    
    void CreateShadowCastingLight() {
        auto light = std::make_shared<HybridPBR::Light>(HybridPBR::LightType::DIRECTIONAL, "ShadowCaster");
        light->SetPosition(glm::vec3(-2.0f, 4.0f, -1.0f));
        light->SetDirection(glm::normalize(glm::vec3(2.0f, -4.0f, 1.0f)));
        HybridPBR::LightProperties lightProps;
        lightProps.color = glm::vec3(1.0f, 1.0f, 1.0f);
        lightProps.intensity = 10.0f;
        light->SetProperties(lightProps);
        scene->AddLight(light);
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
        HybridPBR::ShaderManager::GetInstance().LoadShader("sphere_pbr",HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.vert", HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.frag");
        iblSystem->BindIBLTextures(HybridPBR::ShaderManager::GetInstance().GetShader("sphere_pbr"));

        //创建延迟渲染器
        deferredRenderer = std::make_unique<HybridPBR::DeferredRenderer>();
        if (!deferredRenderer->Initialize(window->GetWidth(), window->GetHeight())){
             return false;
        }
        deferredRenderer->SetIBLSystem(iblSystem);
        deferredRenderer->SetSSAOEnabled(false);
        useDeferredRendering = true;

        //创建光线追踪渲染器
        HybridPBR::RayTracerConfig rtConfig;
            rtConfig.width = GetWindow().GetWidth();
            rtConfig.height = GetWindow().GetHeight();
            rtConfig.maxBounces = 4;
            rtConfig.samplesPerPixel = 1;
            rtConfig.denoiseEnabled = false;
        rayTracer = std::make_unique<HybridPBR::RayTracer>();
        if (!rayTracer->Initialize(rtConfig)){
            return false;
        }
        rayTracer->SetIBLSystem(iblSystem);
        useRayTracing = false;

        auto& shaderManager = HybridPBR::ShaderManager::GetInstance();
        iblSystem->BindIBLTextures(shaderManager.GetShader(HybridPBR::ShaderType::PBR));
        iblSystem->BindIBLTexturesRT(rayTracer->GetPathTracingShader());

        //创建阴影pass
        //shaderManager.LoadShader("shadow", HybridPBR::FileIO::GetAssetsPath() + "shaders/shadow.vert", HybridPBR::FileIO::GetAssetsPath() + "shaders/shadow.frag");
        //shadowPass = std::make_shared<HybridPBR::ShadowPass>(scene.get(), *shaderManager.GetShader("shadow"));
        //shadowPass->Initialize();
        //rasterizer->AddRenderPass(shadowPass);
        //deferredRenderer->SetShadowPass(shadowPass);

        //添加天空盒通道
        auto skyboxPass = std::make_shared<HybridPBR::SkyboxPass>();
        skyboxPass->SetSkyboxTexture(iblSystem->GetEnvironmentMap());
        rasterizer->AddRenderPass(skyboxPass);
        
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
        LoadCornellBox();
        // 创建光源
        CreateLights();
        //CreateShadowCastingLight();
        
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
        
        // 重置光线追踪累积（如果场景发生变化）
        static bool lastSceneDirty = true;
        bool currentSceneDirty = scene->IsDirty();
        
        if (currentSceneDirty != lastSceneDirty) {
            if (rayTracer && currentSceneDirty) {
                rayTracer->ResetAccumulation();
            }
            lastSceneDirty = currentSceneDirty;
            
            // 清除场景的脏标记
            if (currentSceneDirty) {
                scene->ClearDirtyFlag();
            }
        }
        
        // 检查相机是否移动
        static glm::vec3 lastCameraPos = scene->GetMainCamera()->GetPosition();
        static glm::mat4 lastCameraView = scene->GetMainCamera()->GetViewMatrix();

        glm::vec3 currentCameraPos = scene->GetMainCamera()->GetPosition();
        glm::mat4 currentCameraView = scene->GetMainCamera()->GetViewMatrix();
    
        if (currentCameraView != lastCameraView) {
            if (rayTracer) {
                rayTracer->ResetAccumulation();
            }
            lastCameraView = currentCameraView;
        }
    
        if (glm::distance(lastCameraPos, currentCameraPos) > 0.01f) {
            if (rayTracer) {
                rayTracer->ResetAccumulation();
            }
            lastCameraPos = currentCameraPos;
        }
    
        scene->Update();
    }
    
    void OnRender() override {
        
        if (useRayTracing && !useHybridRendering) {
            // 纯光线追踪模式
            rayTracer->Render(*scene);
            // 在这里可以显示光线追踪结果
            // 实际应用中需要将光线追踪纹理渲染到屏幕上
            rayTracer->DrawOutputToScreen();
            
        } else if (useHybridRendering) {
            // 混合渲染模式
            if (useDeferredRendering) {
                deferredRenderer->Render(*scene);
                // 可以在这里组合光线追踪结果
            } else {
                rasterizer->Render(*scene);
            }
            
            // 同时进行光线追踪（异步或同步）
            if (rayTracer) {
                rayTracer->Render(*scene);
            }
        } else {
            // 传统渲染模式
            if (useDeferredRendering) {
                deferredRenderer->Render(*scene);
            } else {
                rasterizer->SetViewport(window->GetWidth(), window->GetHeight());
                rasterizer->Render(*scene);
            }
        }
        
    }

    void OnImGuiRender() override {
        ImGui::SetWindowFontScale(1.5f);
        // 显示渲染统计
        auto stats = deferredRenderer->GetStats();
        ImGui::Begin("Renderer Stats");
        ImGui::Text("Renderer Stats");
        ImGui::Checkbox("Use ray tracing",&useRayTracing);
        ImGui::Checkbox("Use deferred rendering",&useDeferredRendering);

        if (useRayTracing && rayTracer) {
            auto config = rayTracer->GetConfig();
            bool changed = false;
            
            ImGui::Separator();
            ImGui::Text("Ray Tracing Config");
            if (ImGui::Checkbox("Denoise", &config.denoiseEnabled)) changed = true;
            if (config.denoiseEnabled) {
                if (ImGui::SliderFloat("Strength", &config.denoiseStrength, 0.0f, 2.0f)) changed = true;
            }
            
            int bounces = (int)config.maxBounces;
            if (ImGui::SliderInt("Max Bounces", &bounces, 1, 16)) {
                config.maxBounces = (uint32_t)bounces;
                changed = true;
            }

            if (changed) {
                rayTracer->SetConfig(config);
                rayTracer->ResetAccumulation();
            }
        }

        ImGui::Text("Draw calls: %d", stats.drawCalls);
        ImGui::Text("Triangles: %d", stats.triangleCount);
        ImGui::Text("Vertices: %d", stats.vertexCount);
        if(ImGui::CollapsingHeader("Render Passes")){
            for(auto passName : deferredRenderer->GetRenderPassNames()){
                ImGui::Text("%s", passName.c_str());
            }
        }
        ImGui::End();

        auto componentManager = imguiManager->GetComponentManager();

        // 处理gizmo交互
        // componentManager->HandleGizmoInteraction(*camera, *scene, timer.GetDeltaTime());
        // // 渲染gizmo
        // componentManager->RenderGizmo(*camera, *scene);
        
        componentManager->ShowSceneStats(scene);
        componentManager->ShowSceneHierarchy(scene);
        componentManager->ShowInspector(componentManager->GetSelectedNode());
        componentManager->ShowLightHierarchy(scene);

    }
    
    void OnShutdown() override {
        if (rayTracer) {
            rayTracer->Shutdown();
        }
        if (useDeferredRendering) {
            deferredRenderer->Shutdown();
        } else {
            rasterizer->Shutdown();
        }
    }

private:
    std::unique_ptr<HybridPBR::Rasterizer> rasterizer;
    std::unique_ptr<HybridPBR::DeferredRenderer> deferredRenderer;
    std::unique_ptr<HybridPBR::RayTracer> rayTracer;
    std::shared_ptr<HybridPBR::ShadowPass> shadowPass;

    std::shared_ptr<HybridPBR::IBL> iblSystem;
    std::shared_ptr<HybridPBR::Camera> camera;
    float rotationSpeed;

    bool useDeferredRendering = false;
    bool useRayTracing = false;
    bool useHybridRendering = false;


};