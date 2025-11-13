#include "Camera.h"

namespace HybridPBR {

    Camera::Camera() {
        UpdateCameraVectors();
    }

    void Camera::SetPerspective(float fovDegrees, float aspectRatio, float near, float far) {
        mode = CameraMode::PERSPECTIVE;
        fov = fovDegrees;
        aspect = aspectRatio;
        nearPlane = near;
        farPlane = far;
        projectionDirty = true;
    }

    void Camera::SetOrthographic(float left, float right, float bottom, float top, float near, float far) {
        mode = CameraMode::ORTHOGRAPHIC;
        orthoLeft = left;
        orthoRight = right;
        orthoBottom = bottom;
        orthoTop = top;
        nearPlane = near;
        farPlane = far;
        projectionDirty = true;
    }

    void Camera::SetViewport(int width, int height) {
        if (width > 0 && height > 0) {
            aspect = static_cast<float>(width) / height;
            projectionDirty = true;
        }
    }

    void Camera::LookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& upVec) {
        position = eye;
        worldUp = upVec;
        
        viewMatrix = glm::lookAt(eye, target, upVec);
        viewDirty = false;
        
        // 从视图矩阵中提取前向向量
        front = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
        right = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        up = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
        
        // 计算欧拉角
        pitch = glm::degrees(asin(front.y));
        yaw = glm::degrees(atan2(front.z, front.x));
    }

    void Camera::SetPosition(const glm::vec3& newPosition) {
        position = newPosition;
        viewDirty = true;
    }

    void Camera::SetRotation(const glm::vec3& newRotation) {
        yaw = newRotation.y;
        pitch = newRotation.x;
        UpdateCameraVectors();
    }

    void Camera::Translate(const glm::vec3& translation) {
        position += translation;
        viewDirty = true;
    }

    void Camera::Rotate(float yawDelta, float pitchDelta) {
        yaw += yawDelta;
        pitch += pitchDelta;
        
        // 限制俯仰角
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        UpdateCameraVectors();
    }

    const glm::mat4& Camera::GetViewMatrix() const {
        if (viewDirty) {
            UpdateViewMatrix();
        }
        return viewMatrix;
    }

    const glm::mat4& Camera::GetProjectionMatrix() const {
        if (projectionDirty) {
            UpdateProjectionMatrix();
        }
        return projectionMatrix;
    }

    glm::mat4 Camera::GetViewProjectionMatrix() const {
        return GetProjectionMatrix() * GetViewMatrix();
    }

    void Camera::Update() {
        // 如果需要每帧更新，可以在这里添加逻辑
    }

    void Camera::UpdateViewMatrix() const {
        viewMatrix = glm::lookAt(position, position + front, up);
        viewDirty = false;
    }

    void Camera::UpdateProjectionMatrix() const {
        if (mode == CameraMode::PERSPECTIVE) {
            projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
        } else {
            projectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);
        }
        projectionDirty = false;
    }

    void Camera::UpdateCameraVectors() {
        // 计算新的前向向量
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
        
        // 重新计算右向量和上向量
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
        
        viewDirty = true;
    }

} // namespace HybridPBR