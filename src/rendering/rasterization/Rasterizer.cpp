#include "Rasterizer.h"
#include  "core/Window.h"

namespace HybridPBR {

    // 静态成员初始化
    const Scene* Rasterizer::currentScene = nullptr;
    Rasterizer* Rasterizer::currentInstance = nullptr;

    Rasterizer::~Rasterizer() {
        Shutdown();
    }

    Result<void> Rasterizer::Initialize(std::shared_ptr<IRenderDevice> device) {
        LOG_INFO("Initializing Rasterizer with multi-pass architecture");
        
        // 初始化着色器管理器
        auto& shaderManager = ShaderManager::GetInstance();
        shaderManager.SetupPredefinedShaders();
        
        // 设置默认渲染通道
        SetupDefaultRenderPasses();
        
        // 设置OpenGL状态
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 初始化 UBO
        // Binding Point 0: Camera
        // Binding Point 1: Lights
        cameraUBO = std::make_unique<UniformBuffer>(sizeof(CameraData), 0);
        lightUBO = std::make_unique<UniformBuffer>(sizeof(LightData), 1);
        
        LOG_INFO("Rasterizer initialized successfully");
        initialized_ = true;
        currentInstance = this;
        return Result<void>::Success();
    }

    void Rasterizer::Shutdown() {
        initialized_ = false;
        currentInstance = nullptr;
        ClearRenderPasses();
        ShaderManager::GetInstance().ClearShaders();
        LOG_INFO("Rasterizer shutdown");
    }

    Result<void> Rasterizer::Render(const Scene& scene) {
        RETURN_IF_ERROR(BeginFrame());
        
        // 设置当前场景
        SetCurrentScene(scene);
        
        for (auto& pass : renderPass) {
            if (pass) {
                pass->Execute(scene);
            }
        }
        
        RETURN_IF_ERROR(EndFrame());
        return Result<void>::Success();
    }

    Result<void> Rasterizer::BeginFrame() {
        stats.Reset();
        
        // 清除缓冲区
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 在这里更新全局 UBO，这样每个 Pass 都不用管相机和光照了
        if (currentScene) {
            UpdateGlobalUniforms(*currentScene);
        }
        
        return Result<void>::Success();
    }

    Result<void> Rasterizer::EndFrame() {
        // 可以在这里添加帧结束处理
        return Result<void>::Success();
    }
    Result<void> Rasterizer::SetViewport(int width, int height) {
        glViewport(0, 0, width, height);
        return Result<void>::Success();
    }
    
    Result<void> Rasterizer::SetClearColor(const glm::vec4& color) {
        clearColor = color;
        return Result<void>::Success();
    }
    void Rasterizer::SetDepthTest(bool enabled) {
        depthTest = enabled;
        
    }

    void Rasterizer::AddRenderPass(std::shared_ptr<RenderPass> pass) {
        if (pass) {
            pass->Initialize();
            renderPass.push_back(pass);
            LOG_INFO("Added forward render pass: " + renderPass.back()->GetName());
        }

    }

    void Rasterizer::RemoveRenderPass(const std::string& passName) {
        for (auto it = renderPass.begin(); it != renderPass.end(); ++it) {
            if ((*it)->GetName() == passName) {
                (*it)->Cleanup();
                renderPass.erase(it);
                LOG_INFO("Removed render pass: " + passName);
                break;
            }
        }
    }

    void Rasterizer::ClearRenderPasses() {
        for (auto& pass : renderPass) {
            if (pass) {
                pass->Cleanup();
            }
        }
        renderPass.clear();
        LOG_INFO("Cleared all render passes");
    }

