#include "RayTracer.h"
#include "Denoiser.h"
#include "utils/Logger.h"
#include <functional>
#include <chrono>

namespace HybridPBR {
    struct MeshInstance {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        glm::mat4 transform; // 新增：模型矩阵
    };

    RayTracer::RayTracer() {
        bvh = std::make_unique<BVH>();
        denoiser = std::make_unique<Denoiser>();
    }

    RayTracer::~RayTracer() {
        Shutdown();
    }

    bool RayTracer::Initialize(const RayTracerConfig& cfg) {
        if (initialized) {
            Shutdown();
        }
        
        config = cfg;
        
        LOG_INFO("Initializing RayTracer: " + std::to_string(config.width) + "x" + 
                std::to_string(config.height));
        
        if (!CreateShaders()) {
            LOG_ERROR("Failed to create ray tracing shaders");
            return false;
        }
        
        if (!CreateBuffers()) {
            LOG_ERROR("Failed to create compute buffers");
            return false;
        }
        
        if (!CreateTextures()) {
            LOG_ERROR("Failed to create output textures");
            return false;
        }
        
        if (!denoiser->Initialize(config.width, config.height)) {
            LOG_WARNING("Failed to initialize denoiser, continuing without denoising");
        }

        cameraUBO = std::make_unique<UniformBuffer>(sizeof(CameraData), 8);
        lightUBO = std::make_unique<UniformBuffer>(sizeof(LightData), 9);
        
        initialized = true;
        accumulatedFrames = 0;
        
        LOG_INFO("RayTracer initialized successfully");
        return true;
    }

    void RayTracer::Shutdown() {
        if (!initialized) return;
        
        Cleanup();
        denoiser->Shutdown();
        
        initialized = false;
        LOG_INFO("RayTracer shutdown");
    }

    void RayTracer::Render(const Scene& scene) {
        if (!initialized) return;
        // 清除缓冲区
        //glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto startTime = std::chrono::high_resolution_clock::now();
        
        UpdateGlobalUniforms(scene);

        // 更新场景数据（如果发生变化）
        if (!UpdateSceneData(scene)) {
            LOG_ERROR("Failed to update scene data for ray tracing");
            return;
        }
        
        // 生成主光线
        GenerateRays();
        
        // 路径追踪
        TracePaths();
        
        // 复制到纹理
        CopyToTexture();

        // 降噪
        if (config.denoiseEnabled) {
            DenoiseResult();
        }
        DrawOutputToScreen();

        accumulatedFrames++;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        lastRenderTime = std::chrono::duration<float>(endTime - startTime).count();
    }

    void RayTracer::Resize(uint32_t width, uint32_t height) {
        if (width == config.width && height == config.height) return;
        
        config.width = width;
        config.height = height;
        
        if (initialized) {
            CreateTextures();
            denoiser->Resize(width, height);
            ResetAccumulation();
        }
    }

    bool RayTracer::CreateShaders() {
        // 加载光线生成着色器
        rayGenerationShader = std::make_shared<ComputeShader>();
        if (!rayGenerationShader->LoadFromFile(FileIO::GetAssetsPath() +"shaders/compute/ray_generation.comp")) {
            LOG_ERROR("Failed to load ray generation shader");
            return false;
        }
        
        // 加载路径追踪着色器
        pathTracingShader = std::make_shared<ComputeShader>();
        if (!pathTracingShader->LoadFromFile(FileIO::GetAssetsPath() +"shaders/compute/path_tracing.comp")) {
            LOG_ERROR("Failed to load path tracing shader");
            return false;
        }
        
        // 加载降噪着色器
        denoiserShader = std::make_shared<ComputeShader>();
        if (!denoiserShader->LoadFromFile(FileIO::GetAssetsPath() +"shaders/compute/denoiser.comp")) {
            LOG_WARNING("Failed to load denoiser shader");
            // 降噪是可选的，不视为致命错误
        }
        
        return true;
    }

