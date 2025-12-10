#include "HybridRenderer.h"
#include "../rhi/opengl/GL_Device.h" // 具体后端实现
#include "utils/Logger.h"

namespace HybridPBR {

    HybridRenderer::HybridRenderer() {}

    HybridRenderer::~HybridRenderer() {
        Shutdown();
    }

    Result<void> HybridRenderer::Initialize(std::shared_ptr<IRenderDevice> /*oldDevice*/) {
        LOG_INFO("Renderer", "Initializing Hybrid Renderer (RHI Architecture)");

        // 1. 创建 RHI 设备 (OpenGL 后端)
        // 未来这里可以根据配置切换 VulkanDevice
        m_rhiDevice = std::make_unique<OpenGLDevice>();
        
        if (!m_rhiDevice->Initialize()) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Failed to initialize RHI Device"));
        }

        // 2. 创建渲染通道
        m_geometryPass = std::make_unique<GeometryPass>();
        m_geometryPass->Initialize(m_rhiDevice.get());

        initialized_ = true;
        return Result<void>::Success();
    }

    void HybridRenderer::Shutdown() {
        if (m_geometryPass) {
            m_geometryPass->Cleanup();
            m_geometryPass.reset();
        }

        if (m_rhiDevice) {
            m_rhiDevice->Shutdown();
            m_rhiDevice.reset();
        }
        
        initialized_ = false;
    }

    Result<void> HybridRenderer::BeginFrame() {
        if (m_rhiDevice) {
            m_rhiDevice->BeginFrame();
        }
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::Render(const Scene& scene) {
        if (!initialized_ || !m_rhiDevice) return Result<void>::Failure(Error(ErrorType::Initialization, "Not initialized"));

        // 1. 获取每帧的命令列表 (Immediate Mode 下每帧获取一个新的或重置的)
        //BeginFrame();

        auto cmd = m_rhiDevice->GetImmediateCommandList();
        cmd->Begin();

        // 3. 构建渲染上下文
        RenderContext ctx;
        ctx.device = m_rhiDevice.get();
        ctx.cmdList = cmd;
        ctx.scene = &scene;
        
        // 4. 执行 Pass
        if (m_geometryPass) {
            m_geometryPass->Execute(ctx);
        }

        // 5. 结束命令录制
        cmd->End();

        //EndFrame();

        return Result<void>::Success();
    }

    Result<void> HybridRenderer::EndFrame() {
        if (m_rhiDevice) {
            m_rhiDevice->Present(); // SwapBuffers
        }
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::SetViewport(int width, int height) {
        m_width = width;
        m_height = height;
        // RHI 的 SetViewport 是在 CommandList 里做的，这里只需存下来供 Pass 使用
        // 实际上 RenderContext 应该包含 Viewport 信息，或者 Pass 自己去取
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::SetClearColor(const glm::vec4& color) {
        m_clearColor = color;
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::Resize(uint32_t width, uint32_t height) {
        return SetViewport(width, height);
    }

} // namespace HybridPBR