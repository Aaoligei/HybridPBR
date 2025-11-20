#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace HybridPBR {

    enum class CameraMode {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

    class Camera {
    public:
        Camera();
        
        // 投影设置
        void SetPerspective(float fov, float aspect, float nearPlane, float farPlane);
        void SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
        void SetViewport(int width, int height);
        
        // 视图变换
        void LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
        
        // 变换操作
        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec3& rotation);
        
        void Translate(const glm::vec3& translation);
        void Rotate(float yaw, float pitch);
        
        // 获取变换
        const glm::vec3& GetPosition() const { return position; }
        const glm::vec3& GetFront() const { return front; }
        const glm::vec3& GetRight() const { return right; }
        const glm::vec3& GetUp() const { return up; }
        
        // 矩阵获取
        const glm::mat4& GetViewMatrix() const;
        const glm::mat4& GetProjectionMatrix() const;
        glm::mat4 GetViewProjectionMatrix() const;
        
        // 相机参数
        float GetFOV() const { return fov; }
        float GetNearPlane() const { return nearPlane; }
        float GetFarPlane() const { return farPlane; }
        CameraMode GetMode() const { return mode; }
        
        // 更新
        void Update();

    private:
        CameraMode mode = CameraMode::PERSPECTIVE;
        
        // 透视参数
        float fov = 45.0f;
        float aspect = 16.0f / 9.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        
        // 正交参数
        float orthoLeft = -1.0f;
        float orthoRight = 1.0f;
        float orthoBottom = -1.0f;
        float orthoTop = 1.0f;
        
        // 视图参数
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
        glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // 欧拉角
        float yaw = -90.0f;
        float pitch = 0.0f;
        
        // 矩阵缓存
        mutable glm::mat4 viewMatrix = glm::mat4(1.0f);
        mutable glm::mat4 projectionMatrix = glm::mat4(1.0f);
        mutable bool viewDirty = true;
        mutable bool projectionDirty = true;
        
        void UpdateViewMatrix() const;
        void UpdateProjectionMatrix() const;
        void UpdateCameraVectors();
    };

}