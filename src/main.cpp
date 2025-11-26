#include "DefferedApplication.h"

int main() {
    ThreeDApp app;
 
    if (app.Initialize()) {
        app.Run();
    }
    
    app.Shutdown();
    return 0;
}