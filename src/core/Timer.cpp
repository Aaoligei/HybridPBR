#include "Timer.h"
#include "utils/Logger.h"
#include <cmath> 

namespace HybridPBR {
    
    Timer::Timer() {
        Reset();
    }
    
    void Timer::Reset() {
        startTime = Clock::now();
        lastFrameTime = startTime;
    }
    
    float Timer::Elapsed() const {
        auto now = Clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
        return duration.count() / 1000000.0f;
    }
    
    float Timer::ElapsedMillis() const {
        return Elapsed() * 1000.0f;
    }
    
    void Timer::Tick() {
        auto currentTime = Clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastFrameTime);
        deltaTime = frameDuration.count() / 1000000.0f;
        lastFrameTime = currentTime;
        
        // 更新FPS
        frameCount++;
        fpsUpdateTime += deltaTime;
        if (fpsUpdateTime >= 1.0f) {
            fps = frameCount / fpsUpdateTime;
            frameCount = 0;
            fpsUpdateTime = 0.0f;
            
        }
    }

} // namespace HybridPBR