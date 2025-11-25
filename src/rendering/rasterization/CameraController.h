#pragma once

#include "Camera.h"
#include <glm/glm.hpp>

namespace HybridPBR {

    class CameraController {
    public:
        CameraController(Camera* camera);

        void Update(float deltaTime);
        
        // 鼠标事件处理
        void OnMouseMove(double xpos, double ypos);
        void OnMouseButton(int button, int action, int mods);
        void OnMouseScroll(double xoffset, double yoffset);

        // 设置灵敏度参数
        void SetMovementSpeed(float speed) { movementSpeed = speed; }
        void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
        void SetZoomSensitivity(float sensitivity) { zoomSensitivity = sensitivity; }

        float GetMovementSpeed() const { return movementSpeed; }
        float GetMouseSensitivity() const { return mouseSensitivity; }
        float GetZoomSensitivity() const { return zoomSensitivity; }
        Camera* GetCamera() const { return camera; }

    private:
        Camera* camera;

        // 鼠标控制相关
        bool mouseLeftPressed = false;
        bool mouseRightPressed = false;
        bool mouseMiddlePressed = false;
        
        double lastMouseX = 0.0;
        double lastMouseY = 0.0;
        bool firstMouse = true;

        // 控制参数
        float movementSpeed = 5.0f;
        float mouseSensitivity = 0.1f;
        float zoomSensitivity = 0.1f;

        // 相机操作函数
        void ProcessMouseMovement(double xpos, double ypos);
        void ProcessMousePan(double xoffset, double yoffset);
        void ProcessMouseRotate(double xoffset, double yoffset);
        void ProcessMouseZoom(double yoffset);
    };

}