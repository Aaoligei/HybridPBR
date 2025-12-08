#include "core/Application.h"
#include "utils/Logger.h"
#include "DefferedApplication.h"

using namespace HybridPBR;

int main() {
    ThreeDApp app;
    
    if (app.Initialize().IsFailure()) {
        LOG_ERROR("Main", "Failed to initialize application");
        return -1;
    }
    
    if (app.Run().IsFailure()) {
        LOG_ERROR("Main", "Application run failed");
        return -1;
    }
    
    return 0;
}