    bool RayTracer::CreateBuffers() {
        // 创建光线缓冲区
        uint32_t rayCount = config.width * config.height;
        if (!rayBuffer.Create<Ray>(rayCount)) { // 简化大小估计
            return false;
        }
        
        // 创建命中缓冲区
        if (!hitBuffer.Create<HitRecord>(rayCount)) {
            return false;
        }
        
        // 创建输出缓冲区
        if (!outputBuffer.Create<glm::vec4>(rayCount)) {
            return false;
        }
        
        // 创建累积缓冲区
        if (!accumulationBuffer.Create<glm::vec4>(rayCount)) {
            return false;
        }
        
        return true;
    }

    bool RayTracer::CreateTextures() {
        // 创建输出纹理
        outputTexture = std::make_shared<Texture>();
        if (!outputTexture->Create2D(config.width, config.height, GL_RGBA32F, GL_RGBA, GL_FLOAT)) {
            return false;
        }
        
        outputTexture->SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        outputTexture->SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        // 创建降噪纹理
        denoisedTexture = std::make_shared<Texture>();
        if (!denoisedTexture->Create2D(config.width, config.height, GL_RGBA32F, GL_RGBA, GL_FLOAT)) {
            return false;
        }
        
        denoisedTexture->SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        denoisedTexture->SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);

        if (blitFBO == 0) {
            glGenFramebuffers(1, &blitFBO);
        }
        
