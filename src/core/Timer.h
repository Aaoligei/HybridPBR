#pragma once
#include <chrono>

namespace HybridPBR {
    
    class Timer {
    public:
        Timer();
        
        void Reset();
        float Elapsed() const;  // 返回秒数
        float ElapsedMillis() const; // 返回毫秒数
        
        // 帧计时
        void Tick();
        float GetDeltaTime() const { return deltaTime; }
        float GetFPS() const { return fps; }

    private:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;
        
        TimePoint startTime;
        TimePoint lastFrameTime;
        float deltaTime = 0.0f;
        float fps = 0.0f;
        int frameCount = 0;
        float fpsUpdateTime = 0.0f;
    };

} // namespace HybridPBR