    void Rasterizer::SetupDefaultRenderPasses() {
        // 添加几何通道
        auto geometryPass = std::make_shared<GeometryPass>();
        geometryPass->SetWireframe(wireframe);
        geometryPass->SetBackfaceCulling(backfaceCulling);
        AddRenderPass(geometryPass);
        
        // 添加天空盒通道
        //auto skyboxPass = std::make_unique<SkyboxPass>();
        //skyboxPass->SetSkyboxTexture(skyboxTexture); // 需要设置天空盒纹理
        //AddRenderPass(std::move(skyboxPass));
        
        // 可以添加后处理通道
        // auto postProcessPass = std::make_unique<PostProcessPass>();
        // AddRenderPass(std::move(postProcessPass));
    }

    void Rasterizer::SetWireframe(bool enabled) {
        wireframe = enabled;
        // 更新几何通道的设置
        for (auto& pass : renderPass) {
            if (auto geometryPass = dynamic_cast<GeometryPass*>(pass.get())) {
                geometryPass->SetWireframe(enabled);
            }
        }
    }

    void Rasterizer::SetBackfaceCulling(bool enabled) {
        backfaceCulling = enabled;
        // 更新几何通道的设置
        for (auto& pass : renderPass) {
            if (auto geometryPass = dynamic_cast<GeometryPass*>(pass.get())) {
                geometryPass->SetBackfaceCulling(enabled);
            }
        }
    }

    std::vector<std::string> Rasterizer::GetRenderPassNames(){
            std::vector<std::string> names;
            for (auto& pass : renderPass) {
                names.push_back(pass->GetName());
            }
            return names;
        }
    
    void Rasterizer::UpdateGlobalUniforms(const Scene& scene) {
        auto camera = scene.GetMainCamera();
        if (camera) {
            CameraData camData;
            camData.view = camera->GetViewMatrix();
            camData.projection = camera->GetProjectionMatrix();
            camData.viewPos = camera->GetPosition();
            cameraUBO->SetData(&camData, sizeof(CameraData));
        }

        // 收集光源数据
        LightData lightData;
        const auto& lights = scene.GetAllLights();
        lightData.lightCount = std::min((int)lights.size(), 16);
        
        for(int i=0; i < lightData.lightCount; ++i) {
            auto& l = lights[i];
            auto& props = l->GetProperties();
            
            lightData.lights[i].position = l->GetPosition();
            lightData.lights[i].direction = l->GetDirection();
            lightData.lights[i].color = props.color;
            lightData.lights[i].intensity = props.intensity;

            lightData.lights[i].range = props.range;
            lightData.lights[i].constant = props.constant;
            lightData.lights[i].linear = props.linear;
            lightData.lights[i].quadratic = props.quadratic;

            lightData.lights[i].innerCutoff = props.innerCutoff;
            lightData.lights[i].outerCutoff = props.outerCutoff;
            lightData.lights[i].type = (int)l->GetType();
        }
        
        lightUBO->SetData(&lightData, sizeof(LightData));
    }
    
    Result<void> Rasterizer::Resize(uint32_t width, uint32_t height) {
        return SetViewport(static_cast<int>(width), static_cast<int>(height));
    }
    
    std::shared_ptr<IRenderPipelineManager> Rasterizer::GetPipelineManager() {
        // Rasterizer doesn't use pipeline manager in the traditional sense
        // Return nullptr as it uses render passes instead
        return nullptr;
    }
    
    Result<void> Rasterizer::AddPipeline(std::shared_ptr<IRenderPipeline> pipeline) {
        // Rasterizer uses render passes, not pipelines
        return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Rasterizer uses render passes, not pipelines"));
    }
    
    std::shared_ptr<Texture> Rasterizer::GetOutputTexture() const {
        // Rasterizer renders directly to the default framebuffer
        return nullptr;
    }
    
    std::shared_ptr<IRenderDevice> Rasterizer::GetDevice() const {
        // Rasterizer doesn't store a reference to the device
        return nullptr;
    }
    
    void Rasterizer::SetDebugMode(bool enabled) {
        debugMode_ = enabled;
    }
    
    bool Rasterizer::IsDebugMode() const {
        return debugMode_;
    }
} // namespace HybridPBR