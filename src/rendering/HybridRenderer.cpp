#include "HybridRenderer.h"
#include "../rhi/opengl/GL_Device.h"
#include "utils/Logger.h"

namespace HybridPBR {

    HybridRenderer::HybridRenderer() {}

    HybridRenderer::~HybridRenderer() {
        Shutdown();
    }

    // [修改] 移除参数，内部创建 OpenGLDevice
    Result<void> HybridRenderer::Initialize() {
        LOG_INFO("Renderer", "Initializing Hybrid Renderer (RHI Architecture)");

        // 1. 创建 RHI 设备
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
        if (!initialized_ || !m_rhiDevice) 
            return Result<void>::Failure(Error(ErrorType::Initialization, "Not initialized"));

        auto cmd = m_rhiDevice->GetImmediateCommandList();
        cmd->Begin();

        // 设置视口
        Rect2D viewport{0, 0, m_width, m_height};
        cmd->SetViewport(viewport);
        cmd->SetScissor(viewport);
        cmd->Clear(true, true, m_clearColor, 1.0f);

        // 构建上下文并执行 Pass
        RenderContext ctx;
        ctx.device = m_rhiDevice.get();
        ctx.cmdList = cmd;
        ctx.scene = &scene;
        
        if (m_geometryPass) {
            m_geometryPass->Execute(ctx);
        }

        cmd->End();
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::EndFrame() {
        if (m_rhiDevice) {
            m_rhiDevice->Present();
        }
        return Result<void>::Success();
    }

    Result<void> HybridRenderer::SetViewport(int width, int height) {
        m_width = width;
        m_height = height;
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