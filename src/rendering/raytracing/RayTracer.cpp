#include "RayTracer.h"
#include "Denoiser.h"
#include "utils/Logger.h"
#include <functional>
#include <chrono>

namespace HybridPBR {

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
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 更新场景数据（如果发生变化）
        if (!UpdateSceneData(scene)) {
            LOG_ERROR("Failed to update scene data for ray tracing");
            return;
        }
        
        // 生成主光线
        GenerateRays();
        
        // 路径追踪
        TracePaths();
        
        // 降噪
        if (config.denoiseEnabled) {
            DenoiseResult();
        }
        
        // 复制到纹理
        CopyToTexture();
        
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
        if (!rayBuffer.Create<std::uint32_t>(rayCount * 8)) { // 简化大小估计
            return false;
        }
        
        // 创建命中缓冲区
        if (!hitBuffer.Create<std::uint32_t>(rayCount * 4)) {
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
        // 收集所有网格
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
        
        // 从场景中提取网格和材质
        std::function<void(const SceneNode&)> extractNodeData = [&](const SceneNode& node) {
            if (auto mesh = node.GetMesh()) {
                meshes.push_back(mesh);
                if (auto material = node.GetMaterial()) {
                    materials.push_back(material);
                } else {
                    // 添加默认材质
                    materials.push_back(std::make_shared<Material>("Default"));
                }
            }
            
            for (const auto& child : node.GetChildren()) {
                extractNodeData(*child);
            }
        };
        
        extractNodeData(*scene.GetRoot());
        
        // 构建BVH
        return bvh->Build(meshes, materials);
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
    }

} // namespace HybridPBR