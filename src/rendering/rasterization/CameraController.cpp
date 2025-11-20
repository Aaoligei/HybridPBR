#include "CameraController.h"
#include <GLFW/glfw3.h>

namespace HybridPBR {

    CameraController::CameraController(Camera* cam) : camera(cam) {
    }

    void CameraController::Update(float deltaTime) {
        // 相机控制器更新逻辑（如果需要）
    }

    void CameraController::OnMouseMove(double xpos, double ypos) {
        if (firstMouse) {
            lastMouseX = xpos;
            lastMouseY = ypos;
            firstMouse = false;
        }

        if (mouseMiddlePressed) {
            // 鼠标中键按下时进行平移操作
            ProcessMousePan(xpos - lastMouseX, lastMouseY - ypos);
        } 
        else if (mouseRightPressed) {
            // 鼠标右键按下时进行旋转操作
            ProcessMouseRotate(xpos - lastMouseX, lastMouseY - ypos);
        }

        lastMouseX = xpos;
        lastMouseY = ypos;
    }

    void CameraController::OnMouseButton(int button, int action, int mods) {
        bool pressed = (action == GLFW_PRESS);

        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            mouseLeftPressed = pressed;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            mouseRightPressed = pressed;
            if (pressed) {
                firstMouse = true;
            }
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            mouseMiddlePressed = pressed;
            if (pressed) {
                firstMouse = true;
            }
            break;
        }
    }

    void CameraController::OnMouseScroll(double xoffset, double yoffset) {
        ProcessMouseZoom(yoffset);
    }

    void CameraController::ProcessMouseMovement(double xpos, double ypos) {
        // 鼠标移动处理
    }

    void CameraController::ProcessMousePan(double xoffset, double yoffset) {
        if (!camera) return;

        float deltaX = static_cast<float>(xoffset) * mouseSensitivity;
        float deltaY = static_cast<float>(yoffset) * mouseSensitivity;

        // 根据相机距离调整平移速度
        float distanceFactor = glm::length(camera->GetPosition()) * 0.01f + 0.01f;
        
        camera->Translate(
            -(camera->GetRight() * deltaX * distanceFactor) +
            -(camera->GetUp() * deltaY * distanceFactor)
        );
    }

    void CameraController::ProcessMouseRotate(double xoffset, double yoffset) {
        if (!camera) return;

        float deltaX = static_cast<float>(xoffset) * mouseSensitivity;
        float deltaY = static_cast<float>(yoffset) * mouseSensitivity;

        camera->Rotate(deltaX, deltaY);
    }

    void CameraController::ProcessMouseZoom(double yoffset) {
        if (!camera) return;

        float zoomFactor = static_cast<float>(yoffset) * zoomSensitivity;
        
        // 修复：使用对数缩放保持速度一致性
        float distance = glm::max(glm::length(camera->GetPosition()), 1.0f);
        float logDistance = log10f(distance);
        
        camera->Translate(camera->GetFront() * zoomFactor * (logDistance + 1.0f));
    }

}