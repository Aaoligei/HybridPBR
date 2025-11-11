#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>

namespace HybridPBR {
    
    class Input {
    public:
        static Input& GetInstance();
        
        void Update();
        
        // 键盘输入
        bool IsKeyPressed(int keyCode);
        bool IsKeyReleased(int keyCode);
        bool IsKeyHeld(int keyCode);
        
        // 鼠标输入
        bool IsMouseButtonPressed(int button);
        double GetMouseX() const { return mouseX; }
        double GetMouseY() const { return mouseY; }
        double GetMouseDeltaX() const { return mouseDeltaX; }
        double GetMouseDeltaY() const { return mouseDeltaY; }
        
        // 设置鼠标位置（用于重置delta）
        void SetMousePosition(double x, double y);

    private:
        Input() = default;
        
        std::unordered_map<int, bool> keyStates;
        std::unordered_map<int, bool> prevKeyStates;
        std::unordered_map<int, bool> mouseStates;
        std::unordered_map<int, bool> prevMouseStates;
        
        double mouseX = 0.0;
        double mouseY = 0.0;
        double prevMouseX = 0.0;
        double prevMouseY = 0.0;
        double mouseDeltaX = 0.0;
        double mouseDeltaY = 0.0;
        
        friend class Window; // Window类可以更新输入状态
        void SetKeyState(int key, bool pressed);
        void SetMouseButtonState(int button, bool pressed);
        void SetMousePositionInternal(double x, double y);
    };

} // namespace HybridPBR