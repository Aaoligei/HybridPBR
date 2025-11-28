#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include<vector>
#include <memory>

namespace HybridPBR {

    enum class TextureType {
        DIFFUSE,
        SPECULAR,
        NORMAL,
        HEIGHT,
        ROUGHNESS,
        METALLIC,
        AMBIENT_OCCLUSION,
        EMISSIVE,
        HDR,
        CUBEMAP
    };

    enum class TextureWrap {
        REPEAT = GL_REPEAT,
        MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
        CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
        CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER
    };

    enum class TextureFilter {
        NEAREST = GL_NEAREST,
        LINEAR = GL_LINEAR,
        NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
        LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
        NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
        LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
    };

    class Texture {
    public:
        Texture();
        ~Texture();
        
        // 2D纹理创建
        bool Create2D(int width, int height, GLenum internalFormat = GL_RGBA8, 
                     GLenum format = GL_RGBA, GLenum dataType = GL_UNSIGNED_BYTE,
                     const void* data = nullptr);
        
        // 从文件加载
        bool LoadFromFile(const std::string& filepath, TextureType type = TextureType::DIFFUSE);
        bool LoadHDR(const std::string& filepath);
        
        // 立方体贴图
        bool CreateCubemap(int size, GLenum internalFormat = GL_RGB16F);
        bool LoadCubemap(const std::vector<std::string>& faces);
        
        // 参数设置
        void SetWrapMode(TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR = TextureWrap::REPEAT);
        void SetFilter(TextureFilter minFilter, TextureFilter magFilter);
        void GenerateMipmaps();
        
        // 绑定
        void BindImage(uint32_t unit, uint32_t level, GLenum access) const;
        void Bind(uint32_t unit = 0) const;
        void Unbind() const;
        
        // 获取信息
        uint32_t GetID() const { return textureID; }
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        TextureType GetType() const { return type; }
        bool GetIsCubeMap() const { return isCubemap; }
        const std::string& GetFilePath() const { return filePath; }
        GLenum GetInternalFormat() const { return m_internalFormat; }
        GLenum GetFormat() const { return m_format; }
        
        // 工具函数
        static GLenum GetGLInternalFormat(GLenum format, bool sRGB = false);
        static GLenum GetGLFormat(int channels);

    private:
        uint32_t textureID = 0;
        GLenum m_internalFormat=0; // 内部格式
        GLenum m_format=0; // 数据格式
        int width = 0;
        int height = 0;
        TextureType type = TextureType::DIFFUSE;
        std::string filePath;
        bool isCubemap = false;
        
        bool LoadImageData(const std::string& filepath, int& width, int& height, 
                          int& channels, void** data, bool flipY = true);
        void FreeImageData(void* data);
    };

} // namespace HybridPBR