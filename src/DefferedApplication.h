#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h> 
#include "core/Application.h"
#include "core/Result.h"
#include "rendering/Shader.h"
#include "rendering/ShaderManager.h"
#include "utils/FileIO.h"
#include "rendering/rasterization/Rasterizer.h"
#include "rendering/rasterization/RenderPass.h"
#include "rendering/HybridRenderer.h"
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

class ThreeDApp : public HybridPBR::Application {
public:
    void OnWindowConfigChanged() override {
        auto config = GetConfig();
        config.window.width = 1920;
        config.window.height = 1080;
        config.window.title = "HybridPBR 3D Application";
        config.window.vsync = false;
        config.window.fullscreen = false;
        SetConfig(config);
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
            
        // HybridPBR::ServiceLocator::Resolve<HybridPBR::IResourceManager>()->LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_basecolor.png",HybridPBR::TextureType::DIFFUSE);
        // HybridPBR::ServiceLocator::Resolve<HybridPBR::IResourceManager>()->LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_normal.png",HybridPBR::TextureType::NORMAL);
        // HybridPBR::ServiceLocator::Resolve<HybridPBR::IResourceManager>()->LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_metallic.png",HybridPBR::TextureType::METALLIC);
        // HybridPBR::ServiceLocator::Resolve<HybridPBR::IResourceManager>()->LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_roughness.png",HybridPBR::TextureType::ROUGHNESS);
        // HybridPBR::ServiceLocator::Resolve<HybridPBR::IResourceManager>()->LoadTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/rustediron2_ambientocclusion.png",HybridPBR::TextureType::AMBIENT_OCCLUSION);

        HybridPBR::ShaderManager::GetInstance().LoadShader("sphere_pbr",HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.vert", HybridPBR::FileIO::GetAssetsPath() +"shaders/basic.frag");
        iblSystem->BindIBLTextures(HybridPBR::ShaderManager::GetInstance().GetShader("sphere_pbr"));
        
        // Load BRDF LUT texture using ServiceLocator
        auto resourceManager = GetService<HybridPBR::IResourceManager>();
        if (resourceManager) {
            resourceManager->Load(HybridPBR::FileIO::GetAssetsPath() +"textures/HDR/brdf_lut.hdr");
        }

        // auto brdfLUT = HybridPBR::ServiceLocator::Resolve<HybridPBR::IResourceManager>()->GetTexture(HybridPBR::FileIO::GetAssetsPath() +"textures/HDR/brdf_lut.hdr");
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
            
            auto sphereNodeResult = scene->CreateNode("Sphere_" + std::to_string(i));
            if (sphereNodeResult.IsSuccess()) {
                auto sphereNode = sphereNodeResult.GetValue();
                sphereNode->SetMesh(sphereMesh);
                sphereNode->SetMaterial(pbrMaterial);
                sphereNode->GetTransform().SetPosition(glm::vec3(x, y, 0.0f));
            }

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
                nullptr : 
                modelResult.materials[0];
            if (material) {
                material->SetShaderType(HybridPBR::ShaderType::PBR);
            }
            // 创建场景节点
            auto modelNodeResult = scene->CreateNode("Model");
            if (modelNodeResult.IsSuccess()) {
                auto modelNode = modelNodeResult.GetValue();
                modelNode->SetMesh(mesh);
                modelNode->SetMaterial(material);
                modelNode->GetTransform().SetPosition(glm::vec3(0.0f, -1.0f, 2.0f));
            }
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
                    nullptr;
                    
                material->SetCustomShader("sphere_pbr");
                
                // 创建场景节点
                auto modelNodeResult = scene->CreateNode("cornellBox_" + std::to_string(i));
                if (modelNodeResult.IsSuccess()) {
                    auto modelNode = modelNodeResult.GetValue();
                    modelNode->SetMesh(mesh);
                    modelNode->SetMaterial(material);
                    modelNode->GetTransform().SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
                }
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
                nullptr : 
                modelResult.materials[0];
            if (material) {
                material->SetShaderType(HybridPBR::ShaderType::PBR);
            }
            // 创建场景节点
            auto modelNodeResult = scene->CreateNode("Mirror");
            if (modelNodeResult.IsSuccess()) {
                auto modelNode = modelNodeResult.GetValue();
                modelNode->SetMesh(mesh);
                modelNode->SetMaterial(material);
                modelNode->GetTransform().SetPosition(glm::vec3(2.0f, 0.0f, 2.0f));
            }
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
        // 使用正确的HDR格式文件
        if (!iblSystem->SetupFromHDR(HybridPBR::FileIO::GetAssetsPath() + "textures/HDR/ibl_hdr_radiance.png",512)) {
            LOG_ERROR("Failed to setup IBL from HDR file");
            return;
        }
        if (!iblSystem->PrecomputeIrradianceMap(32)) {
            LOG_ERROR("Failed to precompute irradiance map");
        }
        if (!iblSystem->PrecomputePrefilterMap(128,5)) {
            LOG_ERROR("Failed to precompute prefilter map");
        }
        if (!iblSystem->GenerateBRDFLUT(512)) {
            LOG_ERROR("Failed to generate BRDF LUT");
        }
        LOG_INFO("IBL system initialized successfully");

    }
    HybridPBR::Result<void> OnInitialize() override {
        // 创建IBL系统
        CreateIBLlSystem();
        
        // 初始化混合渲染器
        hybridRenderer = std::make_unique<HybridPBR::HybridRenderer>();
        auto renderDevice = GetService<HybridPBR::IRenderDevice>();
        if (!renderDevice) {
            return HybridPBR::Result<void>::Failure(
                HybridPBR::Error(HybridPBR::ErrorType::DeviceLost, "Render device not available"));
        }
        
        auto initResult = hybridRenderer->Initialize(renderDevice);
        if (!initResult.IsSuccess()) {
            return initResult;
        }

        // 初始化传统渲染器（向后兼容）
        rasterizer = std::make_unique<HybridPBR::Rasterizer>();
        auto rasterizerInitResult = rasterizer->Initialize(renderDevice);
        if (!rasterizerInitResult.IsSuccess()) {
            return rasterizerInitResult;
        }

        // 加载着色器
        auto& shaderManager = HybridPBR::ShaderManager::GetInstance();
        shaderManager.LoadShader("sphere_pbr", HybridPBR::FileIO::GetAssetsPath() + "shaders/basic.vert", 
                                HybridPBR::FileIO::GetAssetsPath() + "shaders/basic.frag");
        
        if (auto shader = shaderManager.GetShader("sphere_pbr")) {
            iblSystem->BindIBLTextures(shader);
        }

        // 创建延迟渲染器
        deferredRenderer = std::make_unique<HybridPBR::DeferredRenderer>();
        if (!deferredRenderer->Initialize(GetWindow().GetWidth(), GetWindow().GetHeight())) {
            return HybridPBR::Result<void>::Failure(
                HybridPBR::Error(HybridPBR::ErrorType::Initialization, "Failed to initialize deferred renderer"));
        }
        deferredRenderer->SetIBLSystem(iblSystem);
        deferredRenderer->SetSSAOEnabled(false);
        useDeferredRendering = false;

        // 创建光线追踪渲染器
        HybridPBR::RayTracerConfig rtConfig;
        rtConfig.width = GetWindow().GetWidth();
        rtConfig.height = GetWindow().GetHeight();
        rtConfig.maxBounces = 4;
        rtConfig.samplesPerPixel = 1;
        rtConfig.denoiseEnabled = false;
        
        rayTracer = std::make_unique<HybridPBR::RayTracer>();
        if (!rayTracer->Initialize(rtConfig)) {
            return HybridPBR::Result<void>::Failure(
                HybridPBR::Error(HybridPBR::ErrorType::Initialization, "Failed to initialize ray tracer"));
        }
        rayTracer->SetIBLSystem(iblSystem);
        useRayTracing = false;

        iblSystem->BindIBLTextures(shaderManager.GetShader(HybridPBR::ShaderType::PBR));
        iblSystem->BindIBLTexturesRT(rayTracer->GetPathTracingShader());

        // 添加天空盒通道 - 确保在最后添加
        auto skyboxPass = std::make_shared<HybridPBR::SkyboxPass>();
        auto envMap = iblSystem->GetEnvironmentMap();
        if (envMap) {
            skyboxPass->SetSkyboxTexture(envMap);
            rasterizer->AddRenderPass(skyboxPass);
            LOG_INFO("Added skybox pass with environment map ID: " + std::to_string(envMap->GetID()));
        } else {
            LOG_ERROR("Failed to get environment map from IBL system");
        }
        
        // 创建场景
        scene = std::make_unique<HybridPBR::Scene>();
        if (!scene->Initialize().IsSuccess()) {
            return HybridPBR::Result<void>::Failure(
                HybridPBR::Error(HybridPBR::ErrorType::Initialization, "Failed to initialize scene"));
        }
        
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
        LoadCornellBox();
        
        // 创建光源
        CreateLights();
        
        rotationSpeed = 45.0f; // 度/秒
        
        LOG_INFO("Test application initialized with scene");
        return HybridPBR::Result<void>::Success();
    }
    
    HybridPBR::Result<void> OnUpdate(float deltaTime) override {
        // 更新相机控制器
        if (cameraController) {
            cameraController->Update(deltaTime);
        }
        
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
        static glm::vec3 lastCameraPos = glm::vec3(0.0f);
        static glm::mat4 lastCameraView = glm::mat4(1.0f);
        
        if (auto camera = scene->GetMainCamera()) {
            glm::vec3 currentCameraPos = camera->GetPosition();
            glm::mat4 currentCameraView = camera->GetViewMatrix();
        
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
        }
    
        scene->Update(deltaTime);
        return HybridPBR::Result<void>::Success();
    }
    
    HybridPBR::Result<void> OnRender() override {
        if (useRayTracing && !useHybridRendering) {
            // 纯光线追踪模式
            rayTracer->Render(*scene);
            // 在这里可以显示光线追踪结果
            // 实际应用中需要将光线追踪纹理渲染到屏幕上
            rayTracer->DrawOutputToScreen(GetWindow().GetWidth(), GetWindow().GetHeight());
            
        } else if (useHybridRendering) {
            // 混合渲染模式
            if (useDeferredRendering) {
                deferredRenderer->Render(*scene);
                // 可以在这里组合光线追踪结果
            } else {
                auto renderResult = rasterizer->Render(*scene);
                if (!renderResult.IsSuccess()) {
                    return renderResult;
                }
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
                rasterizer->SetViewport(GetWindow().GetWidth(), GetWindow().GetHeight());
                auto renderResult = rasterizer->Render(*scene);
                if (!renderResult.IsSuccess()) {
                    return renderResult;
                }
            }
        }
        
        return HybridPBR::Result<void>::Success();
    }

    HybridPBR::Result<void> OnImGuiRender() override {
        ImGui::SetWindowFontScale(1.5f);
        
        // 显示渲染统计
        auto stats = rasterizer->GetStats();
        ImGui::Begin("Renderer Stats");
        ImGui::Text("Renderer Stats");
        ImGui::Checkbox("Use ray tracing", &useRayTracing);
        ImGui::Checkbox("Use deferred rendering", &useDeferredRendering);
        ImGui::Checkbox("Use hybrid rendering", &useHybridRendering);
        ImGui::Text("Draw calls: %d", stats.drawCalls);
        ImGui::Text("Triangles: %d", stats.triangleCount);
        ImGui::Text("Vertices: %d", stats.vertexCount);
        if(ImGui::CollapsingHeader("Render Passes")){
            for(auto passName : rasterizer->GetRenderPassNames()){
                ImGui::Text("%s", passName.c_str());
            }
        }
        ImGui::End();

        auto imguiManager = GetService<HybridPBR::ImGuiManager>();
        if (imguiManager) {
            auto componentManager = imguiManager->GetComponentManager();
            if (componentManager) {
                componentManager->ShowSceneStats(scene);
                componentManager->ShowSceneHierarchy(scene);
                componentManager->ShowInspector(componentManager->GetSelectedNode());
                componentManager->ShowLightHierarchy(scene);
            }
        }
        
        return HybridPBR::Result<void>::Success();
    }
    
    void OnMouseMoved(double x, double y) override {
        if (cameraController) {
            cameraController->OnMouseMove(x, y);
        }
    }
    
    void OnMouseClicked(int button) override {
        if (cameraController) {
            cameraController->OnMouseButton(button, GLFW_PRESS, 0);
        }
    }
    
    void OnMouseReleased(int button) override {
        if (cameraController) {
            cameraController->OnMouseButton(button, GLFW_RELEASE, 0);
        }
    }
    
    void OnMouseScroll(double xoffset, double yoffset) override {
        if (cameraController) {
            cameraController->OnMouseScroll(xoffset, yoffset);
        }
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
    // 渲染器
    std::unique_ptr<HybridPBR::HybridRenderer> hybridRenderer;
    std::unique_ptr<HybridPBR::Rasterizer> rasterizer;
    std::unique_ptr<HybridPBR::DeferredRenderer> deferredRenderer;
    std::unique_ptr<HybridPBR::RayTracer> rayTracer;

    // 场景和相机
    std::unique_ptr<HybridPBR::Scene> scene;
    std::shared_ptr<HybridPBR::IBL> iblSystem;
    std::shared_ptr<HybridPBR::Camera> camera;
    std::unique_ptr<HybridPBR::CameraController> cameraController;
    
    // 配置参数
    float rotationSpeed;
    bool useDeferredRendering = false;
    bool useRayTracing = false;
    bool useHybridRendering = false;

};