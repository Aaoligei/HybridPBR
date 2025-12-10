#version 460 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec3 ViewPos;
    vec3 WorldPos;
} fs_in;

struct Light {
    vec3 position;  float pad0;
    vec3 direction; float pad1;
    vec3 color;     float intensity;
    float range;    float constant;
    float linear;   float quadratic;
    float innerCutoff; float outerCutoff;
    int type;       // 0=Directional, 1=Point, 2=Spot
    float pad2; float pad3; float pad4;
};

layout (std140, binding = 1) uniform LightData {
    int lightCount;
    int pad0, pad1, pad2;
    Light lights[16];
};

const float PI = 3.14159265359;

void main()
{       
    vec3 N = normalize(fs_in.Normal);
    vec3 V = normalize(fs_in.ViewPos - fs_in.WorldPos);
    
    // 默认材质参数 (银灰色金属)
    vec3 albedo = vec3(0.8, 0.8, 0.8); 
    float roughness = 0.4;
    float metallic = 0.0; // 先设为非金属，更容易看清光照

    vec3 Lo = vec3(0.0);

    // 调试：如果没有光源，显示红色警告
    if (lightCount == 0) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    for(int i = 0; i < lightCount; ++i) 
    {
        vec3 L;
        float attenuation = 1.0;

        // --- 核心修复：正确处理光照类型 ---
        if (lights[i].type == 0) { 
            // 0 = Directional Light (方向光)
            // 方向光的方向是从光源发出的，计算 L (指向光源) 需要取反
            L = normalize(-lights[i].direction);
        } 
        else { 
            // 1 = Point Light (点光源)
            vec3 distVec = lights[i].position - fs_in.WorldPos;
            float distance = length(distVec);
            L = normalize(distVec);
            
            // 简单衰减
            if (lights[i].range > 0.0) {
                attenuation = 1.0 / (distance * distance);
            }
        }

        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        
        // 简单 Blinn-Phong 用于测试 (确保不是 PBR 公式的问题)
        // 只要 NdotL > 0，这里就必须亮！
        vec3 radiance = lights[i].color * lights[i].intensity * attenuation;
        
        // 简单的漫反射 + 高光
        vec3 diffuse = albedo / PI;
        float spec = pow(max(dot(N, H), 0.0), 32.0); // 硬编码高光
        
        Lo += (diffuse + vec3(spec)) * radiance * NdotL; 
    }   
    
    // 环境光 (防止死黑)
    vec3 ambient = vec3(0.05) * albedo;
    vec3 color = ambient + Lo;
    
    // Tone mapping (Reinhard)
    // 注意：ToneMapping 会把超亮的颜色压回 1.0 (白色)
    // 如果你想看到 "调节强度" 的效果，可以暂时注释掉下面这行
    //color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
    //FragColor = vec4(fs_in.Normal, 1.0); // 用法线调试
}