        return true;
    }

    bool RayTracer::UpdateSceneData(const Scene& scene) {
        // 构建/更新BVH
        if (!BuildBVH(scene)) {
            return false;
        }
        
        // 更新BVH节点缓冲区
        const auto& nodes = bvh->GetNodes();
        if (!bvhNodesBuffer.Create(nodes)) {
            return false;
        }
        
        // 更新三角形缓冲区
        const auto& triangles = bvh->GetTriangles();
        if (!trianglesBuffer.Create(triangles)) {
            return false;
        }
        
        // 更新材质缓冲区
        const auto& materials = bvh->GetGPUMaterials();
        if (!materialsBuffer.Create(materials)) {
            return false;
        }
        
        return true;
    }

    bool RayTracer::BuildBVH(const Scene& scene) {
        std::vector<MeshInstance> instances;
        
        // 从场景中提取网格和材质
        std::function<void(const SceneNode&)> extractNodeData = [&](const SceneNode& node) {
            if (auto mesh = node.GetMesh()) {
                MeshInstance instance;
                instance.mesh = mesh;
                if (auto material = node.GetMaterial()) {
                    instance.material= material;
                } else {
                    instance.material = std::make_shared<Material>("Default");
                }
                instance.transform = node.GetTransform().GetWorldMatrix();
                instances.push_back(instance);
            }
            
            for (const auto& child : node.GetChildren()) {
                extractNodeData(*child);
            }
        };
        
        extractNodeData(*scene.GetRoot());
        
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
        std::vector<glm::mat4> transforms;
        
        for(const auto& inst : instances) {
            meshes.push_back(inst.mesh);
            materials.push_back(inst.material);
            transforms.push_back(inst.transform);
        }

        return bvh->Build(meshes, materials, transforms);
    }

    void RayTracer::GenerateRays() {
        if (!rayGenerationShader) return;
        
        rayGenerationShader->Use();
        
        // 设置统一变量
        rayGenerationShader->SetUint("width", config.width);
        rayGenerationShader->SetUint("height", config.height);
        rayGenerationShader->SetUint("frameNumber", accumulatedFrames);
        
        // 绑定缓冲区
        rayBuffer.Bind(0);
        outputBuffer.Bind(1);
        
        // 分派计算着色器
        uint32_t groupsX = (config.width + 15) / 16;
        uint32_t groupsY = (config.height + 15) / 16;
        rayGenerationShader->Dispatch(groupsX, groupsY, 1);
        
        ComputeShader::MemoryBarrier();
    }

    void RayTracer::TracePaths() {
        if (!pathTracingShader) return;
        
        pathTracingShader->Use();
        
        // 设置统一变量
        pathTracingShader->SetUint("width", config.width);
        pathTracingShader->SetUint("height", config.height);
        pathTracingShader->SetUint("maxBounces", config.maxBounces);
        pathTracingShader->SetUint("samplesPerPixel", config.samplesPerPixel);
        pathTracingShader->SetUint("frameNumber", accumulatedFrames);
        
        // 绑定缓冲区
        rayBuffer.Bind(0);
        hitBuffer.Bind(1);
        outputBuffer.Bind(2);
        accumulationBuffer.Bind(3);
        bvhNodesBuffer.Bind(4);
        trianglesBuffer.Bind(5);
        materialsBuffer.Bind(6);
        
        // 分派计算着色器
        uint32_t groupsX = (config.width + 7) / 8;
        uint32_t groupsY = (config.height + 7) / 8;
        pathTracingShader->Dispatch(groupsX, groupsY, 1);
        
        ComputeShader::MemoryBarrier();
    }

    void RayTracer::DenoiseResult() {
        if (!denoiserShader || !denoiser) return;
        
        // 使用降噪器处理输出
        denoiser->Denoise(outputTexture, denoisedTexture, config.denoiseStrength);
    }

    void RayTracer::CopyToTexture() {
        // 将输出缓冲区复制到纹理
        glBindTexture(GL_TEXTURE_2D, outputTexture->GetID());
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, outputBuffer.GetID());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, config.width, config.height, 
                       GL_RGBA, GL_FLOAT, nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void RayTracer::Cleanup() {
        bvhNodesBuffer.Destroy();
        trianglesBuffer.Destroy();
        materialsBuffer.Destroy();
        rayBuffer.Destroy();
        hitBuffer.Destroy();
        outputBuffer.Destroy();
        accumulationBuffer.Destroy();
        if (blitFBO != 0) {
        glDeleteFramebuffers(1, &blitFBO);
        blitFBO = 0;
    }
    }
    void RayTracer::DrawOutputToScreen() {
        if (!initialized || blitFBO == 0) return;

        // 1. 确定要显示的纹理
        // 如果开启了降噪且降噪纹理存在，就显示降噪纹理，否则显示原始输出
        std::shared_ptr<Texture> textureToShow = outputTexture;
        if (config.denoiseEnabled && denoisedTexture) {
            textureToShow = denoisedTexture;
        }

        if (!textureToShow) return;

        // 保存当前的视口和FBO状态（可选，视你的引擎架构而定，为了安全起见建议保存）
        GLint lastReadFBO, lastDrawFBO;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &lastReadFBO);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastDrawFBO);

        // 2. 准备读取源 (Read Framebuffer)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, blitFBO);
        // 将纹理附加到 FBO 的颜色附件0
        // 注意：glFramebufferTexture2D 开销很小，每帧调用没问题
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                            GL_TEXTURE_2D, textureToShow->GetID(), 0);

        // 3. 准备绘制目标 (Draw Framebuffer) -> 屏幕 (ID 0)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        // 4. 执行 Blit (拷贝)
        // 参数：srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter
        glBlitFramebuffer(0, 0, config.width, config.height,  // 源矩形
                        0, 0, config.width, config.height,  // 目标矩形 (假设铺满窗口)
                        GL_COLOR_BUFFER_BIT,                // 拷贝颜色缓冲
                        GL_NEAREST);                        // 过滤方式 (点对点拷贝用 NEAREST 即可)

        // 5. 恢复状态
        glBindFramebuffer(GL_READ_FRAMEBUFFER, lastReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastDrawFBO);
    }

    void RayTracer::UpdateGlobalUniforms(const Scene& scene) {
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
        const auto& lights = scene.GetLights();
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

} // namespace HybridPBR