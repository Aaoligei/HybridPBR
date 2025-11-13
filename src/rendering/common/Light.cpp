#include "Light.h"

namespace HybridPBR {

    Light::Light(LightType lightType, const std::string& lightName)
        : type(lightType), name(lightName) {
    }

} // namespace HybridPBR