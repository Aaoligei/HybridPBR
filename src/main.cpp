#include <glad/glad.h>
#include "core/Application.h"
#include "rendering/Shader.h"
#include <GLFW/glfw3.h>
#include "utils/FileIO.h"

class TestApp : public HybridPBR::Application {
public:
    bool OnInitialize() override {
        // 创建测试着色器
        shader = std::make_unique<HybridPBR::Shader>();
        if (!shader->LoadFromFile(HybridPBR::FileIO::GetAssetsPath()+"shaders/basic.vert", HybridPBR::FileIO::GetAssetsPath()+"shaders/basic.frag")) {
            return false;
        }
        
        // 设置三角形顶点数据
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,  // 左下
             0.5f, -0.5f, 0.0f,  // 右下
             0.0f,  0.5f, 0.0f   // 顶部
        };
        
        // 创建VAO, VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return true;
    }
    
    void OnUpdate(float deltaTime) override {
        // 简单的颜色动画
        time += deltaTime;
        color.r = (sin(time) + 1.0f) / 2.0f;
        color.g = (cos(time * 0.7f) + 1.0f) / 2.0f;
        color.b = (sin(time * 1.3f) + 1.0f) / 2.0f;
    }
    
    void OnRender() override {
        // 清除屏幕
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // 使用着色器
        shader->Use();
        shader->SetVec3("color", color);
        
        // 渲染三角形
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        // 检查错误
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            HybridPBR::LOG_ERROR("OpenGL error: " + std::to_string(error));
        }
    }
    
    void OnShutdown() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

private:
    std::unique_ptr<HybridPBR::Shader> shader;
    unsigned int VAO = 0, VBO = 0;
    float time = 0.0f;
    glm::vec3 color = glm::vec3(1.0f, 0.5f, 0.2f);
};

int main() {
    TestApp app;
    
    if (app.Initialize()) {
        app.Run();
    }
    
    app.Shutdown();
    return 0;
}