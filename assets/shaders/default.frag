#version 460 core

// 从顶点着色器接收的数据
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// 输出颜色
out vec4 FragColor;

// 材质属性
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    
    bool useDiffuseMap;
    bool useSpecularMap;
};

// 定向光源
struct DirLight {
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// 点光源
struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// 聚光灯
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// Uniforms
uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[4];
uniform SpotLight spotLight;
uniform int pointLightCount;

// 纹理
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

// 函数声明
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    // 属性
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 获取材质颜色
    vec3 diffuseTexel = vec3(1.0);
    vec3 specularTexel = vec3(1.0);
    
    if (material.useDiffuseMap) {
        diffuseTexel = vec3(texture(diffuseMap, TexCoord));
    }
    
    if (material.useSpecularMap) {
        specularTexel = vec3(texture(specularMap, TexCoord));
    }
    
    // 定向光
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    
    // 点光源
    for(int i = 0; i < pointLightCount && i < 4; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }
    
    // 聚光
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    
    // 应用材质颜色
    vec3 ambient = material.ambient * diffuseTexel;
    vec3 diffuse = result * material.diffuse * diffuseTexel;
    vec3 specular = result * material.specular * specularTexel;
    
    vec3 finalColor = ambient + diffuse + specular;
    FragColor = vec4(finalColor, 1.0);
}

// 计算定向光
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // 合并结果
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    vec3 specular = light.specular * spec;
    
    return (ambient + diffuse + specular);
}

// 计算点光源
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // 衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));
    
    // 合并结果
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    vec3 specular = light.specular * spec;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

// 计算聚光
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // 衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));
    
    // 聚光强度
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // 合并结果
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    vec3 specular = light.specular * spec;
    
    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}