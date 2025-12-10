#include "GeometryPass.h"
#include "utils/Logger.h"
#include "utils/FileIO.h"
#include "../common/Material.h" // [新增] 需要包含 Material 头文件

namespace HybridPBR {

    void GeometryPass::Initialize(RHI_Device* device) {
        m_device = device;
        
        // 1. 加载 Shader
        std::string vertPath = FileIO::GetAssetsPath() + "shaders/default.vert";
        std::string fragPath = FileIO::GetAssetsPath() + "shaders/default.frag";
        
        m_defaultShader = device->CreateShader(vertPath, fragPath);
        
        if (m_defaultShader.IsValid()) {
            // 2. 创建 Pipeline
            m_defaultPipeline = device->CreateSimplePipeline(m_defaultShader);
            LOG_INFO("RenderPass", "GeometryPass initialized");
        } else {
            LOG_ERROR("RenderPass", "Failed to initialize GeometryPass shaders");
        }

        // 3. 创建全局 UBO (Camera)
        BufferDesc uboDesc;
        uboDesc.name = "GlobalCameraUBO";
        uboDesc.size = sizeof(CameraBlock);
        uboDesc.usage = (uint32_t)BufferUsageBits::UniformBuffer;
        uboDesc.isDynamic = true; 
        m_globalUBO = m_device->CreateBuffer(uboDesc, nullptr);

        // 4. 创建灯光 UBO (Light)
        BufferDesc lightDesc;
        lightDesc.name = "GlobalLightUBO";
        lightDesc.size = sizeof(LightBlock);
        lightDesc.usage = (uint32_t)BufferUsageBits::UniformBuffer;
        lightDesc.isDynamic = true;
        m_lightUBO = m_device->CreateBuffer(lightDesc, nullptr);

        // 5. [新增] 初始化默认材质 (Fallback)
        // 当模型没有材质时，使用这个纯白材质，避免 Bind 失败
        m_defaultMaterial = std::make_shared<Material>("Default_White");
        m_defaultMaterial->Initialize(m_device); 
        m_defaultMaterial->SetAlbedoColor({1.0f, 1.0f, 1.0f, 1.0f});
        m_defaultMaterial->SetMetallic(0.0f);
        m_defaultMaterial->SetRoughness(0.5f);
        m_defaultMaterial->UpdateToGPU(); // 确保数据上传
    }

    void GeometryPass::Execute(const RenderContext& context) {
        if (!m_defaultPipeline.IsValid()) return;

        auto cmd = context.cmdList;
        
        // 1. 设置视口
        Rect2D viewport{0, 0, 1600, 900}; // 暂时硬编码，建议从 context 获取
        if (context.device) { 
             // 如果能从 device 或 window 获取大小更好
        }
        cmd->SetViewport(viewport);
        cmd->SetScissor(viewport);

        cmd->Clear(true, true, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), 1.0f);
        
        // 2. 绑定管线
        cmd->SetPipelineState(m_defaultPipeline);

        // 3. 更新并绑定全局数据 (Camera slot=0, Light slot=1)
        UpdateGlobalState(context);
        cmd->BindUniformBuffer(0, m_globalUBO, 0, sizeof(CameraBlock)); 
        cmd->BindUniformBuffer(1, m_lightUBO, 0, sizeof(LightBlock));

