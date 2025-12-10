#include "GeometryPass.h"
#include "utils/Logger.h"
#include "utils/FileIO.h" // 假设有这个来获取路径

namespace HybridPBR {

    void GeometryPass::Initialize(RHI_Device* device) {
        m_device = device;
        
        // 1. 加载 Shader
        // 这里假设路径是硬编码的，实际应从 Config 读取
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

        BufferDesc uboDesc;
        uboDesc.name = "GlobalCameraUBO";
        uboDesc.size = sizeof(CameraBlock);
        uboDesc.usage = (uint32_t)BufferUsageBits::UniformBuffer;
        uboDesc.isDynamic = true; // 标记为动态，因为每帧更新
        
        m_globalUBO = m_device->CreateBuffer(uboDesc, nullptr);

        BufferDesc lightDesc;
        lightDesc.name = "GlobalLightUBO";
        lightDesc.size = sizeof(LightBlock);
        lightDesc.usage = (uint32_t)BufferUsageBits::UniformBuffer;
        lightDesc.isDynamic = true;
        
        m_lightUBO = m_device->CreateBuffer(lightDesc, nullptr);
    }

    void GeometryPass::Execute(const RenderContext& context) {
        if (!m_defaultPipeline.IsValid()) return;

        auto cmd = context.cmdList;
        
        // 1. 设置视口 (全屏)
        // 暂时硬编码，应该从 RenderTarget 获取
        Rect2D viewport{0, 0, 1920, 1080}; // TODO: 从 context 获取窗口大小
        cmd->SetViewport(viewport);
        cmd->SetScissor(viewport);

        cmd->Clear(true, true, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), 1.0f);
        // 2. 绑定管线 (Shader)
        cmd->SetPipelineState(m_defaultPipeline);

        // --- 新增：更新并绑定相机数据 ---
        UpdateGlobalState(context);
        
        // 绑定 UBO 到 slot 0 (对应 shader: binding = 0)
        cmd->BindUniformBuffer(0, m_globalUBO, 0, sizeof(CameraBlock)); 
        cmd->BindUniformBuffer(1, m_lightUBO, 0, sizeof(LightBlock));

        // 4. 遍历场景节点
        // 这是一个简单的递归 Lambda
        std::function<void(const SceneNode*)> renderNode = [&](const SceneNode* node) {
            if (!node) return;

            // 如果节点有 Mesh，绘制它
            if (auto mesh = node->GetMesh()) {
                GpuMesh* gpuMesh = GetOrCreateGpuMesh(context.device, mesh);
                
                if (gpuMesh) {
                    // A. 绑定顶点/索引缓冲
                    cmd->BindVertexBuffer(gpuMesh->GetVertexBuffer(), 0, 0);
                    if (gpuMesh->HasIndices()) {
                        cmd->BindIndexBuffer(gpuMesh->GetIndexBuffer(), 0);
                    }

                    // B. 推送 Model 矩阵 (Push Constants)
                    glm::mat4 modelMatrix = node->GetTransform().GetWorldMatrix();
                    cmd->BindPushConstants(m_defaultPipeline, 0, sizeof(glm::mat4), &modelMatrix);

                    // C. 发出绘制命令
                    if (gpuMesh->HasIndices()) {
                        cmd->DrawIndexed(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
                    } else {
                        cmd->Draw(gpuMesh->GetVertexCount(), 1, 0, 0);
                    }
                }
            }

            // 递归子节点
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
            // 销毁缓存的 GpuMeshes
            m_gpuMeshCache.clear(); // unique_ptr 会自动销毁 GpuMesh，GpuMesh 析构会调用 DestroyBuffer
            
            // 销毁 Pipeline 和 Shader
            m_device->DestroyPipeline(m_defaultPipeline);
            // m_device->DestroyShader(m_defaultShader); // 接口里还没加，先留空
        }
        if (m_globalUBO.IsValid()) m_device->DestroyBuffer(m_globalUBO);
        if (m_lightUBO.IsValid()) m_device->DestroyBuffer(m_lightUBO);
    }

    GpuMesh* GeometryPass::GetOrCreateGpuMesh(RHI_Device* device, const std::shared_ptr<Mesh>& mesh) {
        // 使用 mesh 的原始指针作为 key
        const void* key = mesh.get();
        
        auto it = m_gpuMeshCache.find(key);
        if (it != m_gpuMeshCache.end()) {
            return it->second.get();
        }

        // 创建新的 GpuMesh
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

        // 使用 RHI 更新 Buffer
        m_device->UpdateBuffer(m_globalUBO, &camData, sizeof(CameraBlock));
        
        LightBlock lightData = {};
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
            
            // ... 填充其他参数 (range, constant 等) ...
            lightData.lights[i].range = props.range;
            lightData.lights[i].constant = props.constant;
            lightData.lights[i].linear = props.linear;
            lightData.lights[i].quadratic = props.quadratic;
            lightData.lights[i].innerCutoff = props.innerCutoff;
            lightData.lights[i].outerCutoff = props.outerCutoff;
            
        }
        
        m_device->UpdateBuffer(m_lightUBO, &lightData, sizeof(LightBlock));
    }

} // namespace HybridPBR