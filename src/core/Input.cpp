#include "Input.h"

namespace HybridPBR {
    
    Input& Input::GetInstance() {
        static Input instance;
        return instance;
    }
    
    void Input::Update() {
        // 保存上一帧的状态
        prevKeyStates = keyStates;
        prevMouseStates = mouseStates;
        
        // 重置鼠标delta
        mouseDeltaX = 0.0;
        mouseDeltaY = 0.0;
    }
    
    bool Input::IsKeyPressed(int keyCode) {
        return keyStates[keyCode] && !prevKeyStates[keyCode];
    }
    
    bool Input::IsKeyReleased(int keyCode) {
        return !keyStates[keyCode] && prevKeyStates[keyCode];
    }
    
    bool Input::IsKeyHeld(int keyCode) {
        return keyStates[keyCode];
    }
    
    bool Input::IsMouseButtonPressed(int button) {
        return mouseStates[button] && !prevMouseStates[button];
    }
    
    bool Input::IsMouseButtonReleased(int button) {
        return !mouseStates[button] && prevMouseStates[button];
    }
    
    bool Input::IsMouseButtonHeld(int button) {
        return mouseStates[button];
    }
    
    void Input::SetMousePosition(double x, double y) {
        prevMouseX = x;
        prevMouseY = y;
        mouseX = x;
        mouseY = y;
    }
    
    void Input::SetKeyState(int key, bool pressed) {
        keyStates[key] = pressed;
    }
    
    void Input::SetMouseButtonState(int button, bool pressed) {
        mouseStates[button] = pressed;
    }
    
    void Input::SetMousePositionInternal(double x, double y) {
        prevMouseX = mouseX;
        prevMouseY = mouseY;
        mouseX = x;
        mouseY = y;
        mouseDeltaX = mouseX - prevMouseX;
        mouseDeltaY = mouseY - prevMouseY;
    }

} // namespace HybridPBR