        // 4. 遍历场景节点
        std::function<void(const SceneNode*)> renderNode = [&](const SceneNode* node) {
            if (!node) return;

            if (auto mesh = node->GetMesh()) {
                GpuMesh* gpuMesh = GetOrCreateGpuMesh(context.device, mesh);
                
                if (gpuMesh) {
                    // A. 绑定几何体
                    cmd->BindVertexBuffer(gpuMesh->GetVertexBuffer(), 0, 0);
                    if (gpuMesh->HasIndices()) {
                        cmd->BindIndexBuffer(gpuMesh->GetIndexBuffer(), 0);
                    }

                    // B. [关键修复] 绑定材质 (Slot 2)
                    auto material = node->GetMaterial();
                    
                    // 如果节点没有材质，使用默认材质
                    if (!material) {
                        material = m_defaultMaterial;
                    }

                    // 调用 Bind 将 UBO 绑定到 Slot 2
                    if (material) {
                        material->Bind(cmd.get());
                    }

                    // C. 推送 Model 矩阵
                    glm::mat4 modelMatrix = node->GetTransform().GetWorldMatrix();
                    cmd->BindPushConstants(m_defaultPipeline, 0, sizeof(glm::mat4), &modelMatrix);

                    // D. 绘制
                    if (gpuMesh->HasIndices()) {
                        cmd->DrawIndexed(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
                    } else {
                        cmd->Draw(gpuMesh->GetVertexCount(), 1, 0, 0);
                    }
                }
            }

            for (const auto& child : node->GetChildren()) {
                renderNode(child.get());
            }
        };

        if (context.scene && context.scene->GetRoot()) {
            renderNode(context.scene->GetRoot().get());
        }
    }

    void GeometryPass::Cleanup() {
        if (m_device) {
            m_gpuMeshCache.clear();
            m_device->DestroyPipeline(m_defaultPipeline);
            // shader 销毁逻辑...
        }
        if (m_globalUBO.IsValid()) m_device->DestroyBuffer(m_globalUBO);
        if (m_lightUBO.IsValid()) m_device->DestroyBuffer(m_lightUBO);
        
        // 释放默认材质
        m_defaultMaterial.reset();
    }

    GpuMesh* GeometryPass::GetOrCreateGpuMesh(RHI_Device* device, const std::shared_ptr<Mesh>& mesh) {
        const void* key = mesh.get();
        auto it = m_gpuMeshCache.find(key);
        if (it != m_gpuMeshCache.end()) {
            return it->second.get();
        }
        auto gpuMesh = std::make_unique<GpuMesh>(device, mesh);
        GpuMesh* ptr = gpuMesh.get();
        m_gpuMeshCache[key] = std::move(gpuMesh);
        return ptr;
    }

    void GeometryPass::UpdateGlobalState(const RenderContext& context) {
        if (!context.scene) return;
        
        auto camera = context.scene->GetMainCamera();
        if (!camera) return;

        CameraBlock camData;
        camData.view = camera->GetViewMatrix();
        camData.projection = camera->GetProjectionMatrix();
        camData.viewPos = camera->GetPosition();
        camData.padding = 0.0f;

        m_device->UpdateBuffer(m_globalUBO, &camData, sizeof(CameraBlock));
        
        LightBlock lightData = {};
        // 注意：这里使用的是 GetLights 还是 GetAllLights 取决于你的 Scene 类定义
        // 假设 context.scene->GetLights() 返回 std::vector<shared_ptr<Light>>
        const auto& sceneLights = context.scene->GetAllLights(); 
        
        lightData.lightCount = std::min((int)sceneLights.size(), 16);
        
        for (int i = 0; i < lightData.lightCount; ++i) {
            auto& srcLight = sceneLights[i];
            auto& props = srcLight->GetProperties();
            
            lightData.lights[i].position = srcLight->GetPosition();
            lightData.lights[i].direction = srcLight->GetDirection();
            lightData.lights[i].color = props.color;
            lightData.lights[i].intensity = props.intensity;
            lightData.lights[i].type = (int)srcLight->GetType();
            
            lightData.lights[i].range = props.range;
            lightData.lights[i].constant = 1.0f; // 这里的 constant 通常是 1.0
            lightData.lights[i].linear = 0.09f;  // 简化值
            lightData.lights[i].quadratic = 0.032f; // 简化值
            
        }
        
        m_device->UpdateBuffer(m_lightUBO, &lightData, sizeof(LightBlock));
    }

} // namespace HybridPBR