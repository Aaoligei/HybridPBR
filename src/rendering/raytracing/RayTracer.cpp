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
        
        // if(!InitializeHybrid()){
        //     LOG_ERROR("Failed to initialize hybrid renderer");
        //     return false;
        // }
        
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
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        UpdateGlobalUniforms(scene);

        // 检查Scene是否为脏或我们自己的脏标记是否设置
        if(sceneDirty || scene.IsDirty()){
            LOG_INFO("Scene dirty, updating scene data");
            // 更新场景数据（如果发生变化）
            if (!UpdateSceneData(scene)) {
                LOG_ERROR("Failed to update scene data for ray tracing");
                return;
            }
            sceneDirty = false;
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

        accumulatedFrames++;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        lastRenderTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
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
            glCreateFramebuffers(1, &blitFBO);
        }
        
        return true;
    }

    bool RayTracer::UpdateSceneData(const Scene& scene) {
        // 构建/更新BVH
        if (!BuildBVH(scene)) {
            return false;
        }
            sceneTextures.clear();
    
        // [重要] 索引 0 留空，作为“无纹理”的默认值 (或者是纯白纹理)
        // 你可以创建一个 1x1 的纯白纹理放在 slot 0，防止 shader 访问空纹理报错
        auto whiteTexture = std::make_shared<Texture>();
        unsigned char whitePixel[] = {255, 255, 255, 255};
        whiteTexture->Create2D(1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        sceneTextures.push_back(whiteTexture);

        // 获取 BVH 生成的材质列表的引用（我们需要修改它，所以不用 const）
        // 注意：你需要修改 BVH 类，允许非 const 访问 gpuMaterials，或者在这里拷贝一份
        std::vector<GPUMaterial> materials = bvh->GetGPUMaterials(); 
        
        // 为了去重，可以用个 map
        std::unordered_map<uint32_t, uint32_t> textureIDToSlotIndex;
        
        // 辅助 lambda：处理单个纹理
        auto ProcessTexture = [&](uint32_t& matTexIndex, std::shared_ptr<Texture> tex) {
            if (tex) {
                uint32_t texID = tex->GetID();
                // 如果这个纹理已经加过了，直接复用索引
                if (textureIDToSlotIndex.find(texID) != textureIDToSlotIndex.end()) {
                    matTexIndex = textureIDToSlotIndex[texID];
                } else {
                    // 如果没加过，加到列表尾部
                    if (sceneTextures.size() < 32) { // 限制最大数量
                        uint32_t newIndex = static_cast<uint32_t>(sceneTextures.size());
                        sceneTextures.push_back(tex);
                        textureIDToSlotIndex[texID] = newIndex;
                        matTexIndex = newIndex;
                    } else {
                        LOG_WARNING("Texture limit (32) reached!");
                        matTexIndex = 0; // 超过限制就用白色
                    }
                }
            } else {
                matTexIndex = 0; // 无纹理
            }
        };

        // 重新遍历材质，修正索引
        // 注意：这里需要能访问到原始的 Scene Material 对象
        // 这意味着 BVH 构建时最好保留了 Scene Material 的指针列表
        // 假设 bvh->GetMaterials() 返回原始材质指针列表 (你需要去 BVH.h 加这个 getter)
        const auto& sourceMaterials = bvh->GetSourceMaterials(); // 需要你在 BVH 类里加这个
        
        LOG_INFO("Processing materials"+std::to_string(materials.size()));
        for (size_t i = 0; i < materials.size(); i++) {
            if (i >= sourceMaterials.size()) break;
            auto srcMat = sourceMaterials[i];
            LOG_INFO("Processing material "+srcMat->GetName());
            if (!srcMat) continue;
            
            // 处理 Albedo
            ProcessTexture(materials[i].albedoTexture, srcMat->GetTexture(TextureType::DIFFUSE));
            // 处理 Normal, Metallic 等同理...
            ProcessTexture(materials[i].normalTexture, srcMat->GetTexture(TextureType::NORMAL));
            ProcessTexture(materials[i].metallicTexture, srcMat->GetTexture(TextureType::METALLIC));
            ProcessTexture(materials[i].roughnessTexture, srcMat->GetTexture(TextureType::ROUGHNESS));
            ProcessTexture(materials[i].aoTexture, srcMat->GetTexture(TextureType::AMBIENT_OCCLUSION));
            ProcessTexture(materials[i].emissiveTexture, srcMat->GetTexture(TextureType::EMISSIVE));
        }

        // 上传修正后的材质数据到 GPU
        if (!materialsBuffer.Create(materials)) return false;

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

        // 绑定所有纹理
        LOG_INFO("Textures: "+std::to_string(sceneTextures.size()));
        for (int i = 0; i < sceneTextures.size(); ++i) {
            if (sceneTextures[i]) {
                // 绑定到纹理单元 i
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, sceneTextures[i]->GetID());
                
                // 告诉 shader，uniform 数组的第 i 个元素对应纹理单元 i
                std::string name = "textureMaps[" + std::to_string(i) + "]";
                pathTracingShader->SetInt(name, i);
            }
        }
        
        // 分派计算着色器
        uint32_t groupsX = (config.width + 32) / 31;
        uint32_t groupsY = (config.height + 32) / 31;
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
        // 使用 DSA 版本的函数替代 glFramebufferTexture2D
        glNamedFramebufferTexture(blitFBO, GL_COLOR_ATTACHMENT0, textureToShow->GetID(), 0);

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
            //光追需要获取逆矩阵
            camData.view =glm::inverse(camera->GetViewMatrix());
            camData.projection = glm::inverse(camera->GetProjectionMatrix());
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


    // 在 Initialize 或 InitializeHybrid 中加载 Shader 和创建 Texture
    bool RayTracer::InitializeHybrid() {
        // 1. 加载 Shader
        rtShadowShader = std::make_shared<ComputeShader>();
        if (!rtShadowShader->LoadFromFile(FileIO::GetAssetsPath() + "shaders/compute/rt_shadows.comp")) {
            LOG_ERROR("Failed to load RT Shadow shader");
            return false;
        }

        rtReflectionShader = std::make_shared<ComputeShader>();
        if (!rtReflectionShader->LoadFromFile(FileIO::GetAssetsPath() + "shaders/compute/rt_reflections.comp")) {
            LOG_ERROR("Failed to load RT Reflection shader");
            return false;
        }

        // 2. 创建结果纹理
        rtShadowTexture = std::make_shared<Texture>();
        rtShadowTexture->Create2D(config.width, config.height, GL_R8, GL_RED, GL_UNSIGNED_BYTE); // 单通道可见性
        rtShadowTexture->SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        rtShadowTexture->SetFilter(TextureFilter::NEAREST, TextureFilter::NEAREST);

        rtReflectionTexture = std::make_shared<Texture>();
        rtReflectionTexture->Create2D(config.width, config.height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
        rtReflectionTexture->SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        rtReflectionTexture->SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        return true;
    }

    void RayTracer::RenderShadows(std::shared_ptr<GBuffer> gbuffer, const Scene& scene) {
        if (!rtShadowShader || !gbuffer) return;

        rtShadowShader->Use();
        
        // 绑定 G-Buffer
        gbuffer->GetTexture(GBufferTextureType::Position)->Bind(0);
        rtShadowShader->SetInt("gPosition", 0);
        gbuffer->GetTexture(GBufferTextureType::Normal)->Bind(1);
        rtShadowShader->SetInt("gNormal", 1);
        
        // 绑定输出图像
        rtShadowTexture->BindImage(0, 0, GL_WRITE_ONLY);
        
        // 设置 Uniforms
        rtShadowShader->SetInt("width", config.width);
        rtShadowShader->SetInt("height", config.height);
        
        // 获取主光源位置 (假设第一个光源)
        const auto& lights = scene.GetLights();
        if (!lights.empty()) {
            rtShadowShader->SetVec3("lightPos", lights[0]->GetPosition());
        } else {
            rtShadowShader->SetVec3("lightPos", glm::vec3(0, 10, 0));
        }

        // 绑定 BVH 数据
        bvhNodesBuffer.Bind(4);
        trianglesBuffer.Bind(5);
        
        // Dispatch
        uint32_t groupsX = (config.width + 7) / 8;
        uint32_t groupsY = (config.height + 7) / 8;
        rtShadowShader->Dispatch(groupsX, groupsY, 1);
        
        ComputeShader::MemoryBarrier();
    }

    void RayTracer::RenderReflections(std::shared_ptr<GBuffer> gbuffer, const Scene& scene) {
        if (!rtReflectionShader || !gbuffer) return;

        rtReflectionShader->Use();

        // 绑定 G-Buffer
        gbuffer->GetTexture(GBufferTextureType::Position)->Bind(0);
        rtReflectionShader->SetInt("gPosition", 0);
        gbuffer->GetTexture(GBufferTextureType::Normal)->Bind(1);
        rtReflectionShader->SetInt("gNormal", 1);
        gbuffer->GetTexture(GBufferTextureType::MetallicRoughnessAO)->Bind(2);
        rtReflectionShader->SetInt("gMRA", 2);

        // 绑定输出图像
        rtReflectionTexture->BindImage(0, 0, GL_WRITE_ONLY);

        // 设置 Uniforms
        rtReflectionShader->SetInt("width", config.width);
        rtReflectionShader->SetInt("height", config.height);
        if (scene.GetMainCamera()) {
            rtReflectionShader->SetVec3("viewPos", scene.GetMainCamera()->GetPosition());
        }

        // 绑定 BVH 和 材质 数据
        bvhNodesBuffer.Bind(4);
        trianglesBuffer.Bind(5);
        materialsBuffer.Bind(6);
        
        // 绑定纹理
        // (逻辑同 TracePaths)
        for (int i = 0; i < sceneTextures.size(); ++i) {
            if (sceneTextures[i]) {
                glActiveTexture(GL_TEXTURE0 + 10 + i); // Offset to avoid conflict
                glBindTexture(GL_TEXTURE_2D, sceneTextures[i]->GetID());
                std::string name = "textureMaps[" + std::to_string(i) + "]";
                rtReflectionShader->SetInt(name, 10 + i);
            }
        }

        // Dispatch
        uint32_t groupsX = (config.width + 7) / 8;
        uint32_t groupsY = (config.height + 7) / 8;
        rtReflectionShader->Dispatch(groupsX, groupsY, 1);

        ComputeShader::MemoryBarrier();
    }
} // namespace HybridPBR